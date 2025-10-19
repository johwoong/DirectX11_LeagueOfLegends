
#include "Client_Shader_Defines.hlsl"

matrix g_WorldMatrix;
matrix g_ViewMatrix;
matrix g_ProjMatrix;

matrix g_BoneMatrices[400];

texture2D g_DiffuseTexture;
texture2D g_NormalTexture;
texture2D g_DepthTexture;

vector g_vColor = vector(1.f, 1.f, 1.f, 1.f);


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

    // ? 올바른 순서: Projection * View * World
    matrix matWVP = mul(g_ProjMatrix, mul(g_ViewMatrix, g_WorldMatrix));

    // ? Blend weight normalization
    float4 weights = In.vBlendWeight;
    weights.w = 1.f - (weights.x + weights.y + weights.z);

    // ? Bone transform (DirectX는 column-major)
    matrix BoneMatrix =
        g_BoneMatrices[In.vBlendIndex.x] * weights.x +
        g_BoneMatrices[In.vBlendIndex.y] * weights.y +
        g_BoneMatrices[In.vBlendIndex.z] * weights.z +
        g_BoneMatrices[In.vBlendIndex.w] * weights.w;

    // ? 본 변환
    float4 vPosition = mul(BoneMatrix, float4(In.vPosition, 1.f));
    float3 vNormal = mul((float3x3) BoneMatrix, In.vNormal);

    // ? 월드 좌표 변환
    vPosition = mul(vPosition, g_WorldMatrix);
    vNormal = normalize(mul((float3x3) g_WorldMatrix, vNormal));

    // ? 뷰-프로젝션 적용
    Out.vPosition = mul(vPosition, mul(g_ViewMatrix, g_ProjMatrix));
    Out.vProjPos = Out.vPosition;
    Out.vWorldPos = vPosition;
    Out.vNormal = vNormal;
    Out.vTexUV = In.vTexUV;
    Out.vTangent = normalize(mul((float3x3) g_WorldMatrix, In.vTangent));
    Out.vBinormal = normalize(cross(Out.vNormal, Out.vTangent));

    return Out;
}


struct PS_IN
{
    float4 vPosition : SV_POSITION;
    float3 vNormal : NORMAL;
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

    // ? 노멀맵 처리
    float3 vNormalMap = g_NormalTexture.Sample(LinearSampler, In.vTexUV).xyz * 2.f - 1.f;
    float3x3 TBN = float3x3(
        normalize(In.vTangent),
        normalize(In.vBinormal),
        normalize(In.vNormal)
    );
    float3 vWorldNormal = normalize(mul(vNormalMap, TBN));

    // ? 색상/노멀/깊이 출력
    Out.vDiffuse = g_DiffuseTexture.Sample(LinearSampler, In.vTexUV);
    Out.vNormal = float4(vWorldNormal * 0.5f + 0.5f, 1.f);
    float depth = saturate(In.vProjPos.z / In.vProjPos.w);
    Out.vDepth = float4(depth, depth, depth, 1.f);

    if (Out.vDiffuse.a <= 0.05f)
        discard;

    return Out;
}


technique11 DefaultTechnique
{
    pass Default
    {
        SetRasterizerState(RS_Default);
        SetBlendState(BS_Default, float4(0.f, 0.f, 0.f, 1.f), 0xffffffff);
        SetDepthStencilState(DSS_Default, 0);

        VertexShader = compile vs_5_0 VS_MAIN();
        PixelShader = compile ps_5_0 PS_MAIN();
    }
}
