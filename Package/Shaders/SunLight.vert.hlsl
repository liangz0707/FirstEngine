// SunLight Vertex Shader
// Directional light (sun) rendering

struct VertexInput {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD0;
};

struct VertexOutput {
    float4 position : SV_POSITION;
    float3 worldPos : POSITION0;
    float3 normal : NORMAL0;
    float2 texCoord : TEXCOORD0;
    float3 viewDir : TEXCOORD1;
    float3 lightDir : TEXCOORD2;
};

// Uniform Buffers - Set 1, Binding 0 and 1
// Note: PerObject and PerFrame are in Set 1 to avoid conflicts with fragment shader resources in Set 0
[[vk::binding(0, 1)]] cbuffer PerObject {
    float4x4 modelMatrix;
    float4x4 normalMatrix;
};

[[vk::binding(1, 1)]] cbuffer PerFrame {
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
    float4x4 viewProjectionMatrix;
    float3 cameraPos;
    float3 sunDirection;
    float3 sunColor;
    float sunIntensity;
};

VertexOutput main(VertexInput input) {
    VertexOutput output;
    
    // Transform position to world space
    float4 worldPos = mul(float4(input.position, 1.0), modelMatrix);
    output.worldPos = worldPos.xyz;
    
    // Transform position to clip space
    output.position = mul(worldPos, viewProjectionMatrix);
    
    // Transform normal to world space
    output.normal = normalize(mul(input.normal, (float3x3)normalMatrix));
    
    // Calculate view direction
    output.viewDir = normalize(cameraPos - output.worldPos);
    
    // Sun light direction
    output.lightDir = normalize(-sunDirection);
    
    // Pass through texture coordinates
    output.texCoord = input.texCoord;
    
    return output;
}
