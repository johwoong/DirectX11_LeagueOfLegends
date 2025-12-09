#include "StoreNpc.h"
#include "GameInstance.h"


CStoreNpc::CStoreNpc(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CNpc(pDevice, pContext)
{
}

CStoreNpc::CStoreNpc(const CStoreNpc& rhs)
	: CNpc(rhs)
{
}

HRESULT CStoreNpc::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CStoreNpc::Initialize(void* pArg)
{
	__super::Initialize(pArg);


	m_eState = ANIM_BLUE::IDLE;
	m_pModelCom->Set_CurrentAnimIndex((_uint)ANIM_BLUE::IDLE);
	Set_Scale(_float3(0.01f, 0.01f, 0.01f));
	return S_OK;
}


_int CStoreNpc::Update(_float fTimeDelta)
{
	__super::Update(fTimeDelta);
	Update_State(fTimeDelta);

	return OBJ_NOEVENT;
}

void CStoreNpc::Late_Update(_float fTimeDelta)
{
	__super::Late_Update(fTimeDelta);
	LateUpdate_State(fTimeDelta);
}

_bool CStoreNpc::Is_AnimationLoop(_uint eAnimId)
{
	return true;
}

void CStoreNpc::Update_State(_float fTimeDelta)
{
	_float fSpeed = {};
	switch (m_eState)
	{
	case ANIM_BLUE::IDLE:
		fSpeed = 4.f;
		break;
	}
	m_pModelCom->Play_Animation(fTimeDelta * fSpeed, true);
}

void CStoreNpc::LateUpdate_State(_float fTimeDelta)
{
}


HRESULT CStoreNpc::Ready_Components(void* pArg)
{
	if (FAILED(__super::Add_Components(TEXT("Com_Renderer"), LEVEL_STATIC, TEXT("Prototype_Component_Renderer"), (CComponent**)&m_pRendererCom)))
		return E_FAIL;

	/* For.Com_Transform */
	CTransform::TRANSFORMDESC TransformDesc;
	ZeroMemory(&TransformDesc, sizeof(CTransform::TRANSFORMDESC));
	TransformDesc.fSpeedPerSec = 2.5f;
	TransformDesc.fRotationPerSec = XMConvertToRadians(90.0f);

	if (FAILED(__super::Add_Components(TEXT("Com_Transform"), LEVEL_STATIC, TEXT("Prototype_Component_Transform"), (CComponent**)&m_pTransformCom, &TransformDesc)))
		return E_FAIL;

	/* For.Com_Shader */
		/* For.Com_Shader */
	if (FAILED(__super::Add_Components(TEXT("Com_Shader"), LEVEL_STATIC, TEXT("Prototype_Component_Shader_VtxAnimModel"), (CComponent**)&m_pShaderCom)))
		return E_FAIL;

	_tchar			szModeltag[MAX_PATH] = TEXT("");
	MultiByteToWideChar(CP_ACP, 0, m_BuildDesc.ModelDesc.pModeltag, strlen(m_BuildDesc.ModelDesc.pModeltag), szModeltag, MAX_PATH);
	if (FAILED(__super::Add_Components(TEXT("Com_Model"), LEVEL_GAMEPLAY, szModeltag, (CComponent**)&m_pModelCom)))
		return E_FAIL;

	CColider::COLLIDERDESC ColliderDesc;
	ZeroMemory(&ColliderDesc, sizeof(CColider::COLLIDERDESC));
	ColliderDesc.vScale = _float3(5.f, 5.f, 5.f);
	ColliderDesc.vRotation = _float3(0.f, 0.f, 0.f);
	ColliderDesc.vPosition = _float3(0.f, 0.f, 0.f);
	if (FAILED(__super::Add_Components(TEXT("Com_SPHERE"), LEVEL_STATIC, TEXT("Prototype_Component_Colider_Sphere"), (CComponent**)&m_pSphereCom, &ColliderDesc)))
		return E_FAIL;
	return S_OK;
}

CStoreNpc* CStoreNpc::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CStoreNpc* pInstance = new CStoreNpc(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CStoreNpc");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CGameObject* CStoreNpc::Clone(void* pArg)
{
	CStoreNpc* pInstance = new CStoreNpc(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CNexus");
		Safe_Release(pInstance);
	}

	return pInstance;
}


void CStoreNpc::Free()
{
	__super::Free();
}