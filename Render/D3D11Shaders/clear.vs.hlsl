
static const float4 verts[] =
{
	float4(-1, -1, 0.5f, 1),
	float4(3, -1, 0.5f, 1),
	float4(-1, 3, 0.5f, 1),
};

float4 main(uint id : SV_VertexID) : SV_Position
{
	return verts[id];
}
