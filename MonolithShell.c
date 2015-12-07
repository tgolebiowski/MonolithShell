#include <stdlib.h>
#include <stdio.h>
#include <windows.h>

const char g_szClassName[] = "MonolithShellClass";
DWORD screenX, screenY;
COLORREF bgColor, fontColor;
HFONT mainFont, boldFont;
HBRUSH bgBrush;
HPEN linePen;

#define currentPathMaxLen 1024
#define userInputMaxLen 256
int currentPathLen = 0;
char currentPathStr[currentPathMaxLen];

char dirCount = 0;
char dirNameLengths[128];
char dirNames[1024];

char fileCount = 0;
char fileNameLengths[128];
char fileNames [1024];

char userInputCarat = 0;
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
// 	TextOut(hdc, text_x, text_y, "Welcome to MonolithShell v0.1", 24);
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

void DrawUnderlinedTitle(HDC* hdc, int* drawCursorX, int* drawCursorY, char* text, int textlen)
{
	HFONT oldFont = SelectObject(*hdc, boldFont);
	TextOut(*hdc, *drawCursorX, *drawCursorY, text, textlen);
	*drawCursorY += 12 + 4;
	SelectObject(*hdc, linePen);
	MoveToEx(*hdc, *drawCursorX, *drawCursorY, NULL);
	LineTo(*hdc, screenX - 12, *drawCursorY);
	SelectObject(*hdc, oldFont);
	*drawCursorY += 8;
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

	        //File viewing/mgmt mode painting
			{
				SelectObject(hdc, mainFont);
				SetTextColor(hdc, fontColor);
				int paintStart_x = 12;
				int paintStart_y = 12;

	            //draw info regarding current dir
	            SelectObject(hdc, boldFont);
				TextOut(hdc, paintStart_x, paintStart_y, "Current Directory:", 18);
				paintStart_x += 18 * 12 + 4;
				SelectObject(hdc, mainFont);
				TextOut(hdc, paintStart_x, paintStart_y, currentPathStr, currentPathLen + 1);
				paintStart_y += 12 + 4;
				paintStart_x = 12;

				SelectObject(hdc, linePen);
				MoveToEx(hdc, paintStart_x, paintStart_y, NULL);
				LineTo(hdc, screenX - 12, paintStart_y);
				SelectObject(hdc, mainFont);
				paintStart_y += 14;

				DrawUnderlinedTitle(&hdc, &paintStart_x, &paintStart_y, "Sub Directories", 16);

				//draw sub directories in current directory
				int i;
				int dirNameStartIndex = 0;
				for(i = 0; i < dirCount; i++)
				{
					int dirNameLen = dirNameLengths[i];
					TextOut(hdc, paintStart_x, paintStart_y, &dirNames[dirNameStartIndex], dirNameLen);
					dirNameStartIndex += dirNameLen;
					paintStart_y += 12 + 4;
				}

				paintStart_y += 12;
				DrawUnderlinedTitle(&hdc, &paintStart_x, &paintStart_y, "Files", 6);

	            //draw file names in current directory
				int fileNameStartIndex = 0;
				for(i = 0; i < fileCount; i++)
				{
					int fileNameLen = fileNameLengths[i];
					TextOut(hdc, paintStart_x, paintStart_y, &fileNames[fileNameStartIndex], fileNameLen);
					fileNameStartIndex += fileNameLen;
					paintStart_y += 12 + 4;
				}
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
	memset(userInput, 0, userInputMaxLen);
	memset(fileNameLengths, 0, 256);
	memset(fileNames, 0, 1024);
	screenX = GetSystemMetrics(SM_CXSCREEN);
	screenY = GetSystemMetrics(SM_CYSCREEN);
	currentPathLen = GetCurrentDirectory(currentPathMaxLen, &currentPathStr);
	bgColor = RGB(49, 39, 50);
	fontColor = RGB(5, 239, 64);
	mainFont = CreateFont(14, 0, 0, 0, FW_THIN, 0, 0, 0, 0, 0, 0, 2, 0, "OCR A Extended");
	boldFont = CreateFont(16, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 2, 0, "OCR A Extended");
	bgBrush = CreateSolidBrush(bgColor);
	linePen = CreatePen(PS_SOLID, 1, fontColor);

	//get all files in directory and cache for later
	{
		HANDLE fileHandle;
		WIN32_FIND_DATA data;
		char* fileTypeHead = &fileNames;
		char* dirTypeHead = &dirNames;
		char pathSearch[currentPathMaxLen];
		strcpy(pathSearch, currentPathStr);
		strcat(pathSearch, "/*");

		fileCount = 0;
		dirCount = 0;
		fileHandle = FindFirstFile(pathSearch, &data);
		do{
			int fileNameLen = strlen(data.cFileName);

			if((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
			{
				strcpy(dirTypeHead, data.cFileName);
				dirNameLengths[dirCount] = fileNameLen;
				dirTypeHead += fileNameLen;
				dirCount++;
			}
			else
			{
				strcpy(fileTypeHead, data.cFileName);
				fileNameLengths[fileCount] = fileNameLen;
				fileTypeHead += fileNameLen;
				fileCount++;
			}

		} while(FindNextFile(fileHandle, &data));
		FindClose(fileHandle);
	}

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

	hwnd = CreateWindowEx(0, g_szClassName, "title", WS_BORDER, 0, 0, screenX, screenY, NULL, NULL, hInstance, NULL);
	SetWindowLong(hwnd, GWL_STYLE, 0);

	//using SW_SHOWNORMAL rather than nCmdShow
	ShowWindow(hwnd, SW_SHOWNORMAL);
	UpdateWindow(hwnd);

	while(GetMessage(&msg, NULL, 0, 0) > 0)
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return msg.wParam;
}