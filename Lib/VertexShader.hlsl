#include "Shader.hlsli"

cbuffer cbNeverChanges : register( b0 ) {
	float4x4	Projection;
};

cbuffer cbChanges : register( b1 ) {
	float4x4	lrInv;
	float4x4	udInv;
	float4x4	DestScale;
	float4x4	Centering;
	float4x4	ScaleX;
	float4x4	ScaleY;
	float4x4	Rotation;
	float4x4	Translate;
	/* float4x4	Projection; */
	float2		uvOffset;
	float2		uvSize;
	float		Alpha;
};


VS_OUTPUT main( VS_INPUT input )
{
	VS_OUTPUT output = (VS_OUTPUT)0;

	///////////////////////////////////////
	// (x, y, z, w)
	///////////////////////////////////////

	// (in.x, in.y, in.z, 1.0f)
	output.Pos = float4(input.Pos, 1.0f);

	// LR, UD invert if needed
	output.Pos = mul(output.Pos, lrInv);
	output.Pos = mul(output.Pos, udInv);

	// (0,0)->(1,1) => (0,0)->(dw,dh)
	output.Pos = mul(output.Pos, DestScale);

	// Move (cx,cy) in DestBox to (0,0)
	output.Pos = mul(output.Pos, Centering);

	// Scaling
	output.Pos = mul(output.Pos, ScaleX);
	output.Pos = mul(output.Pos, ScaleY);

	// Rotation
	output.Pos = mul(output.Pos, Rotation);

	// Move (cx,cy) to destination (dx,dy)
	output.Pos = mul(output.Pos, Translate);

	// Projection
	// (0,0)->(winw,winh) => (-1,-1)->(1,1)
	output.Pos = mul(output.Pos, Projection);

	///////////////////////////////////////
	// (u, v)
	///////////////////////////////////////

	// (0.0f, 1.0f) => (offset, offset+size)
	output.Tex = uvOffset + uvSize * input.Tex;

	///////////////////////////////////////
	// Alpha
	///////////////////////////////////////
	output.Alpha = Alpha;

	return output;
}
