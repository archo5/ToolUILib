
#include "Menu.h"
#include "Objects.h"
#include "Native.h"

#include <Windows.h>
#include <algorithm>


namespace ui {


MenuItem::MenuItem(StringView text, StringView shortcut, bool disabled, bool checked, ArrayView<MenuItem> submenu)
{
	this->text.append(text.data(), text.size());
	if (!shortcut.empty())
	{
		this->text += "\t";
		this->text.append(shortcut.data(), shortcut.size());
	}
	this->submenu.assign(submenu.begin(), submenu.end());
	this->isChecked = checked;
	this->isDisabled = disabled;
}


MenuItemCollection::Entry::~Entry()
{
	for (const auto& ch : children)
		delete ch.value;
}

void MenuItemCollection::Entry::Finalize()
{
	if (children.size() == 0)
		return;

	std::vector<Entry*> sortedChildren;
	for (const auto& ch : children)
		sortedChildren.push_back(ch.value);

	std::sort(sortedChildren.begin(), sortedChildren.end(), [](const Entry* a, const Entry* b)
	{
		if (a->maxPriority != b->minPriority)
			return a->maxPriority < b->minPriority;
		return a->name < b->name;
	});

	_finalizedChildItems.clear();
	int prevPrio = sortedChildren[0]->minPriority;
	for (Entry* ch : sortedChildren)
	{
		if (ch->minPriority - prevPrio >= SEPARATOR_THRESHOLD)
		{
			_finalizedChildItems.push_back(MenuItem::Separator());
		}
		prevPrio = ch->maxPriority;

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

MenuItemCollection::Entry* MenuItemCollection::CreateEntry(StringView path, int priority)
{
	if (path.empty())
		return &root;

	size_t sep = path.find_last_at(SEPARATOR);
	auto parentPath = path.substr(0, sep == SIZE_MAX ? 0 : sep);
	auto name = path.substr(sep == SIZE_MAX ? 0 : sep + strlen(SEPARATOR));
	std::string nameStr(name.data(), name.size());

	auto* parent = CreateEntry(parentPath, priority);

	auto it = parent->children.find(nameStr);
	if (it.is_valid())
	{
		Entry* E = it->value;
		E->minPriority = min(E->minPriority, priority);
		E->maxPriority = max(E->maxPriority, priority);
		return E;
	}
	else
	{
		Entry* E = new Entry;
		E->name = nameStr;
		E->minPriority = priority;
		E->maxPriority = priority;
		parent->children.insert(E->name, E);

		return E;
	}
}


HMENU CreateMenuFrom(ArrayView<Menu::Item> items, Menu::Item* base, bool topbar = false)
{
	auto hmenu = topbar ? CreateMenu() : CreatePopupMenu();
	for (size_t i = 0; i < items.size(); i += 1 + items[i].subelems)
	{
		const auto& item = *items[i].data;
		UINT flags = 0;
		if (item.isSeparator)
			flags |= MF_SEPARATOR;
		else
		{
			flags |= MF_STRING;
			if (item.isChecked)
				flags |= MF_CHECKED;
			if (item.isDisabled)
				flags |= MF_DISABLED | MF_GRAYED;
		}
		UINT_PTR id = &items[i] - base + 1;
		if (!item.submenu.empty())
		{
			flags |= MF_POPUP;
			id = (UINT_PTR)CreateMenuFrom(ArrayView<Menu::Item>(&items[i + 1], items[i].subelems), base);
		}
		AppendMenuA(hmenu, flags, id, item.text.c_str());
	}
	return hmenu;
}

Menu::Menu(ArrayView<MenuItem> rootItems, bool topbar)
{
	_Unpack(rootItems, SIZE_MAX);

	auto hmenu = CreateMenuFrom(items, items.data(), topbar);
	impl = (void*)hmenu;
}

Menu::~Menu()
{
	DestroyMenu((HMENU)impl);
}

void* Menu::GetNativeHandle() const
{
	return impl;
}

int Menu::Show(UIObject* owner, bool call)
{
	assert(owner);
	if (!owner)
		return -1;
	auto* nativeWindow = owner->GetNativeWindow();
	assert(nativeWindow);
	if (!nativeWindow)
		return -1;

	TPMPARAMS params;
	memset(&params, 0, sizeof(params));
	params.cbSize = sizeof(params);
	POINT cp = {};
	GetCursorPos(&cp);
	int selected = TrackPopupMenuEx((HMENU)impl, TPM_NONOTIFY | TPM_RETURNCMD, cp.x, cp.y, (HWND) nativeWindow->GetNativeHandle(), &params);
	int at = selected - 1;

	if (selected && call)
	{
		if (const auto& f = items[at].data->onActivate)
			f(this, at);
	}

	return at;
}

bool Menu::CallActivationFunction(int which)
{
	if (which < 0 || size_t(which) >= items.size() || !items[which].data->onActivate)
		return false;
	items[which].data->onActivate(this, which);
	return true;
}

void Menu::_Unpack(ArrayView<MenuItem> miv, int parent)
{
	if (items.capacity() < items.size() + miv.size())
		items.reserve(items.capacity() * 2 + miv.size());

	for (const auto& mi : miv)
	{
		items.push_back({ &mi, parent, 0 });
		if (!mi.submenu.empty())
		{
			size_t start = items.size();
			_Unpack(mi.submenu, start - 1);
			items[start - 1].subelems = items.size() - start;
		}
	}
}


TopMenu::TopMenu(NativeWindowBase* w, ArrayView<MenuItem> rootItems) :
	_items(rootItems.begin(), rootItems.end()), _menu(_items, true), _window(w)
{
	_window->SetMenu(&_menu);
}

TopMenu::~TopMenu()
{
	//_window->SetMenu(nullptr);
}

#if 0
MenuItemElement& MenuItemElement::SetText(StringView text, StringView shortcut)
{
	this->text.reserve(text.size() + (shortcut.empty() ? 0 : 1) + shortcut.size());
	this->text.assign(text.data(), text.size());
	if (!shortcut.empty())
	{
		this->text += "\t";
		this->text.append(shortcut.data(), shortcut.size());
	}
	return *this;
}


void MenuElement::OnDestroy()
{
	if (_menu)
	{
		if (IsTopBar())
			GetNativeWindow()->SetMenu(nullptr);
		delete _menu;
		_menu = nullptr;
	}
}

void MenuElement::OnCompleteStructure()
{
	_items.clear();
	_items.reserve(CountChildrenRecursive());
	auto list = _AppendElements(this);
	std::string uid = _GenerateItemUID();

	if (_menu == nullptr || uid != _uid)
	{
		auto oldmenu = _menu;
		_menu = new Menu(list, IsTopBar());
		if (IsTopBar())
			GetNativeWindow()->SetMenu(_menu);
		delete oldmenu; // deleted at the end, in case it was top bar, to avoid momentary disappearance
	}
	std::swap(_uid, uid);
}

ArrayView<MenuItem> MenuElement::_AppendElements(UIObject* o)
{
	size_t start = _items.size();

	// reserve space
	size_t count = 0;
	for (auto* ch = o->firstChild; ch; ch = ch->next)
	{
		if (typeid(*ch) == typeid(MenuItemElement) || typeid(*ch) == typeid(MenuSeparatorElement))
			count++;
	}
	if (!count)
		return {};
	_items.resize(start + count);

	size_t i = start;
	for (auto* ch = o->firstChild; ch; ch = ch->next)
	{
		if (auto* item = dynamic_cast<MenuItemElement*>(ch))
		{
			_items[i++] = MenuItem(item->text, {}, item->isDisabled, item->isChecked, _AppendElements(ch)).Func(item->onActivate);
		}
		else if (auto* sep = dynamic_cast<MenuSeparatorElement*>(ch))
		{
			_items[i++] = MenuItem::Separator();
		}
	}

	return { &_items[start], count };
}

std::string MenuElement::_GenerateItemUID()
{
	std::string ret;
	for (const MenuItem& item : _items)
	{
		if (item.isSeparator)
		{
			ret.push_back('S');
			continue;
		}
		ret.push_back('I');
		ret.push_back((item.isChecked << 0) | (item.isDisabled << 1));

		uint32_t len = item.text.size();
		ret.append((const char*)&len, sizeof(len));
		ret.append(item.text);

		len = item.submenu.size();
		ret.append((const char*)&len, sizeof(len));
		if (len)
		{
			len = item.submenu.data() - _items.data();
			ret.append((const char*)&len, sizeof(len));
		}
	}
	return ret;
}
#endif

} // ui
