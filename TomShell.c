#include <stdlib.h>
#include <stdio.h>
#include <windows.h>

const char g_szClassName[] = "TomsWindowClass";
DWORD screenX, screenY;
COLORREF bgColor, fontColor;
HFONT mainFont, boldFont;
HBRUSH bgBrush;
HPEN linePen;
byte hasPaintedGenInfo = 0;

#define currentPathMaxLen 1024
#define userInputMaxLen 256
int currentPathLen = 0;
char userInputCarat = 0;
char currentPathStr[currentPathMaxLen];
char userInput[userInputMaxLen];

//This was my general overview screen draw code
// {
// 	int text_x = 12;
// 	int text_y = 12;
// 	char monitorX[4];
// 	char monitorY[4];
// 	sprintf(monitorX, "%lu", screenX);
// 	sprintf(monitorY, "%lu", screenY);
// 	const char bufBtwnLines = 4;

// 	SetTextColor(hdc, fontColor);
// 	SelectObject(hdc, mainFont);
// 	TextOut(hdc, text_x, text_y, "Welcome to TomShell v0.1", 24);
// 	text_y += 12 * 2 + bufBtwnLines;

// 	SelectObject(hdc, boldFont);
// 	TextOut(hdc, text_x, text_y, "SYSTEM", 6);

// 	SelectObject(hdc, linePen);
// 	MoveToEx(hdc, text_x + 6 * 12, text_y + 12, NULL);
// 	LineTo(hdc, screenX - 6, text_y + 12);
// 	text_y += 12 + bufBtwnLines;
// }

void DisplayError(const char* message)
{
	MessageBox(NULL, message, "Err", MB_ICONEXCLAMATION | MB_OK);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
		case WM_CLOSE:
		DestroyWindow(hwnd);
		break;

		case WM_DESTROY:
		PostQuitMessage(0);
		break;

		case WM_PAINT:
		{
			HDC hdc;
			PAINTSTRUCT paintStruct;
			HFONT oldFont;

			hdc = BeginPaint(hwnd, &paintStruct);
			oldFont = (HFONT)SelectObject(hdc, bgBrush);
			SetBkColor(hdc, bgColor);
			Rectangle(hdc, 0, 0, screenX, screenY);

	        //File viewing/editting view painting
			{
				HANDLE fileHandle;
				WIN32_FIND_DATA data;
				SelectObject(hdc, mainFont);
				SetTextColor(hdc, fontColor);
				int paintStart_x = 12;
				int paintStart_y = 12;

				TextOut(hdc, paintStart_x, paintStart_y, currentPathStr, currentPathLen + 1);
				paintStart_y += 12 + 4;

				char pathSearch[currentPathMaxLen];
				strcpy(pathSearch, currentPathStr);
				strcat(pathSearch, "/*");

				fileHandle = FindFirstFile(pathSearch, &data);
				TextOut(hdc, paintStart_x, paintStart_y, data.cFileName, strlen(data.cFileName));
				paintStart_y += 12 + 4;
				FindNextFile(fileHandle, &data);
				TextOut(hdc, paintStart_x, paintStart_y, data.cFileName, strlen(data.cFileName));
				paintStart_y += 12 + 4;
				FindNextFile(fileHandle, &data);
				TextOut(hdc, paintStart_x, paintStart_y, data.cFileName, strlen(data.cFileName));
				paintStart_y += 12 + 4;
				FindNextFile(fileHandle, &data);
				TextOut(hdc, paintStart_x, paintStart_y, data.cFileName, strlen(data.cFileName));

				FindClose(fileHandle);
			}

			SelectObject(hdc, oldFont);
			SetTextColor(hdc, RGB(0,0,0));
			EndPaint(hwnd, &paintStruct);

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

	//Initialize my Globals
	bgColor = RGB(49, 39, 50);
	fontColor = RGB(5, 239, 64);
	mainFont = CreateFont(14, 0, 0, 0, FW_THIN, 0, 0, 0, 0, 0, 0, 2, 0, "OCR A Extended");
	boldFont = CreateFont(16, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 2, 0, "OCR A Extended");
	screenX = GetSystemMetrics(SM_CXSCREEN);
	screenY = GetSystemMetrics(SM_CYSCREEN);
	bgBrush = CreateSolidBrush(bgColor);
	linePen = CreatePen(PS_SOLID, 1, fontColor);
	memset(userInput, 0, userInputMaxLen);
	currentPathLen = GetCurrentDirectory(currentPathMaxLen, &currentPathStr);

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
	wc.lpszClassName = g_szClassName;
	wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

	if(!RegisterClassEx(&wc))
	{
		DisplayError("Window Registry Failed");
		return 0;
	}

	hwnd = CreateWindowEx(0, g_szClassName, "THE TITLE I DON'T WANT", WS_BORDER, 0, 0, screenX, screenY, NULL, NULL, hInstance, NULL);
	SetWindowLong(hwnd, GWL_STYLE, 0);

	//if(hwnd == NULL)
	//{
	//	DisplayError("Window Creation Failed");
	//	return 0;
	//}

	//DeleteObject(SelectObject(hdc, font));

	//using SW_SHOWNORMAL rather than nCmdShow
	ShowWindow(hwnd, SW_SHOWNORMAL);
	UpdateWindow(hwnd);

	while(GetMessage(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return msg.wParam;

	return 0;
}

//reminder: how to get working directory.