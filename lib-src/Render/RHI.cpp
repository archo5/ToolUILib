
#include "RHI.h"


namespace ui {
namespace gfx {


Stats g_stats;

Stats Stats::operator - (const Stats& o) const
{
	Stats r;
	r.num_SetTexture = num_SetTexture - o.num_SetTexture;
	r.num_DrawTriangles = num_DrawTriangles - o.num_DrawTriangles;
	r.num_DrawIndexedTriangles = num_DrawIndexedTriangles - o.num_DrawIndexedTriangles;
	return r;
}

Stats Stats::Get()
{
	return g_stats;
}


size_t GetVertexSize(unsigned vertexFormat)
{
	size_t size = sizeof(float) * 3;
	if (vertexFormat & VF_Normal)
		size += sizeof(float) * 3;
	if (vertexFormat & VF_Texcoord)
		size += sizeof(float) * 2;
	if (vertexFormat & VF_Color)
		size += 4;
	return size;
}


} // gfx
} // ui
