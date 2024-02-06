
#include "RHI.h"

#include "../Model/Native.h" // TODO?


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


static int g_initialGraphicsAdapterLocked = -1;
static int g_initialGraphicsAdapterIndex = -1;
static std::string g_initialGraphicsAdapterName;

void GraphicsAdapters::GetInitial(int& index, StringView& name)
{
	index = g_initialGraphicsAdapterIndex;
	name = g_initialGraphicsAdapterName;
}

bool GraphicsAdapters::IsInitialLocked()
{
	return g_initialGraphicsAdapterLocked >= 0;
}

int GraphicsAdapters::GetLockedInitialAdapter()
{
	return g_initialGraphicsAdapterLocked;
}

bool GraphicsAdapters::SetInitialByName(StringView name)
{
	if (g_initialGraphicsAdapterLocked >= 0)
		return false;
	g_initialGraphicsAdapterIndex = -1;
	g_initialGraphicsAdapterName = ui::to_string(name);
	return true;
}

bool GraphicsAdapters::SetInitialByIndex(int index)
{
	if (g_initialGraphicsAdapterLocked >= 0)
		return false;
	g_initialGraphicsAdapterIndex = index;
	g_initialGraphicsAdapterName = {};
	return true;
}

void GraphicsAdapters_Lock(int which)
{
	assert(g_initialGraphicsAdapterLocked < 0);
	assert(which >= 0);
	g_initialGraphicsAdapterLocked = which;
}

void GraphicsAdapters_Unlock()
{
	assert(g_initialGraphicsAdapterLocked >= 0);
	g_initialGraphicsAdapterLocked = -1;
}


} // rhi
} // ui
