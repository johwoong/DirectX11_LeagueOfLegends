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
		RENDER_ALPHABLEND,
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
	class CShader* Get_ShaderPostProcessing() { return m_pShaderPostProcessing; }

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
	class CShader* m_pShaderPostProcessing = { nullptr };

	_float4x4 m_worldMatrix;
	_float4x4 m_ViewMatrix;
	_float4x4 m_ProjMatrix;

private:
	HRESULT Render_Priority();
	HRESULT Render_NonAlphaBlend();
	HRESULT Render_AlphaBlend();
	HRESULT Render_UI();

#ifdef _DEBUG
private:
	HRESULT Render_Debug();
	bool m_bRenderDebug = false;
	bool m_bRenderComponentDebug = false;
#endif // DEBUG

public:
	static CRenderer* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CComponent* Clone(void* pArg = nullptr) override;
	virtual void Free();
};
NS_END

