#include "framework.h"
#include "test_entries.h"

static bool show_test_window = false;
static bool show_another_window = false;
static ImVec4 clear_col = ImColor(114, 144, 154);

static char msg[] = "Chief among these risks are the dramatic \n"
					"flybys Cassini undertakes every six days.\n"
					"These see the spacecraft plunge through the gap\n"
					"between the top of Saturn's atmosphere and the rings.\n" 
					"It is a completely unexplored region from where\n" 
					"Cassini is returning some unique data.";

static char msg_shown[512] = "";

static void BuildImguiContent()
{

	{
		ImGui::Text(msg_shown);
	}


}

static void ShowMsg(int nth)
{
	int i = 0;
	for (i = 0; i < nth; ++i)
		msg_shown[i] = msg[i];
	msg_shown[nth] = '\0';
}

static void ShowTask(void *Param)
{
	int n = strlen(msg);
	int x = 0;
	while (1)
	{
		if (x < n)
		{
			ShowMsg(x);
			x++;
		}
		else
		{
			x = 0;
		}		

		for(int i = 0 ; i < 3 ; ++i)
			WaitForFrame();
	}

}

void CAVGTest::Open()
{
	DoTask(ShowTask);
}

void CAVGTest::BuildGUI()
{
	BuildImguiContent();
}