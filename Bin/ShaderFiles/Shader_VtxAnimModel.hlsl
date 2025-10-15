
#include "Client_Shader_Defines.hlsl"

matrix g_WorldMatrix, g_ViewMatrix, g_ProjMatrix;

float g_fMinRange = 5.f;
float g_fMaxRange = 10.f;
float4 g_PlayerPosition;

matrix g_BoneMatrices[400];

vector g_vColor = vector(1.f, 1.f, 1.f, 1);

texture2D g_DiffuseTexture;
texture2D g_NormalTexture;
texture2D g_DepthTexture;


struct VS_IN
{
    float3 vPosition : POSITION;
    float3 vNormal : NORMAL;
    float2 vTexUV : TEXCOORD0;
    float3 vTangent : TANGENT;
    uint4 vBlendIndex : BLENDINDEX;
    float4 vBlendWeight : BLENDWEIGHT;
};

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float3 vNormal : NORMAL;
    float2 vTexUV : TEXCOORD0;
    float4 vProjPos : TEXCOORD1;
    float3 vTangent : TANGENT;
    float3 vBinormal : BINORMAL;
    float4 vWorldPos : TEXCOORD2;
};

VS_OUT VS_MAIN(VS_IN In)
{
    VS_OUT Out = (VS_OUT) 0;

    matrix matWV, matWVP;
    matWV = mul(g_WorldMatrix, g_ViewMatrix);
    matWVP = mul(matWV, g_ProjMatrix);

    float fW = 1.f - (In.vBlendWeight.x + In.vBlendWeight.y + In.vBlendWeight.z);

    matrix BoneMatrix = g_BoneMatrices[In.vBlendIndex.x] * In.vBlendWeight.x +
		g_BoneMatrices[In.vBlendIndex.y] * In.vBlendWeight.y +
		g_BoneMatrices[In.vBlendIndex.z] * In.vBlendWeight.z +
		g_BoneMatrices[In.vBlendIndex.w] * fW;

    vector vPosition = mul(vector(In.vPosition, 1.f), BoneMatrix);
    vector vNormal = mul(vector(In.vNormal, 0.f), BoneMatrix);
    vNormal = normalize(mul(vNormal, g_WorldMatrix));

	/* 정점의 위치에 월드 뷰 투영행렬을 곱한다. 현재 정점은 ViewSpace에 존재하낟. */
	/* 투영행렬까지 곱하면 정점위치의 w에 뷰스페이스 상의 z를 보관한다. == Out.vPosition이 반드시 float4이어야하는 이유. */
    Out.vPosition = mul(vPosition, matWVP);
    Out.vNormal = vNormal.xyz;
    Out.vTexUV = In.vTexUV;
    Out.vProjPos = Out.vPosition;
    Out.vTangent = normalize(mul(vector(In.vTangent, 0.f), g_WorldMatrix)).xyz;
    Out.vBinormal = cross(Out.vNormal, Out.vTangent);

    return Out;
}

struct PS_IN
{
    float4 vPosition : SV_POSITION;
    float3 vNoraml : NORMAL;
    float2 vTexUV : TEXCOORD0;
    float4 vProjPos : TEXCOORD1;
    
    float3 vTangent : TANGENT;
    float3 vBinormal : BINORMAL;
    
    float4 vWorldPos : TEXCOORD2;
};

struct PS_OUT
{
    float4 vDiffuse : SV_TARGET0;
    float4 vNormal : SV_TARGET1;
    float4 vDepth : SV_TARGET2;
};

PS_OUT PS_MAIN(PS_IN In)
{
    PS_OUT Out = (PS_OUT) 0;
    
    float3 vNoraml = g_NormalTexture.Sample(LinearSampler, In.vTexUV).xyz;
    vNoraml = vNoraml * 2.f - 1.f;
    
    float3x3 WorldMatrix = float3x3(In.vTangent, In.vBinormal, In.vNoraml);
    vNoraml = mul(vNoraml, WorldMatrix);
    
    Out.vDiffuse = g_DiffuseTexture.Sample(LinearSampler, In.vTexUV);
    Out.vNormal = vector(vNoraml * 0.5f + 0.5f, 0.f);
    Out.vDepth = vector(In.vProjPos.z / In.vProjPos.w, In.vProjPos.w / 1000.f, 0.f, 0.f);
    
    if(Out.vDiffuse.a <= 0.f)
        discard;
    
    return Out;
    
}

technique11 DefaultTechnique
{   
    pass Default // 0
    {   
        SetRasterizerState(RS_Default);
        SetBlendState(BS_Default, float4(0.f, 0.f, 0.f, 1.f), 0xffffffff);
        SetDepthStencilState(DSS_Default, 0);
        
        VertexShader = compile vs_5_0 VS_MAIN();
        GeometryShader = NULL;
        PixelShader = compile ps_5_0 PS_MAIN();
    }
}