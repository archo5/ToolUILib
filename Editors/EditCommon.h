
#pragma once
#include "../Model/Objects.h"


namespace ui {

struct MenuItemCollection;

struct IListContextMenuSource
{
	virtual void FillItemContextMenu(MenuItemCollection& mic, uintptr_t item, size_t col) = 0;
	virtual void FillListContextMenu(MenuItemCollection& mic) = 0;
};

enum class SelectionMode : uint8_t
{
	None,
	Single,
	Multiple,
	MultipleToggle,
};

struct ISelectionStorage
{
	virtual void ClearSelection() = 0;
	virtual bool GetSelectionState(uintptr_t item) = 0;
	virtual void SetSelectionState(uintptr_t item, bool sel) = 0;
};

struct BasicSelection : ISelectionStorage
{
	BasicSelection();
	~BasicSelection();

	void ClearSelection() override;
	bool GetSelectionState(uintptr_t item) override;
	void SetSelectionState(uintptr_t item, bool sel) override;

	struct BasicSelectionImpl* _impl;
};

struct SelectionImplementation
{
	bool OnEvent(Event& e, ISelectionStorage* sel, uintptr_t hoverItem, bool hovering, bool onclick = false);
	void OnReset()
	{
		selectionMode = SelectionMode::None;
	}

	SelectionMode selectionMode = SelectionMode::None;
	bool isClicked = false;
	uintptr_t _selStart = UINTPTR_MAX;
	uintptr_t _selEnd = UINTPTR_MAX;
};

} // ui
