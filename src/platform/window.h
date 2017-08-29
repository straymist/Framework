#pragma once
#include <string>

enum EWindowMessage
{
	CONTINUE,
	EXIT
};

enum EKeyInput
{
	KB_UP,
	KB_DOWN,
	KB_LEFT,
	KB_RIGHT,
	KB_ENTER,
	KB_SPACE,
	KB_W,
	KB_S,
	KB_A,
	KB_D
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
bool frIsKeyPressed(EKeyInput Key);

void dprintf(const char * fmt, ...);