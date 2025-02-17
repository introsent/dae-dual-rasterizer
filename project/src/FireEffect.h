#pragma once
#include "pch.h"
#include "Effect.h"
#include "Texture.h"
class FireEffect final : public Effect
{
public:
    FireEffect(ID3D11Device* pDevice, const std::wstring& assetFile);
    virtual ~FireEffect();

    FireEffect(const Effect& other) = delete;
    FireEffect& operator=(const Effect& rhs) = delete;
    FireEffect(Effect&& other) = delete;
    FireEffect& operator=(Effect&& rhs) = delete;

    void SetPointSampling();
    void SetLinearSampling();
    void SetAnisotropicSampling();

    Texture* GetDiffuseTexture() override;

protected:
    ID3D11SamplerState* m_pSamplerPoint{};
    ID3D11SamplerState* m_pSamplerLinear{};
    ID3D11SamplerState* m_pSamplerAnisotropic{};
    ID3DX11EffectSamplerVariable* m_EffectSamplerVariable{};

    std::unique_ptr<Texture> m_pUDiffuseTexture;
};
