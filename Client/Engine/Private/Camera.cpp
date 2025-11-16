#include "Camera.h"
#include "PipeLine.h"

const _tchar* CCamera::m_pTransformTag = TEXT("Com_Transform");

CCamera::CCamera(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CGameObject(pDevice, pContext)
    , m_pPipeLine(CPipeLine::GetInstance())
{
    Safe_AddRef(m_pPipeLine);
}

CCamera::CCamera(const CCamera& rhs)
    : CGameObject(rhs)
    , m_pPipeLine(rhs.m_pPipeLine)
{
    Safe_AddRef(m_pPipeLine);
}

HRESULT CCamera::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CCamera::Initialize(void* pArg)
{
    m_pTransformCom = CTransform::Create(m_pDevice, m_pContext);
    if (nullptr == m_pTransformCom)
        return E_FAIL;

    memcpy(&m_CameraDesc, pArg, sizeof(CAMDESC));
    
    m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, XMLoadFloat4(&m_CameraDesc.vEye));
    m_pTransformCom->LookAt(XMLoadFloat4(&m_CameraDesc.vAt));

    m_pTransformCom->Set_TransformDesc(m_CameraDesc.TransformDesc);

    m_Components.emplace(m_pTransformTag, m_pTransformCom);

    Safe_AddRef(m_pTransformCom);

    return S_OK;
}

int CCamera::Update(_float fTimeDelta)
{
    return OBJ_NOEVENT;
}

void CCamera::Late_Update(_float fTimeDelta)
{
}

HRESULT CCamera::Render()
{
    return S_OK;
}

HRESULT CCamera::Bind_OnPipeLine()
{
    if (nullptr == m_pPipeLine)
        return E_FAIL;

    _matrix matView = XMMatrixInverse(nullptr, m_pTransformCom->Get_WorldMatrix());

    m_pPipeLine->Set_Transform(CPipeLine::D3DTS_VIEW, matView);
    m_pPipeLine->Set_Transform(CPipeLine::D3DTS_PROJ,
        XMMatrixPerspectiveFovLH(m_CameraDesc.fFov, m_CameraDesc.fAspect, m_CameraDesc.fNear, m_CameraDesc.fFar));

    return S_OK;
}



void CCamera::Free()
{
    __super::Free();

    Safe_Release(m_pPipeLine);
    Safe_Release(m_pTransformCom);
}
