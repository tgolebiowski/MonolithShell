#include <stdio.h>
#include <windows.h>
#include <conio.h>

const char g_szClassName[] = "MonolithShell";
DWORD screenX, screenY;
COLORREF bgColor, fontColor;
HFONT mainFont, boldFont;
HBRUSH bgBrush;
HPEN linePen;
HWND hwnd;

#define currentPathMaxLen 1024
#define userInputMaxLen 256
int currentPathLen = 0;
char currentPathStr[currentPathMaxLen];

unsigned char dirCount = 0;
char dirNameLengths[64];
char dirNames[4 * 1024];

unsigned char fileCount = 0;
char fileNames [4 * 1024];
char fileNameLengths[64];

unsigned char userInputCarat = 0;
char userInput[userInputMaxLen];

RECT consoleRect;
RECT wholeScreen;
char repaintConsoleOnly;
int consolePaintStart_y;

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

void DisplayError( const char* message ) {
	MessageBox( NULL, message, "Err", MB_ICONEXCLAMATION | MB_OK );
}

void DrawUnderlinedTitle( const HDC* hdc, const char* text, const int textlen, int *paintStart_x, int *paintStart_y ) {
	HFONT oldFont = SelectObject( *hdc, boldFont );
	TextOut( *hdc, *paintStart_x, *paintStart_y, text, textlen );
	*paintStart_y += 12 + 4;
	SelectObject( *hdc, linePen );
	MoveToEx( *hdc, *paintStart_x, *paintStart_y, NULL );
	LineTo( *hdc, screenX - 12, *paintStart_y );
	SelectObject( *hdc, oldFont );
	*paintStart_y += 10;
}

void DrawTextList( HDC* hdc, const char* text, const char* lineLengths, const char lineCount, int* paintStart_x, int* paintStart_y ) {
	int i;
	int startIndex = 0;
	for( i = 0; i < lineCount; i++ ) {
		int len = lineLengths[i];
		TextOut( *hdc, *paintStart_x, *paintStart_y, &text[startIndex], len );
		startIndex += len;
		*paintStart_y += 16;
	}
}

void CacheDirContents() {
	HANDLE fileHandle;
	WIN32_FIND_DATA data;
	char* fileTypeHead = &fileNames[0];
	char* dirTypeHead = &dirNames[0];
	char pathSearch[currentPathMaxLen];
	strcpy( pathSearch, currentPathStr );
	strcat( pathSearch, "/*" );

	fileCount = 0;
	dirCount = 0;
	fileHandle = FindFirstFile( pathSearch, &data );
	do{
		int fileNameLen = strlen( data.cFileName );

		if( ( data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) != 0 ) {
			strcpy( dirTypeHead, data.cFileName );
			dirNameLengths[dirCount] = fileNameLen;
			dirTypeHead += fileNameLen;
			dirCount++;
		} else {
			strcpy( fileTypeHead, data.cFileName );
			fileNameLengths[fileCount] = fileNameLen;
			fileTypeHead += fileNameLen;
			fileCount++;
		}

	} while( FindNextFile( fileHandle, &data ) );
	FindClose( fileHandle );

	InvalidateRect( hwnd, &wholeScreen, FALSE );
}

LRESULT CALLBACK WndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam ) {

	switch( msg ) {
		case WM_CLOSE:
		DestroyWindow( hwnd );
		break;

		case WM_DESTROY:
		PostQuitMessage( 0 );
		break;

		case WM_CHAR:
		{
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
					if( userInput[0] == 'c' && userInput[1] == 'd' ) {
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
						}
					}

					int i;
					for( i = 0; i < userInputCarat; i++ ) {
						userInput[i] = 0;
					}
					userInputCarat = 0;

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

		case WM_PAINT:
		{
			HDC hdc;
			PAINTSTRUCT paintStruct;
			HFONT oldFont;

			hdc = BeginPaint( hwnd, &paintStruct );
			oldFont = (HFONT)SelectObject( hdc, bgBrush );
			SetBkColor( hdc, bgColor );
			Rectangle( hdc, 0, 0, screenX, screenY );

	        //File viewing/mgmt mode painting
			{
				SelectObject( hdc, mainFont );
				SetTextColor( hdc, fontColor );
				if(repaintConsoleOnly) goto DrawConsoleLabel;
				int paintStart_x = 12;
				int paintStart_y = 12;

	            //draw info about current directory
	            SelectObject( hdc, boldFont );
				TextOut( hdc, paintStart_x, paintStart_y, "Current Directory:", 18 );
				paintStart_x += 18 * 12 + 4;
				SelectObject( hdc, mainFont );
				TextOut( hdc, paintStart_x, paintStart_y, currentPathStr, currentPathLen + 1 );
				paintStart_y += 12 + 4;
				paintStart_x = 12;

				SelectObject( hdc, linePen );
				MoveToEx( hdc, paintStart_x, paintStart_y, NULL );
				LineTo( hdc, screenX - 12, paintStart_y );
				SelectObject( hdc, mainFont );

				paintStart_y += 14;

				//draw sub directories in current directory
				DrawUnderlinedTitle(&hdc, "Sub Directories", 16, &paintStart_x, &paintStart_y);
				DrawTextList(&hdc, dirNames, dirNameLengths, dirCount, &paintStart_x, &paintStart_y);

				paintStart_y += 14;

	            //draw file names in current directory
				DrawUnderlinedTitle(&hdc, "Files", 6, &paintStart_x, &paintStart_y);
				DrawTextList(&hdc, fileNames, fileNameLengths, fileCount, &paintStart_x, &paintStart_y);

	            //draw Console and user input
	            paintStart_y = consolePaintStart_y - 18;
				DrawUnderlinedTitle(&hdc, "Console", 7, &paintStart_x, &paintStart_y);
				DrawConsoleLabel:
				paintStart_x = 12; 
				paintStart_y = consolePaintStart_y;
				TextOut(hdc, 12, consolePaintStart_y, userInput, userInputCarat);
				repaintConsoleOnly = 0;
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
	MSG msg;

	//Initialize my Globals
	memset(userInput, 0, userInputMaxLen);
	memset(fileNameLengths, 0, 256);
	memset(fileNames, 0, 1024);
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
	consoleRect.bottom = consolePaintStart_y + 24;
	wholeScreen.top = 0;
	wholeScreen.bottom = screenY;
	wholeScreen.left = 0;
	wholeScreen.right = screenX;
	repaintConsoleOnly = 0;

	CacheDirContents();

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