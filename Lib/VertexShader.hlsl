cbuffer cbNeverChanges : register( b0 ) {
	uint dummy;
};

cbuffer cbChanges : register( b1 ) {
	float4x4	Scale;
	float4x4	Mirror;
	float4x4	Translate;
	/*
	float4x4	Centering;
	float4x4	Rotate;
	float4x4	Restore;
	*/
	float4x4	Projection;
	float2		uvOffset;
	float2		uvSize;
	/*
	float		alpha;
	*/
};


struct VS_INPUT {
	float3 Pos : POSITION;
	float2 Tex : TEXCOORD0;
};
struct VS_OUTPUT {
	float4 Pos : SV_POSITION;
	float2 Tex : TEXCOORD0;
};


VS_OUTPUT main( VS_INPUT input )
{
	VS_OUTPUT output = (VS_OUTPUT)0;

	///////////////////////////////////////
	// (x, y, z, w)
	///////////////////////////////////////

	// (in.x, in.y, in.z, 1.0f)
	output.Pos = float4(input.Pos, 1.0f);

	// (0,0)->(1,1) => (0,0)->(w,h)
	output.Pos = mul(output.Pos, Scale);

	// LR reflect if needed
	output.Pos = mul(output.Pos, Mirror);

	// (0,0) => (x,y)
	output.Pos = mul(output.Pos, Translate);

	// TODO: rotate

	matrix test = {
		2.0f / 1024 , 0.0f, 0.0f, 0.0f,
		0.0f, -2.0f / 768 , 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		-1.0f, 1.0f, 0.0f, 1.0f };
	// (0,0)->(winw,winh) => (-1,-1)->(1,1)
	output.Pos = mul(output.Pos, /*test*/Projection);

	///////////////////////////////////////
	// (u, v)
	///////////////////////////////////////

	// (0.0f, 1.0f) => (offset, offset+size)
	output.Tex = uvOffset + uvSize * input.Tex;

	return output;
}
