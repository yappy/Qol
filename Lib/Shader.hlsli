/* VS in from app (vertex buffer) */
struct VS_INPUT {
	float3 Pos : POSITION;
	float2 Tex : TEXCOORD0;
};

/* VS out and PS in */
struct VS_OUTPUT {
	float4 Pos : SV_POSITION;
	float2 Tex : TEXCOORD0;
};
