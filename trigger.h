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

/* according to old twitch api comments max is 25, min is 3 with some old accounts < 3 */
#define TWITCH_ACCOUNT_MAXLEN 25

/* win32 limit is 32k lmfao */
#define CMD_MAXLEN 2048

/* structure to hold anything persisted outside of in-memory state */
struct stream_trigger_persist_t
{
	unsigned int num; /* todo: consider removing */
	bool enabled;
	
	size_t account_len;
	const char* account;
	
	size_t cmd_len;
	const char* cmd;

	unsigned int pad;
};

struct stream_trigger_t
{
	HMENU enabledCheckboxId;
	HWND hEditAccount;
	HWND hEditCommand;
	HWND hEnabledCheckbox;
	HWND hStaticStatus;

	struct stream_trigger_persist_t persist;

	/* in memory working state. doesnt need to be persisted */
	bool first_check;
	bool is_online;
	bool prev_online;

	/* account and cmd are persisted, but not written full length out */
	char account[TWITCH_ACCOUNT_MAXLEN];
	char cmd[CMD_MAXLEN];
	
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

