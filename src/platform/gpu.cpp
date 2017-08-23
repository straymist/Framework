#include <stdint.h>
#include "GPU.h"
#include <d3d12.h>
#include <dxgi1_4.h>
#include <wrl.h>
#include <D3Dcompiler.h>
#include "window.h"
#include "d3dx12.h"
#include "../imgui/imgui.h"
#include <vector>
using Microsoft::WRL::ComPtr;

namespace GPU
{

	enum class ResourceUsage : uint8_t
	{
		VB = 1,
		IB = 2,
		CB = 4,
		RTV = 8,
		SRV = 16,
		UAV = 32,
		DSV = 64
	};

	class Resource
	{
	public:
		virtual ~Resource() {};
	};

	struct DescriptorHandle
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE mCPU;
		CD3DX12_GPU_DESCRIPTOR_HANDLE mGPU;

		void Offset(int i, int ElemSize)
		{
			mCPU.Offset(i, ElemSize);
			mGPU.Offset(i, ElemSize);
		}
	};

	std::vector<Resource*> m_Dx12ResourceList;

	template< typename ResourceType >
	ResourceType *DxNew()
	{
		ResourceType * Res = new ResourceType;
		m_Dx12ResourceList.push_back(Res);
		return Res;
	}
	void ReleaseResources()
	{
		for (auto r : m_Dx12ResourceList)
		{
			delete r;
		}
	}


	class CommandList : public Resource
	{
	public:
		ComPtr<ID3D12GraphicsCommandList> m_commandList;;
	};
	class ResourceDX12 : public Resource
	{
	public:
		BarrierState m_state;
		ComPtr<ID3D12Resource> mResource;
		DescriptorHandle mSRV;
		DescriptorHandle mRTV;
	};

	class VertexBuffer : public ResourceDX12
	{
	public:
		D3D12_VERTEX_BUFFER_VIEW mView;
	};

	class IndexBuffer : public ResourceDX12
	{
	public:
		D3D12_INDEX_BUFFER_VIEW mView;
	};
	class ConstBuffer : public ResourceDX12
	{
	public:
		DescriptorHandle mCBV;
		
	};

	inline void ThrowIfFailed(HRESULT hr) { if (FAILED(hr)) { throw; } }

	// DX12 Objects
	ComPtr<IDXGIFactory4> m_factory;
	ComPtr<ID3D12Device> m_device;
	ComPtr<IDXGISwapChain3> m_swapChain;
	ComPtr<ID3D12CommandAllocator> m_commandAllocator;
	ComPtr<ID3D12CommandQueue> m_commandQueue;
	ComPtr<ID3D12RootSignature> m_rootSignature;
	CommandList* m_commandLists[4] = { nullptr };
	ComPtr<ID3D12PipelineState> m_pipelineState;

	// Frame Buffer
	int m_currentBackBufferIndex = 0;
	static const int FrameBufferCount = 2;
	ResourceDX12 m_FrameBuffers[FrameBufferCount];

	// Sync Objects
	HANDLE m_fenceEvent;
	ComPtr<ID3D12Fence> m_fence;
	UINT64 m_fenceValue;
	int m_frameIndex;

	// Just use global heap to manage all the resources for simple use ;
	ComPtr<ID3D12DescriptorHeap> m_RTVHeap;
	DescriptorHandle m_RTVHandle;

	ComPtr<ID3D12DescriptorHeap> m_CBV_SRV_Heap;
	DescriptorHandle m_SRVTableStart;
	DescriptorHandle m_SRVHandle;
	DescriptorHandle m_CBVTableStart;
	DescriptorHandle m_CBVHandle;

	static int m_rtvDescriptorSize = 0;
	static int m_srvDescriptorSize = 0;


	DescriptorHandle AllocateCBV()
	{
		DescriptorHandle RetHandle = m_CBVHandle;
		m_CBVHandle.Offset(1, m_srvDescriptorSize);
		return RetHandle;
	}

	DescriptorHandle AllocateRTV()
	{
		DescriptorHandle RetHandle = m_RTVHandle;
		m_RTVHandle.Offset(1, m_rtvDescriptorSize);
		return RetHandle;
	}
	DescriptorHandle AllocateSRV()
	{
		DescriptorHandle RetHandle = m_SRVHandle;
		m_SRVHandle.Offset(1, m_srvDescriptorSize);
		return RetHandle;
	}
	void CreateDescrptorHeaps()
	{
		// Describe and create a render target view (RTV) descriptor heap.
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = FrameBufferCount;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(m_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&m_RTVHeap)));
		
		// Describe and create a shader resource view (SRV) heap for the texture.
		D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
		srvHeapDesc.NumDescriptors = 16;  //
		srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
		srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
		ThrowIfFailed(m_device->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&m_CBV_SRV_Heap)));

	
		m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		m_srvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		m_RTVHandle.mCPU = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_RTVHeap->GetCPUDescriptorHandleForHeapStart());
		m_RTVHandle.mGPU = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_RTVHeap->GetGPUDescriptorHandleForHeapStart());


		m_CBVHandle.mCPU = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_CBV_SRV_Heap->GetCPUDescriptorHandleForHeapStart());
		m_CBVHandle.mGPU = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_CBV_SRV_Heap->GetGPUDescriptorHandleForHeapStart());
		m_CBVTableStart = m_CBVHandle;


		m_SRVHandle.mCPU = CD3DX12_CPU_DESCRIPTOR_HANDLE(m_CBV_SRV_Heap->GetCPUDescriptorHandleForHeapStart(), srvHeapDesc.NumDescriptors/2, m_srvDescriptorSize);
		m_SRVHandle.mGPU = CD3DX12_GPU_DESCRIPTOR_HANDLE(m_CBV_SRV_Heap->GetGPUDescriptorHandleForHeapStart(), srvHeapDesc.NumDescriptors/2, m_srvDescriptorSize);
		m_SRVTableStart = m_SRVHandle;
	}

	DXGI_FORMAT GetDxFormat(Format InFormat)
	{
		DXGI_FORMAT DxFormat = DXGI_FORMAT_UNKNOWN;
		switch (InFormat)
		{
		case Format::RGBA32: DxFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
		}
		return DxFormat;
	}
	int GetPixelSize(Format InFormat)
	{
		int FmtSize = 0;
		switch (InFormat)
		{
		case Format::RGBA32: FmtSize = 4;
		}
		return FmtSize;
	}

	D3D12_RESOURCE_DIMENSION GetTextureDimesion(int W, int H, int D)
	{
		if (W != 0 && H == 0 && D == 0)
			return D3D12_RESOURCE_DIMENSION_TEXTURE1D;
		else if (W != 0 && H != 0 && D == 0)
			return D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		else if (W != 0 && H != 0 && D != 0)
			return D3D12_RESOURCE_DIMENSION_TEXTURE3D;
		else
			return D3D12_RESOURCE_DIMENSION_UNKNOWN;;
	}

	D3D12_SRV_DIMENSION GetSRVDimesion(int W, int H, int D)
	{
		if (W != 0 && H == 0 && D == 0)
			return D3D12_SRV_DIMENSION_TEXTURE1D;
		else if (W != 0 && H != 0 && D == 0)
			return D3D12_SRV_DIMENSION_TEXTURE2D;
		else if (W != 0 && H != 0 && D != 0)
			return D3D12_SRV_DIMENSION_TEXTURE3D;
		else
			return D3D12_SRV_DIMENSION_UNKNOWN;;
	}

	Resource* CreateTexture(int Width, int Height, int Depth, Format InFormat, unsigned char* InPixels)
	{
		// Create the texture.
		ResourceDX12 * TexResource = DxNew<ResourceDX12>();
		{
			// Describe and create a Texture2D.
			D3D12_RESOURCE_DESC textureDesc = {};
			textureDesc.MipLevels = 1;
			textureDesc.Format = GetDxFormat(InFormat);
			textureDesc.Width = Width;
			textureDesc.Height = Height;
			textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
			textureDesc.DepthOrArraySize = 1;
			textureDesc.SampleDesc.Count = 1;
			textureDesc.SampleDesc.Quality = 0;
			textureDesc.Dimension = GetTextureDimesion (Width, Height, Depth);

			ThrowIfFailed(m_device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
				D3D12_HEAP_FLAG_NONE,
				&textureDesc,
				D3D12_RESOURCE_STATE_COPY_DEST,
				nullptr,
				IID_PPV_ARGS(&TexResource->mResource)));
			TexResource->m_state = GPU::BarrierState::COPY_DST;

			const UINT64 uploadBufferSize = GetRequiredIntermediateSize(TexResource->mResource.Get(), 0, 1);

			// Create the GPU upload buffer.
			ComPtr<ID3D12Resource> textureUploadHeap;
			ThrowIfFailed(m_device->CreateCommittedResource(
				&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
				D3D12_HEAP_FLAG_NONE,
				&CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize),
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&textureUploadHeap)));

			D3D12_SUBRESOURCE_DATA textureData = {};
			textureData.pData = InPixels;
			textureData.RowPitch = Width * GetPixelSize(InFormat);
			textureData.SlicePitch = textureData.RowPitch * Height;

			auto List = GetCommandList();
			GPU::Reset(List);
				UpdateSubresources(List->m_commandList.Get(), TexResource->mResource.Get(), textureUploadHeap.Get(), 0, 0, 1, &textureData);
				GPU::WaitForResource(List, TexResource, BarrierState::PIXEL_SRV);
				GPU::Close(List);
				GPU::SendCommandListToQueue(List);
			GPU::WaitForCommandQueue();

			// Describe and create a SRV for the texture.
			DescriptorHandle srvHandle = AllocateSRV();
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format = textureDesc.Format;
			srvDesc.ViewDimension = GetSRVDimesion ( Width, Height, Depth) ;
			srvDesc.Texture2D.MipLevels = 1;
			m_device->CreateShaderResourceView(TexResource->mResource.Get(), &srvDesc, srvHandle.mCPU);
			TexResource->mSRV = srvHandle;
		}
		return TexResource;
	}
	bool m_NeedToResizeFrameBuffer = false;
	int m_FrameBufferWidth = 0 ;
	int m_FrameBufferHeight = 0 ;
	void NotifyToResizeBuffer(int Width, int Height)
	{
		m_FrameBufferWidth = Width;
		m_FrameBufferHeight = Height;
		m_NeedToResizeFrameBuffer = true;
	}
	void ResizeFrameBuffers()
	{
		if (m_device.Get() == nullptr)
			return;

		if (!m_NeedToResizeFrameBuffer)
			return;
		m_NeedToResizeFrameBuffer = false;

		for (UINT n = 0; n < FrameBufferCount; n++)
		{
			m_FrameBuffers[n].mResource.Reset();
		}

		ThrowIfFailed (m_swapChain->ResizeBuffers(0, m_FrameBufferWidth, m_FrameBufferHeight, DXGI_FORMAT_UNKNOWN, 0 ));

		// Create Resource/RTV for each frame buffer.
		for (UINT n = 0; n < FrameBufferCount; n++)
		{
			ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_FrameBuffers[n].mResource)));

			m_device->CreateRenderTargetView(m_FrameBuffers[n].mResource.Get(), nullptr, m_FrameBuffers[n].mRTV.mCPU);
		}

	}


	void CreateFrameBuffers()
	{
		
		// Create Resource/RTV for each frame buffer.
		for (UINT n = 0; n < FrameBufferCount; n++)
		{
			ThrowIfFailed(m_swapChain->GetBuffer(n, IID_PPV_ARGS(&m_FrameBuffers[n].mResource)));

			DescriptorHandle rtvHandle = AllocateRTV();
			m_device->CreateRenderTargetView(m_FrameBuffers[n].mResource.Get(), nullptr, rtvHandle.mCPU);
			m_FrameBuffers[n].mRTV = rtvHandle;
				
		}
	}

	void CreateDebugLayer()
	{
//		#if defined(_DEBUG)
				// Enable the D3D12 debug layer.
			{
				ComPtr<ID3D12Debug> debugController;
				if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
				{
					debugController->EnableDebugLayer();
				}
			}
	//	#endif
	}

	void CreateDevice()
	{
		ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&m_factory)));

		{
			class Local
			{
			public:
				static void GetHardwareAdapter(IDXGIFactory2* pFactory, IDXGIAdapter1** ppAdapter)
				{
					ComPtr<IDXGIAdapter1> adapter;
					*ppAdapter = nullptr;

					for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != pFactory->EnumAdapters1(adapterIndex, &adapter); ++adapterIndex)
					{
						DXGI_ADAPTER_DESC1 desc;
						adapter->GetDesc1(&desc);

						if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
						{
							// Don't select the Basic Render Driver adapter.
							// If you want a software adapter, pass in "/warp" on the command line.
							continue;
						}

						// Check to see if the adapter supports Direct3D 12, but don't create the
						// actual device yet.
						if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
						{
							break;
						}
					}

					*ppAdapter = adapter.Detach();
				}
			};

			ComPtr<IDXGIAdapter1> hardwareAdapter;
			Local::GetHardwareAdapter(m_factory.Get(), &hardwareAdapter);

			ThrowIfFailed(D3D12CreateDevice(
				hardwareAdapter.Get(),
				D3D_FEATURE_LEVEL_11_0,
				IID_PPV_ARGS(&m_device)
				));
		}
	}

	void CreateCommandQueue()
	{
		// Describe and create the command queue.
		D3D12_COMMAND_QUEUE_DESC queueDesc = {};
		queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		ThrowIfFailed(m_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_commandQueue)));		
	}

	void CreateSwapChain()
	{

		// Describe and create the swap chain.
		DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
		swapChainDesc.BufferCount = FrameBufferCount;
		swapChainDesc.Width = gWindow.Width;
		swapChainDesc.Height = gWindow.Height;
		swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		swapChainDesc.SampleDesc.Count = 1;

		ComPtr<IDXGISwapChain1> swapChain;
		ThrowIfFailed(m_factory->CreateSwapChainForHwnd(
			m_commandQueue.Get(),		// Swap chain needs the queue so that it can force a flush on it.
			(HWND)gWindow.Handle,
			&swapChainDesc,
			nullptr,
			nullptr,
			&swapChain
			));

		// This sample does not support fullscreen transitions.
		ThrowIfFailed(m_factory->MakeWindowAssociation((HWND)gWindow.Handle, DXGI_MWA_NO_ALT_ENTER));

		ThrowIfFailed(swapChain.As(&m_swapChain));
		m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
	}

	void CreateCommandAllocator()
	{
		ThrowIfFailed(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator)));
	}


	CommandList* CreateCommandList()
	{
		// Create the command list.
		CommandList *CmdList = DxNew<CommandList>();
		ThrowIfFailed(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator.Get(), nullptr, IID_PPV_ARGS(&CmdList->m_commandList)));

		// Command lists are created in the recording state, but there is nothing
		// to record yet. The main loop expects it to be closed, so close it now.
		ThrowIfFailed(CmdList->m_commandList->Close());
		return CmdList;

	}

	CommandList* GetCommandList(int idx /* = 0 */)
	{
		if (m_commandLists[idx] == nullptr)
		{
			m_commandLists[idx] = CreateCommandList();
		}
		return m_commandLists[idx];
		
	}


	void CreateSyncObjects()
	{
		ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
		m_fenceValue = 1;

		// Create an event handle to use for frame synchronization.
		m_fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (m_fenceEvent == nullptr)
		{
			ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
		}		
	}

	void CreateImguiPipelineState()
	{
		// Create root signature ( resource layout ) 
		{
			// 1 cbuffer, 1 texture
			CD3DX12_DESCRIPTOR_RANGE rangesCBV[1];
			rangesCBV[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
			CD3DX12_DESCRIPTOR_RANGE rangesSRV[1];
			rangesSRV[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
			
			CD3DX12_ROOT_PARAMETER rootParameters[2];
			rootParameters[0].InitAsDescriptorTable(1, &rangesCBV[0], D3D12_SHADER_VISIBILITY_ALL);
			rootParameters[1].InitAsDescriptorTable(1, &rangesSRV[0], D3D12_SHADER_VISIBILITY_ALL);

			//  1 sampler 
			D3D12_STATIC_SAMPLER_DESC sampler = {};
			sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
			sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
			sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
			sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
			sampler.MipLODBias = 0;
			sampler.MaxAnisotropy = 0;
			sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
			sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
			sampler.MinLOD = 0.0f;
			sampler.MaxLOD = D3D12_FLOAT32_MAX;
			sampler.ShaderRegister = 0;
			sampler.RegisterSpace = 0;
			sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

			CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
			rootSignatureDesc.Init(_countof(rootParameters), rootParameters, 1, &sampler, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

			ComPtr<ID3DBlob> signature;
			ComPtr<ID3DBlob> error;
			ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
			ThrowIfFailed(m_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&m_rootSignature)));
		}
	
		// Create Vertex Shader / Pixel Shader
		static const char* VSBuffer =
		"cbuffer vertexBuffer : register(b0) \
        {\
			float4x4 ProjectionMatrix; \
        };\
        struct VS_INPUT\
        {\
			float2 pos : POSITION;\
			float4 col : COLOR0;\
			float2 uv  : TEXCOORD0;\
        };\
        \
        struct PS_INPUT\
        {\
			float4 pos : SV_POSITION;\
			float4 col : COLOR0;\
			float2 uv  : TEXCOORD0;\
        };\
        \
        PS_INPUT main(VS_INPUT input)\
        {\
			PS_INPUT output;\
			output.pos = mul( ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));\
			output.col = input.col;\
			output.uv  = input.uv;\
			return output;\
        }";
		static const char* PSBuffer =
		"struct PS_INPUT\
        {\
			float4 pos : SV_POSITION;\
			float4 col : COLOR0;\
			float2 uv  : TEXCOORD0;\
        };\
        sampler sampler0;\
        Texture2D texture0;\
        \
        float4 main(PS_INPUT input) : SV_Target\
        {\
			float4 out_col = input.col * texture0.Sample(sampler0, input.uv); \
			return out_col; \
        }";
		ComPtr<ID3DBlob> VSBlob;
		ComPtr<ID3DBlob> PSBlob;
		D3DCompile(VSBuffer, strlen(VSBuffer), NULL, NULL, NULL, "main", "vs_5_0", 0, 0, &VSBlob, NULL);
		D3DCompile(PSBuffer, strlen(PSBuffer), NULL, NULL, NULL, "main", "ps_5_0", 0, 0, &PSBlob, NULL);

		// Vertex Layout 
		D3D12_INPUT_ELEMENT_DESC VertexLayout[] = 
		{
			{ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (size_t)(&((ImDrawVert*)0)->pos), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,   0, (size_t)(&((ImDrawVert*)0)->uv),  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{ "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, (size_t)(&((ImDrawVert*)0)->col), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};
	
		// Create Pipeline State
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { VertexLayout, _countof(VertexLayout) };
		psoDesc.pRootSignature = m_rootSignature.Get();
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(VSBlob.Get());
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(PSBlob.Get());
		auto RS = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		RS.CullMode = D3D12_CULL_MODE_NONE;
		psoDesc.RasterizerState = RS;

		auto BS = CD3DX12_BLEND_DESC(D3D12_DEFAULT);

		BS.RenderTarget[0].BlendEnable = true;
		BS.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
		BS.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
		BS.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		BS.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
		BS.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
		BS.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
		BS.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;


		psoDesc.BlendState = BS;
		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.DepthStencilState.StencilEnable = FALSE;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = FrameBufferCount;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;
		ThrowIfFailed(m_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_pipelineState)));

	}


	void Open()
	{
		CreateDebugLayer();
		CreateDevice();
		CreateCommandQueue();
		CreateSwapChain();
		CreateCommandAllocator();
		CreateDescrptorHeaps();
		CreateFrameBuffers();
		CreateSyncObjects();
		CreateImguiPipelineState();
	}

	void Reset(CommandList * Command)
	{
		// Command list allocators can only be reset when the associated 
		// command lists have finished execution on the GPU; apps should use 
		// fences to determine GPU execution progress.
		ThrowIfFailed(m_commandAllocator->Reset());

		// However, when ExecuteCommandList() is called on a particular command 
		// list, that command list can then be reset at any time and must be before 
		// re-recording.
		ThrowIfFailed(Command->m_commandList->Reset(m_commandAllocator.Get(), m_pipelineState.Get()));
	}

	D3D12_RESOURCE_STATES GetDX12BarrierState(BarrierState InNextState)
	{
		D3D12_RESOURCE_STATES DxBarrierState = D3D12_RESOURCE_STATE_COMMON;
		switch (InNextState)
		{
		case BarrierState::COMMON: DxBarrierState = D3D12_RESOURCE_STATE_COMMON; break;
		case BarrierState::CONSTANT_BUFFER: DxBarrierState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER; break;
		case BarrierState::VERTEX_BUFFER: DxBarrierState = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER; break;
		case BarrierState::INDEX_BUFFER: DxBarrierState = D3D12_RESOURCE_STATE_INDEX_BUFFER; break;
		case BarrierState::RTV: DxBarrierState = D3D12_RESOURCE_STATE_RENDER_TARGET; break;
		case BarrierState::UAV: DxBarrierState = D3D12_RESOURCE_STATE_UNORDERED_ACCESS; break;
		case BarrierState::DEPTH_WRITE: DxBarrierState = D3D12_RESOURCE_STATE_DEPTH_WRITE; break;
		case BarrierState::DEPTH_READ: DxBarrierState = D3D12_RESOURCE_STATE_DEPTH_READ; break;
		case BarrierState::NON_PIXEL_SRV: DxBarrierState = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE; break;
		case BarrierState::PIXEL_SRV: DxBarrierState = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE; break;
		case BarrierState::COPY_DST: DxBarrierState = D3D12_RESOURCE_STATE_COPY_DEST; break;
		case BarrierState::COPY_SRC: DxBarrierState = D3D12_RESOURCE_STATE_COPY_SOURCE; break;
		case BarrierState::PRESENT: DxBarrierState = D3D12_RESOURCE_STATE_PRESENT; break;
		default:
			break;
		}
		return DxBarrierState;
	}
	void WaitForResource(CommandList * InCommandList, Resource *InResource, BarrierState InNextState)
	{
		auto DxResource = (ResourceDX12*)InResource;
		auto cmd = InCommandList->m_commandList;
		D3D12_RESOURCE_STATES CurrState = GetDX12BarrierState(DxResource->m_state);
		D3D12_RESOURCE_STATES NextState = GetDX12BarrierState(InNextState);
		
		cmd->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(DxResource->mResource.Get(), CurrState, NextState));
		DxResource->m_state = InNextState;

	}
	void ClearRTV(CommandList * InCommandList, Resource *InResource, float colors[4])
	{
		auto cmd = InCommandList->m_commandList;
		ResourceDX12 *Res = (ResourceDX12 *)InResource;
		cmd->ClearRenderTargetView(Res->mRTV.mCPU, colors, 0, nullptr);

	}
	void Close(CommandList * InCommandList)
	{
		auto cmd = InCommandList->m_commandList;
		ThrowIfFailed(cmd->Close());
	}
	void SetPrimitiveType(CommandList* InList, PrimitiveType InType)
	{
		InList->m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	}
	void SetVertexBuffer(CommandList* InList, Resource *InBuffer)
	{
		VertexBuffer *ResBuffer = (VertexBuffer*)(InBuffer);
		InList->m_commandList->IASetVertexBuffers(0, 1, &ResBuffer->mView);
	}
	void SetIndexBuffer(CommandList* InList, Resource *InBuffer)
	{
		IndexBuffer *ResBuffer = (IndexBuffer*)(InBuffer);
		InList->m_commandList->IASetIndexBuffer(&ResBuffer->mView);
	}
	void DrawIndexed(CommandList* InList, int IndexCountPerInstance, int IndexOffset, int VertexOffset, int InstanceCount )
	{
		InList->m_commandList->DrawIndexedInstanced(IndexCountPerInstance, InstanceCount, IndexOffset, VertexOffset, 0);
	}
	void SetScissorRect(CommandList* InList, int left, int top, int right, int bottom)
	{
		D3D12_RECT r = { left, top, right, bottom };
		InList->m_commandList->RSSetScissorRects(1, &r);
	}
	void SetViewport3D(CommandList* InList, float TopLeftX, float TopLeftY, float Width, float Height, float MinDepth, float MaxDepth)
	{
		D3D12_VIEWPORT vp;
		vp.TopLeftX = TopLeftX;
		vp.TopLeftY = TopLeftY;
		vp.Width = Width;
		vp.Height = Height;
		vp.MinDepth = MinDepth;
		vp.MaxDepth = MaxDepth;
		InList->m_commandList->RSSetViewports(1, &vp);
	}


	void BeginFrame(CommandList * List)
	{
		GPU::Reset(List);

		GPU::WaitForResource(List, &m_FrameBuffers[m_frameIndex], BarrierState::RTV);

		List->m_commandList->OMSetRenderTargets(1, &m_FrameBuffers[m_frameIndex].mRTV.mCPU, FALSE, nullptr);

		float ClearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };

		GPU::ClearRTV(List, &m_FrameBuffers[m_frameIndex], ClearColor);
	}

	void EndFrame(CommandList *List)
	{
		// Indicate that the back buffer will now be used to present.
		GPU::WaitForResource(List, &m_FrameBuffers[m_frameIndex], BarrierState::PRESENT);

		GPU::Close(List);

		// Execute the command list.
		GPU::SendCommandListToQueue(List);

		// Present the frame.
		GPU::SendFrameSwapToQueue();

		// Wait GPU Done
		GPU::WaitForCommandQueue();

		ResizeFrameBuffers();

		// The Next Frame
		m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
	}

	void SendCommandListToQueue(CommandList* InCommandList)
	{
		ID3D12CommandList* ppCommandLists[] = { InCommandList->m_commandList.Get() };
		m_commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);
	}

	void SendFrameSwapToQueue()
	{
		ThrowIfFailed(m_swapChain->Present(1, 0));  // Queue Present Command in CommandQueue
	}

	void WaitForCommandQueue()
	{
		// WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
		// This is code implemented as such for simplicity. More advanced samples 
		// illustrate how to use fences for efficient resource usage.

		const UINT64 fence = m_fenceValue;
		ThrowIfFailed(m_commandQueue->Signal(m_fence.Get(), fence));
		m_fenceValue++;

		// Wait until the previous frame is finished.
		if (m_fence->GetCompletedValue() < fence)
		{
			ThrowIfFailed(m_fence->SetEventOnCompletion(fence, m_fenceEvent));
			WaitForSingleObject(m_fenceEvent, INFINITE);
		}
	}

	Resource * CreateVertexBuffer(int TotalSizeInBytes, int StrideSizeInBytes)
	{

		VertexBuffer *Buffer = DxNew<VertexBuffer>();
		ThrowIfFailed(m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(TotalSizeInBytes),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&Buffer->mResource)));


		// Initialize the vertex buffer view.
		Buffer->mView.BufferLocation = Buffer->mResource->GetGPUVirtualAddress();
		Buffer->mView.StrideInBytes = StrideSizeInBytes;
		Buffer->mView.SizeInBytes = TotalSizeInBytes;
		return Buffer;
	}


	Resource * CreateIndexBuffer(int TotalSizeInBytes, int StrideSizeInBytes)
	{
		IndexBuffer *Buffer = DxNew<IndexBuffer>();
		ThrowIfFailed(m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(TotalSizeInBytes),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&Buffer->mResource)));

		// Initialize the vertex buffer view.
		Buffer->mView.BufferLocation = Buffer->mResource->GetGPUVirtualAddress();
		Buffer->mView.SizeInBytes = TotalSizeInBytes;
		Buffer->mView.Format = (StrideSizeInBytes == 2) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
		return Buffer;
	}
	void SetImgui(CommandList* InList, Resource *InTexture, Resource *InConst)
	{
		ID3D12DescriptorHeap* ppHeaps[] = { m_CBV_SRV_Heap.Get() };

		InList->m_commandList->SetGraphicsRootSignature(m_rootSignature.Get());
		InList->m_commandList->SetPipelineState(m_pipelineState.Get());

		InList->m_commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

		auto DxTexture = (ResourceDX12 *)InTexture;
		auto DxConstBuf = (ConstBuffer*) InConst;
		InList->m_commandList->SetGraphicsRootDescriptorTable(0, DxConstBuf->mCBV.mGPU);
		InList->m_commandList->SetGraphicsRootDescriptorTable(1, DxTexture->mSRV.mGPU);
	}

	Resource * CreateConstantBuffer(int TotalSizeInBytes)
	{
		ConstBuffer *Buffer = DxNew<ConstBuffer>();
		TotalSizeInBytes = (TotalSizeInBytes + 255) & ~255;	// CB size is required to be 256-byte aligned.
		ThrowIfFailed(m_device->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(TotalSizeInBytes),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&Buffer->mResource)));

		// Describe and create a constant buffer view.
		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = Buffer->mResource->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = TotalSizeInBytes;

		auto CBVHandle = AllocateCBV();
		m_device->CreateConstantBufferView(&cbvDesc, CBVHandle.mCPU);
		Buffer->mCBV = CBVHandle;

		return Buffer;
	}

	void Map(Resource* InResource, void **Dest)
	{
		ResourceDX12 *Data = (ResourceDX12*)InResource;
		Data->mResource->Map(0, nullptr, Dest);
	}
	void Unmap(Resource* InResource)
	{
		ResourceDX12 *Data = (ResourceDX12*)InResource;
		Data->mResource->Unmap(0, nullptr);
	}


	void Close()
	{
		ReleaseResources();
	}
};