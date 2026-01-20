// Light Vertex Shader
// For light volume rendering (point lights, spot lights)

struct VertexInput {
    float3 position : POSITION;
};

struct VertexOutput {
    float4 position : SV_POSITION;
    float3 worldPos : POSITION0;
    float4 lightPos : TEXCOORD0;
};

cbuffer PerObject : register(b0) {
    float4x4 modelMatrix;
};

cbuffer PerFrame : register(b1) {
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
    float4x4 viewProjectionMatrix;
};

VertexOutput main(VertexInput input) {
    VertexOutput output;
    
    // Transform position to world space
    float4 worldPos = mul(float4(input.position, 1.0), modelMatrix);
    output.worldPos = worldPos.xyz;
    
    // Transform position to clip space
    output.position = mul(worldPos, viewProjectionMatrix);
    output.lightPos = output.position;
    
    return output;
}
