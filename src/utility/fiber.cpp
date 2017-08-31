#include <windows.h>
#include <assert.h>
#include "fiber.h"
#include "container.h"


const size_t MAX_FIBER = 128;

struct FiberEntry
{
	enum EState 
	{
		SUSPEND,
		RUNNING,
		WAIT_FRAME,
		WAIT_COUNTER,
		DONE
	};

	std::function<void(void*)> Func;
	LPVOID pFiber;
	int *pCounter;
	EState State;
	void *pUserData; // UserData could be input or output.
};
FixedArray<FiberEntry, MAX_FIBER> FiberEntries;
FiberEntry *CurrentFiber;

FixedArray<FiberEntry*, MAX_FIBER> DelFiberEntries;;

struct WaitCounterEntry
{
	int *Counter;
	int TargetValue;
	FiberEntry *WhoWaitMe;
};
FixedArray<WaitCounterEntry, MAX_FIBER> WaitCounterEntries;
FixedArray<WaitCounterEntry*, MAX_FIBER> DelWaitEntries;;

struct WaitFrameEntry
{
	FiberEntry *WhoWaitMe;
};
FixedArray<WaitFrameEntry, MAX_FIBER> WaitFrameEntries;

LPVOID SchedulerFiber;
size_t RunScheduler()
{
	SchedulerFiber = ConvertThreadToFiber(nullptr);
	if (SchedulerFiber == nullptr)
	{
		printf("ConvertThreadToFiber error (%d)\n", GetLastError());
		return 0;
	}
	
	// Fiber First Run
	for (int i = 0; i < FiberEntries.Size(); ++i)
	{
		if (FiberEntries[i]->State == FiberEntry::SUSPEND)
		{
			CurrentFiber = FiberEntries[i];
			CurrentFiber->State = FiberEntry::RUNNING;
			SwitchToFiber(CurrentFiber->pFiber);
		}	
	}

	// Wait a frame
	for (int i = 0; i < FiberEntries.Size(); ++i)
	{
		if (FiberEntries[i]->State == FiberEntry::WAIT_FRAME)
		{
			CurrentFiber = FiberEntries[i];
			CurrentFiber->State = FiberEntry::RUNNING;
			SwitchToFiber(CurrentFiber->pFiber);
		}
	}
	// Wait for Counter
	for (int i = 0; i < WaitCounterEntries.Size(); ++i)
	{
		WaitCounterEntry *e = WaitCounterEntries[i];
		if ( *e->Counter == e->TargetValue && 
			e->WhoWaitMe->State == FiberEntry::WAIT_COUNTER)
		{
			CurrentFiber = e->WhoWaitMe;
			CurrentFiber->State = FiberEntry::RUNNING;

			
			WaitCounterEntries.Free(i);
			--i;
			SwitchToFiber(CurrentFiber->pFiber);
		}
	}

	// Remove Done Fibers
	for (int i = 0; i < FiberEntries.Size(); ++i)
	{
		if (FiberEntries[i]->State == FiberEntry::DONE)
		{
			FiberEntries.Free(i);
			--i;			
		}
	}

		
	ConvertFiberToThread();

	return FiberEntries.Size();
}


VOID WINAPI FiberTaskDispatcher(LPVOID lpFiberParameter)
{
	FiberEntry* ent = (FiberEntry*)lpFiberParameter;
	CurrentFiber = ent;
	// Barrier Here?
	ent->Func(ent->pUserData);

	if (ent->pCounter)
	{
		(*ent->pCounter)--;
	}
		
	ent->State = FiberEntry::DONE;
	CurrentFiber = nullptr;
	SwitchToFiber(SchedulerFiber);
}

void DoTaskImpl(std::function<void(void*)> &fn, int *pCounter, void *pUserData )
{
	FiberEntry* entry = FiberEntries.Alloc() ;
	entry->Func = fn;
	entry->pFiber = CreateFiber(0, FiberTaskDispatcher, entry);
	if (entry->pFiber == nullptr)
	{
		printf("DoFiberTask error (%d)\n", GetLastError());
		return;
	}
	entry->pCounter = pCounter;
	entry->pUserData = pUserData;
	entry->State = FiberEntry::SUSPEND;
}

void WaitForCounter(int *pCounter, int TargetValue)
{
	WaitCounterEntry *et = WaitCounterEntries.Alloc();
	et->WhoWaitMe = CurrentFiber;
	CurrentFiber->State = FiberEntry::WAIT_COUNTER;
	et->TargetValue = TargetValue;
	et->Counter = pCounter;
	SwitchToFiber(SchedulerFiber);
}
void WaitForFrame()
{
	CurrentFiber->State = FiberEntry::WAIT_FRAME;
	SwitchToFiber(SchedulerFiber);
}
