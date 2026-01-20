// PostProcess Fragment Shader
// Base post-processing shader

struct FragmentInput {
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

struct FragmentOutput {
    float4 color : SV_Target0;
};

Texture2D inputTexture : register(t0);
SamplerState textureSampler : register(s0);

cbuffer PostProcessParams : register(b0) {
    float2 screenSize;
    float time;
    float intensity;
};

FragmentOutput main(FragmentInput input) {
    FragmentOutput output;
    
    // Sample input texture
    float4 color = inputTexture.Sample(textureSampler, input.texCoord);
    
    // Basic post-processing (can be extended)
    output.color = color;
    
    return output;
}
