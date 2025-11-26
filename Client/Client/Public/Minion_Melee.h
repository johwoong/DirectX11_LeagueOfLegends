#pragma once
#include "Client_Defines.h"
#include "Minion.h"
#include "Minion_State.h"

class CMinion_Melee final : public CMinion
{
public:
	enum ANIM
	{
		ATTACK2,
		ATTAKC3,
		ATTACK3_B,
		ATTACK1,
		DEATH2,
		DEATH3,
		DEATH4,
		DEATH,
		IDLE,
		RUN,
	};

private:
	CMinion_Melee(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CMinion_Melee(const CMinion_Melee& rhs);
	virtual ~CMinion_Melee() = default;
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
	virtual void ReUse_Setting(void* pArg) override;  // ★ 추가
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
	virtual void On_Death() override;   // ★ 추가
private:
	class CMinionState* m_pState = nullptr;
public:
	static CMinion_Melee* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg = nullptr);
	virtual void Free() override;
};

