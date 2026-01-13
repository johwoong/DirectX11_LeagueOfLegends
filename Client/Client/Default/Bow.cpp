#include "Bow.h"
#include "GameInstance.h"
#include "EzrealProjectile.h"
#include "PoolManager.h"

CBow::CBow(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CActor(pDevice, pContext)
{

}

CBow::CBow(const CBow& rhs)
	: CActor(rhs)
{
}

HRESULT CBow::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CBow::Initialize(void* pArg)
{
	if (FAILED(Ready_Components(pArg)))
		return E_FAIL;

	Ready_Animation_Event();

	m_pModelCom->Set_CurrentAnimIndex(CBow::ANIM::IDLE);
	m_pModelCom->Set_TimeReset(); // 있으면 추천

	if (pArg)
	{
		auto* pDesc = reinterpret_cast<BOWDESC*>(pArg);
		m_tDesc = *pDesc;

		// 위치
		m_pTransformCom->Set_State(CTransform::STATE_TRANSLATION, XMVectorSetW(m_tDesc.vPos, 1.f));

		// 방향(룩/라이트)
		_vector vLook = XMVector3Normalize(XMVectorSetY(m_tDesc.vDir, 0.f));
		_vector vUp = XMVectorSet(0, 1, 0, 0);
		_vector vRight = XMVector3Normalize(XMVector3Cross(vUp, vLook));
		vUp = XMVector3Cross(vLook, vRight);

		m_pTransformCom->Set_State(CTransform::STATE_RIGHT, vRight);
		m_pTransformCom->Set_State(CTransform::STATE_UP, vUp);
		m_pTransformCom->Set_State(CTransform::STATE_LOOK, vLook);
	}

	// 크기 너무 크다 했으니 일단 줄여
	m_pTransformCom->Set_Scale(CTransform::STATE_RIGHT, 0.008f);
	m_pTransformCom->Set_Scale(CTransform::STATE_UP, 0.008f);
	m_pTransformCom->Set_Scale(CTransform::STATE_LOOK, 0.008f);

	return S_OK;
}

int CBow::Update(_float fTimeDelta)
{
	int iEvent = __super::Update(fTimeDelta);
	if (iEvent == OBJ_DEAD)
		return OBJ_DEAD;

	// =========================
	// 0) 애니 진행 (첫 프레임은 끝판정 무시)
	// =========================
	if (!m_bSpawned)
	{
		m_bSpawned = true;
		m_pModelCom->Play_Animation(fTimeDelta, false, nullptr, 0.1f);
		return OBJ_NOEVENT;
	}

	// 매 프레임 애니 재생
	const _bool bAnimEnd = m_pModelCom->Play_Animation(fTimeDelta, false, nullptr, 0.1f);

	// =========================
	// 1) 이벤트에서 투사체 스폰 (1회)
	// =========================
	if (!m_bProjectileSpawned)
	{
		const vector<ANIMEVENT> events = m_pModelCom->Get_Events();

		for (const auto& evt : events)
		{
			if (!evt.isPlay) continue;
			if (evt.eType != ANIMEVENT::EVENT_PROJECTILE) continue;

			m_bProjectileSpawned = true;

			// (1) 스폰 본
			CBone* pRoot = m_pModelCom->Get_BonePtr("R_Bow_Root");
			if (!pRoot)
			{
				m_bProjectileSpawned = false;
				break;
			}

			// (2) 본 월드행렬 = boneLocal * pivot * bowWorld
			_float4x4 pivot = m_pModelCom->Get_PivotFloat4x4();
			_matrix pivotMat = XMLoadFloat4x4(&pivot);

			_matrix boneWorld =
				pRoot->Get_CombinedTransformationMatrix() *
				pivotMat *
				m_pTransformCom->Get_WorldMatrix();

			// 축 정규화(스케일 섞여도 안전)
			boneWorld.r[0] = XMVector3Normalize(boneWorld.r[0]);
			boneWorld.r[1] = XMVector3Normalize(boneWorld.r[1]);
			boneWorld.r[2] = XMVector3Normalize(boneWorld.r[2]);

			// (3) 스폰 위치
			_vector vSpawnPos = XMVectorSetW(boneWorld.r[3], 1.f);

			// ============================================================
			// ? (4) 투사체 방향: "커서 방향"으로 쏴야 함
			//     근데 m_tDesc.vDir은 활 메시가 제대로 보이게 하려고
			//     SkillStateR에서 한 번 뒤집어둔 값임.
			//     => 여기서 다시 뒤집어서 projectile은 정상 방향으로 발사.
			// ============================================================
			_vector vDir = XMVectorSetW(-m_tDesc.vDir, 0.f);   // ?? 핵심 한 줄
			vDir = XMVectorSetY(vDir, 0.f);

			if (XMVectorGetX(XMVector3LengthSq(vDir)) < 1e-6f)
				vDir = XMVectorSet(0.f, 0.f, 1.f, 0.f);

			vDir = XMVector3Normalize(vDir);
			vDir = XMVectorSetW(vDir, 0.f);

			// (5) R Projectile 생성
			CProjectile::PROJECTILEDESC desc{};
			desc.eCollisionGroup = PLAYER;
			desc.eProjType = CEzrealProjectile::R;
			desc.vInitPositon = vSpawnPos;
			desc.vTargetDir = vDir;
			desc.fVelocity = 22.f;
			desc.fDeadTime = 1.6f;
			desc.iDamage = 250;
			desc.pOwner = reinterpret_cast<CActor*>(m_tDesc.pOwner);
			desc.pTarget = nullptr;
			desc.ePoolLevel = LEVEL_GAMEPLAY;
			desc.pPoolLayerTag = TEXT("Layer_Projectile");
			CGameInstance* GI = CGameInstance::GetInstance();
			CPoolManager* PM = CPoolManager::GetInstance();
			// 1) 풀에 있으면 재사용 (새로 생성 X)
			if (!PM->Reuse_Pooling_Object(LEVEL_GAMEPLAY, TEXT("Layer_Projectile"), &desc))
			{
				if (FAILED(GI->Add_GameObject(TEXT("Prototype_GameObject_EzrealProjectile"),
					LEVEL_GAMEPLAY, TEXT("Layer_Projectile"), &desc)))
				{
					m_bProjectileSpawned = false;
				}
			}

			break; // 이벤트 1회만
		}
	}

	// =========================
	// 2) Bow 애니 끝나면 제거
	// =========================
	if (bAnimEnd)
	{
		Set_Dead(true);
		return OBJ_DEAD;
	}

	return OBJ_NOEVENT;
}

void CBow::Late_Update(_float fTimeDelta)
{
	if (!m_pRendererCom) return;

	m_pRendererCom->Add_RenderGroup(CRenderer::RENDER_NOALPHABLEND, this);

	// ? 글로우 전용 그룹이 있다면 여기에
	m_pRendererCom->Add_RenderGroup(CRenderer::RENDER_GLOW, this);
}

HRESULT CBow::Render()
{
	if (!m_pShaderCom || !m_pModelCom)
		return E_FAIL;

	if (FAILED(SetUp_ShaderResources()))
		return E_FAIL;

	_uint iNumMeshes = m_pModelCom->Get_NumMeshs();

	for (_uint i = 0; i < iNumMeshes; ++i)
	{
		if (FAILED(m_pModelCom->SetUp_Material(m_pShaderCom, "g_DiffuseTexture", i, aiTextureType_DIFFUSE)))
			return E_FAIL;

		if (FAILED(m_pModelCom->SetUp_Material(m_pShaderCom, "g_NormalTexture", i, aiTextureType_NORMALS)))
			return E_FAIL;

		// ? Bow 전용 패스(7)
		if (FAILED(m_pModelCom->Render(m_pShaderCom, i, 7)))
			return E_FAIL;
	}

	return S_OK;
}

HRESULT CBow::Render_Glow()
{
	if (nullptr == m_pShaderCom || nullptr == m_pModelCom)
		return E_FAIL;

	if (FAILED(SetUp_ShaderResources()))
		return E_FAIL;

	const _uint iNumMeshes = m_pModelCom->Get_NumMeshs();

	for (_uint i = 0; i < iNumMeshes; ++i)
	{
		// PS_GLOW_BOW가 Diffuse 기반이면 Diffuse만 있어도 되는데,
		// 너 셰이더가 Normal도 같이 샘플/바인딩 요구할 수 있어서 같이 세팅
		if (FAILED(m_pModelCom->SetUp_Material(m_pShaderCom, "g_DiffuseTexture", i, aiTextureType_DIFFUSE)))
			return E_FAIL;

		// Normal 없으면 여기서 실패날 수 있으니, BowMesh에 normal이 없거나
		// 머티리얼에 normals가 없을 수 있음 -> 그럼 이 줄 주석 처리하고 테스트해봐.
		if (FAILED(m_pModelCom->SetUp_Material(m_pShaderCom, "g_NormalTexture", i, aiTextureType_NORMALS)))
		{
			// Normal 없는 모델 대비(안전)
			// return E_FAIL;  // <- 기존처럼 실패 내고 싶으면 이거로
		}

		// ? Bow_Glow 패스(8)로 렌더
		if (FAILED(m_pModelCom->Render(m_pShaderCom, i, 8)))
			return E_FAIL;
	}

	return S_OK;
}

HRESULT CBow::Render_ShadowDepth()
{
	return S_OK;
}


void CBow::Ready_Animation_Event()
{
	CAnimation* pAnim = m_pModelCom->Get_Animations()[IDLE];
	if (!pAnim) return;

	ANIMEVENT evt{};
	evt.fStartTime = 17.f;
	evt.fEndTime = 18.f;
	evt.eType = ANIMEVENT::EVENT_PROJECTILE;

	pAnim->Add_Event(evt);;
}

HRESULT CBow::Ready_Components(void* pArg)
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
	if (FAILED(__super::Add_Components(TEXT("Com_Model"), LEVEL_GAMEPLAY, TEXT("BowMesh"), (CComponent**)&m_pModelCom)))
		return E_FAIL;

	return S_OK;
}

HRESULT CBow::SetUp_ShaderResources()
{
	if (!m_pShaderCom || !m_pTransformCom)
		return E_FAIL;

	_float4x4 WorldMatrix = m_pTransformCom->Get_World_TP();
	_float4x4 ViewMatrix = m_pGameInstance->Get_TransformFloat4x4_TP(CPipeLine::D3DTS_VIEW);
	_float4x4 ProjMatrix = m_pGameInstance->Get_TransformFloat4x4_TP(CPipeLine::D3DTS_PROJ);

	if (FAILED(m_pShaderCom->Set_RawValue("g_WorldMatrix", &WorldMatrix, sizeof(_float4x4)))) return E_FAIL;
	if (FAILED(m_pShaderCom->Set_RawValue("g_ViewMatrix", &ViewMatrix, sizeof(_float4x4)))) return E_FAIL;
	if (FAILED(m_pShaderCom->Set_RawValue("g_ProjMatrix", &ProjMatrix, sizeof(_float4x4)))) return E_FAIL;

	// ? Bow 틴트 컬러 (스크린샷 값)
	_float4 vColor = { 0.35f, 0.80f, 1.00f, 0.85f };
	if (FAILED(m_pShaderCom->Set_RawValue("g_vColor", &vColor, sizeof(_float4))))
		return E_FAIL;

	// ? Bow 글로우 컬러 (스크린샷 값)
	_float4 vGlowColor = { 0.40f, 1.00f, 1.00f, 1.00f };
	if (FAILED(m_pShaderCom->Set_RawValue("g_vGlowColor", &vGlowColor, sizeof(_float4))))
		return E_FAIL;

	// ? Glow Power
	float fGlowPower = 4.0f;
	if (FAILED(m_pShaderCom->Set_RawValue("g_fGlowPower", &fGlowPower, sizeof(float))))
		return E_FAIL;

	return S_OK;
}


CBow* CBow::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CBow* pInstance = new CBow(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CBow");
		Safe_Release(pInstance);
	}
	return pInstance;
}

CGameObject* CBow::Clone(void* pArg)
{
	CBow* pInstance = new CBow(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CBow");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CBow::Free()
{
	__super::Free();
}


