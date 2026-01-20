// IBL Vertex Shader
// Image Based Lighting vertex shader

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
};

cbuffer PerObject : register(b0) {
    float4x4 modelMatrix;
    float4x4 normalMatrix;
};

cbuffer PerFrame : register(b1) {
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
    
    // Calculate view direction
    output.viewDir = normalize(cameraPos - output.worldPos);
    
    // Pass through texture coordinates
    output.texCoord = input.texCoord;
    
    return output;
}
