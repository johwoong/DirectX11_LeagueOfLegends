#include "Champion.h"

#include "GameInstance.h"
#include "ParticleSystem.h"
#include "PlayerDeadState.h"
#include "PlayerState.h"
#include "Collision_Manager.h"
#include "PlayerIdleState.h"
CChampion::CChampion(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CPlayer(pDevice, pContext)
{

}

CChampion::CChampion(const CChampion& rhs)
	: CPlayer(rhs)
{

}

HRESULT CChampion::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CChampion::Initialize(void* pArg)
{
	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	Ready_Animation_Event();

	return S_OK;
}

int CChampion::Update(_float fTimeDelta)
{
	int iEvent = __super::Update(fTimeDelta);
	if (iEvent == OBJ_DEAD)
		return OBJ_DEAD;

	if (m_fMoveSpeedBuffRemain > 0.f)
	{
		m_fMoveSpeedBuffRemain -= fTimeDelta;

		if (m_fMoveSpeedBuffRemain <= 0.f)
		{
			m_fMoveSpeedBuffRemain = 0.f;
			m_fMoveSpeedMul = 1.f;

			// ? 버프 끝나면 FX도 같이 제거
			Kill_MoveSpeedBuffFX();
		}
		else
		{
			// ? 버프 유지 중엔 계속 따라가게 갱신
			Update_MoveSpeedBuffFX();
		}
	}

	return OBJ_NOEVENT;
}

void CChampion::Late_Update(_float fTimeDelta)
{
	__super::Late_Update(fTimeDelta);
}

HRESULT CChampion::Render()
{
	if (nullptr == m_pShaderCom || nullptr == m_pModelCom)
		return E_FAIL;

	if (FAILED(SetUp_ShaderResources()))
		return E_FAIL;

	/* Rim Light */
	Get_AuraColor();
	if (FAILED(m_pShaderCom->Set_RawValue("g_bRimLight", &m_bOverLimit, sizeof(_bool))))
		return E_FAIL;
	if (FAILED(m_pShaderCom->Set_RawValue("g_vRimColor", &m_vAuraColor, sizeof(_float3))))
		return E_FAIL;
	if (FAILED(m_pShaderCom->Set_RawValue("g_vRimTimer", &m_fFresnelTimer, sizeof(_float))))
		return E_FAIL;

	_float4 vCameraLook = (_float4)(CGameInstance::GetInstance()->Get_CamWorldMatrix().m[2]);
	_float3 vCamLook = _float3(vCameraLook.x, vCameraLook.y, vCameraLook.z);

	if (FAILED(m_pShaderCom->Set_RawValue("g_vCameraLook", &vCamLook, sizeof(_float3))))
		return E_FAIL;

	_uint iNumMeshes = m_pModelCom->Get_NumMeshs();

	for (_uint i = 0; i < iNumMeshes; ++i)
	{
		if (FAILED(m_pModelCom->SetUp_Material(m_pShaderCom, "g_DiffuseTexture", i, aiTextureType_DIFFUSE)))
			return E_FAIL;

		if (FAILED(m_pModelCom->SetUp_Material(m_pShaderCom, "g_NormalTexture", i, aiTextureType_NORMALS)))
			return E_FAIL;

		if (FAILED(m_pModelCom->Render(m_pShaderCom, i, 0)))
			return E_FAIL;
	}

	_bool bRimLight = false;
	if (FAILED(m_pShaderCom->Set_RawValue("g_bRimLight", &bRimLight, sizeof(_bool))))
		return E_FAIL;

	return S_OK;
}

_bool CChampion::Picking(_float3* PickingPoint)
{
	CColider* pCollider = Get_Collider();
	if (!pCollider || !m_pTransformCom)
		return false;

	float   fDist = 0.f;
	_float3 vHitPos = {};

	if (pCollider->RayPick(m_pTransformCom, &vHitPos, &fDist))
	{
		if (PickingPoint)
			*PickingPoint = vHitPos;
		return true;
	}

	return false;
}

void CChampion::PickingTrue()
{
}

void CChampion::UpdateLevelStats()
{

}

void CChampion::Kill_MoveSpeedBuffFX()
{
	for (auto& fx : m_MoveSpeedFX)
	{
		if (!fx) continue;

		// ? 리신처럼: Detach -> Dead (AttachObject AddRef 누수 방지)
		fx->Set_AttachObject(nullptr);

		if (auto ps = dynamic_cast<CParticleSystem*>(fx))
			ps->Set_Stop(true);

		fx->Set_Dead(true);
		fx = nullptr;
	}
	m_MoveSpeedFX.clear();
}

_int CChampion::Take_Damage(int iDamage, CActor* DamageCauser)
{
	// 다운(죽음) 상태면 데미지 무시
	if (Is_Down())
		return 0;

	if (iDamage <= 0)
		return 0;

	// HP 감소
	m_tStat.Hp -= (float)iDamage;
	if (m_tStat.Hp < 0.f)
		m_tStat.Hp = 0.f;

	// 죽음 처리
	if (m_tStat.Hp <= 0.f)
	{
		// 다운 상태 ON (삭제는 안 함)
		Set_Down(true);

		// 이동/타겟/스킬 정리
		m_bMoving = false;
		m_pAttackTarget = nullptr;
		SetUsingSkill(false);
		Hide_AttackRange();

		// 도트/마크/버프 이펙트 정리
		Clear_TeemoPoison();
		Clear_TeemoBlind();
		Remove_Mark(MARK_TYPE::EZREAL_W);
		Remove_Mark(MARK_TYPE::LEE_SIN_Q);
		Remove_Mark(MARK_TYPE::LEE_SIN_E);
		Remove_Mark(MARK_TYPE::VAYNE_SILVER);

		// 이동속도 버프 끄면 FX도 같이 꺼지게 해둔 구조라 이걸로 정리 가능
		Apply_MoveSpeedBuff(1.f, 0.f);

		// 충돌 그룹에서 제거 (피격/타겟 방지)
		CCollision_Manager::GetInstance()->Out_CollisionGroup(
			CCollision_Manager::COLLISION_PLAYER, this);

		// DeadState로 전환
		if (m_pPlayerState)
		{
			CPlayerState* pNew = new Player::CPlayerDeadState(this);
			m_pPlayerState = m_pPlayerState->ChangeState(m_pPlayerState, pNew);
		}

		return 1;
	}

	return 0;
}

void CChampion::Revive_Full()
{
	// 풀피
	m_tStat.Hp = m_tStat.MaxHp;

	// 상태 정리
	Set_Down(false);
	m_bMoving = false;
	m_pAttackTarget = nullptr;
	SetUsingSkill(false);

	Clear_TeemoPoison();
	Clear_TeemoBlind();
	Remove_Mark(MARK_TYPE::EZREAL_W);
	Remove_Mark(MARK_TYPE::LEE_SIN_Q);
	Remove_Mark(MARK_TYPE::LEE_SIN_E);
	Remove_Mark(MARK_TYPE::VAYNE_SILVER);

	Apply_MoveSpeedBuff(1.f, 0.f);
}

void CChampion::Play_MoveSpeedBuffFX(const _tchar* dat)
{
	if (!dat || !m_pTransformCom) return;

	Kill_MoveSpeedBuffFX();

	_matrix ownerW = m_pTransformCom->Get_WorldMatrix();

	// ? 로컬축 기반 오프셋 (회전 따라가게)
	_vector vPos = ownerW.r[3];
	vPos += ownerW.r[0] * m_vMoveSpeedFXOffset.x; // Right
	vPos += ownerW.r[1] * m_vMoveSpeedFXOffset.y; // Up
	vPos += ownerW.r[2] * m_vMoveSpeedFXOffset.z; // Look
	vPos = XMVectorSetW(vPos, 1.f);

	ownerW.r[3] = vPos;

	auto fxList = CEffect::PlayEffectAtLocation(dat, ownerW);
	for (auto& fx : fxList)
	{
		if (!fx) continue;

		// ? 붙이되, 월드도 매 프레임 우리가 갱신
		fx->Set_AttachObject(this);
		fx->Set_WorldPosition(ownerW);

		m_MoveSpeedFX.push_back(fx);
	}
}

void CChampion::Update_MoveSpeedBuffFX()
{
	if (m_MoveSpeedFX.empty() || !m_pTransformCom)
		return;

	m_MoveSpeedFX.erase(std::remove_if(m_MoveSpeedFX.begin(), m_MoveSpeedFX.end(),
		[](CEffect* fx)
		{
			return (fx == nullptr) || fx->Get_Dead();
		}), m_MoveSpeedFX.end());

	if (m_MoveSpeedFX.empty())
		return;

	_matrix ownerW = m_pTransformCom->Get_WorldMatrix();

	// ? 로컬축 기반 오프셋 (회전 따라감)
	_vector vPos = ownerW.r[3];
	vPos += ownerW.r[0] * m_vMoveSpeedFXOffset.x;
	vPos += ownerW.r[1] * m_vMoveSpeedFXOffset.y;
	vPos += ownerW.r[2] * m_vMoveSpeedFXOffset.z;
	vPos = XMVectorSetW(vPos, 1.f);

	ownerW.r[3] = vPos;

	for (auto& fx : m_MoveSpeedFX)
	{
		if (!fx) continue;
		fx->Set_WorldPosition(ownerW);
	}
}

void CChampion::Apply_MoveSpeedBuff(_float fMul, _float fDuration)
{
	Apply_MoveSpeedBuff(fMul, fDuration, nullptr);
}

void CChampion::Apply_MoveSpeedBuff(_float fMul, _float fDuration, const _tchar* pFxDat)
{
	m_fMoveSpeedMul = (fMul < 0.f) ? 0.f : fMul;
	m_fMoveSpeedBuffRemain = (fDuration < 0.f) ? 0.f : fDuration;

	// 꺼진 값이면 FX도 종료
	if (m_fMoveSpeedMul <= 1.001f || m_fMoveSpeedBuffRemain <= 0.f)
	{
		m_fMoveSpeedMul = 1.f;
		m_fMoveSpeedBuffRemain = 0.f;
		Kill_MoveSpeedBuffFX();
		return;
	}

	if (pFxDat)
		Play_MoveSpeedBuffFX(pFxDat);
}

HRESULT CChampion::SetUp_ShaderResources()
{
	if (!m_pShaderCom || !m_pTransformCom)
		return E_FAIL;

	_float4x4 WorldMatrix = m_pTransformCom->Get_World_TP();
	_float4x4 ViewMatrix = m_pGameInstance->Get_TransformFloat4x4_TP(CPipeLine::D3DTS_VIEW);
	_float4x4 ProjMatrix = m_pGameInstance->Get_TransformFloat4x4_TP(CPipeLine::D3DTS_PROJ);


	if (FAILED(m_pShaderCom->Set_RawValue("g_WorldMatrix", &WorldMatrix, sizeof(_float4x4))))
		return E_FAIL;

	if (FAILED(m_pShaderCom->Set_RawValue("g_ViewMatrix", &ViewMatrix, sizeof(_float4x4))))
		return E_FAIL;

	if (FAILED(m_pShaderCom->Set_RawValue("g_ProjMatrix", &ProjMatrix, sizeof(_float4x4))))
		return E_FAIL;

	return S_OK;
}

void CChampion::Free()
{
	Kill_MoveSpeedBuffFX();  // ? 누수/잔상 방지
	__super::Free();
}
