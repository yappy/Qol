#include "Shader.hlsli"

Texture2D gTexture : register(t0);
SamplerState gSample : register(s0);

float4 main( VS_OUTPUT input ) : SV_TARGET
{
	//return float4(1,0,0,1);
	float4 pixel = gTexture.Sample(gSample, input.Tex);
	pixel.rgb = pixel.rgb * (1.0f - input.FontColor.a) +
		input.FontColor.rgb * input.FontColor.a;
	pixel.a = pixel.a * input.Alpha;
	return pixel;
}
