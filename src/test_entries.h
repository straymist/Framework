#pragma once
#include "framework.h"

class CTetrisTest : public CTest
{
public:
	void Open();
	void BuildGUI();
};

class CFiberTest : public CTest
{
public:
	void Open();
};

class CGUITest : public CTest
{
public:
	void BuildGUI();
};

class CAVGTest : public CTest
{
public:
	void Open();
	void BuildGUI();
};