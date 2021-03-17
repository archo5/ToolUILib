
#include "RHI.h"

namespace ui {
namespace rhi {


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


} // rhi
} // ui
