
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
	bool otherSide = false; // defaults to right/bottom
	PainterHandle trackPainter;
	PainterHandle edge1Painter;
	PainterHandle edge2Painter;

	void Serialize(ThemeData& td, IObjectIterator& oi);
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
	bool canZoom = true;
	SubUI<u8> state;
	float origmin = 0;
	float origmax = 0;
	Vec2f dragstartcursor;

	ViewportEditor() { OnReset(); }
	void OnReset()
	{
		LoadStyle();
		canZoom = true;
	}
	void LoadStyle();
	bool OnEvent(Event& e, AABB2f winrect, AABB2f fullarea, AABB2f& viewport);
	void Draw(AABB2f winrect, AABB2f fullarea, AABB2f viewport);
};


} // ui
