#include "trigger.h"

static struct stream_trigger_t triggers[NUM_HARDCODED_TRIGGERS];

struct stream_trigger_t* triggers_get()
{
	return triggers;
}

void triggers_restore(void)
{
}

void triggers_save(void)
{
}

void trigger_fire(struct stream_trigger_t* trigger)
{
}

void triggers_check(void)
{
	for (int i = 0; i < NUM_HARDCODED_TRIGGERS; ++i)
	{
	}
}

void triggers_init()
{
	for (int i = 0; i < NUM_HARDCODED_TRIGGERS; ++i)
	{
		/* NOTE: do not overlap resource ids if anything else gets added */
		triggers[i].enabledCheckboxId = (HMENU)50000 + i;
		triggers[i].num = i + 1;
		triggers[i].enabled = false;
		triggers[i].first_check = true;
		strcpy_s(triggers[i].account, TWITCH_ACCOUNT_MAXLEN, "");
		strcpy_s(triggers[i].cmd, CMD_MAXLEN, "");
	}

	/* TODO: restoration of account/cmd here */
}

void trigger_enable(struct stream_trigger_t* trigger, bool enabled)
{
	trigger->enabled = enabled;
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