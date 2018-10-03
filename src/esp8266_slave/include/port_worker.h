#include <c_types.h>

typedef enum {
	REL1,
	CONFIG_LED,
} Devices;


typedef void(*port_worker_change_callback)(Devices device, bool enabled);


void port_worker_init();
void port_worker_set(Devices device, bool enable);
void port_worker_enable(Devices device);
void port_worker_disable(Devices device);
void port_worker_toggle(Devices device);

bool port_worker_get(Devices device);

void port_worker_enable_to_time(Devices device, unsigned long sec);

void port_worker_set_on_change_cb(port_worker_change_callback cb);

