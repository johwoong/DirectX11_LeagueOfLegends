#include "ParticleSystem.h"
#include "GameInstance.h"
#include "Imgui_Manager.h"

CParticleSystem::CParticleSystem(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CEffect(pDevice, pContext)
{
}

CParticleSystem::CParticleSystem(const CParticleSystem& rhs)
    : CEffect(rhs)
{
}

HRESULT CParticleSystem::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CParticleSystem::Initialize(void* pArg)
{
    if(FAILED(Ready_Components(pArg)))
        return E_FAIL;

    InitializeParticleSystem();
    
    return S_OK;
}

int CParticleSystem::Update(_float fTimeDelta)
{
    if (m_bDead)
        return OBJ_DEAD;

    KillParticles(); // 이전 파티클 제거

    if (m_bPlay)
    {
        if (!m_bCanStart)
        {
            if (m_tParticleDesc.m_fParticleStartAfter == 0)
                m_bCanStart = true;
            else
            {
                if (m_fAccumulatedTime < m_tParticleDesc.m_fParticleStartAfter)
                    m_fAccumulatedTime += fTimeDelta;
                else
                {
                    m_bCanStart = true;
                    m_fAccumulatedTime = 0.f;
                }
            }
        }
        
        if (m_bCanStart)
            EmitParticles(fTimeDelta); // 새 파티클 생성
    }

    UpdateParticles(fTimeDelta); // 파티클 업데이트 (이동, 색, 크기 ,알파, 속도 등)
    SortParticles(); // 파티클 정렬 - 카메라 거리 먼 순으로 

    m_pVIBufferPointInstance->Update(fTimeDelta, m_Particles, m_iCurrentParticleCount);

    return OBJ_NOEVENT;
}

void CParticleSystem::Late_Update(_float fTimeDelta)
{
    if (m_pRenderCom)
    {
        Compute_CamDistance(m_pTransformCom->Get_State(CTransform::STATE_TRANSLATION));

        m_pRenderCom->Add_RenderGroup(CRenderer::RENDER_ALPHABLEND, this);

        if (m_tParticleDesc.m_bGlow)
            m_pRenderCom->Add_RenderGroup(CRenderer::RENDER_GLOW, this);
    }
}

HRESULT CParticleSystem::Render()
{
    if (!m_pVIBufferPointInstance || !m_pShaderCom)
        return E_FAIL;

    __super::Render();

    m_pShaderCom->Begin(0);
    m_pVIBufferPointInstance->Render();

    return S_OK;
}

HRESULT CParticleSystem::Render_Glow()
{
    if (!m_pVIBufferPointInstance || !m_pShaderCom)
        return E_FAIL;

    __super::Render();

    m_pShaderCom->Begin(1);
    m_pVIBufferPointInstance->Render();

    return S_OK;
}

HRESULT CParticleSystem::Ready_Components(void* pArg)
{
    memcpy(&m_tParticleDesc, (PARTICLEDESC*)pArg, sizeof(PARTICLEDESC));

    CTransform::tagTransformDesc TransformDesc;
    ZeroMemory(&TransformDesc, sizeof(CTransform::TRANSFORMDESC));
    TransformDesc.fSpeedPerSec = 0.f;
    TransformDesc.fRotationPerSec = XMConvertToRadians(90.0f);
    // Com Transform
    if (FAILED(__super::Add_Components(TEXT("Com_Transform"), LEVEL_STATIC, TEXT("Prototype_Component_Transform"), (CComponent**)&m_pTransformCom, &TransformDesc)))
        return E_FAIL;

    // Com_Renderer
    if (FAILED(__super::Add_Components(TEXT("Com_Renderer"), LEVEL_STATIC, TEXT("Prototype_Component_Renderer"), (CComponent**)&m_pRenderCom)))
        return E_FAIL;

    /* For.Com_Texture */
    if (FAILED(__super::Add_Components(TEXT("Com_Texture"), LEVEL_STATIC, m_tParticleDesc.wcPrototypeId, (CComponent**)&m_pTextureCom)))
        return E_FAIL;

    if (FAILED(__super::Add_Components(TEXT("Com_Shader"), LEVEL_STATIC, TEXT("Prototype_Component_Shader_VtxPointInstance"), (CComponent**)&m_pShaderCom)))
        return E_FAIL;


    /* For.Com_VIBuffer_Point_Instance */
    if (FAILED(__super::Add_Components(TEXT("Com_VIBuffer_Point_Instance"), LEVEL_STATIC, TEXT("Prototype_Component_VIBuffer_Point_Instance"), (CComponent**)&m_pVIBufferPointInstance, &m_tParticleDesc.m_iMaxParticles)))
        return E_FAIL;

    return S_OK;
}

HRESULT CParticleSystem::SetUp_ShaderResources()
{
    __super::SetUp_ShaderResources();

    if (FAILED(m_pShaderCom->Set_ShaderResourceView("g_DiffuseTexture", m_pTextureCom->Get_SRV(0))))
        return E_FAIL;

    if (FAILED(m_pShaderCom->Set_RawValue("g_fAlphaDiscard", &m_tParticleDesc.m_fAlphaDiscard, sizeof(_float))))
        return E_FAIL;

	_float4 camPos = m_pGameInstance->Get_CamPosition();
    if (FAILED(m_pShaderCom->Set_RawValue("g_vCamPosition", &camPos, sizeof(_float4))))
        return E_FAIL;

    if (m_tParticleDesc.m_bGlow)
    {
        if (FAILED(m_pShaderCom->Set_RawValue("g_vGlowColor", &m_tParticleDesc.vGlowColor, sizeof(_float3))))
            return E_FAIL;
    }

    return S_OK;
}

HRESULT CParticleSystem::ResetParticleSystem()
{
    Safe_Delete_Array(m_Particles);
    InitializeParticleSystem();

    // 인덱스, 인스턴스 버퍼 해제 후 새 파티클 수에 맞게 생성
    m_pVIBufferPointInstance->Reset(&m_tParticleDesc.m_iMaxParticles);

    return S_OK;
}

HRESULT CParticleSystem::InitializeParticleSystem()
{
    // 파티클 생성
    m_Particles = new Particle[m_tParticleDesc.m_iMaxParticles];
    
    // 파티클 배열 초기화
    for (_int i = 0; i < m_tParticleDesc.m_iMaxParticles; i++)
        m_Particles[i].bActive = false;

    // 현재 활성화된 파티클 수를 0으로 초기화
    m_iCurrentParticleCount = 0;

    // 누적 시간 초기화
    m_fAccumulatedTime = 0.f;

    return S_OK;
}

void CParticleSystem::EmitParticles(_float fTimeDelta)
{
	_bool bEmitParticle, bFound;
	_float fPositionX, fPositionY, fPositionZ, fRed, fGreen, fBlue, fInitialAlpha, fAlpha, fInitialVelocity, fVelocity, fInitialSize, fSize;
	_float3 vInitialColor;
	_float3 vDirection;
	_int iIndex, i, j;

	if (m_tParticleDesc.m_eSpawnType == 0)
	{
		// 프레임 시간 누적
		m_fAccumulatedTime += fTimeDelta;

		// 프레임 생성 여부
		bEmitParticle = false;

		// 파티클 시간 체크
		if (m_fAccumulatedTime > (1.f / m_tParticleDesc.m_fParticlePerSecond))
		{
			m_fAccumulatedTime = 0.f;
			bEmitParticle = true;
		}

		// 생성 조건 맞으면 생성
		if ((bEmitParticle == true) && (m_iCurrentParticleCount < (m_tParticleDesc.m_iMaxParticles - 1)))
		{
			m_iCurrentParticleCount++;

			// 랜덤한 파티클 뿌리기
			fPositionX = (((float)rand() - (float)rand()) / RAND_MAX) * m_tParticleDesc.m_fParticleDeviationX;
			fPositionY = (((float)rand() - (float)rand()) / RAND_MAX) * m_tParticleDesc.m_fParticleDeviationY;
			fPositionZ = (((float)rand() - (float)rand()) / RAND_MAX) * m_tParticleDesc.m_fParticleDeviationZ;

			vDirection = m_tParticleDesc.m_vParticleDirection;

			if (m_tParticleDesc.m_bRandomDirectionX)
				vDirection.x = ((float)std::rand() / (float)RAND_MAX) * 2.0f - 1.0f; /* -1 ~ 1 */
			if (m_tParticleDesc.m_bRandomDirectionY)
				vDirection.y = ((float)std::rand() / (float)RAND_MAX) * 2.0f - 1.0f; /* -1 ~ 1 */
			if (m_tParticleDesc.m_bRandomDirectionZ)
				vDirection.z = ((float)std::rand() / (float)RAND_MAX) * 2.0f - 1.0f; /* -1 ~ 1 */

			vInitialColor = m_tParticleDesc.m_vColor;

			fRed = vInitialColor.x;
			fGreen = vInitialColor.y;
			fBlue = vInitialColor.z;

			fInitialAlpha = m_tParticleDesc.m_fAlpha;
			fAlpha = fInitialAlpha;

			fInitialVelocity = m_tParticleDesc.m_fParticleVelocity + (((float)rand() - (float)rand()) / RAND_MAX) * m_tParticleDesc.m_fParticleVelocityVariation;
			fVelocity = fInitialVelocity;

			fInitialSize = m_tParticleDesc.m_fParticleSize + (((float)rand() - (float)rand()) / RAND_MAX) * m_tParticleDesc.m_fParticleSizeVariation;
			fSize = fInitialSize;


			// 알파 블렌딩 정렬 - Z 값 기준으로 정렬된 배열에 새 파티클이 들어갈 위치를 찾는다.
			iIndex = 0;
			bFound = false;
			while (!bFound)
			{
				if ((m_Particles[iIndex].bActive == false) || (m_Particles[iIndex].fPositionZ < fPositionZ))
					bFound = true;
				else
					iIndex++;
			}

			
			// 파티클이 들어갈 위치 확보 뒤 쪽 부터 밀어서 공간 만들기
			i = m_iCurrentParticleCount;
			j = i - 1;

			while (i != iIndex)
			{
				m_Particles[i].fPositionX = m_Particles[j].fPositionX;
				m_Particles[i].fPositionY = m_Particles[j].fPositionY;
				m_Particles[i].fPositionZ = m_Particles[j].fPositionZ;
				m_Particles[i].vDirection = m_Particles[j].vDirection;
				m_Particles[i].vInitialColor = m_Particles[j].vInitialColor;
				m_Particles[i].fRed = m_Particles[j].fRed;
				m_Particles[i].fGreen = m_Particles[j].fGreen;
				m_Particles[i].fBlue = m_Particles[j].fBlue;
				m_Particles[i].fInitialAlpha = m_Particles[j].fInitialAlpha;
				m_Particles[i].fAlpha = m_Particles[j].fAlpha;
				m_Particles[i].fInitialVelocity = m_Particles[j].fInitialVelocity;
				m_Particles[i].fVelocity = m_Particles[j].fVelocity;
				m_Particles[i].fInitialSize = m_Particles[j].fInitialSize;
				m_Particles[i].fSize = m_Particles[j].fSize;
				m_Particles[i].fLife = m_Particles[j].fLife;
				m_Particles[i].bActive = m_Particles[j].bActive;

				i--;
				j--;
			}

			// 정렬 순서를 유지한 위치에 파티클 데이터 넣기
			m_Particles[iIndex].fPositionX = fPositionX;
			m_Particles[iIndex].fPositionY = fPositionY;
			m_Particles[iIndex].fPositionZ = fPositionZ;
			m_Particles[iIndex].vDirection = vDirection;
			m_Particles[iIndex].vInitialColor = vInitialColor;
			m_Particles[iIndex].fRed = fRed;
			m_Particles[iIndex].fGreen = fGreen;
			m_Particles[iIndex].fBlue = fBlue;
			m_Particles[iIndex].fInitialAlpha = fInitialAlpha;
			m_Particles[iIndex].fAlpha = fAlpha;
			m_Particles[iIndex].fInitialVelocity = fInitialVelocity;
			m_Particles[iIndex].fVelocity = fVelocity;
			m_Particles[iIndex].fInitialSize = fInitialSize;
			m_Particles[iIndex].fSize = fSize;
			m_Particles[iIndex].fLife = 0.f;
			m_Particles[iIndex].bActive = true;
		}
	}
	
	// 최대 갯수만큼 한번에 생성
	else
	{
		if (m_iCurrentParticleCount == 0)
		{
			while (m_iCurrentParticleCount < (m_tParticleDesc.m_iMaxParticles - 1))
			{
				m_iCurrentParticleCount++;

				fPositionX = (((float)rand() - (float)rand()) / RAND_MAX) * m_tParticleDesc.m_fParticleDeviationX;
				fPositionY = (((float)rand() - (float)rand()) / RAND_MAX) * m_tParticleDesc.m_fParticleDeviationY;
				fPositionZ = (((float)rand() - (float)rand()) / RAND_MAX) * m_tParticleDesc.m_fParticleDeviationZ;

				vDirection = m_tParticleDesc.m_vParticleDirection;

				if (m_tParticleDesc.m_bRandomDirectionX)
					vDirection.x = ((float)std::rand() / (float)RAND_MAX) * 2.0f - 1.0f;
				if (m_tParticleDesc.m_bRandomDirectionY)
					vDirection.y = ((float)std::rand() / (float)RAND_MAX) * 2.0f - 1.0f;
				if (m_tParticleDesc.m_bRandomDirectionZ)
					vDirection.z = ((float)std::rand() / (float)RAND_MAX) * 2.0f - 1.0f; 

				vInitialColor = m_tParticleDesc.m_vColor;

				fRed = vInitialColor.x;
				fGreen = vInitialColor.y;
				fBlue = vInitialColor.z;

				fInitialAlpha = m_tParticleDesc.m_fAlpha;
				fAlpha = fInitialAlpha;

				fInitialVelocity = m_tParticleDesc.m_fParticleVelocity + (((float)rand() - (float)rand()) / RAND_MAX) * m_tParticleDesc.m_fParticleVelocityVariation;
				fVelocity = fInitialVelocity;

				fInitialSize = m_tParticleDesc.m_fParticleSize + (((float)rand() - (float)rand()) / RAND_MAX) * m_tParticleDesc.m_fParticleSizeVariation;
				fSize = fInitialSize;

				// 알파블렌딩 정렬
				iIndex = 0;
				bFound = false;
				while (!bFound)
				{
					if ((m_Particles[iIndex].bActive == false) || (m_Particles[iIndex].fPositionZ < fPositionZ))
						bFound = true;
					else
						iIndex++;
				}

				i = m_iCurrentParticleCount;
				j = i - 1;

				while (i != iIndex)
				{
					m_Particles[i].fPositionX = m_Particles[j].fPositionX;
					m_Particles[i].fPositionY = m_Particles[j].fPositionY;
					m_Particles[i].fPositionZ = m_Particles[j].fPositionZ;
					m_Particles[i].vDirection = m_Particles[j].vDirection;
					m_Particles[i].vInitialColor = m_Particles[j].vInitialColor;
					m_Particles[i].fRed = m_Particles[j].fRed;
					m_Particles[i].fGreen = m_Particles[j].fGreen;
					m_Particles[i].fBlue = m_Particles[j].fBlue;
					m_Particles[i].fInitialAlpha = m_Particles[j].fInitialAlpha;
					m_Particles[i].fAlpha = m_Particles[j].fAlpha;
					m_Particles[i].fInitialVelocity = m_Particles[j].fInitialVelocity;
					m_Particles[i].fVelocity = m_Particles[j].fVelocity;
					m_Particles[i].fInitialSize = m_Particles[j].fInitialSize;
					m_Particles[i].fSize = m_Particles[j].fSize;
					m_Particles[i].fLife = m_Particles[j].fLife;
					m_Particles[i].bActive = m_Particles[j].bActive;

					i--;
					j--;
				}

				m_Particles[iIndex].fPositionX = fPositionX;
				m_Particles[iIndex].fPositionY = fPositionY;
				m_Particles[iIndex].fPositionZ = fPositionZ;
				m_Particles[iIndex].vDirection = vDirection;
				m_Particles[iIndex].vInitialColor = vInitialColor;
				m_Particles[iIndex].fRed = fRed;
				m_Particles[iIndex].fGreen = fGreen;
				m_Particles[iIndex].fBlue = fBlue;
				m_Particles[iIndex].fInitialAlpha = fInitialAlpha;
				m_Particles[iIndex].fAlpha = fAlpha;
				m_Particles[iIndex].fInitialVelocity = fInitialVelocity;
				m_Particles[iIndex].fVelocity = fVelocity;
				m_Particles[iIndex].fInitialSize = fInitialSize;
				m_Particles[iIndex].fSize = fSize;
				m_Particles[iIndex].fLife = 0.f;
				m_Particles[iIndex].bActive = true;
			}

			m_bDidBurst = true;
		}
	}
}

void CParticleSystem::UpdateParticles(_float fTimeDelta)
{
	// 매 프레임, 파티클을 위치 / 속도 / 델타타임으로 이동시켜 업데이트한다.
	for (_int i = 0; i < m_iCurrentParticleCount; i++)
	{
		_float3 vPosition = _float3(m_Particles[i].fPositionX, m_Particles[i].fPositionY, m_Particles[i].fPositionZ);
		_vector vNewPosition = XMVector3Normalize(XMLoadFloat3(&m_Particles[i].vDirection)) * m_Particles[i].fVelocity * fTimeDelta;
		XMStoreFloat3(&vPosition, XMLoadFloat3(&vPosition) + vNewPosition);

		m_Particles[i].fPositionX = vPosition.x;
		m_Particles[i].fPositionY = vPosition.y;
		m_Particles[i].fPositionZ = vPosition.z;

		ColorLerp(i);
		VelocityLerp(i);
		SizeLerp(i);
		AlphaLerp(i);

		m_Particles[i].fLife += fTimeDelta;
	}

	return;
}

void CParticleSystem::SortParticles()
{
	_float4 camPos = CGameInstance::GetInstance()->Get_CamPosition();
	const XMVECTOR vCameraPosition = XMLoadFloat4(&camPos);

	const XMMATRIX vParticleWorldMatrix = m_pTransformCom->Get_WorldMatrix();

	std::sort(m_Particles, m_Particles + m_iCurrentParticleCount,
		[&](const Particle& s, const Particle& d)
		{
			const XMVECTOR ws = XMVector3TransformCoord(
				XMVectorSet(s.fPositionX, s.fPositionY, s.fPositionZ, 1.f), vParticleWorldMatrix);

			const XMVECTOR wd = XMVector3TransformCoord(
				XMVectorSet(d.fPositionX, d.fPositionY, d.fPositionZ, 1.f), vParticleWorldMatrix);

			const float ds = XMVectorGetX(XMVector3Length(vCameraPosition - ws));
			const float dd = XMVectorGetX(XMVector3Length(vCameraPosition - wd));

			return ds > dd; // 먼 게 먼저(Back-to-front)
		});
}

void CParticleSystem::ColorLerp(_uint iParticleIndex)
{
	if (m_ColorCurves.empty())
		return;

	const float life = (m_tParticleDesc.m_fParticleLifeTime > 0.f) ? m_tParticleDesc.m_fParticleLifeTime : 1.f;
	const float fCurrentLifeNormalized = m_Particles[iParticleIndex].fLife / life;

	for (std::array<_float, 5>&fColorCurve : m_ColorCurves)
	{
		if (fCurrentLifeNormalized < fColorCurve[3])
			break;

		if (fCurrentLifeNormalized > fColorCurve[4])
		{
			m_Particles[iParticleIndex].vInitialColor = _float3(fColorCurve[0], fColorCurve[1], fColorCurve[2]);
			continue;
		}

		const float fFactorDividend = (m_Particles[iParticleIndex].fLife - (life * fColorCurve[3]));
		const float fFactorDivisor = ((life * fColorCurve[4]) - (life * fColorCurve[3]));
		const float fInterpFactor = (fFactorDivisor != 0.f) ? (fFactorDividend / fFactorDivisor) : 0.f;

		_float3 targetColor(fColorCurve[0], fColorCurve[1], fColorCurve[2]);
		const XMVECTOR vInit = XMLoadFloat3(&m_Particles[iParticleIndex].vInitialColor);
		const XMVECTOR vTarget = XMLoadFloat3(&targetColor);

		const XMVECTOR vLerp = vInit + fInterpFactor * (vTarget - vInit);

		_float3 vColor;
		XMStoreFloat3(&vColor, vLerp);

		m_Particles[iParticleIndex].fRed = vColor.x;
		m_Particles[iParticleIndex].fGreen = vColor.y;
		m_Particles[iParticleIndex].fBlue = vColor.z;

		break;
	}
}

void CParticleSystem::VelocityLerp(_uint iParticleIndex)
{
	if (m_VelocityCurves.empty())
		return;

	// 0 ~ 1 
	_float fCurrentLifeNormalized = m_Particles[iParticleIndex].fLife / m_tParticleDesc.m_fParticleLifeTime;

	for (_float3& fVelocityCurve : m_VelocityCurves)
	{
		// 아직 이 커브 시작 시간이 아니면 break y = 시작 시간
		if (fCurrentLifeNormalized < fVelocityCurve.y)
			break;

		// 이미 커브 종료시간을 지나면 최종값으로 고정하고 continue z = 끝 시간
		if (fCurrentLifeNormalized > fVelocityCurve.z)
		{
			m_Particles[iParticleIndex].fInitialVelocity = fVelocityCurve.x;
			continue;
		}

		_float fFactorDividend = (m_Particles[iParticleIndex].fLife - (m_tParticleDesc.m_fParticleLifeTime * fVelocityCurve.y));
		_float fFactorDivisor = ((m_tParticleDesc.m_fParticleLifeTime * fVelocityCurve.z) - (m_tParticleDesc.m_fParticleLifeTime * fVelocityCurve.y));

		_float fInterpFactor = fFactorDividend / fFactorDivisor;
		_float fLerpVelocity = m_Particles[iParticleIndex].fInitialVelocity + fInterpFactor * (fVelocityCurve.x - m_Particles[iParticleIndex].fInitialVelocity);
		m_Particles[iParticleIndex].fVelocity = fLerpVelocity;

		break;
	}
}

void CParticleSystem::SizeLerp(_uint iParticleIndex)
{
	if (m_SizeCurves.empty())
		return;

	_float fCurrentLifeNormalized = m_Particles[iParticleIndex].fLife / m_tParticleDesc.m_fParticleLifeTime;

	for (_float3& fSizeCurve : m_SizeCurves)
	{
		if (fCurrentLifeNormalized < fSizeCurve.y)
			break;

		if (fCurrentLifeNormalized > fSizeCurve.z)
		{
			m_Particles[iParticleIndex].fInitialSize = fSizeCurve.x;
			continue;
		}

		_float fFactorDividend = (m_Particles[iParticleIndex].fLife - (m_tParticleDesc.m_fParticleLifeTime * fSizeCurve.y));
		_float fFactorDivisor = ((m_tParticleDesc.m_fParticleLifeTime * fSizeCurve.z) - (m_tParticleDesc.m_fParticleLifeTime * fSizeCurve.y));

		_float fInterpFactor = fFactorDividend / fFactorDivisor;
		_float fLerpSize = m_Particles[iParticleIndex].fInitialSize + fInterpFactor * (fSizeCurve.x - m_Particles[iParticleIndex].fInitialSize);
		m_Particles[iParticleIndex].fSize = fLerpSize;

		break;
	}
}

void CParticleSystem::AlphaLerp(_uint iParticleIndex)
{
	if (m_AlphaCurves.empty())
		return;

	/* 0 ~ 1 */
	_float fCurrentLifeNormalized = m_Particles[iParticleIndex].fLife / m_tParticleDesc.m_fParticleLifeTime;

	for (_float3& fAlphaCurve : m_AlphaCurves)
	{
		/* Break cause Curve should not start yet ('Y' is the Curve Start Time). */
		if (fCurrentLifeNormalized < fAlphaCurve.y)
			break;

		/* Skip cause Curve has already been lerped through ('Z' is the Curve End Time). */
		if (fCurrentLifeNormalized > fAlphaCurve.z)
		{
			m_Particles[iParticleIndex].fInitialAlpha = fAlphaCurve.x;
			continue;
		}

		_float fFactorDividend = (m_Particles[iParticleIndex].fLife - (m_tParticleDesc.m_fParticleLifeTime * fAlphaCurve.y));
		_float fFactorDivisor = ((m_tParticleDesc.m_fParticleLifeTime * fAlphaCurve.z) - (m_tParticleDesc.m_fParticleLifeTime * fAlphaCurve.y));

		_float fInterpFactor = fFactorDividend / fFactorDivisor;
		_float fLerpAlpha = m_Particles[iParticleIndex].fInitialAlpha + fInterpFactor * (fAlphaCurve.x - m_Particles[iParticleIndex].fInitialAlpha);
		m_Particles[iParticleIndex].fAlpha = fLerpAlpha;

		break;
	}
}

void CParticleSystem::KillParticles()
{
	for (_int i = 0; i < m_tParticleDesc.m_iMaxParticles; i++)
	{
		if ((m_Particles[i].bActive == true) && (m_Particles[i].fLife > m_tParticleDesc.m_fParticleLifeTime))
		{
			m_Particles[i].bActive = false;
			m_iCurrentParticleCount--;


			// 죽은 파티클을 지우기 위해서 뒤에 있는 파티클들을 앞으로 댕겨 배열을 채움 (정렬된 상태 유지)
			for (_int j = i; j < m_tParticleDesc.m_iMaxParticles - 1; j++)
			{
				m_Particles[j].fPositionX = m_Particles[j + 1].fPositionX;
				m_Particles[j].fPositionY = m_Particles[j + 1].fPositionY;
				m_Particles[j].fPositionZ = m_Particles[j + 1].fPositionZ;
				m_Particles[j].vDirection = m_Particles[j + 1].vDirection;
				m_Particles[j].vInitialColor = m_Particles[j + 1].vInitialColor;
				m_Particles[j].fRed = m_Particles[j + 1].fRed;
				m_Particles[j].fGreen = m_Particles[j + 1].fGreen;
				m_Particles[j].fBlue = m_Particles[j + 1].fBlue;
				m_Particles[j].fInitialAlpha = m_Particles[j + 1].fInitialAlpha;
				m_Particles[j].fAlpha = m_Particles[j + 1].fAlpha;
				m_Particles[j].fInitialVelocity = m_Particles[j + 1].fInitialVelocity;
				m_Particles[j].fVelocity = m_Particles[j + 1].fVelocity;
				m_Particles[j].fInitialSize = m_Particles[j + 1].fInitialSize;
				m_Particles[j].fSize = m_Particles[j + 1].fSize;
				m_Particles[j].fLife = m_Particles[j + 1].fLife;
				m_Particles[j].bActive = m_Particles[j + 1].bActive;
			}

			// Loop 모드에서 마지막 파티클까지 다 사라지면 방출 중지
			if (m_tParticleDesc.m_eSpawnType == 0 && m_iCurrentParticleCount == 0)
				m_bCanStart = false;

			// BURST 모드에서 마지막 파티클까지 다사라지고 이미 한번 터졌다면 재생 중지
			if (m_tParticleDesc.m_eSpawnType == 1 && m_iCurrentParticleCount == 0 && m_bDidBurst)
			{
				m_bPlay = false;
				m_bCanStart = false;
				CImGui_Manager::GetInstance()->Set_Play(false);
			}
		}
	}
}

CParticleSystem* CParticleSystem::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CParticleSystem* pInstance = new CParticleSystem(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CParticleSystem");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CGameObject* CParticleSystem::Clone(void* pArg)
{
	CParticleSystem* pInstance = new CParticleSystem(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CParticleSystem");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CParticleSystem::Free()
{
	__super::Free();

	Safe_Delete_Array(m_Particles);

	Safe_Release(m_pTextureCom);
	Safe_Release(m_pVIBufferPointInstance);
}
