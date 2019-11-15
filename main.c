#include "main.h"

#define WINDOW_TITLE ("StreamTrigger2")
#define WINDOW_CLASSNAME ("ST2")
#define WINDOW_WIDTH 500
#define WINDOW_HEIGHT 300
#define POLL_TIMER_ID (1)
#define API_POLL_MS 10 * 1000

#define ID_FILE_EXIT                    40001
#define ID_ABOUT_LICENSES               40002


static HWND hGroupBoxTrigger1;
static HWND hStatus;

// VOID(CALLBACK* TIMERPROC)(HWND, UINT, UINT_PTR, DWORD);
void on_timer_expire(HWND hwnd, UINT msg, UINT_PTR timerId, DWORD dwTime)
{
	(void*)hwnd;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_SIZE:
		/* make groupboxes resize with parent - not really used, we disabled resize
		MoveWindow(hGroupBoxTrigger1, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
		*/
		return 0;
	case WM_CREATE:
	{
		HMENU hMenu, hSubMenu;

		hMenu = CreateMenu();

		hSubMenu = CreatePopupMenu();
		AppendMenu(hSubMenu, MF_STRING, ID_FILE_EXIT, "E&xit");
		AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT)hSubMenu, "&File");

		hSubMenu = CreatePopupMenu();
		AppendMenu(hSubMenu, MF_STRING, ID_ABOUT_LICENSES, "&Licenses");
		AppendMenu(hMenu, MF_STRING | MF_POPUP, (UINT)hSubMenu, "&About");

		SetMenu(hwnd, hMenu);

		hStatus = CreateWindowEx(0, STATUSCLASSNAME, NULL,
			WS_CHILD | WS_VISIBLE, 0, 0, 0, 0,
			hwnd, NULL, GetModuleHandle(NULL), NULL);
		
		hGroupBoxTrigger1 = CreateWindowEx(WS_EX_CLIENTEDGE, "BUTTON", "Trigger 1",
			WS_CHILD | WS_VISIBLE | BS_GROUPBOX | WS_GROUP,
			5, 5, 400, 150, hwnd, NULL, GetModuleHandle(NULL), NULL);
				
		HFONT hfDefault;
		HWND hEdit = CreateWindowEx(WS_EX_CLIENTEDGE, "EDIT", "",
			WS_CHILD | WS_VISIBLE,
			5, 20, 200, 20, hGroupBoxTrigger1, NULL, GetModuleHandle(NULL), NULL);
		if (hEdit == NULL)
			MessageBox(hwnd, "Could not create edit box.", "Error", MB_OK | MB_ICONERROR);
			hfDefault = GetStockObject(DEFAULT_GUI_FONT);
		SendMessage(hEdit, WM_SETFONT, (WPARAM)hfDefault, MAKELPARAM(FALSE, 0));

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
		switch (LOWORD(wParam))
		{
		case ID_FILE_EXIT:
			PostMessage(hwnd, WM_CLOSE, 0, 0);
			break;
		case ID_ABOUT_LICENSES:
			MessageBox(hwnd, "licenses", "About Licenses", MB_OK | MB_ICONERROR);
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
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
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