
#pragma once

#include <string>
#include <vector>
#include <functional>
#include "../Core/Memory.h"
#include "../Core/String.h"
#include "../Core/HashTable.h"
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
		onActivate = f ? [=](Menu*, int) { f(); } : std::function<void(Menu*, int)>();
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


// text can be separated with "/", submenus are not supported
struct MenuItemCollection
{
	static constexpr const char* SEPARATOR = "/";

	struct Entry
	{
		std::string name;
		int priority = 100;
		bool checked = false;
		bool disabled = false;
		std::function<void()> function;

		HashMap<std::string, Entry*> children;

		std::vector<MenuItem> _finalizedChildItems;

		~Entry()
		{
			for (const auto& ch : children)
				delete ch.value;
		}
		void Finalize()
		{
			std::vector<Entry*> sortedChildren;
			for (const auto& ch : children)
				sortedChildren.push_back(ch.value);

			std::sort(sortedChildren.begin(), sortedChildren.end(), [](const Entry* a, const Entry* b)
			{
				if (a->priority != b->priority)
					return a->priority < b->priority;
				return a->name < b->name;
			});

			_finalizedChildItems.clear();
			for (Entry* ch : sortedChildren)
			{
				ch->Finalize();
				_finalizedChildItems.push_back(
					MenuItem(
						ch->name,
						{},
						ch->disabled,
						ch->checked,
						ch->_finalizedChildItems
					).Func(ch->function)
				);
			}
		}
	};

	Entry* CreateEntry(StringView path)
	{
		if (path.empty())
			return &root;

		size_t sep = path.find_last_at(SEPARATOR, SIZE_MAX, 0);
		auto parentPath = path.substr(0, sep);
		auto name = path.substr(sep + 1);

		auto* parent = CreateEntry(parentPath);

		Entry* E = new Entry;
		E->name.assign(name.data(), name.size());
		parent->children.insert(E->name, E);

		return E;
	}
	std::function<void()>& Add(StringView path, int priority = 100, bool checked = false, bool disabled = false)
	{
		Entry* E = CreateEntry(path);
		E->priority = priority;
		E->checked = checked;
		E->disabled = disabled;
		return E->function;
	}
	void Clear()
	{
		root.~Entry();
		new (&root) Entry;
	}

	ArrayView<MenuItem> Finalize()
	{
		root.Finalize();
		return root._finalizedChildItems;
	}

	Entry root;
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
	static constexpr bool Persistent = true;

	virtual bool IsTopBar() { return false; }
	void OnDestroy() override;
	void OnCompleteStructure() override;

private:
	ArrayView<MenuItem> _AppendElements(UIObject* o);
	std::string _GenerateItemUID();

	Menu* _menu = nullptr;
	std::vector<MenuItem> _items;
	std::string _uid;
};

class MenuBarElement : public MenuElement
{
public:
	bool IsTopBar() override { return true; }
};


} // ui
