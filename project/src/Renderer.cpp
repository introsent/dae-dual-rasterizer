#include "pch.h"
#include "Renderer.h"
#include "Mesh3D.h"
#include "Utils.h"

const std::string MAGENTA = "\033[35m";
const std::string YELLOW = "\033[33m";
const std::string GREEN = "\033[32m";
const std::string RESET = "\033[0m";
//extern ID3D11Debug* d3d11Debug;
namespace dae {

	Renderer::Renderer(SDL_Window* pWindow) :
		m_pWindow(pWindow)
	{
		//Initialize
		SDL_GetWindowSize(pWindow, &m_Width, &m_Height);

		//Initialize DirectX pipeline
		const HRESULT result = InitializeDirectX();
		if (result == S_OK)
		{
			m_IsInitialized = true;

			// Create Buffers
			m_pFrontBuffer = SDL_GetWindowSurface(pWindow);
			m_pBackBuffer = SDL_CreateRGBSurface(0, m_Width, m_Height, 32, 0, 0, 0, 0);
			m_pBackBufferPixels = (uint32_t*)m_pBackBuffer->pixels;

			m_pDepthBufferPixels = new float[m_Width * m_Height];

			m_pVehicleEffect = std::make_unique<VehicleEffect>(m_pDevice, L"resources/PosCol3D.fx");
			InitializeVehicle();

			m_pFireEffect = std::make_unique<FireEffect>(m_pDevice, L"resources/Fire3D.fx");
			InitializeFire();

			m_pCamera = std::make_unique<Camera>(Vector3{ 0.f, 0.f , -50.f }, 45.f, float(m_Width), float(m_Height));
		}
	}

	Renderer::~Renderer()
	{
		delete m_pDepthBufferPixels;
		CleanupDirectX();
	}


	void Renderer::Update(const Timer* pTimer)
	{
		m_pCamera.get()->Update(pTimer);

		if (m_IsRotating)
		{
			m_WorldMatrix = Matrix(Matrix::CreateRotationY(pTimer->GetElapsed() * PI / 4) * m_WorldMatrix);
		}
		

		// Apply transformations
		if (m_RenderingBackendType == RenderingBackendType::Software)
		{
			m_pVehicle->VertexTransformationFunction(*m_pCamera.get(), m_WorldMatrix);
			m_pFire->VertexTransformationFunction(*m_pCamera.get(), m_WorldMatrix);
		}
			
	}

	void Renderer::RenderGPU() const
	{
		if (!m_IsInitialized)
			return;

		// 1. Clear RTV & DSV
		float color[4]; // Declare the array

		if (m_IsClearColorUniform) {
			color[0] = 0.1f;
			color[1] = 0.1f;
			color[2] = 0.1f;
			color[3] = 1.0f;
		}
		else {
			color[0] = 0.39f;
			color[1] = 0.59f;
			color[2] = 0.93f;
			color[3] = 1.0f;
		}
		
		m_pDeviceContext->ClearRenderTargetView(m_pRenderTargetView, color);
		m_pDeviceContext->ClearDepthStencilView(m_pDepthStencilView, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

		//2. Set pipeline + invoke draw calls (= RENDER)
		m_pVehicle.get()->RenderGPU(m_pCamera->origin, m_WorldMatrix, m_WorldMatrix * m_pCamera->GetViewMatrix() * m_pCamera->GetProjectionMatrix(), m_pDeviceContext);
		if (m_ToRenderFireMesh)
			m_pFire.get()->RenderGPU(m_pCamera->origin, m_WorldMatrix, m_WorldMatrix * m_pCamera->GetViewMatrix() * m_pCamera->GetProjectionMatrix(), m_pDeviceContext);

		//3. Present backbuffer (SWAP)
		m_pSwapChain->Present(0, 0);
	}

	void Renderer::RenderCPU() const
	{
		// Reset depth buffer and clear screen
		std::fill(m_pDepthBufferPixels, m_pDepthBufferPixels + (m_Width * m_Height), std::numeric_limits<float>::max());

		// Clear screen with black color
		SDL_Color clearColor;
		if (m_IsClearColorUniform) 
		{
			clearColor = { int(0.1f * 255),  int(0.1f * 255),  int(0.1f * 255), 255 };
		}
		else
		{
			clearColor = { int(0.39f * 255),  int(0.39f * 255),  int(0.39f * 255), 255 };
		}

		Uint32 color = SDL_MapRGB(m_pBackBuffer->format, clearColor.r, clearColor.g, clearColor.b);
		SDL_FillRect(m_pBackBuffer, nullptr, color);

		// Lock the back buffer before drawing
		SDL_LockSurface(m_pBackBuffer);

		// RENDER LOGIC
		m_pVehicle.get()->RenderCPU(m_Width, m_Height, m_CurrentShadingMode, m_CurrentDisplayMode, m_CullingMode, *m_pCamera.get(), m_IsNormalMap, m_pBackBuffer, m_pBackBufferPixels, m_pDepthBufferPixels);
		if (m_ToRenderFireMesh)
		{
			if (m_CurrentShadingMode == ShadingMode::Combined && m_CurrentDisplayMode == DisplayMode::ShadingMode)
			{
				m_pFire.get()->RenderCPU(m_Width, m_Height, m_CurrentShadingMode, m_CurrentDisplayMode, CullingMode::No, *m_pCamera.get(), false, m_pBackBuffer, m_pBackBufferPixels, m_pDepthBufferPixels);
			}
		}
		// Unlock after rendering
		SDL_UnlockSurface(m_pBackBuffer);

		// Copy the back buffer to the front buffer for display
		SDL_BlitSurface(m_pBackBuffer, nullptr, m_pFrontBuffer, nullptr);
		SDL_UpdateWindowSurface(m_pWindow);
	}

	void Renderer::ChangeShadingMode()
	{
		switch (m_CurrentShadingMode)
		{
		case ShadingMode::Combined:
			std::cout << MAGENTA << "**(SOFTWARE) Shading Mode = OBSERVED_AREA" << RESET << std::endl;
			m_CurrentShadingMode = ShadingMode::ObservedArea;
			break;
		case ShadingMode::ObservedArea:
			std::cout << MAGENTA << "**(SOFTWARE) Shading Mode = DIFFUSE" << RESET << std::endl;
			m_CurrentShadingMode = ShadingMode::Diffuse;
			break;
		case ShadingMode::Diffuse:
			std::cout << MAGENTA << "**(SOFTWARE) Shading Mode = SPECULAR" << RESET << std::endl;
			m_CurrentShadingMode = ShadingMode::Specular;
			break;
		case ShadingMode::Specular:
			std::cout << MAGENTA << "**(SOFTWARE) Shading Mode = COMBINED" << RESET << std::endl;
			m_CurrentShadingMode = ShadingMode::Combined;
			break;
		}
	}

	void Renderer::SetDisplayMode(DisplayMode displayMode)
	{
		m_CurrentDisplayMode = displayMode;
	}

	DisplayMode Renderer::GetDisplayMode() const
	{
		return m_CurrentDisplayMode;
	}

	void Renderer::ChangeFilteringTechnique()
	{
		switch (m_FilteringTechnique)
		{
		case FilteringTechnique::Point:
			m_pVehicleEffect->SetLinearSampling();
			std::cout << GREEN << "**(HARDWARE) Sampler Filter = LINEAR" << RESET << std::endl;

			m_FilteringTechnique = FilteringTechnique::Linear;
			break;
		case FilteringTechnique::Linear:
			m_pVehicleEffect->SetAnisotropicSampling();
			std::cout << GREEN << "**(HARDWARE) Sampler Filter = ANISOTROPIC" << RESET << std::endl;

			m_FilteringTechnique = FilteringTechnique::Anisotropic;
			break;
		case FilteringTechnique::Anisotropic:
			m_pVehicleEffect->SetPointSampling();
			std::cout << GREEN << "**(HARDWARE) Sampler Filter = POINT" << RESET << std::endl;

			m_FilteringTechnique = FilteringTechnique::Point;
			break;
		}
	}

	void Renderer::ChangeRenderingBackendType()
	{
		switch (m_RenderingBackendType)
		{
		case RenderingBackendType::Software:
			std::cout << YELLOW << "**(SHARED)Rasterizer Mode = HARDWARE" << RESET << std::endl;
			m_RenderingBackendType = RenderingBackendType::Hardware;
			break;
		case RenderingBackendType::Hardware:
			std::cout << YELLOW << "**(SHARED)Rasterizer Mode = SOFTWARE" << RESET << std::endl;
			m_RenderingBackendType = RenderingBackendType::Software;
			break;
		}
	}

	void Renderer::ChangeIsRotating()
	{
		m_IsRotating = !m_IsRotating;

		if (m_IsRotating)
		{
			std::cout << YELLOW << "**(SHARED)Vehicle Rotation ON" << RESET << std::endl;
		}
		else
		{
			std::cout << YELLOW << "**(SHARED)Vehicle Rotation OFF" << RESET << std::endl;
		}
		
	}

	void Renderer::ChangeToRenderFireMesh()
	{
		m_ToRenderFireMesh = !m_ToRenderFireMesh;

		if (m_ToRenderFireMesh)
		{
			std::cout << YELLOW << "**(SHARED) FireFX ON" << RESET << std::endl;
		}
		else
		{
			std::cout << YELLOW << "**(SHARED) FireFX OFF" << RESET << std::endl;
		}
	}

	void Renderer::ChangeIsNormalMap()
	{
		m_IsNormalMap = !m_IsNormalMap; 

		if (m_IsNormalMap)
		{
			std::cout << MAGENTA << "**(SOFTWARE) NormalMap ON" << RESET << std::endl;
		}
		else
		{
			std::cout << MAGENTA << "**(SOFTWARE) NormalMap OFF" << RESET << std::endl;
		}
	}

	void Renderer::ChangeIsClearColorUniform()
	{
		m_IsClearColorUniform = !m_IsClearColorUniform;

		if (m_IsClearColorUniform)
		{
			std::cout << MAGENTA << "**(SHARED) Uniform ClearColor ON" << RESET << std::endl;
		}
		else
		{
			std::cout << MAGENTA << "**(SHARED) Uniform ClearColor OFF" << RESET << std::endl;
		}
	}

	void Renderer::ChangeCullingMode()
	{
		switch (m_CullingMode)
		{
		case CullingMode::Back:
			std::cout << YELLOW << "**(SHARED) CullMode = FRONT" << RESET << std::endl;
			m_CullingMode = CullingMode::Front;
			break;
		case CullingMode::Front:
			std::cout << YELLOW << "**(SHARED) CullMode = NONE" << RESET << std::endl;
			m_CullingMode = CullingMode::No;
			break;
		case CullingMode::No:
			std::cout << YELLOW << "**(SHARED) CullMode = BACK" << RESET << std::endl;
			m_CullingMode = CullingMode::Back;
			break;
		}

		m_pVehicle->SetCullingMode(m_CullingMode, m_pDeviceContext);
		m_pFire->SetCullingMode(CullingMode::No, m_pDeviceContext);
	}

	void Renderer::OnDeviceLost()
	{
		// Release all resources tied to the device
		CleanupDirectX();

		// Recreate device and swap chain
		InitializeDirectX();

		// Recreate resources
		InitializeVehicle();
		InitializeFire();

	}
	
	void Renderer::InitializeVehicle()
	{
		//Create some data for our mesh
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;
		
		Utils::ParseOBJ("resources/vehicle.obj", vertices, indices);

		m_pVehicle = std::make_unique<Mesh3D>(m_pDevice, vertices, indices, m_pVehicleEffect.get(), false);
	}

	void Renderer::InitializeFire()
	{
		std::vector<Vertex> vertices;
		std::vector<uint32_t> indices;

		Utils::ParseOBJ("resources/fireFX.obj", vertices, indices);

		m_pFire = std::make_unique<Mesh3D>(m_pDevice, vertices, indices, m_pFireEffect.get(), true);
	}


	HRESULT Renderer::InitializeDirectX()
	{
		//==== 1.Create Device & DeviceContext ====
		D3D_FEATURE_LEVEL featureLevel = D3D_FEATURE_LEVEL_11_1;
		uint32_t createDeviceFlags = 0;
		#if defined(DEBUG) || defined(_DEBUG)
			createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
		#endif


		HRESULT result = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, 0, createDeviceFlags, &featureLevel,
						1, D3D11_SDK_VERSION, &m_pDevice, nullptr, &m_pDeviceContext);
		if (FAILED(result)) return result;
//		m_pDevice->QueryInterface(__uuidof(ID3D11Debug), reinterpret_cast<void**>(&d3d11Debug));

		//Create DXGI Factory
		IDXGIFactory1* pDxgiFactory{};
		result = CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&pDxgiFactory));
		if (FAILED(result)) return result;

		//==== 2. Create Swapchain ====
		DXGI_SWAP_CHAIN_DESC swapChainDesc{};
		swapChainDesc.BufferDesc.Width = m_Width;
		swapChainDesc.BufferDesc.Height = m_Height;
		swapChainDesc.BufferDesc.RefreshRate.Numerator = 1;
		swapChainDesc.BufferDesc.RefreshRate.Denominator = 60;
		swapChainDesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
		swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		swapChainDesc.SampleDesc.Count = 1;
		swapChainDesc.SampleDesc.Quality = 0;
		swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		swapChainDesc.BufferCount = 1;
		swapChainDesc.Windowed = true;
		swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		swapChainDesc.Flags = 0;

		//Get the handle (HWND) from the SDL backbuffer
		SDL_SysWMinfo sysWMInfo{};
		SDL_GetVersion(&sysWMInfo.version);
		SDL_GetWindowWMInfo(m_pWindow, &sysWMInfo);
		swapChainDesc.OutputWindow = sysWMInfo.info.win.window;

		//Create Swapchain
		result = pDxgiFactory->CreateSwapChain(m_pDevice, &swapChainDesc, &m_pSwapChain);
		if (FAILED(result)) return result;
		if (pDxgiFactory)
		{
			pDxgiFactory->Release();
			pDxgiFactory = nullptr;
		}

		//==== 3. Create DepthStencil (DS) & DepthStencilView (DSV) ====
		//Resource
		D3D11_TEXTURE2D_DESC depthStencilDesc{};
		depthStencilDesc.Width = m_Width;
		depthStencilDesc.Height = m_Height;
		depthStencilDesc.MipLevels = 1;
		depthStencilDesc.ArraySize = 1;
		depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
		depthStencilDesc.SampleDesc.Count = 1;
		depthStencilDesc.SampleDesc.Quality = 0;
		depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
		depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
		depthStencilDesc.CPUAccessFlags = 0;
		depthStencilDesc.MiscFlags = 0;

		//View
		D3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc{};
		depthStencilViewDesc.Format = depthStencilDesc.Format;
		depthStencilViewDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
		depthStencilViewDesc.Texture2D.MipSlice = 0;

		result = m_pDevice->CreateTexture2D(&depthStencilDesc, nullptr, &m_pDepthStencilBuffer);
		if (FAILED(result)) return result;

		result = m_pDevice->CreateDepthStencilView(m_pDepthStencilBuffer, &depthStencilViewDesc, &m_pDepthStencilView);
		if (FAILED(result)) return result;


		//==== 4. Create RenderTarget (RT) & RenderTargetView (RTV) ====
		result = m_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&m_pRenderTargetBuffer));
		if (FAILED(result)) return result;

		result = m_pDevice->CreateRenderTargetView(m_pRenderTargetBuffer, nullptr, &m_pRenderTargetView);
		if (FAILED(result)) return result;


		//==== 5. Bind RTV & DSV to Output merger Stage ====
		m_pDeviceContext->OMSetRenderTargets(1, &m_pRenderTargetView, m_pDepthStencilView);

		//==== 6. Set Viewport ====
		D3D11_VIEWPORT viewport{};
		viewport.Width = static_cast<float>(m_Width);
		viewport.Height = static_cast<float>(m_Height);
		viewport.TopLeftX = 0.f;
		viewport.TopLeftY = 0.f;
		viewport.MinDepth = 0.f;
		viewport.MaxDepth = 1.f;
		m_pDeviceContext->RSSetViewports(1, &viewport);
		
		//return S_FALSE;
		return S_OK;
	}

	

	void Renderer::CleanupDirectX()
	{
		// Clear state and flush the device context before releasing DeviceContext
		if (m_pDepthStencilView)
		{
			m_pDepthStencilView->Release();
			m_pDepthStencilView = nullptr;
		}

		if (m_pDepthStencilBuffer)
		{
			m_pDepthStencilBuffer->Release();
			m_pDepthStencilBuffer = nullptr;
		}

		if (m_pRenderTargetView)
		{
			m_pRenderTargetView->Release();
			m_pRenderTargetView = nullptr;
		}

		if (m_pRenderTargetBuffer)
		{
			m_pRenderTargetBuffer->Release();
			m_pRenderTargetBuffer = nullptr;
		}

		if (m_pSwapChain)
		{
			m_pSwapChain->Release();
			m_pSwapChain = nullptr;
		}

		if (m_pDeviceContext)
		{
			m_pDeviceContext->ClearState();
			m_pDeviceContext->Flush();
			m_pDeviceContext->Release();
			m_pDeviceContext = nullptr;
		}

		if (m_pDevice)
		{
			m_pDevice->Release();
			m_pDevice = nullptr;
		}
	}

	void Renderer::Render() const
	{
		if (m_RenderingBackendType == RenderingBackendType::Hardware)
		{
			RenderGPU();
		}
		else if (m_RenderingBackendType == RenderingBackendType::Software)
		{
			RenderCPU();
		}
	}

}
