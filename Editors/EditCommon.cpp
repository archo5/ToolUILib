
#include "EditCommon.h"

#include <unordered_set>


namespace ui {

struct BasicSelectionImpl
{
	std::unordered_set<uintptr_t> sel;
};

BasicSelection::BasicSelection()
{
	_impl = new BasicSelectionImpl;
}

BasicSelection::~BasicSelection()
{
	delete _impl;
}

void BasicSelection::ClearSelection()
{
	_impl->sel.clear();
}

bool BasicSelection::GetSelectionState(uintptr_t item)
{
	return _impl->sel.count(item);
}

void BasicSelection::SetSelectionState(uintptr_t item, bool sel)
{
	if (sel)
		_impl->sel.insert(item);
	else
		_impl->sel.erase(item);
}


bool SelectionImplementation::OnEvent(Event& e, ISelectionStorage* sel, uintptr_t hoverItem, bool hovering, bool onclick)
{
	if (selectionMode == SelectionMode::None || !sel)
		return false;

	if (DragDrop::GetData())
	{
		isClicked = false;
		return false;
	}

	bool selChanged = false;
	if (onclick)
	{
		if (e.type == EventType::Click && e.GetButton() == MouseButton::Left && hoverItem < UINTPTR_MAX && hovering)
		{
			if ((selectionMode != SelectionMode::MultipleToggle) ^ e.IsCtrlPressed() ||
				selectionMode == SelectionMode::Single)
				sel->ClearSelection();
			sel->SetSelectionState(hoverItem, !sel->GetSelectionState(hoverItem));

			selChanged = true;
		}
		return selChanged;
	}

	if (e.type == EventType::ButtonDown && e.GetButton() == MouseButton::Left && hoverItem < UINTPTR_MAX && hovering)
	{
		if ((selectionMode != SelectionMode::MultipleToggle) ^ e.IsCtrlPressed())
			sel->ClearSelection();

		sel->SetSelectionState(hoverItem, !sel->GetSelectionState(hoverItem));
		_selStart = hoverItem;
		_selEnd = hoverItem;
		isClicked = true;
		selChanged = true;
		//e.StopPropagation();
	}
	if (e.type == EventType::ButtonUp && e.GetButton() == MouseButton::Left)
	{
		isClicked = false;
	}
	if (e.type == EventType::MouseMove)
	{
		if (isClicked && hoverItem < UINTPTR_MAX && hovering)
		{
			if (selectionMode == SelectionMode::Single)
			{
				sel->ClearSelection();
				sel->SetSelectionState(hoverItem, !sel->GetSelectionState(hoverItem));
			}
			else
			{
				while (_selEnd != hoverItem)
				{
					bool increasing = (_selStart < _selEnd
							? hoverItem < _selStart || hoverItem > _selEnd
							: hoverItem < _selEnd || hoverItem > _selStart);

					if (!increasing)
						sel->SetSelectionState(_selEnd, !sel->GetSelectionState(_selEnd));

					if (_selEnd < hoverItem)
						_selEnd++;
					else
						_selEnd--;

					if (increasing)
						sel->SetSelectionState(_selEnd, !sel->GetSelectionState(_selEnd));

					selChanged = true;
				}
			}
		}
	}
	return selChanged;
}

void SelectionImplementation::OnSerialize(IDataSerializer& s)
{
	s << isClicked << _selStart << _selEnd;
}

} // ui
