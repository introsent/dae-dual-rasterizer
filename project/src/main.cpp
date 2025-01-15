#include "pch.h"

#if defined(_DEBUG)
#include "vld.h"
#endif

#undef main
#include "Renderer.h"

using namespace dae;

//ID3D11Debug* d3d11Debug;

void ShutDown(SDL_Window* pWindow)
{
	//d3d11Debug->ReportLiveDeviceObjects(D3D11_RLDO_DETAIL | D3D11_RLDO_IGNORE_INTERNAL);
	//
	//if (d3d11Debug)
	//{
	//	d3d11Debug->Release();
	//	d3d11Debug = nullptr;
	//}

	SDL_DestroyWindow(pWindow);
	SDL_Quit();
}




int main(int argc, char* args[])
{
	//Unreferenced parameters
	(void)argc;
	(void)args;

	//Create window + surfaces
	SDL_Init(SDL_INIT_VIDEO);

	const uint32_t width = 640;
	const uint32_t height = 480;

	SDL_Window* pWindow = SDL_CreateWindow(
		"DirectX - Ivans Minajevs / 2GD11",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		width, height, 0);

	if (!pWindow)
		return 1;

	//Initialize "framework"
	const auto pTimer = new Timer();
	const auto pRenderer = new Renderer(pWindow);

	//Start loop
	pTimer->Start();
	float printTimer = 0.f;
	bool isLooping = true;
	while (isLooping)
	{
		//--------- Get input events ---------
		SDL_Event e;
		while (SDL_PollEvent(&e))
		{
			switch (e.type)
			{
			case SDL_QUIT:
				isLooping = false;
				break;
			case SDL_KEYUP:
				//Test for a key
				if (e.key.keysym.scancode == SDL_SCANCODE_F1)
				{
					pRenderer->ChangeRenderingBackendType();
				}
				if (e.key.keysym.scancode == SDL_SCANCODE_F2)
				{
					pRenderer->ChangeIsRotating();
				}
				if (e.key.keysym.scancode == SDL_SCANCODE_F3)
				{
					pRenderer->ChangeToRenderFireMesh();
				}
				if (e.key.keysym.scancode == SDL_SCANCODE_F4)
				{
					pRenderer->ChangeFilteringTechnique();
				}
				if (e.key.keysym.scancode == SDL_SCANCODE_F5)
				{
					if (pRenderer->GetDisplayMode() == DisplayMode::DepthBuffer)
					{
						pRenderer->SetDisplayMode(DisplayMode::ShadingMode);
					}
					else
					{
						pRenderer->ChangeShadingMode();
					}
				}
				if (e.key.keysym.scancode == SDL_SCANCODE_F6)
				{
					pRenderer->ChangeIsNormalMap();
				}
				if (e.key.keysym.scancode == SDL_SCANCODE_F7)
				{
					pRenderer->SetDisplayMode(DisplayMode::DepthBuffer);
				}
				if (e.key.keysym.scancode == SDL_SCANCODE_F7)
				{
					pRenderer->SetDisplayMode(DisplayMode::DepthBuffer);
				}
				if (e.key.keysym.scancode == SDL_SCANCODE_F8)
				{
					pRenderer->SetDisplayMode(DisplayMode::BoundingBox);
				}

				break;
			default: ;
			}
		}

		//--------- Update ---------
		pRenderer->Update(pTimer);

		//--------- Render ---------
		pRenderer->Render();

		//--------- Timer ---------
		pTimer->Update();
		printTimer += pTimer->GetElapsed();
		if (printTimer >= 1.f)
		{
			printTimer = 0.f;
			std::cout << "dFPS: " << pTimer->GetdFPS() << std::endl;
		}
	}
	pTimer->Stop();

	//Shutdown "framework"
	delete pRenderer;

	

	delete pTimer;

	ShutDown(pWindow);
	return 0;
}