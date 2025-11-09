#pragma once
#include "UI_Base.h"
#include "UI_SkillImage.h"
#include "Champion.h"     
#include <vector>

NS_BEGIN(Client)

class CUI_SkillMenu final : public CUI_Base
{
private:
    CUI_SkillMenu(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CUI_SkillMenu(const CUI_SkillMenu& rhs);
    virtual ~CUI_SkillMenu() = default;

public:
    virtual HRESULT Initialize_Prototype();
    virtual HRESULT Initialize(void* pArg);
    virtual int Update(_float fTimeDelta);
    virtual void Late_Update(_float fTimeDelta);
    virtual HRESULT Render();

public:
    // ? Champion의 내부 구조체 사용 → include로 정의를 가져와야 함.
    void Set_SkillIcons(const CChampion::SkillInfo* pSkills);

private:
    virtual HRESULT Ready_Components(void* pArg) override;
    HRESULT SetUp_ShaderResources();

private:
    std::vector<CUI_SkillImage*> m_pSkillImages;

public:
    static CUI_SkillMenu* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject* Clone(void* pArg = nullptr);
    virtual void Free() override;
};

NS_END

