
#include "Menu.h"
#include "Objects.h"
#include "Native.h"

#include <Windows.h>


namespace ui {


MenuItem::MenuItem(StringView text, StringView shortcut, bool disabled, bool checked, ArrayView<MenuItem> submenu)
{
	this->text.append(text.data(), text.size());
	if (!shortcut.empty())
	{
		this->text += "\t";
		this->text.append(shortcut.data(), shortcut.size());
	}
	this->submenu = submenu;
	this->isChecked = checked;
	this->isDisabled = disabled;
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
	_items.reserve(CountChildElements());
	auto list = _AppendElements(this);

	auto oldmenu = _menu;
	_menu = new Menu(list, IsTopBar());
	if (IsTopBar())
		GetNativeWindow()->SetMenu(_menu);
	delete oldmenu; // deleted at the end, in case it was top bar, to avoid momentary disappearance
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
			_items[i++] = MenuItem(item->text, {}, item->isDisabled, item->isChecked, _AppendElements(ch));
		}
		else if (auto* sep = dynamic_cast<MenuSeparatorElement*>(ch))
		{
			_items[i++] = MenuItem::Separator();
		}
	}

	return { &_items[start], count };
}


} // ui
