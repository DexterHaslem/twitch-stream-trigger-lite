#include "main.h"

#define WINDOW_TITLE ("Twitch Stream Trigger v2")
#define WINDOW_CLASSNAME ("ST2")
#define WINDOW_WIDTH 380
#define WINDOW_HEIGHT 550
#define POLL_TIMER_ID (1)
#define API_POLL_MS 1 * 1000

/* this coudl be dynamic but just hardcode enough for now */
#define NUM_HARDCODED_TRIGGERS 4

#define ID_FILE_EXIT        40001
#define ID_HELP_LICENSES    40002
#define ID_HELP_ABOUT		40003

static HWND hStatus;
struct stream_trigger triggers[NUM_HARDCODED_TRIGGERS];

void on_timer_expire(HWND hwnd, UINT msg, UINT_PTR timerId, DWORD dwTime)
{
	
}

void create_trigger_group(struct stream_trigger *trigger, int start_y, HWND owner)
{
	HINSTANCE hInstance = GetModuleHandle(NULL);// (HINSTANCE)GetWindowLong(owner, GWL_HINSTANCE);
	
	char titlebuf[256];
	sprintf_s(&titlebuf, 256, "Trigger %d", trigger->serialize.num);

	HWND hGroupBoxTrigger = CreateWindowEx(WS_EX_TRANSPARENT, WC_BUTTON, titlebuf,
		WS_CHILD | WS_VISIBLE | BS_GROUPBOX | WS_GROUP,
		5, start_y, 320, 100, owner, NULL, hInstance, NULL);
	
	/* TODO: set initial checked to state */
	HWND hEnabled = CreateWindowEx(0, WC_BUTTON, "Enabled", WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX,
		10, start_y + 15, 75, 25, owner, (HMENU) trigger->enabledCheckboxId, hInstance, NULL);
	trigger->hEnabledCheckbox = hEnabled;

	HWND hAccountLabel = CreateWindowEx(0, WC_STATIC, "Account:", WS_CHILD | WS_VISIBLE, 
		10, start_y + 45, 100, 25, owner,
		NULL, hInstance, NULL);
	
	HWND hEditAccount = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "Account here",
		WS_CHILD | WS_VISIBLE,
		75, start_y + 40, 230, 25, owner, NULL, hInstance, NULL);
	trigger->hEditAccount = hEditAccount;

	HWND hCmdLabel = CreateWindowEx(0, WC_STATIC, "Command:", WS_CHILD | WS_VISIBLE,
		10, start_y + 75, 100, 25, owner,
		NULL, hInstance, NULL);

	HWND hEditCommandLine = CreateWindowEx(WS_EX_CLIENTEDGE, WC_EDIT, "Command line here",
		WS_CHILD | WS_VISIBLE,
		75, start_y + 70, 230, 25, owner, NULL, hInstance, NULL);
	trigger->hEditCommand = hEditCommandLine;

	HFONT hfDefault = GetStockObject(DEFAULT_GUI_FONT);
	SendMessage(hGroupBoxTrigger, WM_SETFONT, (WPARAM)hfDefault, MAKELPARAM(FALSE, 0));
	SendMessage(hEnabled, WM_SETFONT, (WPARAM)hfDefault, MAKELPARAM(FALSE, 0));
	SendMessage(hAccountLabel, WM_SETFONT, (WPARAM)hfDefault, MAKELPARAM(FALSE, 0));
	SendMessage(hCmdLabel, WM_SETFONT, (WPARAM)hfDefault, MAKELPARAM(FALSE, 0));
	SendMessage(hEditCommandLine, WM_SETFONT, (WPARAM)hfDefault, MAKELPARAM(FALSE, 0));
	SendMessage(hEditAccount, WM_SETFONT, (WPARAM)hfDefault, MAKELPARAM(FALSE, 0));
}

static void setup_triggers()
{
	for (int i = 0; i < NUM_HARDCODED_TRIGGERS; ++i)
	{
		/* NOTE: do not overlap resource ids if anything else gets added */
		triggers[i].enabledCheckboxId = 50000 + i;
		triggers[i].serialize.num = i + 1;
	}

	/* TODO: restoration of account/cmd here */
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
		AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT)hSubMenu, "&File");

		hSubMenu = CreatePopupMenu();
		AppendMenu(hSubMenu, MF_STRING, ID_HELP_LICENSES, "&Licenses");
		AppendMenu(hSubMenu, MF_STRING, ID_HELP_ABOUT, "&About");
		AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT)hSubMenu, "&Help");

		SetMenu(hwnd, hMenu);

		hStatus = CreateWindowEx(0, STATUSCLASSNAME, NULL,
			WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
			hwnd, NULL, GetModuleHandle(NULL), NULL);
		
		setup_triggers();

		for (int i = 0; i < NUM_HARDCODED_TRIGGERS; ++i)
		{
			create_trigger_group(&triggers[i], (i * 110 + 5), hwnd);
		}

		/* start polling timer now */
		SetTimer(hwnd, POLL_TIMER_ID, API_POLL_MS, on_timer_expire);
		break;
	}
	case WM_CLOSE:
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
			/* check if it was one of our trigger enable checkboxes */
			for (int i = 0; i < NUM_HARDCODED_TRIGGERS; ++i)
			{
				if (from == triggers[i].enabledCheckboxId)
				{
					if (HIWORD(wParam) == BN_CLICKED)
					{
						/* have to ask current state everytime */
						bool enabled = SendDlgItemMessage(hwnd, from, BM_GETCHECK, 0, 0);
						triggers[i].serialize.enabled = enabled;
					}
					break;
				}
			}
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
	HWND hwnd;
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
		return 0;
	}

	hwnd = CreateWindowEx(
		WS_EX_CLIENTEDGE,
		WINDOW_CLASSNAME,
		WINDOW_TITLE,
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU,
		CW_USEDEFAULT, CW_USEDEFAULT, WINDOW_WIDTH, WINDOW_HEIGHT,
		NULL, NULL, hInstance, NULL);

	if (hwnd == NULL)
	{
		MessageBox(NULL, "Window Creation Failed!", "Error!",
			MB_ICONEXCLAMATION | MB_OK);
		return 0;
	}

	ShowWindow(hwnd, nCmdShow);
	UpdateWindow(hwnd);

	while (GetMessage(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return msg.wParam;
}