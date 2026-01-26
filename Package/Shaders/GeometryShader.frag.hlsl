// GeometryShader Fragment Shader
// Outputs G-Buffer for deferred rendering

struct FragmentInput {
    float4 position : SV_POSITION;
    float3 worldPos : POSITION0;
    float3 normal : NORMAL0;
    float2 texCoord : TEXCOORD0;
    float3 tangent : TANGENT0;
    float3 bitangent : BINORMAL0;
};

struct FragmentOutput {
    float4 albedo : SV_Target0;        // RGB: Albedo, A: Metallic
    float4 normal : SV_Target1;        // RGB: Normal, A: Roughness
    float4 material : SV_Target2;       // R: AO, G: Emission, B: unused, A: unused
};

// Uniform Buffer - Set 0, Binding 0
[[vk::binding(0, 0)]] cbuffer MaterialParams {
    float4 baseColor;
    float metallic;
    float roughness;
    float ao;
    float3 emission;
    int useAlbedoMap;
    int useNormalMap;
    int useMetallicRoughnessMap;
    int useAoMap;
    int useEmissionMap;
};

// Separate Images - Set 0, Binding 1, 2, 3, 4, 5
[[vk::binding(1, 0)]] Texture2D albedoMap;
[[vk::binding(2, 0)]] Texture2D normalMap;
[[vk::binding(3, 0)]] Texture2D metallicRoughnessMap;
[[vk::binding(4, 0)]] Texture2D aoMap;
[[vk::binding(5, 0)]] Texture2D emissionMap;

// Separate Sampler - Set 0, Binding 6
[[vk::binding(6, 0)]] SamplerState textureSampler;

FragmentOutput main(FragmentInput input) {
    FragmentOutput output;
    
    // Initialize all outputs to ensure they are written
    output.albedo = float4(0, 0, 0, 0);
    output.normal = float4(0, 0, 0, 0);
    output.material = float4(0, 0, 0, 0);
    
    // Sample textures
    float4 albedo = useAlbedoMap ? albedoMap.Sample(textureSampler, input.texCoord) : float4(1, 1, 1, 1);
    float4 metallicRoughness = useMetallicRoughnessMap ? metallicRoughnessMap.Sample(textureSampler, input.texCoord) : float4(metallic, roughness, 0, 1);
    float aoValue = useAoMap ? aoMap.Sample(textureSampler, input.texCoord).r : ao;
    float3 emissionValue = useEmissionMap ? emissionMap.Sample(textureSampler, input.texCoord).rgb : emission;
    
    // Calculate normal from normal map
    float3 N = normalize(input.normal);
    if (useNormalMap) {
        float3 normalSample = normalMap.Sample(textureSampler, input.texCoord).rgb * 2.0 - 1.0;
        float3x3 TBN = float3x3(normalize(input.tangent), normalize(input.bitangent), normalize(input.normal));
        N = normalize(mul(normalSample, TBN));
    }
    
    // Output G-Buffer - ensure all outputs are written
    // SV_Target0: Albedo (RGB) + Metallic (A)
    output.albedo = float4(albedo.rgb * baseColor.rgb, metallicRoughness.r);
    
    // SV_Target1: Normal (RGB) + Roughness (A)
    output.normal = float4(N * 0.5 + 0.5, metallicRoughness.g);
    
    // SV_Target2: AO (R) + Emission (G) + unused (BA)
    output.material = float4(aoValue, length(emissionValue), 0, 0);
    
    return output;
}
