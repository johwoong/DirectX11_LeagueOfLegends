#include "MushRoom.h"
#include "GameInstance.h"
#include "Collision_Manager.h"
#include "Effect.h"
#include "Minion.h"
#include <cmath>

USING(Client)

static constexpr float kMushForwardSign = 1.f;

std::vector<CMushRoom*> CMushRoom::s_Alive;

HRESULT CMushRoom::Initialize_Prototype()
{
    return S_OK;
}

CMushRoom::CMushRoom(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CActor(pDevice, pContext)
{
}

CMushRoom::CMushRoom(const CMushRoom& rhs)
    : CActor(rhs)
{
}

void CMushRoom::RegisterSelf()
{
    if (m_bRegistered) return;
    s_Alive.push_back(this);
    m_bRegistered = true;
}

void CMushRoom::UnregisterSelf()
{
    if (!m_bRegistered) return;
    auto it = std::find(s_Alive.begin(), s_Alive.end(), this);
    if (it != s_Alive.end())
        s_Alive.erase(it);
    m_bRegistered = false;
}

void CMushRoom::ResolvePlacePos_Backward(_vector& ioPos, _vector dirXZ, float minDist, int maxIter)
{
    dirXZ = XMVectorSetY(dirXZ, 0.f);
    if (XMVectorGetX(XMVector3LengthSq(dirXZ)) < 1e-6f)
        return;

    dirXZ = XMVector3Normalize(dirXZ);
    _vector back = dirXZ;

    const float keepY = XMVectorGetY(ioPos);

    for (int iter = 0; iter < maxIter; ++iter)
    {
        bool pushed = false;

        for (CMushRoom* m : s_Alive)
        {
            if (!m) continue;
            if (m->Get_Dead()) continue;

            _vector p1 = m->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);

            _vector d = XMVectorSetY(ioPos - p1, 0.f);
            float dist = XMVectorGetX(XMVector3Length(d));

            if (dist < minDist)
            {
                float push = (minDist - dist) + 0.05f;
                ioPos = ioPos + back * push;

                ioPos = XMVectorSetY(ioPos, keepY);
                ioPos = XMVectorSetW(ioPos, 1.f);

                pushed = true;
                break;
            }
        }

        if (!pushed)
            break;
    }
}

HRESULT CMushRoom::Initialize(void* pArg)
{
    if (pArg)
        memcpy(&m_Desc, pArg, sizeof(MUSHROOM_DESC));

    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    if (FAILED(Ready_Components(pArg)))
        return E_FAIL;

    m_fLifeAcc = 0.f;
    m_bTriggered = false;
    m_bArmed = false;

    // ? 시작 위치 = 커서(또는 사거리 클램프된 위치)
    _vector start = XMVectorSet(m_Desc.vPos.x, m_Desc.vPos.y, m_Desc.vPos.z, 1.f);

    // ? final 위치 = 겹치면 뒤로 밀린 위치 (기존 버섯들 기준)
    _vector dirXZ = XMVectorSet(m_Desc.vDir.x, 0.f, m_Desc.vDir.z, 0.f);
    _vector finalPos = start;
    ResolvePlacePos_Backward(finalPos, dirXZ, 1.8f, 12);

    m_vPlaceStart = start;
    m_vPlaceFinal = finalPos;
    m_fPlaceAcc = 0.f;

    // 이동이 실제로 발생할 때만 “튕김”
    _vector delta = XMVectorSetY(finalPos - start, 0.f);
    m_bPlaceSlide = (XMVectorGetX(XMVector3LengthSq(delta)) > 1e-6f);

    // ? 일단 start로 스폰
    m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, start);

    // ? 리스트 등록은 final 계산 후에 (자기 자신이 Resolve에 걸리지 않게)
    RegisterSelf();

    // 방향
    Apply_DirToTransform();

    // 모델 스케일(시각만)
    m_pTransformCom->Set_Scale(CTransform::STATE_RIGHT, 0.015f);
    m_pTransformCom->Set_Scale(CTransform::STATE_UP, 0.015f);
    m_pTransformCom->Set_Scale(CTransform::STATE_LOOK, 0.015f);

    // 애니
    if (m_pModelCom)
    {
        m_pModelCom->Set_CurrentAnimIndex(CMushRoom::ANIM::SPAWN);
        m_pModelCom->Set_TimeReset();
    }

    Update_Sphere_NoScale();
    return S_OK;
}

void CMushRoom::Update_Sphere_NoScale()
{
    if (!m_pSphereCom || !m_pTransformCom) return;

    _matrix W = m_pTransformCom->Get_WorldMatrix();

    // 축 정규화(스케일 제거) - 안전 처리
    auto SafeNorm = [](_vector v)
        {
            float lsq = XMVectorGetX(XMVector3LengthSq(v));
            if (lsq < 1e-12f) return XMVectorSet(1.f, 0.f, 0.f, 0.f);
            return XMVector3Normalize(v);
        };

    W.r[0] = SafeNorm(W.r[0]);
    W.r[1] = SafeNorm(W.r[1]);
    W.r[2] = SafeNorm(W.r[2]);

    m_pSphereCom->Update(W);
}

int CMushRoom::Update(_float dt)
{
    int iEvent = __super::Update(dt);
    if (iEvent == OBJ_DEAD)
    {
        UnregisterSelf();
        return OBJ_DEAD;
    }

    Update_Sphere_NoScale();
    // ? 설치 겹침이면 0.12초 동안 뒤로 “튕기듯” 이동
    if (m_bPlaceSlide)
    {
        m_fPlaceAcc += dt;
        float t = (m_fPlaceTime > 1e-6f) ? (m_fPlaceAcc / m_fPlaceTime) : 1.f;
        if (t > 1.f) t = 1.f;

        // SmoothStep
        t = t * t * (3.f - 2.f * t);

        _vector pos = XMVectorLerp(m_vPlaceStart, m_vPlaceFinal, t);
        m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, pos);

        if (t >= 1.f)
        {
            m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, m_vPlaceFinal);
            m_bPlaceSlide = false;
        }
    }

    // 수명
    m_fLifeAcc += dt;
    if (m_Desc.fLifeTime > 0.f && m_fLifeAcc >= m_Desc.fLifeTime)
    {
        UnregisterSelf();
        m_bDead = true;
        return OBJ_DEAD;
    }

    if (m_bTriggered)
        return OBJ_NOEVENT;

    // 애니
    if (m_pModelCom)
    {
        if (!m_bArmed)
        {
            const bool spawnEnd = m_pModelCom->Play_Animation(dt, false, nullptr, 0.1f);
            if (spawnEnd)
            {
                m_bArmed = true;
                m_pModelCom->Set_CurrentAnimIndex(CMushRoom::ANIM::IDLE);
                m_pModelCom->Set_TimeReset();
            }
            return OBJ_NOEVENT; // 스폰 끝나기 전 밟기 금지
        }
        else
        {
            m_pModelCom->Play_Animation(dt, true, nullptr, 0.1f);
        }
    }
    else
    {
        m_bArmed = true;
    }

    // 밟음 체크
    auto& enemyGroup = CCollision_Manager::GetInstance()->Get_CollisionGroup(CCollision_Manager::COLLISION_ENEMY);
    auto& minionGroup = CCollision_Manager::GetInstance()->Get_CollisionGroup(CCollision_Manager::COLLISION_MINION);

    auto CheckGroup = [&](auto& group) -> bool
        {
            for (auto obj : group)
            {
                CActor* a = dynamic_cast<CActor*>(obj);
                if (!a) continue;
                if (a->Get_Dead()) continue;

                // ?? 블루팀 미니언은 스킵
                if (CMinion* pMinion = dynamic_cast<CMinion*>(a))
                {
                    if (pMinion->Get_Team() == TEAM_BLUE)
                        continue;
                }

                CColider* targetCol = a->Get_Collider();
                if (!targetCol || !m_pSphereCom) continue;

                if (m_pSphereCom->Collision(targetCol))
                {
                    Trigger_Explode(a);
                    m_pGameInstance->PlaySounds(const_cast<TCHAR*>(L"11_teemo_ noxioustrap_obd_1.wav"), SOUND_PLAYER, 0.8f);
                    return true;
                }
            }
            return false;
        };

    if (CheckGroup(enemyGroup))  return OBJ_NOEVENT;
    if (CheckGroup(minionGroup)) return OBJ_NOEVENT;

    return OBJ_NOEVENT;
}

void CMushRoom::Late_Update(_float fTimeDelta)
{
    __super::Late_Update(fTimeDelta);
    if (!m_pRendererCom) return;
    m_pRendererCom->Add_RenderGroup(CRenderer::RENDER_NOALPHABLEND, this);
}

HRESULT CMushRoom::Render()
{
    if (!m_pShaderCom || !m_pModelCom)
        return E_FAIL;

    if (FAILED(SetUp_ShaderResources()))
        return E_FAIL;

    const _uint n = m_pModelCom->Get_NumMeshs();
    for (_uint i = 0; i < n; ++i)
    {
        if (FAILED(m_pModelCom->SetUp_Material(m_pShaderCom, "g_DiffuseTexture", i, aiTextureType_DIFFUSE)))
            return E_FAIL;

        if (FAILED(m_pModelCom->SetUp_Material(m_pShaderCom, "g_NormalTexture", i, aiTextureType_NORMALS)))
            return E_FAIL;

        if (FAILED(m_pModelCom->Render(m_pShaderCom, i, 7)))
            return E_FAIL;
    }
    return S_OK;
}

HRESULT CMushRoom::Render_Glow() { return S_OK; }
HRESULT CMushRoom::Render_ShadowDepth() { return S_OK; }

void CMushRoom::Trigger_Explode(CActor* pVictim)
{
    if (m_bTriggered) return;
    m_bTriggered = true;

    UnregisterSelf();

    _matrix world = m_pTransformCom->Get_WorldMatrix();
    if (m_Desc.pExplodeDat)
        CEffect::PlayEffectAtLocation(TEXT("TeemoRBurst.dat"), world);

    if (pVictim)
    {
        if (m_Desc.iExplodeDamage > 0)
            pVictim->Take_Damage(m_Desc.iExplodeDamage, m_Desc.pOwner);

        pVictim->Apply_TeemoPoison(
            m_Desc.pOwner,
            m_Desc.fPoisonDuration,
            m_Desc.iPoisonDamagePerTick,
            m_Desc.fPoisonTickInterval);
    }

    m_bDead = true;
}

void CMushRoom::Apply_DirToTransform()
{
    if (!m_pTransformCom) return;

    _vector vDir = XMVectorSet(m_Desc.vDir.x, 0.f, m_Desc.vDir.z, 0.f);
    if (XMVectorGetX(XMVector3LengthSq(vDir)) < 1e-6f)
        vDir = XMVectorSet(0.f, 0.f, 1.f, 0.f);
    else
        vDir = XMVector3Normalize(vDir);

    _vector vLook = XMVector3Normalize(vDir * kMushForwardSign);

    const _vector upW = XMVectorSet(0.f, 1.f, 0.f, 0.f);
    _vector vRight = XMVector3Cross(upW, vLook);
    if (XMVectorGetX(XMVector3LengthSq(vRight)) < 1e-6f)
        vRight = XMVectorSet(1.f, 0.f, 0.f, 0.f);
    else
        vRight = XMVector3Normalize(vRight);

    _vector vUp = XMVector3Normalize(XMVector3Cross(vLook, vRight));

    m_pTransformCom->Set_State(CTransform::STATE_RIGHT, vRight);
    m_pTransformCom->Set_State(CTransform::STATE_UP, vUp);
    m_pTransformCom->Set_State(CTransform::STATE_LOOK, vLook);
}

HRESULT CMushRoom::Ready_Components(void* pArg)
{
    if (FAILED(__super::Add_Components(TEXT("Com_Renderer"), LEVEL_STATIC, TEXT("Prototype_Component_Renderer"), (CComponent**)&m_pRendererCom)))
        return E_FAIL;

    CTransform::TRANSFORMDESC td{};
    td.fSpeedPerSec = 0.f;
    td.fRotationPerSec = XMConvertToRadians(90.0f);
    if (FAILED(__super::Add_Components(TEXT("Com_Transform"), LEVEL_STATIC, TEXT("Prototype_Component_Transform"), (CComponent**)&m_pTransformCom, &td)))
        return E_FAIL;

    if (FAILED(__super::Add_Components(TEXT("Com_Shader"), LEVEL_STATIC, TEXT("Prototype_Component_Shader_VtxAnimModel"), (CComponent**)&m_pShaderCom)))
        return E_FAIL;

    if (FAILED(__super::Add_Components(TEXT("Com_Model"), LEVEL_GAMEPLAY, TEXT("MushRoom"), (CComponent**)&m_pModelCom)))
        return E_FAIL;

    // ? 트리거 반경 기반으로 콜라이더 크기 설정
    float r = 0.9f;
    if (pArg)
    {
        MUSHROOM_DESC tmp{};
        memcpy(&tmp, pArg, sizeof(MUSHROOM_DESC));
        r = tmp.fTriggerRadius;
    }

    CColider::COLLIDERDESC cd{};
    cd.vScale = _float3(0.01f, 0.01f, 0.01f); // (네 콜라이더가 반지름 스케일이면 r로 바꿔)
    cd.vRotation = _float3(0.f, 0.f, 0.f);
    cd.vPosition = _float3(0.f, 0.2f, 0.f);

    if (FAILED(__super::Add_Components(TEXT("Com_SPHERE"), LEVEL_STATIC, TEXT("Prototype_Component_Colider_Sphere"), (CComponent**)&m_pSphereCom, &cd)))
        return E_FAIL;


    return S_OK;
}

HRESULT CMushRoom::SetUp_ShaderResources()
{
    if (!m_pShaderCom || !m_pTransformCom)
        return E_FAIL;

    _float4x4 W = m_pTransformCom->Get_World_TP();
    _float4x4 V = m_pGameInstance->Get_TransformFloat4x4_TP(CPipeLine::D3DTS_VIEW);
    _float4x4 P = m_pGameInstance->Get_TransformFloat4x4_TP(CPipeLine::D3DTS_PROJ);

    if (FAILED(m_pShaderCom->Set_RawValue("g_WorldMatrix", &W, sizeof(_float4x4)))) return E_FAIL;
    if (FAILED(m_pShaderCom->Set_RawValue("g_ViewMatrix", &V, sizeof(_float4x4)))) return E_FAIL;
    if (FAILED(m_pShaderCom->Set_RawValue("g_ProjMatrix", &P, sizeof(_float4x4)))) return E_FAIL;

    return S_OK;
}

CMushRoom* CMushRoom::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CMushRoom* pInstance = new CMushRoom(pDevice, pContext);
    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : CMushRoom");
        Safe_Release(pInstance);
    }
    return pInstance;
}

CGameObject* CMushRoom::Clone(void* pArg)
{
    CMushRoom* pInstance = new CMushRoom(*this);
    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned : CMushRoom");
        Safe_Release(pInstance);
    }
    return pInstance;
}

void CMushRoom::Free()
{
    UnregisterSelf();
    __super::Free();
}
