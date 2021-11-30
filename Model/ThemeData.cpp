
#include "ThemeData.h"

#include "../Core/Serialization.h"
#include "../Core/FileSystem.h"

#include "../ThirdParty/json.h"


namespace ui {

template <class E> struct EnumKeys
{
	static E StringToValue(const char* name) = delete;
	static const char* ValueToString(E e) = delete;
};

inline constexpr size_t CountStrings(const char** list)
{
	const char** it = list;
	while (*it)
		it++;
	return it - list;
}

template <class E, const char** list, E def = (E)0> struct EnumKeysStringList
{
	static E StringToValue(const char* name)
	{
		for (auto it = list; *it; it++)
			if (!strcmp(*it, name))
				return E(uintptr_t(it - list));
		return def;
	}
	static const char* ValueToString(E v)
	{
		static size_t MAX = CountStrings(list);
		if (size_t(v) >= MAX)
			return "";
		return list[size_t(v)];
	}
};

template <class E> inline void OnFieldEnumString(IObjectIterator& oi, const FieldInfo& FI, E& val)
{
	std::string str;
	if (!oi.IsUnserializer())
		str = EnumKeys<E>::ValueToString(val);
	if (oi.IsUnserializer() && !oi.HasField(FI.GetNameOrEmptyStr()))
		return;
	OnField(oi, FI, str);
	if (oi.IsUnserializer())
		val = EnumKeys<E>::StringToValue(str.c_str());
}


static const char* EnumKeys_Presence[] =
{
	"Visible",
	"LayoutOnly",
	"None",
	nullptr,
};
template <> struct EnumKeys<Presence> : EnumKeysStringList<Presence, EnumKeys_Presence> {};

static const char* EnumKeys_StackingDirection[] =
{
	"",
	"LeftToRight",
	"TopDown",
	"RightToLeft",
	"BottomUp",
	nullptr,
};
template <> struct EnumKeys<StackingDirection> : EnumKeysStringList<StackingDirection, EnumKeys_StackingDirection> {};

static const char* EnumKeys_Edge[] =
{
	"left",
	"top",
	"right",
	"bottom",
	nullptr,
};
template <> struct EnumKeys<Edge> : EnumKeysStringList<Edge, EnumKeys_Edge> {};

static const char* EnumKeys_BoxSizing[] =
{
	"",
	"ContentBox",
	"BorderBox",
	nullptr,
};
template <> struct EnumKeys<BoxSizing> : EnumKeysStringList<BoxSizing, EnumKeys_BoxSizing> {};

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

template <> struct EnumKeys<FontWeight>
{
	static FontWeight StringToValue(const char* name)
	{
		if (!strcmp(name, "thin")) return FontWeight::Thin;
		if (!strcmp(name, "extraLight")) return FontWeight::ExtraLight;
		if (!strcmp(name, "light")) return FontWeight::Light;
		if (!strcmp(name, "normal")) return FontWeight::Normal;
		if (!strcmp(name, "medium")) return FontWeight::Medium;
		if (!strcmp(name, "semibold")) return FontWeight::Semibold;
		if (!strcmp(name, "bold")) return FontWeight::Bold;
		if (!strcmp(name, "extraBold")) return FontWeight::ExtraBold;
		if (!strcmp(name, "black")) return FontWeight::Black;
		return FontWeight::Normal;
	}
	static const char* ValueToString(FontWeight e)
	{
		switch (e)
		{
		case FontWeight::Thin: return "thin";
		case FontWeight::ExtraLight: return "extraLight";
		case FontWeight::Light: return "light";
		case FontWeight::Normal: return "normal";
		case FontWeight::Medium: return "medium";
		case FontWeight::Semibold: return "semibold";
		case FontWeight::Bold: return "bold";
		case FontWeight::ExtraBold: return "extraBold";
		case FontWeight::Black: return "black";
		default: return "";
		}
	}
};

static const char* EnumKeys_FontStyle[] =
{
	"normal",
	"italic",
	nullptr,
};
template <> struct EnumKeys<FontStyle> : EnumKeysStringList<FontStyle, EnumKeys_FontStyle> {};


draw::ImageSetHandle ThemeData::FindImageSetByName(const std::string& name)
{
	auto it = imageSets.find(name);
	if (it.is_valid())
		return it->value;
	return {};
}

PainterHandle ThemeData::FindPainterByName(const std::string& name)
{
	auto it = painters.find(name);
	if (it.is_valid())
		return it->value;
	return {};
}

StyleBlockRef ThemeData::FindStyleByName(const std::string& name)
{
	auto it = styles.find(name);
	if (it.is_valid())
		return it->value;
	return {};
}

static size_t CalcNewSize(size_t curSize, size_t curMaxCount)
{
	if (curMaxCount <= curSize)
		return curSize;
	return curSize + curMaxCount;
}

template <class TC, class TID> TC& GetCacheSlot(std::vector<TC>& cache, const StaticID<TID>& id)
{
	size_t newSize = CalcNewSize(cache.size(), StaticID<TID>::GetCount());
	if (cache.size() < newSize)
		cache.resize(newSize);
	return cache[id._id];
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

StyleBlockRef ThemeData::GetStyle(const StaticID_Style& id, bool returnDefaultIfMissing)
{
	auto& slot = GetCacheSlot(_cachedStyles, id);
	if (!slot)
		slot = FindStyleByName(id._name);
	return slot ? slot : returnDefaultIfMissing ? GetObjectStyle() : nullptr;
}


static HashMap<std::string, PainterCreateFunc*> g_painterCreateFuncs;

struct ThemeFile : RefCountedST
{
	std::string text;
	JSONUnserializerObjectIterator unserializer;
};
using ThemeFileHandle = RCHandle<ThemeFile>;

struct ThemeElementRef
{
	ThemeFileHandle file;
	const char* key;
};

struct ThemeLoaderData : IThemeLoader
{
	std::vector<ThemeFileHandle> files;
	HashMap<std::string, ThemeElementRef> imageSets;
	HashMap<std::string, ThemeElementRef> painters;
	HashMap<std::string, ThemeElementRef> styles;
	ThemeDataHandle loadedData;

	ThemeFileHandle curFile;

	PainterHandle LoadPainter() override
	{
		if (!curFile)
			return nullptr;

		auto type = curFile->unserializer.ReadString("__");
		if (!type.HasValue())
			return nullptr;

		auto it = g_painterCreateFuncs.find(type.GetValue());
		if (!it.is_valid())
			return nullptr;

		return it->value(this, curFile->unserializer);
	}

	StyleBlockRef GetStyleBlock(const std::string& name)
	{
		auto it = loadedData->styles.find(name);
		if (it.is_valid())
			return it->value;

		auto it2 = styles.find(name);
		if (!it2.is_valid())
		{
			printf("Failed to find style: \"%s\"\n", name.c_str());
			return nullptr;
		}

		return _LoadStyleBlock(name, it2->value);
	}

	StyleBlockRef _LoadStyleBlock(const std::string& styleKey, const ThemeElementRef& styleElemRef)
	{
		StyleBlockRef loaded;

		auto& u = styleElemRef.file->unserializer;
		u.BeginDict(styleElemRef.key);
		{
			std::string basedOn;
			OnField(u, "basedOn", basedOn);
			if (!basedOn.empty())
				loaded = GetStyleBlock(basedOn);
			if (!loaded)
				loaded = new StyleBlock;

			std::string layout;
			OnField(u, "layout", layout);
			if (layout == "stack")
				loaded->layout = layouts::Stack();
			else if (layout == "stack_expand")
				loaded->layout = layouts::StackExpand();
			else if (layout == "inline_block")
				loaded->layout = layouts::InlineBlock();
			else if (layout == "edge_slice")
				loaded->layout = layouts::EdgeSlice();

			std::string painterName;
			OnField(u, "backgroundPainter", painterName);
			auto pit = loadedData->painters.find(painterName);
			if (pit != loadedData->painters.end())
				loaded->background_painter = pit->value;
			else
				loaded->background_painter = EmptyPainter::Get();

			OnFieldEnumString(u, "presence", loaded->presence);
			OnFieldEnumString(u, "stackingDirection", loaded->stacking_direction);
			OnFieldEnumString(u, "edge", loaded->edge);
			OnFieldEnumString(u, "boxSizing", loaded->box_sizing);
			OnFieldEnumString(u, "horAlign", loaded->h_align);

			OnFieldEnumString(u, "fontWeight", loaded->font_weight);
			//OnFieldEnumInt(u, "fontWeight", loaded->font_weight); -- may want to support both?
			OnFieldEnumString(u, "fontStyle", loaded->font_style);
			OnField(u, "fontSize", loaded->font_size);

			OnField(u, "width", loaded->width);
			OnField(u, "height", loaded->height);
			OnField(u, "minWidth", loaded->min_width);
			OnField(u, "minHeight", loaded->min_height);
			OnField(u, "maxWidth", loaded->max_width);
			OnField(u, "maxHeight", loaded->max_height);

			if (u.IsUnserializer())
			{
				float f = 0;
				OnField(u, "margin", f);
				loaded->margin_left = f;
				loaded->margin_top = f;
				loaded->margin_right = f;
				loaded->margin_bottom = f;
			}
			if (u.HasField("marginLeft"))
				OnField(u, "marginLeft", loaded->margin_left);
			if (u.HasField("marginTop"))
				OnField(u, "marginTop", loaded->margin_top);
			if (u.HasField("marginRight"))
				OnField(u, "marginRight", loaded->margin_right);
			if (u.HasField("marginBottom"))
				OnField(u, "marginBottom", loaded->margin_bottom);

			if (u.IsUnserializer())
			{
				float f = 0;
				OnField(u, "padding", f);
				loaded->padding_left = f;
				loaded->padding_top = f;
				loaded->padding_right = f;
				loaded->padding_bottom = f;
			}
			if (u.HasField("paddingH"))
			{
				float p = 0;
				OnField(u, "paddingH", p);
				loaded->padding_left = loaded->padding_right = p;
			}
			if (u.HasField("paddingV"))
			{
				float p = 0;
				OnField(u, "paddingV", p);
				loaded->padding_top = loaded->padding_bottom = p;
			}
			if (u.HasField("paddingLeft"))
				OnField(u, "paddingLeft", loaded->padding_left);
			if (u.HasField("paddingTop"))
				OnField(u, "paddingTop", loaded->padding_top);
			if (u.HasField("paddingRight"))
				OnField(u, "paddingRight", loaded->padding_right);
			if (u.HasField("paddingBottom"))
				OnField(u, "paddingBottom", loaded->padding_bottom);

			loadedData->styles.insert(styleKey, loaded);
		}
		u.EndDict();

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
		return loadedData->imageSets.get(name, nullptr);
	}
};


void RegisterPainter(const char* type, PainterCreateFunc* createFunc)
{
	g_painterCreateFuncs.insert(type, createFunc);
}


ThemeDataHandle LoadTheme(StringView folder)
{
	ThemeLoaderData tld;

	DirectoryIterator di(folder);
	std::string entry;
	while (di.GetNext(entry))
	{
		if (StringView(entry).ends_with(".json"))
		{
			auto tf = AsRCHandle(new ThemeFile);

			auto path = to_string(folder, "/", entry);
			tf->text = ReadTextFile(path);

			if (tf->unserializer.Parse(tf->text, JPF_AllowAll))
			{
				tld.files.push_back(tf);

				// enumerate the contents
				if (auto* root = tf->unserializer._root)
				{
					if (root->type == json_type_object)
					{
						auto* obj = static_cast<json_object_s*>(root->payload);
						for (auto* el = obj->start; el; el = el->next)
						{
							StringView name(el->name->string, el->name->string_size);
							if (name.ends_with(":imgset"))
							{
								auto key = to_string(name.substr(0, name.find_last_at(":imgset")));
								tld.imageSets.insert(key, { tf, el->name->string });
							}
							if (name.ends_with(":painter"))
							{
								auto key = to_string(name.substr(0, name.find_last_at(":painter")));
								tld.painters.insert(key, { tf, el->name->string });
							}
							if (name.ends_with(":style"))
							{
								auto key = to_string(name.substr(0, name.find_last_at(":style")));
								tld.styles.insert(key, { tf, el->name->string });
							}
						}
					}
				}
			}
			else
			{
				printf("FAILED to parse %s\n", path.c_str());
			}
		}
	}

	tld.loadedData = new ThemeData;

	for (auto& imgSet : tld.imageSets)
	{
		auto& u = imgSet.value.file->unserializer;
		u.BeginDict(imgSet.value.key);
		{
			auto loaded = AsRCHandle(new draw::ImageSet);

			size_t count = u.BeginArray(0, "images");
			for (size_t i = 0; i < count; i++)
			{
				draw::ImageSet::Entry e;

				u.BeginObject({}, "ImageSetEntry");
				{
					if (auto path = u.ReadString("path"))
					{
						auto fullpath = to_string(folder, "/", path.GetValue());
						e.image = draw::ImageLoadFromFile(fullpath);
					}
					if (auto edge = u.ReadInt("edge"))
						e.edgeWidth = AABB2f::UniformBorder(float(edge.GetValue()));

					if (e.image)
					{
						if (e.edgeWidth.x0 == 0 &&
							e.edgeWidth.y0 == 0 &&
							e.edgeWidth.x1 == 0 &&
							e.edgeWidth.y1 == 0)
						{
							e.sliced = false;
							e.innerUV = { 0, 0, 1, 1 };
						}
						else
						{
							e.sliced = true;
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

				loaded->entries.push_back(e);
			}
			u.EndArray();

			tld.loadedData->imageSets.insert(imgSet.key, loaded);
		}
		u.EndDict();
	}

	for (auto& painter : tld.painters)
	{
		auto& u = painter.value.file->unserializer;
		tld.curFile = painter.value.file;
		u.BeginDict(painter.value.key);
		{
			auto loaded = tld.LoadPainter();
			tld.loadedData->painters.insert(painter.key, loaded);
		}
		u.EndDict();
		tld.curFile = nullptr;
	}

	for (auto& style : tld.styles)
	{
		tld.GetStyleBlock(style.key);
	}

	return tld.loadedData;
}


static IPainter* LayerPainterCreateFunc(IThemeLoader* loader, IObjectIterator& OI)
{
	auto* p = new LayerPainter;

	OI.BeginArray(0, "painters");
	while (OI.HasMoreArrayElements())
	{
		OI.BeginObject({}, "Layer");
		{
			if (auto cp = loader->LoadPainter())
				p->layers.push_back(cp);
		}
		OI.EndObject();
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

	OI.BeginObject("painter", "Painter");
	p->painter = loader->LoadPainter();
	OI.EndObject();

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

			OI.BeginObject("painter", "Painter");
			item.painter = loader->LoadPainter();
			OI.EndObject();
		}
		OI.EndObject();
		p->items.push_back(item);
	}
	OI.EndArray();

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

	OnField(OI, "shrink", p->shrink);
	OnField(OI, "contentOffset", p->contentOffset);

	return p;
}

void RegisterPainters()
{
	RegisterPainter("layer", LayerPainterCreateFunc);
	RegisterPainter("conditional", ConditionalPainterCreateFunc);
	RegisterPainter("select_first", SelectFirstPainterCreateFunc);
	RegisterPainter("color_fill", ColorFillPainterCreateFunc);
	RegisterPainter("imgset", ImageSetPainterCreateFunc);
}

} // ui
