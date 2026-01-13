#pragma once
#include "UI_Base.h"
class CUI_SkillImage final : public CUI_Base
{
public:
    struct SkillDesc
    {
        std::wstring IconTag;
        int SlotIndex;
        float Cooldown;
    };

private:
    // ★ 아이콘 변경용 상태 저장
    int          m_iSlotIndex = -1;
    std::wstring m_CurIconTag;

private:
    CUI_SkillImage(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CUI_SkillImage(const CUI_SkillImage& rhs);
    virtual ~CUI_SkillImage() = default;

public:
    virtual HRESULT Initialize_Prototype();
    virtual HRESULT Initialize(void* pArg);
    virtual int Update(_float fTimeDelta);
    virtual void Late_Update(_float fTimeDelta);
    virtual HRESULT Render();

private:
    virtual HRESULT Ready_Components(void* pArg) override;
    HRESULT SetUp_ShaderResources();

public:
    void Set_Icon(const std::wstring& IconTag);

public:
    static CUI_SkillImage* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject* Clone(void* pArg = nullptr);
    virtual void Free() override;

};

