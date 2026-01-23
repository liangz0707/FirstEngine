// PBR Vertex Shader
// Physically Based Rendering vertex shader

struct VertexInput {
    float3 position : POSITION;
    float3 normal : NORMAL;
    float2 texCoord : TEXCOORD0;
    float4 tangent : TANGENT;
};

struct VertexOutput {
    float4 position : SV_POSITION;
    float3 worldPos : POSITION0;
    float3 normal : NORMAL0;
    float2 texCoord : TEXCOORD0;
    float3 tangent : TANGENT0;
    float3 bitangent : BINORMAL0;
    float3 viewDir : TEXCOORD1;
};

// Uniform Buffers - Set 0, Binding 0 and 1
[[vk::binding(0, 0)]] cbuffer PerObject {
    float4x4 modelMatrix;
    float4x4 normalMatrix;
};

[[vk::binding(1, 0)]] cbuffer PerFrame {
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
    float4x4 viewProjectionMatrix;
    float3 cameraPos;
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
    
    // Transform tangent to world space
    output.tangent = normalize(mul(input.tangent.xyz, (float3x3)normalMatrix));
    
    // Calculate bitangent
    output.bitangent = cross(output.normal, output.tangent) * input.tangent.w;
    
    // Calculate view direction
    output.viewDir = normalize(cameraPos - output.worldPos);
    
    // Pass through texture coordinates
    output.texCoord = input.texCoord;
    
    return output;
}
