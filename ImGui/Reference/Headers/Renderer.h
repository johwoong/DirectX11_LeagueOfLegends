#pragma once
#include "Component.h"

NS_BEGIN(Engine)
class ENGINE_DLL CRenderer final : public CComponent
{
public:
	enum RENDERGROUP
	{
		RENDER_PRIORITY,
		RENDER_NOALPHABLEND,
		RENDER_GLOW,
		RENDER_ALPHABLEND,
		RENDER_DISTORTION,
		RENDER_EDGE_DETECTION,
		RENDER_UI_FRONT,
		RENDER_UI_BACK,
		RENDER_END
	};

private:
	CRenderer(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CRenderer() = default;

public:
	virtual HRESULT Initialize_Prototype() override;
	virtual HRESULT Initialize(void* pArg) override;

public:
	void Set_Distort(_bool bDistort) { m_bDistort = bDistort; }
	void Set_Saturation(_bool bSaturation, _float fSaturationPower = 1.f);

public:
	HRESULT Add_RenderGroup(RENDERGROUP eRenderGroup, class CGameObject* pGameObject);
	HRESULT Render_GameObjects();

#ifdef _DEBUG
public:
	HRESULT Add_Debug(class CComponent* pDebugCom);
	void Debug_Clear();
#endif // DEBUG

private:
	list<class CGameObject*> m_GameObjects[RENDER_END];
	typedef list<class CGameObject*> GAMEOBJECTS;

#ifdef _DEBUG
private:
	list<class CComponent*> m_DebugComponents;
#endif

private:
	class CTarget_Manager* m_pTarget_Manager = nullptr;
	class CLightManager* m_pLight_Manager = nullptr;

	class CVIBuffer_Rect* m_pVIBuffer = nullptr;
	class CShader* m_pShaderDeferred = nullptr;
	class CShader* m_pShaderPostProcessing = nullptr;

	_float4x4 m_worldMatrix;
	_float4x4 m_ViewMatrix;
	_float4x4 m_ProjMatrix;

private:
	HRESULT Render_Priority();
	HRESULT Render_NonAlphaBlend();
	HRESULT Render_Glow();
	HRESULT Render_Distortion();
	HRESULT Render_EdgeDetection();
	HRESULT Render_AlphaBlend();
	HRESULT Render_UI();

#ifdef _DEBUG
private:
	HRESULT Render_Debug();
	bool m_bRenderDebug = false;
	bool m_bRenderComponentDebug = false;
#endif // DEBUG

	/* Distortion Objects */
	class CTexture* m_pDistortionTexture = nullptr;
	class CTexture* m_pDistortionNoiseTexture = nullptr;
	_float m_fDistortionTimer = 0.f;

	/* Distortion (Post Processing) */
	_bool m_bDistort = false;
	class CTexture* m_pScreenDistortionTexture = nullptr;
	_float m_fScreenDistortionTimer = 0.f;

	/* Saturation */
	_bool m_bSaturation = true;
	_float m_fSaturationPower = 1.5f;

public:
	static CRenderer* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CComponent* Clone(void* pArg = nullptr) override;
	virtual void Free();
};
NS_END

