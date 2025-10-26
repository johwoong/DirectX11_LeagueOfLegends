#include "UI_BackGround.h"
#include "GameInstance.h"

UI_BackGround::UI_BackGround(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CUI_Base(pDevice, pContext)
{
}

UI_BackGround::UI_BackGround(const UI_BackGround& rhs)
	: CUI_Base(rhs)
{
}

HRESULT UI_BackGround::Initialize_Prototype()
{
	return S_OK;
}

HRESULT UI_BackGround::Initialize(void* pArg)
{
	m_eShaderID = UI_DEFAULT;
	m_fSize.x = g_WinSizeX;
	m_fSize.y = g_WinSizeY;
	m_fPosition.x = g_WinSizeX >> 1;
	m_fPosition.y = g_WinSizeY >> 1;
	m_fAlpha = 1.f;

	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	return S_OK;
}

int UI_BackGround::Update(_float fTimeDelta)
{
	__super::Update(fTimeDelta);
}

void UI_BackGround::Late_Update(_float fTimeDelta)
{
	__super::Late_Update(fTimeDelta);
}

HRESULT UI_BackGround::Render()
{
	if (nullptr == m_pShaderCom ||
		nullptr == m_pVIBufferCom)
		return E_FAIL;

	if (FAILED(SetUp_ShaderID()))
		return E_FAIL;

	if (FAILED(SetUp_ShaderResources()))
		return E_FAIL;

	m_pShaderCom->Begin(m_eShaderID);

	m_pVIBufferCom->Render();

	if (FAILED(m_pShaderCom->Set_ShaderResourceView("g_DiffuseTexture", m_pTextureCom->Get_SRV(0))))
		return E_FAIL;



	return S_OK;

}

HRESULT UI_BackGround::Ready_Components(void* pArg)
{
	/* For.Com_Renderer */
	if (FAILED(__super::Add_Components(TEXT("Com_Renderer"), LEVEL_STATIC, TEXT("Prototype_Component_Renderer"), (CComponent**)&m_pRendererCom)))
		return E_FAIL;

	/* For.Com_Transform */
	if (FAILED(__super::Add_Components(TEXT("Com_Transform"), LEVEL_STATIC, TEXT("Prototype_Component_Transform"), (CComponent**)&m_pTransformCom)))
		return E_FAIL;

	/* For.Com_Shader */
	if (FAILED(__super::Add_Components(TEXT("Com_Shader"), LEVEL_STATIC, TEXT("Prototype_Component_Shader_UI"), (CComponent**)&m_pShaderCom)))
		return E_FAIL;

	/* For.Com_Texture */
	if (FAILED(__super::Add_Components(TEXT("Com_Texture"), LEVEL_LOGO, TEXT("Prototype_Component_Texture_Logo"), (CComponent**)&m_pTextureCom)))
		return E_FAIL;

	/* For.Com_VIBuffer */
	if (FAILED(__super::Add_Components(TEXT("Com_VIBuffer"), LEVEL_STATIC, TEXT("Prototype_Component_VIBuffer_Rect"), (CComponent**)&m_pVIBufferCom)))
		return E_FAIL;
}

HRESULT UI_BackGround::SetUp_ShaderResources()
{
	if (nullptr == m_pShaderCom)
		return E_FAIL;
}

UI_BackGround* UI_BackGround::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	return nullptr;
}

CGameObject* UI_BackGround::Clone(void* pArg)
{
	return nullptr;
}

void UI_BackGround::Free()
{
}
