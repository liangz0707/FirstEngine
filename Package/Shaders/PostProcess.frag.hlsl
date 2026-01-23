// PostProcess Fragment Shader
// Base post-processing shader

struct FragmentInput {
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

struct FragmentOutput {
    float4 color : SV_Target0;
};

// Uniform Buffer - Set 0, Binding 0
[[vk::binding(0, 0)]] cbuffer PostProcessParams {
    float2 screenSize;
    float time;
    float intensity;
};

// Separate Image - Set 0, Binding 1
[[vk::binding(1, 0)]] Texture2D inputTexture;

// Separate Sampler - Set 0, Binding 2
[[vk::binding(2, 0)]] SamplerState textureSampler;

FragmentOutput main(FragmentInput input) {
    FragmentOutput output;
    
    // Sample input texture
    float4 color = inputTexture.Sample(textureSampler, input.texCoord);
    
    // Basic post-processing (can be extended)
    output.color = color;
    
    return output;
}
