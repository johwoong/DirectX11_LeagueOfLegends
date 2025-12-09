#include "Client_Shader_Defines.hlsl"

matrix g_WorldMatrix, g_ViewMatrix, g_ProjMatrix;
Texture2D g_DiffuseTexture;

float4 g_Color; // ★ 추가 : 공격 사거리 색상
float g_fAlpha; // HPBar에서 쓰던 이름 그대로 사용

struct VS_IN
{
    float3 vPosition : POSITION;
    float2 vTexUV : TEXCOORD0;
};

struct VS_OUT
{
    float4 vPosition : SV_POSITION;
    float2 vTexUV : TEXCOORD0;
};

VS_OUT VS_MAIN(VS_IN In)
{
    VS_OUT Out;
    matrix matWVP = mul(mul(g_WorldMatrix, g_ViewMatrix), g_ProjMatrix);

    Out.vPosition = mul(float4(In.vPosition, 1.f), matWVP);
    Out.vTexUV = In.vTexUV;
    return Out;
}

struct PS_OUT
{
    float4 vColor : SV_TARGET0;
};

// ===== AttackRange 전용 =====
PS_OUT PS_MAIN(VS_OUT In)
{
    PS_OUT Out;

    float4 tex = g_DiffuseTexture.Sample(LinearSampler, In.vTexUV);

    // 텍스처는 "알파 마스크"만 사용
    float mask = tex.a;

    float4 col = g_Color; // 색은 전부 g_Color 에서
    col.a *= mask * g_fAlpha; // 알파 = 텍스처 알파 * g_fAlpha

    Out.vColor = col;
    return Out;
}

// ===== HPBar 그대로 유지 =====
PS_OUT PS_HPBar(VS_OUT In)
{
    PS_OUT Out;

    float4 tex = g_DiffuseTexture.Sample(LinearSampler, In.vTexUV);
    tex.a *= g_fAlpha;

    Out.vColor = tex;
    return Out;
}

BlendState BS_AttackRange
{
    AlphaToCoverageEnable = FALSE;
    BlendEnable[0] = TRUE;

    SrcBlend = SRC_ALPHA;
    DestBlend = INV_SRC_ALPHA;
    BlendOp = ADD;

    SrcBlendAlpha = ONE;
    DestBlendAlpha = ZERO;
    BlendOpAlpha = ADD;

    RenderTargetWriteMask[0] = 0x0F;
};

DepthStencilState DSS_AttackRange
{
    DepthEnable = TRUE;
    DepthWriteMask = ZERO;
    DepthFunc = LESS_EQUAL;
};

RasterizerState RS_AttackRange
{
    CullMode = NONE;
    FillMode = SOLID;
};

technique11 AttackRangeTech
{
    // P0 : AttackRange
    pass P0
    {
        SetRasterizerState(RS_AttackRange);
        SetBlendState(BS_AttackRange, float4(0, 0, 0, 0), 0xffffffff);
        SetDepthStencilState(DSS_AttackRange, 0);

        VertexShader = compile vs_5_0 VS_MAIN();
        PixelShader = compile ps_5_0 PS_MAIN();
    }

    // P1 : HPBar 프레임/바 (그대로)
    pass P1
    {
        SetRasterizerState(RS_AttackRange);
        SetBlendState(BS_AttackRange, float4(0, 0, 0, 0), 0xffffffff);
        SetDepthStencilState(DSS_AttackRange, 0);

        VertexShader = compile vs_5_0 VS_MAIN();
        PixelShader = compile ps_5_0 PS_HPBar();
    }
}