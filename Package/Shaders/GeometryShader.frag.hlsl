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

Texture2D albedoMap : register(t0);
Texture2D normalMap : register(t1);
Texture2D metallicRoughnessMap : register(t2);
Texture2D aoMap : register(t3);
Texture2D emissionMap : register(t4);

SamplerState textureSampler : register(s0);

cbuffer MaterialParams : register(b0) {
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

FragmentOutput main(FragmentInput input) {
    FragmentOutput output;
    
    // Sample textures
    float4 albedo = useAlbedoMap ? albedoMap.Sample(textureSampler, input.texCoord) : float4(1, 1, 1, 1);
    float4 metallicRoughness = useMetallicRoughnessMap ? metallicRoughnessMap.Sample(textureSampler, input.texCoord) : float4(metallic, roughness, 0, 1);
    float aoValue = useAoMap ? aoMap.Sample(textureSampler, input.texCoord).r : ao;
    float3 emissionValue = useEmissionMap ? emissionMap.Sample(textureSampler, input.texCoord).rgb : emission;
    
    // Calculate normal from normal map
    float3 N = input.normal;
    if (useNormalMap) {
        float3 normalSample = normalMap.Sample(textureSampler, input.texCoord).rgb * 2.0 - 1.0;
        float3x3 TBN = float3x3(normalize(input.tangent), normalize(input.bitangent), normalize(input.normal));
        N = normalize(mul(normalSample, TBN));
    }
    
    // Output G-Buffer
    output.albedo = float4(albedo.rgb * baseColor.rgb, metallicRoughness.r); // Metallic in alpha
    output.normal = float4(N * 0.5 + 0.5, metallicRoughness.g); // Normal in RGB, Roughness in alpha
    output.material = float4(aoValue, length(emissionValue), 0, 0);
    
    return output;
}
