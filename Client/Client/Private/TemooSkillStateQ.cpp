#include "TemooSkillStateQ.h"

#include "GameInstance.h"
#include "Champion.h"
#include "Enemy.h"
#include "PickingManager.h"
#include "PoolManager.h"

#include "Temoo.h"
#include "TeemoProjectile.h"

#include "PlayerIdleState.h"
#include "PlayerRunState.h"

USING(Client)
USING(Player)

CTemooSkillStateQ::CTemooSkillStateQ(CPlayer* pPlayer)
{
	m_pOwner = pPlayer;
	m_ePlayerID = m_pOwner->Get_PlayerID();

	m_bProjectileSpawned = false;
	m_ePhase = PHASE::ATTACK;
	m_bIsAnimationFinished = false;

	m_iAttackAnim = CTemoo::ANIM::Q;
}

CPlayerState* CTemooSkillStateQ::HandleInput()
{
	// 스킬 시전 중 입력으로 끊기지 않게(베인 E랑 동일)
	return nullptr;
}

void CTemooSkillStateQ::Enter()
{
	m_bProjectileSpawned = false;
	m_bIsAnimationFinished = false;
	m_ePhase = PHASE::ATTACK;

	// Q 애니로 강제 전환
	CModel* pModel = m_pOwner->Get_Model();
	if (!pModel) return;

	pModel->Set_CurrentAnimIndex(m_iAttackAnim);
}

CPlayerState* CTemooSkillStateQ::Update(_float fTimeDelta)
{
	constexpr float kInterp = 0.25f;

	CModel* pModel = m_pOwner->Get_Model();
	if (!pModel)
		return new CIdleState(m_pOwner, CIdleState::IDLE_MAIN);

	// =========================
	// 0) 타겟 (공격 타겟 우선)
	// =========================
	CActor* pTarget = m_pOwner->Get_AttackTarget();

	// 타겟 없으면 Picking으로 찍힌 Enemy라도 잡아줌(스킬 Q 대상)
	if (!pTarget)
	{
		CPickingManager* PM = CPickingManager::GetInstance();
		if (PM)
		{
			if (CEnemy* enemy = dynamic_cast<CEnemy*>(PM->Get_PickedObj()))
				pTarget = enemy;
		}
	}

	// 타겟 없거나 죽었으면 그냥 끝
	if (!pTarget || pTarget->Get_Dead())
	{
		// 애니는 진행하다가 끝나면 Idle로
		bool bEnd = pModel->Play_Animation(fTimeDelta, false, nullptr, kInterp);
		if (bEnd)
			return new CIdleState(m_pOwner, CIdleState::IDLE_MAIN);
		return nullptr;
	}

	// =========================
	// 1) 타겟 바라보기 (티모 공격이랑 동일)
	// =========================
	_vector myPos = m_pOwner->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
	_vector tarPos = pTarget->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);

	_vector lookDir = tarPos - myPos;
	lookDir = XMVectorSetY(lookDir, 0.f);
	if (XMVectorGetX(XMVector3LengthSq(lookDir)) > 0.0001f)
	{
		lookDir = XMVector3Normalize(lookDir);
		m_pOwner->Get_Transform()->SmoothLookAt(-lookDir, fTimeDelta, 8.f);
	}

	// =========================
	// 2) 애니 재생
	// =========================
	bool bAnimEnd = pModel->Play_Animation(fTimeDelta, false, nullptr, kInterp);
	if (bAnimEnd)
		m_bIsAnimationFinished = true;

	// =========================
	// 3) 애니 이벤트 시점에 Q 발사 1회
	// =========================
	if (!m_bProjectileSpawned)
	{
		const vector<ANIMEVENT> events = pModel->Get_Events();
		for (const auto& evt : events)
		{
			if (!evt.isPlay) continue;
			if (evt.eType != ANIMEVENT::EVENT_PROJECTILE) continue;

			Spawn_TemooQProjectile(pTarget);
			m_bProjectileSpawned = true;
			break;
		}
	}

	// =========================
	// 4) 종료 → Idle
	// =========================
	if (m_bIsAnimationFinished && m_bProjectileSpawned)
		return new CIdleState(m_pOwner, CIdleState::IDLE_MAIN);

	return nullptr;
}

CPlayerState* CTemooSkillStateQ::Late_Update(_float /*fTimeDelta*/)
{
	return nullptr;
}

void CTemooSkillStateQ::Exit()
{
	// 따로 해제할 게 있으면 여기
}

void CTemooSkillStateQ::Spawn_TemooQProjectile(CActor* pTarget)
{
	if (!pTarget) return;

	// === 손뼈 기반 스폰 (티모 공격과 동일) ===
	CBone* pHand = m_pOwner->Get_Model()->Get_BonePtr("L_Hand");
	_vector vSpawn = m_pOwner->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
	vSpawn = XMVectorSetW(vSpawn, 1.f);

	if (pHand)
	{
		_float4x4 pivot = m_pOwner->Get_Model()->Get_PivotFloat4x4();
		_matrix pivotMat = XMLoadFloat4x4(&pivot);

		_matrix HandMatrix =
			pHand->Get_CombinedTransformationMatrix() *
			pivotMat *
			m_pOwner->Get_Transform()->Get_WorldMatrix();

		HandMatrix.r[0] = XMVector4Normalize(HandMatrix.r[0]);
		HandMatrix.r[1] = XMVector4Normalize(HandMatrix.r[1]);
		HandMatrix.r[2] = XMVector4Normalize(HandMatrix.r[2]);

		vSpawn = HandMatrix.r[3];
		vSpawn = XMVectorSetW(vSpawn, 1.f);
	}

	// ===== Q 투사체 DESC =====
	CProjectile::PROJECTILEDESC ProjDesc{};
	ProjDesc.eCollisionGroup = PLAYER;     // 플레이어 투사체 그룹 (티모 공격과 동일)
	ProjDesc.fDeadTime = 10.f;
	ProjDesc.fVelocity = 10.f;
	ProjDesc.iDamage = 60;                // 너 밸런스 값으로 수정

	// ★ 여기: 너 TeemoProjectile에서 Q 타입 enum이 있으면 그걸로 바꿔
	// ProjDesc.eProjType = CTemooProjectile::SKILL_Q;
	ProjDesc.eProjType = CTemooProjectile::BASE;

	ProjDesc.vInitPositon = vSpawn;

	ProjDesc.pOwner = m_pOwner;
	ProjDesc.pTarget = pTarget;
	ProjDesc.vTargetPosition = pTarget->Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
	ProjDesc.vTargetDir = XMVector3Normalize(ProjDesc.vTargetPosition - ProjDesc.vInitPositon);
	ProjDesc.eProjType = CTemooProjectile::Q_BLIND;
	ProjDesc.ePoolLevel = LEVEL_GAMEPLAY;
	ProjDesc.pPoolLayerTag = TEXT("Layer_TemooProjectile");
	ProjDesc.bPrewarm = false;

	CPoolManager* PM = CPoolManager::GetInstance();
	CGameInstance* GI = CGameInstance::GetInstance();
	const int pick = rand() % 2;
	switch (pick)
	{
	case 0:
		GI->PlaySounds(const_cast<TCHAR*>(L"04_teemo_blindingdart_OML_2.wav"), SOUND_PLAYER, 0.4f);
		break;
	case 1:
		GI->PlaySounds(const_cast<TCHAR*>(L"05_teemo_blindingdart_OML_3.wav"), SOUND_PLAYER, 0.4f);
		break;
	}
	// 풀 우선 재사용
	if (!PM->Reuse_Pooling_Object(LEVEL_GAMEPLAY, TEXT("Layer_TemooProjectile"), &ProjDesc))
	{
		GI->Add_GameObject(
			TEXT("Prototype_GameObject_TemooProjectile"),
			LEVEL_GAMEPLAY,
			TEXT("Layer_TemooProjectile"),
			&ProjDesc);
	}
}

