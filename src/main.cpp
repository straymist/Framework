
#include <stdio.h>

#include "platform/gpu.h"
#include "platform/window.h"
#include "fiber.h"


void Fade(void *UserData)
{
	for (int i = 0; i < 10; ++i)
	{
		printf("Fade Tick %d\n", i);
		WaitForFrame();
	}
}

void Fade2(void *UserData)
{
	for (int i = 0; i < 10; ++i)
	{
		printf("Fade2 Tick %d\n", i);
		WaitForFrame();
	}
}

void Anim(void *UserData)
{
	for (int i = 0; i < 10; ++i)
	{
		printf("Anim Tick %d\n", i);
		WaitForFrame();
	}
}



void Walk(void *ud)
{
	for (int i = 0; i < 10; ++i)
	{
		printf("Walk Tick %d\n", i);
		WaitForFrame();
	}
}

void Patrol(void *ud)
{
	for (int i = 0; i < 10; ++i)
	{
		printf("Patrol Tick %d\n", i);
		WaitForFrame();
	}
}

void Idle(void *ud)
{
	for (int i = 0; i < 10; ++i)
	{
		printf("Idle Tick %d\n", i);
		WaitForFrame();
	}
}


struct BlackBoard
{
	int State;
};

void BehaviorTree(void * UserData)
{
	BlackBoard *bb = (BlackBoard*)UserData;
	int Wait = 1;
	switch (bb->State)
	{
	case 0:
		DoTask(Patrol, &Wait);
		break;
	case 1:
		DoTask(Walk, &Wait);
		break;
	default:
		DoTask(Idle, &Wait);
		break;
	}
	WaitForCounter(&Wait, 0);
}

void Root(void *UserData)
{
	int Wait = 2;

	DoTask(Fade, &Wait);
	DoTask(Anim, &Wait);

	WaitForCounter(&Wait, 0);

	Wait = 2;
	DoTask(Fade2, &Wait);
	DoTask(Fade, &Wait);
	WaitForCounter(&Wait, 0);

	printf("Done\n");

	BlackBoard bb1;
	BlackBoard bb2;
	BlackBoard bb3;
	bb1.State = 0;
	bb2.State = 1;
	bb3.State = 2;

	printf("BT test begin \n");
	Wait = 1;
	DoTask(BehaviorTree, &Wait, &bb1);
	WaitForCounter(&Wait, 0);

	Wait = 1;
	DoTask(BehaviorTree, &Wait, &bb2);
	WaitForCounter(&Wait, 0);

	Wait = 1;
	DoTask(BehaviorTree, &Wait, &bb3);
	WaitForCounter(&Wait, 0);


	printf("BT done\n");

}

void ImguiGPUNewframe();

int main(int argc, char *argv[]) 
{
	GPU::CommandList *Cmd = GPU::GetCommandList();

	DoTask(Root);

	while (frGetInputMessage() == EInputMessage::CONTINUE)
	{

		RunScheduler();

		ImguiGPUNewframe();
		GPU::RenderTest(Cmd);
	}
	

	printf("-- OK --\n");

	return 0;
}




