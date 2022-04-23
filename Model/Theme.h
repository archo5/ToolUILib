
#pragma once

#include "Layout.h"
#include "../Render/Render.h"
#include "../Core/HashTable.h"
#include "../Core/StaticID.h"


namespace ui {

struct ThemeData;

struct IThemeStructLoader
{
	size_t id = 0;

	static size_t AllocID();

	virtual const char* GetName() const = 0;
	virtual size_t GetSize() const = 0;
	virtual void* ReadStruct(ThemeData&, IObjectIterator&) = 0;
	virtual void FreeStruct(void*) = 0;
};

template <class T> struct ThemeStructLoaderImpl : IThemeStructLoader
{
	ThemeStructLoaderImpl() { id = AllocID(); }
	const char* GetName() const override { return T::NAME; }
	size_t GetSize() const override { return sizeof(T); }
	void* ReadStruct(ThemeData& curTheme, IObjectIterator& oi) override
	{
		T* data = new T;
		data->Serialize(curTheme, oi);
		return data;
	}
	void FreeStruct(void* data) override { delete (T*)data; }
};

template <class T> IThemeStructLoader* GetThemeStructLoader()
{
	static ThemeStructLoaderImpl<T> impl;
	return &impl;
}

struct IThemeLoader
{
	virtual PainterHandle LoadPainter(const FieldInfo& FI) = 0;
	virtual Color4b LoadColor(const FieldInfo& FI) = 0;
	virtual draw::ImageSetHandle FindImageSet(const std::string& name) = 0;
};

void OnFieldBorderBox(IObjectIterator& oi, const FieldInfo& FI, AABB2f& bbox);
void OnFieldPainter(IObjectIterator& oi, ThemeData& td, const FieldInfo& FI, PainterHandle& ph);

using StaticID_Color = StaticID<Color4b>;
using StaticID_ImageSet = StaticID<draw::ImageSet>;
using StaticID_Painter = StaticID<IPainter>;
using StaticID_Style = StaticID<StyleBlock>;

struct ThemeData : RefCountedST
{
	struct CustomStructData : RefCountedST
	{
		IThemeStructLoader* structLoader = nullptr;
		void* defaultInstance = nullptr;
		HashMap<std::string, void*> instances;
		std::vector<void*> cachedInstances;
	};
	using CustomStructDataHandle = RCHandle<CustomStructData>;

	HashMap<std::string, Color4b> colors;
	HashMap<std::string, draw::ImageSetHandle> imageSets;
	HashMap<std::string, PainterHandle> painters;
	HashMap<std::string, StyleBlockRef> styles;

	std::vector<Optional<Color4b>> _cachedColors;
	std::vector<draw::ImageSetHandle> _cachedImageSets;
	std::vector<PainterHandle> _cachedPainters;
	std::vector<StyleBlockRef> _cachedStyles;
	std::vector<CustomStructDataHandle> _cachedStructs;

	HashMap<std::string, RCHandle<RefCountedST>> _customStructSources;

	Optional<Color4b> FindColorByName(const std::string& name);
	draw::ImageSetHandle FindImageSetByName(const std::string& name);
	PainterHandle FindPainterByName(const std::string& name);
	StyleBlockRef FindStyleByName(const std::string& name);
	void* _FindStructByNameImpl(IThemeStructLoader* loader, const std::string& name);
	template <class T> T* FindStructByName(const std::string& name)
	{
		return static_cast<T*>(_FindStructByNameImpl(GetThemeStructLoader<T>(), name));
	}

	Optional<Color4b> GetColor(const StaticID_Color& id);
	Color4b GetBackgroundColor(const StaticID_Color& id);
	Color4b GetForegroundColor(const StaticID_Color& id);
	draw::ImageSetHandle GetImageSet(const StaticID_ImageSet& id);
	PainterHandle GetPainter(const StaticID_Painter& id, bool returnDefaultIfMissing = true);
	StyleBlockRef GetStyle(const StaticID_Style& id, bool returnDefaultIfMissing = true);
	void* _GetStructImpl(IThemeStructLoader* loader, const char* name, uint32_t id, bool returnDefaultIfMissing);
	template <class T> T* GetStruct(const StaticID<T>& id, bool returnDefaultIfMissing = true)
	{
		return static_cast<T*>(_GetStructImpl(GetThemeStructLoader<T>(), id._name, id._id, returnDefaultIfMissing));
	}

	void LoadTheme(StringView folder);
};
using ThemeDataHandle = RCHandle<ThemeData>;

ThemeDataHandle LoadTheme(StringView folder);

ThemeData* GetCurrentTheme();
void SetCurrentTheme(ThemeData* theme);


typedef IPainter* PainterCreateFunc(IThemeLoader*, IObjectIterator&);

void RegisterPainter(const char* type, PainterCreateFunc* createFunc);

} // ui
