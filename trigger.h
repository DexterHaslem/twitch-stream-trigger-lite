#pragma once

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <commctrl.h>
#include <WinUser.h>

#include <stdio.h> 
#include <stdlib.h> 
#include <string.h> 
#include <stdint.h>
#include <stdbool.h>

/* this could be dynamic but just hardcode enough for now */
#define NUM_HARDCODED_TRIGGERS 4

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

	/* debugging */
	int poll_count;
};

void triggers_check(void);
bool triggers_restore(void);
void triggers_save(void);
void trigger_fire(struct stream_trigger_t* trigger);
void triggers_init(void);
void trigger_enable(struct stream_trigger_t* trigger, bool enabled);
void triggers_reset_online(void);
void trigger_user_online(const char* user_name);
struct stream_trigger_t* triggers_get(void);
bool triggers_any_enabled(void);
void triggers_update_from_ui(void);
void triggers_copy_to_ui(void);

