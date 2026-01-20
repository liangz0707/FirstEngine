// Tonemapping Vertex Shader
// Full-screen quad for tone mapping

struct VertexInput {
    float2 position : POSITION;
    float2 texCoord : TEXCOORD0;
};

struct VertexOutput {
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

VertexOutput main(VertexInput input) {
    VertexOutput output;
    
    // Full-screen quad: position is already in clip space
    output.position = float4(input.position, 0.0, 1.0);
    output.texCoord = input.texCoord;
    
    return output;
}
