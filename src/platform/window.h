#pragma once
#include <string>

enum EWindowMessage
{
	CONTINUE,
	EXIT
};
struct PlatformWindow
{
	size_t Width;
	size_t Height;
	const wchar_t *Title;
	void * Handle;
};

extern PlatformWindow gWindow;

PlatformWindow* frCreateWindow(int width, int height, const wchar_t *title);
void frShowWindow(PlatformWindow *window);
EWindowMessage frGetWindowMessage(PlatformWindow *window);
void dprintf(const char * fmt, ...);