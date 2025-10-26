#pragma once
#include "Base.h"
#include "Client_Defines.h"
#include "Model.h"
#include "NonAnim.h"

NS_BEGIN(Client)

class CModelManager :
    public CBase
{
	DECLARE_SINGLETON(CModelManager)

private:
	CModelManager();
	virtual ~CModelManager() = default;

public:
	HRESULT Create_Model(LEVEL eLevel, const _tchar* pPrototypeTag, const _tchar* pLayerTag, ID3D11Device* pDevice, ID3D11DeviceContext* pContext, CModel::TYPE eModelType, _fmatrix PivotMatrix, _bool bCreatePrototype = false);

public:
	const vector<class CNonAnim*> Get_CreatedModel() const { return m_CreatedModel; }

public:
	void Add_PrototypeTag(const _tchar* PrototypeTag);
	void Add_FileName(const _tchar* Layertag, const _tchar* FileName);
	void Add_CreatedModel(class CNonAnim* pNonAnimModel) { m_CreatedModel.push_back(pNonAnimModel); }
	void Set_AllPickedFalse();
	void Set_InitModelDesc(NONANIMDESC ModelDesc) { memcpy(&m_InitModelDesc, &ModelDesc, sizeof(NONANIMDESC)); }
	void Out_CreatedModel(CNonAnim* pGameObject);
	void Clear_Layer();
	vector<const _tchar*> Get_PrototypeTag() { return m_PrototypeTag; }

private:
	const _tchar* Find_ModelTag(const _tchar* ModelTag);


private:
	map<const _tchar*, const _tchar*>  m_ModelTags;  //Prototype Tag & Path
	vector<const _tchar*> m_PrototypeTag;
	vector<class CNonAnim*> m_CreatedModel;
	NONANIMDESC  m_InitModelDesc;


public:
	void Free() override;
};

NS_END

