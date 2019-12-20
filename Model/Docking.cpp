
#include "Docking.h"

namespace ui {

void DockingArea::RegisterDockable(Dockable* d)
{
}

void DockingArea::UnregisterDockable(Dockable* d)
{
}

void Dockable::OnInit()
{
	_area = FindParentOfType<DockingArea>();
	_area->RegisterDockable(this);
}

void Dockable::OnDestroy()
{
	_area->UnregisterDockable(this);
}

} // ui
