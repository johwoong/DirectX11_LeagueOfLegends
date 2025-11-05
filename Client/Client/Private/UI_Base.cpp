
#include "UI_Base.h"
#include "GameInstance.h"

CUI_Base::CUI_Base(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CGameObject(pDevice, pContext)
{
}

CUI_Base::CUI_Base(const CUI_Base& rhs)
	: CGameObject(rhs)
{
}

HRESULT CUI_Base::Initialize(void* pArg)
{
	if (FAILED(Ready_Components(pArg)))
		return E_FAIL;

	XMStoreFloat4x4(&m_ViewMatrix, XMMatrixTranspose(XMMatrixIdentity()));
	XMStoreFloat4x4(&m_ProjMatrix, XMMatrixTranspose(XMMatrixOrthographicLH((_float)g_WinSizeX, (_float)g_WinSizeY, 0.f, 1.f)));


	m_pTransformCom->Set_Scale(CTransform::STATE_RIGHT, m_fSize.x);
	m_pTransformCom->Set_Scale(CTransform::STATE_UP, m_fSize.y);
	m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, XMVectorSet(m_fPosition.x - g_WinSizeX * 0.5f, -m_fPosition.y + g_WinSizeY * 0.5f, 0.f, 1.f));

	return S_OK;

}



int CUI_Base::Update(_float fTimeDelta)
{
	if (m_bDead)
		return OBJ_DEAD;

	m_pTransformCom->Set_Scale(CTransform::STATE_RIGHT, m_fSize.x);
	m_pTransformCom->Set_Scale(CTransform::STATE_UP, m_fSize.y);
	m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, XMVectorSet(m_fPosition.x - g_WinSizeX * 0.5f, -m_fPosition.y + g_WinSizeY * 0.5f, 0.f, 1.f));

	return OBJ_NOEVENT;
}

void CUI_Base::Late_Update(_float fTimeDelta)
{
	if (nullptr != m_pRendererCom)
		m_pRendererCom->Add_RenderGroup(CRenderer::RENDER_UI_BACK, this);

}

void CUI_Base::SetUp_BillBoard()
{
	_float4x4 ViewMatrix;

	ViewMatrix = CGameInstance::GetInstance()->Get_TransformFloat4x4(CPipeLine::D3DTS_VIEW); 
	_matrix matViewInv = XMMatrixInverse(nullptr, XMLoadFloat4x4(&ViewMatrix));     

	_vector vRight = (_vector)matViewInv.r[0];
	_vector vUp = (_vector)matViewInv.r[1];
	_vector vLook = (_vector)matViewInv.r[2];
	m_pTransformCom->Set_State(CTransform::STATE_RIGHT, XMVector3Normalize(vRight) * Get_Scale_float3().x);
	m_pTransformCom->Set_State(CTransform::STATE_UP, XMVector3Normalize(vUp) * Get_Scale_float3().y);
	m_pTransformCom->Set_State(CTransform::STATE_LOOK, XMVector3Normalize(vLook) * Get_Scale_float3().z);
}




void CUI_Base::Set_Scale(_float3 vScale)
{
	m_vScale = vScale;

	m_pTransformCom->Set_Scale(CTransform::STATE_RIGHT, m_vScale.x);
	m_pTransformCom->Set_Scale(CTransform::STATE_UP, m_vScale.y);
	m_pTransformCom->Set_Scale(CTransform::STATE_LOOK, m_vScale.z);

}

HRESULT CUI_Base::Render()
{
	if (nullptr == m_pShaderCom ||
		nullptr == m_pVIBufferCom)
		return E_FAIL;

	if (FAILED(SetUp_ShaderID()))
		return E_FAIL;

	if (FAILED(SetUp_ShaderResources()))
		return E_FAIL;

	m_pShaderCom->Begin(m_eShaderID);

	m_pVIBufferCom->Render();

	return S_OK;
}

void CUI_Base::Free()
{
	__super::Free();

	Safe_Release(m_pTransformCom);
	Safe_Release(m_pShaderCom);

	Safe_Release(m_pTextureCom);

	Safe_Release(m_pVIBufferCom);
}