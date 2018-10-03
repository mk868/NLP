#pragma once

#include <eagle_soc.h>

#define KEY_REL_IO_MUX		PERIPHS_IO_MUX_GPIO0_U
#define KEY_REL_IO_NUM		0
#define KEY_REL_IO_FUNC		FUNC_GPIO0

#define KEY_CONFIG_IO_MUX	PERIPHS_IO_MUX_GPIO5_U
#define KEY_CONFIG_IO_NUM	5
#define KEY_CONFIG_IO_FUNC	FUNC_GPIO5


#define CONFIG_IO_MUX		PERIPHS_IO_MUX_GPIO2_U
#define CONFIG_IO_NUM		2
#define CONFIG_IO_FUNC		FUNC_GPIO2
#define CONFIG_IO_ONSTATE	false

#define REL1_IO_MUX     PERIPHS_IO_MUX_GPIO4_U
#define REL1_IO_NUM     4
#define REL1_IO_FUNC    FUNC_GPIO4
#define REL1_IO_ONSTATE	true


void io_init();