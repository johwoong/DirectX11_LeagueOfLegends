#pragma once
#include "Enemy.h"
#include "Client_Defines.h"

NS_BEGIN(Client)
class CVayne final : public CEnemy
{
public:
    enum ANIM {
        ATTACK1, ATTACK2, CHANNEL, CHANNEL_WIND, CRIT,DANCE, DEATH, IDLE1, IDLE2, IDLE3, IDLE4,
        JOKE, LAUGH, RUN, Q,E, TAUNT, Q_ATTACK, Q_IDLE, Q_RUN, R_ATTACK, RQ_IDLE, R_RUN,
        R_THREE_ATTACK, R_IDLE, IDLE
    };
	
	enum LINE_TYPE
	{
		TOP, MID, BOTTOM,
	};

public:
	CModel* Get_Model() { return m_pModelCom; }
	CTransform* Get_Transform() { return m_pTransformCom; }
	CShader* Get_Shader() { return m_pShaderCom; }
	class CVayneState* Get_State() { return m_pState; }
public:
	virtual _bool Is_AnimationLoop(_uint eAnimId) override;
	virtual HRESULT SetUp_ShaderID() override;

private:
	CVayne(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CVayne(const CVayne& rhs);
	virtual ~CVayne() = default;

public:
	virtual HRESULT Initialize_Prototype();
	virtual HRESULT Initialize(void* pArg);
	virtual int Update(_float fTimeDelta);
	virtual void Late_Update(_float fTimeDelta);
	virtual HRESULT Render() override;
	virtual HRESULT Render_Glow() override;

public: /*For.State*/
	void AI_Behavior(_float fTimeDelta);
	void Update_State(_float fTimeDelta);
	void LateUpdate_State(_float fTimeDelta);

public:	/*For Navigation*/
	virtual void Check_Navigation() override;

private:
	virtual HRESULT Ready_Components(void* pArg) override;

private:
	class CVayneState* m_pState = nullptr;
	LINE_TYPE m_eLineType = LINE_TYPE::BOTTOM;


public:
	static CVayne* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg = nullptr);
	virtual void Free() override;

};
NS_END

