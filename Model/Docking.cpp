
#include "Docking.h"

namespace ui {

void DockingArea::RegisterDockable(Dockable* d)
{
}

void DockingArea::UnregisterDockable(Dockable* d)
{
}

void Dockable::OnEnterTree()
{
	_area = FindParentOfType<DockingArea>();
	_area->RegisterDockable(this);
}

void Dockable::OnExitTree()
{
	_area->UnregisterDockable(this);
}

} // ui
