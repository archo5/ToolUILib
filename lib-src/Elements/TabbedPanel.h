
#pragma once
#include "../Model/Objects.h"


namespace ui {

struct TabbedPanelStyle
{
	static constexpr const char* NAME = "TabbedPanelStyle";

	//float tabHeight = 22; // TODO autodetect?
	float tabButtonOverlap = 2;
	float tabButtonYOffsetInactive = 1;
	float tabInnerButtonMargin = 4;
	AABB2f tabButtonPadding = {};
	PainterHandle tabButtonPainter;
	FontSettings tabButtonFont;
	AABB2f tabPanelPadding = {};
	PainterHandle tabPanelPainter;
	Size2f tabCloseButtonSize = {};
	PainterHandle tabCloseButtonPainter;

	void Serialize(ThemeData& td, IObjectIterator& oi);
};

struct TabbedPanel;
struct DragDropTab : DragDropData
{
	static constexpr const char* NAME = "tabbed_panel_tab";

	TabbedPanel* curPanel;
	size_t origTabNum;

	DragDropTab(TabbedPanel* tp, size_t tab) : DragDropData(NAME), curPanel(tp), origTabNum(tab) {}
	bool ShouldBuild() override { return false; }
};

struct TabbedPanel : UIObjectSingleChild
{
	struct Tab
	{
		std::string text;
		UIObject* obj = nullptr;
		uintptr_t uid = UINTPTR_MAX;

		float _contentWidth = 0;
	};

	Array<Tab> _tabs;
	size_t _curTabNum = 0;
	float _lastDragUnfinishedDiff = 0; // (-) = the big element is on the left, (+) = right
	SubUI<uint32_t> _tabUI;

	TabbedPanelStyle style;
	float tabHeight = 22;

	UIObject* _tabBarExtension = nullptr;
	bool rebuildOnChange = true;
	bool allowDragReorder = false;
	bool allowDragRemove = false;
	bool _lastMoveWasInside = false;
	bool showCloseButton = false;
	std::function<void(size_t, uintptr_t)> onClose;

	void SetTabBarExtension(UIObject* o);
	void AddTextTab(StringView text, uintptr_t uid = UINTPTR_MAX);
	void AddUITab(UIObject* obj, uintptr_t uid = UINTPTR_MAX);
	uintptr_t GetCurrentTabUID(uintptr_t def = UINTPTR_MAX) const;
	bool SetActiveTabByUID(uintptr_t uid);

	template <class E> void AddEnumTab(StringView name, E value)
	{
		AddTextTab(name, uintptr_t(value));
	}
	template <class E> E GetCurrentTabEnumValue(E def = E(0)) const
	{
		return E(GetCurrentTabUID(uintptr_t(def)));
	}
	template <class E> bool SetActiveTabByEnumValue(E value)
	{
		return SetActiveTabByUID(uintptr_t(value));
	}

	void OnReset() override;
	void OnPaint(const UIPaintContext& ctx) override;
	void OnEvent(Event& e) override;

	Size2f GetReducedContainerSize(Size2f size);
	Rangef CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
	Rangef CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override;
	void OnLayout(const UIRect& rect) override;

	UIObject* SlotIterator_GetNext(UIObjectIteratorData& data) override;
	void RemoveChildImpl(UIObject* ch) override;
	void DetachChildren(bool recursive) override;
	UIObject* FindLastChildContainingPos(Point2f pos) const override;
	void _AttachToFrameContents(FrameContents* owner) override;
	void _DetachFromFrameContents() override;
	void _DetachFromTree() override;
};

} // ui
