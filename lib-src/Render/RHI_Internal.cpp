
#pragma once
#include "RHI.h"

#include "../Core/Array.h"
#include "../Core/Memory.h"


namespace ui {
namespace rhi {


static Array<IRHIListener*> g_listeners;


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
	g_listeners.Append(L);
}

void DetachListener(IRHIListener* L)
{
	for (size_t i = 0; i < g_listeners.size(); i++)
	{
		if (g_listeners[i] == L)
		{
			g_listeners.RemoveAt(i);
			OnListenerRemove(L);
			break;
		}
	}
}


} // rhi
} // ui
