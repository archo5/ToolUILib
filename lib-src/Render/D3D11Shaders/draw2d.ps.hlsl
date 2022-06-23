
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
	return curTex.Sample(curSmp, input.tex) * input.col;
}
