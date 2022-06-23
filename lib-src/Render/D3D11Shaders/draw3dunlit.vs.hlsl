
uniform float4x4 worldViewProjMatrix : register(c0);

struct v2p
{
	float4 pos : SV_Position;
	float2 tex : TEXCOORD0;
	float4 col : COLOR0;
};

v2p main(float3 pos : POSITION0, float2 tex : TEXCOORD0, float4 col : COLOR0)
{
	v2p ret;
	ret.pos = mul(worldViewProjMatrix, float4(pos, 1));
	ret.tex = tex;
	ret.col = col;
	return ret;
}
