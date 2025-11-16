#include "SpawnBox.h"
#include "GameInstance.h"
#include "ModelManager.h"
#include "PickingMgr.h"

CSpawnBox::CSpawnBox(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CActor(pDevice, pContext)
{
}

HRESULT CSpawnBox::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CSpawnBox::Initialize(void* pArg)
{
	if (pArg != nullptr)
		memcpy(&m_eTreasureBoxDesc, pArg, sizeof(BOXTAG));

	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	if (FAILED(Ready_Components(pArg)))
		return E_FAIL;

	m_eObjectID = OBJID::OBJ_BLOCK;
	Set_Scale(_float3(1.2f, 1.2f, 1.2f));

	if (pArg != nullptr)
	{
		_vector vPosition = XMLoadFloat3(&m_eTreasureBoxDesc.vPosition);
		vPosition = XMVectorSetW(vPosition, 1.f);
		m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, vPosition);
		Set_Scale(m_eTreasureBoxDesc.vScale);

		if (m_eTreasureBoxDesc.m_fAngle != 0)
			m_pTransformCom->Rotation(XMLoadFloat3(&m_eTreasureBoxDesc.vRotation), XMConvertToRadians(m_eTreasureBoxDesc.m_fAngle));
	}

	CModelManager::GetInstance()->Add_TreasureBoxGroup(this);
	return S_OK;
}

int CSpawnBox::Update(_float fTimeDelta)
{
	if (m_bDead)
		return OBJ_DEAD;

	return OBJ_NOEVENT;
}

void CSpawnBox::Late_Update(_float fTimeDelta)
{
	if (nullptr != m_pRenderCom)
		m_pRenderCom->Add_RenderGroup(CRenderer::RENDER_NOALPHABLEND, this);
	SetUp_ShaderID();
}

HRESULT CSpawnBox::Render()
{
	if (nullptr == m_pShaderCom ||
		nullptr == m_pModelCom)
		return E_FAIL;

	if (FAILED(SetUp_ShaderResources()))
		return E_FAIL;


	_uint		iNumMeshes = m_pModelCom->Get_NumMeshs();

	for (_uint i = 0; i < iNumMeshes; ++i)
	{
		if (FAILED(m_pModelCom->SetUp_Material(m_pShaderCom, "g_DiffuseTexture", i, aiTextureType_DIFFUSE)))
			return E_FAIL;

		if (FAILED(m_pModelCom->Render(m_pShaderCom, i, m_eShaderID)))
			return E_FAIL;
	}

	return S_OK;
}

void CSpawnBox::Set_Picked()
{

	if (m_bPicked)
	{
		CModelManager::GetInstance()->Set_AllPickedFalse();
		CPickingMgr::GetInstance()->Set_PickedObj(nullptr);
	}
	else
	{
		CModelManager::GetInstance()->Set_AllPickedFalse();
		CPickingMgr::GetInstance()->Set_PickedObj(this);
	}
}



HRESULT CSpawnBox::Ready_Components(void* pArg)
{
	/* For.Com_Renderer */
	if (FAILED(__super::Add_Components(TEXT("Com_Renderer"), LEVEL_STATIC, TEXT("Prototype_Component_Renderer"), (CComponent**)&m_pRenderCom)))
		return E_FAIL;

	/* For.Com_Transform */
	CTransform::TRANSFORMDESC		TransformDesc;
	ZeroMemory(&TransformDesc, sizeof(CTransform::TRANSFORMDESC));

	TransformDesc.fSpeedPerSec = 2.0f;
	TransformDesc.fRotationPerSec = XMConvertToRadians(1.0f);
	if (FAILED(__super::Add_Components(TEXT("Com_Transform"), LEVEL_STATIC, TEXT("Prototype_Component_Transform"), (CComponent**)&m_pTransformCom, &TransformDesc)))
		return E_FAIL;

	/* For.Com_Shader */
	if (FAILED(__super::Add_Components(TEXT("Com_Shader"), LEVEL_GAMEPLAY, TEXT("Prototype_Component_Shader_VtxModel"), (CComponent**)&m_pShaderCom)))
		return E_FAIL;

	/* For.Com_Model*/
	if (FAILED(__super::Add_Components(TEXT("Com_Model"), LEVEL_GAMEPLAY, TEXT("TreasureBox02"), (CComponent**)&m_pModelCom)))
		return E_FAIL;

	return S_OK;

}

HRESULT CSpawnBox::SetUp_ShaderResources()
{
	if (!m_pShaderCom) return E_FAIL;

	const _float4x4& worldTP = m_pTransformCom->Get_World_TP();
	if (FAILED(m_pShaderCom->Set_RawValue("g_WorldMatrix", &worldTP, sizeof(_float4x4))))
		return E_FAIL;

	const _float4x4& viewTP = m_pGameInstance->Get_TransformFloat4x4_TP(CPipeLine::D3DTS_VIEW);
	const _float4x4& projTP = m_pGameInstance->Get_TransformFloat4x4_TP(CPipeLine::D3DTS_PROJ);
	if (FAILED(m_pShaderCom->Set_RawValue("g_ViewMatrix", &viewTP, sizeof(_float4x4)))) return E_FAIL;
	if (FAILED(m_pShaderCom->Set_RawValue("g_ProjMatrix", &projTP, sizeof(_float4x4)))) return E_FAIL;
	return S_OK;
}



CSpawnBox* CSpawnBox::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CSpawnBox* pInstance = new CSpawnBox(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CTreasureBox");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CGameObject* CSpawnBox::Clone(void* pArg)
{
	CSpawnBox* pInstance = new CSpawnBox(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CTreasureBox");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CSpawnBox::Free()
{
	__super::Free();
	Safe_Release(m_pModelCom);
}