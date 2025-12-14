#include "pch.h"
#include "Anim.h"

#include "GameInstance.h"

CAnim::CAnim(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CActor(pDevice, pContext)
{
}

CAnim::CAnim(const CAnim& rhs)
	: CActor(rhs)
{
}

void CAnim::Set_AnimIndex(_int iIndex)
{
	if (nullptr != m_pModelCom)
		m_pModelCom->Set_CurrentAnimIndex(iIndex);
}

void CAnim::PlayAnimation(void)
{
	if (nullptr != m_pModelCom)
		m_pModelCom->Play_Animation(0.f);
}

HRESULT CAnim::Initialize_Prototype(void)
{
	return S_OK;
}

HRESULT CAnim::Initialize(void* pArg)
{
	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	if (nullptr != pArg)
		memcpy(&m_ModelDesc, pArg, sizeof(MODELDESC));

	if (FAILED(Ready_Components(pArg)))
		return E_FAIL;

	_float scaleFactor;

	switch (m_ModelDesc.eModel)
	{
	case MODEL_VAYNE:
		scaleFactor = 1.f;
		break;
	default:
		scaleFactor = 0.001F;
	}

	// 스케일 적용
	m_pTransformCom->Set_Scale(CTransform::STATE_RIGHT, m_ModelDesc.vScale.x * scaleFactor);
	m_pTransformCom->Set_Scale(CTransform::STATE_UP, m_ModelDesc.vScale.y * scaleFactor);
	m_pTransformCom->Set_Scale(CTransform::STATE_LOOK, m_ModelDesc.vScale.z * scaleFactor);

	m_pTransformCom->Set_Rotation(m_ModelDesc.vRotation);
	m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, XMVectorSetW(XMLoadFloat3(&m_ModelDesc.vPosition), 1.f));

	m_pModelCom->Set_CurrentAnimIndex(0);
	m_eShaderID = 0;
	m_pModelCom->Play_Animation(0.1f);

	return S_OK;
}

int CAnim::Update(_float fTimeDelta)
{
	if (__super::Update(fTimeDelta))
		return OBJ_NOEVENT;

	if (m_isPlayAnim)
	{
		m_bIsFinished = m_pModelCom->Play_Animation(fTimeDelta, true);
	}

	return OBJ_NOEVENT;
}

void CAnim::Late_Update(_float fTimeDelta)
{
	if (nullptr != m_pRenderCom)
	{
		m_pRenderCom->Add_RenderGroup(CRenderer::RENDER_NOALPHABLEND, this);
	}
}

HRESULT CAnim::Render(void)
{
	if (nullptr == m_pShaderCom)
		return E_FAIL;

	if (nullptr == m_pModelCom)
		return E_FAIL;

	if (FAILED(SetUp_ShaderID()))
		return E_FAIL;

	if (FAILED(SetUp_ShaderResources()))
		return E_FAIL;

	_uint iNumMeshes = m_pModelCom->Get_NumMeshs();

	for (_uint i = 0; i < iNumMeshes; ++i)
	{
		if (FAILED(m_pModelCom->SetUp_Material(m_pShaderCom, "g_DiffuseTexture", i, aiTextureType_DIFFUSE)))
			return E_FAIL;

		if (FAILED(m_pModelCom->SetUp_Material(m_pShaderCom, "g_NormalTexture", i, aiTextureType_NORMALS)))
			return E_FAIL;

		if (FAILED(m_pModelCom->Render(m_pShaderCom, i, m_eShaderID)))
			return E_FAIL;
	}

	return S_OK;
}

HRESULT CAnim::Ready_Components(void* pArg)
{
	// Com_Renderer
	if (FAILED(__super::Add_Components(TEXT("Com_Renderer"), LEVEL_STATIC, TEXT("Prototype_Component_Renderer"), (CComponent**)&m_pRenderCom)))
		return E_FAIL;

	// Com Transform
	CTransform::TRANSFORMDESC TransformDesc;
	ZeroMemory(&TransformDesc, sizeof(CTransform::TRANSFORMDESC));

	TransformDesc.fSpeedPerSec = 2.f;
	TransformDesc.fRotationPerSec = XMConvertToRadians(90.f);

	if (FAILED(__super::Add_Components(TEXT("Com_Transform"), LEVEL_STATIC, TEXT("Prototype_Component_Transform"), (CComponent**)&m_pTransformCom, &TransformDesc)))
		return E_FAIL;

	if (FAILED(__super::Add_Components(TEXT("Com_Shader"), LEVEL_GAMEPLAY, TEXT("Prototype_Component_Shader_VtxAnimModel"), (CComponent**)&m_pShaderCom)))
		return E_FAIL;

	switch (m_ModelDesc.eModel)
	{
	case MODEL_EZREAL:
		if (FAILED(__super::Add_Components(TEXT("Com_Model"), LEVEL_GAMEPLAY, TEXT("Ezreal"), (CComponent**)&m_pModelCom)))
			return E_FAIL;
		break;

	case MODEL_BRAUM:
		if (FAILED(__super::Add_Components(TEXT("Com_Model"), LEVEL_GAMEPLAY, TEXT("Braum"), (CComponent**)&m_pModelCom)))
			return E_FAIL;
		break;

	case MODEL_GALIO:
		if (FAILED(__super::Add_Components(TEXT("Com_Model"), LEVEL_GAMEPLAY, TEXT("Galio"), (CComponent**)&m_pModelCom)))
			return E_FAIL;
		break;

	case MODEL_LEESIN:
		if (FAILED(__super::Add_Components(TEXT("Com_Model"), LEVEL_GAMEPLAY, TEXT("LeeSin"), (CComponent**)&m_pModelCom)))
			return E_FAIL;
		break;

	case MODEL_RENEKTON:
		if (FAILED(__super::Add_Components(TEXT("Com_Model"), LEVEL_GAMEPLAY, TEXT("Renekton"), (CComponent**)&m_pModelCom)))
			return E_FAIL;
		break;

	case MODEL_YASUO:
		if (FAILED(__super::Add_Components(TEXT("Com_Model"), LEVEL_GAMEPLAY, TEXT("Yasuo"), (CComponent**)&m_pModelCom)))
			return E_FAIL;
		break;

	case MODEL_GAREN:
		if (FAILED(__super::Add_Components(TEXT("Com_Model"), LEVEL_GAMEPLAY, TEXT("Garen"), (CComponent**)&m_pModelCom)))
			return E_FAIL;
		break;

	case MODEL_GWEN:
		if (FAILED(__super::Add_Components(TEXT("Com_Model"), LEVEL_GAMEPLAY, TEXT("Gwen"), (CComponent**)&m_pModelCom)))
			return E_FAIL;
		break;

	case MODEL_LEONA:
		if (FAILED(__super::Add_Components(TEXT("Com_Model"), LEVEL_GAMEPLAY, TEXT("Leona"), (CComponent**)&m_pModelCom)))
			return E_FAIL;
		break;

	case MODEL_VI:
		if (FAILED(__super::Add_Components(TEXT("Com_Model"), LEVEL_GAMEPLAY, TEXT("Vi"), (CComponent**)&m_pModelCom)))
			return E_FAIL;
		break;
	case MODEL_VAYNE:
		if (FAILED(__super::Add_Components(TEXT("Com_Model"), LEVEL_GAMEPLAY, TEXT("Vayne"), (CComponent**)&m_pModelCom)))
			return E_FAIL;
		break;
	case MODEL_ALISTAR:
		if (FAILED(__super::Add_Components(TEXT("Com_Model"), LEVEL_GAMEPLAY, TEXT("Alistar"), (CComponent**)&m_pModelCom)))
			return E_FAIL;
		break;
	case MODEL_MINION_CANNON_B:
		if (FAILED(__super::Add_Components(TEXT("Com_Model"), LEVEL_GAMEPLAY, TEXT("Minion_Cannon_Blue"), (CComponent**)&m_pModelCom)))
			return E_FAIL;
		break;

	case MODEL_MINION_CANNON_R:
		if (FAILED(__super::Add_Components(TEXT("Com_Model"), LEVEL_GAMEPLAY, TEXT("Minion_Cannon_Red"), (CComponent**)&m_pModelCom)))
			return E_FAIL;
		break;

	case MODEL_MINION_MELEE_B:
		if (FAILED(__super::Add_Components(TEXT("Com_Model"), LEVEL_GAMEPLAY, TEXT("Minion_Melee_Blue"), (CComponent**)&m_pModelCom)))
			return E_FAIL;
		break;

	case MODEL_MINION_MELEE_R:
		if (FAILED(__super::Add_Components(TEXT("Com_Model"), LEVEL_GAMEPLAY, TEXT("Minion_Melee_Red"), (CComponent**)&m_pModelCom)))
			return E_FAIL;
		break;

	case MODEL_MINION_RANGE_B:
		if (FAILED(__super::Add_Components(TEXT("Com_Model"), LEVEL_GAMEPLAY, TEXT("Minion_Range_Blue"), (CComponent**)&m_pModelCom)))
			return E_FAIL;
		break;

	case MODEL_MINION_RANGE_R:
		if (FAILED(__super::Add_Components(TEXT("Com_Model"), LEVEL_GAMEPLAY, TEXT("Minion_Range_Red"), (CComponent**)&m_pModelCom)))
			return E_FAIL;
		break;

	case MODEL_MINION_SUPER_B:
		if (FAILED(__super::Add_Components(TEXT("Com_Model"), LEVEL_GAMEPLAY, TEXT("Minion_Super_Blue"), (CComponent**)&m_pModelCom)))
			return E_FAIL;
		break;

	case MODEL_MINION_SUPER_R:
		if (FAILED(__super::Add_Components(TEXT("Com_Model"), LEVEL_GAMEPLAY, TEXT("Minion_Super_Red"), (CComponent**)&m_pModelCom)))
			return E_FAIL;
		break;

	case MODEL_NEXUS_B:
		if (FAILED(__super::Add_Components(TEXT("Com_Model"), LEVEL_GAMEPLAY, TEXT("Nexus_Blue"), (CComponent**)&m_pModelCom)))
			return E_FAIL;
		break;

	case MODEL_NEXUS_R:
		if (FAILED(__super::Add_Components(TEXT("Com_Model"), LEVEL_GAMEPLAY, TEXT("Nexus_Red"), (CComponent**)&m_pModelCom)))
			return E_FAIL;
		break;
	case TURRET_BLUE:
		if (FAILED(__super::Add_Components(TEXT("Com_Model"), LEVEL_GAMEPLAY, TEXT("Turret_Blue"), (CComponent**)&m_pModelCom)))
			return E_FAIL;
		break;
	case TURRET_RED:
		if (FAILED(__super::Add_Components(TEXT("Com_Model"), LEVEL_GAMEPLAY, TEXT("Turret_Red"), (CComponent**)&m_pModelCom)))
			return E_FAIL;
		break;
	case TURRET_SHIELD:
		if (FAILED(__super::Add_Components(TEXT("Com_Model"), LEVEL_GAMEPLAY, TEXT("Turret_Shield"), (CComponent**)&m_pModelCom)))
			return E_FAIL;
		break;
	case NPC_STORE:
		if (FAILED(__super::Add_Components(TEXT("Com_Model"), LEVEL_GAMEPLAY, TEXT("Npc_Store"), (CComponent**)&m_pModelCom)))
			return E_FAIL;
		break;
	case NPC_STOREKEPPER:
		if (FAILED(__super::Add_Components(TEXT("Com_Model"), LEVEL_GAMEPLAY, TEXT("Npc_StoreKeeper"), (CComponent**)&m_pModelCom)))
			return E_FAIL;
		break;
	}

	return S_OK;
}

HRESULT CAnim::SetUp_ShaderResources(void)
{
	if (nullptr == m_pShaderCom)
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

	return S_OK;
}

HRESULT CAnim::SetUp_ShaderID(void)
{
	if (m_bDead)
		m_eShaderID = SHADER_PICKED;
	else
		m_eShaderID = SHADER_DEFAULT;

	return S_OK;
}

CAnim* CAnim::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CAnim* pInstance = new CAnim(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CAnim");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CGameObject* CAnim::Clone(void* pArg)
{
	CAnim* pInstance = new CAnim(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CAnim");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CAnim::Free(void)
{
	__super::Free();

	Safe_Release(m_pTransformCom);
	Safe_Release(m_pShaderCom);
	Safe_Release(m_pModelCom);
	Safe_Release(m_pRenderCom);
}