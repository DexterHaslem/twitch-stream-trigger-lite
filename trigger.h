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

#define TWITCH_ACCOUNT_MAXLEN 25
#define CMD_MAXLEN 128

struct stream_trigger_t
{
	int num;
	bool enabled;
	char account[TWITCH_ACCOUNT_MAXLEN];
	char cmd[CMD_MAXLEN];

	HMENU enabledCheckboxId;
	HWND hEditAccount;
	HWND hEditCommand;
	HWND hEnabledCheckbox;
	HWND hStaticStatus;

	/* TODO: consider timestamp to debounce restarts */
	bool first_check;
	bool is_online;
	bool prev_online;
};

struct stream_triggers_t
{
	struct stream_trigger_t *triggers;
	size_t count;
};