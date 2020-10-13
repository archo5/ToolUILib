
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

bool BasicSelection::GetSelectionState(size_t row)
{
	return _impl->sel.count(row);
}

void BasicSelection::SetSelectionState(size_t row, bool sel)
{
	if (sel)
		_impl->sel.insert(row);
	else
		_impl->sel.erase(row);
}


bool SelectionImplementation::OnEvent(UIEvent& e, ISelectionStorage* sel, size_t hoverRow, bool hovering, bool onclick)
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
		if (e.type == UIEventType::Click && e.GetButton() == UIMouseButton::Left && hoverRow < SIZE_MAX && hovering)
		{
			if (selectionMode != SelectionMode::MultipleToggle ||
				selectionMode == SelectionMode::Single) // TODO modifiers
				sel->ClearSelection();
			sel->SetSelectionState(hoverRow, !sel->GetSelectionState(hoverRow));

			selChanged = true;
		}
		return selChanged;
	}

	if (e.type == UIEventType::ButtonDown && e.GetButton() == UIMouseButton::Left && hoverRow < SIZE_MAX && hovering)
	{
		// TODO modifiers
		//e.GetKeyActionModifier();
		if (selectionMode != SelectionMode::MultipleToggle)
			sel->ClearSelection();

		sel->SetSelectionState(hoverRow, !sel->GetSelectionState(hoverRow));
		_selStart = hoverRow;
		_selEnd = hoverRow;
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
		if (isClicked && hoverRow < SIZE_MAX && hovering)
		{
			while (_selEnd != hoverRow)
			{
				if (selectionMode == SelectionMode::Single)
					sel->ClearSelection();

				bool increasing = selectionMode == SelectionMode::Single ||
					(_selStart < _selEnd
						? hoverRow < _selStart || hoverRow > _selEnd
						: hoverRow < _selEnd || hoverRow > _selStart);

				if (!increasing)
					sel->SetSelectionState(_selEnd, !sel->GetSelectionState(_selEnd));

				if (_selEnd < hoverRow)
					_selEnd++;
				else
					_selEnd--;

				if (increasing)
					sel->SetSelectionState(_selEnd, !sel->GetSelectionState(_selEnd));

				selChanged = true;
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
