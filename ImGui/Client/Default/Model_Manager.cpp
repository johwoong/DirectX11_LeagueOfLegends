#include "pch.h"
#include "Model_Manager.h"
#include "GameInstance.h"
#include "Model.h"
#include "GameObject.h"

IMPLEMENT_SINGLETON(CModel_Manager)

CModel_Manager::CModel_Manager()
{

}

HRESULT CModel_Manager::Create_Model(LEVEL eLevel, const _tchar* pPrototypeTag, const _tchar* pLayerTag, ID3D11Device* pDevice, ID3D11DeviceContext* pContext, CModel::TYPE eModelType, _fmatrix PivotMatrix, _bool bCreatePrototype)
{
	return E_NOTIMPL;
}

void CModel_Manager::Add_PrototypeTag(const _tchar* PrototypeTag)
{
}

void CModel_Manager::Add_FileName(const _tchar* LayerTag, const _tchar* FileName)
{
}

void CModel_Manager::Set_AllPickedFalse()
{
}

void CModel_Manager::Out_CreateModel(CNonAnim* pGameObject)
{
}

void CModel_Manager::Clear_Layer()
{
}

const _tchar* CModel_Manager::Find_ModelTag(const _tchar* ModelTag)
{
	return nullptr;
}

void CModel_Manager::Free()
{
}
