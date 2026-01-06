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
		return OBJ_DEAD; 
	// ? 마크(이즈W/베인스택) 위치 갱신은 매 프레임 돌려야 함
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

	if (!pMarkDat)
		return;

	_vector vPos = Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
	vPos += XMVectorSet(vOffset.x, vOffset.y, vOffset.z, 0.f);
	vPos = XMVectorSetW(vPos, 1.f);

	_matrix world = Get_Transform()->Get_WorldMatrix();
	world.r[3] = vPos;

	auto fxList = CEffect::PlayEffectAtLocation(pMarkDat, world);

	for (auto& fx : fxList)
	{
		if (!fx) continue;

		fx->Set_AttachObject(this);   // ? 이거 추가
		m_tMark.fx.push_back(fx);
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

void CActor::Update_Mark(float dt)
{
	if (!m_tMark.bActive)
		return;

	m_tMark.fAcc += dt;

	// ===== 베인 스택 시간 초과 =====
	if (m_tMark.eType == MARK_TYPE::VAYNE_SILVER &&
		m_tMark.fAcc >= m_tMark.fDuration)
	{
		Clear_Mark();   // 스택 리셋
		return;
	}

	// 기존 위치 추적
	_vector vPos = Get_Transform()->Get_State(CTransform::STATE_TRANSLATION);
	vPos += XMVectorSet(m_tMark.vOffset.x, m_tMark.vOffset.y, m_tMark.vOffset.z, 0.f);
	vPos = XMVectorSetW(vPos, 1.f);

	_matrix world = Get_Transform()->Get_WorldMatrix();
	world.r[3] = vPos;

	for (auto& fx : m_tMark.fx)
		if (fx) fx->Set_WorldPosition(world);
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

			// ?? Burst FX는 "마크와 분리"해서 붙인다
			auto burstFx = CEffect::PlayEffectAtLocation(TEXT("Vayne_WBurst.dat"), world);
			for (auto& fx : burstFx)
			{
				if (!fx) continue;
				fx->Set_AttachObject(this);   // 따라가긴 함
				// ? m_tMark.fx에 절대 넣지 마라
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

void CActor::Clear_Mark()
{
	if (!m_tMark.bActive)
		return;

	for (auto& fx : m_tMark.fx)
		if (fx) fx->Set_Dead(true);
	m_tMark.fx.clear();

	m_tMark = MARK_DATA{};
}

void CActor::Free()
{
	__super::Free();
	Safe_Release(m_pRendererCom);
}