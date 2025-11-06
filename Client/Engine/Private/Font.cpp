#include "Font.h"

CFont::CFont(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: m_pDevice(pDevice)
	, m_pContext(pContext)
{
	Safe_AddRef(m_pDevice);
	Safe_AddRef(m_pContext);
}

HRESULT CFont::Initialize(const _tchar* pFontFilePath)
{
	m_pFont = new SpriteFont(m_pDevice, pFontFilePath);
	m_pSprite = new SpriteBatch(m_pContext);

	return S_OK;
}

HRESULT CFont::Render(const _tchar* pText, _fvector vPosition, _fvector vColor, _float fScale)
{
	m_pContext->GSSetShader(nullptr, nullptr, 0);

	m_pSprite->Begin();

	m_pFont->DrawString(m_pSprite, pText, vPosition, vColor, 0, g_XMZero, fScale);

	m_pSprite->End();

	return S_OK;
}

CFont* CFont::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const _tchar* pFontFilePath)
{
	CFont* pInstance = new CFont(pDevice, pContext);

	if (FAILED(pInstance->Initialize(pFontFilePath)))
	{
		MSG_BOX("Failed to Created : CCustomFont");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CFont::Free()
{
	Safe_Delete(m_pFont);
	Safe_Delete(m_pSprite);

	Safe_Release(m_pDevice);
	Safe_Release(m_pContext);
}