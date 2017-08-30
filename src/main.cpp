
#include "framework.h"

#define TEST_FILE "test_entries.h"
#define TEST_CLASS CAVGTest

#if defined(TEST_CLASS) && defined(TEST_FILE)
	#include TEST_FILE
	TEST_CLASS gTestImpl;
	CTest *gTest = &gTestImpl;
#else
	CTest *gTest = nullptr;
#endif 

void Render()
{
	if ( gTest )
		gTest->BuildGUI();

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
	
	if (gTest)
		gTest->Open();

	while (frGetWindowMessage(Win) == EWindowMessage::CONTINUE)
	{
		ImGui::FrameTick();
		RunScheduler();
		Render();	
	}
	
	if (gTest)
		gTest->Close();

	ImGui::Close();
	GPU::Close();

	dprintf("------------------- Framework Closed -------------------------------\n");

	return 0;
}




