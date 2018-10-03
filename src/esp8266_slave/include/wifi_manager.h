
#include "c_types.h"

void wifi_manager_init();
void wifi_manager_start();
void wifi_manager_stop();

bool wifi_manager_station_connect(char* ssid);
bool wifi_manager_station_disconnect(char* ssid);

const char* wifi_manager_get_station_ssid();

void wifi_manager_disconnect();

//void wifi_manager_wps_connect();

bool wifi_manager_is_internet_access();

