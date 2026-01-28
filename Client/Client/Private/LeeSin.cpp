#include "LeeSin.h"

#include "GameInstance.h"
#include "Channel.h"
#include "PickingManager.h"
#include "CursorManager.h"
#include "Effect.h"
_bool CLeeSin::Is_AnimationLoop(_uint eAnimId)
{
	switch ((ANIM)eAnimId)
	{
	case IDLE1:
	case IDLE_BASE:
	case IDLE_BASE1:
	case RUN1:
	case RUN_B:
		return true;
	}
	return false;
}

void CLeeSin::Clear_WShieldEffect()
{
	// ★ Detach -> Dead 순서로 처리 (AttachObject AddRef 누수 방지)
	for (auto& fx : m_vecWShieldFx)
	{
		if (!fx) continue;

		// AttachObject가 AddRef 하고 있으면 반드시 끊어줘야 함
		fx->Set_AttachObject(nullptr);

		// 바로 제거
		fx->Set_Dead(true);
	}

	m_vecWShieldFx.clear();
}

void CLeeSin::Play_WShieldEffect(const _tchar* pDat)
{
	if (!pDat) return;

	Clear_WShieldEffect();

	_matrix world = Get_Transform()->Get_WorldMatrix();
	_vector vPos = Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);

	vPos += XMVectorSet(0.f, 1.f, 0.f, 0.f); // ★ 추가
	vPos = XMVectorSetW(vPos, 1.f);
	world.r[3] = vPos;
	CGameInstance::GetInstance()->PlaySounds(const_cast<TCHAR*>(L"LeeSinWSound.wav"), SOUND_PLAYERTALK, 0.4f);
	CGameInstance::GetInstance()->PlaySounds(const_cast<TCHAR*>(L"LeeSinWShiledSound.wav"), SOUND_PLAYER, 0.4f);
	auto fxList = CEffect::PlayEffectAtLocation(pDat, world);
	for (auto& fx : fxList)
	{
		if (!fx) continue;
		fx->Set_AttachObject(this);
		fx->Set_WorldPosition(world);
		m_vecWShieldFx.push_back(fx);
	}
}

void CLeeSin::Cast_W_Self(float shieldHP, float shieldDur, float recastWindow,
	const _tchar* pShieldFxDat)
{
	Apply_WShield(shieldHP, shieldDur);
	Start_WRecast(recastWindow);

	// 1) 기존 실드 FX
	Play_WShieldEffect(pShieldFxDat);

	// 2) 추가 발밑 FX (너가 말한 이름)
	Play_WDownEffect(TEXT("LeeSin_WDown.dat")); // <- 파일명이 다르면 여기만 바꿔
}

void CLeeSin::Ready_Animation_Event()
{
	CAnimation* pAnim = m_pModelCom->Get_Animations()[ATTACK_L_PUNCH];
	if (!pAnim) return;

	ANIMEVENT evt{};
	evt.fStartTime = 8.2f;
	evt.fEndTime = 12.f;
	evt.eType = ANIMEVENT::EVENT_COLLIDER;
	pAnim->Add_Event(evt);

	pAnim = m_pModelCom->Get_Animations()[ATTACK_KICK];
	evt.fStartTime = 9.f;
	evt.fEndTime = 30.f;
	evt.eType = ANIMEVENT::EVENT_COLLIDER;
	pAnim->Add_Event(evt);

	pAnim = m_pModelCom->Get_Animations()[ATTACK_R_PUCNH];
	evt.fStartTime = 8.8f;
	evt.fEndTime = 28.f;
	evt.eType = ANIMEVENT::EVENT_COLLIDER;
	pAnim->Add_Event(evt);

	pAnim = m_pModelCom->Get_Animations()[ATTACK_R_KICK];
	evt.fStartTime = 8.f;
	evt.fEndTime = 32.f;
	evt.eType = ANIMEVENT::EVENT_COLLIDER;
	pAnim->Add_Event(evt);;

	// ==========================
   // ===== 리신 Q 발사 이벤트 =====
   // Q 애니에서 음파 투사체 스폰
   // ==========================
	pAnim = m_pModelCom->Get_Animations()[Q];
	if (pAnim)
	{
		ANIMEVENT evt{};
		evt.fStartTime = 7.f;   // 튜닝
		evt.fEndTime = 8.f;   // 튜닝
		evt.eType = ANIMEVENT::EVENT_PROJECTILE;
		pAnim->Add_Event(evt);
	}

	// ===== Q2 타격(TRANS) 이벤트 =====
	auto AddQTransHit = [&](ANIM anim, float s, float e)
		{
			CAnimation* a = m_pModelCom->Get_Animations()[anim];
			if (!a) return;
			ANIMEVENT evt{};
			evt.fStartTime = s;
			evt.fEndTime = e;
			evt.eType = ANIMEVENT::EVENT_COLLIDER;
			a->Add_Event(evt);
		};

	AddQTransHit(Q_LONGTRANS, 6.f, 10.f);  
	AddQTransHit(Q_MIDTRANS, 6.f, 10.f);  
	AddQTransHit(Q_SHORTTRANS, 6.f, 10.f);   

	pAnim = m_pModelCom->Get_Animations()[E];
	evt.fStartTime = 7.f;
	evt.fEndTime = 20.f;
	evt.eType = ANIMEVENT::EVENT_COLLIDER;
	pAnim->Add_Event(evt);;


	pAnim = m_pModelCom->Get_Animations()[E_B];
	evt.fStartTime = 7.f;
	evt.fEndTime = 20.f;
	evt.eType = ANIMEVENT::EVENT_COLLIDER;
	pAnim->Add_Event(evt);;

	// ==========================
// ===== 리신 R (R_A/R_B/R_C) 킥 타격 이벤트 =====
// ==========================
	auto AddRHit = [&](ANIM anim, float s, float e)
		{
			CAnimation* a = m_pModelCom->Get_Animations()[anim];
			if (!a) return;
			ANIMEVENT evt{};
			evt.fStartTime = s;
			evt.fEndTime = e;
			evt.eType = ANIMEVENT::EVENT_COLLIDER;
			a->Add_Event(evt);
		};

	// 시간은 네 모델에 맞게 튜닝해
	AddRHit(R_A, 8.f, 18.f);
	AddRHit(R_B, 8.f, 18.f);
	AddRHit(R_C, 8.f, 18.f);
}


CLeeSin::CLeeSin(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CChampion(pDevice, pContext)
{
}

CLeeSin::CLeeSin(const CLeeSin& rhs)
	: CChampion(rhs)
{
}

HRESULT CLeeSin::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CLeeSin::Initialize(void* pArg)
{
	m_ePlayerID = LEESIN;

	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;
	m_tStat.AttackRange = 80.f; 
	m_tStat.AttackDamage = 100.f;
	m_tStat.MaxHp = 10000.f;
	m_tStat.Hp = m_tStat.MaxHp;
	Set_Scale(_float3(0.017f, 0.017f, 0.017f));
	SetSkillInfo();
	Set_DeathAnim(CLeeSin::ANIM::DEATH);
	Set_SpawnAnim(CLeeSin::ANIM::RESPAWN);   // 네 enum 실제 이름으로
	return S_OK;
}

int CLeeSin::Update(_float fTimeDelta)
{
	const int iRet = __super::Update(fTimeDelta);
	if (iRet == OBJ_DEAD)
		return OBJ_DEAD;

	Update_WShield(fTimeDelta);
	Update_RecastTimers(fTimeDelta);
	Update_WEmpowerEffect();
	// Q 표식 자동 정리(너 기존 로직 유지)
	if (m_pQMarkTarget)
	{
		if (!IsUsingSkill())
		{
			if (!Can_QRecast())
				Clear_QMarkTarget();
		}
	}

	Update_SkillIcons();

	CPickingManager::GetInstance()->Out_PickingGroup(this);
	return OBJ_NOEVENT;
}

void CLeeSin::Late_Update(_float fTimeDelta)
{
	__super::Late_Update(fTimeDelta);
	CPickingManager::GetInstance()->Add_PickingGroup(this);
}

void CLeeSin::Start_WRecast(float window)
{
	m_bWRecastReady = true;
	m_fWRecastAcc = 0.f;
	m_fWRecastWindow = window;
}

void CLeeSin::Consume_WRecast(bool bUsed)
{
	m_bWRecastReady = false;
	m_fWRecastAcc = 0.f;
	
	if (!bUsed)
		return;

	// ? W2 사용 시: 실드 "버블 이펙트"만 제거
	Clear_WShieldEffect();
}

void CLeeSin::Apply_WEmpower(int stacks, float duration)
{
	m_bWEmpower = true;
	m_iWEmpowerStacks = max(1, stacks);
	m_fWEmpowerDuration = max(0.01f, duration);
	m_fWEmpowerAcc = 0.f;
	CGameInstance::GetInstance()->PlaySounds(const_cast<TCHAR*>(L"LeeSinWSound2.wav"), SOUND_PLAYER, 0.4f);
	CGameInstance::GetInstance()->PlaySounds(const_cast<TCHAR*>(L"527148964.wav"), SOUND_PLAYER, 0.4f);
	// ★ 강화 시작 FX (양손)
	Play_WEmpowerEffect(TEXT("LeeSinAttackUp.dat"));
}

bool CLeeSin::Consume_WEmpowerStack()
{
	if (!m_bWEmpower || m_iWEmpowerStacks <= 0)
		return false;

	m_iWEmpowerStacks--;

	if (m_iWEmpowerStacks <= 0)
	{
		m_bWEmpower = false;
		m_fWEmpowerAcc = 0.f;
		m_fWEmpowerDuration = 0.f;

		// ★ 강화 종료 FX 제거
		Clear_WEmpowerEffect();
	}
	return true;
}

void CLeeSin::Start_ERecast(float window)
{
	m_bERecastReady = true;
	m_fERecastAcc = 0.f;
	m_fERecastWindow = window;
	Update_SkillIcons();
}

void CLeeSin::Consume_ERecast()
{
	m_bERecastReady = false;
	m_fERecastAcc = 0.f;
	Update_SkillIcons();
}

void CLeeSin::Update_RecastTimers(float dt)
{
	if (m_bWRecastReady)
	{
		m_fWRecastAcc += dt;
		if (m_fWRecastAcc >= m_fWRecastWindow)
			Consume_WRecast(false);   // ? 타이머 만료는 false
	}

	// W2 평타강화 버프 타이머
	if (m_bWEmpower)
	{
		m_fWEmpowerAcc += dt;

		// ★ 손 FX는 매 프레임 본 따라가야 함
		Update_WEmpowerEffect();

		if (m_fWEmpowerAcc >= m_fWEmpowerDuration)
		{
			m_bWEmpower = false;
			m_iWEmpowerStacks = 0;
			m_fWEmpowerAcc = 0.f;
			m_fWEmpowerDuration = 0.f;

			// ★ 만료 시 FX 제거
			Clear_WEmpowerEffect();
		}
	}
	// E 재사용 타이머
	if (m_bERecastReady)
	{
		m_fERecastAcc += dt;

		// ★ 표식 대상이 죽었으면 정리
		Cleanup_EMarkTargets();

		// ★ 표식이 하나도 없으면 E2 자체가 불가능 → 즉시 닫기
		if (m_vecEMarkTargets.empty())
		{
			Consume_ERecast();
			return;
		}

		// 시간 만료되면 닫고 표식도 제거
		if (m_fERecastAcc >= m_fERecastWindow)
		{
			Consume_ERecast();
			Clear_EMarkTargets();
		}
	}
}

void CLeeSin::Update_SkillIcons()
{
	// Q: 표식 기반(기존 Can_QRecast)
	m_Skills[1].IconTag = Can_QRecast()
		? L"Prototype_Component_Texture_LeeSinSkill_Q_2"
		: L"Prototype_Component_Texture_LeeSinSkill_Q_1";

	// W: W1 후 재사용 가능하면 W2
	m_Skills[2].IconTag = Can_WRecast()
		? L"Prototype_Component_Texture_LeeSinSkill_W_2"
		: L"Prototype_Component_Texture_LeeSinSkill_W_1";

	// E: E1 후 재사용 가능하면 E2
	m_Skills[3].IconTag = Can_ERecast()
		? L"Prototype_Component_Texture_LeeSinSkill_E_2"
		: L"Prototype_Component_Texture_LeeSinSkill_E_1";
}

_bool CLeeSin::Picking(_float3* PickingPoint)
{
	CColider* pCollider = Get_Collider();
	if (pCollider == nullptr)
		return false;

	float   fDist = 0.f;
	_float3 vHitPos = {};

	if (pCollider->RayPick(m_pTransformCom, &vHitPos, &fDist))
	{
		if (PickingPoint)
			*PickingPoint = vHitPos;

		return true; // ★ 여기서 절대 커서/타겟 세팅하지 마
	}

	return false;
}

void CLeeSin::PickingTrue()
{
	CChampion* pPlayer = CPlayerManager::GetInstance()->Get_ActivePlayer();
	if (!pPlayer) return;

	// ★ 선택된 "바로 그 오브젝트"가 공격 대상일 때만 여기서 세팅
	CCursorManager::GetInstance()->SetState(CURSOR_ATTACK);
	pPlayer->Set_AttackTarget(this);
}

void CLeeSin::Set_Picked()
{
}


void CLeeSin::SetSkillInfo()
{
	m_ChampionIconTag = L"Prototype_Component_Texture_LeeSin_Icon";
	m_Skills[0] = { L"Rising Spell Force", L"Prototype_Component_Texture_LeeSinSkill_Passive", 0.f };
	m_Skills[1] = { L"Mystic Shot", L"Prototype_Component_Texture_LeeSinSkill_Q_1", 0.f };
	m_Skills[2] = { L"Essence Flux", L"Prototype_Component_Texture_LeeSinSkill_W_1", 0.f };
	m_Skills[3] = { L"Arcane Shift", L"Prototype_Component_Texture_LeeSinSkill_E_1", 0.f };
	m_Skills[4] = { L"Trueshot Barrage", L"Prototype_Component_Texture_LeeSinSkill_R", 0.f };
}

HRESULT CLeeSin::Ready_Components(void* pArg)
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
	if (FAILED(__super::Add_Components(TEXT("Com_Shader"), LEVEL_STATIC, TEXT("Prototype_Component_Shader_VtxAnimModel"), (CComponent**)&m_pShaderCom)))
		return E_FAIL;

	/* For.Com_Model*/
	if (FAILED(__super::Add_Components(TEXT("Com_Model"), LEVEL_GAMEPLAY, TEXT("LeeSin"), (CComponent**)&m_pModelCom)))
		return E_FAIL;

	CColider::COLLIDERDESC ColliderDesc;
	ZeroMemory(&ColliderDesc, sizeof(CColider::COLLIDERDESC));
	ColliderDesc.vScale = _float3(1.2f, 1.2f, 1.2f);
	ColliderDesc.vRotation = _float3(0.f, 0.f, 0.f);
	ColliderDesc.vPosition = _float3(0.f, 0.5f, 0.f);
	if (FAILED(__super::Add_Components(TEXT("Com_SPHERE"), LEVEL_STATIC, TEXT("Prototype_Component_Colider_Sphere"), (CComponent**)&m_pSphereCom, &ColliderDesc)))
		return E_FAIL;

	return S_OK;
}

void CLeeSin::Set_QMarkTarget(CActor* pTarget)
{
	m_pQMarkTarget = pTarget;

	// Q2 아이콘으로 변경
	m_Skills[1].IconTag = L"Prototype_Component_Texture_LeeSinSkill_Q_2";
}

void CLeeSin::Clear_QMarkTarget()
{
	m_pQMarkTarget = nullptr;

	// Q1 아이콘으로 복귀
	m_Skills[1].IconTag = L"Prototype_Component_Texture_LeeSinSkill_Q_1";
}

bool CLeeSin::Can_QRecast() const
{
	if (!m_pQMarkTarget) return false;
	if (m_pQMarkTarget->Get_Dead()) return false;

	if (!m_pQMarkTarget->Has_Mark(CActor::MARK_TYPE::LEE_SIN_Q))
		return false;

	// 내가 묻힌 표식만 인정
	return (m_pQMarkTarget->Get_MarkCaster(CActor::MARK_TYPE::LEE_SIN_Q) == (CActor*)this);
}

void CLeeSin::Apply_WShield(float fShieldHP, float fDuration)
{
	m_fWShieldHP = max(0.f, fShieldHP);
	m_fWShieldDuration = max(0.01f, fDuration);
	m_fWShieldAcc = 0.f;
}

// =========================
// W 실드 시간 갱신
// =========================
void CLeeSin::Update_WShield(_float dt)
{
	if (m_fWShieldHP <= 0.f) return;

	m_fWShieldAcc += dt;
	if (m_fWShieldAcc >= m_fWShieldDuration)
	{
		m_fWShieldHP = 0.f;
		m_fWShieldAcc = 0.f;
		m_fWShieldDuration = 0.f;
		Clear_WShieldEffect();
		Clear_WDownEffect();   // ★ 추가
		return;
	}

	_matrix world = Get_Transform()->Get_WorldMatrix();
	_vector vPos = Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);

	vPos += XMVectorSet(0.f, 1.f, 0.f, 0.f); // ★ 추가
	vPos = XMVectorSetW(vPos, 1.f);
	world.r[3] = vPos;

	for (auto& fx : m_vecWShieldFx)
	{
		if (!fx) continue;
		fx->Set_WorldPosition(world);
	}
	Update_WDownEffect();
}

// =========================
// 데미지 처리 (실드 먼저)
// =========================
_int CLeeSin::Take_Damage(int iDamage, CActor* DamageCauser)
{
	int remain = iDamage;

	if (m_fWShieldHP > 0.f && remain > 0)
	{
		float blocked = min(m_fWShieldHP, (float)remain);
		m_fWShieldHP -= blocked;
		remain -= (int)blocked;

		if (m_fWShieldHP <= 0.f)
		{
			m_fWShieldHP = 0.f;
			m_fWShieldAcc = 0.f;
			m_fWShieldDuration = 0.f;

			Clear_WShieldEffect();
			Clear_WDownEffect();   // ★ 추가
		}
	}

	if (remain <= 0)
		return 0;

	return __super::Take_Damage(remain, DamageCauser);
}

_matrix CLeeSin::Get_BoneWorldMatrix(const char* pBoneName) const
{
	// const 함수에서 Get_Transform() 호출하면 E1086 뜸 -> 컴포넌트 직접 사용
	CTransform* pTr = m_pTransformCom;
	_matrix world = pTr ? pTr->Get_WorldMatrix() : XMMatrixIdentity();

	if (!m_pModelCom || !pBoneName)
		return world;

	CBone* pBone = m_pModelCom->Get_BonePtr(pBoneName);
	if (!pBone)
		return world;

	_float4x4 pivot = m_pModelCom->Get_PivotFloat4x4();
	_matrix pivotMat = XMLoadFloat4x4(&pivot);

	_matrix boneWorld =
		pBone->Get_CombinedTransformationMatrix() *
		pivotMat *
		world;

	// (선택) 축 정규화
	boneWorld.r[0] = XMVector4Normalize(boneWorld.r[0]);
	boneWorld.r[1] = XMVector4Normalize(boneWorld.r[1]);
	boneWorld.r[2] = XMVector4Normalize(boneWorld.r[2]);

	return boneWorld;
}

void CLeeSin::Clear_WEmpowerEffect()
{
	for (auto& it : m_vecWEmpowerFx)
	{
		if (!it.pFx) continue;

		it.pFx->Set_AttachObject(nullptr);
		it.pFx->Set_Dead(true);
		it.pFx = nullptr;
	}

	m_vecWEmpowerFx.clear();
}

void CLeeSin::Play_WEmpowerEffect(const _tchar* pDat)
{
	if (!pDat) return;

	Clear_WEmpowerEffect();

	// "본 위치만 따라가고 회전은 고정" 월드 만들기
	auto BuildNoBoneRotWorld = [&](const _vector& vPosW) -> _matrix
		{
			// 1) 회전 완전 고정(월드축 고정) -> 가장 확실히 "회전 안함"
			_matrix m = XMMatrixIdentity();
			m.r[3] = XMVectorSetW(vPosW, 1.f);
			return m;
		};

	auto SpawnOnBone = [&](const char* boneName)
		{
			// 본 월드에서 "위치만" 뽑기
			_matrix boneW = Get_BoneWorldMatrix(boneName);
			_vector vPosW = boneW.r[3];

			// 회전 고정 월드
			_matrix w = BuildNoBoneRotWorld(vPosW);

			auto fxList = CEffect::PlayEffectAtLocation(pDat, w);
			for (auto* fx : fxList)
			{
				if (!fx) continue;

				// ★ 여기서 AttachObject가 회전을 또 건드리는 엔진이면
				//   아래 줄을 nullptr로 바꿔(회전 절대 못 건드리게)
				fx->Set_AttachObject(this);

				fx->Set_WorldPosition(w);
				m_vecWEmpowerFx.push_back({ boneName, fx });
			}
		};

	SpawnOnBone("L_Hand");
	SpawnOnBone("R_Hand");
}

void CLeeSin::Update_WEmpowerEffect()
{
	if (!m_bWEmpower)
		return;

	auto BuildNoBoneRotWorld = [&](const _vector& vPosW) -> _matrix
		{
			// 1) 회전 완전 고정(월드축)
			_matrix m = XMMatrixIdentity();
			m.r[3] = XMVectorSetW(vPosW, 1.f);
			return m;
		};

	// 혹시 비었으면 다시 생성(안전장치)
	if (m_vecWEmpowerFx.empty())
		Play_WEmpowerEffect(TEXT("LeeSinAttackUp.dat"));

	for (auto& it : m_vecWEmpowerFx)
	{
		if (!it.pFx) continue;

		_matrix boneW = Get_BoneWorldMatrix(it.Bone.c_str());
		_vector vPosW = boneW.r[3];

		_matrix w = BuildNoBoneRotWorld(vPosW);
		it.pFx->Set_WorldPosition(w);
	}
}

void CLeeSin::Clear_WDownEffect()
{
	for (auto& fx : m_vecWDownFx)
	{
		if (!fx) continue;
		fx->Set_AttachObject(nullptr);
		fx->Set_Dead(true);
	}
	m_vecWDownFx.clear();
}

void CLeeSin::Play_WDownEffect(const _tchar* pDat)
{
	if (!pDat) return;

	// 이미 살아있으면 재생성 X (W2 눌러도 유지)
	// (죽었거나 nullptr이면 정리 후 재생성)
	m_vecWDownFx.erase(std::remove_if(m_vecWDownFx.begin(), m_vecWDownFx.end(),
		[](CEffect* fx)
		{
			return (fx == nullptr) || fx->Get_Dead();
		}), m_vecWDownFx.end());

	if (!m_vecWDownFx.empty())
		return;

	_matrix world = Get_Transform()->Get_WorldMatrix();
	_vector vPos = Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);

	// 발밑 위치(필요하면 0.02~0.1로 튜닝)
	vPos += XMVectorSet(0.f, 0.05f, 0.f, 0.f);
	vPos = XMVectorSetW(vPos, 1.f);
	world.r[3] = vPos;

	auto fxList = CEffect::PlayEffectAtLocation(pDat, world);
	for (auto* fx : fxList)
	{
		if (!fx) continue;

		// "리신에 붙어서" 따라가게
		fx->Set_AttachObject(this);
		fx->Set_WorldPosition(world);

		m_vecWDownFx.push_back(fx);
	}
}

void CLeeSin::Update_WDownEffect()
{
	// 실드가 살아있는 동안만 따라가게(원하면 조건 제거 가능)
	if (m_fWShieldHP <= 0.f)
		return;

	_matrix world = Get_Transform()->Get_WorldMatrix();
	_vector vPos = Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);

	vPos += XMVectorSet(0.f, 0.05f, 0.f, 0.f);
	vPos = XMVectorSetW(vPos, 1.f);
	world.r[3] = vPos;

	// 죽은 건 정리
	m_vecWDownFx.erase(std::remove_if(m_vecWDownFx.begin(), m_vecWDownFx.end(),
		[](CEffect* fx)
		{
			return (fx == nullptr) || fx->Get_Dead();
		}), m_vecWDownFx.end());

	for (auto& fx : m_vecWDownFx)
	{
		if (!fx) continue;
		fx->Set_WorldPosition(world);
	}
}

CLeeSin* CLeeSin::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CLeeSin* pInstance = new CLeeSin(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CLeeSin");
		Safe_Release(pInstance);
	}
	return pInstance;
}

CGameObject* CLeeSin::Clone(void* pArg)
{
	CLeeSin* pInstance = new CLeeSin(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CLeeSin");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CLeeSin::Free()
{
	Clear_WShieldEffect();
	Clear_WDownEffect();      // ★ 추가
	Clear_WEmpowerEffect();   // (E용으로 쓸 거면 나중에 이름/로직 바꾸면 됨)
	__super::Free();
	
}