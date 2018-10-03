
#include "wifi_manager.h"

#include "wifi_list.h"
#include <user_interface.h>
#include <osapi.h>
#include "esplogger.h"
#include "esplib.h"
#include "wpa2_enterprise.h"


static void rescan_wifi_after_cb(void *arg);
static void rescan_wifi_after(int ms);
static void rescan_wifi();
static void disconnect_wifi_after_cb(void *arg);
static void disconnect_wifi_after(int ms);
static void disconnect_wifi();
static void wifi_scan_cb(void *arg, STATUS status);
static void wifi_event_cb(System_Event_t *event);
static void connect(char* ssid, char* login, char* password, uint8* bssid);

static char _connectedSSID[32];
static bool _internetConnection = false;
static bool _started = false;
static char _toConnectSSID[32];

static os_timer_t   _mainTimer;
static os_timer_t   _disconnectTimer;


//TODO exclude dynamic int array

static struct ignoredStationList {
	uint8 bssid[6];
	struct ignoredStationList* next;
} ignoredStationListStruct;

static struct ignoredStationList* _ignoredStations = NULL;

static void ICACHE_FLASH_ATTR ignored_station_add(uint8 bssid[6]) {
	struct ignoredStationList* station = (struct ignoredStationList*)malloc(sizeof(struct ignoredStationList));
	memcpy(station->bssid, bssid, sizeof(uint8) * 6);
	station->next = NULL;

	if (_ignoredStations == NULL) {
		_ignoredStations = station;
		return;
	}

	struct ignoredStationList* ignoredParent = _ignoredStations;

	while (ignoredParent->next != NULL) {
		ignoredParent = ignoredParent->next;
	}

	ignoredParent->next = station;
}

static void ICACHE_FLASH_ATTR ignored_station_clear() {
	struct ignoredStationList* station = _ignoredStations;
	struct ignoredStationList* tmp;

	while (station != NULL) {
		tmp = station;
		station = station->next;
		free(tmp);
	}

	_ignoredStations = NULL;
}
static bool ICACHE_FLASH_ATTR ignored_station_exist(uint8 bssid[6]) {

	struct ignoredStationList* station = _ignoredStations;
	struct ignoredStationList* tmp;

	while (station != NULL) {
		if (memcmp(station->bssid, bssid, sizeof(uint8) * 6) == 0)
			return true;

		station = station->next;
	}

	return false;
}



static void ICACHE_FLASH_ATTR disconnect_wifi_after_cb(void *arg)
{
	disconnect_wifi();
}

static void ICACHE_FLASH_ATTR disconnect_wifi_after(int ms) {
	os_timer_disarm(&_disconnectTimer);
	os_timer_setfn(&_disconnectTimer, (os_timer_func_t *)disconnect_wifi_after_cb, NULL);
	os_timer_arm(&_disconnectTimer, ms, false);
}

static void ICACHE_FLASH_ATTR disconnect_wifi() {
	wifi_station_disconnect();
}


//todo list of wifi online

static void ICACHE_FLASH_ATTR rescan_wifi_after_cb(void *arg)
{
	rescan_wifi();
}

static void ICACHE_FLASH_ATTR rescan_wifi_after(int ms) {
	os_timer_disarm(&_mainTimer);
	os_timer_setfn(&_mainTimer, (os_timer_func_t *)rescan_wifi_after_cb, NULL);
	os_timer_arm(&_mainTimer, ms, false);
}

static void ICACHE_FLASH_ATTR rescan_wifi() {
	wifi_station_scan(NULL, wifi_scan_cb);
}


static void ICACHE_FLASH_ATTR wifi_scan_cb(void *arg, STATUS status)
{
	log_info("wifi_scan_cb, status: %d", status);

	if (status != OK)
	{
		log_error("scan error! status: %d", status);
		rescan_wifi_after(1000);
		return;
	}

	if (_internetConnection) {
		log_info("already connected");
		return;
	}

	struct bss_info *bssInfo = (struct bss_info *)arg;
	struct bss_info *selectedBss = NULL;
	// skip the first in the chain … it is invalid
	//bssInfo = STAILQ_NEXT(bssInfo, next);
	for (; bssInfo != NULL; bssInfo = STAILQ_NEXT(bssInfo, next))
	{
		log_info("ssid: %s\t%d dbm", bssInfo->ssid, bssInfo->rssi);

		if (!wifi_list_ssid_exist(bssInfo->ssid))
			continue;

		if (os_strncmp(_toConnectSSID, bssInfo->ssid, 32) == 0) {
			log_info("priority SSID! [%s]", bssInfo->ssid);
			selectedBss = bssInfo;
			_toConnectSSID[0] = 0;
			break;
		}

		if (ignored_station_exist(bssInfo->bssid))
			continue;

		if (selectedBss == NULL || selectedBss->rssi < bssInfo->rssi) { //first or better quality
			selectedBss = bssInfo;
		}
	}

	if (selectedBss == NULL)
	{
		log_warn("no known wifi AP\n");
		ignored_station_clear();

		rescan_wifi_after(5000);
		return;
	}

	//try connect
	char* password = wifi_list_get_password(selectedBss->ssid);
	char* login = wifi_list_get_login(selectedBss->ssid);

	connect(selectedBss->ssid, login, password, selectedBss->bssid);

	ignored_station_add(selectedBss->bssid);
}

//TODO add ping 8.8.8.8 to test internet access...
static void ICACHE_FLASH_ATTR wifi_event_cb(System_Event_t *event)
{
	switch (event->event)
	{
	case EVENT_STAMODE_CONNECTED:
		log_info("EVENT: EVENT_STAMODE_CONNECTED");
		memcpy(_connectedSSID, event->event_info.connected.ssid, 32);
		break;
	case EVENT_STAMODE_DISCONNECTED:
		_internetConnection = false;
		rescan_wifi_after(500);
		log_info("EVENT: EVENT_STAMODE_DISCONNECTED[%s] REASON: %d", event->event_info.disconnected.ssid, event->event_info.disconnected.reason);
		break;
	case EVENT_STAMODE_AUTHMODE_CHANGE:
		log_info("EVENT: EVENT_STAMODE_AUTHMODE_CHANGE");
		break;
	case EVENT_STAMODE_GOT_IP:
		log_info("EVENT: EVENT_STAMODE_GOT_IP");
		_internetConnection = true;
		break;
	case EVENT_SOFTAPMODE_STACONNECTED:
		log_info("EVENT: EVENT_SOFTAPMODE_STACONNECTED");
		break;
	case EVENT_SOFTAPMODE_STADISCONNECTED:
		log_info("EVENT: EVENT_SOFTAPMODE_STADISCONNECTED");
		break;
	case EVENT_SOFTAPMODE_PROBEREQRECVED:
		break;
	default:
		log_warn("Unexpected event: %d", event->event);
		break;
	}
}




//dodać task do zarządzania połączeniem wifi
void ICACHE_FLASH_ATTR wifi_manager_init()
{
	log_trace("init");

	wifi_set_event_handler_cb(wifi_event_cb);
	wifi_station_set_auto_connect(false); //nie chcemy auto łączenia

	_toConnectSSID[0] = 0;

	//wifi_station_set_hostname(ESP_STA_HOSTNAME)
}

void ICACHE_FLASH_ATTR wifi_manager_start()
{
	log_trace("wifi manager started");

	_started = true;

	uint8 mode = wifi_get_opmode();
	mode |= STATION_MODE;
	wifi_set_opmode(mode);

	rescan_wifi();
}

void ICACHE_FLASH_ATTR wifi_manager_stop()
{
	_started = false;

	uint8 mode = wifi_get_opmode();
	mode &= ~STATION_MODE;
	wifi_set_opmode(mode);

	disconnect_wifi_after(500);

}

bool ICACHE_FLASH_ATTR wifi_manager_station_connect(char* ssid)
{
	log_info("[%s]", ssid);
	
	if (!wifi_list_ssid_exist(ssid)) {
		return false;
	}

	
	os_bzero(_toConnectSSID, sizeof(_toConnectSSID));
	strncpy(_toConnectSSID, ssid, 32);
	//wifi_station_disconnect();
	disconnect_wifi_after(500);

	return true;
}

bool ICACHE_FLASH_ATTR wifi_manager_station_disconnect(char* ssid)
{
	if (!_internetConnection || strncmp(_connectedSSID, ssid, 32) != 0)
		return false;

	disconnect_wifi_after(500);

	return true;
}

const char* ICACHE_FLASH_ATTR wifi_manager_get_station_ssid()
{
	if (!_internetConnection)
		return NULL;

	return _connectedSSID;
}



bool ICACHE_FLASH_ATTR wifi_manager_is_internet_access()
{
	return _internetConnection;
}



static void ICACHE_FLASH_ATTR connect(char* ssid, char* login, char* password, uint8* bssid)
{
	log_info("connecting %s %s:%s", ssid, login, password);
	wifi_station_disconnect();


	if (!_started) {
		log_info("not started, connect abort");
		return;
	}

	struct station_config stationConfig;
	os_bzero(&stationConfig, sizeof(struct station_config));

	strncpy(stationConfig.ssid, ssid, 32);
	if (password != NULL) {
		strncpy(stationConfig.password, password, 64);
	}

	if (bssid) {
		stationConfig.bssid_set = 1;
		memcpy(stationConfig.bssid, bssid, sizeof(uint8) * 6);
	}

	wifi_station_set_config_current(&stationConfig);


	if (login != NULL && password != NULL) {
		wifi_station_set_wpa2_enterprise_auth(true);

		wifi_station_set_enterprise_username(login, os_strlen(login));
		wifi_station_set_enterprise_password(password, os_strlen(password));
		//wifi_station_set_enterprise_ca_cert(ca, os_strlen(ca)+1);//This is an option for EAP_PEAP and EAP_TTLS.
	}
	else {
		wifi_station_set_wpa2_enterprise_auth(false);
	}

	wifi_station_connect();
}
