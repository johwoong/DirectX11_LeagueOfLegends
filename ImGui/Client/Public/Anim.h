#pragma once
#include "Actor.h"

NS_BEGIN(Engine)
class CModel;
NS_END

NS_BEGIN(Engine)
class CAnim final : public CActor
{
public:
	typedef struct tagModelDesc
	{
		_float3 vPosition;
		_float3 vScale;
		_float3 vRotation;
		MODEL_ID eModel;
		_bool isOriginalLoad = false;
	}MODELDESC;


private:
	CAnim(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CAnim(const CAnim& rhs);
	virtual ~CAnim() = default;

public:
	void Set_AnimIndex(_int iIndex);
	void StartAnim(void) { m_isPlayAnim = true; }
	void StopAnim(void) { m_isPlayAnim = false; }

public:
	void PlayAnimation(void);

public:
	virtual HRESULT Initialize_Prototype(void);
	virtual HRESULT Initialize(void* pArg);
	virtual int Update(_float fTimeDelta);
	virtual void Late_Update(_float fTimeDelta);
	virtual HRESULT Render(void);

public:
	MODELDESC Get_Desc(void) { return m_ModelDesc; }
	void Set_ModelDesc(NONANIMDESC* NonAnimDesc) { memcpy(&m_ModelDesc, NonAnimDesc, sizeof(NONANIMDESC)); }

private:
	CModel* m_pModelCom = { nullptr };
	_bool m_isPlayAnim = false;
	MODELDESC m_ModelDesc;
	_bool m_bIsFinished = false;

private:
	virtual HRESULT Ready_Components(void* pArg) override;
	virtual HRESULT SetUp_ShaderResources(void) override;
	virtual HRESULT SetUp_ShaderID(void) override;

public:
	static CAnim* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject* Clone(void* pArg = nullptr);
	virtual void Free(void) override;
};

NS_END