

#include "user_config.h"
#include <esp8266.h>
#include "stdout.h"

#include "esplogger.h"
#include "wifi_list.h"
#include "wifi_manager.h"

#include <ets_sys.h>
#include <osapi.h>

#include "io.h"

#include "driver/uart.h"
#include "user_interface.h"

#include "wifi_list.h"

#include "webserver.h"
#include <espconn.h>
#include "port_worker.h"


#include "ws_worker.h"




void ICACHE_FLASH_ATTR dns_server_init() {
	ip_addr_t dns;
	SET_DNS1(&dns);
	espconn_dns_setserver(0, &dns);
	SET_DNS2(&dns);
	espconn_dns_setserver(1, &dns);
}



static void ICACHE_FLASH_ATTR on_change(Devices device, bool enabled) {//for each enabled/disabled device
	os_printf(":::::device: %d   enabled: %d\n", device, enabled);
	

	char *name = NULL;

	switch (device) {
		case REL1:
			name = "REL1";
	}

	if(name != NULL)
		ws_worker_send_state(name, enabled);
}


void ICACHE_FLASH_ATTR init_done() {
	log_info("init done");

	wifi_list_init();
	
	//wifi_list_save();
	wifi_list_load();
	//flash_test();


	port_worker_init();
	port_worker_set_on_change_cb(on_change);

	ws_worker_start();

	wifi_manager_init();
	wifi_manager_start();
}


//Main routine. Initialize stdout, the I/O, filesystem and the webserver and we're done.
void user_init(void) {
	stdoutInit();

	log_info("Hi");

	wifi_set_opmode(NULL_MODE);


	webserver_init();

	//httpd init start

	gpio_init();
	
	io_init();

	dns_server_init();

	system_init_done_cb(init_done);
}

void user_rf_pre_init() {
	//Not needed, but some SDK versions want this defined.
}


//Sdk 2.0.0 needs extra sector to store rf cal stuff. Place that at the end of the flash.
uint32 ICACHE_FLASH_ATTR
user_rf_cal_sector_set(void)
{
	enum flash_size_map size_map = system_get_flash_size_map();
	uint32 rf_cal_sec = 0;

	switch (size_map) {
	case FLASH_SIZE_4M_MAP_256_256:
		rf_cal_sec = 128 - 8;
		break;

	case FLASH_SIZE_8M_MAP_512_512:
		rf_cal_sec = 256 - 5;
		break;

	case FLASH_SIZE_16M_MAP_512_512:
	case FLASH_SIZE_16M_MAP_1024_1024:
		rf_cal_sec = 512 - 5;
		break;

	case FLASH_SIZE_32M_MAP_512_512:
	case FLASH_SIZE_32M_MAP_1024_1024:
		rf_cal_sec = 1024 - 5;
		break;

	default:
		rf_cal_sec = 0;
		break;
	}

	return rf_cal_sec;
}

