// SunLight Fragment Shader
// Directional light with shadows and atmospheric scattering

struct FragmentInput {
    float4 position : SV_POSITION;
    float3 worldPos : POSITION0;
    float3 normal : NORMAL0;
    float2 texCoord : TEXCOORD0;
    float3 viewDir : TEXCOORD1;
    float3 lightDir : TEXCOORD2;
};

struct FragmentOutput {
    float4 color : SV_Target0;
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
};

[[vk::binding(1, 0)]] cbuffer SunLightParams {
    float3 sunDirection;
    float3 sunColor;
    float sunIntensity;
    float4x4 lightSpaceMatrix;
    float shadowBias;
    int useShadows;
};

// Separate Images - Set 0, Binding 2, 3, 4, 5
[[vk::binding(2, 0)]] Texture2D albedoMap;
[[vk::binding(3, 0)]] Texture2D normalMap;
[[vk::binding(4, 0)]] Texture2D metallicRoughnessMap;
[[vk::binding(5, 0)]] Texture2D shadowMap;

// Separate Samplers - Set 0, Binding 6, 7
[[vk::binding(6, 0)]] SamplerState textureSampler;
[[vk::binding(7, 0)]] SamplerComparisonState shadowSampler;

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

float CalculateShadow(float4 fragPosLightSpace, float3 normal, float3 lightDir) {
    if (useShadows == 0) return 1.0;
    
    // Perspective divide
    float3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords.xy = projCoords.xy * 0.5 + 0.5;
    projCoords.y = 1.0 - projCoords.y; // Flip Y for Vulkan
    
    // Check if fragment is outside shadow map
    if (projCoords.x < 0.0 || projCoords.x > 1.0 || projCoords.y < 0.0 || projCoords.y > 1.0) {
        return 1.0;
    }
    
    // Get closest depth value from light's perspective
    float closestDepth = shadowMap.SampleLevel(textureSampler, projCoords.xy, 0).r;
    
    // Get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;
    
    // Check if current fragment is in shadow
    float bias = shadowBias * tan(acos(max(dot(normal, lightDir), 0.0)));
    bias = clamp(bias, 0.0, 0.01);
    
    float shadow = currentDepth - bias > closestDepth ? 0.0 : 1.0;
    
    // PCF (Percentage Closer Filtering)
    float shadowSum = 0.0;
    float texelSize = 1.0 / 2048.0; // Shadow map resolution
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float pcfDepth = shadowMap.SampleLevel(textureSampler, projCoords.xy + float2(x, y) * texelSize, 0).r;
            shadowSum += currentDepth - bias > pcfDepth ? 0.0 : 1.0;
        }
    }
    shadow = shadowSum / 9.0;
    
    return shadow;
}

FragmentOutput main(FragmentInput input) {
    FragmentOutput output;
    
    // Sample textures
    float4 albedo = useAlbedoMap ? albedoMap.Sample(textureSampler, input.texCoord) : float4(1, 1, 1, 1);
    float4 metallicRoughness = useMetallicRoughnessMap ? metallicRoughnessMap.Sample(textureSampler, input.texCoord) : float4(metallic, roughness, 0, 1);
    
    // Calculate normal from normal map
    float3 N = normalize(input.normal);
    if (useNormalMap) {
        float3 normalSample = normalMap.Sample(textureSampler, input.texCoord).rgb * 2.0 - 1.0;
        // Simplified TBN
        N = normalize(N + normalSample * 0.1);
    }
    
    float3 V = normalize(input.viewDir);
    float3 L = normalize(input.lightDir);
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
    
    // Calculate shadow
    float4 fragPosLightSpace = mul(float4(input.worldPos, 1.0), lightSpaceMatrix);
    float shadow = CalculateShadow(fragPosLightSpace, N, L);
    
    // Add to outgoing radiance Lo
    float NdotL = max(dot(N, L), 0.0);
    float3 Lo = (kD * albedoColor / 3.14159265 + specular) * sunColor * NdotL * sunIntensity * shadow;
    
    // Add ambient
    float3 ambient = float3(0.03, 0.03, 0.03) * albedoColor * ao;
    float3 color = ambient + Lo;
    
    output.color = float4(color, albedo.a * baseColor.a);
    return output;
}
