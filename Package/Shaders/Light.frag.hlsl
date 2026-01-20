// Light Fragment Shader
// Deferred lighting pass for point/spot lights

struct FragmentInput {
    float4 position : SV_POSITION;
    float3 worldPos : POSITION0;
    float4 lightPos : TEXCOORD0;
};

struct FragmentOutput {
    float4 color : SV_Target0;
};

// G-Buffer inputs
Texture2D gAlbedo : register(t0);
Texture2D gNormal : register(t1);
Texture2D gMaterial : register(t2);
Texture2D gDepth : register(t3);

SamplerState gBufferSampler : register(s0);

cbuffer LightParams : register(b0) {
    float3 lightPosition;
    float3 lightColor;
    float lightRadius;
    float lightIntensity;
    int lightType; // 0: Point, 1: Spot, 2: Directional
    float3 lightDirection;
    float spotAngle;
    float spotFalloff;
};

cbuffer PerFrame : register(b1) {
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
    float4x4 invViewProjectionMatrix;
    float3 cameraPos;
};

// Reconstruct world position from depth
float3 ReconstructWorldPos(float2 screenUV, float depth) {
    float4 clipPos = float4(screenUV * 2.0 - 1.0, depth, 1.0);
    clipPos.y = -clipPos.y; // Flip Y for Vulkan
    float4 worldPos = mul(clipPos, invViewProjectionMatrix);
    return worldPos.xyz / worldPos.w;
}

FragmentOutput main(FragmentInput input) {
    FragmentOutput output;
    
    // Calculate screen space UV
    float2 screenUV = input.lightPos.xy / input.lightPos.w * 0.5 + 0.5;
    screenUV.y = 1.0 - screenUV.y; // Flip Y for Vulkan
    
    // Sample G-Buffer
    float4 albedoMetallic = gAlbedo.Sample(gBufferSampler, screenUV);
    float4 normalRoughness = gNormal.Sample(gBufferSampler, screenUV);
    float4 material = gMaterial.Sample(gBufferSampler, screenUV);
    float depth = gDepth.Sample(gBufferSampler, screenUV).r;
    
    // Reconstruct world position
    float3 worldPos = ReconstructWorldPos(screenUV, depth);
    
    // Extract material properties
    float3 albedo = albedoMetallic.rgb;
    float metallic = albedoMetallic.a;
    float3 normal = normalize(normalRoughness.rgb * 2.0 - 1.0);
    float roughness = normalRoughness.a;
    float ao = material.r;
    
    // Calculate light direction and distance
    float3 L = float3(0, 0, 0);
    float attenuation = 1.0;
    
    if (lightType == 0) { // Point light
        L = normalize(lightPosition - worldPos);
        float distance = length(lightPosition - worldPos);
        attenuation = 1.0 / (1.0 + 0.09 * distance + 0.032 * distance * distance);
        attenuation *= smoothstep(lightRadius, lightRadius * 0.5, distance);
    } else if (lightType == 1) { // Spot light
        L = normalize(lightPosition - worldPos);
        float distance = length(lightPosition - worldPos);
        float3 D = normalize(-lightDirection);
        float cosAngle = dot(L, D);
        float angle = acos(cosAngle);
        float spotFactor = smoothstep(spotAngle, spotAngle * spotFalloff, angle);
        attenuation = spotFactor / (1.0 + 0.09 * distance + 0.032 * distance * distance);
    } else { // Directional light
        L = normalize(-lightDirection);
        attenuation = 1.0;
    }
    
    // Simple lighting calculation
    float NdotL = max(dot(normal, L), 0.0);
    float3 diffuse = albedo * lightColor * NdotL * attenuation * lightIntensity;
    
    // Simple specular
    float3 V = normalize(cameraPos - worldPos);
    float3 H = normalize(V + L);
    float NdotH = max(dot(normal, H), 0.0);
    float specular = pow(NdotH, (1.0 - roughness) * 128.0);
    float3 spec = lightColor * specular * attenuation * lightIntensity;
    
    output.color = float4(diffuse + spec, 1.0);
    return output;
}
