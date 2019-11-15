#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <commctrl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "cJSON.h"


struct stream_trigger
{
	bool enabled;
	const char* account;
	const char* cmd;
	bool is_online;
};

struct all_triggers
{
	struct stream_trigger trigger_one;
	struct stream_trigger trigger_two;
	struct stream_trigger trigger_three;
};