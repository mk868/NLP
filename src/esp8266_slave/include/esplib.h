#pragma once

#include <string.h>
#include <mem.h>

#define free(s)			os_free(s)
#define malloc(s)		os_malloc(s)
#define calloc(s)		os_calloc(s)
#define realloc(p, s)	os_realloc(p, s)
#define zalloc(s)		os_zalloc(s)

