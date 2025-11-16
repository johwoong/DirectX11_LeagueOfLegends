#pragma once

#include "Actor.h"
NS_BEGIN(Engine)
class CModel;
NS_END

NS_BEGIN(Client)


class CSpawnBox final : public CActor
{
public:
	typedef struct TreasureBoxTag
	{
		XMFLOAT3	vPosition = XMFLOAT3(0.f, 0.f, 0.f);
		XMFLOAT3	vScale = XMFLOAT3(1.f, 1.f, 1.f);
		XMFLOAT3	vRotation = XMFLOAT3(0.f, 0.f, 0.f);
		float		m_fAngle = 0.f;
	}BOXTAG;


private:
	CSpawnBox(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CSpawnBox() = default;

public:
	virtual HRESULT Initialize_Prototype() override;
	virtual HRESULT Initialize(void* pArg) override;
	virtual int Update(_float fTimeDelta) override;
	virtual void Late_Update(_float fTimeDelta) override;
	virtual HRESULT Render() override;
	BOXTAG	Get_BoxDesc() { return m_eTreasureBoxDesc; }
	void	Set_BoxDesc(BOXTAG BoxDesc) { memcpy(&m_eTreasureBoxDesc, &BoxDesc, sizeof(BOXTAG)); }
	void	Set_Picked();

	virtual HRESULT Ready_Components(void* pArg = nullptr) override;
	virtual HRESULT SetUp_ShaderResources() override;

private:
	CModel* m_pModelCom = nullptr;
	BOXTAG					m_eTreasureBoxDesc;
	_bool					m_bGet = false;

public:
	static CSpawnBox* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg = nullptr) override;
	virtual void Free() override;
};

NS_END