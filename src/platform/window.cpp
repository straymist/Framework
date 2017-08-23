
#include <string>
#include <windows.h>

#include "window.h"
#include "gpu.h"
#include "gui.h"



extern LRESULT ImGui_ImplDX12_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplDX12_WndProcHandler(hWnd, message, wParam, lParam))
		return true;

	switch (message)
	{

	case WM_SIZE:
		if ( wParam != SIZE_MINIMIZED) 
			ImGui::WindowResize(LOWORD(lParam), HIWORD(lParam));
		break;

	case WM_CREATE:
		{
			// Save the DXSample* passed in to CreateWindow.
			LPCREATESTRUCT pCreateStruct = reinterpret_cast<LPCREATESTRUCT>(lParam);
			SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCreateStruct->lpCreateParams));
		}
		return 0;

	case WM_KEYDOWN:
		return 0;
	case WM_KEYUP:
		return 0;

	case WM_PAINT:
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}

	// Handle any messages the switch statement didn't.
	return DefWindowProc(hWnd, message, wParam, lParam);
}




PlatformWindow gWindow;
HINSTANCE gAppInstance;

PlatformWindow* frCreateWindow(int width, int height, const wchar_t *title)
{
	
	// Initialize the window class.
	WNDCLASSEX windowClass = { 0 };
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = WindowProc;
	windowClass.hInstance = gAppInstance;
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	windowClass.lpszClassName = L"FrameworkClass";
	RegisterClassEx(&windowClass);

	RECT windowRect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	// Create the window and store a handle to it.
	HWND m_hwnd = CreateWindow(
		windowClass.lpszClassName,
		title,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		nullptr,		// We have no parent window.
		nullptr,		// We aren't using menus.
		gAppInstance,
		nullptr);

	gWindow.Width = width;
	gWindow.Height = height;
	gWindow.Title = title;
	gWindow.Handle = m_hwnd;
	
	return &gWindow;
}

MSG msg = {};
EWindowMessage frGetWindowMessage(PlatformWindow *Window)
{
	if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	if (msg.message != WM_QUIT)
		return EWindowMessage::CONTINUE;
	else
		return EWindowMessage::EXIT;
}

void frShowWindow(PlatformWindow *window)
{
	ShowWindow((HWND)window->Handle, SW_SHOWNORMAL);
}

extern int main(int argc, char *argv[]);
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	
	// Main sample loop.

	main(0, nullptr);

	return 0;
}

void dprintf(const char * fmt, ...)
{

	va_list args;
	va_start(args, fmt);
	char buf[256];
	vsnprintf(buf, 256, fmt, args);
	OutputDebugStringA(buf);
	va_end(args);
}
