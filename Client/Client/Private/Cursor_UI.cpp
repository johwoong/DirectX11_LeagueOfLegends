#include "Cursor_UI.h"
#include "GameInstance.h"
#include "CursorManager.h"


CCursor_UI::CCursor_UI(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CUI_Base(pDevice, pContext)
{
}

CCursor_UI::CCursor_UI(const CCursor_UI& rhs)
	: CUI_Base(rhs)
{
}

HRESULT CCursor_UI::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CCursor_UI::Initialize(void* pArg)
{
	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;
	// 커서 크기 (픽셀) ? 게임 해상도에 맞게 조절
	m_fSize.x = 32.f;
	m_fSize.y = 32.f;


	return S_OK;
}

int CCursor_UI::Update(_float fTimeDelta)
{
	if (__super::Update(fTimeDelta) < 0)
		return OBJ_DEAD;

	// 마우스 위치 가져오기
	CCursorManager* pMgr = CCursorManager::GetInstance();
	POINT pt = pMgr->GetCursorPosClient();

	m_fPosition.x = (float)pt.x + 10.f;
	m_fPosition.y = (float)pt.y + 15.f;

	// 크기
	m_fSize.x = 50.f;
	m_fSize.y = 50.f;
	
	m_pTransformCom->Set_Rotation(_float3(0.f, 0.f, 0.f));
	return OBJ_NOEVENT;
}

void CCursor_UI::Late_Update(_float fTimeDelta)
{
	if (nullptr != m_pRendererCom)
		m_pRendererCom->Add_RenderGroup(CRenderer::RENDER_UI_FRONT, this);
}

HRESULT CCursor_UI::Render()
{
	if (nullptr == m_pShaderCom || nullptr == m_pVIBufferCom)
		return E_FAIL;

	if (FAILED(SetUp_ShaderID()))
		return E_FAIL;

	if (FAILED(SetUp_ShaderResources()))
		return E_FAIL;

	// 커서 상태 확인
	CURSOR_TYPE eState = CCursorManager::GetInstance()->GetState();

	CTexture* pTex = (eState == CURSOR_ATTACK) ? m_pTexAttack : m_pTexNormal;

	if (FAILED(m_pShaderCom->Set_ShaderResourceView("g_DiffuseTexture", pTex->Get_SRV(0))))
		return E_FAIL;

	m_pShaderCom->Begin(1);
	m_pVIBufferCom->Render();

	return S_OK;
}

HRESULT CCursor_UI::Ready_Components(void* pArg)
{
	// Renderer
	if (FAILED(__super::Add_Components(TEXT("Com_Renderer"), LEVEL_STATIC,
		TEXT("Prototype_Component_Renderer"), (CComponent**)&m_pRendererCom)))
		return E_FAIL;

	// Transform
	if (FAILED(__super::Add_Components(TEXT("Com_Transform"), LEVEL_STATIC,
		TEXT("Prototype_Component_Transform"), (CComponent**)&m_pTransformCom)))
		return E_FAIL;

	// Shader
	if (FAILED(__super::Add_Components(TEXT("Com_Shader"), LEVEL_STATIC,
		TEXT("Prototype_Component_Shader_UI"), (CComponent**)&m_pShaderCom)))
		return E_FAIL;


	if (FAILED(__super::Add_Components(TEXT("Com_TexNormal"), LEVEL_STATIC,
		TEXT("Prototype_Component_Texture_Cursor_Normal"), (CComponent**)&m_pTexNormal)))
		return E_FAIL;

	if (FAILED(__super::Add_Components(TEXT("Com_TexAttack"), LEVEL_STATIC,
		TEXT("Prototype_Component_Texture_Cursor_Attack"), (CComponent**)&m_pTexAttack)))
		return E_FAIL;

	// VIBuffer
	if (FAILED(__super::Add_Components(TEXT("Com_VIBuffer"), LEVEL_STATIC,
		TEXT("Prototype_Component_VIBuffer_Rect"), (CComponent**)&m_pVIBufferCom)))
		return E_FAIL;

	return S_OK;
}

HRESULT CCursor_UI::SetUp_ShaderResources()
{
	if (nullptr == m_pShaderCom)
		return E_FAIL;

	_float4x4 WorldMatrix = m_pTransformCom->Get_World_TP();

	if (FAILED(m_pShaderCom->Set_RawValue("g_WorldMatrix", &WorldMatrix, sizeof(_float4x4))))
		return E_FAIL;
	if (FAILED(m_pShaderCom->Set_RawValue("g_ViewMatrix", &m_ViewMatrix, sizeof(_float4x4))))
		return E_FAIL;
	if (FAILED(m_pShaderCom->Set_RawValue("g_ProjMatrix", &m_ProjMatrix, sizeof(_float4x4))))
		return E_FAIL;

	return S_OK;
}



CCursor_UI* CCursor_UI::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CCursor_UI* pInstance = new CCursor_UI(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Cloned : CUI_SkillImage");
		Safe_Release(pInstance);
	}

	return pInstance;
}


CGameObject* CCursor_UI::Clone(void* pArg)
{
	CCursor_UI* pInstance = new CCursor_UI(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CUI_Skillbutton");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CCursor_UI::Free()
{
	__super::Free();
}