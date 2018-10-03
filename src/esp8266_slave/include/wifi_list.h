
#include "c_types.h"

#ifndef __WIFI_LIST_H__
#define __WIFI_LIST_H__


struct wifi_auth_node_data //struct & size corresponding to wifi_auth_node data fields
{
	char ssid[32];
	char login[32];
	char password[64];
};

struct wifi_auth_node {
	char ssid[32];
	char login[32];
	char password[64];
	struct wifi_auth_node* next;
};

void wifi_list_init();

void wifi_list_add(const char* ssid, const char* login, const char* password);

void wifi_list_delete(const char* ssid);

const char* wifi_list_get_password(const char* ssid);

bool wifi_list_ssid_exist(const char* ssid);

const char* wifi_list_get_login(const char* ssid);

struct wifi_auth_node* wifi_list_get_root();

void wifi_list_load();

void wifi_list_save();


#endif
