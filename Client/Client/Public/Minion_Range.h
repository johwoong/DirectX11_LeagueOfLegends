#pragma once
#include "Minion.h"
#include "Minion.h"
#include "Minion_State.h"
class CMinion_Range :
    public CMinion
{
public:
	enum ANIM
	{
		ATTACK1,
		ATTACK2,
		DEATH2,
		DEATH3,
		DEATH1,
		IDLE,
		RUN,
	};

private:
	CMinion_Range(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CMinion_Range(const CMinion_Range& rhs);
	virtual ~CMinion_Range() = default;
public:
	CModel* Get_Model() { return m_pModelCom; }
	CTransform* Get_Transform() { return m_pTransformCom; }
	class CMinionState* Get_State() { return m_pState; }

	virtual _int Take_Damage(int fDamage, CActor* DamageCauser) override;
	virtual HRESULT SetUp_ShaderID() override;
	virtual HRESULT Ready_Components(void* pArg = nullptr) override;
public:
	virtual _bool Is_AnimationLoop(_uint eAnimId) override;
public:
	virtual void ReUse_Setting(void* pArg) override;  
public:
	virtual HRESULT Initialize_Prototype();
	virtual HRESULT Initialize(void* pArg);
	virtual int Update(_float fTimeDelta);
	virtual void Late_Update(_float fTimeDelta);

public: /*For.State*/
	void AI_Behavior(_float fTimeDelta);
	void Update_State(_float fTimeDelta);
	void LateUpdate_State(_float fTimeDelta);

public:	/*For Navigation*/
	virtual void Check_Navigation() override;
	virtual void On_Death() override;  
private:
	class CMinionState* m_pState = nullptr;
public:
	static CMinion_Range* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg = nullptr);
	virtual void Free() override;
};

