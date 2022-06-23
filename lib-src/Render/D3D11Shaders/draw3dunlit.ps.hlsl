
uniform float alphaTestShift : register(c4);

struct v2p
{
	float4 pos : SV_Position;
	float2 tex : TEXCOORD0;
	float4 col : COLOR0;
};

Texture2D curTex : register(t0);
SamplerState curSmp : register(s0);

float4 main(v2p input) : SV_Target0
{
	float4 ret = curTex.Sample(curSmp, input.tex) * input.col;
	if (ret.a - alphaTestShift < 0)
		discard;
	return ret;
}
