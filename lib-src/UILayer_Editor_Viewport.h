
#pragma once

#include "Model/Events.h"
#include "Model/Theme.h"


namespace ui {


struct CombinedCoord
{
	float vpmin = 0;
	float px = 0;

	float Eval(float vp) const
	{
		return vpmin * vp + px;
	}
	void OnSerialize(IObjectIterator& oi, const FieldInfo& fi)
	{
		oi.BeginObject(fi, "CombinedCoord");
		OnField(oi, "vpmin", vpmin);
		OnField(oi, "px", px);
		oi.EndObject();
	}
};

struct ViewportEditorStyle
{
	static constexpr const char* NAME = "ViewportEditorStyle";

	CombinedCoord areaMargin = { 0.01f, 1 };
	CombinedCoord trackWidth = { 0.05f, 5 };
	CombinedCoord trackEdgeMargin = { 0.01f, 1 };
	CombinedCoord trackMinLength = { 0.11f, 11 };
	bool otherSide = false; // defaults to right/bottom
	PainterHandle trackPainter;
	PainterHandle edge1Painter;
	PainterHandle edge2Painter;

	void Serialize(ThemeData& td, IObjectIterator& oi);
};

enum class ViewportEditorScrollMode : u8
{
	None,
	Inherit, // for scroll track modes
	Scroll,
	Zoom,
};

struct ViewportEditorConfig
{
	float zoomScrollAmount = 1.25f;
	float zoomDragAmount = 1.01f;
	float hScrollAmountPx = 50;
	float vScrollAmountPx = 50;
	ViewportEditorScrollMode baseScrollMode = ViewportEditorScrollMode::Zoom;
	ViewportEditorScrollMode hScrollMode = ViewportEditorScrollMode::Scroll;
	ViewportEditorScrollMode vScrollMode = ViewportEditorScrollMode::Scroll;
	enum : u8
	{
		MMB_Enable = 1, // mmb = drag, ctrl+mmb = zoom
		MMB_Flip = 2, // flip ctrl effect
	};
	u8 middleMouseBtnFlags = MMB_Enable;
};

struct ViewportEditorInputs
{
	AABB2f winrect;
	AABB2f fullarea;
	AABB2f& viewport;
	bool flipY = false;
};

struct ViewportEditor
{
	enum Elements : u8
	{
		None = u8(-1),
		HSlide = 0,
		VSlide,
		HEdgeL,
		HEdgeR,
		VEdgeT,
		VEdgeB,
	};
	ViewportEditorStyle styleH;
	ViewportEditorStyle styleV;
	ViewportEditorConfig config;
	SubUI<u8> state;
	u8 isMMBDown : 1;
	u8 isOnHScroll : 1;
	u8 isOnVScroll : 1;
	u8 isZooming : 1;
	AABB2f origvp;
	Vec2f dragstartcursor;
	Vec2f dragstartvpcur;

	ViewportEditor() : isMMBDown(0), isOnHScroll(0), isOnVScroll(0), isZooming(0) { OnReset(); }
	void OnReset()
	{
		LoadStyle();
		config = {};
	}
	void LoadStyle();
	bool OnEvent(Event& e, const ViewportEditorInputs& inputs);
	void Draw(const ViewportEditorInputs& inputs);
};


} // ui
