#include "NonAnim.h"
#include "Imgui_Manager.h"
#include "Model.h"
#include "ModelManager.h"
#include "GameInstance.h"
#include "Picking.h"
#include "Camera_Manager.h"
#include "PickingMgr.h"

CNonAnim::CNonAnim(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CActor(pDevice, pContext)
{
}

CNonAnim::CNonAnim(const CNonAnim& rhs)
	: CActor(rhs)
{
	memcpy(&m_ModelDesc, &rhs.m_ModelDesc, sizeof(NONANIMDESC));
}

HRESULT CNonAnim::Initialize_Prototype()
{
	XMStoreFloat4x4(&m_ModelDesc.WorldMatrix, XMMatrixIdentity());
	return S_OK;
}

HRESULT CNonAnim::Initialize(void* pArg)
{
	if (pArg != nullptr)
		memcpy(&m_ModelDesc, pArg, sizeof(NONANIMDESC));

	if (FAILED(Ready_Components(pArg)))
		return E_FAIL;

	if (strcmp(m_ModelDesc.pModeltag, "Picking_Symbol"))
		CPickingMgr::GetInstance()->Add_PickingGroup(this);

	m_eObjectID = OBJ_BLOCK;

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
	
	CModelManager::GetInstance()->Add_CreatedModel(this);

	return S_OK;
}

int CNonAnim::Update(_float fTimeDelta)
{
	if (__super::Update(fTimeDelta))
		return OBJ_DEAD;

	return OBJ_NOEVENT;
}

void CNonAnim::Late_Update(_float fTimeDelta)
{
	if (nullptr != m_pRenderCom)
		m_pRenderCom->Add_RenderGroup(CRenderer::RENDER_NOALPHABLEND, this);

	XMStoreFloat3(&m_ModelDesc.vPosition, Get_Position());
	m_ModelDesc.vScale = Get_Scale();
	m_ModelDesc.WorldMatrix = m_pTransformCom->Get_World();
}

HRESULT CNonAnim::Render()
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
		if (m_iSymbolType != -1)
		{
			if (FAILED(m_pModelCom->SetUp_Material(m_pShaderCom, "g_DiffuseTexture", i, aiTextureType_DIFFUSE, m_iSymbolType)))
				return E_FAIL;

			if (m_iSymbolType == 0)
			{
				m_eShaderID = SHADER_SYMBOL;

				_float4 color;
				XMStoreFloat4(&color, XMVectorSet(1.f, 1.f, 0.f, 1.f));  
				if (FAILED(m_pShaderCom->Set_RawValue("g_vColor", &color, sizeof(_float4))))
					return E_FAIL;
			}
			else if (m_iSymbolType == 1)
			{
				m_eShaderID = SHADER_SYMBOL;

				_float4 color;
				XMStoreFloat4(&color, XMVectorSet(1.f, 0.f, 1.f, 1.f));
				if (FAILED(m_pShaderCom->Set_RawValue("g_vColor", &color, sizeof(_float4))))
					return E_FAIL;
			}

		}
		else
		{
			if (FAILED(m_pModelCom->SetUp_Material(m_pShaderCom, "g_DiffuseTexture", i, aiTextureType_DIFFUSE)))
				return E_FAIL;
		}


		if (FAILED(m_pModelCom->SetUp_Material(m_pShaderCom, "g_NormalTexture", i, aiTextureType_NORMALS)))
			return E_FAIL;

		if (FAILED(m_pModelCom->Render(m_pShaderCom, i, m_eShaderID)))
			return E_FAIL;
	}

	return S_OK;
}

_bool CNonAnim::Picking(_float3* PickingPoint)
{
	CImGui_Manager::PICKING_TYPE PickingType = CImGui_Manager::GetInstance()->Get_PickingType();

	if (m_pModelCom->Picking(m_pTransformCom, PickingPoint))
		return true;

	return false;
}

void CNonAnim::PickingTrue()
{
	CImGui_Manager::PICKING_TYPE ePickingtype = CImGui_Manager::GetInstance()->Get_PickingType();

	switch (ePickingtype)
	{
	case Client::CImGui_Manager::PICKING_MODEL:
		Set_Picked();
		break;
	default:
		break;
	}
}

void CNonAnim::Set_Picked()
{
	CGameInstance* pGameInstance = CGameInstance::GetInstance();
	if (pGameInstance->Mouse_Up(DIMK::DIMK_LBUTTON))
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
}

void CNonAnim::Turn(_float3 vAxis, _float fAngle)
{
	_float fInputAngle = fAngle;
	if (fInputAngle > 180)
		fInputAngle = 360 - fInputAngle;

	m_ModelDesc.vRotation = vAxis;
	m_ModelDesc.m_fAngle = fAngle;

	m_pTransformCom->Set_Rotation(m_ModelDesc.vRotation);
}

HRESULT CNonAnim::Ready_Components(void* pArg)
{
	/* For.Com_Renderer */
	if (FAILED(__super::Add_Components(TEXT("Com_Renderer"), LEVEL_STATIC, TEXT("Prototype_Component_Renderer"), (CComponent**)&m_pRenderCom)))
		return E_FAIL;

	/* For.Com_Transform */
	CTransform::TRANSFORMDESC TransformDesc;
	TransformDesc.fRotationPerSec = 1.f;
	TransformDesc.fSpeedPerSec = 1.f;

	if (FAILED(__super::Add_Components(TEXT("Com_Transform"), LEVEL_STATIC, TEXT("Prototype_Component_Transform"), (CComponent**)&m_pTransformCom, &TransformDesc)))
		return E_FAIL;

	/* For.Com_Shader */
	if (FAILED(__super::Add_Components(TEXT("Com_Shader"), LEVEL_GAMEPLAY, TEXT("Prototype_Component_Shader_VtxModel"), (CComponent**)&m_pShaderCom)))
		return E_FAIL;

	/* For.Com_Model*/
	_tchar			szModeltag[MAX_PATH] = TEXT("");
	MultiByteToWideChar(CP_ACP, 0, m_ModelDesc.pModeltag, strlen(m_ModelDesc.pModeltag), szModeltag, MAX_PATH);

	if (FAILED(__super::Add_Components(TEXT("Com_Model"), LEVEL_GAMEPLAY, szModeltag, (CComponent**)&m_pModelCom)))
		return E_FAIL;

	return S_OK;
}

HRESULT CNonAnim::SetUp_ShaderResources()
{
	if (!m_pShaderCom) return E_FAIL;

	const _float4x4& worldTP = m_pTransformCom->Get_World_TP();
	if (FAILED(m_pShaderCom->Set_RawValue("g_WorldMatrix", &worldTP, sizeof(_float4x4))))
		return E_FAIL;

	CGameInstance* pGI = CGameInstance::GetInstance();
	const _float4x4& viewTP = pGI->Get_TransformFloat4x4_TP(CPipeLine::D3DTS_VIEW);
	const _float4x4& projTP = pGI->Get_TransformFloat4x4_TP(CPipeLine::D3DTS_PROJ);
	if (FAILED(m_pShaderCom->Set_RawValue("g_ViewMatrix", &viewTP, sizeof(_float4x4)))) return E_FAIL;
	if (FAILED(m_pShaderCom->Set_RawValue("g_ProjMatrix", &projTP, sizeof(_float4x4)))) return E_FAIL;

	return S_OK;
}

HRESULT CNonAnim::SetUp_ShaderID()
{
	if (m_bPicked)
		m_eShaderID = SHADER_PICKED;
	else
		m_eShaderID = SHADER_DEFAULT;

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

	Safe_Release(m_pTransformCom);
	Safe_Release(m_pShaderCom);
	Safe_Release(m_pModelCom);
}
