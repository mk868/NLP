#pragma once

#include <esp8266.h>
#include "httpd.h"

int www_wifi_scan(HttpdConnData *connData);
int www_wifi_list(HttpdConnData *connData);
int www_wifi_summary(HttpdConnData *connData);
int www_wifi_station_info(HttpdConnData *connData);
int www_wifi_let_connect(HttpdConnData *connData);
int www_wifi_let_delete(HttpdConnData *connData);
int www_wifi_let_save(HttpdConnData *connData);

