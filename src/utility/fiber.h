#pragma once
#include <functional>

void DoTaskImpl(std::function<void(void*)> &fn, int *pCounter = nullptr, void *pUserData = nullptr);

template <typename FuncType>
void DoTask(FuncType &FuncIn, int *pCounter = nullptr, void *pUserData = nullptr)
{
	std::function<void(void*)> Fn = FuncIn;
	DoTaskImpl(Fn, pCounter, pUserData);
}

void WaitForCounter(int *pCounter, int TargetValue);
void WaitForFrame();

size_t RunScheduler();