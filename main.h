#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <commctrl.h>
#include <WinUser.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "cJSON.h"

#define ACCOUNT_MAXLEN 16
#define CMD_MAXLEN 128

struct stream_trigger_serialize
{
	int num;
	bool enabled;
	char account[ACCOUNT_MAXLEN];
	char cmd[CMD_MAXLEN];
};

struct stream_trigger
{
	struct stream_trigger_serialize serialize;
	HMENU enabledCheckboxId;
	HWND hEditAccount;
	HWND hEditCommand;
	HWND hEnabledCheckbox;
	HWND hStaticStatus;
	bool is_online;
	bool prev_online;
};
