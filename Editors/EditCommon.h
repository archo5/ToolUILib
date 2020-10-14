
#pragma once
#include "../Model/Objects.h"


namespace ui {

struct ItemLoc
{
	uintptr_t index; // pointer to node or offset to entry in node container
	void* cont; // pointer to node container or nothing

	ItemLoc() : index(UINTPTR_MAX), cont(nullptr) {}
	ItemLoc(uintptr_t n, void* c = nullptr) : index(n), cont(c) {}
	static ItemLoc Null() { return { UINTPTR_MAX, nullptr }; }
	bool IsValid() const { return *this != Null(); }
	bool IsNull() const { return *this == Null(); }
	bool operator == (ItemLoc o) const { return index == o.index && cont == o.cont; }
	bool operator != (ItemLoc o) const { return index != o.index || cont != o.cont; }
};

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
	bool OnEvent(UIEvent& e, ISelectionStorage* sel, uintptr_t hoverItem, bool hovering, bool onclick = false);
	void OnSerialize(IDataSerializer& s);

	SelectionMode selectionMode = SelectionMode::None;
	bool isClicked = false;
	uintptr_t _selStart = UINTPTR_MAX;
	uintptr_t _selEnd = UINTPTR_MAX;
};

} // ui
