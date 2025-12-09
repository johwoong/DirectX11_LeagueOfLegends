#pragma once
#include "Npc.h"


// Æ÷Å¾
NS_BEGIN(Client)
class CStoreNpc final : public CNpc
{
public:
	enum TEAMTYPE { RED, BLUE };

	enum class ANIM_BLUE
	{
		IDLE, SPAWN, SPAWN_HOLD, ANIM_END
	};

	enum class ANIM_RED
	{
		IDLE1, IDLE2, IDLE3, IDLE4, IDLE5, IDLE6, SPAWN, SPAWN_HOLD, 
	};

private:
	CStoreNpc(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CStoreNpc(const CStoreNpc& rhs);
	virtual ~CStoreNpc() = default;

public:
	virtual HRESULT Initialize_Prototype();
	virtual HRESULT Initialize(void* pArg);
	virtual int		Update(_float fTimeDelta);
	virtual void	Late_Update(_float fTimeDelta);

private:
	virtual HRESULT Ready_Components(void* pArg);

public: /*For.State*/
	virtual _bool	Is_AnimationLoop(_uint eAnimId);
	void			Update_State(_float fTimeDelta);
	void			LateUpdate_State(_float fTimeDelta);

private:
	ANIM_BLUE m_eState = ANIM_BLUE::IDLE;
	ANIM_RED m_eRedState = ANIM_RED::IDLE1;

public:
	static CStoreNpc* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg = nullptr);
	virtual void Free() override;
};
NS_END

