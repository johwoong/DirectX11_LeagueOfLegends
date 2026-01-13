
#include "Effect.h"
#include "GameInstance.h"
#include "EffectTexture.h"
#include "EffectMesh.h"
#include "ParticleSystem.h"



CEffect::CEffect(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CActor(pDevice, pContext)
{
}

CEffect::CEffect(const CEffect& rhs)
	: CActor(rhs)
{
}

HRESULT CEffect::Initialize_Prototype()
{
	return S_OK;
}


HRESULT CEffect::Initialize(void* pArg)
{
	return S_OK;
}

int CEffect::Update(_float fTimeDelta)
{
	// ★ 핵심: 죽음 처리
	if (m_bDead || m_bPreDead)
		return OBJ_POOL;

	return OBJ_NOEVENT;
}

void CEffect::Late_Update(_float fTimeDelta)
{
}

HRESULT CEffect::Render()
{
	if (FAILED(SetUp_ShaderResources()))
		return E_FAIL;

	return S_OK;
}

vector<class CEffect*> CEffect::PlayEffectAtLocation(const _tchar* wcEffectName, _matrix mWorldMatrix)
{
	vector<CEffect*> outEffects;

	// 1) 파일 열기
	HANDLE hFileEffect = nullptr;
	_tchar LoadPathEffect[MAX_PATH] = TEXT("../../../Bin/Data/EffectData/");
	wcscat_s(LoadPathEffect, MAX_PATH, wcEffectName);

	hFileEffect = CreateFile(LoadPathEffect, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFileEffect == INVALID_HANDLE_VALUE)
		return outEffects;

	CGameInstance* pGameInstance = CGameInstance::GetInstance();
	pGameInstance->AddRef();

	DWORD dwByte = 0;

	// 2) 이펙트 개수 읽기
	_uint iEffectsCount = 0;
	if (!ReadFile(hFileEffect, &iEffectsCount, sizeof(_uint), &dwByte, nullptr) || dwByte != sizeof(_uint))
	{
		pGameInstance->Release();
		CloseHandle(hFileEffect);
		return outEffects;
	}

	// 3) 개수만큼 순차 파싱 (파일 포맷은 "타입+desc+커브들..." 이므로, 스폰 실패해도 끝까지 읽어야 함)
	for (_uint i = 0; i < iEffectsCount; ++i)
	{
		CEffect::EFFECT_TYPE eType = CEffect::TYPE_END;
		if (!ReadFile(hFileEffect, &eType, sizeof(CEffect::EFFECT_TYPE), &dwByte, nullptr) || dwByte != sizeof(CEffect::EFFECT_TYPE))
			break;

		CEffect* pEffect = nullptr;
		bool bSpawned = false;

		CEffectTexture::TEXTUREEFFECTDESC tTextureEffectDesc{};
		CEffectMesh::MESHEFFECTDESC      tMeshEffectDesc{};
		PARTICLEDESC                     tParticleDesc{};

		// 3-1) 타입별 desc 읽고 스폰 시도 (성공 여부만 bSpawned에 저장)
		switch (eType)
		{
		case CEffect::TYPE_TEXTURE:
		{
			if (!ReadFile(hFileEffect, &tTextureEffectDesc, sizeof(CEffectTexture::TEXTUREEFFECTDESC), &dwByte, nullptr) ||
				dwByte != sizeof(CEffectTexture::TEXTUREEFFECTDESC))
			{
				// 파일이 깨졌거나 끝
				goto END_PARSE;
			}

			HRESULT hr = pGameInstance->Add_GameObject_Out(
				TEXT("Prototype_GameObject_EffectTexture"),
				pGameInstance->Get_CurrentLevelIndex(),
				TEXT("Layer_Effects"),
				(CGameObject*&)pEffect,
				&tTextureEffectDesc);

			if (SUCCEEDED(hr) && pEffect)
			{
				pEffect->Set_EffectType(CEffect::TYPE_TEXTURE);
				bSpawned = true;
			}
			break;
		}
		case CEffect::TYPE_MESH:
		{
			if (!ReadFile(hFileEffect, &tMeshEffectDesc, sizeof(CEffectMesh::MESHEFFECTDESC), &dwByte, nullptr) ||
				dwByte != sizeof(CEffectMesh::MESHEFFECTDESC))
			{
				goto END_PARSE;
			}

			HRESULT hr = pGameInstance->Add_GameObject_Out(
				TEXT("Prototype_GameObject_EffectMesh"),
				pGameInstance->Get_CurrentLevelIndex(),
				TEXT("Layer_Effects"),
				(CGameObject*&)pEffect,
				&tMeshEffectDesc);

			if (SUCCEEDED(hr) && pEffect)
			{
				pEffect->Set_EffectType(CEffect::TYPE_MESH);
				bSpawned = true;
			}
			break;
		}
		case CEffect::TYPE_PARTICLE:
		{
			if (!ReadFile(hFileEffect, &tParticleDesc, sizeof(PARTICLEDESC), &dwByte, nullptr) ||
				dwByte != sizeof(PARTICLEDESC))
			{
				goto END_PARSE;
			}

			HRESULT hr = pGameInstance->Add_GameObject_Out(
				TEXT("Prototype_GameObject_ParticleSystem"),
				pGameInstance->Get_CurrentLevelIndex(),
				TEXT("Layer_Effects"),
				(CGameObject*&)pEffect,
				&tParticleDesc);

			if (SUCCEEDED(hr) && pEffect)
			{
				pEffect->Set_EffectType(CEffect::TYPE_PARTICLE);
				bSpawned = true;
			}
			break;
		}
		default:
			// 알 수 없는 타입이면 더 파싱 불가
			goto END_PARSE;
		}

		// 3-2) 커브들 읽기 (스폰 실패해도 파일 포인터 맞추려고 "무조건" 읽어야 함)

		// COLOR
		_uint iColorCurvesCount = 0;
		if (!ReadFile(hFileEffect, &iColorCurvesCount, sizeof(_uint), &dwByte, nullptr) || dwByte != sizeof(_uint)) goto END_PARSE;
		{
			vector<array<_float, 5>> ColorCurves;
			ColorCurves.reserve(iColorCurvesCount);
			array<_float, 5> ColorCurve{};
			for (_uint j = 0; j < iColorCurvesCount; ++j)
			{
				if (!ReadFile(hFileEffect, &ColorCurve, sizeof(array<_float, 5>), &dwByte, nullptr) || dwByte != sizeof(array<_float, 5>))
					goto END_PARSE;
				ColorCurves.push_back(ColorCurve);
			}
			if (bSpawned && pEffect && !ColorCurves.empty())
				pEffect->Set_ColorCurves(ColorCurves);
		}

		// ALPHA
		_uint iAlphaCurvesCount = 0;
		if (!ReadFile(hFileEffect, &iAlphaCurvesCount, sizeof(_uint), &dwByte, nullptr) || dwByte != sizeof(_uint)) goto END_PARSE;
		{
			vector<_float3> AlphaCurves;
			AlphaCurves.reserve(iAlphaCurvesCount);
			_float3 AlphaCurve{};
			for (_uint j = 0; j < iAlphaCurvesCount; ++j)
			{
				if (!ReadFile(hFileEffect, &AlphaCurve, sizeof(_float3), &dwByte, nullptr) || dwByte != sizeof(_float3))
					goto END_PARSE;
				AlphaCurves.push_back(AlphaCurve);
			}
			if (bSpawned && pEffect && !AlphaCurves.empty())
				pEffect->Set_AlphaCurves(AlphaCurves);
		}

		// SIZE
		_uint iSizeCurvesCount = 0;
		if (!ReadFile(hFileEffect, &iSizeCurvesCount, sizeof(_uint), &dwByte, nullptr) || dwByte != sizeof(_uint)) goto END_PARSE;
		{
			vector<_float3> SizeCurves;
			SizeCurves.reserve(iSizeCurvesCount);
			_float3 SizeCurve{};
			for (_uint j = 0; j < iSizeCurvesCount; ++j)
			{
				if (!ReadFile(hFileEffect, &SizeCurve, sizeof(_float3), &dwByte, nullptr) || dwByte != sizeof(_float3))
					goto END_PARSE;
				SizeCurves.push_back(SizeCurve);
			}
			if (bSpawned && pEffect && !SizeCurves.empty())
				pEffect->Set_SizeCurves(SizeCurves);
		}

		// SCALE
		_uint iScaleCurvesCount = 0;
		if (!ReadFile(hFileEffect, &iScaleCurvesCount, sizeof(_uint), &dwByte, nullptr) || dwByte != sizeof(_uint)) goto END_PARSE;
		{
			vector<array<_float, 5>> ScaleCurves;
			ScaleCurves.reserve(iScaleCurvesCount);
			array<_float, 5> ScaleCurve{};
			for (_uint j = 0; j < iScaleCurvesCount; ++j)
			{
				if (!ReadFile(hFileEffect, &ScaleCurve, sizeof(array<_float, 5>), &dwByte, nullptr) || dwByte != sizeof(array<_float, 5>))
					goto END_PARSE;
				ScaleCurves.push_back(ScaleCurve);
			}
			if (bSpawned && pEffect && !ScaleCurves.empty())
				pEffect->Set_ScaleCurves(ScaleCurves);
		}

		// VELOCITY
		_uint iVelocityCurvesCount = 0;
		if (!ReadFile(hFileEffect, &iVelocityCurvesCount, sizeof(_uint), &dwByte, nullptr) || dwByte != sizeof(_uint)) goto END_PARSE;
		{
			vector<_float3> VelocityCurves;
			VelocityCurves.reserve(iVelocityCurvesCount);
			_float3 VelocityCurve{};
			for (_uint j = 0; j < iVelocityCurvesCount; ++j)
			{
				if (!ReadFile(hFileEffect, &VelocityCurve, sizeof(_float3), &dwByte, nullptr) || dwByte != sizeof(_float3))
					goto END_PARSE;
				VelocityCurves.push_back(VelocityCurve);
			}
			if (bSpawned && pEffect && !VelocityCurves.empty())
				pEffect->Set_VelocityCurves(VelocityCurves);
		}

		// TURN VELOCITY
		_uint iTurnVelocityCurvesCount = 0;
		if (!ReadFile(hFileEffect, &iTurnVelocityCurvesCount, sizeof(_uint), &dwByte, nullptr) || dwByte != sizeof(_uint)) goto END_PARSE;
		{
			vector<_float3> TurnVelocityCurves;
			TurnVelocityCurves.reserve(iTurnVelocityCurvesCount);
			_float3 TurnVelocityCurve{};
			for (_uint j = 0; j < iTurnVelocityCurvesCount; ++j)
			{
				if (!ReadFile(hFileEffect, &TurnVelocityCurve, sizeof(_float3), &dwByte, nullptr) || dwByte != sizeof(_float3))
					goto END_PARSE;
				TurnVelocityCurves.push_back(TurnVelocityCurve);
			}
			if (bSpawned && pEffect && !TurnVelocityCurves.empty())
				pEffect->Set_TurnVelocityCurves(TurnVelocityCurves);
		}

		// NOISE POWER
		_uint iNoisePowerCurvesCount = 0;
		if (!ReadFile(hFileEffect, &iNoisePowerCurvesCount, sizeof(_uint), &dwByte, nullptr) || dwByte != sizeof(_uint)) goto END_PARSE;
		{
			vector<_float3> NoisePowerCurves;
			NoisePowerCurves.reserve(iNoisePowerCurvesCount);
			_float3 NoisePowerCurve{};
			for (_uint j = 0; j < iNoisePowerCurvesCount; ++j)
			{
				if (!ReadFile(hFileEffect, &NoisePowerCurve, sizeof(_float3), &dwByte, nullptr) || dwByte != sizeof(_float3))
					goto END_PARSE;
				NoisePowerCurves.push_back(NoisePowerCurve);
			}
			if (bSpawned && pEffect && !NoisePowerCurves.empty())
				pEffect->Set_NoisePowerCurves(NoisePowerCurves);
		}

		// DISTORT POWER
		_uint iDistortPowerCurvesCount = 0;
		if (!ReadFile(hFileEffect, &iDistortPowerCurvesCount, sizeof(_uint), &dwByte, nullptr) || dwByte != sizeof(_uint)) goto END_PARSE;
		{
			vector<_float3> DistortPowerCurves;
			DistortPowerCurves.reserve(iDistortPowerCurvesCount);
			_float3 DistortPowerCurve{};
			for (_uint j = 0; j < iDistortPowerCurvesCount; ++j)
			{
				if (!ReadFile(hFileEffect, &DistortPowerCurve, sizeof(_float3), &dwByte, nullptr) || dwByte != sizeof(_float3))
					goto END_PARSE;
				DistortPowerCurves.push_back(DistortPowerCurve);
			}
			if (bSpawned && pEffect && !DistortPowerCurves.empty())
				pEffect->Set_DistortPowerCurves(DistortPowerCurves);
		}

		// 3-3) 성공했을 때만 월드세팅 + 벡터에 등록
		if (bSpawned && pEffect)
		{
			pEffect->Set_WorldPosition(mWorldMatrix);
			outEffects.push_back(pEffect);
		}
	}

END_PARSE:
	pGameInstance->Release();
	CloseHandle(hFileEffect);
	return outEffects;
}


HRESULT CEffect::Ready_Components(void* pArg)
{
	return S_OK;
}

HRESULT CEffect::SetUp_ShaderResources()
{
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

void CEffect::Free()
{
	__super::Free();

	Safe_Release(m_pShaderCom);
	Safe_Release(m_pRendererCom);
	Safe_Release(m_pTransformCom);

	Safe_Release(m_pMaskTexture);
	Safe_Release(m_pNoiseTexture);
}