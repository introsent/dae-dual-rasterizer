#include "Effect.h"
#include "VehicleEffect.h"

VehicleEffect::VehicleEffect(ID3D11Device* pDevice, const std::wstring& assetFile) : Effect(pDevice, assetFile)
{
	//World matrix
	m_pMatWorldVariable = m_pEffect->GetVariableByName("gWorldMatrix")->AsMatrix();
	if (!m_pMatWorldVariable->IsValid())
	{
		std::wcout << L"m_pMatWorldVariable not valid\n";
	}

	//Camera
	m_pVecCameraVariable = m_pEffect->GetVariableByName("gCameraPosition")->AsVector();
	if (!m_pVecCameraVariable->IsValid())
	{
		std::wcout << L"m_pVecCameraVariable not valid\n";
	}

	//Effect sampler
	m_EffectSamplerVariable = m_pEffect->GetVariableByName("gSamplerState")->AsSampler();
	if (!m_EffectSamplerVariable->IsValid())
	{
		std::wcout << L"Effect sampler veriable is not valid\n";
		return;
	}


	// Point sampler
	D3D11_SAMPLER_DESC pointDesc = {};
	pointDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	pointDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	pointDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	pointDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	pointDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	pointDesc.MinLOD = 0;
	pointDesc.MaxLOD = D3D11_FLOAT32_MAX;
	HRESULT hr = pDevice->CreateSamplerState(&pointDesc, &m_pSamplerPoint);
	if (FAILED(hr))
	{
		std::wcout << L"Point Sampling state not valid\n";
	}

	// Linear sampler
	D3D11_SAMPLER_DESC linearDesc = {};
	linearDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	linearDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	linearDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	linearDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	linearDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	linearDesc.MinLOD = 0;
	linearDesc.MaxLOD = D3D11_FLOAT32_MAX;
	hr = pDevice->CreateSamplerState(&linearDesc, &m_pSamplerLinear);
	if (FAILED(hr))
	{
		std::wcout << L"Linear Sampling state not valid\n";
	}

	// Anisotropic sampler
	D3D11_SAMPLER_DESC anisotropicDesc = {};
	anisotropicDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	anisotropicDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	anisotropicDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	anisotropicDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	anisotropicDesc.ComparisonFunc = D3D11_COMPARISON_ALWAYS;
	anisotropicDesc.MinLOD = 0;
	anisotropicDesc.MaxLOD = D3D11_FLOAT32_MAX;
	anisotropicDesc.MaxAnisotropy = 16; // Maximum anisotropy level
	hr = pDevice->CreateSamplerState(&anisotropicDesc, &m_pSamplerAnisotropic);
	if (FAILED(hr))
	{
		std::wcout << L"Anisotropic Sampling state not valid\n";
	}

	if (m_pSamplerAnisotropic != nullptr)
	{
		m_EffectSamplerVariable->SetSampler(0, m_pSamplerAnisotropic);
	}


	m_pUDiffuseTexture = Texture::LoadFromFile(pDevice, "resources/vehicle_diffuse.png");
	ID3DX11EffectShaderResourceVariable* pDiffuseMapVariable = m_pEffect->GetVariableByName("gDiffuseMap")->AsShaderResource();
	if (pDiffuseMapVariable->IsValid()) {
		pDiffuseMapVariable->SetResource(m_pUDiffuseTexture.get()->GetShaderResourceView());
	}
	else
	{
		std::wcout << L"m_pDiffuseMapVariable not valid!\n";
	}

	m_pUNormalTexture = Texture::LoadFromFile(pDevice, "resources/vehicle_normal.png");
	ID3DX11EffectShaderResourceVariable*  pNormalMapVariable = m_pEffect->GetVariableByName("gNormalMap")->AsShaderResource();
	if (pNormalMapVariable->IsValid())
	{
		pNormalMapVariable->SetResource(m_pUNormalTexture.get()->GetShaderResourceView());
	}
	else
	{
		std::wcout << L"m_pNormalMapVariable not valid!\n";
	}

	m_pUSpecularTexture = Texture::LoadFromFile(pDevice, "resources/vehicle_specular.png");
	ID3DX11EffectShaderResourceVariable*  pSpecularMapVariable = m_pEffect->GetVariableByName("gSpecularMap")->AsShaderResource();
	if (pSpecularMapVariable->IsValid())
	{
		pSpecularMapVariable->SetResource(m_pUSpecularTexture.get()->GetShaderResourceView());
		
	}
	else
	{
		std::wcout << L"m_pSpecularMapVariable not valid!\n";
	}

	m_pUGlossinessTexture = Texture::LoadFromFile(pDevice, "resources/vehicle_gloss.png");
	ID3DX11EffectShaderResourceVariable* pGlossinessMapVariable = m_pEffect->GetVariableByName("gGlossinessMap")->AsShaderResource();
	if (pGlossinessMapVariable->IsValid())
	{
		pGlossinessMapVariable->SetResource(m_pUGlossinessTexture.get()->GetShaderResourceView());
	}
	else
	{
		std::wcout << L"m_pGlossinessMapVariable not valid!\n";
	}

}

VehicleEffect::~VehicleEffect()
{
	//Release vectors
	if (m_pVecCameraVariable)
	{
		m_pVecCameraVariable->Release();
		m_pVecCameraVariable = nullptr;
	}

	//Release matrices
	if (m_pMatWorldVariable)
	{
		m_pMatWorldVariable->Release();
		m_pMatWorldVariable = nullptr;
	}

	//Release samplers
	if (m_pSamplerAnisotropic)
	{
		m_pSamplerAnisotropic->Release();
		m_pSamplerAnisotropic = nullptr;
	}

	if (m_pSamplerLinear)
	{
		m_pSamplerLinear->Release();
		m_pSamplerLinear = nullptr;
	}

	if (m_pSamplerPoint)
	{
		m_pSamplerPoint->Release();
		m_pSamplerPoint = nullptr;
	}

	if (m_EffectSamplerVariable)
	{
		m_EffectSamplerVariable->Release();
		m_EffectSamplerVariable = nullptr;
	}
}

void VehicleEffect::SetPointSampling()
{
	HRESULT hr = m_EffectSamplerVariable->SetSampler(0, m_pSamplerPoint);
	if (SUCCEEDED(hr))
	{
		return;
	}
}

void VehicleEffect::SetLinearSampling()
{
	HRESULT hr = m_EffectSamplerVariable->SetSampler(0, m_pSamplerLinear);
	if (SUCCEEDED(hr))
	{
		return;
	}
}

void VehicleEffect::SetAnisotropicSampling()
{
	HRESULT hr = m_EffectSamplerVariable->SetSampler(0, m_pSamplerAnisotropic);
	if (SUCCEEDED(hr))
	{
		return;
	}
}

Texture* VehicleEffect::GetDiffuseTexture()
{
	return  m_pUDiffuseTexture.get();
}

Texture* VehicleEffect::GetNormalTexture()
{
	return  m_pUNormalTexture.get();
}

Texture* VehicleEffect::GetSpecularTexture()
{
	return m_pUSpecularTexture.get();
}

Texture* VehicleEffect::GetGlossinessTexture()
{
	return m_pUGlossinessTexture.get();
}

void VehicleEffect::Update(const Vector3& cameraPosition, const Matrix& pWorldMatrix, const Matrix& pWorldViewProjectionMatrix)
{
	Effect::Update(cameraPosition, pWorldMatrix, pWorldViewProjectionMatrix);

	m_pVecCameraVariable->SetFloatVector(reinterpret_cast<const float*>(&cameraPosition));
	m_pMatWorldVariable->SetMatrix(reinterpret_cast<const float*>(&pWorldMatrix));

}
