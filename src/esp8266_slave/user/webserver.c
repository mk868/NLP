#include "webserver.h"

#include <ets_sys.h>
#include <esp8266.h>
#include "platform.h"
#include "auth.h"
#include "espfs.h"
#include "webpages-espfs.h"
#include "httpd.h"
#include "httpdespfs.h"
#include "cgi_wifi_manager.h"
#include "esplogger.h"


//Function that tells the authentication system what users/passwords live on the system.
//This is disabled in the default build; if you want to try it, enable the authBasic line in
//the builtInUrls below.
static int ICACHE_FLASH_ATTR myPassFn(HttpdConnData *connData, int no, char *user, int userLen, char *pass, int passLen) {
	if (no == 0) {
		os_strcpy(user, "admin");
		os_strcpy(pass, "s3cr3t");
		return 1;
	}
	return 0;
}


/*
This is the main url->function dispatching data struct.
In short, it's a struct with various URLs plus their handlers. The handlers can
be 'standard' CGI functions you wrote, or 'special' CGIs requiring an argument.
They can also be auth-functions. An asterisk will match any url starting with
everything before the asterisks; "*" matches everything. The list will be
handled top-down, so make sure to put more specific rules above the more
general ones. Authorization things (like authBasic) act as a 'barrier' and
should be placed above the URLs they protect.
*/
static HttpdBuiltInUrl builtInUrls[] = {
	//{"*", cgiRedirectApClientToHostname, "esp8266.nonet"},
	{ "/", cgiRedirect, "/index.html" },



	//Routines to make the /wifi URL and everything beneath it work.

	//Enable the line below to protect the WiFi configuration with an username/password combo.
	//	{"/*", authBasic, myPassFn},


	{ "/go/scan.json", www_wifi_scan, NULL },
	{ "/go/list.json", www_wifi_list, NULL },
	{ "/go/summary.json", www_wifi_summary, NULL },
	{ "/go/station-info.json", www_wifi_station_info, NULL },
	{ "/go/let-connect.json", www_wifi_let_connect, NULL },
	{ "/go/let-delete.json", www_wifi_let_delete, NULL },
	{ "/go/let-save.json", www_wifi_let_save, NULL },

	{ "*", cgiEspFsHook, NULL }, //Catch-all cgi function for the filesystem
	{ NULL, NULL, NULL }
};

static bool _started = false;


void webserver_init()
{
	// 0x40200000 is the base address for spi flash memory mapping, ESPFS_POS is the position
	// where image is written in flash that is defined in Makefile.

	#ifdef ESPFS_POS
		espFsInit((void*)(0x40200000 + ESPFS_POS));
	#else
		espFsInit((void*)(webpages_espfs_start));
	#endif
}

void webserver_start()
{
	log_info("started");

	uint8 mode = wifi_get_opmode();
	mode |= SOFTAP_MODE;
	wifi_set_opmode(mode);

	if(!_started){
		httpdInit(builtInUrls, 80);

		log_info("httpd inited");

		_started = true;
	}
}

void webserver_stop()
{
	log_info("stopped");

	uint8 mode = wifi_get_opmode();
	mode &= ~SOFTAP_MODE;
	wifi_set_opmode(mode);
}
