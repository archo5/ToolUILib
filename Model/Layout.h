
#pragma once
#include <vector>
#include <functional>
#include "../Core/Math.h"
#include "../Core/String.h"


struct UIObject;

using UIRect = AABB<float>;

template <int& at>
struct InstanceCounter
{
	InstanceCounter() { at++; }
	InstanceCounter(const InstanceCounter&) { at++; }
	~InstanceCounter() { at--; }
};


namespace style {

extern int g_numBlocks;

enum EnumKindID
{
	EK_Display,
	EK_Position,
	EK_FlexDirection,
	EK_FlexWrap,
	EK_JustifyContent,
	EK_AlignItems,
	EK_AlignSelf,
	EK_AlignContent,
};

enum class Presence : uint8_t
{
	Undefined,
	Inherit,

	None,
	LayoutOnly,
	Visible,
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
	Point<float> scaleOrigin;
};

enum class EstSizeType
{
	Exact,
	Expanding,
};

struct Layout
{
	virtual float CalcEstimatedWidth(UIObject* curObj, const Size<float>& containerSize, EstSizeType type) = 0;
	virtual float CalcEstimatedHeight(UIObject* curObj, const Size<float>& containerSize, EstSizeType type) = 0;
	virtual void OnLayout(UIObject* curObj, const UIRect& inrect, LayoutState& state) = 0;
};

struct Placement
{
	virtual void OnApplyPlacement(UIObject* curObj, UIRect& outRect) = 0;
	bool applyOnLayout = false;
};

struct PointAnchoredPlacement : Placement
{
	void OnApplyPlacement(UIObject* curObj, UIRect& outRect) override;

	void SetAnchorAndPivot(Point<float> p)
	{
		anchor = p;
		pivot = p;
	}

	Point<float> anchor = { 0, 0 };
	Point<float> pivot = { 0, 0 };
	Point<float> bias = { 0, 0 };
	bool useContentBox = false;
};

struct RectAnchoredPlacement : Placement
{
	void OnApplyPlacement(UIObject* curObj, UIRect& outRect) override;

	UIRect anchor = { 0, 0, 1, 1 };
	UIRect bias = { 0, 0, 0, 0 };
};

namespace layouts {

Layout* InlineBlock();
Layout* Stack();
Layout* StackExpand();
Layout* EdgeSlice();

}

enum class StackingDirection : uint8_t
{
	Undefined,
	Inherit,

	TopDown,
	RightToLeft,
	BottomUp,
	LeftToRight,
};

enum class Edge : uint8_t
{
	Undefined,
	Inherit,

	Top,
	Right,
	Bottom,
	Left,
};

enum class BoxSizing : uint8_t
{
	Undefined,
	Inherit,

	ContentBox,
	BorderBox,
};

enum class HAlign : uint8_t
{
	Undefined,
	Inherit,

	Left,
	Center,
	Right,
	Justify,
};

enum class FontWeight : int16_t
{
	Undefined = 0,
	Inherit = -1,

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
	Undefined,
	Inherit,

	Normal,
	Italic,
};

enum class Display
{
	Undefined,
	Inherit,

	None,
	Block,
	Inline,
	Flex,
	InlineFlex,
	Grid,
	InlineGrid,
};
struct Display_
{
	enum { _ID = EK_Display };
	using _Value = Display;
};

enum class Position
{
	Undefined,
	Inherit,

	Static,
	Relative,
	Absolute,
	Fixed,
	Sticky,
};
struct Position_
{
	enum { _ID = EK_Position };
	enum _Value
	{
		Static,
		Relative,
		Absolute,
		Fixed,
	};
};

struct FlexDirection
{
	enum { _ID = EK_FlexDirection };
	enum _Value
	{
		Row,
		RowReverse,
		Column,
		ColumnReverse,
	};
};

struct FlexWrap
{
	enum { _ID = EK_FlexWrap };
	enum _Value
	{
		NoWrap,
		Wrap,
		WrapReverse,
	};
};

struct JustifyContent
{
	enum { _ID = EK_JustifyContent };
	enum _Value
	{
		FlexStart,
		FlexEnd,
		Center,
		SpaceBetween,
		SpaceAround,
		SpaceEvenly,
	};
};

struct AlignItems
{
	enum { _ID = EK_AlignItems };
	enum _Value
	{
		FlexStart,
		FlexEnd,
		Center,
		Baseline,
		Stretch,
	};
};

struct AlignSelf
{
	enum { _ID = EK_AlignSelf };
	enum _Value
	{
		FlexStart,
		FlexEnd,
		Center,
		Baseline,
		Stretch,
		Auto,
	};
};

struct AlignContent
{
	enum { _ID = EK_AlignContent };
	enum _Value
	{
		FlexStart,
		FlexEnd,
		Center,
		SpaceBetween,
		SpaceAround,
		SpaceEvenly,
		Stretch,
	};
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

struct Color
{
	ui::Color4b color;
	bool inherit = true;

	Color() {}
	Color(ui::Color4b col) : color(col), inherit(false) {}
	Color(ui::Color4f col) : color(col), inherit(false) {}
	bool operator == (const Color& o) const
	{
		return inherit == o.inherit && (inherit || color == o.color);
	}
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
};

enum PaintInfoItemState
{
	PS_Hover = 1 << 0,
	PS_Down = 1 << 1,
	PS_Disabled = 1 << 2,
	PS_Focused = 1 << 3,
};

struct PaintInfo
{
	UIRect rect;
	const UIObject* obj;
	uint8_t state = 0;
	uint8_t checkState = 0;

	PaintInfo() {}
	PaintInfo(const UIObject* o);
	bool IsHovered() const { return (state & PS_Hover) != 0; }
	bool IsDown() const { return (state & PS_Down) != 0; }
	bool IsDisabled() const { return (state & PS_Disabled) != 0; }
	bool IsChecked() const { return checkState != 0; }
	bool IsFocused() const { return (state & PS_Focused) != 0; }
};

using PaintFunction = std::function<void(const PaintInfo&)>;
inline bool operator == (const PaintFunction& a, const PaintFunction& b)
{
	// cannot compare two instances in practice
	// even if the underlying function pointer is the same, data may be different
	return false;
}

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

struct Block
{
	~Block();
	void MergeDirect(const Block& o);
	void MergeParent(const Block& o);
	void _Release();

	Block* _GetWithChange(int off, FnIsPropEqual feq, FnPropCopy fcopy, const void* ref);
	template <class T> Block* GetWithChange(int off, const T& val)
	{
		return _GetWithChange(off, PropFuncs<T>::IsEqual, PropFuncs<T>::Copy, &val);
	}

	uint32_t _refCount = 0;
	int _numChildren = 0;
	int _parentDiffOffset = 0;
	FnIsPropEqual* _parentDiffFunc = nullptr;
	Block* _parent = nullptr;
	Block* _prev = nullptr;
	Block* _next = nullptr;
	Block* _firstChild = nullptr;
	Block* _lastChild = nullptr;

	Presence presence = Presence::Undefined;
	Layout* layout = nullptr;
	Placement* placement = nullptr;
	StackingDirection stacking_direction = StackingDirection::Undefined;
	Edge edge = Edge::Undefined;
	BoxSizing box_sizing = BoxSizing::Undefined;
	HAlign h_align = HAlign::Undefined;
	PaintFunction paint_func;

	FontWeight font_weight = FontWeight::Inherit;
	FontStyle font_style = FontStyle::Inherit;
	Coord font_size;
	Color text_color;

	Coord width;
	Coord height;
	Coord min_width;
	Coord min_height;
	Coord max_width;
	Coord max_height;

	Coord left;
	Coord right;
	Coord top;
	Coord bottom;
	Coord margin_left;
	Coord margin_right;
	Coord margin_top;
	Coord margin_bottom;
	Coord padding_left;
	Coord padding_right;
	Coord padding_top;
	Coord padding_bottom;

	InstanceCounter<g_numBlocks> _ic;
};
static_assert(sizeof(Block) < 268 + (sizeof(void*) - 4) * 20, "style block getting too big?");

class BlockRef
{
public:
	BlockRef() : block(nullptr) {}
	BlockRef(Block* b) : block(b) { b->_refCount++;  }
	BlockRef(const BlockRef& b) : block(b.block) { if (block) block->_refCount++; }
	BlockRef(BlockRef&& b) : block(b.block) { b.block = nullptr; }
	~BlockRef() { if (block) block->_Release(); }
	BlockRef& operator = (Block* b)
	{
		if (b)
			b->_refCount++;
		if (block)
			block->_Release();
		block = b;
		return *this;
	}
	BlockRef& operator = (const BlockRef& b)
	{
		if (b.block)
			b.block->_refCount++;
		if (block)
			block->_Release();
		block = b.block;
		return *this;
	}
	BlockRef& operator = (BlockRef&& b)
	{
		if (block && block != b.block)
			block->_Release();
		block = b.block;
		b.block = nullptr;
		return *this;
	}
	operator Block* () const { return block; }
	Block& operator * () const { return *block; }
	Block* operator -> () const { return block; }

private:
	Block* block;
};

class Accessor
{
public:

	explicit Accessor(Block* b);
	explicit Accessor(BlockRef& r) = delete;
	explicit Accessor(BlockRef& r, UIObject* o);

#if 0
	Display GetDisplay() const;
	void SetDisplay(Display display);

	Position GetPosition() const;
	void SetPosition(Position position);
#endif

	Layout* GetLayout() const;
	void SetLayout(Layout* v);

	Placement* GetPlacement() const;
	void SetPlacement(Placement* v);

	StackingDirection GetStackingDirection() const;
	void SetStackingDirection(StackingDirection v);

	Edge GetEdge() const;
	void SetEdge(Edge v);

	BoxSizing GetBoxSizing() const;
	void SetBoxSizing(BoxSizing v);

	HAlign GetHAlign() const;
	void SetHAlign(HAlign a);

	PaintFunction GetPaintFunc() const;
	void SetPaintFunc(const PaintFunction& f);


	FontWeight GetFontWeight() const;
	void SetFontWeight(FontWeight v);

	FontStyle GetFontStyle() const;
	void SetFontStyle(FontStyle v);

	Coord GetFontSize() const;
	void SetFontSize(Coord v);

	Color GetTextColor() const;
	void SetTextColor(Color v);


	Coord GetWidth() const;
	void SetWidth(Coord v);

	Coord GetHeight() const;
	void SetHeight(Coord v);

	Coord GetMinWidth() const;
	void SetMinWidth(Coord v);

	Coord GetMinHeight() const;
	void SetMinHeight(Coord v);

	Coord GetMaxWidth() const;
	void SetMaxWidth(Coord v);

	Coord GetMaxHeight() const;
	void SetMaxHeight(Coord v);


	Coord GetLeft() const;
	void SetLeft(Coord v);

	Coord GetRight() const;
	void SetRight(Coord v);

	Coord GetTop() const;
	void SetTop(Coord v);

	Coord GetBottom() const;
	void SetBottom(Coord v);


	Coord GetMarginLeft() const;
	void SetMarginLeft(Coord v);

	Coord GetMarginRight() const;
	void SetMarginRight(Coord v);

	Coord GetMarginTop() const;
	void SetMarginTop(Coord v);

	Coord GetMarginBottom() const;
	void SetMarginBottom(Coord v);

	void SetMargin(Coord v) { SetMargin(v, v, v, v); }
	void SetMargin(Coord tb, Coord lr) { SetMargin(tb, lr, tb, lr); }
	void SetMargin(Coord t, Coord lr, Coord b) { SetMargin(t, lr, b, lr); }
	void SetMargin(Coord t, Coord r, Coord b, Coord l);


	Coord GetPaddingLeft() const;
	void SetPaddingLeft(Coord v);

	Coord GetPaddingRight() const;
	void SetPaddingRight(Coord v);

	Coord GetPaddingTop() const;
	void SetPaddingTop(Coord v);

	Coord GetPaddingBottom() const;
	void SetPaddingBottom(Coord v);

	void SetPadding(Coord v) { SetPadding(v, v, v, v); }
	void SetPadding(Coord tb, Coord lr) { SetPadding(tb, lr, tb, lr); }
	void SetPadding(Coord t, Coord lr, Coord b) { SetPadding(t, lr, b, lr); }
	void SetPadding(Coord t, Coord r, Coord b, Coord l);


	Block* block;
	BlockRef* blkref;
	UIObject* owner;
};

void FlexLayout(UIObject* obj);

}
