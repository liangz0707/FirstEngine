// PBR Fragment Shader
// Physically Based Rendering using Cook-Torrance BRDF

struct FragmentInput {
    float4 position : SV_POSITION;
    float3 worldPos : POSITION0;
    float3 normal : NORMAL0;
    float2 texCoord : TEXCOORD0;
    float3 tangent : TANGENT0;
    float3 bitangent : BINORMAL0;
    float3 viewDir : TEXCOORD1;
};

struct FragmentOutput {
    float4 albedo : SV_Target0;        // RGB: Albedo, A: Metallic
    float4 normal : SV_Target1;        // RGB: Normal, A: Roughness
    float4 material : SV_Target2;       // R: AO, G: Emission, B: unused, A: unused
};

// Uniform Buffers - Set 0, Binding 0 and 1
[[vk::binding(0, 0)]] cbuffer MaterialParams {
    float4 baseColor;
    float metallic;
    float roughness;
    float ao;
    int useAlbedoMap;
    int useNormalMap;
    int useMetallicRoughnessMap;
    int useAoMap;
};

[[vk::binding(1, 0)]] cbuffer LightingParams {
    float3 lightDirection;
    float3 lightColor;
    float3 ambientColor;
};

// Separate Images - Set 0, Binding 2, 3, 4, 5
[[vk::binding(2, 0)]] Texture2D albedoMap;
[[vk::binding(3, 0)]] Texture2D normalMap;
[[vk::binding(4, 0)]] Texture2D metallicRoughnessMap;
[[vk::binding(5, 0)]] Texture2D aoMap;

// Separate Sampler - Set 0, Binding 6
[[vk::binding(6, 0)]] SamplerState textureSampler;

// PBR Functions
float DistributionGGX(float3 N, float3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float num = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = 3.14159265 * denom * denom;
    
    return num / denom;
}

float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;
    
    float num = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return num / denom;
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

float3 fresnelSchlick(float cosTheta, float3 F0) {
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

FragmentOutput main(FragmentInput input) {
    FragmentOutput output;
    
    // Sample textures
    float4 albedo = useAlbedoMap ? albedoMap.Sample(textureSampler, input.texCoord) : float4(1, 1, 1, 1);
    float4 metallicRoughness = useMetallicRoughnessMap ? metallicRoughnessMap.Sample(textureSampler, input.texCoord) : float4(metallic, roughness, 0, 1);
    float aoValue = useAoMap ? aoMap.Sample(textureSampler, input.texCoord).r : ao;
    
    // Calculate normal from normal map
    float3 N = normalize(input.normal);
    if (useNormalMap) {
        float3 normalSample = normalMap.Sample(textureSampler, input.texCoord).rgb * 2.0 - 1.0;
        float3x3 TBN = float3x3(normalize(input.tangent), normalize(input.bitangent), normalize(input.normal));
        N = normalize(mul(normalSample, TBN));
    }

    // Output G-Buffer - ensure all outputs are written
    // SV_Target0: Albedo (RGB) + 1
    output.albedo = albedo;
    
    // SV_Target1: Normal (RGB) + 1
    output.normal = float4(N * 0.5 + 0.5, 1);
    
    // SV_Target2: Material (RGB)+ ao
    // Note: PBR shader doesn't have emission support yet, use 0 for now
    float3 emissionValue = float3(0, 0, 0);
    output.material = float4(metallicRoughness.xyz, aoValue);
    return output;
}
