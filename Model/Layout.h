
#pragma once
#include <vector>
#include <functional>
#include "../Core/Math.h"
#include "../Core/String.h"


struct UIObject;

using UIRect = AABB<float>;


namespace style {

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

struct Coord
{
	float value;
	CoordTypeUnit unit;

	Coord() : value(0), unit(CoordTypeUnit::Undefined) {}
	Coord(float px) : value(px), unit(CoordTypeUnit::Pixels) {}
	Coord(float v, CoordTypeUnit u) : value(v), unit(u) {}
	bool IsDefined(bool fraction = false) const { return unit != CoordTypeUnit::Undefined && (fraction ? true : unit != CoordTypeUnit::Fraction); }
	static const Coord Undefined() { return {}; }
	static Coord Percent(float p) { return Coord(p, CoordTypeUnit::Percent); }
	static Coord Fraction(float f) { return Coord(f, CoordTypeUnit::Fraction); }
};

enum PaintInfoItemState
{
	PS_Hover = 1 << 0,
	PS_Down = 1 << 1,
	PS_Disabled = 1 << 2,
	PS_Checked = 1 << 3,
};

struct PaintInfo
{
	UIRect rect;
	const UIObject* obj;
	uint32_t state = 0;

	PaintInfo() {}
	PaintInfo(const UIObject* o);
	bool IsHovered() const { return (state & PS_Hover) != 0; }
	bool IsDown() const { return (state & PS_Down) != 0; }
	bool IsDisabled() const { return (state & PS_Disabled) != 0; }
	bool IsChecked() const { return (state & PS_Checked) != 0; }
};

using PaintFunction = std::function<void(const PaintInfo&)>;

struct IErrorCallback
{
	virtual void OnError(const char* message, int line, int ch) = 0;
	void OnError(const char* message, const char* at);

	StringView fullText;
	bool hadError = false;
};

struct Block
{
	void Compile(StringView& text, IErrorCallback* err);
	void MergeDirect(const Block& o);
	void MergeParent(const Block& o);
	void _Release();

	uint32_t _refCount = 0;

	Presence presence = Presence::Undefined;
	Layout* layout = nullptr;
	StackingDirection stacking_direction = StackingDirection::Undefined;
	Edge edge = Edge::Undefined;
	BoxSizing box_sizing = BoxSizing::Undefined;
	HAlign h_align = HAlign::Undefined;
	PaintFunction paint_func;

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
};
static_assert(sizeof(Block) < 256, "style block getting too big?");

class Selector
{
public:
	struct Element
	{
		bool IsDirectMatch(UIObject* obj) const;

		StringView element;
		std::vector<StringView> classes;
		std::vector<StringView> ids;
		bool immParent = false;
		bool prevSibling = false;
		bool firstChild = false;
		bool hover = false;
		bool active = false;
	};

	void Compile(StringView& text, IErrorCallback* err);
	bool Check(UIObject* obj) const;
	bool _CheckOne(UIObject* obj, size_t el) const;

	std::vector<Element> elements;
};

class Definition
{
public:
	void Compile(StringView& text, IErrorCallback* err);
	bool Check(UIObject* obj) const;

	std::vector<Selector> selectors;
	Block block;
};

class Sheet
{
public:
	void Compile(StringView text, IErrorCallback* err);
	void _Preprocess(StringView text, IErrorCallback* err);

	std::vector<Definition> definitions;
	std::string fullText;
};

class Engine
{
public:
	std::vector<Sheet> sheets;
};

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
	Block* GetOrCreate();

public:

	explicit Accessor(Block* b);
	explicit Accessor(BlockRef& r, bool unique = true);

#if 0
	Display GetDisplay() const;
	void SetDisplay(Display display);

	Position GetPosition() const;
	void SetPosition(Position position);
#endif

	Layout* GetLayout() const;
	void SetLayout(Layout* v);

	StackingDirection GetStackingDirection() const;
	void SetStackingDirection(StackingDirection v);

	Edge GetEdge() const;
	void SetEdge(Edge v);

	BoxSizing GetBoxSizing() const;
	void SetBoxSizing(BoxSizing v);

	HAlign GetHAlign() const;
	void SetHAlign(HAlign a);

	PaintFunction GetPaintFunc() const;
	PaintFunction& MutablePaintFunc();


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
};

void FlexLayout(UIObject* obj);

}
