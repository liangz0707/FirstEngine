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
    float4 color : SV_Target0;
};

Texture2D albedoMap : register(t0);
Texture2D normalMap : register(t1);
Texture2D metallicRoughnessMap : register(t2);
Texture2D aoMap : register(t3);

SamplerState textureSampler : register(s0);

cbuffer MaterialParams : register(b0) {
    float4 baseColor;
    float metallic;
    float roughness;
    float ao;
    int useAlbedoMap;
    int useNormalMap;
    int useMetallicRoughnessMap;
    int useAoMap;
};

cbuffer LightingParams : register(b1) {
    float3 lightDirection;
    float3 lightColor;
    float3 ambientColor;
};

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
    
    float3 V = normalize(input.viewDir);
    float3 L = normalize(-lightDirection);
    float3 H = normalize(V + L);
    
    // Material properties
    float3 albedoColor = albedo.rgb * baseColor.rgb;
    float metallicValue = metallicRoughness.r;
    float roughnessValue = metallicRoughness.g;
    
    // Calculate F0 for Fresnel
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedoColor, metallicValue);
    
    // Cook-Torrance BRDF
    float NDF = DistributionGGX(N, H, roughnessValue);
    float G = GeometrySmith(N, V, L, roughnessValue);
    float3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
    
    float3 kS = F;
    float3 kD = (1.0 - kS) * (1.0 - metallicValue);
    
    float3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    float3 specular = numerator / denominator;
    
    // Add to outgoing radiance Lo
    float NdotL = max(dot(N, L), 0.0);
    float3 Lo = (kD * albedoColor / 3.14159265 + specular) * lightColor * NdotL;
    
    // Add ambient
    float3 ambient = ambientColor * albedoColor * aoValue;
    float3 color = ambient + Lo;
    
    output.color = float4(color, albedo.a * baseColor.a);
    return output;
}
