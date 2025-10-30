#include "VIBuffer_Terrian.h"
#include "Transform.h"
#include "Picking.h"


CVIBuffer_Terrian::CVIBuffer_Terrian(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CVIBuffer(pDevice, pContext)
{
}

CVIBuffer_Terrian::CVIBuffer_Terrian(const CVIBuffer_Terrian& rhs)
    : CVIBuffer(rhs)
    , m_iNumVerticesX(rhs.m_iNumVerticesX)
    , m_iNumVerticesZ(rhs.m_iNumVerticesZ)
{
    memcpy(&m_pTerrianDesc, &rhs.m_pTerrianDesc, sizeof(TERRIANDESC));
}



HRESULT CVIBuffer_Terrian::Initialize_Prototype(_uint iNumVerticeX, _uint iNumVerticeZ)
{
    return S_OK;
}


HRESULT CVIBuffer_Terrian::Initialize(void* pArg)
{
    if (pArg == nullptr)
        return S_OK;
    
    memcpy(&m_pTerrianDesc, pArg, sizeof(TERRIANDESC));

#pragma region VERTEXBUFFER
	m_iNumVertexBuffers = 1;
	m_iNumVerticesX = (m_pTerrianDesc).m_iVerticeNumX;
	m_iNumVerticesZ = (m_pTerrianDesc).m_iVerticeNumZ;


	m_iNumVertices = m_iNumVerticesX * m_iNumVerticesZ;
	m_iStride = sizeof(VTXNORTEX);

	VTXNORTEX* pVertices = new VTXNORTEX[m_iNumVertices];
	ZeroMemory(pVertices, sizeof(VTXNORTEX) * m_iNumVertices);

	m_pVerticesPos = new _float3[m_iNumVertices];

	for (_uint i = 0; i < m_iNumVerticesZ; ++i)
	{
		for (_uint j = 0; j < m_iNumVerticesX; ++j)
		{
			_int		iIndex = i * m_iNumVerticesX + j;

			pVertices[iIndex].vPosition = m_pVerticesPos[iIndex] = _float3((_float)j, -0.f, (_float)i);
			pVertices[iIndex].vNormal = _float3(0.f, 0.f, 0.f);
			pVertices[iIndex].vTexture = _float2(j / _float(m_iNumVerticesX - 1), i / _float(m_iNumVerticesZ - 1));
		}
	}
#pragma endregion 

#pragma region INDEXBUFFER
	m_iNumPrimitive = (m_iNumVerticesX - 1) * (m_iNumVerticesZ - 1) * 2;
	m_iIndicesByte = sizeof(FACEINDICES32);
	m_iNumIndicesPerPrimitive = 3;
	m_eFormat = DXGI_FORMAT_R32_UINT;
	m_eTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	FACEINDICES32* pIndices = new FACEINDICES32[m_iNumPrimitive];
	ZeroMemory(pIndices, sizeof(FACEINDICES32) * m_iNumPrimitive);


	_uint			iNumFaces = 0;

	for (_uint i = 0; i < m_iNumVerticesZ - 1; ++i)
	{
		for (_uint j = 0; j < m_iNumVerticesX - 1; ++j)
		{
			_uint		iIndex = i * m_iNumVerticesX + j;

			_uint		iIndices[4] = {
				iIndex + m_iNumVerticesX,
				iIndex + m_iNumVerticesX + 1,
				iIndex + 1,
				iIndex
			};

			_vector		vSourDir, vDestDir, vNormal;

			pIndices[iNumFaces]._0 = iIndices[0];
			pIndices[iNumFaces]._1 = iIndices[1];
			pIndices[iNumFaces]._2 = iIndices[2];

			vSourDir = XMLoadFloat3(&pVertices[pIndices[iNumFaces]._1].vPosition) - XMLoadFloat3(&pVertices[pIndices[iNumFaces]._0].vPosition);
			vDestDir = XMLoadFloat3(&pVertices[pIndices[iNumFaces]._2].vPosition) - XMLoadFloat3(&pVertices[pIndices[iNumFaces]._1].vPosition);

			vNormal = XMVector3Normalize(XMVector3Cross(vSourDir, vDestDir));

			XMStoreFloat3(&pVertices[pIndices[iNumFaces]._0].vNormal,
				XMLoadFloat3(&pVertices[pIndices[iNumFaces]._0].vNormal) + vNormal);
			XMStoreFloat3(&pVertices[pIndices[iNumFaces]._1].vNormal,
				XMLoadFloat3(&pVertices[pIndices[iNumFaces]._1].vNormal) + vNormal);
			XMStoreFloat3(&pVertices[pIndices[iNumFaces]._2].vNormal,
				XMLoadFloat3(&pVertices[pIndices[iNumFaces]._2].vNormal) + vNormal);

			++iNumFaces;

			pIndices[iNumFaces]._0 = iIndices[0];
			pIndices[iNumFaces]._1 = iIndices[2];
			pIndices[iNumFaces]._2 = iIndices[3];

			vSourDir = XMLoadFloat3(&pVertices[pIndices[iNumFaces]._1].vPosition) - XMLoadFloat3(&pVertices[pIndices[iNumFaces]._0].vPosition);
			vDestDir = XMLoadFloat3(&pVertices[pIndices[iNumFaces]._2].vPosition) - XMLoadFloat3(&pVertices[pIndices[iNumFaces]._1].vPosition);

			vNormal = XMVector3Normalize(XMVector3Cross(vSourDir, vDestDir));

			XMStoreFloat3(&pVertices[pIndices[iNumFaces]._0].vNormal,
				XMLoadFloat3(&pVertices[pIndices[iNumFaces]._0].vNormal) + vNormal);
			XMStoreFloat3(&pVertices[pIndices[iNumFaces]._1].vNormal,
				XMLoadFloat3(&pVertices[pIndices[iNumFaces]._1].vNormal) + vNormal);
			XMStoreFloat3(&pVertices[pIndices[iNumFaces]._2].vNormal,
				XMLoadFloat3(&pVertices[pIndices[iNumFaces]._2].vNormal) + vNormal);

			++iNumFaces;

		}
	}
#pragma endregion 

	ZeroMemory(&m_BufferDesc, sizeof(D3D11_BUFFER_DESC));
	m_BufferDesc.ByteWidth = m_iNumVertices * m_iStride;
	m_BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	m_BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	m_BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	m_BufferDesc.MiscFlags = 0;
	m_BufferDesc.StructureByteStride = m_iStride;

	ZeroMemory(&m_SubResourceData, sizeof(D3D11_SUBRESOURCE_DATA));
	m_SubResourceData.pSysMem = pVertices;

	if (FAILED(__super::Create_VertexBuffer()))
		return E_FAIL;

	ZeroMemory(&m_BufferDesc, sizeof(D3D11_BUFFER_DESC));
	m_BufferDesc.ByteWidth = m_iNumPrimitive * m_iIndicesByte;
	m_BufferDesc.Usage = D3D11_USAGE_DEFAULT;
	m_BufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	m_BufferDesc.CPUAccessFlags = 0;
	m_BufferDesc.MiscFlags = 0;
	m_BufferDesc.StructureByteStride = 0;

	ZeroMemory(&m_SubResourceData, sizeof(D3D11_SUBRESOURCE_DATA));
	m_SubResourceData.pSysMem = pIndices;

	if (FAILED(__super::Create_IndexBuffer()))
		return E_FAIL;


	Safe_Delete_Array(pVertices);
	Safe_Delete_Array(pIndices);

	return S_OK;
}

_bool CVIBuffer_Terrian::Picking(CTransform* pTransform, _float3* pOut)
{
	CPicking* pPicking = CPicking::GetInstance();
	pPicking->AddRef();

	_matrix WorldMatrixInv = pTransform->Get_WorldMatrixInverse();

	_float3 vTempRayDir, vTempRayPos;
	XMVECTOR vRayDir, vRayPos;
	pPicking->Compute_LocalRayInfo(&vTempRayDir, &vTempRayPos, pTransform);

	vRayPos = XMLoadFloat3(&vTempRayPos);
	vRayPos = XMVectorSetW(vRayPos, 1.f);
	vRayDir = XMLoadFloat3(&vTempRayDir);
	vRayDir = XMVector3Normalize(vRayDir);

	for (_uint i = 0; i < m_iNumVerticesZ - 1; ++i)
	{
		for (_uint j = 0; j < m_iNumVerticesX - 1; ++j)
		{
			_uint		iIndex = i * m_iNumVerticesX + j;

			//	if (false == pFrustum->isIn_WorldFrustum(XMVectorSetW(XMLoadFloat3(&m_pVerticesPos[iIndex]), 1.f), 1))
			//		continue;

			_uint		iIndices[] = {
				iIndex + m_iNumVerticesX,
				iIndex + m_iNumVerticesX + 1,
				iIndex + 1,
				iIndex
			};



			//iIndices[0]이랑 iIndices[2]사이에 직선이 있거나
			//iIndices[1]이랑 iIndices[3]사이에 직선이 있으면 픽킹 체크 
			_float discriminant0 = (vTempRayDir.z * (m_pVerticesPos[iIndices[0]].x - vTempRayPos.x) + vTempRayPos.z) / vTempRayDir.x - m_pVerticesPos[iIndices[0]].z;
			_float discriminant1 = (vTempRayDir.z * (m_pVerticesPos[iIndices[1]].x - vTempRayPos.x) + vTempRayPos.z) / vTempRayDir.x - m_pVerticesPos[iIndices[1]].z;
			_float discriminant2 = (vTempRayDir.z * (m_pVerticesPos[iIndices[2]].x - vTempRayPos.x) + vTempRayPos.z) / vTempRayDir.x - m_pVerticesPos[iIndices[2]].z;
			_float discriminant3 = (vTempRayDir.z * (m_pVerticesPos[iIndices[3]].x - vTempRayPos.x) + vTempRayPos.z) / vTempRayDir.x - m_pVerticesPos[iIndices[3]].z;


			if (discriminant0 * discriminant2 < 0 && discriminant1 * discriminant3 < 0)
				continue;


			_float		fDist;
			_matrix	WorldMatrix = pTransform->Get_WorldMatrix();


			_vector vTemp_1 = XMLoadFloat3(&m_pVerticesPos[iIndices[0]]);
			vTemp_1 = XMVectorSetW(vTemp_1, 1.f);
			_vector vTemp_2 = XMLoadFloat3(&m_pVerticesPos[iIndices[1]]);
			vTemp_2 = XMVectorSetW(vTemp_2, 1.f);
			_vector vTemp_3 = XMLoadFloat3(&m_pVerticesPos[iIndices[2]]);
			vTemp_3 = XMVectorSetW(vTemp_3, 1.f);
			if (true == TriangleTests::Intersects((FXMVECTOR)vRayPos, (FXMVECTOR)vRayDir, (FXMVECTOR)vTemp_1, (GXMVECTOR)vTemp_2, (HXMVECTOR)vTemp_3, fDist))
			{
				_vector	vPickPos = vRayPos + vRayDir * fDist;
				XMStoreFloat3(&m_bPickingLocalPos, vPickPos);
				XMStoreFloat3(pOut, XMVector3TransformCoord(vPickPos, WorldMatrix));

				Safe_Release(pPicking);
				return true;
			}

			vTemp_1 = XMLoadFloat3(&m_pVerticesPos[iIndices[0]]);
			vTemp_1 = XMVectorSetW(vTemp_1, 1.f);
			vTemp_2 = XMLoadFloat3(&m_pVerticesPos[iIndices[2]]);
			vTemp_2 = XMVectorSetW(vTemp_2, 1.f);
			vTemp_3 = XMLoadFloat3(&m_pVerticesPos[iIndices[3]]);
			vTemp_3 = XMVectorSetW(vTemp_3, 1.f);
			if (true == TriangleTests::Intersects((FXMVECTOR)vRayPos, (FXMVECTOR)vRayDir, (FXMVECTOR)vTemp_1, (GXMVECTOR)vTemp_2, (HXMVECTOR)vTemp_3, fDist))
			{
				_vector	vPickPos = vRayPos + vRayDir * fDist;

				XMStoreFloat3(pOut, XMVector3TransformCoord(vPickPos, WorldMatrix));

				Safe_Release(pPicking);
				return true;
			}
		}
	}

	pPicking->Release();
    return false();
}

void CVIBuffer_Terrian::Set_Terrain_UpDown(_float fVec, _float fRad, _float fSharp, _float3 vPoint, _float fTimeDelta)
{
	D3D11_MAPPED_SUBRESOURCE SubResource;

	m_pContext->Map(m_pVB, 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &SubResource);

	VTXNORTEX* pVertices = (VTXNORTEX*)SubResource.pData;
	if (pVertices == nullptr)
	{
		m_pContext->Unmap(m_pVB, 0);
		return;
	}

	for (_uint i = 0; i < m_iNumVerticesZ - 1; ++i)
	{
		for (_uint j = 0; j < m_iNumVerticesX - 1; ++j)
		{
			_uint iIndex = i * m_iNumVerticesX + j;

			_float3 vPos = pVertices[iIndex].vPosition;

			_float fH = fVec;
			_float fRange = fRad;

			_float3 vTemp1 = vPoint;
			_float3 vTemp2 = pVertices[iIndex].vPosition;
			vTemp1.y = vTemp2.y = 0.f;

			_vector fDis = XMLoadFloat3(&vTemp1) - XMLoadFloat3(&vTemp2);
			_float fLen = XMVectorGetX(XMVector3Length(fDis));

			if (fRange > fLen)
			{

				_float fAcc = (fRange - fLen) / fRange;

				fAcc = pow(fAcc, (1.f / fSharp));

				_float fTempY = fH * fAcc;
				if (fH > pVertices[iIndex].vPosition.y)
				{
					vPos.y += fTempY * fTimeDelta;
				}
				else if (fH < pVertices[iIndex].vPosition.y)
				{
					vPos.y -= fTempY * fTimeDelta;
				}
				else
					vPos.y = vPos.y;

			}

			pVertices[iIndex].vPosition = vPos;
		}
	}
}

void CVIBuffer_Terrian::Set_Terrain_Buffer(TERRIANDESC TerrainDesc)
{
	D3D11_MAPPED_SUBRESOURCE	SubResource;

	m_pContext->Map(m_pVB, 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &SubResource);

	VTXNORTEX* pVertices = (VTXNORTEX*)SubResource.pData;

	for (_int i = 0; i < TerrainDesc.m_iVerticeNumZ; ++i)
	{
		for (_int j = 0; j < TerrainDesc.m_iVerticeNumX; ++j)
		{
			_int	iIndex = i * m_iNumVerticesX + j;

			pVertices[iIndex].vPosition = m_pVerticesPos[iIndex] = _float3((_float)TerrainDesc.m_iPositionX + j, TerrainDesc.m_fHeight, (_float)TerrainDesc.m_iPositionZ + i);
			pVertices[iIndex].vNormal = _float3(0.f, 0.f, 0.f);
			pVertices[iIndex].vTexture = _float2(j / _float(m_iNumVerticesX - 1), i / _float(m_iNumVerticesZ - 1));
		}
	}
	m_pContext->Unmap(m_pVB, 0);
}

HRESULT CVIBuffer_Terrian::Save_VertexPosition(HANDLE hFile, _ulong* dwByte)
{
	/* 한번 TerrainDesc 정보 읽기 */
	WriteFile(hFile, &m_pTerrianDesc, sizeof(TERRIANDESC), dwByte, nullptr);


	D3D11_MAPPED_SUBRESOURCE		SubResource;

	m_pContext->Map(m_pVB, 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &SubResource);
	VTXNORTEX* pVertices = (VTXNORTEX*)SubResource.pData;

	for (_uint i = 0; i < m_pTerrianDesc.m_iVerticeNumX; ++i)
	{
		for (_uint j = 0; j < m_pTerrianDesc.m_iVerticeNumZ; ++j)
		{
			_uint	iIndex = i * m_pTerrianDesc.m_iVerticeNumX + j;

			WriteFile(hFile, &pVertices[iIndex], sizeof(VTXNORTEX), dwByte, nullptr);  
		}
	}

	m_pContext->Unmap(m_pVB, 0);

	return S_OK;
}

HRESULT CVIBuffer_Terrian::Load_Prototype(HANDLE hFile, _ulong& dwByte, _bool bCreateQuadTree)
{
	VTXNORTEX VertexTag;

	/* 한번 TerrainDesc 정보 읽기 */
	ReadFile(hFile, &m_pTerrianDesc, sizeof(TERRIANDESC), &dwByte, nullptr);

	m_pTerrianDesc.m_isHeightMap = true;
#pragma region VERTEXBUFFER
	 
	m_iNumVerticesX = (m_pTerrianDesc).m_iVerticeNumX;
	m_iNumVerticesZ = (m_pTerrianDesc).m_iVerticeNumZ;

	ZeroMemory(&m_BufferDesc, sizeof(D3D11_BUFFER_DESC));
	m_iStride = sizeof(VTXNORTEX);
	m_iNumVertices = m_iNumVerticesX * m_iNumVerticesZ;
	m_iNumVertexBuffers = 1;
	m_eFormat = DXGI_FORMAT_R32_UINT;
	m_eTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	VTXNORTEX* pVertices = new VTXNORTEX[m_iNumVertices];
	ZeroMemory(pVertices, sizeof(VTXNORTEX) * m_iNumVertices);
	m_pVerticesPos = new _float3[m_iNumVertices];

	for (_uint i = 0; i < m_iNumVerticesZ; ++i)
	{
		for (_uint j = 0; j < m_iNumVerticesX; ++j)
		{
			_int		iIndex = i * m_iNumVerticesX + j;

			ReadFile(hFile, &pVertices[iIndex], sizeof(VTXNORTEX), &dwByte, nullptr);
			m_pVerticesPos[iIndex] = pVertices[iIndex].vPosition;
		}
	}


#pragma endregion

#pragma region INDEXBUFFER
	ZeroMemory(&m_BufferDesc, sizeof(D3D11_BUFFER_DESC));

	m_iIndicesByte = sizeof(FACEINDICES32);
	m_iNumPrimitive = (m_iNumVerticesX - 1) * (m_iNumVerticesZ - 1) * 2;
	m_iNumIndicesPerPrimitive = 3;

	FACEINDICES32* pIndices = new FACEINDICES32[m_iNumPrimitive];

	_uint		iNumFaces = 0;

	for (_uint i = 0; i < m_iNumVerticesZ - 1; i++)
	{
		for (_uint j = 0; j < m_iNumVerticesX - 1; j++)
		{
			_uint		iIndex = i * m_iNumVerticesX + j;

			_uint		iIndices[4] = {
				iIndex + m_iNumVerticesX,
				iIndex + m_iNumVerticesX + 1,
				iIndex + 1,
				iIndex
			};

			pIndices[iNumFaces]._0 = iIndices[0];
			pIndices[iNumFaces]._1 = iIndices[1];
			pIndices[iNumFaces]._2 = iIndices[2];

			_vector		vSourDir, vDestDir, vNormal;

			vSourDir = XMLoadFloat3(&pVertices[pIndices[iNumFaces]._1].vPosition) - XMLoadFloat3(&pVertices[pIndices[iNumFaces]._0].vPosition);
			vDestDir = XMLoadFloat3(&pVertices[pIndices[iNumFaces]._2].vPosition) - XMLoadFloat3(&pVertices[pIndices[iNumFaces]._1].vPosition);
			vNormal = XMVector3Normalize(XMVector3Cross(vSourDir, vDestDir));

			XMStoreFloat3(&pVertices[pIndices[iNumFaces]._0].vNormal, XMLoadFloat3(&pVertices[pIndices[iNumFaces]._0].vNormal) + vNormal);
			XMStoreFloat3(&pVertices[pIndices[iNumFaces]._1].vNormal, XMLoadFloat3(&pVertices[pIndices[iNumFaces]._1].vNormal) + vNormal);
			XMStoreFloat3(&pVertices[pIndices[iNumFaces]._2].vNormal, XMLoadFloat3(&pVertices[pIndices[iNumFaces]._2].vNormal) + vNormal);
			++iNumFaces;

			pIndices[iNumFaces]._0 = iIndices[0];
			pIndices[iNumFaces]._1 = iIndices[2];
			pIndices[iNumFaces]._2 = iIndices[3];

			vSourDir = XMLoadFloat3(&pVertices[pIndices[iNumFaces]._1].vPosition) - XMLoadFloat3(&pVertices[pIndices[iNumFaces]._0].vPosition);
			vDestDir = XMLoadFloat3(&pVertices[pIndices[iNumFaces]._2].vPosition) - XMLoadFloat3(&pVertices[pIndices[iNumFaces]._1].vPosition);
			vNormal = XMVector3Normalize(XMVector3Cross(vSourDir, vDestDir));

			XMStoreFloat3(&pVertices[pIndices[iNumFaces]._0].vNormal, XMLoadFloat3(&pVertices[pIndices[iNumFaces]._0].vNormal) + vNormal);
			XMStoreFloat3(&pVertices[pIndices[iNumFaces]._1].vNormal, XMLoadFloat3(&pVertices[pIndices[iNumFaces]._1].vNormal) + vNormal);
			XMStoreFloat3(&pVertices[pIndices[iNumFaces]._2].vNormal, XMLoadFloat3(&pVertices[pIndices[iNumFaces]._2].vNormal) + vNormal);
			++iNumFaces;
		}
	}

	for (_uint i = 0; i < m_iNumVertices; ++i)
		XMStoreFloat3(&pVertices[i].vNormal, XMVector3Normalize(XMLoadFloat3(&pVertices[i].vNormal)));


	ZeroMemory(&m_BufferDesc, sizeof(D3D11_BUFFER_DESC));
	m_BufferDesc.ByteWidth = m_iNumVertices * m_iStride;
	m_BufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	m_BufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	m_BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	m_BufferDesc.MiscFlags = 0;
	m_BufferDesc.StructureByteStride = m_iStride;

	ZeroMemory(&m_SubResourceData, sizeof(D3D11_SUBRESOURCE_DATA));
	m_SubResourceData.pSysMem = pVertices;

	if (FAILED(__super::Create_VertexBuffer()))
		return E_FAIL;

	m_BufferDesc.ByteWidth = m_iIndicesByte * m_iNumPrimitive;
	m_BufferDesc.Usage = D3D11_USAGE_DYNAMIC; /* 정적버퍼를 생성한다. */
	m_BufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	m_BufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	m_BufferDesc.MiscFlags = 0;
	m_BufferDesc.StructureByteStride = sizeof(_ushort);

	ZeroMemory(&m_SubResourceData, sizeof(D3D11_SUBRESOURCE_DATA));
	m_SubResourceData.pSysMem = pIndices;

	/* 정점을 담기 위한 공간을 할당하고, 내가 전달해준 배열의 값들을 멤카피한다. */
	if (FAILED(__super::Create_IndexBuffer()))
		return E_FAIL;

	Safe_Delete_Array(pVertices);
	Safe_Delete_Array(pIndices);
#pragma endregion

	return S_OK;
}

CVIBuffer_Terrian* CVIBuffer_Terrian::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, _uint iNumVerticeX, _uint iNumVerticeZ, _float fHeight)
{
	CVIBuffer_Terrian* pInstance = new CVIBuffer_Terrian(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype(iNumVerticeX, iNumVerticeZ)))
	{
		MSG_BOX("Failed to Created : CVIBuffer_Terrian");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CVIBuffer_Terrian* CVIBuffer_Terrian::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, HANDLE hFile, _ulong& dwByte)
{
	CVIBuffer_Terrian* pInstance = new CVIBuffer_Terrian(pDevice, pContext);

	if (FAILED(pInstance->Load_Prototype(hFile, dwByte)))
	{
		MSG_BOX("Failed to Created : CVIBuffer_Terrian");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CComponent* CVIBuffer_Terrian::Clone(void* pArg)
{
	CVIBuffer_Terrian* pInstance = new CVIBuffer_Terrian(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CVIBuffer_Terrian");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CVIBuffer_Terrian::Free()
{
	__super::Free();

	if (m_pTerrianDesc.m_isHeightMap == false)
		Safe_Delete_Array(m_pVerticesPos);
}
