#include "Particle_Rect.h"
#include "GameInstance.h"
CParticle_Rect::CParticle_Rect(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CGameObject(pDevice, pContext)
{
}

CParticle_Rect::CParticle_Rect(const CParticle_Rect& rhs)
	: CGameObject(rhs)
{
}

HRESULT CParticle_Rect::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CParticle_Rect::Initialize(void* pArg)
{
	if (FAILED(Ready_Components()))
		return E_FAIL;

	return S_OK;
}

int CParticle_Rect::Update(_float fTimeDelta)
{
	m_pVIBufferCom->Update(fTimeDelta);
	return OBJ_NOEVENT;
}

void CParticle_Rect::Late_Update(_float fTimeDelta)
{
	if (m_pRendererCom)
		m_pRendererCom->Add_RenderGroup(CRenderer::RENDER_NOLIGHT, this);
}

HRESULT CParticle_Rect::Render()
{
	if (!m_pShaderCom || !m_pVIBufferCom)
		return E_FAIL;

	if (FAILED(SetUp_ShaderResources()))
		return E_FAIL;

	m_pShaderCom->Begin();
	m_pVIBufferCom->Render();

	return S_OK;
}

HRESULT CParticle_Rect::Ready_Components()
{
	/* For.Com_Renderer */
	if (FAILED(__super::Add_Components(TEXT("Com_Renderer"), LEVEL_STATIC, TEXT("Prototype_Component_Renderer"), (CComponent**)&m_pRendererCom)))
		return E_FAIL;
	/* For.Com_Transform */
	if (FAILED(__super::Add_Components(TEXT("Com_Transform"), LEVEL_STATIC, TEXT("Prototype_Component_Transform"), (CComponent**)&m_pTransformCom)))
		return E_FAIL;
	/* For.Com_Shader */
	if (FAILED(__super::Add_Components(TEXT("Com_Shader"), LEVEL_STATIC, TEXT("Prototype_Component_Shader_VtxRectInstance"), (CComponent**)&m_pShaderCom)))
		return E_FAIL;

	/* For.Com_VIBuffer */
	if (FAILED(__super::Add_Components(TEXT("Com_VIBuffer"), LEVEL_STATIC, TEXT("Prototype_Component_VIBuffer_RectInstance"), (CComponent**)&m_pVIBufferCom)))
		return E_FAIL;

	return S_OK;
}

HRESULT CParticle_Rect::SetUp_ShaderResources()
{
	if (!m_pShaderCom)
		return E_FAIL;

	_float4x4 WorldMatrix = m_pTransformCom->Get_World_TP();
	_float4x4 ViewMatrix = m_pGameInstance->Get_TransformFloat4x4_TP(CPipeLine::D3DTS_VIEW);
	_float4x4 ProjMatrix = m_pGameInstance->Get_TransformFloat4x4_TP(CPipeLine::D3DTS_PROJ);


	if (FAILED(m_pShaderCom->Set_RawValue("g_WorldMatrix", &WorldMatrix, sizeof(_float4x4))))
		return E_FAIL;

	if (FAILED(m_pShaderCom->Set_RawValue("g_ViewMatrix", &ViewMatrix, sizeof(_float4x4))))
		return E_FAIL;

	if (FAILED(m_pShaderCom->Set_RawValue("g_ProjMatrix", &ProjMatrix, sizeof(_float4x4))))
		return E_FAIL;


	if (FAILED(m_pShaderCom->Set_ShaderResourceView("g_DiffuseTexture", m_pTextureCom->Get_SRV(0))))
		return E_FAIL;

	return S_OK;
}

CParticle_Rect* CParticle_Rect::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CParticle_Rect* pInstance = new CParticle_Rect(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CParticle_Rect");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CGameObject* CParticle_Rect::Clone(void* pArg)
{
	CParticle_Rect* pInstance = new CParticle_Rect(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CParticle_Rect");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CParticle_Rect::Free()
{
	__super::Free();

}