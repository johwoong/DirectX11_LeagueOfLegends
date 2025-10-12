#pragma once

#include "Client_Defines.h"
#include "Level.h"

/* 다음레벨에 대한 자원들을 준비한다.(추가스레드) */
/* 자원을 준비하는 동안 로딩 장면을 렌더링 해준다.(메인스레드) */

NS_BEGIN(Client)

class CLevel_Loading final : public CLevel
{
private:
	CLevel_Loading(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CLevel_Loading() = default;


public:
	virtual HRESULT Initialize(LEVEL eNextLevelID);
	virtual void Update(_float fTimeDelta) override;
	virtual HRESULT Render() override;

private:
	class CLoader*  m_pLoader = { nullptr };
	LEVEL			m_eNextLevelID = { LEVEL::END };

private:
	ID3D11Device* m_pDevice = { nullptr };
	ID3D11DeviceContext* m_pContext = { nullptr };

private:
	HRESULT Ready_Layer_UI();

public:
	static CLevel_Loading* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, LEVEL eNextLevelID);
	virtual void Free() override;
};

NS_END