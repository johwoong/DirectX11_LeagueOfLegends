#ifndef Engine_Struct_h__
#define Engine_Struct_h__

namespace Engine
{
	typedef struct tagKeyframe
	{
		float				fTime;
		XMFLOAT3			vScale;
		XMFLOAT4			vRotation;
		XMFLOAT3			vPosition;
	}KEYFRAME;

	typedef struct tagModelMaterial
	{
		class CTexture* pMaterials[AI_TEXTURE_TYPE_MAX];
	}MODELMATERIAL;



	typedef struct tagGraphicDesc
	{
		enum WINMODE {MODE_FULL, MODE_WIN, MODE_END};
		HWND			hWnd;
		unsigned int	iWinSizeX;
		unsigned int	iWinSizeY;
		WINMODE			eWindowMode;
	}GRAPHIC_DESC;

	typedef struct tagLineIndices16
	{
		unsigned short		_0, _1;
	}LINEINDICES16;

	typedef struct tagLineIndices32
	{
		unsigned long		_0, _1;
	}LINEINDICES32;

	typedef struct tagFaceIndices16
	{
		unsigned short		_0, _1, _2;
	}FACEINDICES16;

	typedef struct tagFaceIndices32
	{
		unsigned long		_0, _1, _2;
	}FACEINDICES32;

	typedef struct tagVertexPosition
	{
		XMFLOAT3		vPosition;
	}VTXPOS;

	typedef struct tagVertexModel
	{
		XMFLOAT3	vPosition;
		XMFLOAT3	vNormal;
		XMFLOAT2	vTexUV;
		XMFLOAT3	vTangent;
	}VTXMODEL;

	typedef struct ENGINE_DLL tagVertexModel_Declaration
	{
		static const unsigned int iNumElements = 4;
		static const D3D11_INPUT_ELEMENT_DESC Elements[iNumElements];
	}VTXMODEL_DECLARATION;

	typedef struct ENGINE_DLL tagVertexPosition_Declaration
	{
		static const unsigned int iNumElements = 1;
		static const D3D11_INPUT_ELEMENT_DESC Elements[iNumElements];
	}VTXPOS_DECLARATION;

	typedef struct tagVertexMatrix
	{
		XMFLOAT4 vRight;
		XMFLOAT4 vUp;
		XMFLOAT4 vLook;
		XMFLOAT4 vPosition;
		float fAlpha;
		XMFLOAT3 vColor;
	}VTXMATRIX;

	// 애니메이션 재생이 필요한 모델에게 필요한 정점.
	typedef struct tagVertexAnimModel
	{
		XMFLOAT3	vPosition;
		XMFLOAT3	vNormal;
		XMFLOAT2	vTexUV;
		XMFLOAT3	vTangent;
		XMUINT4		vBlendIndex; // 정점에게 영향을 주는 뼈의 인덱스
		XMFLOAT4	vBlendWeight;	// 정점에게 영향을 주는 뼈 상태의 비율
	}VTXANIMMODEL;
	
	typedef struct tagVertexTexture
	{
		XMFLOAT3		vPosition;
		XMFLOAT2		vTexture;
	}VTXTEX;

	typedef struct ENGINE_DLL tagVertexTexture_Declaration
	{
		static const unsigned int iNumElements = 2;
		static const D3D11_INPUT_ELEMENT_DESC Elements[iNumElements];
	}VTXTEX_DECLARATION;

	// Data

	typedef struct tagBinBoneNode
	{
		char cName[MAX_PATH];
		char cParent[MAX_PATH];
		int  iDepth;
		XMFLOAT4X4 mTransform;
	}DATA_BINNODE;


	typedef struct tagBinBone
	{
		char	cNames[MAX_PATH];
		XMFLOAT4X4 m_OffsetTransform;
	}DATA_BINBONE;

	typedef struct tagBinMesh
	{
		char			cName[MAX_PATH];
		int				iMaterialIndex;

		int				NumVertices;
		VTXMODEL* pNonAnimVertices;
		VTXANIMMODEL* pAnimVertices;

		int				iNumPrimitives;
		FACEINDICES32*	pIndices;

		int				iNumBones;
		DATA_BINBONE* pBones;
	}DATA_BINMESH;

	typedef struct tagBinChannel
	{

		char				szName[MAX_PATH];
		int					iNumKeyFrames;
		KEYFRAME* pKeyFrames;

	}DATA_BINCHANNEL;

	typedef struct tagBinAnim
	{

		bool				bLoop;
		char				szName[MAX_PATH];
		int					iNumChannels;
		float				fDuration;
		float				fTickPerSecond;
		DATA_BINCHANNEL* pBinChannel;


	}DATA_BINANIM;

	typedef struct NonAnimModelTag
	{
		char pModeltag[MAX_PATH] = "";
		XMFLOAT3	vPosition = XMFLOAT3(0.f, 0.f, 0.f);
		XMFLOAT3	vScale = XMFLOAT3(1.f, 1.f, 1.f);
		XMFLOAT3	vRotation = XMFLOAT3(0.f, 0.f, 0.f);
		float		m_fAngle = 0.f;
		XMFLOAT4X4 WorldMatrix;

	}NONANIMDESC;

	typedef struct tagJBinBone
	{
		char szName[MAX_PATH];
		XMFLOAT4X4 Transformation;
		unsigned int iNumChildren;
	}BINBONE;

	typedef struct tagBinAnimation
	{
		char szName[MAX_PATH];
		double dbDuration;
		double dbTickPerSecond;

		int iNumChannels;
	}BINANIM;

	typedef struct tagJBinChannel
	{
		char szName[MAX_PATH];
		int iNumKeyFrames;
	}BINCHANNEL;

	typedef struct tagAnimationEvent
	{
		enum EVENTTYPE { EVENT_SOUND, EVENT_EFFECT, EVENT_COLLIDER, EVENT_STATE, EVENT_INPUT, EVENT_END };

		bool isPlay = false;
		float fStartTime;
		float fEndTime;
		EVENTTYPE eType;
		char szName[MAX_PATH];
	}ANIMEVENT;

}


#endif // Engine_Struct_h__
