
#pragma once
#include "../Model/Objects.h"


namespace ui {

struct MenuItemCollection;

struct IListContextMenuSource
{
	virtual void FillItemContextMenu(MenuItemCollection& mic, size_t row, size_t col) = 0;
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
	virtual bool GetSelectionState(size_t row) = 0;
	virtual void SetSelectionState(size_t row, bool sel) = 0;
};

struct BasicSelection : ISelectionStorage
{
	BasicSelection();
	~BasicSelection();

	void ClearSelection() override;
	bool GetSelectionState(size_t row) override;
	void SetSelectionState(size_t row, bool sel) override;

	struct BasicSelectionImpl* _impl;
};

struct SelectionImplementation
{
	bool OnEvent(UIEvent& e, ISelectionStorage* sel, size_t hoverRow, bool hovering, bool onclick = false);
	void OnSerialize(IDataSerializer& s);

	SelectionMode selectionMode = SelectionMode::None;
	bool isClicked = false;
	size_t _selStart = 0;
	size_t _selEnd = 0;
};

} // ui
