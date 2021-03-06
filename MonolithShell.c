#include <stdio.h>
#include <windows.h>
#include <conio.h>
#include <psapi.h>

const char g_szClassName[] = "MonolithShell";
DWORD screenX, screenY;
COLORREF bgColor, fontColor;
HFONT mainFont, boldFont;
HBRUSH bgBrush;
HPEN linePen;
HWND hwnd;
HINSTANCE myInstance;

#define currentPathMaxLen 1024
#define userInputMaxLen 256
#define MAX_CONSOLE_LINES 4
#define OVERVIEW_MODE 1
#define FILE_MGMT_MODE 2
#define PROCESS_MGMT_MODE 3

typedef struct StringList{
	unsigned int count;
	char strlens[64];
	char buffer[4 * 1024];
}StringList;

typedef struct PixelCoords{
	int x;
	int y;
}PixelCoords;

//view mode
char mode;

//repaint rects
RECT consoleRect;
RECT wholeScreen;

//General console stuff
unsigned char userInputCarat = 0;
char userInput[userInputMaxLen];
char pastConsoleLines[MAX_CONSOLE_LINES][userInputMaxLen];
char repaintConsoleOnly;
int consolePaintStart_y;

//File management stuff
int currentPathLen = 0;
char currentPathStr[currentPathMaxLen];
StringList dirsList;
StringList filesList;

//Process management stuff
StringList processList;


void DisplayError( const char* message ) {
	MessageBox( NULL, message, "Err", MB_ICONEXCLAMATION | MB_OK );
}

void PushMessageToConsole(const char* msg) {
	int i;
	for(i = MAX_CONSOLE_LINES - 1; i > 0; i--) {
		strcpy(&pastConsoleLines[i][0], &pastConsoleLines[i- 1][0]);
	}
	strcpy(&pastConsoleLines[0][0], msg);
}

void CacheDirContents() {
	HANDLE fileHandle;
	WIN32_FIND_DATA data;
	char* fileTypeHead = &filesList.buffer[0];
	char* dirTypeHead = &dirsList.buffer[0];
	char pathSearch[currentPathMaxLen];
	strcpy( pathSearch, currentPathStr );
	strcat( pathSearch, "/*" );

	filesList.count = 0;
	dirsList.count = 0;
	fileHandle = FindFirstFile( pathSearch, &data );
	do{
		int fileNameLen = strlen( data.cFileName );

		if( ( data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) != 0 ) {
			strcpy( dirTypeHead, data.cFileName );
			dirsList.strlens[dirsList.count] = fileNameLen;
			dirsList.count++;
			dirTypeHead += fileNameLen;
		} else {
			strcpy( fileTypeHead, data.cFileName );
			filesList.strlens[filesList.count] = fileNameLen;
			filesList.count++;
			fileTypeHead += fileNameLen;
		}

		if((fileTypeHead - &filesList.buffer[0]) > 4 * 1024 || ((dirTypeHead - &dirsList.buffer[0]) > 4 * 1024)) break;
	} while( FindNextFile( fileHandle, &data ) && dirsList.count >= 256 && filesList.count >= 256);
	FindClose( fileHandle );

	InvalidateRect( hwnd, &wholeScreen, FALSE );
}

void CacheProcessesList() {
	HANDLE process;
	int processes[1024];
	DWORD processListSize, bytesReturned, processCount;
	char processName[256];
	char* processNameTypeHead = &processList.buffer[0];
	processListSize = sizeof(processes);
	EnumProcesses(processes, processListSize, &bytesReturned);
	processCount = bytesReturned / sizeof(int);
	processList.count = 0;

	unsigned int i;
	for(i = 0; i < processCount; i++) {
		memset(&processName[0], 0, 256);
		process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processes[i]);

		if(process != NULL){
			DWORD cbNeeded;
			HMODULE hMod;
			if(EnumProcessModules(process, hMod, sizeof(HINSTANCE), &cbNeeded)) {
				GetModuleBaseName(process, NULL, &processName[0], 256);
				int nameLen = strlen(&processName[0]);
				strcpy(processNameTypeHead, &processName[0]);
				processNameTypeHead += nameLen;
				processList.strlens[processList.count] = nameLen;
				processList.count++;

				printf(&processName[0]); printf("\n");
			}
		}


		CloseHandle(process);
	}
}

void DrawUnderlinedTitle( const HDC* hdc, const char* text, const int textlen, PixelCoords* paintStart ) {
	HFONT oldFont = SelectObject( *hdc, boldFont );
	TextOut( *hdc, paintStart->x, paintStart->y, text, textlen );
	paintStart->y += 12 + 4;
	SelectObject( *hdc, linePen );
	MoveToEx( *hdc, paintStart->x, paintStart->y, NULL );
	LineTo( *hdc, screenX - 12, paintStart->y );
	SelectObject( *hdc, oldFont );
	paintStart->y += 10;
}

void DrawTextList( HDC* hdc, const char* text, const char* lineLengths, const char lineCount, const int botCutoff, PixelCoords* paintStart) {
	int i;
	int startIndex = 0;
	for( i = 0; i < lineCount && paintStart->y < botCutoff; i++ ) {
		int len = lineLengths[i];
		TextOut( *hdc, paintStart->x, paintStart->y, &text[startIndex], len );
		startIndex += len;
		paintStart->y += 16;
	}
}

void ProcessCommand_FileMgmt() {
	if( userInput[0] == 'c' && userInput[1] == 'd' ) { //Change directory
		char newDir [MAX_PATH];
		char* catPath = &userInput[3];
		int newPathLen;
		strcpy( &newDir[0], &currentPathStr[0] );

		if( userInput[3] == '.' && userInput[4] == '.' ) {
			int i = currentPathLen;
			newPathLen = currentPathLen;
			while( newDir[i] != '\\' ) {
				newDir[i] = 0;
				newPathLen--;
				i--;
			}
			newDir[i] = 0;
		} else {
            //build new dir string
			strcat( &newDir[currentPathLen], "\\" );
			strcat( &newDir[currentPathLen + 1], catPath );
			newPathLen = currentPathLen + 1 + userInputCarat - 3;
		}

	    //attempt change
		if( SetCurrentDirectory( &newDir[0] ) ) {  
		    //update state if change succeeded
			memset( currentPathStr, 0, currentPathMaxLen );
			memcpy( currentPathStr, newDir, newPathLen );
			currentPathLen = newPathLen;
			CacheDirContents();
			PushMessageToConsole(&userInput[0]);
			memset(userInput, 0, userInputCarat);
			userInputCarat = 0;
		} else {
			char error [MAX_PATH];
			strcpy(&error[0], "could not find directory: ");
			strcat(&error[25], &newDir[0]);
			PushMessageToConsole(&error[0]);
			InvalidateRect( hwnd, &consoleRect, FALSE );
			repaintConsoleOnly = 1;
			PushMessageToConsole(&userInput[0]);
			memset(userInput, 0, userInputCarat);
			userInputCarat = 0;
		}
	} else if (userInput[0] == 'r' && userInput[1] == 'n') { //Rename file
		char srcTypeSuffix [8];
		char newNameSuffix [8];
		char srcFile [MAX_PATH];
		char newName [MAX_PATH];
		int splitIndex, i, srcNameLen, newNameLen;
		for(i = 2; i < userInputCarat; i++) { //figure out where in user input are the two file names
			if(userInput[i] == '-' && userInput[i + 1] == '>') {
				splitIndex = i;
				break;
			}
		}
		srcNameLen = splitIndex - 3;
		newNameLen = userInputCarat - (splitIndex + 2);
		memset(&srcFile[0], 0, MAX_PATH);
		memset(&newName[0], 0, MAX_PATH);
		memcpy(&srcFile[0], &userInput[3], srcNameLen);
		memcpy(&newName[0], &userInput[splitIndex + 2], newNameLen);

		memset(&srcTypeSuffix[0], 0, 8);
		memset(&newNameSuffix[0], 0, 8);
		for(i = srcNameLen; i > 0; i--) {
			if(srcFile[i] == '.') {
				memcpy(&srcTypeSuffix[0], &srcFile[i], srcNameLen - i);
				break;
			}
		}
		for(i = newNameLen; i > 0; i--) {
			if(newName[i] == '.') {
				memcpy(&newNameSuffix[0], &newName[i], newNameLen - i);
				break;
			}
		}
		if(newNameSuffix[0] == 0 && srcTypeSuffix[0] != 0) {
		    strcat(newName, srcTypeSuffix); //if no suffix given in new name, use old suffix
		}

		if(MoveFile(&srcFile[0], &newName[0])) {
			CacheDirContents();
			PushMessageToConsole(&userInput[0]);
			memset(userInput, 0, userInputCarat);
			userInputCarat = 0;
		} else {
			PushMessageToConsole(&userInput[0]);
			memset(userInput, 0, userInputCarat);
			userInputCarat = 0;
			PushMessageToConsole("could not rename");
			InvalidateRect( hwnd, &consoleRect, FALSE );
			repaintConsoleOnly = 1;
		}
	} else if (userInput[0] == 's' && userInput[1] == 'w') { //switch to overview
		mode = OVERVIEW_MODE;
		InvalidateRect(hwnd, &wholeScreen, FALSE);
		PushMessageToConsole(&userInput[0]);
		memset(userInput, 0, userInputCarat);
		userInputCarat = 0;
	}
}

void ProcessCommand_Overview() {
	if(userInput[0] == 's' && userInput[1] == 'w') {
		mode = FILE_MGMT_MODE;
		PushMessageToConsole(&userInput[0]);
		memset(userInput, 0, userInputCarat);
		userInputCarat = 0;
		InvalidateRect(hwnd, &wholeScreen, FALSE);
	}
}

void Paint_FileMgmtMode(HDC hdc) {
	SelectObject( hdc, mainFont );
	SetTextColor( hdc, fontColor );

	PixelCoords paintStart;
	paintStart.x = 12;
	paintStart.y = 12;

	//draw basic info about current directory
	SelectObject( hdc, boldFont );
	TextOut( hdc, paintStart.x, paintStart.y, "Current Directory:", 18 );
	paintStart.x += 18 * 12 + 4;
	SelectObject( hdc, mainFont );
	TextOut( hdc, paintStart.x, paintStart.y, currentPathStr, currentPathLen + 1 );
	paintStart.x = 12;
	paintStart.y += 12 + 4;

    //underline
	SelectObject( hdc, linePen );
	MoveToEx( hdc, paintStart.x, paintStart.y, NULL );
	LineTo( hdc, screenX - 12, paintStart.y );
	SelectObject( hdc, mainFont );
	paintStart.y += 14;

	paintStart.y += 12; //a little top breathing room
    //Draw titles for Sub Directories and Files lists
	SelectObject(hdc, boldFont);
	TextOut(hdc, paintStart.x, paintStart.y, "Sub Directories", 15);
	paintStart.x = (screenX / 2) + 12;
	TextOut(hdc, paintStart.x, paintStart.y, "Files", 5);
	paintStart.x = 12;
	paintStart.y += 16;

    //underline
	SelectObject( hdc, linePen );
	MoveToEx( hdc, paintStart.x, paintStart.y, NULL );
	LineTo( hdc, screenX - 12, paintStart.y );
	SelectObject( hdc, mainFont );
	paintStart.y += 8;

	//Draw dividing line btwn lists
	SelectObject( hdc, linePen );
	MoveToEx( hdc, screenX / 2, paintStart.y, NULL );
	LineTo( hdc, screenX / 2, consolePaintStart_y - 12 - 8 );
	SelectObject( hdc, mainFont );

	//draw sub directories list
	int paintStartY_pos = paintStart.y; //save paintstart y position
	DrawTextList(&hdc, &dirsList.buffer[0], &dirsList.strlens[0], dirsList.count, consolePaintStart_y - 18 -16, &paintStart);
	paintStart.y = paintStartY_pos; //reset paintstart.y's carry
	//draw file names list
	paintStart.x = (screenX / 2) + 10; //move drawing to that side
	DrawTextList(&hdc, &filesList.buffer[0], &filesList.strlens[0], filesList.count, consolePaintStart_y - 18 - 16, &paintStart);
}

void Paint_Overview(HDC hdc){
	int text_x = 12;
	int text_y = 12;
	char monitorX[4];
	char monitorY[4];
	sprintf(monitorX, "%lu", screenX);
	sprintf(monitorY, "%lu", screenY);
	const char bufBtwnLines = 4;

	SetTextColor(hdc, fontColor);
	SelectObject(hdc, mainFont);
	TextOut(hdc, text_x, text_y, "Welcome to MonolithShell v0.1", 24);
	text_y += 12 * 2 + bufBtwnLines;

	SelectObject(hdc, boldFont);
	TextOut(hdc, text_x, text_y, "SYSTEM", 6);

	SelectObject(hdc, linePen);
	MoveToEx(hdc, text_x + 6 * 12, text_y + 12, NULL);
	LineTo(hdc, screenX - 6, text_y + 12);
	text_y += 12 + bufBtwnLines;
}

void Paint_ProcessView(HDC hdc) {
	SelectObject(hdc, mainFont);
	SetTextColor(hdc, fontColor);

	PixelCoords paintStart;
	paintStart.x = 12;
	paintStart.y = 12;

	TextOut(hdc, paintStart.x, paintStart.y, "Processes", 9);
	DrawTextList(&hdc, &processList.buffer[0], &processList.strlens[0], processList.count, consolePaintStart_y - 18 - 16, &paintStart);
}

LRESULT CALLBACK WndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam ) {

	switch( msg ) {
		case WM_CLOSE:
		DestroyWindow( hwnd );
		break;

		case WM_DESTROY:
		PostQuitMessage( 0 );
		break;

		case WM_CHAR: {
			char keypress;

			if(userInputCarat >= 254) {
				repaintConsoleOnly = 0;
				break;
			} 

			keypress = wParam;
			switch( keypress ) {
				case 8:
			    //backspace
			    if( userInputCarat > 0 ) {
			    	userInput[userInputCarat] = 0;
			    	userInputCarat--;
			    	InvalidateRect( hwnd, &consoleRect, FALSE );
			    	repaintConsoleOnly = 1;
			    }
				break;

				case 13:
				{   //enter
					//process input
					switch(mode) {
						case OVERVIEW_MODE:
						ProcessCommand_Overview();
						break;

						case FILE_MGMT_MODE:
						ProcessCommand_FileMgmt();
						break;

						case PROCESS_MGMT_MODE:

						break;
					}
					break;
				}
				default:
				if(userInputCarat <= 255) {
					userInput[userInputCarat] = keypress;
					userInputCarat++;
					InvalidateRect( hwnd, &consoleRect, FALSE );
			    	repaintConsoleOnly = 1;
				}
				break;
			}
		}

		case WM_PAINT: {
			HDC hdc;
			PAINTSTRUCT paintStruct;
			HFONT oldFont;

			hdc = BeginPaint( hwnd, &paintStruct );
			oldFont = (HFONT)SelectObject( hdc, bgBrush );
			SetBkColor( hdc, bgColor );
			Rectangle( hdc, 0, 0, screenX, screenY );
			if(repaintConsoleOnly) goto paintconsole;

	        switch(mode) {
	        	case FILE_MGMT_MODE:
	        	Paint_FileMgmtMode(hdc);
	        	break;

	        	case OVERVIEW_MODE:
	        	Paint_Overview(hdc);
	        	break;

	        	case PROCESS_MGMT_MODE:
	        	Paint_ProcessView(hdc);
	        	break;
	        }

	        paintconsole:

	        SelectObject(hdc, boldFont);
	        SetTextColor(hdc, fontColor);
	        PixelCoords paintStart;
	        paintStart.x = 12; 
	        paintStart.y = consolePaintStart_y;
	        DrawUnderlinedTitle(&hdc, "Console", 7, &paintStart);
	        SelectObject(hdc, mainFont);
	        TextOut(hdc, paintStart.x, paintStart.y, userInput, userInputCarat);
	        paintStart.y += 16;

	        int i;
	        int consoleLineLen;
	        for(i = 0; i < MAX_CONSOLE_LINES; i++) {
	        	consoleLineLen = strlen(&pastConsoleLines[i][0]);
	        	TextOut(hdc, paintStart.x, paintStart.y, &pastConsoleLines[i][0], consoleLineLen);
	        	paintStart.y += 16;
	        }
	        repaintConsoleOnly = 0;

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

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
	myInstance = hInstance;
	WNDCLASSEX wc;
	MSG msg;

	//Initialize my Globals
	memset(userInput, 0, userInputMaxLen);
	memset(pastConsoleLines, 0, MAX_CONSOLE_LINES * userInputMaxLen);
	memset(&processList.buffer[0], 0, 4 * 1024);
	screenX = GetSystemMetrics(SM_CXSCREEN);
	screenY = GetSystemMetrics(SM_CYSCREEN);
	currentPathLen = GetCurrentDirectory(currentPathMaxLen, &currentPathStr[0]);
	bgColor = RGB(49, 39, 50);
	fontColor = RGB(5, 239, 64);
	mainFont = CreateFont(14, 0, 0, 0, FW_THIN, 0, 0, 0, 0, 0, 0, 2, 0, "OCR A Extended");
	boldFont = CreateFont(16, 0, 0, 0, FW_BOLD, 0, 0, 0, 0, 0, 0, 2, 0, "OCR A Extended");
	bgBrush = CreateSolidBrush(bgColor);
	linePen = CreatePen(PS_SOLID, 1, fontColor);
	consolePaintStart_y = 600;
	consoleRect.top = consolePaintStart_y;
	consoleRect.left = 12;
	consoleRect.right = 12 + 10 * userInputMaxLen;
	consoleRect.bottom = consolePaintStart_y + (MAX_CONSOLE_LINES + 1) * 16;
	wholeScreen.top = 0;
	wholeScreen.bottom = screenY;
	wholeScreen.left = 0;
	wholeScreen.right = screenX;
	repaintConsoleOnly = 0;
	mode = PROCESS_MGMT_MODE;

	CacheDirContents();
	CacheProcessesList();

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

	if(!RegisterClassEx(&wc)) {
		DisplayError("Window Registry Failed");
		return 0;
	}

	hwnd = CreateWindowEx(0, g_szClassName, "title", WS_BORDER, 0, 0, screenX, screenY, NULL, NULL, hInstance, NULL);
	SetWindowLong(hwnd, GWL_STYLE, 0);
	//GetClientRect(hwnd, &rect);

	//using SW_SHOWNORMAL rather than nCmdShow
	ShowWindow(hwnd, SW_SHOWNORMAL);
	UpdateWindow(hwnd);

	while(GetMessage(&msg, NULL, 0, 0) > 0)	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return msg.wParam;
}