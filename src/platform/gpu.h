#include <cstdint>

namespace GPU 
{

	enum class Format : uint8_t
	{
		RGBA32
	};

	enum class PrimitiveType : uint8_t
	{
		TRIANGLE_LIST
	};

	enum class BarrierState : uint8_t
	{
		COMMON,
		CONSTANT_BUFFER,
		VERTEX_BUFFER,
		INDEX_BUFFER,
		RTV,
		UAV,
		DEPTH_WRITE,
		DEPTH_READ,
		NON_PIXEL_SRV,
		PIXEL_SRV,
		COPY_DST,
		COPY_SRC,
		PRESENT
	};
	
	class Resource;
	class ResourceLayout;
	class PipelineState;
	class CommandList;
	class CommandQueue;

	void Open();
	void Close();
	
	// Resource Management	
	Resource* CreateVertexBuffer(int TotalSizeInBytes, int StrideSizeInBytes);
	Resource* CreateIndexBuffer(int TotalSizeInBytes, int StrideSizeInBytes = 2);
	Resource* CreateConstantBuffer(int TotalSizeInBytes);
	Resource* CreateTexture(int Width, int Height, int Depth, Format InFormat, unsigned char* InPixels = nullptr);
	void Map(Resource* InResource, void **Dest);
	void Unmap(Resource* InResource);

	// CommandList Operation
	CommandList* GetCommandList(int idx = 0);
	void Reset(CommandList * InList);
	void Close(CommandList * InList);
	void SetPrimitiveType(CommandList* InList, PrimitiveType InType);
	void SetVertexBuffer(CommandList* InList, Resource *InBuffer);
	void SetIndexBuffer(CommandList* InList, Resource *InBuffer);
	void SetScissorRect(CommandList* InList, int left, int top, int right, int bottom);
	void SetViewport3D(CommandList* InList, float TopLeftX, float TopLeftY, float Width, float Height, float MinDepth, float MaxDepth);
	void DrawIndexed(CommandList* InList, int IndexCountPerInstance, int IndexOffset, int VertexOffset, int InstanceCount = 1);
	void ClearRTV(CommandList *InList, Resource *InResource, float InColors[4]);
	void WaitForResource(CommandList * InList, Resource *InResource, BarrierState InNextState);
	// Todo: Provide better API to set the structures.
	void SetPipelineState(CommandList * InList, PipelineState * InPSO);
	void SetResourceLayout(CommandList * InList, ResourceLayout *InLayout);
	void SetResourceParameters(CommandList * InList);
	
	// CommandQueue Operation 
	void SendCommandListToQueue(CommandList* InList);
	void SendFrameSwapToQueue();
	void WaitForCommandQueue();

	// Tests
	void SetImgui(CommandList* InList, Resource *InTexture, Resource *InConst);
	void RenderTest(CommandList * InList);
	void NotifyToResizeBuffer(int Width, int Height);
	void ResizeFrameBuffers();	

	// 
	void BeginFrame(CommandList *InList);
	void EndFrame(CommandList *InList);
};


