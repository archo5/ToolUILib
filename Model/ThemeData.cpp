
#include "ThemeData.h"

#include "../Core/Serialization.h"
#include "../Core/FileSystem.h"

#include "../ThirdParty/json.h"


namespace ui {

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

	draw::ImageSetHandle FindImageSet(const std::string& name)
	{
		return loadedData->imageSets.get(name, nullptr);
	}
};


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
	OnField(oi, FI, str);
	if (oi.IsUnserializer())
		val = EnumKeys<E>::StringToValue(str.c_str());
}


static const char* EnumKeys_Presence[] =
{
	"",
	"",
	"None",
	"LayoutOnly",
	"Visible",
	nullptr,
};
template <> struct EnumKeys<Presence> : EnumKeysStringList<Presence, EnumKeys_Presence> {};

static const char* EnumKeys_StackingDirection[] =
{
	"",
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
	"",
	"",
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
	"",
	"ContentBox",
	"BorderBox",
	nullptr,
};
template <> struct EnumKeys<BoxSizing> : EnumKeysStringList<BoxSizing, EnumKeys_BoxSizing> {};

static const char* EnumKeys_HAlign[] =
{
	"",
	"",
	"left",
	"center",
	"right",
	"justify",
	nullptr,
};
template <> struct EnumKeys<HAlign> : EnumKeysStringList<HAlign, EnumKeys_HAlign> {};

static const char* EnumKeys_FontStyle[] =
{
	"",
	"",
	"normal",
	"italic",
	nullptr,
};
template <> struct EnumKeys<FontStyle> : EnumKeysStringList<FontStyle, EnumKeys_FontStyle> {};


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

			tf->text = ReadTextFile(to_string(folder, "/", entry));

			if (tf->unserializer.Parse(tf->text))
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
		auto& u = style.value.file->unserializer;
		u.BeginDict(style.value.key);
		{
			StyleBlockRef loaded = new StyleBlock;

			std::string painterName;
			OnField(u, "backgroundPainter", painterName);
			auto pit = tld.loadedData->painters.find(painterName);
			if (pit != tld.loadedData->painters.end())
				loaded->background_painter = pit->value;

			OnFieldEnumString(u, "presence", loaded->presence);
			OnFieldEnumString(u, "stackingDirection", loaded->stacking_direction);
			OnFieldEnumString(u, "edge", loaded->edge);
			OnFieldEnumString(u, "boxSizing", loaded->box_sizing);
			OnFieldEnumString(u, "horAlign", loaded->h_align);

			OnFieldEnumInt(u, "fontWeight", loaded->font_weight);
			OnFieldEnumString(u, "fontStyle", loaded->font_style);
			OnField(u, "fontSize", loaded->font_size);

			OnField(u, "width", loaded->width);
			OnField(u, "height", loaded->height);
			OnField(u, "minWidth", loaded->min_width);
			OnField(u, "minHeight", loaded->min_height);
			OnField(u, "maxWidth", loaded->max_width);
			OnField(u, "maxHeight", loaded->max_height);

			OnField(u, "left", loaded->left);
			OnField(u, "top", loaded->top);
			OnField(u, "right", loaded->right);
			OnField(u, "bottom", loaded->bottom);

			if (u.IsUnserializer())
			{
				Coord c;
				OnField(u, "margin", c);
				loaded->margin_left = c;
				loaded->margin_top = c;
				loaded->margin_right = c;
				loaded->margin_bottom = c;
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
				Coord c;
				OnField(u, "padding", c);
				loaded->padding_left = c;
				loaded->padding_top = c;
				loaded->padding_right = c;
				loaded->padding_bottom = c;
			}
			if (u.HasField("paddingLeft"))
				OnField(u, "paddingLeft", loaded->padding_left);
			if (u.HasField("paddingTop"))
				OnField(u, "paddingTop", loaded->padding_top);
			if (u.HasField("paddingRight"))
				OnField(u, "paddingRight", loaded->padding_right);
			if (u.HasField("paddingBottom"))
				OnField(u, "paddingBottom", loaded->padding_bottom);

			tld.loadedData->styles.insert(style.key, loaded);
		}
		u.EndDict();
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

static IPainter* ImageSetPainterCreateFunc(IThemeLoader* loader, IObjectIterator& OI)
{
	auto* p = new ImageSetPainter;

	std::string imgset;
	OnField(OI, "imgset", imgset);
	p->imageSet = loader->FindImageSet(imgset);

	OnField(OI, "shrink", p->shrink);

	return p;
}

void RegisterPainters()
{
	RegisterPainter("layer", LayerPainterCreateFunc);
	RegisterPainter("conditional", ConditionalPainterCreateFunc);
	RegisterPainter("select_first", SelectFirstPainterCreateFunc);
	RegisterPainter("imgset", ImageSetPainterCreateFunc);
}

} // ui
