
// 0..size -> -1..1
uniform float2 invHalfSize : register(c0);

struct v2p
{
	float4 pos : SV_Position;
	float2 tex : TEXCOORD0;
	float4 col : COLOR0;
};

v2p main(float2 pos : POSITION0, float2 tex : TEXCOORD0, float4 col : COLOR0)
{
	v2p ret;
	ret.pos = float4(pos * invHalfSize + float2(-1, 1), 0.5f, 1);
	ret.tex = tex;
	ret.col = col;
	return ret;
}
