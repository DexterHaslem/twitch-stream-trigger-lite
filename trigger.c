#include "trigger.h"

#define SAVE_FILE_NAME "triggers.dat"

static struct stream_trigger_t triggers[NUM_HARDCODED_TRIGGERS];

struct stream_trigger_t* triggers_get()
{
	return triggers;
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

		WPARAM checked = triggers[i].enabled ? BST_CHECKED : BST_UNCHECKED;
		SendDlgItemMessage(triggers[i].hEnabledCheckbox,
			triggers[i].enabledCheckboxId, BM_SETCHECK, checked, 0);
	}
}

/* deserialize triggers into in mem bufs */
bool triggers_restore(void)
{
	FILE* open_file;
	const errno_t open_err = fopen_s(&open_file, SAVE_FILE_NAME, "r");
	if (open_file == NULL || open_err != 0)
	{
		/* this is ok, might be first run */
		return false;
	}

	/* this is again not update safe or portable. we should also ideally read to a temp
	 * buffer instead incase anything goes wrong but caller will set default state anyway
	 */
	size_t elements_read = fread(&triggers, sizeof(struct stream_trigger_t),
		NUM_HARDCODED_TRIGGERS, open_file);
	
	fclose(open_file);
	return elements_read > 0;
}

/* serialize triggers */
void triggers_save(void)
{
	FILE* save_file;
	const errno_t open_err = fopen_s(&save_file, SAVE_FILE_NAME, "w");
	if (save_file == NULL || open_err != 0)
	{
		/* TODO: win32 logging */
		return;
	}
	
	/* write structures as is, this is not portable and update safe */
	size_t elements_written = fwrite(&triggers, sizeof(struct stream_trigger_t),
		NUM_HARDCODED_TRIGGERS, save_file);
	
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
		NULL,       // Process handle not inheritable
		NULL,        // Thread handle not inheritable
		FALSE,			// Set handle inheritance to FALSE
		DETACHED_PROCESS,
		NULL,						// Use parent's environment block
		NULL,        // Use parent's starting directory 
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
		if (!triggers[i].enabled)
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
			triggers[i].num = i + 1;
			triggers[i].enabled = false;
			triggers[i].first_check = true;
			triggers[i].poll_count = 0;
			strcpy_s(triggers[i].account, TWITCH_ACCOUNT_MAXLEN, "");
			strcpy_s(triggers[i].cmd, CMD_MAXLEN, "");
		}
	}
}

void trigger_enable(struct stream_trigger_t* trigger, bool enabled)
{
	trigger->enabled = enabled;
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
		if (triggers[i].enabled && _strcmpi(triggers[i].account, user_name) == 0)
		{
			triggers[i].is_online = true;
		}
	}
}

bool triggers_any_enabled(void)
{
	for (size_t i = 0; i < NUM_HARDCODED_TRIGGERS; ++i)
	{
		if (triggers[i].enabled)
		{
			return true;
		}
	}
	
	return false;
}
