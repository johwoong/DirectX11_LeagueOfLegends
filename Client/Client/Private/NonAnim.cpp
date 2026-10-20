#include "NonAnim.h"
#include "GameInstance.h"
#include "PickingManager.h"

CNonAnim::CNonAnim(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CActor(pDevice, pContext)
{
}

CNonAnim::CNonAnim(const CNonAnim& rhs)
	: CActor(rhs)
{
}

HRESULT CNonAnim::Initialize_Prototype()
{
	return S_OK;
}


HRESULT CNonAnim::Initialize(void* pArg)
{
	if (pArg != nullptr)
		memcpy(&m_ModelDesc, pArg, sizeof(NONANIMDESC));

	if (FAILED(Ready_Components(pArg)))
		return E_FAIL;

	

	if (pArg != nullptr)
	{
		m_pTransformCom->Set_WorldMatrix(XMLoadFloat4x4(&m_ModelDesc.WorldMatrix));
		_vector vPosition = XMLoadFloat3(&m_ModelDesc.vPosition);
		vPosition = XMVectorSetW(vPosition, 1.f);
		m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, vPosition);
		m_pTransformCom->Set_Rotation(m_ModelDesc.vRotation);
		Set_Scale(m_ModelDesc.vScale);

		if (m_ModelDesc.m_fAngle != 0)
			m_pTransformCom->Rotation(XMVectorSet(0.f, 1.f, 0.f, 0.f), XMConvertToRadians(m_ModelDesc.m_fAngle));
	}
	return S_OK;
}

int CNonAnim::Update(_float fTimeDelta)
{
	if (__super::Update(fTimeDelta))
		return OBJ_NOEVENT;


	return OBJ_NOEVENT;
}

void CNonAnim::Late_Update(_float fTimeDelta)
{
	if (Is_Frustum(Check_CullingRadius()) == false)
	{
		return;
	}
	if (m_pRendererCom != nullptr)
	{
		m_pRendererCom->Add_RenderGroup(CRenderer::RENDER_NOALPHABLEND, this);
	}
	return;
}

HRESULT CNonAnim::Render()
{
	if (nullptr == m_pShaderCom || nullptr == m_pModelCom)
		return E_FAIL;

	if(FAILED(SetUp_ShaderID()))
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

HRESULT CNonAnim::Render_ShadowDepth()
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

	_uint		iNumMeshes = m_pModelCom->Get_NumMeshs();

	for (_uint i = 0; i < iNumMeshes; ++i)
	{
		if (FAILED(m_pModelCom->Render(m_pShaderCom, i, SHADER_ANIMSHADOW)))
			return E_FAIL;
	}
	return S_OK;
}

HRESULT CNonAnim::Render_EdgeDetection()
{
	if (nullptr == m_pShaderCom || nullptr == m_pModelCom)
		return E_FAIL;

	if (FAILED(SetUp_ShaderResources()))
		return E_FAIL;

	_uint iNumMeshes = m_pModelCom->Get_NumMeshs();
	for (_uint i = 0; i < iNumMeshes; ++i)
	{
		if (FAILED(m_pModelCom->SetUp_Material(m_pShaderCom, "g_DiffuseTexture", i, aiTextureType_DIFFUSE)))
			return E_FAIL;
		if (FAILED(m_pModelCom->Render(m_pShaderCom, i, 5)))
			return E_FAIL;
	}

	return S_OK;
}

_float CNonAnim::Check_CullingRadius()
{
	if (!strcmp(m_ModelDesc.pModeltag, "Map"))
		return 400.f;
	return 0.f;
}

_bool CNonAnim::Picking(_float3* PickingPoint)
{
	if (m_pModelCom->Picking(m_pTransformCom, PickingPoint))
		return true;

	return false;
}

void CNonAnim::PickingTrue()
{
	if (m_pGameInstance->Mouse_Up(DIMK::DIMK_LBUTTON))
	{
		if (m_bPicked)
		{
			int a = 0;
		}
	}
}


HRESULT CNonAnim::Ready_Components(void* pArg)
{
	// Renderer
	if (FAILED(__super::Add_Components(TEXT("Com_Renderer"), LEVEL_STATIC,
		TEXT("Prototype_Component_Renderer"), (CComponent**)&m_pRendererCom)))
		return E_FAIL;

	// Transform 
	CTransform::TRANSFORMDESC TransformDesc;
	TransformDesc.fRotationPerSec = 1.f;
	TransformDesc.fSpeedPerSec = 1.f;
	if (FAILED(__super::Add_Components(TEXT("Com_Transform"), LEVEL_STATIC,
		TEXT("Prototype_Component_Transform"), (CComponent**)&m_pTransformCom)))
		return E_FAIL;

	// Shader
	if (FAILED(__super::Add_Components(TEXT("Com_Shader"), LEVEL_STATIC,
		TEXT("Prototype_Component_Shader_VtxModel"), (CComponent**)&m_pShaderCom)))
		return E_FAIL;

	// Model
	_tchar			szModeltag[MAX_PATH] = TEXT("");
	MultiByteToWideChar(CP_ACP, 0, m_ModelDesc.pModeltag, strlen(m_ModelDesc.pModeltag), szModeltag, MAX_PATH);
	if (FAILED(__super::Add_Components(TEXT("Com_Model"), LEVEL_GAMEPLAY, szModeltag, (CComponent**)&m_pModelCom)))
		return E_FAIL;

	return S_OK;

}

HRESULT CNonAnim::SetUp_ShaderResources()
{
	if (!m_pShaderCom || !m_pTransformCom)
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

HRESULT CNonAnim::SetUp_ShaderID()
{
	return S_OK;
}

CNonAnim* CNonAnim::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CNonAnim* pInstance = new CNonAnim(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CNonAnim");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CGameObject* CNonAnim::Clone(void* pArg)
{
	CNonAnim* pInstance = new CNonAnim(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CNonAnim");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CNonAnim::Free()
{
	__super::Free();

}