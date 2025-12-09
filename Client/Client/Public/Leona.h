#pragma once
#include "Enemy.h"
#include "Client_Defines.h"

NS_BEGIN(Client)
class CLeona final : public CEnemy
{
public:
	enum ANIM {
		IDLE, ATTACK1, ATTACK2, ATTACK3, CHANNEL, CHANNEL_WIND, CRIT, DANCE, DEATH, IDLE2, IDLE3, JOKE, LAUGH,
		RUN, Q, W, W_ATTACK, W_IDLE, W_RUN, E, R, TAUNT, ANIM_END
	};

	enum LINE_TYPE
	{
		TOP, MID, BOTTOM,
	};

public:
	CModel* Get_Model() { return m_pModelCom; }
	CTransform* Get_Transform() { return m_pTransformCom; }
	CShader* Get_Shader() { return m_pShaderCom; }
	class CLeonaState* Get_State() { return m_pState; }
public:
	virtual _bool Is_AnimationLoop(_uint eAnimId) override;
	virtual void Ready_Animation_Event() override;
	virtual HRESULT SetUp_ShaderID() override;

private:
	CLeona(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CLeona(const CLeona& rhs);
	virtual ~CLeona() = default;

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
	class CLeonaState* m_pState = nullptr;
	LINE_TYPE m_eLineType = LINE_TYPE::BOTTOM;


public:
	static CLeona* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg = nullptr);
	virtual void Free() override;

};
NS_END