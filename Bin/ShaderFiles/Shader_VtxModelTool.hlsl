
#include "Client_Shader_Defines.hlsl"

matrix g_WorldMatrix, g_ViewMatrix, g_ProjMatrix;

Texture2D g_DiffuseTexture;
Texture2D g_NormalTexture;
Texture2D g_SpecularTexture;

Texture2D g_MaskTexture;
Texture2D g_NoiseTexture;
Texture2D g_DissolveTexture;

float4 g_vColor = float4(1, 1, 0, 1);
float g_fAlpha = 1.0f;
float g_fTimer = 0.0f;

/* Feature toggles */
bool g_bDiffuse;

bool g_bMask;
float g_fMaskSpeed;
float g_fMaskDirectionX;
float g_fMaskDirectionY;

bool g_bNoise;
float g_fNoiseSpeed;
float g_fNoisePower;
float g_fNoiseDirectionX;
float g_fNoiseDirectionY;

bool g_bDissolve;

float3 g_vGlowColor;
float g_fGlowPower;

// ======================= IO Structs =======================
struct VS_IN
{
    float3 vPosition : POSITION;
    float3 vNormal : NORMAL;
    float2 vTexUV : TEXCOORD0;
    float3 vTangent : TANGENT;
};

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float3 vNormal : NORMAL;
    float2 vTexUV : TEXCOORD0;
    float4 vProjPos : TEXCOORD1;
    float3 vTangent : TANGENT;
    float3 vBinormal : BINORMAL;
};

// ======================= VS/PS =======================
VS_OUT VS_MAIN(VS_IN In)
{
    VS_OUT Out = (VS_OUT) 0;

    matrix matWV = mul(g_WorldMatrix, g_ViewMatrix);
    matrix matWVP = mul(matWV, g_ProjMatrix);

    float3 nWS = normalize(mul(float4(In.vNormal, 0.f), g_WorldMatrix).xyz);

    Out.vPosition = mul(float4(In.vPosition, 1.f), matWVP);
    Out.vNormal = nWS;
    Out.vTexUV = In.vTexUV;
    Out.vProjPos = Out.vPosition;

    float3 tWS = normalize(mul(float4(In.vTangent, 0.f), g_WorldMatrix).xyz);
    float3 bWS = normalize(cross(nWS, tWS));

    Out.vTangent = tWS;
    Out.vBinormal = bWS;
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
};

struct PS_OUT
{
    float4 vDiffuse : SV_TARGET0;
    float4 vNormal : SV_TARGET1;
    float4 vDepth : SV_TARGET2;
    float4 vLightDepth : SV_TARGET4; // 자리 채움
};

PS_OUT PS_MAIN(PS_IN In)
{
    PS_OUT Out = (PS_OUT) 0;

    float3 nTS = g_NormalTexture.Sample(LinearSampler, In.vTexUV).xyz * 2.f - 1.f;
    float3x3 TBN = float3x3(normalize(In.vTangent), normalize(In.vBinormal), normalize(In.vNormal));
    float3 nWS = mul(nTS, TBN);

    Out.vDiffuse = g_DiffuseTexture.Sample(LinearSampler, In.vTexUV);
    Out.vNormal = float4(nWS * 0.5f + 0.5f, 0.f);
    Out.vDepth = float4(In.vProjPos.z / In.vProjPos.w, In.vProjPos.w / 1000.f, 0.f, 0.f);
    Out.vDiffuse.a *= g_fAlpha;

    if (Out.vDiffuse.a <= 0.0f)
        discard;
    return Out;
}

PS_OUT PS_PICKED(PS_IN In)
{
    PS_OUT Out = PS_MAIN(In);
    Out.vDiffuse.rgb += 0.2f;
    return Out;
}

PS_OUT PS_SYMBOL(PS_IN In)
{
    PS_OUT Out = (PS_OUT) 0;
    Out.vNormal = float4(In.vNormal * 0.5f + 0.5f, 0.f);
    Out.vDiffuse = g_vColor;
    Out.vDepth = float4(In.vProjPos.z / In.vProjPos.w, In.vProjPos.w / 1000.f, 0.f, 0.f);
    return Out;
}

// ======================= Technique =======================
technique11 DefaultTechnique
{
    pass Default // 0
    {
        SetRasterizerState(RS_Default_NoCull);
        SetBlendState(BS_Default, float4(0, 0, 0, 1), 0xffffffff);
        SetDepthStencilState(DSS_Default, 0);

        VertexShader = compile vs_5_0 VS_MAIN();
        GeometryShader = NULL;
        PixelShader = compile ps_5_0 PS_MAIN();
    }

    pass Wire // 1
    {
        SetRasterizerState(RS_Wireframe);
        SetBlendState(BS_Default, float4(0, 0, 0, 1), 0xffffffff);
        SetDepthStencilState(DSS_Default, 0);

        VertexShader = compile vs_5_0 VS_MAIN();
        GeometryShader = NULL;
        PixelShader = compile ps_5_0 PS_MAIN();
    }

    pass Picked // 2
    {
        SetRasterizerState(RS_Default_NoCull);
        SetBlendState(BS_Default, float4(0, 0, 0, 1), 0xffffffff);
        SetDepthStencilState(DSS_Default, 0);

        VertexShader = compile vs_5_0 VS_MAIN();
        GeometryShader = NULL;
        PixelShader = compile ps_5_0 PS_PICKED();
    }

    pass PickingSymbol // 3
    {
        SetRasterizerState(RS_Default);
        SetBlendState(BS_Default, float4(0, 0, 0, 1), 0xffffffff);
        SetDepthStencilState(DSS_Default, 0);

        VertexShader = compile vs_5_0 VS_MAIN();
        GeometryShader = NULL;
        PixelShader = compile ps_5_0 PS_SYMBOL();
    }
}