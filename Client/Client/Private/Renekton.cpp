#include "Renekton.h"

#include "GameInstance.h"

CRenekton::CRenekton(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CChampion(pDevice, pContext)
{
}

CRenekton::CRenekton(const CRenekton& rhs)
	: CChampion(rhs)
{
}

HRESULT CRenekton::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CRenekton::Initialize(void* pArg)
{
	m_ePlayerID = EZREAL;

	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	return S_OK;
}

HRESULT CRenekton::Ready_Components(void* pArg)
{
	/* For.Com_Renderer */
	if (FAILED(__super::Add_Components(TEXT("Com_Renderer"), LEVEL_STATIC, TEXT("Prototype_Component_Renderer"), (CComponent**)&m_pRendererCom)))
		return E_FAIL;

	/* For.Com_Transform */
	CTransform::TRANSFORMDESC TransformDesc;
	ZeroMemory(&TransformDesc, sizeof(CTransform::TRANSFORMDESC));
	TransformDesc.fSpeedPerSec = 5.f;
	TransformDesc.fRotationPerSec = XMConvertToRadians(90.0f);
	if (FAILED(__super::Add_Components(TEXT("Com_Transform"), LEVEL_STATIC, TEXT("Prototype_Component_Transform"), (CComponent**)&m_pTransformCom, &TransformDesc)))
		return E_FAIL;


	/* For.Com_Shader */
	if (FAILED(__super::Add_Components(TEXT("Com_Shader"), LEVEL_STATIC, TEXT("Prototype_Component_Shader_VtxModel"), (CComponent**)&m_pShaderCom)))
		return E_FAIL;

	/* For.Com_Model*/
	if (FAILED(__super::Add_Components(TEXT("Com_Model"), LEVEL_GAMEPLAY, TEXT("Renekton"), (CComponent**)&m_pModelCom)))
		return E_FAIL;

	return S_OK;
}


CRenekton* CRenekton::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CRenekton* pInstance = new CRenekton(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CRenekton");
		Safe_Release(pInstance);
	}
	return pInstance;
}

CGameObject* CRenekton::Clone(void* pArg)
{
	CRenekton* pInstance = new CRenekton(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CRenekton");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CRenekton::Free()
{
	__super::Free();
}
