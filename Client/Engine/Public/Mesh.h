
#pragma once;

#include "VIBuffer.h"
#include "Model.h"

struct aiMesh;

NS_BEGIN(Engine)
class CMesh final : public CVIBuffer
{
private:
    CMesh(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CMesh(const CMesh& rhs);
    virtual ~CMesh() = default;

public:
    _uint Get_MaterialIndex() const { return m_iMaterialIndex; }
    void  Get_BoneMatricese(_float4x4* pBoneMatrices, _fmatrix PivotMatrix);

public:
    // 바이너리 로더용
    virtual HRESULT Initialize_Prototype(CModel::TYPE eModelType, HANDLE hFile, _ulong* pdwByte, class CModel* pModel, _fmatrix PivotMatrix);

    // FBX/Assimp 로더용
    HRESULT Initialize_FromAiMesh(CModel::TYPE eType, const aiMesh* aimesh, class CModel* pModel, _fmatrix PivotMatrix);

    virtual HRESULT Initialize(void* pArg);

    // Bin 경로
    virtual HRESULT Bin_Initialize_Prototype(CModel::TYPE eModelType, DATA_BINMESH* pAIMesh, class CModel* pModel, _fmatrix PivotMatrix);
    virtual HRESULT Bin_Initialize(void* pArg);

    char* Get_Name() { return m_szName; }

public:
    HRESULT SetUp_Bones(HANDLE hFile, _ulong* pdwByte, class CModel* pModel);
    HRESULT Bin_SetUp_Bones(class CModel* pModel, DATA_BINMESH* pAIMesh);
    _bool   Picking(CTransform* pTransform, _float3* pOut);

private:
    char                m_szName[MAX_PATH] = "";
    const aiMesh* m_pAIMesh = { nullptr };
    _uint               m_iMaterialIndex = 0;

    _uint                       m_iNumBones = 0;
    vector<class CBone*>        m_Bones;

public:
    // FBX/Assimp 로더용 버텍스 버퍼 생성
    HRESULT Create_VertexBuffer_NonAnimModel(const aiMesh* pAIMesh, _fmatrix PivotMatrix);
    HRESULT Create_VertexBuffer_AnimModel(const aiMesh* pAIMesh, class CModel* pModel);

    // 바이너리 로더용 버텍스 버퍼 생성
    HRESULT Create_VertexBuffer_NonAnimModel(HANDLE hFile, _ulong* pdwByte, _fmatrix PivotMatrix);
    HRESULT Create_VertexBuffer_AnimModel(HANDLE hFile, _ulong* pdwByte, class CModel* pModel);

private:
    HRESULT Bin_Create_VertexBuffer_NonAnimModel(DATA_BINMESH* pAIMesh, _fmatrix PivotMatrix);
    HRESULT Bin_Create_VertexBuffer_AnimModel(DATA_BINMESH* pAIMesh, CModel* pModel);

    // ★ FBX/Assimp용 인덱스 버퍼 생성 (추가)
    HRESULT Build_IndexBuffer_FromAiMesh(const aiMesh* pAIMesh);

public:
    void Get_MeshData(DATA_BINMESH* pMeshData);

private:
    VTXMODEL* m_pNonAnimVertices = { nullptr };
    VTXANIMMODEL* m_pAnimVertices = { nullptr };
    FACEINDICES32* m_pIndices = { nullptr };
    _bool            m_bIsProto = false;

private:
    _float3* m_pPickingVertices = { nullptr };
    FACEINDICES32* m_pPickingIndices = { nullptr };

public:
    static CMesh* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, CModel::TYPE eModelType, HANDLE hFile, _ulong* pdwByte, CModel* pModel, _fmatrix PivotMatrix);
    static CMesh* Bin_Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, CModel::TYPE eModelType, DATA_BINMESH* pAIMesh, class CModel* pModel, _fmatrix PivotMatrix);

    // ★ FBX/Assimp용 팩토리 (추가)
    static CMesh* CreateFromAiMesh(ID3D11Device* pDevice, ID3D11DeviceContext* pContext,
        CModel::TYPE eType, const aiMesh* aimesh, class CModel* pModel, _fmatrix PivotMatrix);

    virtual CComponent* Clone(void* pArg = nullptr) override;
    virtual void Free() override;
};
NS_END

