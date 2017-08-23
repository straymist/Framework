
#include <stdio.h>

#include "imgui/imgui.h"
#include "platform/gpu.h"
#include "platform/gui.h"
#include "platform/window.h"
#include "fiber.h"


void Fade(void *UserData)
{
	for (int i = 0; i < 10; ++i)
	{
		dprintf("Fade Tick %d\n", i);
		WaitForFrame();
	}
}

void Fade2(void *UserData)
{
	for (int i = 0; i < 10; ++i)
	{
		dprintf("Fade2 Tick %d\n", i);
		WaitForFrame();
	}
}

void Anim(void *UserData)
{
	for (int i = 0; i < 10; ++i)
	{
		dprintf("Anim Tick %d\n", i);
		WaitForFrame();
	}
}



void Walk(void *ud)
{
	for (int i = 0; i < 10; ++i)
	{
		dprintf("Walk Tick %d\n", i);
		WaitForFrame();
	}
}

void Patrol(void *ud)
{
	for (int i = 0; i < 10; ++i)
	{
		dprintf("Patrol Tick %d\n", i);
		WaitForFrame();
	}
}

void Idle(void *ud)
{
	for (int i = 0; i < 10; ++i)
	{
		dprintf("Idle Tick %d\n", i);
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

	dprintf("Done\n");

	BlackBoard bb1;
	BlackBoard bb2;
	BlackBoard bb3;
	bb1.State = 0;
	bb2.State = 1;
	bb3.State = 2;

	dprintf("BT test begin \n");
	Wait = 1;
	DoTask(BehaviorTree, &Wait, &bb1);
	WaitForCounter(&Wait, 0);

	Wait = 1;
	DoTask(BehaviorTree, &Wait, &bb2);
	WaitForCounter(&Wait, 0);

	Wait = 1;
	DoTask(BehaviorTree, &Wait, &bb3);
	WaitForCounter(&Wait, 0);


	dprintf("BT done\n");

}



bool show_test_window = true;
bool show_another_window = false;
ImVec4 clear_col = ImColor(114, 144, 154);
void BuildImguiContent()
{

	// 1. Show a simple window
	// Tip: if we don't call ImGui::Begin()/ImGui::End() the widgets appears in a window automatically called "Debug"
	{
		static float f = 0.0f;
		ImGui::Text("Hello, world!");
		ImGui::SliderFloat("float", &f, 0.0f, 1.0f);
		ImGui::ColorEdit3("clear color", (float*)&clear_col);
		if (ImGui::Button("Test Window")) show_test_window ^= 1;
		if (ImGui::Button("Another Window")) show_another_window ^= 1;
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	}
	// 2. Show another simple window, this time using an explicit Begin/End pair
	if (show_another_window)
	{
		ImGui::SetNextWindowSize(ImVec2(200, 100), ImGuiSetCond_FirstUseEver);
		ImGui::Begin("Another Window", &show_another_window);
		ImGui::Text("Hello");
		ImGui::End();
	}

	// 3. Show the ImGui test window. Most of the sample code is in ImGui::ShowTestWindow()
	if (show_test_window)
	{
		ImGui::SetNextWindowPos(ImVec2(650, 20), ImGuiSetCond_FirstUseEver);     // Normally user code doesn't need/want to call it because positions are saved in .ini file anyway. Here we just want to make the demo initial state a bit more friendly!
		ImGui::ShowTestWindow(&show_test_window);
	}

}


void Render()
{
	ImGui::FrameTick();


	BuildImguiContent();


	GPU::CommandList *Cmd = GPU::GetCommandList();

	GPU::BeginFrame(Cmd);

	ImGui::Render();

	GPU::EndFrame(Cmd);
}

int main(int argc, char *argv[]) 
{
	auto Win = frCreateWindow(1280, 720, L"Demo");
	
	GPU::Open();
	ImGui::Open();

	frShowWindow(Win);
	
	DoTask(Root);

	while (frGetWindowMessage(Win) == EWindowMessage::CONTINUE)
	{
		RunScheduler();
		Render();	
	}
	

	ImGui::Close();
	GPU::Close();

	dprintf("-- OK --\n");

	return 0;
}




