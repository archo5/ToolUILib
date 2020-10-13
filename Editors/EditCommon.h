
#pragma once
#include "../Model/Objects.h"


namespace ui {

enum class SelectionMode : uint8_t
{
	None,
	Single,
	Multiple,
	MultipleToggle,
};

struct ISelectionStorage
{
	virtual void ClearSelection() {}
	virtual bool GetSelectionState(size_t row) { return false; }
	virtual void SetSelectionState(size_t row, bool sel) {}
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
