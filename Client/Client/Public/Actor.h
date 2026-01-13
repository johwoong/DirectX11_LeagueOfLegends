#pragma once
#include "GameObject.h"
#include "Client_Defines.h"
#include "Transform.h"

NS_BEGIN(Engine)
class CRenderer;
class CTransform;
class CShader;
class CColider;
NS_END

NS_BEGIN(Client)
class CEffect;
class CActor abstract :public CGameObject
{
public:
	enum class MARK_TYPE : uint8_t
	{
		NONE = 0,
		EZREAL_W,
		LEE_SIN_Q,
		LEE_SIN_E, 
		VAYNE_SILVER,
	};

public:
	struct TEEMO_POISON_DATA
	{
		bool    bActive = false;
		float   fAcc = 0.f;          // 경과 시간
		float   fDuration = 0.f;     // 총 지속 시간(3초)
		float   fTickAcc = 0.f;      // 틱 누적
		float   fTickInterval = 1.f; // 1초마다 한 번
		int     iDamagePerTick = 0;  // 틱당 데미지
		CActor* pCaster = nullptr;   // 데미지 가해자(티모)
	};

	struct TEEMO_BLIND_DATA
	{
		bool    bActive = false;
		float   fAcc = 0.f;
		float   fDuration = 0.f;
		CActor* pCaster = nullptr;
	};

	struct MARK_DATA
	{
		bool        bActive = false;
		MARK_TYPE   eType = MARK_TYPE::NONE;

		float       fAcc = 0.f;
		float       fDuration = 0.f;

		CActor* pCaster = nullptr;

		int         iHitCount = 0;   // ? 베인용 스택
		_float3     vOffset = { 0.f, 1.1f, 0.f };
		std::vector<CEffect*> fx;
		vector<CEffect*> fxDown;   // QDown 같은 "추가 FX"
		_float3 vDownOffset = _float3(0.f, 0.f, 0.f);

	};
protected:
	CActor(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CActor(const CActor& rhs);
	virtual ~CActor() = default;

public:
	virtual HRESULT Initialize_Prototype();
	virtual HRESULT Initialize(void* pArg);
	virtual int Update(_float fTimeDelta);
	virtual void Late_Update(_float fTimeDelta);
	virtual HRESULT Render();


public:
	virtual _bool Picking(_float3* PickingPoint) { return false; }
	virtual void PickingTrue() {};
	_bool Is_Frustum(_float fOffset = 0);
	void SetUp_BillBoard();
	void Update_Collider();

public: // 마크용 
	void Apply_Mark(MARK_TYPE eType, CActor* pCaster, float fDuration,
		const _tchar* pMarkDat, const _float3& vOffset);
	void Set_MarkOffset(const _float3& v) { m_tMark.vOffset = v; } // 툴에서 호출용
	_float3 Get_MarkOffset() const { return m_tMark.vOffset; }
	bool Has_Mark(MARK_TYPE type) const;
	bool Pop_EzrealW_Mark(CActor* pDamageCauser, int iPopDamage, const _tchar* pPopDat);
	void Apply_VayneSilverMark(CActor* pCaster);
public:
	// 재적용: 시간만 리셋(스택X)
	void Apply_TeemoPoison(CActor* pCaster, float fDuration, int iDamagePerTick, float fTickInterval = 1.f);
	void Clear_TeemoPoison();
	bool Has_TeemoPoison() const { return m_tTeemoPoison.bActive; }
public:
	void Apply_TeemoBlind(CActor* pCaster, float fDuration);
	void Clear_TeemoBlind();
	bool Has_TeemoBlind() const { return m_tTeemoBlind.bActive; }
protected:
	void Update_TeemoBlind(float dt);
protected:
	void Update_TeemoPoison(float dt);
public:
	_float3 Get_Scale() { return m_vScale; }
	CTransform* Get_Transform() { return m_pTransformCom; }
	CRenderer* Get_Renderer(void) { return m_pRendererCom; }
	_vector Get_TransformState(CTransform::STATE eState);
	void Set_State(CTransform::STATE eState, _fvector vState);
	void Set_Scale(_float3 vScale);
	virtual void Set_Picked(_bool type) { m_bPicked = type; }
	CColider* Get_Collider();
	CColider* Get_AABBCollider() { return m_pAABBCom; }
	CColider* Get_OBBCollider() { return m_pOBBCom; }
	CColider* Get_SphereCollider() { return m_pSphereCom; }
	CActor* Get_MarkCaster(MARK_TYPE type) const;
	void    Remove_Mark(MARK_TYPE type);
public:
	virtual _int Take_Damage(int iDamage, CActor* DamageCauser) { return 0; }
	virtual bool IsDeadForTargeting() const { return m_bDead; }
protected:
	virtual HRESULT Ready_Components(void* pArg) = 0;
	virtual HRESULT SetUp_ShaderID() { return S_OK; }
	virtual HRESULT SetUp_ShaderResources() = 0;

protected:
	void Update_Mark(float dt);
	void Clear_Mark();


protected:
	// Component
	CShader*				m_pShaderCom = { nullptr };
	CRenderer*				m_pRendererCom = { nullptr };
	CTransform*				m_pTransformCom = { nullptr };

	CColider*				m_pAABBCom = { nullptr };
	CColider*				m_pOBBCom = { nullptr };
	CColider*				m_pSphereCom = { nullptr };


protected:
	_uint	m_eShaderID = SHADER_NOANIM_DEFAULT;
	_float3 m_vScale = _float3(1.f, 1.f, 1.f);
	_float3 m_vMousePickedPos;
	_bool m_bPicked = false;
	_float m_fAlpha = 1.f;

protected:
	MARK_DATA m_tMark;
	TEEMO_POISON_DATA m_tTeemoPoison;
	vector<CEffect*> m_vecTeemoPoisonFX;
	TEEMO_BLIND_DATA        m_tTeemoBlind;
	vector<CEffect*>        m_vecTeemoBlindFX;
public:
	virtual void Free() override;

};

NS_END

