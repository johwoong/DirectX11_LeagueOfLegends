// ======================= States (¸Ç À§) =======================

SamplerState LinearSampler
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = WRAP;
    AddressV = WRAP;
};

SamplerState PointSampler
{
    Filter = MIN_MAG_MIP_POINT;
    AddressU = WRAP;
    AddressV = WRAP;
};

RasterizerState RS_Default
{
    FillMode = SOLID;
    CullMode = BACK;
    FrontCounterClockwise = FALSE;
};

RasterizerState RS_Default_NoCull
{
    FillMode = SOLID;
    CullMode = NONE;
    FrontCounterClockwise = FALSE;
};

RasterizerState RS_Wireframe
{
    FillMode = WIREFRAME;
    CullMode = BACK;
    FrontCounterClockwise = FALSE;
};

RasterizerState RS_SkyBox
{
    FillMode = SOLID;
    CullMode = FRONT;
    FrontCounterClockwise = FALSE;
};

BlendState BS_Default
{
    BlendEnable[0] = FALSE;
};

BlendState BS_AlphaBlending
{
    BlendEnable[0] = TRUE;

    SrcBlend = SRC_ALPHA;
    DestBlend = INV_SRC_ALPHA;
    BlendOp = ADD;

    SrcBlendAlpha = ONE;
    DestBlendAlpha = ZERO;
    BlendOpAlpha = ADD;
    RenderTargetWriteMask[0] = 0x0F;
};

BlendState BS_AddBlending
{
    BlendEnable[0] = TRUE;

    SrcBlend = ONE;
    DestBlend = ONE;
    BlendOp = ADD;

    SrcBlendAlpha = ONE;
    DestBlendAlpha = ONE;
    BlendOpAlpha = ADD;
    RenderTargetWriteMask[0] = 0x0F;
};

DepthStencilState DSS_Default
{
    DepthEnable = TRUE;
    DepthWriteMask = ALL;
    DepthFunc = LESS_EQUAL;
};

DepthStencilState DSS_NoWrite
{
    DepthEnable = TRUE;
    DepthWriteMask = ZERO;
    DepthFunc = LESS_EQUAL;
};

DepthStencilState DSS_Disabled
{
    DepthEnable = FALSE;
    DepthWriteMask = ALL;
};

DepthStencilState DSS_Priority
{
    DepthEnable = FALSE;
    DepthWriteMask = ZERO;
};