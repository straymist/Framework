#pragma once
#include "platform/gpu.h"
#include "platform/gui.h"
#include "platform/window.h"
#include "utility/fiber.h"

#define def_concat(a,b) a ## b
#define def_str(x) #x

class CTest
{
public:
	virtual void Open(){};
	virtual void BuildGUI() {};
	virtual void Close() {};
};




