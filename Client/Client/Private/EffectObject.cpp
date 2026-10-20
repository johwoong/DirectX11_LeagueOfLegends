#include "EffectObject.h"
#include "GameInstance.h"


CEffectObject::CEffectObject(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CActor(pDevice, pContext)
{
}

CEffectObject::CEffectObject(const CEffectObject& rhs)
	: CActor(rhs)
{
}

HRESULT CEffectObject::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CEffectObject::Initialize(void* pArg)
{
	if (pArg != nullptr)
		memcpy(&m_EffectDesc, pArg, sizeof(EFFECTDESC));

	if (FAILED(Ready_Components(pArg)))
		return E_FAIL;


	if (pArg != nullptr)
	{

		_vector vPosition = XMLoadFloat3(&m_EffectDesc.m_ModelDesc.vPosition);
		vPosition = XMVectorSetW(vPosition, 1.f);
		m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, vPosition);
		m_pTransformCom->Set_Rotation(m_EffectDesc.m_ModelDesc.vRotation);
		Set_Scale(m_EffectDesc.m_ModelDesc.vScale);

		if (m_EffectDesc.m_ModelDesc.m_fAngle != 0)
			m_pTransformCom->Rotation(XMVectorSet(0.f, 1.f, 0.f, 0.f), XMConvertToRadians(m_EffectDesc.m_ModelDesc.m_fAngle));

	}


	return S_OK;
}

int CEffectObject::Update(_float fTimeDelta)
{
	if (__super::Update(fTimeDelta))
		return OBJ_DEAD;

	m_fTime += fTimeDelta;
	Update_Direction(fTimeDelta);

	return OBJ_NOEVENT;
}

void CEffectObject::Late_Update(_float fTimeDelta)
{
	if (m_EffectDesc.fDeadTime < m_fTime)
	{
		m_bDead = true;
	}

	if (nullptr != m_pRendererCom)
	{
		m_pRendererCom->Add_RenderGroup(CRenderer::RENDER_NOALPHABLEND, this);

#ifdef _DEBUG
		if (m_pAABBCom != nullptr)
			m_pRendererCom->Add_Debug(m_pAABBCom);
		if (m_pOBBCom != nullptr)
			m_pRendererCom->Add_Debug(m_pOBBCom);
		if (m_pSphereCom != nullptr)
			m_pRendererCom->Add_Debug(m_pSphereCom);
#endif
	}


	Compute_CamDistance(m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION));
	return;

}

HRESULT CEffectObject::Render()
{
	if (nullptr == m_pShaderCom ||
		nullptr == m_pModelCom)
		return E_FAIL;

	if (FAILED(SetUp_ShaderID()))
		return E_FAIL;

	if (FAILED(SetUp_ShaderResources()))
		return E_FAIL;

	_uint		iNumMeshes = m_pModelCom->Get_NumMeshs();

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

HRESULT CEffectObject::Render_ShadowDepth()
{
	_float4x4 WorldMatrix = m_pTransformCom->Get_World_TP();
	_float4x4 ViewMatrix = m_pGameInstance->Get_TransformFloat4x4_TP(CPipeLine::D3DTS_VIEW);
	_float4x4 ProjMatrix = m_pGameInstance->Get_TransformFloat4x4_TP(CPipeLine::D3DTS_PROJ);


	if (FAILED(m_pShaderCom->Set_RawValue("g_WorldMatrix", &WorldMatrix, sizeof(_float4x4))))
		return E_FAIL;

	if (FAILED(m_pShaderCom->Set_RawValue("g_ViewMatrix", &ViewMatrix, sizeof(_float4x4))))
		return E_FAIL;

	if (FAILED(m_pShaderCom->Set_RawValue("g_ProjMatrix", &ProjMatrix, sizeof(_float4x4))))
		return E_FAIL;

	LEVEL iLevel = (LEVEL)m_pGameInstance->Get_CurrentLevelIndex();

	_uint		iNumMeshes = m_pModelCom->Get_NumMeshs();

	for (_uint i = 0; i < iNumMeshes; ++i)
	{
		if (FAILED(m_pModelCom->RenderShadow(m_pShaderCom, i, iLevel, SHADER_NOANIM_SHADOW)))
			return E_FAIL;

	}

	return S_OK;
}

_float CEffectObject::Is_Check_Culling()
{
	LEVEL iLevel = (LEVEL)CGameInstance::GetInstance()->Get_DestinationLevelIndex();


	return 3.f;
}

void CEffectObject::Update_Direction(_float fTimeDelta)
{
	m_pTransformCom->Move_PosDir(fTimeDelta * m_EffectDesc.fVelocity, m_EffectDesc.vTargetDir);
}


HRESULT CEffectObject::Ready_Components(void* pArg)
{

	LEVEL iLevel = (LEVEL)CGameInstance::GetInstance()->Get_DestinationLevelIndex();

	/* For.Com_Renderer */
	if (FAILED(__super::Add_Components(TEXT("Com_Renderer"), LEVEL_STATIC, TEXT("Prototype_Component_Renderer"), (CComponent**)&m_pRendererCom)))
		return E_FAIL;

	/* For.Com_Transform */
	CTransform::TRANSFORMDESC TransformDesc;
	TransformDesc.fRotationPerSec = 1.f;
	TransformDesc.fSpeedPerSec = 1.f;
	/* For.Com_Transform */
	if (FAILED(__super::Add_Components(TEXT("Com_Transform"), LEVEL_STATIC, TEXT("Prototype_Component_Transform"), (CComponent**)&m_pTransformCom, &TransformDesc)))
		return E_FAIL;

	/* For.Com_Shader */
	if (FAILED(__super::Add_Components(TEXT("Com_Shader"), LEVEL_STATIC, TEXT("Prototype_Component_Shader_VtxModel"), (CComponent**)&m_pShaderCom)))
		return E_FAIL;

	/* For.Com_Model*/
	_tchar			szModeltag[MAX_PATH] = TEXT("");
	MultiByteToWideChar(CP_ACP, 0, m_EffectDesc.m_ModelDesc.pModeltag, (int)strlen(m_EffectDesc.m_ModelDesc.pModeltag), szModeltag, MAX_PATH);
	if (FAILED(__super::Add_Components(TEXT("Com_Model"), LEVEL_STATIC, szModeltag, (CComponent**)&m_pModelCom)))
		return E_FAIL;

	return S_OK;
}

HRESULT CEffectObject::SetUp_ShaderResources()
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

HRESULT CEffectObject::SetUp_ShaderID()
{


	return S_OK;
}

CEffectObject* CEffectObject::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CEffectObject* pInstance = new CEffectObject(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CEffectObject");
		Safe_Release(pInstance);
	}

	return pInstance;
}


CGameObject* CEffectObject::Clone(void* pArg)
{
	CEffectObject* pInstance = new CEffectObject(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CEffectObject");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CEffectObject::Free()
{
	__super::Free();
}
