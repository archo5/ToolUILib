
#pragma once

#include <string>
#include <functional>
#include "../Core/Array.h"
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
	Array<MenuItem> submenu;
	bool isSeparator = false;
	bool isChecked = false;
	bool isDisabled = false;
	std::function<void(Menu*, int)> onActivate;
};


// text can be separated with "/", submenus are not supported
struct MenuItemCollection
{
	static constexpr const char* SEPARATOR = "/";
	static constexpr int SEPARATOR_THRESHOLD = 100;
	static constexpr int BASE_ADVANCE = 10000;
	static constexpr int MIN_SAFE_PRIORITY = -4950;
	static constexpr int MAX_SAFE_PRIORITY = 4950;

	struct Entry
	{
		std::string name;
		int minPriority = 0;
		int maxPriority = 0;
		bool checked = false;
		bool disabled = false;
		std::function<void()> function;

		HashMap<std::string, Entry*> children;

		Array<MenuItem> _finalizedChildItems;

		~Entry();
		void Finalize();
	};

	Entry* CreateEntry(StringView path, int priority);
	std::function<void()>& Add(StringView path, bool disabled = false, bool checked = false, int priorityTweak = 0)
	{
		Entry* E = CreateEntry(path, basePriority + priorityTweak);
		E->checked = checked;
		E->disabled = disabled;
		return E->function;
	}
	std::function<void()>& AddNext(StringView path, bool disabled = false, bool checked = false)
	{
		basePriority++;
		auto& fn = Add(path, disabled, checked);
		basePriority++;
		return fn;
	}
	void Clear()
	{
		root.~Entry();
		new (&root) Entry;
		basePriority = 0;
	}
	void NewOrderedSet()
	{
		basePriority += 1;
	}
	void NewSection()
	{
		basePriority += SEPARATOR_THRESHOLD;
	}
	void StartNew()
	{
		Clear();
		_version++;
	}
	bool HasAny() const
	{
		return root.children.size() != 0;
	}
	uint32_t GetVersion() const { return _version; }

	ArrayView<MenuItem> Finalize()
	{
		root.Finalize();
		return root._finalizedChildItems;
	}

	Entry root;
	int basePriority = 0;
	uint32_t _version = 0;
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

	Array<Item> items;
	void* impl;
};

struct TopMenu
{
	Array<MenuItem> _items;
	Menu _menu;
	NativeWindowBase* _window;

	TopMenu(NativeWindowBase* w, ArrayView<MenuItem> rootItems);
	~TopMenu();
};

} // ui
