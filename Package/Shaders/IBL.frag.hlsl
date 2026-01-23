// IBL Fragment Shader
// Image Based Lighting using environment maps

struct FragmentInput {
    float4 position : SV_POSITION;
    float3 worldPos : POSITION0;
    float3 normal : NORMAL0;
    float2 texCoord : TEXCOORD0;
    float3 viewDir : TEXCOORD1;
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

[[vk::binding(1, 0)]] cbuffer IBLParams {
    float iblIntensity;
    float prefilterLOD;
};

// Separate Images - Set 0, Binding 2, 3, 4, 5, 6, 7
[[vk::binding(2, 0)]] Texture2D albedoMap;
[[vk::binding(3, 0)]] Texture2D normalMap;
[[vk::binding(4, 0)]] Texture2D metallicRoughnessMap;
[[vk::binding(5, 0)]] TextureCube irradianceMap;
[[vk::binding(6, 0)]] TextureCube prefilterMap;
[[vk::binding(7, 0)]] Texture2D brdfLUT;

// Separate Samplers - Set 0, Binding 8, 9
[[vk::binding(8, 0)]] SamplerState textureSampler;
[[vk::binding(9, 0)]] SamplerState cubeSampler;

// PBR Functions
float3 fresnelSchlickRoughness(float cosTheta, float3 F0, float roughness) {
    return F0 + (max(float3(1.0 - roughness, 1.0 - roughness, 1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
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
        // Simplified TBN - would need tangent/bitangent in real implementation
        N = normalize(N + normalSample * 0.1);
    }
    
    float3 V = normalize(input.viewDir);
    float3 R = reflect(-V, N);
    
    // Material properties
    float3 albedoColor = albedo.rgb * baseColor.rgb;
    float metallicValue = metallicRoughness.r;
    float roughnessValue = metallicRoughness.g;
    
    // Calculate F0 for Fresnel
    float3 F0 = lerp(float3(0.04, 0.04, 0.04), albedoColor, metallicValue);
    
    // Sample IBL
    float3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughnessValue);
    float3 kS = F;
    float3 kD = 1.0 - kS;
    kD *= 1.0 - metallicValue;
    
    // Diffuse IBL (irradiance)
    float3 irradiance = irradianceMap.Sample(cubeSampler, N).rgb;
    float3 diffuse = irradiance * albedoColor;
    
    // Specular IBL (prefiltered environment + BRDF)
    float3 prefilteredColor = prefilterMap.SampleLevel(cubeSampler, R, roughnessValue * prefilterLOD).rgb;
    float2 envBRDF = brdfLUT.Sample(textureSampler, float2(max(dot(N, V), 0.0), roughnessValue)).rg;
    float3 specular = prefilteredColor * (F * envBRDF.x + envBRDF.y);
    
    // Combine
    float3 ambient = (kD * diffuse + specular) * ao * iblIntensity;
    
    output.color = float4(ambient, albedo.a * baseColor.a);
    return output;
}
