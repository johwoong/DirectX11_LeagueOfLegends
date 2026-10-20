#include "PlayerManager.h"
#include "Player.h"
#include "GameInstance.h"

#include "PlayerState.h"
#include "PlayerIdleState.h"
#include "Collision_Manager.h"
#include "LeeSinSkillStateW.h"
#include "Minion.h"
#include "CursorManager.h"

using namespace Player;

CPlayer::CPlayer(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CActor(pDevice, pContext)
{

}

CPlayer::CPlayer(const CPlayer& rhs)
	: CActor(rhs)
{
}

HRESULT CPlayer::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CPlayer::Initialize(void* pArg)
{
	if (FAILED(Ready_Components(pArg)))
		return E_FAIL;

	CNavigation::NAVDESC NaviDesc;
	ZeroMemory(&NaviDesc, sizeof NaviDesc);
	if (FAILED(__super::Add_Components(TEXT("Com_Navigation"), LEVEL_STATIC, TEXT("Prototype_Component_ChampionNavigation"), (CComponent**)&m_pNavigationCom, &NaviDesc)))
		return E_FAIL;

	HANDLE hFile = 0;
	_ulong dwByte = 0;

	NONANIMDESC PlayerDeSC;
	_uint iNum = 0;

	hFile = CreateFile(TEXT("../../../Bin/Data/PositionData/PlayerSpawnPos.dat"), GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	if (0 == hFile)
		return E_FAIL;

	ReadFile(hFile, &(iNum), sizeof(_uint), &dwByte, nullptr);

	for (_uint i = 0; i < iNum; ++i)
	{
		ReadFile(hFile, &(PlayerDeSC), sizeof(NONANIMDESC), &dwByte, nullptr);
	}
	CloseHandle(hFile);

	

	// 1) 스폰 좌표 저장
	m_vSpawnPos = PlayerDeSC.vPosition;

	// 2) 반드시 w = 1.f 로 만들어서 Set_State
	_vector vPos = XMVectorSet(m_vSpawnPos.x, m_vSpawnPos.y, m_vSpawnPos.z, 1.f);
	m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, vPos);

	// 3) 네비 인덱스 & 높이 보정 (있으면)
	m_pNavigationCom->Compute_CurrentIndex_byXZ(vPos);
	Check_Navigation();    // 지형 높이 맞추고 싶으면 여기서

	m_pPlayerState = new Player::CIdleState(this, Client::Player::CIdleState::IDLE_MAIN);
	CCollision_Manager::GetInstance()->Add_CollisionGroup(CCollision_Manager::COLLISION_PLAYER, this);
	return S_OK;
}

int CPlayer::Update(_float fTimeDelta)
{
	int iEvent = __super::Update(fTimeDelta);
	Update_Collider();
	if (iEvent == OBJ_DEAD)
		return OBJ_DEAD;

	HandleInput();
	Update_State(fTimeDelta);
	return OBJ_NOEVENT;
}

void CPlayer::Late_Update(_float fTimeDelta)
{
	if (!m_pRendererCom)
		return;

	m_pRendererCom->Add_RenderGroup(CRenderer::RENDER_NOALPHABLEND, this);
	m_pRendererCom->Add_RenderGroup(CRenderer::RENDER_SHADOWDEPTH, this);
	m_pRendererCom->Add_RenderGroup(CRenderer::RENDER_EDGE_DETECTION, this);

	/* 발밑 접촉 그림자. 그림자 맵만으로는 탑뷰에서 그림자가 몸에 가려 거의 안 보인다. */
	/* 챔피언 3종이 모두 레이어에 있으므로, 현재 조종 중인 챔피언만 설정해야 다른 챔피언 위치로 덮어써지지 않는다. */
	if (CPlayerManager::GetInstance()->Get_ActivePlayer() == dynamic_cast<CChampion*>(this))
	{
		_float3 vFoot;
		XMStoreFloat3(&vFoot, m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION));
		vFoot.x += 0.35f; vFoot.z -= 0.2f;   // 그림자가 떨어지는 쪽(오른쪽 아래)으로 살짝 치우친다
		m_pRendererCom->Set_BlobShadow(vFoot, 1.1f, 0.5f);   // 월드 단위: 1 단위가 화면에서 약 50픽셀
	}
#ifdef _DEBUG
	m_pRendererCom->Add_Debug(m_pNavigationCom);
	__super::Late_Update(fTimeDelta);
#endif
	Late_Update_State(fTimeDelta);
}

HRESULT CPlayer::Render_ShadowDepth()
{
	 if (nullptr == m_pShaderCom || nullptr == m_pModelCom || nullptr == m_pTransformCom)
        return E_FAIL;

    // World (TP)
    _float4x4 WorldTP = m_pTransformCom->Get_World_TP();

    // Light View / Proj (TP)
    // Get_ShadowLightView()는 너 코드상 "Transpose해서 반환" 구조임.
    _float4x4 LightViewTP = m_pGameInstance->Get_ShadowLightView();

    // ★중요: 카메라 Proj 쓰면 안 됨. Shadow Light Proj를 써야 함.
    // 아래 둘 중 네 프로젝트에 있는 걸로 사용해.
    _float4x4 LightProjTP = m_pGameInstance->Get_ShadowLightProj();

    // PlayerShader(Shadow_Depth pass)에서 사용하는 상수는 g_ViewMatrix/g_ProjMatrix다.
    if (FAILED(m_pShaderCom->Set_RawValue("g_WorldMatrix", &WorldTP, sizeof(_float4x4))))
        return E_FAIL;
    if (FAILED(m_pShaderCom->Set_RawValue("g_ViewMatrix", &LightViewTP, sizeof(_float4x4))))
        return E_FAIL;
    if (FAILED(m_pShaderCom->Set_RawValue("g_ProjMatrix", &LightProjTP, sizeof(_float4x4))))
        return E_FAIL;

    const _uint iNumMeshes = m_pModelCom->Get_NumMeshs();
    for (_uint i = 0; i < iNumMeshes; ++i)
    {
        if (FAILED(m_pModelCom->Render(m_pShaderCom, i, SHADER_ANIMSHADOW)))
            return E_FAIL;
    }

    return S_OK;
}

HRESULT CPlayer::Render_EdgeDetection()
{
	if (nullptr == m_pShaderCom || nullptr == m_pModelCom)
		return E_FAIL;

	if (FAILED(SetUp_ShaderResources()))
		return E_FAIL;

	_uint iNumMeshes = m_pModelCom->Get_NumMeshs();
	for (_uint i = 0; i < iNumMeshes; ++i)
	{
		if (FAILED(m_pModelCom->SetUp_Material(m_pShaderCom, "g_DiffuseTexture", i, aiTextureType_DIFFUSE)))
			return E_FAIL;
		if (FAILED(m_pModelCom->Render(m_pShaderCom, i, 5)))
			return E_FAIL;
	}

	return S_OK;
}

void CPlayer::Reset_State()
{
	Safe_Delete(m_pPlayerState);
	m_pPlayerState = new Player::CIdleState(this, Client::Player::CIdleState::IDLE_MAIN);
}

void CPlayer::HandleInput()
{
	CGameInstance* pGI = CGameInstance::GetInstance();

	// ★ A 키 토글 : 활성 챔피언만 처리
	if (pGI->Key_Down(DIK_A))
	{
		CChampion* pChamp = dynamic_cast<CChampion*>(this);
		if (pChamp)
			pChamp->Toggle_AttackRange();
	}

	if (Get_PlayerID() == CPlayer::LEESIN && pGI->Key_Down(DIK_W))
	{
		CLeeSin* pLee = dynamic_cast<CLeeSin*>(this);
		if (!pLee)
			return;

		// -----------------------
		// 1) W2 (재사용) : 공격 강화
		// -----------------------
		if (pLee->Can_WRecast())
		{
			pLee->Consume_WRecast(true);   // ★ 여기 true 필수
			pLee->Apply_WEmpower(2, 3.f);
			return;
		}

		// =========================
		// 2) W1 : "마우스 아래 블루 미니언"만 직접 레이픽킹
		// (PickedObj / Terrain 절대 안 믿음)
		// =========================
		CCursorManager::GetInstance()->Update_Ray(); // 레이 최신화

		CMinion* pBest = nullptr;
		float bestDist = FLT_MAX;

		auto& minionGroup =
			CCollision_Manager::GetInstance()->Get_CollisionGroup(CCollision_Manager::COLLISION_MINION);

		for (auto obj : minionGroup)
		{
			CMinion* m = dynamic_cast<CMinion*>(obj);
			if (!m) continue;

			// 죽은 애 제외 (네 프로젝트에 있는 걸로 하나만 쓰면 됨)
			if (m->Get_Dead()) continue;
			// if (m->IsDeadOrDying()) continue;

			// ★ 블루팀만
			if (m->Get_Team() != TEAM_BLUE) continue;   // 아래 3)에서 Get_Team() 추가

			CColider* col = m->Get_Collider();
			CTransform* tr = m->Get_Transform();
			if (!col || !tr) continue;

			float dist = 0.f;
			_float3 hit{};
			if (col->RayPick(tr, &hit, &dist))
			{
				if (dist < bestDist)
				{
					bestDist = dist;
					pBest = m;
				}
			}
		}

		// (A) 블루 미니언이 찍혔으면 => 대시 W
		if (pBest)
		{
			if (m_pPlayerState)
			{
				CPlayerState* pNew = new Player::CLeeSinSkillStateW(this, pBest);
				m_pPlayerState = m_pPlayerState->ChangeState(m_pPlayerState, pNew);
			}
			return;
		}

		// (B) 블루 미니언이 없으면 => 자기 자신 W(실드)
		pLee->Cast_W_Self(80.f, 2.f, 3.f, TEXT("LeeSin_W.dat"));
		return;
	}

	// =========================================================
	// 기존 State 입력 처리
	// =========================================================
	if (!m_pPlayerState)
		return;

	CPlayerState* pNewState = m_pPlayerState->HandleInput();
	if (pNewState)
		m_pPlayerState = m_pPlayerState->ChangeState(m_pPlayerState, pNewState);
}

void CPlayer::Update_State(_float fTimeDelta)
{
	if (!m_pPlayerState)
		return;

	CPlayerState* pNewState = m_pPlayerState->Update(fTimeDelta);
	if (pNewState)
		m_pPlayerState = m_pPlayerState->ChangeState(m_pPlayerState, pNewState);
}

void CPlayer::Late_Update_State(_float fTimeDelta)
{
	if (!m_pPlayerState)
		return;

	CPlayerState* pNewState = m_pPlayerState->Late_Update(fTimeDelta);
	if (pNewState)
		m_pPlayerState = m_pPlayerState->ChangeState(m_pPlayerState, pNewState);
}

void CPlayer::Respawon()
{
	_vector vPos = XMVectorSet(m_vSpawnPos.x, m_vSpawnPos.y, m_vSpawnPos.z, 1.f);
	m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, vPos);

	m_pNavigationCom->Compute_CurrentIndex_byXZ(vPos);
	Check_Navigation();   // 필요하면
}

void CPlayer::Check_Navigation()
{
	_vector vPosition = m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION);
	_float m_fWlakingHeight = m_pNavigationCom->Compute_Height(vPosition, 0.f);

	vPosition = XMVectorSetY(vPosition, m_fWlakingHeight);
	m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, vPosition);
}

void CPlayer::Compute_CurrentIndex()
{
	m_pNavigationCom->Compute_CurrentIndex_byXZ(m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION));
}

void CPlayer::Get_AuraColor()
{
	switch (m_ePlayerID)
	{
	case CPlayer::PLAYERID::EZREAL:
		m_vAuraColor = _float3(1.f, .35f, 0.f);
		break;
	case CPlayer::PLAYERID::LEESIN:
		m_vAuraColor = _float3(0.f, .3f, 1.f);
		break;
	}
}


void CPlayer::Free()
{
	__super::Free();
	Safe_Delete(m_pPlayerState);
	Safe_Release(m_pPlayerManager);
}


