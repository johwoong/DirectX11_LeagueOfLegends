#include "Client_Shader_Defines.hlsl"

matrix g_WorldMatrix, g_ViewMatrix, g_ProjMatrix;
Texture2D g_DiffuseTexture;


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

PS_OUT PS_MAIN(VS_OUT In)
{
    PS_OUT Out;

    float4 tex = g_DiffuseTexture.Sample(LinearSampler, In.vTexUV);

    // ? discard 없이 알파만 사용
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
    pass P0
    {
        SetRasterizerState(RS_AttackRange);
        SetBlendState(BS_AttackRange, float4(0, 0, 0, 0), 0xffffffff);
        SetDepthStencilState(DSS_AttackRange, 0);

        VertexShader = compile vs_5_0 VS_MAIN();
        PixelShader = compile ps_5_0 PS_MAIN();
    }
}



    
