
#pragma once
#include "RHI.h"

#include "../Core/Memory.h"
#include <vector>


namespace ui {
namespace rhi {


static std::vector<IRHIListener*> g_listeners;


void OnListenerAdd(IRHIListener*);
void OnListenerRemove(IRHIListener*);

ArrayView<IRHIListener*> GetListeners()
{
	return g_listeners;
}


void AttachListener(IRHIListener* L)
{
	for (auto* lnr : g_listeners)
		if (lnr == L)
			return;

	OnListenerAdd(L);
	g_listeners.push_back(L);
}

void DetachListener(IRHIListener* L)
{
	for (size_t i = 0; i < g_listeners.size(); i++)
	{
		if (g_listeners[i] == L)
		{
			g_listeners.erase(g_listeners.begin() + i);
			OnListenerRemove(L);
			break;
		}
	}
}


} // rhi
} // ui
