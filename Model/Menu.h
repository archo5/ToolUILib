
#pragma once

#include <string>
#include <vector>
#include <functional>
#include "../Core/Memory.h"
#include "../Core/String.h"
#include "Objects.h"


namespace ui {


class Menu;

struct MenuItem
{
	MenuItem() {}
	MenuItem(StringView text, StringView shortcut = {}, bool disabled = false, bool checked = false, ArrayView<MenuItem> submenu = {});
	static MenuItem Submenu(const std::string& text, ArrayView<MenuItem> submenu, bool disabled = false)
	{
		return MenuItem(text, {}, disabled, {}, submenu);
	}
	static MenuItem Separator()
	{
		MenuItem m;
		m.isSeparator = true;
		return m;
	}

	MenuItem& Func(const std::function<void()>& f)
	{
		onActivate = [=](Menu*, int) { f(); };
		return *this;
	}
	MenuItem& Func(const std::function<void(Menu*, int)>& f)
	{
		onActivate = f;
		return *this;
	}
	StringView GetText() const
	{
		return StringView(text).until_first("\t");
	}
	StringView GetShortcut() const
	{
		return StringView(text).after_first("\t");
	}

	std::string text;
	ArrayView<MenuItem> submenu;
	bool isSeparator = false;
	bool isChecked = false;
	bool isDisabled = false;
	std::function<void(Menu*, int)> onActivate;
};


class Menu
{
public:
	struct Item
	{
		const MenuItem* data;
		int parent;
		int subelems;
	};

	Menu(ArrayView<MenuItem> rootItems, bool topbar = false);
	~Menu();

	ArrayView<Item> GetItems() const { return items; }
	void* GetNativeHandle() const;

	int Show(UIObject* owner, bool call = true);
	bool CallActivationFunction(int which);

private:

	void _Unpack(ArrayView<MenuItem> miv, int parent);

	std::vector<Item> items;
	void* impl;
};


class MenuItemElement : public UIElement
{
public:
	MenuItemElement& SetText(StringView text, StringView shortcut = {});
	MenuItemElement& SetChecked(bool checked) { isChecked = checked; return *this; }
	MenuItemElement& SetDisabled(bool disabled) { isDisabled = disabled; return *this; }
	MenuItemElement& Func(const std::function<void()>& f)
	{
		onActivate = f;
		return *this;
	}
	StringView GetText() const
	{
		return StringView(text).until_first("\t");
	}
	StringView GetShortcut() const
	{
		return StringView(text).after_first("\t");
	}

	std::string text;
	bool isChecked = false;
	bool isDisabled = false;
	std::function<void()> onActivate;
};

class MenuSeparatorElement : public UIElement
{
};

class MenuElement : public UIElement
{
public:
	virtual bool IsTopBar() { return false; }
	void OnDestroy() override;
	void OnCompleteStructure() override;

private:
	ArrayView<MenuItem> _AppendElements(UIObject* o);

	Menu* _menu = nullptr;
	std::vector<MenuItem> _items;
};

class MenuBarElement : public MenuElement
{
public:
	bool IsTopBar() override { return true; }
};


} // ui
