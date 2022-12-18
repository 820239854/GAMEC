#include <stdio.h>

#pragma warning(push, 3)

#include <windows.h>

#pragma warning(pop)

#include <stdint.h>

#include "Main.h"

HWND gGameWindow;

BOOL gGameIsRunning;

GAMEBITMAP gBackBuffer;

GAMEPERFDATA gPerformanceData;


int __stdcall WinMain(HINSTANCE Instance, HINSTANCE PreviousInstance, PSTR CommandLine, INT CmdShow)
{
	UNREFERENCED_PARAMETER(Instance);

	UNREFERENCED_PARAMETER(PreviousInstance);

	UNREFERENCED_PARAMETER(CommandLine);

	UNREFERENCED_PARAMETER(CmdShow);

	MSG Message = { 0 };

	int64_t FrameStart = 0;

	int64_t FrameEnd = 0;

	int64_t ElapsedMicrosecondsPerFrame;

	int64_t ElapsedMicrosecondsPerFrameAccumulatorRaw = 0;

	int64_t ElapsedMicrosecondsPerFrameAccumulatorCooked = 0;


	if (GameIsAlreadyRunning() == TRUE)
	{
		MessageBoxA(NULL, "Another instance of this program is already running!", "Error!", MB_ICONEXCLAMATION | MB_OK);

		goto Exit;
	}

	if (CreateMainGameWindow() != ERROR_SUCCESS)
	{
		goto Exit;
	}

    
	QueryPerformanceFrequency((LARGE_INTEGER*)&gPerformanceData.PerfFrequency);

    

	gBackBuffer.BitmapInfo.bmiHeader.biSize = sizeof(gBackBuffer.BitmapInfo.bmiHeader);

	gBackBuffer.BitmapInfo.bmiHeader.biWidth = GAME_RES_WIDTH;

	gBackBuffer.BitmapInfo.bmiHeader.biHeight = GAME_RES_HEIGHT;

	gBackBuffer.BitmapInfo.bmiHeader.biBitCount = GAME_BPP;

	gBackBuffer.BitmapInfo.bmiHeader.biCompression = BI_RGB;

	gBackBuffer.BitmapInfo.bmiHeader.biPlanes = 1;

	gBackBuffer.Memory = VirtualAlloc(NULL, GAME_DRAWING_AREA_MEMORY_SIZE, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

	if (gBackBuffer.Memory == NULL)
	{
		MessageBoxA(NULL, "Failed to allocate memory for drawing surface!", "Error!", MB_ICONEXCLAMATION | MB_OK);

		goto Exit;
	}

	memset(gBackBuffer.Memory, 0x7F, GAME_DRAWING_AREA_MEMORY_SIZE);

    

	gGameIsRunning = TRUE;

	while (gGameIsRunning == TRUE)
	{
		QueryPerformanceCounter((LARGE_INTEGER*)&FrameStart);

		while (PeekMessageA(&Message, gGameWindow, 0, 0, PM_REMOVE))
		{
			DispatchMessageA(&Message);
		}

		ProcessPlayerInput();

		RenderFrameGraphics();

		QueryPerformanceCounter((LARGE_INTEGER*)&FrameEnd);

		ElapsedMicrosecondsPerFrame = FrameEnd - FrameStart;

		ElapsedMicrosecondsPerFrame *= 1000000;

		ElapsedMicrosecondsPerFrame /= gPerformanceData.PerfFrequency;

		gPerformanceData.TotalFramesRendered++;

		ElapsedMicrosecondsPerFrameAccumulatorRaw += ElapsedMicrosecondsPerFrame;

		while (ElapsedMicrosecondsPerFrame <= TARGET_MICROSECONDS_PER_FRAME)
		{
			Sleep(0);

			ElapsedMicrosecondsPerFrame = FrameEnd - FrameStart;

			ElapsedMicrosecondsPerFrame *= 1000000;

			ElapsedMicrosecondsPerFrame /= gPerformanceData.PerfFrequency;

			QueryPerformanceCounter((LARGE_INTEGER*)&FrameEnd);
		}

		ElapsedMicrosecondsPerFrameAccumulatorCooked += ElapsedMicrosecondsPerFrame;



		if ((gPerformanceData.TotalFramesRendered % CALCULATE_AVG_FPS_EVERY_X_FRAMES) == 0)
		{
			int64_t AverageMicrosecondsPerFrameRaw = ElapsedMicrosecondsPerFrameAccumulatorRaw / CALCULATE_AVG_FPS_EVERY_X_FRAMES;

			int64_t AverageMicrosecondsPerFrameCooked = ElapsedMicrosecondsPerFrameAccumulatorCooked / CALCULATE_AVG_FPS_EVERY_X_FRAMES;

			gPerformanceData.RawFPSAverage = 1.0f / ((ElapsedMicrosecondsPerFrameAccumulatorRaw / 60) * 0.000001f);

			gPerformanceData.CookedFPSAverage = 1.0f / ((ElapsedMicrosecondsPerFrameAccumulatorCooked / 60) * 0.000001f);

            
			char str[256] = { 0 };

			_snprintf_s(str, _countof(str), _TRUNCATE, 
			            "Avg milliseconds/frame raw: %.02f\tAvg FPS Cooked: %.01f\tAvg FPS Raw: %.01f\n", 
			            AverageMicrosecondsPerFrameRaw,
			            gPerformanceData.CookedFPSAverage,
			            gPerformanceData.RawFPSAverage);
            
			OutputDebugStringA(str);

			ElapsedMicrosecondsPerFrameAccumulatorRaw = 0;

			ElapsedMicrosecondsPerFrameAccumulatorCooked = 0;
		}
	}


	Exit:

    
		return(0);
}

LRESULT CALLBACK MainWindowProc(_In_ HWND WindowHandle, _In_ UINT Message, _In_ WPARAM WParam, _In_ LPARAM LParam)
{
	LRESULT Result = 0;

	switch (Message)
	{
		case WM_CLOSE:
		{
			gGameIsRunning = FALSE;

			PostQuitMessage(0);

			break;
		}
		default:
		{
			Result = DefWindowProcA(WindowHandle, Message, WParam, LParam);
		}
	}

	return(Result);
}

DWORD CreateMainGameWindow(void)
{
	DWORD Result = ERROR_SUCCESS;

	WNDCLASSEXA WindowClass = { 0 };

	WindowClass.cbSize = sizeof(WNDCLASSEXA);

	WindowClass.style = 0;

	WindowClass.lpfnWndProc = MainWindowProc;

	WindowClass.cbClsExtra = 0;

	WindowClass.cbWndExtra = 0;

	WindowClass.hInstance = GetModuleHandleA(NULL);

	WindowClass.hIcon = LoadIconA(NULL, IDI_APPLICATION);

	WindowClass.hIconSm = LoadIconA(NULL, IDI_APPLICATION);

	WindowClass.hCursor = LoadCursorA(NULL, IDC_ARROW);

	WindowClass.hbrBackground = CreateSolidBrush(RGB(255, 0, 255));

	WindowClass.lpszMenuName = NULL;

	WindowClass.lpszClassName = GAME_NAME "_WINDOWCLASS";   

	// SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	if (RegisterClassExA(&WindowClass) == 0)
	{
		Result = GetLastError();

		MessageBoxA(NULL, "Window Registration Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);

		goto Exit;
	}

	gGameWindow = CreateWindowExA(0, WindowClass.lpszClassName, "Window Title", WS_OVERLAPPEDWINDOW | WS_VISIBLE, CW_USEDEFAULT, CW_USEDEFAULT, 640, 480, NULL, NULL, GetModuleHandleA(NULL), NULL);

	if (gGameWindow == NULL)
	{
		Result = GetLastError();

		MessageBox(NULL, "Window Creation Failed!", "Error!", MB_ICONEXCLAMATION | MB_OK);

		goto Exit;
	}

	gPerformanceData.MonitorInfo.cbSize = sizeof(MONITORINFO);

	if (GetMonitorInfoA(MonitorFromWindow(gGameWindow, MONITOR_DEFAULTTOPRIMARY), &gPerformanceData.MonitorInfo) == 0)
	{
		Result = ERROR_MONITOR_NO_DESCRIPTOR;

		goto Exit;
	}

	gPerformanceData.MonitorWidth = gPerformanceData.MonitorInfo.rcMonitor.right - gPerformanceData.MonitorInfo.rcMonitor.left;

	gPerformanceData.MonitorHeight = gPerformanceData.MonitorInfo.rcMonitor.bottom - gPerformanceData.MonitorInfo.rcMonitor.top;

	if (SetWindowLongPtrA(gGameWindow, GWL_STYLE, (WS_OVERLAPPEDWINDOW | WS_VISIBLE) & ~WS_OVERLAPPEDWINDOW) == 0)
	{
		Result = GetLastError();

		goto Exit;
	}

	if (SetWindowPos(gGameWindow, 
	                 HWND_TOP, 
	                 gPerformanceData.MonitorInfo.rcMonitor.left, 
	                 gPerformanceData.MonitorInfo.rcMonitor.top, 
	                 gPerformanceData.MonitorWidth, 
	                 gPerformanceData.MonitorHeight, 
	                 SWP_NOOWNERZORDER | SWP_FRAMECHANGED) == 0)
	{
		Result = GetLastError();

		goto Exit;
	}


	Exit:

		return(Result);
}

BOOL GameIsAlreadyRunning(void)
{
	HANDLE Mutex = NULL;

	Mutex = CreateMutexA(NULL, FALSE, GAME_NAME "_GameMutex");

	if (GetLastError() == ERROR_ALREADY_EXISTS)
	{
		return(TRUE);
	}
	else
	{
		return(FALSE);
	}
}

void ProcessPlayerInput(void)
{
	int16_t EscapeKeyIsDown = GetAsyncKeyState(VK_ESCAPE);

	if (EscapeKeyIsDown)
	{
		SendMessageA(gGameWindow, WM_CLOSE, 0, 0);
	}
}

void RenderFrameGraphics(void)
{
	PIXEL32 Pixel = { 0 };

	Pixel.Blue = 0x7f;

	Pixel.Green = 0;

	Pixel.Red = 0;

	Pixel.Alpha = 0xff;

	for (int x = 0; x < GAME_RES_WIDTH * GAME_RES_HEIGHT; x++)
	{
		memcpy_s((PIXEL32*)gBackBuffer.Memory + x, sizeof(PIXEL32), &Pixel, sizeof(PIXEL32));        
	}

	HDC DeviceContext = GetDC(gGameWindow);

	StretchDIBits(DeviceContext, 
	              0, 
	              0, 
	              gPerformanceData.MonitorWidth,
	              gPerformanceData.MonitorHeight,
	              0, 
	              0, 
	              GAME_RES_WIDTH, 
	              GAME_RES_HEIGHT, 
	              gBackBuffer.Memory, 
	              &gBackBuffer.BitmapInfo, 
	              DIB_RGB_COLORS, 
	              SRCCOPY);

	ReleaseDC(gGameWindow, DeviceContext);
}