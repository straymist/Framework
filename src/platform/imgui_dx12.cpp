#include "../imgui/imgui.h"
#include <Windows.h>
#include <d3d12.h>

#include "window.h"
#include "gpu.h"

GPU::Resource *VertexBuffer = nullptr;;
GPU::Resource *IndexBuffer = nullptr;
GPU::Resource *ConstantBuffer = nullptr;
GPU::Resource *ImguiFontTexture = nullptr;

void ImGui_ImplDX12_CreateFontsTexture()
{
	// Build texture atlas
	ImGuiIO& io = ImGui::GetIO();
	unsigned char* pixels;
	int width, height;
	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	ImguiFontTexture = GPU::CreateTexture(width, height, 0,  GPU::Format::RGBA32, pixels);

	// Store our identifier
	io.Fonts->TexID = (void *)ImguiFontTexture;
}

ImDrawVert * vtx_begin = nullptr;
ImDrawIdx  * idx_begin = nullptr;
void *cbufdst;

void AllocateImguiResources()
{
	if (VertexBuffer != nullptr)
		return;
	VertexBuffer = GPU::CreateVertexBuffer( 1024*1024*4, sizeof(ImDrawVert));
	IndexBuffer = GPU::CreateIndexBuffer( 1024 * 1024 , sizeof(ImDrawIdx));
	ConstantBuffer = GPU::CreateConstantBuffer( sizeof(float) *4 *4 );

	GPU::Map(VertexBuffer, (void**)&vtx_begin);
	GPU::Map(IndexBuffer, (void**)&idx_begin);
	GPU::Map(ConstantBuffer, &cbufdst);
}

void ImGui_ImplDX12_RenderDrawLists(ImDrawData* draw_data)
{
	AllocateImguiResources();
	// copy draw list data into the target buffers
	ImDrawVert * vtx_dst = vtx_begin;
	ImDrawIdx  * idx_dst = idx_begin;

	for (int n = 0; n < draw_data->CmdListsCount; n++)
	{
		const ImDrawList* cmd_list = draw_data->CmdLists[n];
		memcpy(vtx_dst, &cmd_list->VtxBuffer[0], cmd_list->VtxBuffer.size() * sizeof(ImDrawVert));
		memcpy(idx_dst, &cmd_list->IdxBuffer[0], cmd_list->IdxBuffer.size() * sizeof(ImDrawIdx));
		vtx_dst += cmd_list->VtxBuffer.size();
		idx_dst += cmd_list->IdxBuffer.size();
	}

	// Setup orthographic projection matrix into our constant buffer
	{

		float L = 0.0f;
		float R = ImGui::GetIO().DisplaySize.x;
		float B = ImGui::GetIO().DisplaySize.y;
		float T = 0.0f;
		float mvp[4][4] =
		{
			{ 2.0f / (R - L),   0.0f,           0.0f,       0.0f },
			{ 0.0f,         2.0f / (T - B),     0.0f,       0.0f },
			{ 0.0f,         0.0f,           0.5f,       0.0f },
			{ (R + L) / (L - R),  (T + B) / (B - T),    0.5f,       1.0f },
		};
		memcpy(cbufdst, mvp, sizeof(mvp));
	}


	// Bind Primitives
	auto CmdList = GPU::GetCommandList();

	GPU::SetImgui(CmdList, ImguiFontTexture, ConstantBuffer );

	GPU::SetPrimitiveType(CmdList, GPU::PrimitiveType::TRIANGLE_LIST);
	GPU::SetVertexBuffer(CmdList, VertexBuffer);
	GPU::SetIndexBuffer(CmdList, IndexBuffer);
	GPU::SetViewport3D(CmdList, 0.0, 0.0, ImGui::GetIO().DisplaySize.x, ImGui::GetIO().DisplaySize.y, 0.0f, 1.0f);


	// Render command lists
	int vtx_offset = 0;
	int idx_offset = 0;
	for (int n = 0; n < draw_data->CmdListsCount; n++)
	{
		const ImDrawList* cmd_list = draw_data->CmdLists[n];
		for (int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.size(); cmd_i++)
		{
			const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
			if (pcmd->UserCallback)
			{
				pcmd->UserCallback(cmd_list, pcmd);
			}
			else
			{
				pcmd->TextureId;
				GPU::SetScissorRect(CmdList, (int)pcmd->ClipRect.x, (int)pcmd->ClipRect.y, (int)pcmd->ClipRect.z, (int)pcmd->ClipRect.w);
				GPU::DrawIndexed(CmdList, pcmd->ElemCount, idx_offset, vtx_offset);
			}
			idx_offset += pcmd->ElemCount;
		}
		vtx_offset += cmd_list->VtxBuffer.size();
	}
}
static INT64                    g_Time = 0;
static INT64                    g_TicksPerSecond = 0;

void ImguiGPUInit()
{
	// 
	if (!QueryPerformanceFrequency((LARGE_INTEGER *)&g_TicksPerSecond))
		return ;
	if (!QueryPerformanceCounter((LARGE_INTEGER *)&g_Time))
		return ;

	ImGuiIO& io = ImGui::GetIO();
	io.KeyMap[ImGuiKey_Tab] = VK_TAB;                       // Keyboard mapping. ImGui will use those indices to peek into the io.KeyDown[] array that we will update during the application lifetime.
	io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;
	io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;
	io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
	io.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
	io.KeyMap[ImGuiKey_PageUp] = VK_PRIOR;
	io.KeyMap[ImGuiKey_PageDown] = VK_NEXT;
	io.KeyMap[ImGuiKey_Home] = VK_HOME;
	io.KeyMap[ImGuiKey_End] = VK_END;
	io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
	io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
	io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
	io.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
	io.KeyMap[ImGuiKey_A] = 'A';
	io.KeyMap[ImGuiKey_C] = 'C';
	io.KeyMap[ImGuiKey_V] = 'V';
	io.KeyMap[ImGuiKey_X] = 'X';
	io.KeyMap[ImGuiKey_Y] = 'Y';
	io.KeyMap[ImGuiKey_Z] = 'Z';

	io.RenderDrawListsFn = ImGui_ImplDX12_RenderDrawLists;  // Alternatively you can set this to NULL and call ImGui::GetDrawData() after ImGui::Render() to get the same ImDrawData pointer.
	io.ImeWindowHandle = GApplication.Window->GetHandle();
	
	static const ImWchar seto_ranges[] =
	{
		//0x0020, 0x00FF, // Basic Latin + Latin Supplement
		0x3000, 0x30FF, // Punctuations, Hiragana, Katakana
		0x31F0, 0x31FF, // Katakana Phonetic Extensions
		0xFF00, 0xFFEF, // Half-width characters
		0x4e00, 0x9FAF, // CJK Ideograms
		0,
	};

	io.Fonts->AddFontDefault();
	//ImFontConfig config;
	//config.MergeMode = true;
	//io.Fonts->AddFontFromFileTTF("Assets/Fonts/setofont.ttf", 18.0f, &config, seto_ranges);
	ImGui_ImplDX12_CreateFontsTexture();

}

void ImguiGPUNewframe()
{
	// Setup display size (every frame to accommodate for window resizing)
	ImGuiIO& io = ImGui::GetIO();
	RECT rect;
	GetClientRect( (HWND)GApplication.Window->GetHandle(), &rect);
	io.DisplaySize = ImVec2((float)(rect.right - rect.left), (float)(rect.bottom - rect.top));

	// Setup time step
	INT64 current_time;
	QueryPerformanceCounter((LARGE_INTEGER *)&current_time);
	io.DeltaTime = (float)(current_time - g_Time) / g_TicksPerSecond;
	g_Time = current_time;

	// Read keyboard modifiers inputs
	io.KeyCtrl = (GetKeyState(VK_CONTROL) & 0x8000) != 0;
	io.KeyShift = (GetKeyState(VK_SHIFT) & 0x8000) != 0;
	io.KeyAlt = (GetKeyState(VK_MENU) & 0x8000) != 0;
	io.KeySuper = false;
	// io.KeysDown : filled by WM_KEYDOWN/WM_KEYUP events
	// io.MousePos : filled by WM_MOUSEMOVE events
	// io.MouseDown : filled by WM_*BUTTON* events
	// io.MouseWheel : filled by WM_MOUSEWHEEL events

	// Hide OS mouse cursor if ImGui is drawing it
	SetCursor(io.MouseDrawCursor ? NULL : LoadCursor(NULL, IDC_ARROW));

	// Start the frame
	ImGui::NewFrame();
}

void ImguiGPUFini()
{
	ImGui::Shutdown();
}

void ImguiGPUWindowResize(int Width, int Height)
{
	GPU::NotifyToResizeBuffer(Width, Height);
}

IMGUI_API LRESULT ImGui_ImplDX12_WndProcHandler(HWND, UINT msg, WPARAM wParam, LPARAM lParam)
{
	ImGuiIO& io = ImGui::GetIO();
	switch (msg)
	{
	case WM_LBUTTONDOWN:
		io.MouseDown[0] = true;
		return true;
	case WM_LBUTTONUP:
		io.MouseDown[0] = false;
		return true;
	case WM_RBUTTONDOWN:
		io.MouseDown[1] = true;
		return true;
	case WM_RBUTTONUP:
		io.MouseDown[1] = false;
		return true;
	case WM_MBUTTONDOWN:
		io.MouseDown[2] = true;
		return true;
	case WM_MBUTTONUP:
		io.MouseDown[2] = false;
		return true;
	case WM_MOUSEWHEEL:
		io.MouseWheel += GET_WHEEL_DELTA_WPARAM(wParam) > 0 ? +1.0f : -1.0f;
		return true;
	case WM_MOUSEMOVE:
		io.MousePos.x = (signed short)(lParam);
		io.MousePos.y = (signed short)(lParam >> 16);
		return true;
	case WM_KEYDOWN:
		if (wParam < 256)
			io.KeysDown[wParam] = 1;
		return true;
	case WM_KEYUP:
		if (wParam < 256)
			io.KeysDown[wParam] = 0;
		return true;
	case WM_CHAR:
		// You can also use ToAscii()+GetKeyboardState() to retrieve characters.
		if (wParam > 0 && wParam < 0x10000)
			io.AddInputCharacter((unsigned short)wParam);
		return true;
	}
	return 0;
}