
#pragma once

#include "../Core/Math.h"
#include "../Core/String.h"
#include "../Core/Image.h"
#include "../Core/RefCounted.h"
#include "../Core/Font.h"

#include "../Render/Render.h"

#include <vector>
#include <functional>


namespace ui {

struct UIObject;

using UIRect = AABB2f;

enum class Presence : uint8_t
{
	Visible, // most common = 0
	LayoutOnly,
	None,
};

enum class EstSizeType
{
	Exact,
	Expanding,
};

struct ILayout
{
	virtual float CalcEstimatedWidth(UIObject* curObj, const Size2f& containerSize, EstSizeType type) = 0;
	virtual float CalcEstimatedHeight(UIObject* curObj, const Size2f& containerSize, EstSizeType type) = 0;
	virtual void OnLayout(UIObject* curObj, UIRect& inrect) = 0;
};

struct IPlacement
{
	virtual void OnApplyPlacement(UIObject* curObj, UIRect& outRect) const = 0;
	bool applyOnLayout = false;
	bool fullScreenRelative = false;
};

namespace layouts {

ILayout* Stack();

}

enum class StackingDirection : uint8_t
{
	Undefined,

	LeftToRight,
	TopDown,
};

enum class HAlign : uint8_t
{
	Undefined,

	Left,
	Center,
	Right,
	Justify,
};

enum class FontWeight : uint16_t
{
	Thin = 100,
	ExtraLight = 200,
	Light = 300,
	Normal = 400,
	Medium = 500,
	Semibold = 600,
	Bold = 700,
	ExtraBold = 800,
	Black = 900,
};

template <> struct EnumKeys<FontWeight>
{
	static FontWeight StringToValue(const char* name);
	static const char* ValueToString(FontWeight e);
};

enum class FontStyle : uint8_t
{
	Normal,
	Italic,
};

extern const char* EnumKeys_FontStyle[];
template <> struct EnumKeys<FontStyle> : EnumKeysStringList<FontStyle, EnumKeys_FontStyle> {};

enum class CoordTypeUnit
{
	Undefined,
	Inherit,
	Auto,

	Pixels,
	Percent,
	Fraction,
};

struct Coord
{
	float value;
	CoordTypeUnit unit;

	Coord() : value(0), unit(CoordTypeUnit::Undefined) {}
	Coord(float px) : value(px), unit(CoordTypeUnit::Pixels) {}
	Coord(float v, CoordTypeUnit u) : value(v), unit(u) {}
	bool IsDefined(bool fraction = false) const { return unit != CoordTypeUnit::Undefined && (fraction ? true : unit != CoordTypeUnit::Fraction); }
	bool operator == (const Coord& o) const
	{
		return unit == o.unit && (unit <= CoordTypeUnit::Auto || value == o.value);
	}
	static const Coord Undefined() { return {}; }
	static Coord Percent(float p) { return Coord(p, CoordTypeUnit::Percent); }
	static Coord Fraction(float f) { return Coord(f, CoordTypeUnit::Fraction); }

	void OnSerialize(IObjectIterator& oi, const FieldInfo& FI);
};


enum PaintInfoItemState
{
	PS_Hover = 1 << 0,
	PS_Down = 1 << 1,
	PS_Disabled = 1 << 2,
	PS_Focused = 1 << 3,
	PS_Checked = 1 << 4,
};

struct PaintInfo
{
	UIRect rect;
	const UIObject* obj;
	uint8_t state = 0;
	uint8_t checkState = 0;

	PaintInfo() {}
	PaintInfo(const UIObject* o);
	void SetChecked(bool checked);
	void SetCheckState(uint8_t cs);
	bool IsHovered() const { return (state & PS_Hover) != 0; }
	bool IsDown() const { return (state & PS_Down) != 0; }
	bool IsDisabled() const { return (state & PS_Disabled) != 0; }
	bool IsChecked() const { return (state & PS_Checked) != 0; }
	bool IsFocused() const { return (state & PS_Focused) != 0; }
};

struct ContentPaintAdvice
{
	bool _hasTextColor = false;
	Color4b _textColor;
	Vec2f offset;

	bool HasTextColor() const { return _hasTextColor; }
	Color4b GetTextColor() const { return _textColor; }
	void SetTextColor(Color4b col)
	{
		_hasTextColor = true;
		_textColor = col;
	}

	void MergeOver(const ContentPaintAdvice& o)
	{
		if (o._hasTextColor)
			SetTextColor(o._textColor);
		offset += o.offset;
	}
};

struct IPainter : RefCountedST
{
	virtual ContentPaintAdvice Paint(const PaintInfo&) = 0;
};
using PainterHandle = RCHandle<IPainter>;

struct EmptyPainter : IPainter
{
	ContentPaintAdvice Paint(const PaintInfo&) override;
	static EmptyPainter* Get();
private:
	EmptyPainter();
};

struct CheckerboardPainter : IPainter
{
	ContentPaintAdvice Paint(const PaintInfo&) override;
	static CheckerboardPainter* Get();
private:
	CheckerboardPainter();
};

struct LayerPainter : IPainter
{
	std::vector<PainterHandle> layers;

	ContentPaintAdvice Paint(const PaintInfo&) override;
	static RCHandle<LayerPainter> Create();
};

struct ConditionalPainter : IPainter
{
	PainterHandle painter;
	uint8_t condition = 0;

	ContentPaintAdvice Paint(const PaintInfo&) override;
};

struct SelectFirstPainter : IPainter
{
	struct Item
	{
		PainterHandle painter;
		uint8_t condition = 0;
		uint8_t checkState = 0xff;
	};

	std::vector<Item> items;

	ContentPaintAdvice Paint(const PaintInfo&) override;
};

struct PointAnchoredPlacementRectModPainter : IPainter
{
	PainterHandle painter;
	Vec2f anchor = { 0.5f, 0.5f };
	Vec2f pivot = { 0.5f, 0.5f };
	Vec2f bias;
	Vec2f sizeAddFraction;
	Vec2f size;

	ContentPaintAdvice Paint(const PaintInfo&) override;
};

struct ColorFillPainter : IPainter
{
	Color4b color;
	Color4b borderColor = Color4b::Zero();
	int shrink = 0;
	float borderWidth = 0;
	Vec2f contentOffset;

	ContentPaintAdvice Paint(const PaintInfo&) override;
};

struct ImageSetPainter : IPainter
{
	draw::ImageSetHandle imageSet;
	Color4b color;
	int shrink = 0;
	Vec2f contentOffset;

	ContentPaintAdvice Paint(const PaintInfo&) override;
};

template <class F>
struct FunctionPainterT : IPainter
{
	F func;

	FunctionPainterT(F&& f) : func(std::move(f)) {}
	ContentPaintAdvice Paint(const PaintInfo& info) override { return func(info); }
};

template <class F> inline PainterHandle CreateFunctionPainter(F&& f) { return new FunctionPainterT<F>(std::move(f)); }


struct ThemeData; // Theme.h

struct FontSettings
{
	static constexpr const char* NAME = "FontSettings";

	CachedFontRef _cachedFont;

	std::string family = FONT_FAMILY_SANS_SERIF;
	FontWeight weight = FontWeight::Normal;
	FontStyle style = FontStyle::Normal;
	int size = 12;

	void _SerializeContents(IObjectIterator& oi);
	void Serialize(ThemeData& td, IObjectIterator& oi);
	void OnSerialize(IObjectIterator& oi, const FieldInfo& FI);

	Font* GetFont() const;
};


} // ui
