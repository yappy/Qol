struct VS_INPUT {
	float3 Pos : POSITION;
	float2 Tex: TEXCOORD0;
};
struct VS_OUTPUT {
	float4 Pos : SV_POSITION;
	float2 Tex: TEXCOORD0;
};

VS_OUTPUT main( VS_INPUT input )
{
	VS_OUTPUT output = (VS_OUTPUT)0;
	output.Pos = float4(input.Pos, 1.0f);
	output.Tex = input.Tex;
	return output;
}
