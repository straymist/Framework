#pragma once

#include <string>

class OSWindow
{
public:
	void SetWidth(int width);
	void SetHeight(int height);
	void SetTitle(const wchar_t* title);
	int GetWidth();
	int GetHeight();
	std::wstring GetTitle();
	void *GetHandle();
	void SetHandle(void * Handle);

private:
	int Width;
	int Height;
	void *Handle;
	std::wstring Title;
	
};

enum EInputMessage 
{
	CONTINUE,
	EXIT
};

size_t frCreateWindow(int width, int height, const wchar_t *title);
EInputMessage frGetInputMessage();

struct OSApplication
{
	OSWindow* Window;
};

extern OSApplication GApplication;