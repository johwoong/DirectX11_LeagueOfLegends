#include "Turret.h"
#include "GameInstance.h"
#include "UI_AttackRange.h"
#include "Collision_Manager.h"
#include "Projectile.h"
#include "Minion.h"
#include "PlayerManager.h"
#include "Champion.h"

static float GetDistanceXZ(_vector a, _vector b)
{
	a = XMVectorSetY(a, 0.f);
	b = XMVectorSetY(b, 0.f);
	return XMVectorGetX(XMVector3Length(b - a));
}


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

    // 공격 쿨타임
    m_fAttackCoolTime = (m_tStats.fAttackSpeed > 0.f)
        ? 1.f / m_tStats.fAttackSpeed : 1.f;

    // === RED 팀만 사거리 UI 생성 ===
    if (Get_TeamType() == TEAM_RED)
    {
        CGameInstance* GI = CGameInstance::GetInstance();
        CGameObject* pObj = nullptr;

        if (FAILED(GI->Add_GameObject_Out(
            TEXT("Prototype_GameObject_UI_AttackRange"),
            LEVEL_GAMEPLAY,
            TEXT("Layer_UI_AttackRange"),
            pObj,
            this)))         // owner = turret
            return E_FAIL;

        m_pAttackRange = dynamic_cast<CAttackRange*>(pObj);
        if (m_pAttackRange)
        {
            m_pAttackRange->Set_Owner(this);
            m_pAttackRange->Set_RadiusWorld(m_tStats.fAttackRange);
            m_pAttackRange->Set_Visible(false);
        }
    }
    else
    {
        m_pAttackRange = nullptr;   // BLUE는 아예 없음
    }

    return S_OK;
}


_int CTurret::Update(_float fTimeDelta)
{
    __super::Update(fTimeDelta);
    Update_State(fTimeDelta);

    // ★ 죽었으면 공격/타겟/사거리 UI 로직 전부 스킵
    if (m_bDead)
        return OBJ_NOEVENT;

    m_fAttackTimer += fTimeDelta;

    // 1) 타겟 검증
    if (m_pTarget)
    {
        if (m_pTarget->Get_Dead())
            m_pTarget = nullptr;
        else
        {
            float dist = GetDistanceXZ(
                Get_Transform()->Get_State(CTransform::STATE_TRANSLATION),
                m_pTarget->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION));

            if (dist > m_tStats.fAggroRange * 1.2f)
                m_pTarget = nullptr;
        }
    }

    // 2) 타겟이 없으면 탐색
    if (!m_pTarget)
    {
        CCollision_Manager* pCM = CCollision_Manager::GetInstance();
        if (!pCM || !m_pTransformCom)
            return OBJ_NOEVENT;

        CActor* best = nullptr;
        float   bestDist = m_tStats.fAggroRange;

        _vector myPos = Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);

        // ============================
        // 1) 미니언 (상대 팀만)
        // ============================
        auto& minions = pCM->Get_CollisionGroup(CCollision_Manager::COLLISION_MINION);
        for (auto obj : minions)
        {
            auto pMinion = dynamic_cast<CMinion*>(obj);
            if (!pMinion) continue;
            if (pMinion->Get_Team() == Get_TeamType()) continue; // 같은 팀 제외
            if (pMinion->IsDeadOrDying()) continue;

            float d = GetDistanceXZ(myPos,
                pMinion->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION));
            if (d < bestDist)
            {
                bestDist = d;
                best = pMinion;
            }
        }

        // ============================
        // 2) 적 챔피언 (AI 적: COLLISION_ENEMY)
        //    -> 특히 블루 포탑이 레드 챔피언(Vayne) 보게 하는 부분
        // ============================
        auto& enemies = pCM->Get_CollisionGroup(CCollision_Manager::COLLISION_ENEMY);
        for (auto obj : enemies)
        {
            CActor* pA = dynamic_cast<CActor*>(obj);
            if (!pA || pA->Get_Dead())
                continue;

            // 지금 구조상 COLLISION_ENEMY에는 전부 "플레이어의 적"이 들어오니까
            // 블루 포탑이면 전부 적이라고 봐도 됨.
            // (나중에 Enemy에 Team 생기면 여기서 팀도 체크)
            if (Get_TeamType() == TEAM_RED)
            {
                // 레드 포탑 기준으로는 COLLISION_ENEMY 쪽은 아군이니까 스킵
                continue;
            }

            float d = GetDistanceXZ(myPos,
                pA->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION));
            if (d < bestDist)
            {
                bestDist = d;
                best = pA;
            }
        }

        // ============================
        // 3) 플레이어 챔피언 (ActivePlayer)
        //    -> 레드 포탑이 블루 플레이어를 때리는 용도
        // ============================
        CChampion* pPlayer = CPlayerManager::GetInstance()->Get_ActivePlayer();
        if (pPlayer && !pPlayer->Get_Dead())
        {
            // 포탑 팀과 플레이어 팀이 다를 때만 공격 (지금은 RED 포탑만)
            if (Get_TeamType() != TEAM_BLUE)
            {
                float d = GetDistanceXZ(myPos,
                    pPlayer->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION));
                if (d < bestDist)
                {
                    bestDist = d;
                    best = pPlayer;
                }
            }
        }

        m_pTarget = best;
    }

    // 3) 타겟이 있고 사거리 & 쿨타임 OK → 투사체 발사
    if (m_pTarget)
    {
        _vector myPos = Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
        _vector tarPos = m_pTarget->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
        float dist = GetDistanceXZ(myPos, tarPos);

        if (dist <= m_tStats.fAttackRange && m_fAttackTimer >= m_fAttackCoolTime)
        {
            m_fAttackTimer = 0.f;

            CProjectile::PROJECTILEDESC desc{};
            desc.eType = CProjectile::PROJ_TARGET;
            desc.pOwner = this;
            desc.pTarget = m_pTarget;
            desc.fSpeed = 15.f;
            desc.fDamage = m_tStats.fAttackDamage;
            desc.vStartPos = myPos;          // 포탄 발사 위치 (필요하면 포탑 머리본 사용)

            // 포탑 평타라면 별도 태그
            desc.eSkillTag = CProjectile::SKILL_NONE;

            CGameInstance::GetInstance()->Add_GameObject(
                TEXT("Prototype_GameObject_Projectile"),
                LEVEL_GAMEPLAY,
                TEXT("Layer_Projectile"),
                &desc);
        }
    }

    // 4) 포탑 사거리 UI 업데이트 (플레이어 거리 기반)
    if (m_pAttackRange)
    {
        CChampion* pPlayer = CPlayerManager::GetInstance()->Get_ActivePlayer();
        if (pPlayer)
        {
            _vector myPos = Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
            _vector plPos = pPlayer->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
            float d = GetDistanceXZ(myPos, plPos);

            float showDist = m_tStats.fAttackRange + 4.f;
            float fadeInner = m_tStats.fAttackRange + 1.f;

            float alpha = 0.f;
            if (d < showDist)
            {
                if (d <= fadeInner)
                    alpha = 1.f;
                else
                {
                    float t = (d - fadeInner) / (showDist - fadeInner); // 0~1
                    alpha = 1.f - t;
                }
            }

            // ★ 최대 투명도 제한 (롤처럼 은은하게)
            const float maxAlpha = 0.8f;        // 필요하면 0.4~0.7 사이로 조절
            m_pAttackRange->Set_Alpha(alpha * maxAlpha);

            m_pAttackRange->Set_Visible(alpha > 0.01f);
            m_pAttackRange->Set_RadiusWorld(m_tStats.fAttackRange);
        }
    }

    return OBJ_NOEVENT;
}

void CTurret::Late_Update(_float fTimeDelta)
{
	__super::Late_Update(fTimeDelta);
	LateUpdate_State(fTimeDelta);
}
_bool CTurret::Is_AnimationLoop(_uint eAnimId)
{
    switch (eAnimId)
    {
    case IDLE:    return true;
    case DEATH:   return false;
    case DESTORY: return false;
    }
    return false;
}

void CTurret::Update_State(_float fTimeDelta)
{
    float fSpeed = 0.f;

    switch (m_eState)
    {
    case IDLE:
        fSpeed = 4.f;
        if (Get_TeamType() == TEAMTYPE::BLUE)
            m_pTransformCom->Set_Rotation(_float3(0.f, -130.f, 0.f));
        else
            m_pTransformCom->Set_Rotation(_float3(0.f, 130.f, 0.f));

        // ? 이게 문제
        // m_pModelCom->Set_CurrentAnimIndex(IDLE);

        // IDLE은 루프 애니메이션
        m_pModelCom->Play_Animation(fTimeDelta * fSpeed, true);
        break;

    case DEATH:
    {
        fSpeed = 1.f;

        if (Get_TeamType() == TEAMTYPE::BLUE)
            m_pTransformCom->Set_Rotation(_float3(0.f, 90.f, 0.f));
        else
            m_pTransformCom->Set_Rotation(_float3(0.f, -90.f, 0.f));
        m_pModelCom->Play_Animation(fTimeDelta * fSpeed, false);
    }
    break;
    }
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
	ColliderDesc.vScale = _float3(3.f, 4.f, 3.f);
	ColliderDesc.vPosition = _float3(0.f, 200.f, 0.f); 
	if (FAILED(__super::Add_Components(TEXT("Com_SPHERE"), LEVEL_STATIC, TEXT("Prototype_Component_Colider_Sphere"), (CComponent**)&m_pSphereCom, &ColliderDesc)))
		return E_FAIL;
	return S_OK;
}

_int CTurret::Take_Damage(int iDamage, CActor* DamageCauser)
{
    _int curHp = CBuilding::Take_Damage(iDamage, DamageCauser);

    if (m_bDead && m_eState != DEATH && m_eState != DESTORY)
    {
        m_eState = DEATH;
        if (m_pModelCom)
            m_pModelCom->Set_CurrentAnimIndex(DEATH); // ★ 여기에서 딱 한 번만 세팅

        m_pTarget = nullptr;
        if (m_pAttackRange)
        {
            m_pAttackRange->Set_Visible(false);
            m_pAttackRange->Set_Alpha(0.f);
        }
    }

    return curHp;
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