// Tonemapping Fragment Shader
// Tone mapping and gamma correction

struct FragmentInput {
    float4 position : SV_POSITION;
    float2 texCoord : TEXCOORD0;
};

struct FragmentOutput {
    float4 color : SV_Target0;
};

Texture2D hdrTexture : register(t0);
SamplerState textureSampler : register(s0);

cbuffer TonemappingParams : register(b0) {
    float exposure;
    float gamma;
    int tonemapMode; // 0: Reinhard, 1: ACES, 2: Uncharted2, 3: None
};

// Reinhard tone mapping
float3 reinhard(float3 color) {
    return color / (color + 1.0);
}

// ACES Filmic tone mapping
float3 acesFilmic(float3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

// Uncharted 2 tone mapping
float3 uncharted2Tonemap(float3 x) {
    float A = 0.15;
    float B = 0.50;
    float C = 0.10;
    float D = 0.20;
    float E = 0.02;
    float F = 0.30;
    
    return ((x * (A * x + C * B) + D * E) / (x * (A * x + B) + D * F)) - E / F;
}

float3 uncharted2(float3 color) {
    float W = 11.2;
    float3 curr = uncharted2Tonemap(exposure * color);
    float3 whiteScale = 1.0 / uncharted2Tonemap(W);
    return curr * whiteScale;
}

FragmentOutput main(FragmentInput input) {
    FragmentOutput output;
    
    // Sample HDR texture
    float3 hdrColor = hdrTexture.Sample(textureSampler, input.texCoord).rgb;
    
    // Apply exposure
    float3 color = hdrColor * exposure;
    
    // Apply tone mapping
    if (tonemapMode == 0) {
        color = reinhard(color);
    } else if (tonemapMode == 1) {
        color = acesFilmic(color);
    } else if (tonemapMode == 2) {
        color = uncharted2(hdrColor);
    }
    // tonemapMode == 3: No tone mapping
    
    // Gamma correction
    color = pow(color, float3(1.0 / gamma, 1.0 / gamma, 1.0 / gamma));
    
    output.color = float4(color, 1.0);
    return output;
}
