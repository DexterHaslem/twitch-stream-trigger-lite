#include "trigger.h"
#include <assert.h>

#define SAVE_FILE_NAME "triggers.dat"
#define SAVE_PAD_MAGIC (0xCAFEBABE)

/* stamp of version of save file, changing this will invalidate any previous save! only change when destructive
 * file format changes are done 
 */
#define EXPECTED_SERIALIZE_VERSION (0x324)


static struct stream_trigger_t triggers[NUM_HARDCODED_TRIGGERS];

struct stream_trigger_t* triggers_get()
{
	return triggers;
}

static void update_persist(struct stream_trigger_t* trigger)
{
	/* just point at in memory arrays, but this trick wont write out full buf,
	 * just used. extremely not worth it
	 */
	size_t acct_len = strlen(trigger->account);
	trigger->persist.account_len = acct_len;
	trigger->persist.account = trigger->account;
	
	size_t cmd_len = strlen(trigger->cmd);
	trigger->persist.cmd_len = cmd_len;
	trigger->persist.cmd = trigger->cmd;

	trigger->persist.pad = SAVE_PAD_MAGIC;
}

/* move ui state into trigger buffers state */
void triggers_update_from_ui()
{
	for (int i = 0; i < NUM_HARDCODED_TRIGGERS; ++i)
	{
		/* zero out bufs so we dont end up serializing trailing data*/
		memset(triggers[i].account, 0, TWITCH_ACCOUNT_MAXLEN);
		memset(triggers[i].cmd, 0, CMD_MAXLEN);
		
		GetWindowText(triggers[i].hEditAccount, triggers[i].account, TWITCH_ACCOUNT_MAXLEN);
		GetWindowText(triggers[i].hEditCommand, triggers[i].cmd, CMD_MAXLEN);

		bool enabled = SendDlgItemMessage(triggers[i].hEnabledCheckbox, 
			triggers[i].enabledCheckboxId, BM_GETCHECK, 0, 0);

		update_persist(&triggers[i]);
		
		trigger_enable(&triggers[i], enabled);
	}
}

/* move trigger memory buffers into ui controls */
void triggers_copy_to_ui()
{
	for (int i = 0; i < NUM_HARDCODED_TRIGGERS; ++i)
	{
		SetWindowText(triggers[i].hEditAccount, triggers[i].account);
		SetWindowText(triggers[i].hEditCommand, triggers[i].cmd);

		WPARAM checked = triggers[i].persist.enabled ? BST_CHECKED : BST_UNCHECKED;
		SendDlgItemMessage(triggers[i].hEnabledCheckbox,
			triggers[i].enabledCheckboxId, BM_SETCHECK, checked, 0);
	}
}

/* deserialize triggers into in mem bufs */
bool triggers_restore(void)
{
	FILE* open_file;
	const errno_t open_err = fopen_s(&open_file, SAVE_FILE_NAME, "rb");
	if (open_file == NULL || open_err != 0)
	{
		/* this is ok, might be first run */
		return false;
	}

	/* first */
	unsigned int tmp;
	fread(&tmp, sizeof(tmp), 1, open_file);
	if (tmp != EXPECTED_SERIALIZE_VERSION)
	{
		/* TODO: let user know exactly what happened, maybe reset file */
		char err_msg[255];
		snprintf(err_msg, 255, "Failed to restore %s from an incompatible version.\nTriggers have been reset.",
			SAVE_FILE_NAME);
		
		MessageBox(NULL, err_msg, "Trigger restore error", MB_OK | MB_ICONEXCLAMATION);
		fclose(open_file);
		return false;
	}
	
	size_t total_restored = 0;
	for (int i = 0; i < NUM_HARDCODED_TRIGGERS; ++i)
	{
		/* read out structure by elem so we can handle var strings. MUST MATCH SAVE ORDER */
		fread(&triggers[i].persist.num, sizeof(triggers[i].persist.num), 1, open_file);
		fread(&triggers[i].persist.enabled, sizeof(triggers[i].persist.enabled), 1, open_file);

		/* read into non-persisted bufs. our persist pointers are bogus. they will be updated to point at buf at exit/update
		 * beware the strings coming in are not zero delimited. trigger bufs are zero but make sure.*/
		fread(&triggers[i].persist.account_len, sizeof(triggers[i].persist.account_len), 1, open_file);
		fread(triggers[i].account, sizeof(char), triggers[i].persist.account_len, open_file);
		triggers[i].account[triggers[i].persist.account_len] = 0;
		
		fread(&triggers[i].persist.cmd_len, sizeof(triggers[i].persist.cmd_len), 1, open_file);
		fread(triggers[i].cmd, sizeof(char), triggers[i].persist.cmd_len, open_file);
		triggers[i].cmd[triggers[i].persist.cmd_len] = 0;
		
		/* do magic pad last for string fuckupery checking */
		fread(&triggers[i].persist.pad, sizeof(triggers[i].persist.pad), 1, open_file);

		assert(triggers[i].persist.pad == SAVE_PAD_MAGIC);
		++total_restored;
	}
	
	fclose(open_file);
	return total_restored == NUM_HARDCODED_TRIGGERS;
}

/* serialize triggers */
void triggers_save(void)
{
	FILE* save_file;
	const errno_t open_err = fopen_s(&save_file, SAVE_FILE_NAME, "wb");
	if (save_file == NULL || open_err != 0)
	{
		/* TODO: win32 logging */
		return;
	}

	/* stamp file so we can safely load it/handle it later */
	unsigned int tmp = EXPECTED_SERIALIZE_VERSION;
	fwrite(&tmp, sizeof(tmp), 1, save_file);
	
	for (int i = 0; i < NUM_HARDCODED_TRIGGERS; ++i)
	{
		/* write out structure by elem so we can handle var strings. MUST MATCH RESTORE ORDER */
		fwrite(&triggers[i].persist.num, sizeof(triggers[i].persist.num), 1, save_file);
		fwrite(&triggers[i].persist.enabled, sizeof(triggers[i].persist.enabled), 1, save_file);

		/* ok now the string overengineering. do size first, then bytes from ptr to buff. if you try to serialize
		 * whole struct just the pointer value is persisted */
		fwrite(&triggers[i].persist.account_len, sizeof(triggers[i].persist.account_len), 1, save_file);
		fwrite(triggers[i].persist.account, sizeof(char), triggers[i].persist.account_len, save_file);
		
		fwrite(&triggers[i].persist.cmd_len, sizeof(triggers[i].persist.cmd_len), 1, save_file);
		fwrite(triggers[i].persist.cmd, sizeof(char), triggers[i].persist.cmd_len, save_file);
		
		/* this leaves the strings not zero delimited. beware */
		
		/* do magic pad last for string fuckupery checking */
		fwrite(&triggers[i].persist.pad, sizeof(triggers[i].persist.pad), 1, save_file);
		
		/* TODO error check save */
	}
	
	fclose(save_file);
}


/* unconditionally fires a triggers command regardless of state */
/* TODO: consider private usage. we never need direct use? */
void trigger_fire(struct stream_trigger_t* trigger)
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	 ZeroMemory(&pi, sizeof(pi));

	/* TODO: error checking */
	if (CreateProcess(NULL,
		trigger->cmd,				// Command line
		NULL,		// Process handle not inheritable
		NULL,			// Thread handle not inheritable
		FALSE,			// Set handle inheritance to FALSE
		DETACHED_PROCESS,
		NULL,						// Use parent's environment block
		NULL,				// Use parent's starting directory 
		&si,						// Pointer to STARTUPINFO structure
		&pi)) // Pointer to PROCESS_INFORMATION structure	
	{
		/* TODO: is this needed for batches ? we dont want to block app for direct executions */
		// WaitForSingleObject(pi.hProcess, INFINITE);
	}

	// Close process and thread handles.
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
}

/* checks all triggers state and fires any that transitioned online */
void triggers_check(void)
{
	for (int i = 0; i < NUM_HARDCODED_TRIGGERS; ++i)
	{
		if (!triggers[i].persist.enabled)
		{
			/* preserve first_check logic for disabled triggers, if enabled later .. */
			continue;
		}
		
		if (triggers[i].is_online && !triggers[i].prev_online)
		{
			trigger_fire(&triggers[i]);
		}

		++triggers[i].poll_count;
		triggers[i].prev_online = triggers[i].is_online;
		triggers[i].first_check = false;
	}
}

void triggers_init()
{
	memset(triggers, 0, sizeof(struct stream_trigger_t) * NUM_HARDCODED_TRIGGERS);
	
	const bool got_restored = triggers_restore();
	if (!got_restored)
	{
		for (int i = 0; i < NUM_HARDCODED_TRIGGERS; ++i)
		{
			/* NOTE: do not overlap resource ids if anything else gets added */
			triggers[i].enabledCheckboxId = (HMENU)50000 + i;
			triggers[i].persist.num = i + 1;
			triggers[i].persist.enabled = false;
			triggers[i].first_check = true;
			triggers[i].poll_count = 0;
		}
	}
}

void trigger_enable(struct stream_trigger_t* trigger, bool enabled)
{
	trigger->persist.enabled = enabled;
	if (!enabled)
	{
		trigger->first_check = true;
		trigger->prev_online = false;
	}
}

void triggers_reset_online(void)
{
	/* reset online, but leave previous state alone so we know to trigger */
	for (size_t i = 0; i < NUM_HARDCODED_TRIGGERS; ++i)
	{
		triggers[i].is_online = false;
	}
}

void trigger_user_online(const char* user_name)
{
	for (size_t i = 0; i < NUM_HARDCODED_TRIGGERS; ++i)
	{
		if (triggers[i].persist.enabled && _strcmpi(triggers[i].account, user_name) == 0)
		{
			triggers[i].is_online = true;
		}
	}
}

bool triggers_any_enabled(void)
{
	for (size_t i = 0; i < NUM_HARDCODED_TRIGGERS; ++i)
	{
		if (triggers[i].persist.enabled)
		{
			return true;
		}
	}
	
	return false;
}
