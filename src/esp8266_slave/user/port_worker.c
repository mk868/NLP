#include "port_worker.h"

#include <gpio.h>
#include <eagle_soc.h>
#include <os_type.h>
#include <osapi.h>
#include "io.h"

static os_timer_t   _timer[2];///DEVICES COUNT
static port_worker_change_callback _change_cb = NULL;

void port_worker_init()
{
	PIN_FUNC_SELECT(REL1_IO_MUX, REL1_IO_FUNC);//move to io.h
}

void port_worker_set(Devices device, bool enable)
{
	if (enable) {
		port_worker_enable(device);
	}
	else {
		port_worker_disable(device);
	}
}

void port_worker_enable(Devices device)
{
	switch (device)
	{
	case REL1:
		GPIO_OUTPUT_SET(REL1_IO_NUM, REL1_IO_ONSTATE);
		break;
	}

	os_timer_disarm(&_timer[device]);

	if (_change_cb != NULL) {
		_change_cb(device, true);
	}
}

void port_worker_disable(Devices device)
{
	switch (device)
	{
	case REL1:
		GPIO_OUTPUT_SET(REL1_IO_NUM, !REL1_IO_ONSTATE);
		break;
	}

	os_timer_disarm(&_timer[device]);

	if (_change_cb != NULL) {
		_change_cb(device, false);
	}
}

void port_worker_toggle(Devices device)
{
	port_worker_set(device, !port_worker_get(device));
}

bool port_worker_get(Devices device)
{
	switch (device)
	{
	case REL1:
		return GPIO_INPUT_GET(REL1_IO_NUM);
	}

	return false;
}

void ICACHE_FLASH_ATTR timer_time_end_cb(void *arg)
{
	Devices device = (Devices)arg;

	port_worker_disable(device);
}


void port_worker_enable_to_time(Devices device, unsigned long sec)
{
	port_worker_enable(device);

	os_timer_disarm(&_timer[device]);
	os_timer_setfn(&_timer[device], (os_timer_func_t *)timer_time_end_cb, (void*)device);
	os_timer_arm(&_timer[device], sec * 1000, false);//fixme time limit???
}

void port_worker_set_on_change_cb(port_worker_change_callback cb)
{
	_change_cb = cb;
}
