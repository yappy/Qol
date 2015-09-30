#include "Shader.hlsli"

Texture2D gTexture : register(t0);
SamplerState gSample : register(s0);

float4 main( VS_OUTPUT input ) : SV_TARGET
{
	//return float4(1, 0, 0, 1);	// R
	return gTexture.Sample(gSample, input.Tex);
}
