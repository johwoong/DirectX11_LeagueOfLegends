#pragma once

#include "Base.h"

/* 클라이언트 개발자가 만드는 모들 레벨들의 부모가될 클래스다. */

NS_BEGIN(Engine)

class ENGINE_DLL CLevel abstract : public CBase
{
protected:
	CLevel(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CLevel() = default;

public:
	virtual HRESULT Initialize();
	virtual void Update(_float fTimeDelta);
	virtual void Late_Update(_float fTimeDelta);
	void		Set_NextLevel(_bool type, _uint iLevelIndex) {
		m_bNextNevel = type; m_iNextLevelIndex
			= iLevelIndex;
	}

	_bool Get_NextLevel() { return m_bNextNevel; }
	_uint Get_NextLevelIndex() { return m_iNextLevelIndex; }

protected:
	ID3D11Device* m_pDevice = nullptr;
	ID3D11DeviceContext* m_pContext = nullptr;
	_bool		m_bNextNevel = false;
	_uint		m_iNextLevelIndex = 0;
	class CGameInstance* m_pGameInstance = { nullptr };

public:
	virtual void Free() override;
};

NS_END