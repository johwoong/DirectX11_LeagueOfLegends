#include "Turret.h"
#include "GameInstance.h"


CTurret::CTurret(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CBuilding(pDevice, pContext)
{
}

CTurret::CTurret(const CTurret& rhs)
	: CBuilding(rhs)
{
}

HRESULT CTurret::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CTurret::Initialize(void* pArg)
{
	__super::Initialize(pArg);
	m_eState = IDLE;
	m_pModelCom->Set_CurrentAnimIndex(m_eState);
	Set_Scale(_float3(0.01f, 0.01f, 0.01f));
	return S_OK;
}


_int CTurret::Update(_float fTimeDelta)
{
	__super::Update(fTimeDelta);
	Update_State(fTimeDelta);

	return OBJ_NOEVENT;
}

void CTurret::Late_Update(_float fTimeDelta)
{
	__super::Late_Update(fTimeDelta);
	LateUpdate_State(fTimeDelta);
}
_bool CTurret::Is_AnimationLoop(_uint eAnimId)
{
	return true;
}

void CTurret::Update_State(_float fTimeDelta)
{
	_float fSpeed = {};
	switch (m_eState)
	{
	case IDLE:
		fSpeed = 4.f;
		if (Get_TeamType() == TEAMTYPE::BLUE)
		{
			m_pTransformCom->Set_Rotation(_float3(0.f, -130.f, 0.f));
		}
		else
		{
			m_pTransformCom->Set_Rotation(_float3(0.f, 130.f, 0.f));
		}
		break;
	}
	m_pModelCom->Play_Animation(fTimeDelta * fSpeed, true);
}

void CTurret::LateUpdate_State(_float fTimeDelta)
{
}


HRESULT CTurret::Ready_Components(void* pArg)
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
	if (FAILED(__super::Add_Components(TEXT("Com_Shader"), LEVEL_STATIC, TEXT("Prototype_Component_Shader_VtxAnimModel"), (CComponent**)&m_pShaderCom)))
		return E_FAIL;

	_tchar			szModeltag[MAX_PATH] = TEXT("");
	MultiByteToWideChar(CP_ACP, 0, m_BuildDesc.ModelDesc.pModeltag, strlen(m_BuildDesc.ModelDesc.pModeltag), szModeltag, MAX_PATH);
	if (FAILED(__super::Add_Components(TEXT("Com_Model"), LEVEL_GAMEPLAY, szModeltag, (CComponent**)&m_pModelCom)))
		return E_FAIL;

	CColider::COLLIDERDESC ColliderDesc;
	ZeroMemory(&ColliderDesc, sizeof(CColider::COLLIDERDESC));
	ColliderDesc.vScale = _float3(7.f, 7.f, 7.f);
	ColliderDesc.vRotation = _float3(0.f, 0.f, 0.f);
	ColliderDesc.vPosition = _float3(0.f, 14.f, 0.f);
	if (FAILED(__super::Add_Components(TEXT("Com_SPHERE"), LEVEL_STATIC, TEXT("Prototype_Component_Colider_Sphere"), (CComponent**)&m_pSphereCom, &ColliderDesc)))
		return E_FAIL;
	return S_OK;
}

CBuilding::HPBARDESC CTurret::Get_HpBarDesc() const
{
	HPBARDESC desc;
	desc.fWidth = 3.f;    // 넥서스보다 조금 작게
	desc.fHeight = 0.5f;
	desc.fOffsetY = 5.f;    // 포탑 높이에 맞게
	return desc;
}

CTurret* CTurret::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CTurret* pInstance = new CTurret(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CNexus");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CGameObject* CTurret::Clone(void* pArg)
{
	CTurret* pInstance = new CTurret(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CNexus");
		Safe_Release(pInstance);
	}

	return pInstance;
}


void CTurret::Free()
{
	__super::Free();
}