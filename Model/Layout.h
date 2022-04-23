
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

extern int g_numStyleBlocks;

enum class Presence : uint8_t
{
	Visible, // most common = 0
	LayoutOnly,
	None,
};
/*
enum class Layout : uint8_t
{
	Undefined,
	Inherit,

	//None, // skipped when doing layout and painting
	//Inline, // text-line formatting, can be split
	InlineBlock, // text-line formatting, cannot be split
	Stack, // parent-controlled single line stacking
	StackExpand, // same as above but try to fill the space
	EdgeSlice, // child-controlled multidirectional stacking at the edges of remaining space
};
*/
struct LayoutState
{
	UIRect finalContentRect;
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
	virtual void OnLayout(UIObject* curObj, const UIRect& inrect, LayoutState& state) = 0;
};

struct IPlacement
{
	virtual void OnApplyPlacement(UIObject* curObj, UIRect& outRect) const = 0;
	bool applyOnLayout = false;
	bool fullScreenRelative = false;
};

struct PointAnchoredPlacement : IPlacement
{
	void OnApplyPlacement(UIObject* curObj, UIRect& outRect) const override;

	void SetAnchorAndPivot(Point2f p)
	{
		anchor = p;
		pivot = p;
	}

	Point2f anchor = { 0, 0 };
	Point2f pivot = { 0, 0 };
	Point2f bias = { 0, 0 };
	bool useContentBox = false;
};

struct RectAnchoredPlacement : IPlacement
{
	void OnApplyPlacement(UIObject* curObj, UIRect& outRect) const override;

	UIRect anchor = { 0, 0, 1, 1 };
	UIRect bias = { 0, 0, 0, 0 };
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

enum class Edge : uint8_t
{
	Left,
	Top,
	Right,
	Bottom,
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

enum class FontStyle : uint8_t
{
	Normal,
	Italic,
};

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


struct PropChange
{
	int which;

	virtual bool Equal(PropChange* other) = 0;
};

template <class T>
struct PropChangeImpl : PropChange
{
	T value;

	bool Equal(PropChange* other) override
	{
		if (which != other->which)
			return false;
		return value == static_cast<PropChangeImpl*>(other)->value;
	}
};

typedef bool FnIsPropEqual(const void* a, const void* b);
typedef void FnPropCopy(void* a, const void* b);

template <class T>
struct PropFuncs
{
	static bool IsEqual(const void* a, const void* b)
	{
		return *static_cast<const T*>(a) == *static_cast<const T*>(b);
	}
	static void Copy(void* a, const void* b)
	{
		*static_cast<T*>(a) = *static_cast<const T*>(b);
	}
};

struct StyleBlock
{
	~StyleBlock();
	void _Release();

	StyleBlock* _GetWithChange(int off, FnIsPropEqual feq, FnPropCopy fcopy, const void* ref);
	template <class T> StyleBlock* GetWithChange(int off, const T& val)
	{
		return _GetWithChange(off, PropFuncs<T>::IsEqual, PropFuncs<T>::Copy, &val);
	}

	uint32_t _refCount = 0;
	int _numChildren = 0;
	int _parentDiffOffset = 0;
	FnIsPropEqual* _parentDiffFunc = nullptr;
	StyleBlock* _parent = nullptr;
	StyleBlock* _prev = nullptr;
	StyleBlock* _next = nullptr;
	StyleBlock* _firstChild = nullptr;
	StyleBlock* _lastChild = nullptr;

	CachedFontRef _cachedFont;

	PainterHandle background_painter;

	Presence presence = Presence::Visible;
	StackingDirection stacking_direction = StackingDirection::Undefined;
	HAlign h_align = HAlign::Undefined;

	std::string font_family = FONT_FAMILY_SANS_SERIF;
	FontWeight font_weight = FontWeight::Normal;
	FontStyle font_style = FontStyle::Normal;
	int font_size = 12;
	Color4b text_color;

	Coord width;
	Coord height;

	float padding_left = 0;
	float padding_right = 0;
	float padding_top = 0;
	float padding_bottom = 0;
	UI_FORCEINLINE UIRect GetPaddingRect() const { return { padding_left, padding_top, padding_right, padding_bottom }; }

	InstanceCounter<g_numStyleBlocks> _ic;

	Font* GetFont();
};
static_assert(sizeof(StyleBlock) < 300 + (sizeof(void*) - 4) * 20, "style block getting too big?");

class StyleBlockRef
{
public:
	StyleBlockRef() : block(nullptr) {}
	StyleBlockRef(StyleBlock* b) : block(b) { if (b) b->_refCount++;  }
	StyleBlockRef(const StyleBlockRef& b) : block(b.block) { if (block) block->_refCount++; }
	StyleBlockRef(StyleBlockRef&& b) : block(b.block) { b.block = nullptr; }
	~StyleBlockRef() { if (block) block->_Release(); }
	StyleBlockRef& operator = (StyleBlock* b)
	{
		if (b)
			b->_refCount++;
		if (block)
			block->_Release();
		block = b;
		return *this;
	}
	StyleBlockRef& operator = (const StyleBlockRef& b)
	{
		if (b.block)
			b.block->_refCount++;
		if (block)
			block->_Release();
		block = b.block;
		return *this;
	}
	StyleBlockRef& operator = (StyleBlockRef&& b)
	{
		if (block && block != b.block)
			block->_Release();
		block = b.block;
		b.block = nullptr;
		return *this;
	}
	operator StyleBlock* () const { return block; }
	StyleBlock& operator * () const { return *block; }
	StyleBlock* operator -> () const { return block; }

private:
	StyleBlock* block;
};

class StyleAccessor
{
public:

	explicit StyleAccessor(StyleBlock* b);
	explicit StyleAccessor(StyleBlockRef& r) = delete;
	explicit StyleAccessor(StyleBlockRef& r, UIObject* o);


	PainterHandle GetBackgroundPainter() const;
	void SetBackgroundPainter(const PainterHandle& h);


	StackingDirection GetStackingDirection() const;
	void SetStackingDirection(StackingDirection v);

	HAlign GetHAlign() const;
	void SetHAlign(HAlign a);


	const std::string& GetFontFamily() const;
	void SetFontFamily(const std::string& v);

	FontWeight GetFontWeight() const;
	void SetFontWeight(FontWeight v);

	FontStyle GetFontStyle() const;
	void SetFontStyle(FontStyle v);

	int GetFontSize() const;
	void SetFontSize(int v);

	Color4b GetTextColor() const;
	void SetTextColor(Color4b v);


	Coord GetWidth() const;
	void SetWidth(Coord v);

	Coord GetHeight() const;
	void SetHeight(Coord v);


	float GetPaddingLeft() const;
	void SetPaddingLeft(float v);

	float GetPaddingRight() const;
	void SetPaddingRight(float v);

	float GetPaddingTop() const;
	void SetPaddingTop(float v);

	float GetPaddingBottom() const;
	void SetPaddingBottom(float v);

	void SetPadding(float v) { SetPadding(v, v, v, v); }
	void SetPadding(float tb, float lr) { SetPadding(tb, lr, tb, lr); }
	void SetPadding(float t, float lr, float b) { SetPadding(t, lr, b, lr); }
	void SetPadding(float t, float r, float b, float l);


	StyleBlock* block;
	StyleBlockRef* blkref;
	UIObject* owner;
};

StyleBlockRef GetObjectStyle();
StyleBlockRef GetTextStyle();

} // ui
