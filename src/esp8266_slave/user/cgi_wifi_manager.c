
#include "cgi_wifi_manager.h"
#include <esp8266.h>

#include "esplogger.h"
#include <user_interface.h>
#include "json/jsonparse.h"
#include "json/jsontree.h"
#include "json/json.h"
#include "user_json.h"


#include "wifi_manager.h"
#include "wifi_list.h"


static enum {
	S_OK = 0,
	S_CON = 1,
	S_NEW = 2,
	S_ERR = 4
} statesEnum;


typedef struct {
	char ssid[32];
	char bssid[8];
	int channel;
	char rssi;
	char enc;
} ApData;

typedef struct {
	bool everScanned;
	bool scanInProgress; //if 1, don't access the underlying stuff from the webpage.
	ApData **apData;
	int noAps;
} ScanResultData;

static ScanResultData cgiWifiAps = { false, false, NULL, 0 };//FIXME SET TO NULL


static void ICACHE_FLASH_ATTR wifiScanDoneCb(void *arg, STATUS status) {
	int n;
	struct bss_info *bss_link = (struct bss_info *)arg;
	httpd_printf("wifiScanDoneCb %d\n", status);
	if (status != OK) {
		cgiWifiAps.scanInProgress = 0;
		return;
	}

	//Clear prev ap data if needed.
	if (cgiWifiAps.apData != NULL) {
		for (n = 0; n < cgiWifiAps.noAps; n++) free(cgiWifiAps.apData[n]);
		free(cgiWifiAps.apData);
	}

	//Count amount of access points found.
	n = 0;
	while (bss_link != NULL) {
		bss_link = bss_link->next.stqe_next;
		n++;
	}
	//Allocate memory for access point data
	cgiWifiAps.apData = (ApData **)malloc(sizeof(ApData *)*n);
	if (cgiWifiAps.apData == NULL) {
		printf("Out of memory allocating apData\n");
		return;
	}
	cgiWifiAps.noAps = n;
	httpd_printf("Scan done: found %d APs\n", n);

	//Copy access point data to the static struct
	n = 0;
	bss_link = (struct bss_info *)arg;
	while (bss_link != NULL) {
		if (n >= cgiWifiAps.noAps) {
			//This means the bss_link changed under our nose. Shouldn't happen!
			//Break because otherwise we will write in unallocated memory.
			httpd_printf("Huh? I have more than the allocated %d aps!\n", cgiWifiAps.noAps);
			break;
		}
		//Save the ap data.
		cgiWifiAps.apData[n] = (ApData *)malloc(sizeof(ApData));
		if (cgiWifiAps.apData[n] == NULL) {
			httpd_printf("Can't allocate mem for ap buff.\n");
			cgiWifiAps.scanInProgress = 0;
			return;
		}
		cgiWifiAps.apData[n]->rssi = bss_link->rssi;
		cgiWifiAps.apData[n]->channel = bss_link->channel;
		cgiWifiAps.apData[n]->enc = bss_link->authmode;
		strncpy(cgiWifiAps.apData[n]->ssid, (char*)bss_link->ssid, 32);
		strncpy(cgiWifiAps.apData[n]->bssid, (char*)bss_link->bssid, 6);

		bss_link = bss_link->next.stqe_next;
		n++;
	}
	//We're done.
	cgiWifiAps.scanInProgress = 0;
}


//Routine to start a WiFi access point scan.
static void ICACHE_FLASH_ATTR wifiStartScan() {
	//	int x;
	if (cgiWifiAps.scanInProgress) return;
	cgiWifiAps.scanInProgress = 1;
	wifi_station_scan(NULL, wifiScanDoneCb);
}

int ICACHE_FLASH_ATTR www_wifi_scan(HttpdConnData *connData) {
	int pos = (int)connData->cgiData;
	int len;
	char buff[1024];

	if (!cgiWifiAps.scanInProgress && pos != 0) {
		//Fill in json code for an access point
		if (pos - 1 < cgiWifiAps.noAps) {
			int state = S_OK;

			//state |= S_ERR;
			if (!wifi_list_ssid_exist(cgiWifiAps.apData[pos - 1]->ssid)) {
				state |= S_NEW;
			}

			const char* actualSSID = wifi_manager_get_station_ssid();
			if (actualSSID != NULL && os_strncmp(cgiWifiAps.apData[pos - 1]->ssid, actualSSID, 32) == 0) {
				state |= S_CON;
			}

			len = sprintf(buff, "{\"ssid\": \"%s\", \"bssid\": \"" MACSTR "\", \"rssi\": %d, \"authmode\": %d, \"channel\": %d, \"state\": %d}%s\n",
				cgiWifiAps.apData[pos - 1]->ssid, MAC2STR(cgiWifiAps.apData[pos - 1]->bssid), cgiWifiAps.apData[pos - 1]->rssi,
				cgiWifiAps.apData[pos - 1]->enc, cgiWifiAps.apData[pos - 1]->channel, state, (pos - 1 == cgiWifiAps.noAps - 1) ? "" : ",");
			httpdSend(connData, buff, len);
		}
		pos++;
		if ((pos - 1) >= cgiWifiAps.noAps) {
			len = sprintf(buff, "]\n}\n");
			httpdSend(connData, buff, len);
			//Also start a new scan.
			wifiStartScan();
			return HTTPD_CGI_DONE;
		}
		else {
			connData->cgiData = (void*)pos;
			return HTTPD_CGI_MORE;
		}
	}

	httpdStartResponse(connData, 200);
	httpdHeader(connData, "Content-Type", "text/json");
	httpdEndHeaders(connData);

	if (cgiWifiAps.everScanned == false) {
		//first scan
		len = sprintf(buff, "{\n \"ready\": false\n}\n");
		httpdSend(connData, buff, len);

		cgiWifiAps.everScanned = true;
		wifiStartScan();

		return HTTPD_CGI_DONE;
	}
	if (cgiWifiAps.scanInProgress == 1) {
		//We're still scanning. Tell Javascript code that.
		len = sprintf(buff, "{\n \"ready\": false\n}\n");
		httpdSend(connData, buff, len);
		return HTTPD_CGI_DONE;
	}
	else {
		//We have a scan result. Pass it on.
		len = sprintf(buff, "{\n\"ready\": true,\n\"result\": [\n");
		httpdSend(connData, buff, len);
		if (cgiWifiAps.apData == NULL) cgiWifiAps.noAps = 0;
		connData->cgiData = (void *)1;
		return HTTPD_CGI_MORE;
	}
}




int ICACHE_FLASH_ATTR www_wifi_list(HttpdConnData *connData)
{
	struct wifi_auth_node* listNode = (struct wifi_auth_node*)connData->cgiData;
	int len;
	char buff[1024];

	if (listNode != NULL) {//iteration started
		struct wifi_auth_node* nextNode = listNode->next;
		int state = S_OK;

		const char* actualSSID = wifi_manager_get_station_ssid();
		if (actualSSID != NULL && os_strncmp(listNode->ssid, actualSSID, 32) == 0) {
			state |= S_CON;
		}

		char loginBuff[35];
		if (listNode->login[0] == 0){
			memcpy(loginBuff, "null", 5);
		}
		else {
			sprintf(loginBuff, "\"%s\"", listNode->login);
		}

		len = sprintf(buff, "{\"ssid\": \"%s\", \"login\": %s, \"state\": %d}%s\n",
			listNode->ssid, loginBuff, state, nextNode == NULL ? "" : ",");
		httpdSend(connData, buff, len);


		if (nextNode == NULL) {
			len = sprintf(buff, "]\n}");
			httpdSend(connData, buff, len);
			//Also start a new scan.
			wifiStartScan();
			return HTTPD_CGI_DONE;
		}

		connData->cgiData = (void*)nextNode;
		return HTTPD_CGI_MORE;

	}

	httpdStartResponse(connData, 200);
	httpdHeader(connData, "Content-Type", "text/json");
	httpdEndHeaders(connData);

	struct wifi_auth_node* rootNode = wifi_list_get_root();
	if (rootNode == NULL) {
		len = sprintf(buff, "{\n\"result\": []\n}");
		httpdSend(connData, buff, len);
		return HTTPD_CGI_DONE;
	}

	//>0
	len = sprintf(buff, "{\n\"result\": [\n");
	httpdSend(connData, buff, len);
	connData->cgiData = (void *)rootNode;
	return HTTPD_CGI_MORE;
}





int ICACHE_FLASH_ATTR www_wifi_summary(HttpdConnData *connData)
{
	int len;
	char buff[1024];

	const char* ssid = wifi_manager_get_station_ssid();
	char ssidBuff[35];
	if (ssid == NULL) {
		memcpy(ssidBuff, "null", 5);
	}
	else {
		sprintf(ssidBuff, "\"%s\"", ssid);
	}

	httpdStartResponse(connData, 200);
	httpdHeader(connData, "Content-Type", "text/json");
	httpdEndHeaders(connData);
	len = sprintf(buff, "{\n\"ssid\": %s\n}", ssidBuff);
	httpdSend(connData, buff, len);
	return HTTPD_CGI_DONE;
}



struct station_info_set
{
	char ssid[32];
	char login[32];
	bool loginSet;
	char password[64];
	bool passwordSet;
};
struct station_info_set* info_buffer = NULL;

static int ICACHE_FLASH_ATTR ws_message_set(struct jsontree_context *js_ctx, struct jsonparse_state *parser)
{
	log_info("station_info_set");
	int type;
	//uint8 station_tree;

	os_bzero(info_buffer->ssid, 32);
	os_bzero(info_buffer->login, 32);
	os_bzero(info_buffer->password, 64);
	info_buffer->loginSet = false;
	info_buffer->passwordSet = false;

	while ((type = jsonparse_next(parser)) != 0) {
		if (type == JSON_TYPE_PAIR_NAME) {

			if (jsonparse_strcmp_value(parser, "ssid") == 0) {
				jsonparse_next(parser);
				jsonparse_next(parser);
				jsonparse_copy_value(parser, info_buffer->ssid, sizeof(info_buffer->ssid));
				//os_memcpy(new_ssid, buffer, os_strlen(buffer));
			}
			else if (jsonparse_strcmp_value(parser, "login") == 0) {
				jsonparse_next(parser);
				jsonparse_next(parser);
				jsonparse_copy_value(parser, info_buffer->login, sizeof(info_buffer->login));
				info_buffer->loginSet = true;
				//os_memcpy(new_password, buffer, os_strlen(buffer));
			}
			else if (jsonparse_strcmp_value(parser, "password") == 0) {
				jsonparse_next(parser);
				jsonparse_next(parser);
				jsonparse_copy_value(parser, info_buffer->password, sizeof(info_buffer->password));
				info_buffer->passwordSet = true;
				//os_memcpy(new_password, buffer, os_strlen(buffer));
			}
		}
	}

	return 0;
}

static struct jsontree_callback ws_message_info_callback = JSONTREE_CALLBACK(NULL, ws_message_set);

JSONTREE_OBJECT(ws_message_info_tree,
	JSONTREE_PAIR("ssid", &ws_message_info_callback),
	JSONTREE_PAIR("login", &ws_message_info_callback),
	JSONTREE_PAIR("password", &ws_message_info_callback),
	JSONTREE_PAIR("error", &ws_message_info_callback),
	JSONTREE_PAIR("state", &ws_message_info_callback)
);

JSONTREE_OBJECT(ws_message_info_root,
	JSONTREE_PAIR("root", &ws_message_info_tree));



int ICACHE_FLASH_ATTR www_wifi_station_info(HttpdConnData *connData)
{
	int len;
	char buff[1024];

	struct station_info_set info_buffer_local;
	info_buffer = &info_buffer_local;

	struct jsontree_context js;
	jsontree_setup(&js, (struct jsontree_value *)&ws_message_info_root, json_putchar);
	json_parse(&js, connData->post->buff);

	info_buffer = NULL;


	char* ssid = info_buffer_local.ssid;

	bool exist = wifi_list_ssid_exist(ssid);

	if (!exist) {
		len = sprintf(buff, "{\n\"exist\": false\n}");
		httpdSend(connData, buff, len);
		return HTTPD_CGI_DONE;
	}

	const char* login = wifi_list_get_login(ssid);
	const char* password = wifi_list_get_password(ssid);

	char loginBuff[35];
	if (login == NULL) {
		memcpy(loginBuff, "null", 5);
	}
	else {
		sprintf(loginBuff, "\"%s\"", login);
	}

	char passwordBuff[67];
	if (password != NULL) {//mask password by * char
		os_bzero(passwordBuff, 67);
		memset(passwordBuff + 1, '*', strlen(password));
		passwordBuff[0] = '"';
		passwordBuff[strlen(password) + 1] = '"';
	}
	else {
		memcpy(passwordBuff, "null", 5);
	}

	int state = S_OK;
	const char* actualSSID = wifi_manager_get_station_ssid();
	if (actualSSID != NULL && os_strncmp(ssid, actualSSID, 32) == 0) {
		state |= S_CON;
	}


	char* errorBuff = "null";//or ["error1", "error2"]

	httpdStartResponse(connData, 200);
	httpdHeader(connData, "Content-Type", "text/json");
	httpdEndHeaders(connData);
	len = sprintf(buff, "{\"exist\": true,\n\"ssid\": \"%s\",\n\"login\": %s,\n\"password\": %s,\n\"error\": %s,\n\"state\": %d}",
		ssid, loginBuff, passwordBuff, errorBuff, state),
		httpdSend(connData, buff, len);

	return HTTPD_CGI_DONE;
}



int ICACHE_FLASH_ATTR www_wifi_let_connect(HttpdConnData *connData)
{
	int len;
	char buff[1024];
	log_info("let_connect");

	struct station_info_set info_buffer_local;
	info_buffer = &info_buffer_local;

	struct jsontree_context js;
	jsontree_setup(&js, (struct jsontree_value *)&ws_message_info_root, json_putchar);
	json_parse(&js, connData->post->buff);

	log_info("let_connect: %s", info_buffer->ssid);

	bool result = wifi_manager_station_connect(info_buffer->ssid);

	if (result) {
		len = sprintf(buff, "{\"status\":\"OK\"}");
	}
	else {
		len = sprintf(buff, "{\"status\":\"ERROR\"}");
	}


	httpdStartResponse(connData, 200);
	httpdHeader(connData, "Content-Type", "text/json");
	httpdEndHeaders(connData);
	httpdSend(connData, buff, len);

	info_buffer = NULL;
	return HTTPD_CGI_DONE;
}


int ICACHE_FLASH_ATTR www_wifi_let_delete(HttpdConnData *connData)
{
	int len;
	char buff[1024];
	log_info("let_delete");

	struct station_info_set info_buffer_local;
	info_buffer = &info_buffer_local;

	struct jsontree_context js;
	jsontree_setup(&js, (struct jsontree_value *)&ws_message_info_root, json_putchar);
	json_parse(&js, connData->post->buff);

	len = sprintf(buff, "{\"status\":\"OK\"}");

	httpdStartResponse(connData, 200);
	httpdHeader(connData, "Content-Type", "text/json");
	httpdEndHeaders(connData);
	httpdSend(connData, buff, len);

	wifi_list_delete(info_buffer->ssid);
	wifi_list_save();

	wifi_manager_station_disconnect(info_buffer->ssid);

	info_buffer = NULL;

	return HTTPD_CGI_DONE;
}



int www_wifi_let_save(HttpdConnData *connData)
{
	int len;
	char buff[1024];
	log_info("www_wifi_let_save");

	struct station_info_set info_buffer_local;
	info_buffer = &info_buffer_local;

	struct jsontree_context js;
	jsontree_setup(&js, (struct jsontree_value *)&ws_message_info_root, json_putchar);
	json_parse(&js, connData->post->buff);

	len = sprintf(buff, "{\"status\":\"OK\"}");

	httpdStartResponse(connData, 200);
	httpdHeader(connData, "Content-Type", "text/json");
	httpdEndHeaders(connData);
	httpdSend(connData, buff, len);

	char* login = info_buffer->loginSet ? info_buffer->login : NULL;
	log_info("%s %d", info_buffer->password, info_buffer->passwordSet);
	const char* password = info_buffer->passwordSet ? (strlen(info_buffer->password) == 0 ? wifi_list_get_password(info_buffer->ssid) : info_buffer->password) : NULL;

	log_info("%s %d %s", info_buffer->password, info_buffer->passwordSet, password);

	wifi_list_add(info_buffer->ssid, login, password);
	wifi_list_save();

	info_buffer = NULL;
	return HTTPD_CGI_DONE;
}



