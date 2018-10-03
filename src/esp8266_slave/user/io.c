#include "io.h"

#include "webserver.h"
#include "esplogger.h"
#include "c_types.h"
#include "port_worker.h"
#include <osapi.h>
#include "driver/key.h"
#include <ets_sys.h>




static struct keys_param keys;
static struct single_key_param *single_key[2];


static void ICACHE_FLASH_ATTR io_key_rel_short_press(void)
{
	log_info("clicked");
	port_worker_toggle(REL1);
}

static bool io_key_config_enabled = false;
static void ICACHE_FLASH_ATTR io_key_config_long_press(void)
{
	log_info("clicked");
	io_key_config_enabled = !io_key_config_enabled;

	if (io_key_config_enabled) {
		webserver_start();
		GPIO_OUTPUT_SET(CONFIG_IO_NUM, CONFIG_IO_ONSTATE);
	}
	else {
		webserver_stop();
		GPIO_OUTPUT_SET(CONFIG_IO_NUM, !CONFIG_IO_ONSTATE);
	}
}


void io_init() {

	PIN_FUNC_SELECT(CONFIG_IO_MUX, CONFIG_IO_FUNC);//config led

	//input keys init
	single_key[0] = key_init_single(KEY_REL_IO_NUM, KEY_REL_IO_MUX, KEY_REL_IO_FUNC,
		NULL, io_key_rel_short_press);

	single_key[1] = key_init_single(KEY_CONFIG_IO_NUM, KEY_CONFIG_IO_MUX, KEY_CONFIG_IO_FUNC,
		NULL, io_key_config_long_press);

	keys.key_num = 2;
	keys.single_key = single_key;

	key_init(&keys);
	//

}