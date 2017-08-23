
#include "framework.h"

void Render()
{
	void MakeTetrisDrawList();
	MakeTetrisDrawList();

	//void BuildImguiContent();	
	//BuildImguiContent();

	GPU::CommandList *Cmd = GPU::GetCommandList();

	GPU::BeginFrame(Cmd);

		ImGui::Render();
	GPU::EndFrame(Cmd);
}

int main(int argc, char *argv[]) 
{
	dprintf("------------------- Framework Begin -------------------------------\n");

	auto Win = frCreateWindow(1280, 720, L"Demo");
	
	GPU::Open();
	ImGui::Open();

	frShowWindow(Win);
	

	void DoTetris();
	DoTetris();
	

	while (frGetWindowMessage(Win) == EWindowMessage::CONTINUE)
	{
		ImGui::FrameTick();

		RunScheduler();
		Render();	
	}
	
	ImGui::Close();
	GPU::Close();

	dprintf("------------------- Framework Closed -------------------------------\n");

	return 0;
}




