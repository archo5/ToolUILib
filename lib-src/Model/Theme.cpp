
#include "Theme.h"

#include "../Core/Logging.h"
#include "../Core/SerializationJSON.h"
#include "../Core/FileSystem.h"

#include "../../ThirdParty/json.h"


namespace ui {

LogCategory LOG_THEME("Theme");

size_t IThemeStructLoader::AllocID()
{
	static size_t v;
	return v++;
}

struct ThemeFile : RefCountedST
{
	BufferHandle text;
	JSONUnserializerObjectIterator unserializer;
};
using ThemeFileHandle = RCHandle<ThemeFile>;


void OnFieldBorderBox(IObjectIterator& oi, const FieldInfo& FI, AABB2f& bbox)
{
	if (!oi.IsUnserializer())
		return;

	std::string name = FI.name;
	FieldInfo FIcopy = FI;

	float val = 0;
	OnField(oi, FIcopy, val);
	bbox = AABB2f::UniformBorder(val);

	name = FI.name;
	name += "H";
	FIcopy.name = name.c_str();
	val = NAN;
	OnField(oi, FIcopy, val);
	if (isfinite(val))
		bbox.x0 = bbox.x1 = val;

	name = FI.name;
	name += "V";
	FIcopy.name = name.c_str();
	val = NAN;
	OnField(oi, FIcopy, val);
	if (isfinite(val))
		bbox.y0 = bbox.y1 = val;

	name = FI.name;
	name += "Left";
	FIcopy.name = name.c_str();
	val = NAN;
	OnField(oi, FIcopy, val);
	if (isfinite(val))
		bbox.x0 = val;

	name = FI.name;
	name += "Right";
	FIcopy.name = name.c_str();
	val = NAN;
	OnField(oi, FIcopy, val);
	if (isfinite(val))
		bbox.x1 = val;

	name = FI.name;
	name += "Top";
	FIcopy.name = name.c_str();
	val = NAN;
	OnField(oi, FIcopy, val);
	if (isfinite(val))
		bbox.y0 = val;

	name = FI.name;
	name += "Bottom";
	FIcopy.name = name.c_str();
	val = NAN;
	OnField(oi, FIcopy, val);
	if (isfinite(val))
		bbox.y1 = val;
}

void OnFieldPainter(IObjectIterator& oi, ThemeData& td, const FieldInfo& FI, PainterHandle& ph)
{
	std::string text;
	OnField(oi, FI, text);
	ph = td.FindPainterByName(text);
}

void OnFieldFontSettings(IObjectIterator& oi, ThemeData& td, const FieldInfo& FI, FontSettings& fs)
{
	std::string text;
	OnField(oi, FI, text);
	if (auto* s = td.FindStructByName<FontSettings>(text))
		fs = *s;
	else
		fs = {};
}


ThemeData::CustomStructData::~CustomStructData()
{
	structLoader->FreeStruct(defaultInstance);
	for (auto kvp : instances)
		structLoader->FreeStruct(kvp.value);
}


static const char* EnumKeys_Presence[] =
{
	"Visible",
	"LayoutOnly",
	"None",
	nullptr,
};
template <> struct EnumKeys<Presence> : EnumKeysStringList<Presence, EnumKeys_Presence> {};

static const char* EnumKeys_HAlign[] =
{
	"",
	"left",
	"center",
	"right",
	"justify",
	nullptr,
};
template <> struct EnumKeys<HAlign> : EnumKeysStringList<HAlign, EnumKeys_HAlign> {};


Optional<Color4b> ThemeData::FindColorByName(const std::string& name)
{
	return colors.GetValueOrDefault(name);
}

draw::ImageSetHandle ThemeData::FindImageSetByName(const std::string& name)
{
	return imageSets.GetValueOrDefault(name);
}

PainterHandle ThemeData::FindPainterByName(const std::string& name)
{
	return painters.GetValueOrDefault(name);
}

static void InitStruct(ThemeData& td, ThemeData::CustomStructData& csd, IThemeStructLoader* loader)
{
	csd.structLoader = loader;
	JSONUnserializerObjectIterator oi;
	oi.Parse("{}");
	oi.BeginObject({}, loader->GetName());
	csd.defaultInstance = loader->ReadStruct(td, oi);
}

static ThemeData::CustomStructData& GetStructData(ThemeData& td, IThemeStructLoader* loader)
{
	if (loader->id >= td._cachedStructs.size())
	{
		td._cachedStructs.Resize(loader->id + 1);
		auto csd = new ThemeData::CustomStructData;
		InitStruct(td, *csd, loader);
		td._cachedStructs[loader->id] = csd;
	}
	return *td._cachedStructs[loader->id];
}

void* LoadStructFromSource(ThemeData& td, ThemeData::CustomStructData& csd, IThemeStructLoader* loader, const std::string& name)
{
	auto key = to_string(name, ":", loader->GetName());
	if (auto css = td._customStructSources.GetValueOrDefault(key))
	{
		auto* tf = static_cast<ThemeFile*>(css.get_ptr());
		tf->unserializer.BeginEntry(tf->unserializer._root);
		tf->unserializer.BeginObject(key.c_str(), loader->GetName());
		void* data = loader->ReadStruct(td, tf->unserializer);
		tf->unserializer.EndObject();
		tf->unserializer.EndEntry();

		csd.instances.Insert(name, data);
		return data;
	}
	return nullptr;
}

void* ThemeData::_FindStructByNameImpl(IThemeStructLoader* loader, const std::string& name)
{
	auto& csd = GetStructData(*this, loader);

	if (auto* pv = csd.instances.GetValuePtr(name))
		return *pv;

	if (auto* data = LoadStructFromSource(*this, csd, loader, name))
		return data;

	return nullptr;
}

static size_t CalcNewSize(size_t curSize, size_t curMaxCount)
{
	if (curMaxCount <= curSize)
		return curSize;
	return curSize + curMaxCount;
}

template <class TC, class TID> TC& GetCacheSlot(Array<TC>& cache, const StaticID<TID>& id)
{
	size_t newSize = CalcNewSize(cache.size(), StaticID<TID>::GetCount());
	if (cache.size() < newSize)
		cache.Resize(newSize);
	return cache[id._id];
}

Optional<Color4b> ThemeData::GetColor(const StaticID_Color& id)
{
	auto& slot = GetCacheSlot(_cachedColors, id);
	if (!slot.HasValue())
		slot = FindColorByName(id._name);
	return slot;
}

static StaticID_Color sid_color_background("background");
Color4b ThemeData::GetBackgroundColor(const StaticID_Color& id)
{
	if (auto col = GetColor(id))
		return col.GetValue();
	if (auto col = GetColor(sid_color_background))
		return col.GetValue();
	return { 0 };
}

static StaticID_Color sid_color_foreground("foreground");
Color4b ThemeData::GetForegroundColor(const StaticID_Color& id)
{
	if (auto col = GetColor(id))
		return col.GetValue();
	if (auto col = GetColor(sid_color_foreground))
		return col.GetValue();
	return { 255 };
}

draw::ImageSetHandle ThemeData::GetImageSet(const StaticID_ImageSet& id)
{
	auto& slot = GetCacheSlot(_cachedImageSets, id);
	if (!slot)
		slot = FindImageSetByName(id._name);
	return slot;
}

PainterHandle ThemeData::GetPainter(const StaticID_Painter& id, bool returnDefaultIfMissing)
{
	auto& slot = GetCacheSlot(_cachedPainters, id);
	if (!slot)
		slot = FindPainterByName(id._name);
	return slot ? slot : returnDefaultIfMissing ? EmptyPainter::Get() : nullptr;
}

void* ThemeData::_GetStructImpl(IThemeStructLoader* loader, const char* name, uint32_t id, bool returnDefaultIfMissing)
{
	auto& csd = GetStructData(*this, loader);

	if (id < csd.cachedInstances.size() && csd.cachedInstances[id])
		return csd.cachedInstances[id];

	if (id >= csd.cachedInstances.size())
		csd.cachedInstances.ResizeWith(id + 1, nullptr);

	if (auto* pv = csd.instances.GetValuePtr(name))
	{
		csd.cachedInstances[id] = *pv;
		return *pv;
	}

	if (auto* data = LoadStructFromSource(*this, csd, loader, name))
	{
		csd.cachedInstances[id] = data;
		return data;
	}

	if (returnDefaultIfMissing)
		return csd.defaultInstance;

	return nullptr;
}

static ThemeDataHandle g_curTheme;
ThemeData* GetCurrentTheme()
{
	return g_curTheme;
}

void SetCurrentTheme(ThemeData* theme)
{
	g_curTheme = theme;
}


static HashMap<std::string, PainterCreateFunc*> g_painterCreateFuncs;

struct ThemeElementRef
{
	ThemeFileHandle file;
	const char* key;
};

struct ThemeLoaderData : IThemeLoader
{
	Array<ThemeFileHandle> files;
	HashMap<std::string, ThemeElementRef> colors;
	HashMap<std::string, ThemeElementRef> imageSets;
	HashMap<std::string, ThemeElementRef> painters;
	ThemeDataHandle loadedData;

	ThemeFileHandle curFile;

	PainterHandle LoadPainter(const FieldInfo& FI) override
	{
		if (!curFile)
			return nullptr;

		auto* e = curFile->unserializer.FindEntry(FI.name);
		if (!e)
			return nullptr;

		if (e->type == json_type_string)
		{
			auto* s = json_value_as_string(e);

			return _LoadPainterByName(s->string);
		}
		else if (e->type == json_type_object)
		{
			curFile->unserializer.BeginEntry(e);

			auto type = curFile->unserializer.ReadString("__");
			if (!type.HasValue())
				return nullptr;

			auto fnPainterCreate = g_painterCreateFuncs.GetValueOrDefault(type.GetValue());
			if (!fnPainterCreate)
				return nullptr;

			auto ret = fnPainterCreate(this, curFile->unserializer);

			curFile->unserializer.EndEntry();
			return ret;
		}

		return nullptr;
	}

	PainterHandle _LoadPainterByName(const std::string& name)
	{
		if (auto* pp = loadedData->painters.GetValuePtr(name))
			return *pp;

		auto* pthelref = painters.GetValuePtr(name);
		if (!pthelref)
			return nullptr;

		auto& u = pthelref->file->unserializer;
		auto prevCurFile = curFile;
		curFile = pthelref->file;
		u.BeginEntry(u._root);

		auto loaded = LoadPainter(pthelref->key);
		loadedData->painters.Insert(name, loaded);

		u.EndEntry();
		curFile = prevCurFile;

		return loaded;
	}

	Color4b LoadColor(const FieldInfo& FI) override
	{
		std::string text;
		OnField(curFile->unserializer, FI, text);
		StringView s = text;

		if (s.starts_with("f:"))
		{
			// TODO locale
			float tmp[4];
			switch (sscanf(text.c_str(), "f:%g;%g;%g;%g", &tmp[0], &tmp[1], &tmp[2], &tmp[3]))
			{
			default: return Color4b::White();
			case 1: return Color4f(tmp[0]);
			case 2: return Color4f(tmp[0], tmp[1]);
			case 3: return Color4f(tmp[0], tmp[1], tmp[2]);
			case 4: return Color4f(tmp[0], tmp[1], tmp[2], tmp[3]);
			}
		}
		if (s.starts_with("b:"))
		{
			unsigned tmp[4];
			switch (sscanf(text.c_str(), "b:%u;%u;%u;%u", &tmp[0], &tmp[1], &tmp[2], &tmp[3]))
			{
			default: return Color4b::White();
			case 1: return Color4b(tmp[0]);
			case 2: return Color4b(tmp[0], tmp[1]);
			case 3: return Color4b(tmp[0], tmp[1], tmp[2]);
			case 4: return Color4b(tmp[0], tmp[1], tmp[2], tmp[3]);
			}
		}
		// TODO refs
		return Color4b::White();
	}

	draw::ImageSetHandle FindImageSet(const std::string& name)
	{
		return loadedData->imageSets.GetValueOrDefault(name, nullptr);
	}
};

void ThemeData::LoadTheme(StringView folder)
{
	ThemeLoaderData tld;

	auto dih = FSCreateDirectoryIterator(folder);
	std::string entry;
	while (dih->GetNext(entry))
	{
		if (StringView(entry).ends_with(".json"))
		{
			auto tf = AsRCHandle(new ThemeFile);

			auto path = to_string(folder, "/", entry);
			auto frr = FSReadTextFile(path);
			if (!frr.data)
				continue;

			tf->text = frr.data;

			if (tf->unserializer.Parse(tf->text->GetStringView(), JPF_AllowAll))
			{
				tld.files.Append(tf);

				// enumerate the contents
				if (auto* root = tf->unserializer._root)
				{
					if (root->type == json_type_object)
					{
						auto* obj = static_cast<json_object_s*>(root->payload);
						for (auto* el = obj->start; el; el = el->next)
						{
							StringView name(el->name->string, el->name->string_size);
							if (name.ends_with(":color"))
							{
								auto key = to_string(name.substr(0, name.FindLastAt(":color")));
								tld.colors.Insert(key, { tf, el->name->string });
							}
							else if (name.ends_with(":imgset"))
							{
								auto key = to_string(name.substr(0, name.FindLastAt(":imgset")));
								tld.imageSets.Insert(key, { tf, el->name->string });
							}
							else if (name.ends_with(":painter"))
							{
								auto key = to_string(name.substr(0, name.FindLastAt(":painter")));
								tld.painters.Insert(key, { tf, el->name->string });
							}
							else
							{
								_customStructSources.Insert(el->name->string, tf);
							}
						}
					}
				}
			}
			else
			{
				LogError(LOG_THEME, "Failed to parse %s\n", path.c_str());
			}
		}
	}

	tld.loadedData = this;

	for (auto& col : tld.colors)
	{
		auto& u = col.value.file->unserializer;
		tld.curFile = col.value.file;

		auto loaded = tld.LoadColor(col.value.key);
		tld.loadedData->colors.Insert(col.key, loaded);

		tld.curFile = nullptr;
	}

	for (auto& imgSet : tld.imageSets)
	{
		auto& u = imgSet.value.file->unserializer;
		u.BeginDict(imgSet.value.key);
		{
			auto loaded = AsRCHandle(new draw::ImageSet);

			bool hasBaseType = false;
			bool hasBaseEdge = false;
			if (auto edge = u.ReadFloat("edge"))
			{
				loaded->baseEdgeWidth = AABB2f::UniformBorder(float(edge.GetValue()));
				loaded->type = draw::ImageSetType::Sliced;

				hasBaseType = true;
				hasBaseEdge = true;
			}

			if (auto size = u.ReadFloat("size"))
				loaded->baseSize = { float(size.GetValue()), float(size.GetValue()) };

			if (auto sizeX = u.ReadFloat("sizeX"))
				loaded->baseSize.x = float(sizeX.GetValue());

			if (auto sizeY = u.ReadFloat("sizeY"))
				loaded->baseSize.y = float(sizeY.GetValue());

			if (auto type = u.ReadString("type"))
			{
				if (type.GetValue() == "icon")
					loaded->type = draw::ImageSetType::Icon;
				else if (type.GetValue() == "sliced")
					loaded->type = draw::ImageSetType::Sliced;
				else if (type.GetValue() == "pattern")
					loaded->type = draw::ImageSetType::Pattern;
				else if (type.GetValue() == "raw")
					loaded->type = draw::ImageSetType::RawImage;
				hasBaseType = true;
			}

			size_t count = u.BeginArray(0, "images");
			for (size_t i = 0; i < count; i++)
			{
				draw::ImageSet::BitmapImageEntry e;

				u.BeginObject({}, "ImageSetBitmapEntry");
				{
					if (auto path = u.ReadString("path"))
					{
						auto fullpath = to_string(folder, "/", path.GetValue());
						draw::TexFlags flags =
							loaded->type == draw::ImageSetType::Pattern
							? draw::TexFlags::Repeat
							: draw::TexFlags::Packed;
						e.image = draw::ImageLoadFromFile(fullpath, flags);
					}
					if (auto edge = u.ReadInt("edge"))
					{
						e.edgeWidth = AABB2f::UniformBorder(float(edge.GetValue()));
						if (!hasBaseType)
							loaded->type = draw::ImageSetType::Sliced;
						if (!hasBaseEdge)
							loaded->baseEdgeWidth = e.edgeWidth;
					}

					if (e.image)
					{
						if (e.edgeWidth.x0 != 0 ||
							e.edgeWidth.y0 != 0 ||
							e.edgeWidth.x1 != 0 ||
							e.edgeWidth.y1 != 0)
						{
							float iw = 1.0f / e.image->GetWidth();
							float ih = 1.0f / e.image->GetHeight();
							e.innerUV.x0 = e.edgeWidth.x0 * iw;
							e.innerUV.y0 = e.edgeWidth.y0 * ih;
							e.innerUV.x1 = 1 - e.edgeWidth.x1 * iw;
							e.innerUV.y1 = 1 - e.edgeWidth.y1 * ih;
						}
					}
				}
				u.EndObject();

				loaded->bitmapImageEntries.Append(new draw::ImageSet::BitmapImageEntry(e));
			}
			u.EndArray();

			count = u.BeginArray(0, "vectorimages");
			for (size_t i = 0; i < count; i++)
			{
				draw::ImageSet::VectorImageEntry e;

				u.BeginObject({}, "ImageSetBitmapEntry");
				{
					if (auto path = u.ReadString("path"))
					{
						auto fullpath = to_string(folder, "/", path.GetValue());
						e.image = VectorImageLoadFromFile(fullpath);
					}
					if (auto edge = u.ReadInt("edge"))
					{
						e.edgeWidth = AABB2f::UniformBorder(float(edge.GetValue()));
						if (!hasBaseType)
							loaded->type = draw::ImageSetType::Sliced;
						if (!hasBaseEdge)
							loaded->baseEdgeWidth = e.edgeWidth;
					}

					OnField(u, "minSizeHint", e.minSizeHint);

					if (e.image)
					{
						if (e.edgeWidth.x0 != 0 ||
							e.edgeWidth.y0 != 0 ||
							e.edgeWidth.x1 != 0 ||
							e.edgeWidth.y1 != 0)
						{
							float iw = 1.0f / e.image->GetWidth();
							float ih = 1.0f / e.image->GetHeight();
							e.innerUV.x0 = e.edgeWidth.x0 * iw;
							e.innerUV.y0 = e.edgeWidth.y0 * ih;
							e.innerUV.x1 = 1 - e.edgeWidth.x1 * iw;
							e.innerUV.y1 = 1 - e.edgeWidth.y1 * ih;
						}
					}
				}
				u.EndObject();

				loaded->vectorImageEntries.Append(new draw::ImageSet::VectorImageEntry(e));
			}
			u.EndArray();

			tld.loadedData->imageSets.Insert(imgSet.key, loaded);
		}
		u.EndDict();
	}

	for (auto& painter : tld.painters)
	{
		tld._LoadPainterByName(painter.key);
	}
}

ThemeDataHandle LoadTheme(StringView folder)
{
	auto theme = AsRCHandle(new ThemeData);
	theme->LoadTheme(folder);
	return theme;
}


void RegisterPainter(const char* type, PainterCreateFunc* createFunc)
{
	g_painterCreateFuncs.Insert(type, createFunc);
}


static IPainter* LayerPainterCreateFunc(IThemeLoader* loader, IObjectIterator& OI)
{
	auto* p = new LayerPainter;

	OI.BeginArray(0, "painters");
	while (OI.HasMoreArrayElements())
	{
		if (auto cp = loader->LoadPainter({}))
			p->layers.Append(cp);
	}
	OI.EndArray();

	return p;
}

struct ConditionLoader : IBufferRW
{
	mutable uint8_t cond = 0;

	void Assign(StringView sv) const override
	{
		if (sv == "disabled")
			cond |= PS_Disabled;
		else if (sv == "down")
			cond |= PS_Down;
		else if (sv == "focused")
			cond |= PS_Focused;
		else if (sv == "checked")
			cond |= PS_Checked;
		else if (sv == "hover")
			cond |= PS_Hover;
	}
	StringView Read() const override
	{
		// TODO?
		return "";
	}
};

static uint8_t ParseCondition(IObjectIterator& OI, const FieldInfo& FI)
{
	OI.BeginArray(0, FI);

	ConditionLoader cl;
	while (OI.HasMoreArrayElements())
	{
		OI.OnFieldString({}, cl);
	}

	OI.EndArray();

	return cl.cond;
}

static IPainter* ConditionalPainterCreateFunc(IThemeLoader* loader, IObjectIterator& OI)
{
	auto* p = new ConditionalPainter;

	p->condition = ParseCondition(OI, "condition");

	p->painter = loader->LoadPainter("painter");

	return p;
}

static IPainter* SelectFirstPainterCreateFunc(IThemeLoader* loader, IObjectIterator& OI)
{
	auto* p = new SelectFirstPainter;

	OI.BeginArray(0, "painters");
	while (OI.HasMoreArrayElements())
	{
		SelectFirstPainter::Item item;
		OI.BeginObject({}, "Item");
		{
			item.condition = ParseCondition(OI, "condition");
			OnField(OI, "checkState", item.checkState);

			item.painter = loader->LoadPainter("painter");
		}
		OI.EndObject();
		p->items.Append(item);
	}
	OI.EndArray();

	return p;
}

static IPainter* TextStyleModPainterCreateFunc(IThemeLoader* loader, IObjectIterator& OI)
{
	auto* p = new TextStyleModPainter;

	p->painter = loader->LoadPainter("painter");
	if (OI.HasField("textColor"))
		p->textColor = loader->LoadColor("textColor");
	OnField(OI, "contentOffsetInherit", p->contentOffsetInherit);
	OnField(OI, "contentOffset", p->contentOffset);

	return p;
}

static IPainter* PointAnchoredPlacementRectModPainterCreateFunc(IThemeLoader* loader, IObjectIterator& OI)
{
	auto* p = new PointAnchoredPlacementRectModPainter;

	p->painter = loader->LoadPainter("painter");
	OnField(OI, "anchor", p->anchor);
	OnField(OI, "pivot", p->pivot);
	OnField(OI, "bias", p->bias);
	OnField(OI, "sizeAddFraction", p->sizeAddFraction);
	OnField(OI, "size", p->size);

	return p;
}

static IPainter* ColorFillPainterCreateFunc(IThemeLoader* loader, IObjectIterator& OI)
{
	auto* p = new ColorFillPainter;

	p->color = loader->LoadColor("color");
	p->borderColor = loader->LoadColor("borderColor");

	OnField(OI, "shrink", p->shrink);
	OnField(OI, "borderWidth", p->borderWidth);
	OnField(OI, "contentOffset", p->contentOffset);

	return p;
}

static IPainter* ImageSetPainterCreateFunc(IThemeLoader* loader, IObjectIterator& OI)
{
	auto* p = new ImageSetPainter;

	std::string imgset;
	OnField(OI, "imgset", imgset);
	p->imageSet = loader->FindImageSet(imgset);
	p->color = loader->LoadColor("color");

	if (OI.HasField("textColor"))
	{
		p->overrideTextColor = true;
		p->textColor = loader->LoadColor("textColor");
	}

	OnField(OI, "shrink", p->shrink);
	OnField(OI, "contentOffset", p->contentOffset);

	return p;
}

static IPainter* BoxShadowPainterCreateFunc(IThemeLoader* loader, IObjectIterator& OI)
{
	auto* p = new BoxShadowPainter;

	if (OI.HasField("color"))
		p->color = loader->LoadColor("color");
	OnField(OI, "offset", p->offset);
	OnField(OI, "blurSize", p->blurSize);
	if (OI.HasField("corner"))
	{
		u32 val;
		OnField(OI, "corner", val);
		p->cornerLT = val;
		p->cornerRT = val;
		p->cornerLB = val;
		p->cornerRB = val;
	}
	if (OI.HasField("cornerLT"))
		OnField(OI, "cornerLT", p->cornerLT);
	if (OI.HasField("cornerRT"))
		OnField(OI, "cornerRT", p->cornerRT);
	if (OI.HasField("cornerLB"))
		OnField(OI, "cornerLB", p->cornerLB);
	if (OI.HasField("cornerRB"))
		OnField(OI, "cornerRB", p->cornerRB);

	return p;
}

void RegisterPainters()
{
	RegisterPainter("layer", LayerPainterCreateFunc);
	RegisterPainter("conditional", ConditionalPainterCreateFunc);
	RegisterPainter("select_first", SelectFirstPainterCreateFunc);
	RegisterPainter("text_style_mod", TextStyleModPainterCreateFunc);
	RegisterPainter("point_anchored_placement_rect_mod", PointAnchoredPlacementRectModPainterCreateFunc);
	RegisterPainter("color_fill", ColorFillPainterCreateFunc);
	RegisterPainter("imgset", ImageSetPainterCreateFunc);
	RegisterPainter("box_shadow", BoxShadowPainterCreateFunc);
}

} // ui
