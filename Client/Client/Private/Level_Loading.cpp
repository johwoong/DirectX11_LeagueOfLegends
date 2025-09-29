#include "Level_Loading.h"

#include "Loader.h"
#include "Level_Logo.h"
#include "Level_GamePlay.h"

#include "GameInstance.h"

CLevel_Loading::CLevel_Loading(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CLevel { pDevice, pContext } 
{
}

HRESULT CLevel_Loading::Initialize(LEVEL eNextLevelID)
{
	m_eNextLevelID = eNextLevelID;

	/* ���������� ���� �ڿ����� �غ��Ѵ�.(�߰�������) */
	m_pLoader = CLoader::Create(m_pDevice, m_pContext, m_eNextLevelID);
	if (nullptr == m_pLoader)
		return E_FAIL;

	/* �ڿ��� �غ��ϴ� ���� �ε� ����� ������ ���ش�.(���ν�����) */
	if (FAILED(Ready_Layer_UI()))
		return E_FAIL;


    return S_OK;
}

void CLevel_Loading::Update(_float fTimeDelta)
{
	if (true == m_pLoader->isFinished() &&
		GetKeyState(VK_RETURN) & 0x8000)
	{
		CLevel* pNewLevel = { nullptr };

		switch (m_eNextLevelID)
		{
			case LEVEL::LOGO:
				pNewLevel = CLevel_Logo::Create(m_pDevice, m_pContext);
				break;
			case LEVEL::GAMEPLAY:
				pNewLevel = CLevel_GamePlay::Create(m_pDevice, m_pContext);
				break;
		}

		if (nullptr == pNewLevel)
			return;

		if (SUCCEEDED(m_pGameInstance->Change_Level(ENUM_TO_UINT(m_eNextLevelID), pNewLevel)))
			return;

		MSG_BOX("���� �� �̻��ѵ�?");
	}	
}

HRESULT CLevel_Loading::Render()
{
	m_pLoader->Output();

    return S_OK;
}

HRESULT CLevel_Loading::Ready_Layer_UI()
{
	return S_OK;
}

CLevel_Loading* CLevel_Loading::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, LEVEL eNextLevelID)
{
	CLevel_Loading* pInstance = new CLevel_Loading(pDevice, pContext);

	if (FAILED(pInstance->Initialize(eNextLevelID)))
	{
		MSG_BOX("Failed to Created : CLevel_Loading");
		Safe_Release(pInstance);
	}
	return pInstance;
}


void CLevel_Loading::Free()
{
    __super::Free();

}
