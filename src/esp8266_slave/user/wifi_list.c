
#include "wifi_list.h"

#include <user_interface.h>
#include <mem.h>
#include <osapi.h>
#include <spi_flash.h>
#include "esplib.h"
#include "esplogger.h"


static struct wifi_auth_node* _listRoot = NULL;
//max 32 records(one sector)
//SPI_FLASH_SEC_SIZE / sizeof(struct wifi_auth_node_data) = 32
//4096 / 128 = 32
//at end we have empty record EOR
static uint32 _flashEntryCount = 31;

static struct wifi_auth_node* ICACHE_FLASH_ATTR create_node(char* ssid, char* login, char* password) {
	struct wifi_auth_node* node = (struct wifi_auth_node*)malloc(sizeof(struct wifi_auth_node));
	//TODO: malloc->zalloc && remove os_bzero

	os_bzero(node->ssid, 32);
	strncpy(node->ssid, ssid, 32);
	node->ssid[31] = 0;

	os_bzero(node->login, 32);
	if (login != NULL) {
		strncpy(node->login, login, 32);
		node->login[31] = 0;
	}

	os_bzero(node->password, 64);
	if (password != NULL) {
		strncpy(node->password, password, 32);
		node->password[63] = 0;
	}

	node->next = NULL;

	return node;
}

static struct wifi_auth_node* ICACHE_FLASH_ATTR search_node(char* ssid) {
	struct wifi_auth_node* tmp = _listRoot;

	while (tmp != NULL) {
		if (strncmp(tmp->ssid, ssid, 32) == 0)
			return tmp;
		tmp = tmp->next;
	}

	return NULL;
}


void ICACHE_FLASH_ATTR wifi_list_init() {
}

void ICACHE_FLASH_ATTR wifi_list_add(const char* ssid, const char* login, const char* password) {
	struct wifi_auth_node* node = search_node(ssid);
	if (node != NULL) {
		wifi_list_delete(ssid);
		//return;
	}

	node = create_node(ssid, login, password);

	if (_listRoot == NULL) {
		_listRoot = node;
	}
	else {
		struct wifi_auth_node* tmp = _listRoot;
		while (tmp->next != NULL) {
			tmp = tmp->next;
		}

		tmp->next = node;
	}
}

void ICACHE_FLASH_ATTR wifi_list_delete(const char* ssid) {
	struct wifi_auth_node* node = search_node(ssid);

	if (node == NULL)
		return;

	if (node == _listRoot) {
		_listRoot = node->next;
	}
	else {
		struct wifi_auth_node* parent = _listRoot;

		while (parent->next != node) {
			parent = parent->next;
		}

		parent->next = node->next;
	}

	free(node);
}

const char* ICACHE_FLASH_ATTR wifi_list_get_password(const char* ssid) {//fixme if ssid null
	struct wifi_auth_node* node = search_node(ssid);

	if (node == NULL)
		return NULL;

	if (node->password[0] == 0)
		return NULL;

	return node->password;
}

const char* ICACHE_FLASH_ATTR wifi_list_get_login(const char* ssid) {//fixme if ssid null
	struct wifi_auth_node* node = search_node(ssid);

	if (node == NULL)
		return NULL;

	if (node->login[0] == 0)
		return NULL;

	return node->login;
}

struct wifi_auth_node* ICACHE_FLASH_ATTR wifi_list_get_root()
{
	return _listRoot;
}

void ICACHE_FLASH_ATTR wifi_list_load()
{
	struct wifi_auth_node_data tmp;
	uint32 pos = 0;

	int i = 0;
	for (i = 0; i < _flashEntryCount; i++) {
		spi_flash_read(WIFI_LIST_CONFIG_ADDRESS + pos, (uint32 *)(&tmp), sizeof(struct wifi_auth_node_data));
		pos += sizeof(struct wifi_auth_node_data);
		
		if (tmp.ssid[0] == 0 || tmp.ssid[0] == 255)//blank record
			break;

		log_info("load. ssid: %d %s", tmp.ssid[0], tmp.ssid);
		wifi_list_add(tmp.ssid, tmp.login[0] == 0 ? NULL : tmp.login, tmp.password[0] == 0 ? NULL : tmp.password);
	}
}

void ICACHE_FLASH_ATTR wifi_list_save()
{
	spi_flash_erase_protect_enable();
	uint32 pos = 0;
	struct wifi_auth_node *tmp = NULL;

	spi_flash_erase_sector(WIFI_LIST_CONFIG_SECTOR);

	tmp = _listRoot;
	int i = 0;
	for (i = 0; i < _flashEntryCount; i++) {
		if (tmp == NULL)
			break;

		log_info("save: %s, size: %d", tmp->ssid, sizeof(struct wifi_auth_node_data));

		spi_flash_write(WIFI_LIST_CONFIG_ADDRESS + pos, (uint32 *)(tmp), sizeof(struct wifi_auth_node_data));
		pos += sizeof(struct wifi_auth_node_data);
		tmp = tmp->next;
	}

	struct wifi_auth_node_data blank;
	os_bzero(&blank, sizeof(struct wifi_auth_node_data));
	log_info("save blank, size: %d", sizeof(struct wifi_auth_node_data));
	spi_flash_write(WIFI_LIST_CONFIG_ADDRESS + pos, (uint32 *)(&blank), sizeof(struct wifi_auth_node_data));
}

bool ICACHE_FLASH_ATTR wifi_list_ssid_exist(const char* ssid) {
	return search_node(ssid) != NULL;
}