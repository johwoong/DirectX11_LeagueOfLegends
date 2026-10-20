#include "Actor.h"
#include "GameInstance.h"
#include "Effect.h"

CActor::CActor(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CGameObject(pDevice, pContext)
{
}

CActor::CActor(const CActor& rhs)
	: CGameObject(rhs)
{

}

HRESULT CActor::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CActor::Initialize(void* pArg)
{
	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	return S_OK;
}

int CActor::Update(_float fTimeDelta)
{
	int iEvent = __super::Update(fTimeDelta);

	if (m_bDead)
	{
		// 죽으면 남아있는 상태 정리
		Clear_TeemoPoison();
		Clear_TeemoBlind();
		return OBJ_DEAD;
	}

	// ? Teemo 독 DOT
	Update_TeemoPoison(fTimeDelta);

	// ? Teemo 실명
	Update_TeemoBlind(fTimeDelta);

	// ? 기존 마크(이즈W/베인스택 등)
	Update_Mark(fTimeDelta);

	return OBJ_NOEVENT;
}

void CActor::Late_Update(_float fTimeDelta)
{
	__super::Late_Update(fTimeDelta);

#ifdef _DEBUG
	if (m_pAABBCom != nullptr)
		m_pRendererCom->Add_Debug(m_pAABBCom);
	if (m_pOBBCom != nullptr)
		m_pRendererCom->Add_Debug(m_pOBBCom);
	if (m_pSphereCom != nullptr)
		m_pRendererCom->Add_Debug(m_pSphereCom);
#endif
}

HRESULT CActor::Render()
{
	return S_OK;
}

_bool CActor::Is_Frustum(_float fOffset)
{
	_float3 vScale = Get_Scale();
	_float fCullingRadius = max(max(vScale.x, vScale.y), vScale.z);
	return m_pGameInstance->isIn_WorldFrustum(m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION), fCullingRadius + fOffset);
}

void CActor::Apply_TeemoPoison(CActor* pCaster, float fDuration, int iDamagePerTick, float fTickInterval)
{
	// 이미 걸려있으면 "시간만 리셋" (스택X)
	if (m_tTeemoPoison.bActive)
	{
		m_tTeemoPoison.fAcc = 0.f;
		m_tTeemoPoison.fTickAcc = 0.f;
		m_tTeemoPoison.fDuration = fDuration;
		return;
	}

	m_tTeemoPoison.bActive = true;
	m_tTeemoPoison.fAcc = 0.f;
	m_tTeemoPoison.fDuration = fDuration;
	m_tTeemoPoison.fTickAcc = 0.f;
	m_tTeemoPoison.fTickInterval = (fTickInterval <= 0.f) ? 1.f : fTickInterval;
	m_tTeemoPoison.iDamagePerTick = iDamagePerTick;
	m_tTeemoPoison.pCaster = pCaster;

	// =========================
	// ? 독 버프 이펙트 생성 (1회)
	// =========================
	const _tchar* pPoisonDat = TEXT("Temoo_Poision.dat"); // 너 툴에서 만들 파일명
	_vector vPos = Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
	vPos += XMVectorSet(0.f, 1.0f, 0.f, 0.f);
	vPos = XMVectorSetW(vPos, 1.f);

	_matrix mWorld = XMMatrixIdentity();
	mWorld.r[3] = vPos;

	m_vecTeemoPoisonFX = CEffect::PlayEffectAtLocation(pPoisonDat, mWorld);
	for (auto& fx : m_vecTeemoPoisonFX)
		if (fx) fx->Set_AttachObject(this);
}

void CActor::Update_TeemoPoison(float dt)
{
	if (!m_tTeemoPoison.bActive)
		return;

	if (m_bDead)
	{
		Clear_TeemoPoison();
		return;
	}

	// ? 독 이펙트 위치 추적(대상 따라가게)
	if (!m_vecTeemoPoisonFX.empty())
	{
		_vector vPos = Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
		vPos += XMVectorSet(0.f, 1.0f, 0.f, 0.f);
		vPos = XMVectorSetW(vPos, 1.f);

		_matrix mWorld = XMMatrixIdentity();
		mWorld.r[3] = vPos;

		for (auto& fx : m_vecTeemoPoisonFX)
			if (fx) fx->Set_WorldPosition(mWorld);
	}

	m_tTeemoPoison.fAcc += dt;
	m_tTeemoPoison.fTickAcc += dt;

	while (m_tTeemoPoison.fTickAcc >= m_tTeemoPoison.fTickInterval)
	{
		m_tTeemoPoison.fTickAcc -= m_tTeemoPoison.fTickInterval;

		if (m_tTeemoPoison.iDamagePerTick > 0)
			Take_Damage(m_tTeemoPoison.iDamagePerTick, m_tTeemoPoison.pCaster);

		if (m_bDead)
		{
			Clear_TeemoPoison();
			return;
		}
	}

	if (m_tTeemoPoison.fAcc >= m_tTeemoPoison.fDuration)
	{
		Clear_TeemoPoison();
		return;
	}
}

void CActor::Clear_TeemoPoison()
{
	for (auto& fx : m_vecTeemoPoisonFX)
		if (fx) { fx->Set_AttachObject(nullptr); fx->Set_Dead(true); }
	m_vecTeemoPoisonFX.clear();
	m_tTeemoPoison = TEEMO_POISON_DATA{};
}

void CActor::SetUp_BillBoard()
{
	_float4x4 ViewMatrix = CGameInstance::GetInstance()->Get_TransformFloat4x4(CPipeLine::D3DTS_VIEW);
	_matrix matViewInv = XMMatrixInverse(nullptr, XMLoadFloat4x4(&ViewMatrix));

	_vector vRight = XMVector3Normalize(matViewInv.r[0]);
	_vector vUp = XMVector3Normalize(matViewInv.r[1]);
	_vector vLook = XMVector3Normalize(matViewInv.r[2]);

	// HP Bar는 항상 카메라를 향하고, 가로로 눕는다.
	m_pTransformCom->Set_State(CTransform::STATE_RIGHT, vRight * Get_Scale().x);
	m_pTransformCom->Set_State(CTransform::STATE_UP, vUp * Get_Scale().y);
	m_pTransformCom->Set_State(CTransform::STATE_LOOK, vLook * Get_Scale().z);
}

void CActor::Update_Collider()
{
	if (m_pAABBCom != nullptr)
		m_pAABBCom->Update(m_pTransformCom->Get_WorldMatrix());
	if (m_pOBBCom != nullptr)
		m_pOBBCom->Update(m_pTransformCom->Get_WorldMatrix());
	if (m_pSphereCom != nullptr)
		m_pSphereCom->Update(m_pTransformCom->Get_WorldMatrix());
}

void CActor::Apply_Mark(MARK_TYPE eType, CActor* pCaster, float fDuration,
	const _tchar* pMarkDat, const _float3& vOffset)
{
	Clear_Mark();

	m_tMark.bActive = true;
	m_tMark.eType = eType;
	m_tMark.fAcc = 0.f;
	m_tMark.fDuration = fDuration;
	m_tMark.pCaster = pCaster;
	m_tMark.vOffset = vOffset;

	// ===== (1) 메인 마크 FX =====
	if (pMarkDat)
	{
		_vector vPos = Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
		vPos += XMVectorSet(vOffset.x, vOffset.y, vOffset.z, 0.f);
		vPos = XMVectorSetW(vPos, 1.f);

		_matrix world = Get_Transform()->Get_WorldMatrix();
		world.r[3] = vPos;

		auto fxList = CEffect::PlayEffectAtLocation(pMarkDat, world);
		for (auto& fx : fxList)
		{
			if (!fx) continue;

			// 기존대로 붙여도 되고(회전/추적 필요하면)
			fx->Set_AttachObject(this);
			fx->Set_WorldPosition(world);
			m_tMark.fx.push_back(fx);
		}
	}

	// ===== (2) 리신 Q면 QDown도 마크에 종속으로 생성 =====
	if (eType == MARK_TYPE::LEE_SIN_Q)
	{
		// 너가 원하는 발밑 높이로 튜닝
		m_tMark.vDownOffset = _float3(0.f, 0.03f, 0.f);

		_vector vDown = Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
		vDown += XMVectorSet(m_tMark.vDownOffset.x, m_tMark.vDownOffset.y, m_tMark.vDownOffset.z, 0.f);
		vDown = XMVectorSetW(vDown, 1.f);

		_matrix downWorld = XMMatrixIdentity();   // ★ 회전 없이 위치만
		downWorld.r[3] = vDown;

		const _tchar* pDownDat = TEXT("LeeSin_QMark_Down.dat");
		auto downList = CEffect::PlayEffectAtLocation(pDownDat, downWorld);

		for (auto& fx : downList)
		{
			if (!fx) continue;

			// 바닥 FX는 보통 Attach 안 하는 게 안전(회전 섞임/누수 방지)
			// fx->Set_AttachObject(this);

			fx->Set_WorldPosition(downWorld);

			m_tMark.fxDown.push_back(fx);
		}
	}
}

bool CActor::Has_Mark(MARK_TYPE type) const
{
	return m_tMark.bActive && m_tMark.eType == type;
}

bool CActor::Pop_EzrealW_Mark(CActor* pDamageCauser, int iPopDamage, const _tchar* pPopDat)
{
	if (!Has_Mark(MARK_TYPE::EZREAL_W))
		return false;


	// 1) 터지는 이펙트 재생 
	if (pPopDat)
	{
		_vector vPos = Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
		vPos += XMVectorSet(m_tMark.vOffset.x, m_tMark.vOffset.y, m_tMark.vOffset.z, 0.f);
		vPos = XMVectorSetW(vPos, 1.f);

		_matrix world = Get_Transform()->Get_WorldMatrix();
		world.r[3] = vPos;
		m_pGameInstance->PlaySounds(const_cast<TCHAR*>(L"EzrealWBurstSound.wav"), SOUND_EFFECT, g_fSoundVolume);
		CEffect::PlayEffectAtLocation(pPopDat, world);
	}

	Clear_Mark();
	return true;
}

_vector CActor::Get_TransformState(CTransform::STATE eState)
{
	if (m_pTransformCom == nullptr)
		return _vector();

	if (eState == CTransform::STATE_TRANSLATION)
		return XMVectorSetW(m_pTransformCom->Get_State(eState), 1.f);

	return m_pTransformCom->Get_State(eState);
}

void CActor::Set_State(CTransform::STATE eState, _fvector vState)
{
	if (m_pTransformCom == nullptr)
		return;

	m_pTransformCom->Set_State(eState, vState);
}

void CActor::Set_Scale(_float3 vScale)
{
	m_vScale = vScale;

	m_pTransformCom->Set_Scale(CTransform::STATE_RIGHT, m_vScale.x);
	m_pTransformCom->Set_Scale(CTransform::STATE_UP, m_vScale.y);
	m_pTransformCom->Set_Scale(CTransform::STATE_LOOK, m_vScale.z);
}

CColider* CActor::Get_Collider()
{
	if (m_pAABBCom != nullptr)
		return m_pAABBCom;
	if (m_pOBBCom != nullptr)
		return m_pOBBCom;
	if (m_pSphereCom != nullptr)
		return m_pSphereCom;
	else
		return nullptr;
}

CActor* CActor::Get_MarkCaster(MARK_TYPE type) const
{
	if (!m_tMark.bActive) return nullptr;
	if (m_tMark.eType != type) return nullptr;
	return m_tMark.pCaster;
}

void CActor::Remove_Mark(MARK_TYPE type)
{
	if (!m_tMark.bActive) return;
	if (m_tMark.eType != type) return;
	Clear_Mark();
}

void CActor::Update_Mark(float dt)
{
	if (!m_tMark.bActive)
		return;

	// ★ 먼저: 죽었거나 PreDead인 FX는 바로 빼줘야 Set_WorldPosition에서 안 터짐
	auto PruneFx = [](std::vector<CEffect*>& v)
		{
			for (auto& fx : v)
			{
				if (!fx) continue;

				// PreDead/Dead면 더 이상 만지지 말고 참조 해제
				if (fx->Get_PreDead() || fx->Get_Dead())
				{
					fx->Set_AttachObject(nullptr);
					fx = nullptr;
				}
			}
			v.erase(std::remove(v.begin(), v.end(), nullptr), v.end());
		};

	PruneFx(m_tMark.fx);
	PruneFx(m_tMark.fxDown);

	m_tMark.fAcc += dt;

	// ===== 시간 만료 처리 =====
	if (m_tMark.eType == MARK_TYPE::VAYNE_SILVER &&
		m_tMark.fAcc >= m_tMark.fDuration)
	{
		Clear_Mark();
		return;
	}

	if (m_tMark.eType == MARK_TYPE::LEE_SIN_Q &&
		m_tMark.fAcc >= m_tMark.fDuration)
	{
		Clear_Mark();
		return;
	}

	// ===== 위치 추적 =====
	_vector vPos = Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
	vPos += XMVectorSet(m_tMark.vOffset.x, m_tMark.vOffset.y, m_tMark.vOffset.z, 0.f);
	vPos = XMVectorSetW(vPos, 1.f);

	_matrix world = Get_Transform()->Get_WorldMatrix();
	world.r[3] = vPos;

	for (auto& fx : m_tMark.fx)
		if (fx) fx->Set_WorldPosition(world);

	// ===== QDown 위치 추적(리신Q만) =====
	if (m_tMark.eType == MARK_TYPE::LEE_SIN_Q)
	{
		_vector vDown = Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
		vDown += XMVectorSet(m_tMark.vDownOffset.x, m_tMark.vDownOffset.y, m_tMark.vDownOffset.z, 0.f);
		vDown = XMVectorSetW(vDown, 1.f);

		_matrix downWorld = XMMatrixIdentity();
		downWorld.r[3] = vDown;

		for (auto& fx : m_tMark.fxDown)
			if (fx) fx->Set_WorldPosition(downWorld);
	}
}

void CActor::Apply_VayneSilverMark(CActor* pCaster)
{
	// ===== 이미 스택 있음 =====
	if (Has_Mark(MARK_TYPE::VAYNE_SILVER))
	{
		m_tMark.iHitCount++;
		m_tMark.fAcc = 0.f; // 시간 리셋

		// 기존 스택 FX 제거
		for (auto& fx : m_tMark.fx)
			if (fx) fx->Set_Dead(true);
		m_tMark.fx.clear();

		_vector vPos = Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
		vPos += XMVectorSet(0.f, 1.1f, 0.f, 0.f);
		vPos = XMVectorSetW(vPos, 1.f);

		_matrix world = Get_Transform()->Get_WorldMatrix();
		world.r[3] = vPos;

		// ===== 3타 =====
		if (m_tMark.iHitCount >= 3)
		{
			Take_Damage(60, pCaster);
			m_pGameInstance->PlaySounds(const_cast<TCHAR*>(L"VayneStackBurstSound.wav"), SOUND_EFFECT, g_fSoundVolume);
			auto burstFx = CEffect::PlayEffectAtLocation(TEXT("Vayne_WBurst.dat"), world);
			for (auto& fx : burstFx)
			{
				if (!fx) continue;

				// ? 절대 Attach 하지 마라
				fx->Set_WorldPosition(world);   // 위치만 한 번 고정
			}

			// ?? 마크만 정리
			Clear_Mark();
			return;
		}

		// ===== 2타 =====
		auto fx = CEffect::PlayEffectAtLocation(TEXT("Vayne_WStack.dat"), world);
		for (auto& e : fx)
		{
			if (!e) continue;
			e->Set_AttachObject(this);      // ??? 핵심
			m_tMark.fx.push_back(e);
		}

		return;
	}

	// ===== 첫 타 =====
	Apply_Mark(
		MARK_TYPE::VAYNE_SILVER,
		pCaster,
		3.f,            // 스택 유지 시간
		nullptr,        // FX는 직접 처리
		{ 0.f, 1.1f, 0.f }
	);

	m_tMark.iHitCount = 1;

	_vector vPos = Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
	vPos += XMVectorSet(0.f, 1.1f, 0.f, 0.f);
	vPos = XMVectorSetW(vPos, 1.f);

	_matrix world = Get_Transform()->Get_WorldMatrix();
	world.r[3] = vPos;

	auto fx = CEffect::PlayEffectAtLocation(TEXT("Vayne_W.dat"), world);
	for (auto& e : fx)
	{
		if (!e) continue;
		e->Set_AttachObject(this);
		e->Set_WorldPosition(world);   // ?
		m_tMark.fx.push_back(e);
	}

}

void CActor::Apply_TeemoBlind(CActor* pCaster, float fDuration)
{
	// 이미 걸려있으면 시간만 리셋
	if (m_tTeemoBlind.bActive)
	{
		m_tTeemoBlind.fAcc = 0.f;
		m_tTeemoBlind.fDuration = fDuration;
		m_tTeemoBlind.pCaster = pCaster;
		return;
	}

	m_tTeemoBlind.bActive = true;
	m_tTeemoBlind.fAcc = 0.f;
	m_tTeemoBlind.fDuration = fDuration;
	m_tTeemoBlind.pCaster = pCaster;

	// ? 실명 버프 FX (대상 머리 위에 붙이기)
	const _tchar* pBlindDat = TEXT("Temoo_Blind.dat"); // 네 툴에서 만들 것
	_vector vPos = Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
	vPos += XMVectorSet(0.f, 1.15f, 0.f, 0.f);
	vPos = XMVectorSetW(vPos, 1.f);

	_matrix mWorld = XMMatrixIdentity();
	mWorld.r[3] = vPos;

	m_vecTeemoBlindFX = CEffect::PlayEffectAtLocation(pBlindDat, mWorld);
	for (auto& fx : m_vecTeemoBlindFX)
		if (fx) fx->Set_AttachObject(this);
}

void CActor::Update_TeemoBlind(float dt)
{
	if (!m_tTeemoBlind.bActive)
		return;

	if (m_bDead)
	{
		Clear_TeemoBlind();
		return;
	}

	// ? FX 위치 추적
	if (!m_vecTeemoBlindFX.empty())
	{
		_vector vPos = Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
		vPos += XMVectorSet(0.f, 1.15f, 0.f, 0.f);
		vPos = XMVectorSetW(vPos, 1.f);

		_matrix mWorld = XMMatrixIdentity();
		mWorld.r[3] = vPos;

		for (auto& fx : m_vecTeemoBlindFX)
			if (fx) fx->Set_WorldPosition(mWorld);
	}

	m_tTeemoBlind.fAcc += dt;

	if (m_tTeemoBlind.fAcc >= m_tTeemoBlind.fDuration)
	{
		Clear_TeemoBlind();
		return;
	}
}

void CActor::Clear_TeemoBlind()
{
	for (auto& fx : m_vecTeemoBlindFX)
		if (fx) { fx->Set_AttachObject(nullptr); fx->Set_Dead(true); }
	m_vecTeemoBlindFX.clear();
	m_tTeemoBlind = TEEMO_BLIND_DATA{};
}

void CActor::Clear_Mark()
{
	// FX는 레이어가 소유/해제한다. Actor는 죽이기만 한다(Release 금지).
	for (auto& fx : m_tMark.fx)
	{
		if (!fx) continue;
		fx->Set_AttachObject(nullptr);
		fx->Set_Dead(true);
		// Safe_Release(fx); // ? 금지 (레이어가 Release 함)
		fx = nullptr;
	}
	m_tMark.fx.clear();

	for (auto& fx : m_tMark.fxDown)
	{
		if (!fx) continue;
		fx->Set_AttachObject(nullptr);
		fx->Set_Dead(true);
		// Safe_Release(fx); // ? 금지
		fx = nullptr;
	}
	m_tMark.fxDown.clear();

	m_tMark = MARK_DATA{};
}

void CActor::Free()
{
	__super::Free();
}