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

	m_pTransformCom->Set_Scale(CTransform::STATE_RIGHT, m_ModelDesc.vScale.x);
	m_pTransformCom->Set_Scale(CTransform::STATE_UP, m_ModelDesc.vScale.y);
	m_pTransformCom->Set_Scale(CTransform::STATE_LOOK, m_ModelDesc.vScale.z);

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
	case MODEL_NEXUS:
		if (FAILED(__super::Add_Components(TEXT("Com_Model"), LEVEL_GAMEPLAY, TEXT("Nexus"), (CComponent**)&m_pModelCom)))
			return E_FAIL;
		break;
	case MODEL_EZREAL:
		if (FAILED(__super::Add_Components(TEXT("Com_Model"), LEVEL_GAMEPLAY, TEXT("Ezreal"), (CComponent**)&m_pModelCom)))
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
