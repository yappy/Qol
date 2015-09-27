struct VS_OUTPUT {
	float4 Pos : SV_POSITION;
	float2 Tex : TEXCOORD0;
};

Texture2D gTexture : register(t0);
SamplerState gSample : register(s0);

float4 main( VS_OUTPUT input ) : SV_TARGET
{
	return gTexture.Sample(gSample, input.Tex);
}
