
#include <string>
#include <windows.h>

#include "window.h"
#include "gpu.h"
#include "imgui_dx12.h"

OSApplication GApplication;

void OSWindow::SetWidth(int InWidth)
{
	Width = InWidth;
}
int OSWindow::GetWidth()
{
	return Width;
}

void OSWindow::SetHeight(int InHeight)
{
	Height = InHeight;
}
int OSWindow::GetHeight()
{
	return Height;
}

void OSWindow::SetTitle(const wchar_t* InTitle)
{
	Title = InTitle;
}
std::wstring OSWindow::GetTitle()
{
	return Title;
}

void OSWindow::SetHandle(void * InHandle)
{
	Handle = InHandle;
}
void *OSWindow::GetHandle()
{
	return Handle;
}

extern LRESULT ImGui_ImplDX12_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (ImGui_ImplDX12_WndProcHandler(hWnd, message, wParam, lParam))
		return true;

	switch (message)
	{

	case WM_SIZE:
		if ( wParam != SIZE_MINIMIZED) 
			ImguiGPUWindowResize(LOWORD(lParam), HIWORD(lParam));
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

OSWindow OSWin;

size_t frCreateWindow(int width, int height, const wchar_t *title)
{
	OSWin.SetWidth(width);
	OSWin.SetHeight(height);
	OSWin.SetTitle(title);
	return 0;
}

MSG msg = {};
EInputMessage frGetInputMessage()
{
	if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	if (msg.message != WM_QUIT)
		return EInputMessage::CONTINUE;
	else
		return EInputMessage::EXIT;	
}

extern int main(int argc, char *argv[]);
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow)
{
	
	
	
	GApplication.Window = &OSWin;

	frCreateWindow(1280, 768, L"Hello");

	// Initialize the window class.
	WNDCLASSEX windowClass = { 0 };
	windowClass.cbSize = sizeof(WNDCLASSEX);
	windowClass.style = CS_HREDRAW | CS_VREDRAW;
	windowClass.lpfnWndProc = WindowProc;
	windowClass.hInstance = hInstance;
	windowClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	windowClass.lpszClassName = L"HarmonyClass";
	RegisterClassEx(&windowClass);

	RECT windowRect = { 0, 0, static_cast<LONG>(OSWin.GetWidth()), static_cast<LONG>(OSWin.GetHeight()) };
	AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

	// Create the window and store a handle to it.
	HWND m_hwnd = CreateWindow(
		windowClass.lpszClassName,
		OSWin.GetTitle().c_str(),
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		windowRect.right - windowRect.left,
		windowRect.bottom - windowRect.top,
		nullptr,		// We have no parent window.
		nullptr,		// We aren't using menus.
		hInstance,
		nullptr);

	OSWin.SetHandle(m_hwnd);

	GPU::Init();

	ImguiGPUInit();

	
	ShowWindow(m_hwnd, nCmdShow);


	main(0, nullptr);

	// Main sample loop.
	
	ImguiGPUFini();
	//HeartEnd();

	GPU::Fini();

	return 0;
}

