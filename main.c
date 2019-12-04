#include "trigger.h"
#include "twitch_api.h"

#define WINDOW_TITLE		("Twitch Stream Trigger lite")
#define WINDOW_CLASSNAME	("dmh-tstl")
#define WINDOW_WIDTH		(380)
#define WINDOW_HEIGHT		(550)
#define POLL_TIMER_ID		(1)
#define UI_UPDATE_MS		(500)
#define API_POLL_UPDATES	(60) /* hit api every 60 ui updates, or 30 seconds */


#define ID_FILE_EXIT		40001
#define ID_HELP_LICENSES	40002
#define ID_HELP_ABOUT		40003

static HWND hStatus;
static HWND hMain;
static struct stream_trigger_t* triggers;
static unsigned int api_poll_counter;

static void update_state()
{
	/* ensure triggers have latest ui state first. we do not handle messages */
	triggers_update_from_ui(hMain);
	
	triggers_reset_online();

	if (triggers_any_enabled())
	{
		get_streams_status(triggers);

		triggers_check();
	}
}

static void update_ui(HWND hwnd, UINT msg, UINT_PTR timerId, DWORD dwTime)
{
	char status_text[256] = { 0 };
	
	if (api_poll_counter++ >= API_POLL_UPDATES)
	{
		update_state();
		api_poll_counter = 0;
		snprintf(status_text, 256, "Updating now");
	}
	else
	{
		int seconds_remaining = ((API_POLL_UPDATES - api_poll_counter) * UI_UPDATE_MS)/1000;
		snprintf(status_text, 256, "Updating in %u seconds..", seconds_remaining);
	}
	
	SetWindowText(hStatus, status_text);
}


void create_trigger_group(struct stream_trigger_t *trigger, int start_y, HWND owner)
{
	HINSTANCE hInstance = GetModuleHandle(NULL); // (HINSTANCE)GetWindowLong(owner, GWL_HINSTANCE);

	char titlebuf[32];
	sprintf_s(titlebuf, 32, "Trigger %d", trigger->persist.num);

	/* NOTE: ui state is reflected from restored triggers after creation */
	HWND hGroupBoxTrigger = CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, titlebuf,
										   WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
										   5, start_y, 320, 100, owner, NULL, hInstance, NULL);

	HWND hEnabled = CreateWindowEx(0, WC_BUTTON, "Enabled", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
								   10, start_y + 15, 75, 25, owner, (HMENU)trigger->enabled_checkbox_id, hInstance, NULL);
	//trigger->hEnabledCheckbox = hEnabled;

	HWND hAccountLabel = CreateWindowEx(0, WC_STATIC, "Account:", WS_CHILD | WS_VISIBLE,
										10, start_y + 45, 100, 25, owner,
										NULL, hInstance, NULL);

	HWND hEditAccount = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "Account here",
									   WS_CHILD | WS_VISIBLE | WS_TABSTOP,
									   75, start_y + 40, 230, 25, owner, NULL, hInstance, NULL);
	trigger->hEditAccount = hEditAccount;
	SendMessage(hEditAccount, EM_SETLIMITTEXT, TWITCH_ACCOUNT_MAXLEN, 0);
	
	HWND hCmdLabel = CreateWindowEx(0, WC_STATIC, "Command:", WS_CHILD | WS_VISIBLE,
									10, start_y + 75, 100, 25, owner,
									NULL, hInstance, NULL);

	HWND hEditCommandLine = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "Command line here",
										   WS_CHILD | WS_VISIBLE | WS_TABSTOP,
										   75, start_y + 70, 230, 25, owner, NULL, hInstance, NULL);
	trigger->hEditCommand = hEditCommandLine;
	SendMessage(hEditCommandLine, EM_SETLIMITTEXT, CMD_MAXLEN, 0);
	
	HFONT hfDefault = GetStockObject(DEFAULT_GUI_FONT);
	SendMessage(hGroupBoxTrigger, WM_SETFONT, (WPARAM)hfDefault, MAKELPARAM(FALSE, 0));
	SendMessage(hEnabled, WM_SETFONT, (WPARAM)hfDefault, MAKELPARAM(FALSE, 0));
	SendMessage(hAccountLabel, WM_SETFONT, (WPARAM)hfDefault, MAKELPARAM(FALSE, 0));
	SendMessage(hCmdLabel, WM_SETFONT, (WPARAM)hfDefault, MAKELPARAM(FALSE, 0));
	SendMessage(hEditCommandLine, WM_SETFONT, (WPARAM)hfDefault, MAKELPARAM(FALSE, 0));
	SendMessage(hEditAccount, WM_SETFONT, (WPARAM)hfDefault, MAKELPARAM(FALSE, 0));
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_SIZE:
		/* make groupboxes resize with parent - not really used, we disabled resize
		MoveWindow(hGroupBoxTrigger1, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
		*/
		break;
	case WM_CREATE:
	{
		HMENU hMenu, hSubMenu;

		hMenu = CreateMenu();

		hSubMenu = CreatePopupMenu();
		AppendMenu(hSubMenu, MF_STRING, ID_FILE_EXIT, "E&xit");
		AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hSubMenu, "&File");

		hSubMenu = CreatePopupMenu();
		AppendMenu(hSubMenu, MF_STRING, ID_HELP_LICENSES, "&Licenses");
		AppendMenu(hSubMenu, MF_STRING, ID_HELP_ABOUT, "&About");
		AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT_PTR)hSubMenu, "&Help");

		SetMenu(hwnd, hMenu);

		hStatus = CreateWindowEx(0, STATUSCLASSNAME, NULL,
								 WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
								 hwnd, NULL, GetModuleHandle(NULL), NULL);

		for (int i = 0; i < NUM_HARDCODED_TRIGGERS; ++i)
		{
			create_trigger_group(&triggers[i], (i * 110 + 5), hwnd);
		}

		/* start polling timer now */
		SetTimer(hwnd, POLL_TIMER_ID, UI_UPDATE_MS, update_ui);

		/* do not do an initial check for state here, we do not have ui state restored yet*/
		
		break;
	}
	case WM_CLOSE:
		/* important! this needs to be done before destroyed (eg after message loop pump) */
		triggers_update_from_ui(hMain);
		triggers_save();
		
		KillTimer(hwnd, POLL_TIMER_ID);
		DestroyWindow(hwnd);
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_COMMAND:
	{
		WORD from = LOWORD(wParam);
			
		switch (from)
		{
		case ID_FILE_EXIT:
			PostMessage(hwnd, WM_CLOSE, 0, 0);
			break;
		case ID_HELP_LICENSES:
			MessageBox(hwnd, "licenses", "About Licenses", MB_OK | MB_ICONERROR);
			break;
		case ID_HELP_ABOUT:
			/* TODO: */
			break;
		default:
		{
#if 0
			/* UGH: you *have to do this here* - trying to ask for getcheck later will not update */
			/* check if it was one of our trigger enable checkboxes */
			for (int i = 0; i < NUM_HARDCODED_TRIGGERS; ++i)
			{
				if ((HMENU)from == triggers[i].enabled_checkbox_id)
				{
					if (HIWORD(wParam) == BN_CLICKED)
					{
						/* have to ask current state everytime */
						bool enabled = SendDlgItemMessage(hwnd, from, BM_GETCHECK, 0, 0);
						trigger_enable(&triggers[i], enabled);
					}
					break;
				}
			}
#endif
		}
		}
		break;
	}
	default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	WNDCLASSEX wc;
	MSG msg;

	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = 0;
	wc.lpfnWndProc = WndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = hInstance;
	wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW);
	wc.lpszMenuName = NULL;
	wc.lpszClassName = WINDOW_CLASSNAME;
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	if (!RegisterClassEx(&wc))
	{
		MessageBox(NULL, "Window Registration Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);
		return 1;
	}

	/* important: create triggers before createwindow - wm create called before show,
	 * causing confusing races in debugging */
	triggers_init();
	triggers = triggers_get();
	
	hMain = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		WINDOW_CLASSNAME,
		WINDOW_TITLE,
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
		CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_WIDTH, WINDOW_HEIGHT,
		NULL, NULL, hInstance, NULL);

	if (hMain == NULL)
	{
		MessageBox(NULL, "Window Creation Failed!", "Error!",
				   MB_ICONEXCLAMATION | MB_OK);
		return 1;
	}

	/* show initial state. this is safe, controls have been created by WM_CREATE */
	triggers_copy_to_ui(hMain);
	
	ShowWindow(hMain, nCmdShow);
	UpdateWindow(hMain);

	/* kick off an update right away to get initial stream(s) state*/
	update_state();
	
	while (GetMessage(&msg, NULL, 0, 0) > 0)
	{
		/* Translate virtual-key messages into character messages */
		if (IsDialogMessage(hMain, &msg) == 0)
		{
			TranslateMessage(&msg);
			/* Send message to WindowProcedure */
			DispatchMessage(&msg);
		}
	}

	return 0;
}
