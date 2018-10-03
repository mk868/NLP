#pragma once

#include <c_types.h>

void ws_worker_start();

void ws_worker_send_state(char* name, bool enabled);
