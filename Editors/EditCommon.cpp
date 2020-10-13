
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

bool BasicSelection::GetSelectionState(ItemLoc item)
{
	return _impl->sel.count(item.index);
}

void BasicSelection::SetSelectionState(ItemLoc item, bool sel)
{
	if (sel)
		_impl->sel.insert(item.index);
	else
		_impl->sel.erase(item.index);
}


bool SelectionImplementation::OnEvent(UIEvent& e, ISelectionStorage* sel, ItemLoc hoverItem, bool hovering, bool onclick)
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
		if (e.type == UIEventType::Click && e.GetButton() == UIMouseButton::Left && hoverItem.IsValid() && hovering)
		{
			if (selectionMode != SelectionMode::MultipleToggle ||
				selectionMode == SelectionMode::Single) // TODO modifiers
				sel->ClearSelection();
			sel->SetSelectionState(hoverItem, !sel->GetSelectionState(hoverItem));

			selChanged = true;
		}
		return selChanged;
	}

	if (e.type == UIEventType::ButtonDown && e.GetButton() == UIMouseButton::Left && hoverItem.IsValid() && hovering)
	{
		// TODO modifiers
		//e.GetKeyActionModifier();
		if (selectionMode != SelectionMode::MultipleToggle)
			sel->ClearSelection();

		sel->SetSelectionState(hoverItem, !sel->GetSelectionState(hoverItem));
		_selStart = hoverItem;
		_selEnd = hoverItem;
		isClicked = true;
		selChanged = true;
		//e.StopPropagation();
	}
	if (e.type == UIEventType::ButtonUp && e.GetButton() == UIMouseButton::Left)
	{
		isClicked = false;
	}
	if (e.type == UIEventType::MouseMove)
	{
		if (isClicked && hoverItem.IsValid() && hovering)
		{
			if (selectionMode == SelectionMode::Single)
			{
				sel->ClearSelection();
				sel->SetSelectionState(hoverItem, !sel->GetSelectionState(hoverItem));
			}
			else if (_selEnd.cont == hoverItem.cont)
			{
				while (_selEnd != hoverItem)
				{
					if (selectionMode == SelectionMode::Single)
						sel->ClearSelection();

					bool increasing = selectionMode == SelectionMode::Single ||
						(_selStart.index < _selEnd.index
							? hoverItem.index < _selStart.index || hoverItem.index > _selEnd.index
							: hoverItem.index < _selEnd.index || hoverItem.index > _selStart.index);

					if (!increasing)
						sel->SetSelectionState(_selEnd, !sel->GetSelectionState(_selEnd));

					if (_selEnd.index < hoverItem.index)
						_selEnd.index++;
					else
						_selEnd.index--;

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
