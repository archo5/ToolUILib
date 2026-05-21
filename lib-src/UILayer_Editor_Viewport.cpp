
#include "UILayer_Editor_Viewport.h"

#include "Model/EventSystem.h"


namespace ui {


void ViewportEditorStyle::Serialize(ThemeData& td, IObjectIterator& oi)
{
	OnField(oi, "areaMargin", areaMargin);
	OnField(oi, "trackWidth", trackWidth);
	OnField(oi, "trackEdgeMargin", trackEdgeMargin);
	OnField(oi, "otherSide", otherSide);
	OnFieldPainter(oi, td, "trackPainter", trackPainter);
	OnFieldPainter(oi, td, "edge1Painter", edge1Painter);
	OnFieldPainter(oi, td, "edge2Painter", edge2Painter);
}

StaticID<ViewportEditorStyle> sid_viewport_editor_style_hor("viewport_editor_style_hor");
StaticID<ViewportEditorStyle> sid_viewport_editor_style_vert("viewport_editor_style_vert");

void ViewportEditor::LoadStyle()
{
	styleH = *GetCurrentTheme()->GetStruct<ViewportEditorStyle>(sid_viewport_editor_style_hor);
	styleV = *GetCurrentTheme()->GetStruct<ViewportEditorStyle>(sid_viewport_editor_style_vert);
}

struct ViewportEditorLayout
{
	AABB2f hsliderect, vsliderect;
	AABB2f hedgelrect, hedgerrect, vedgetrect, vedgebrect;
	float htracklen, vtracklen;
	float htrackwidth, vtrackwidth;

	ViewportEditorLayout(const ViewportEditorStyle& styleH, const ViewportEditorStyle& styleV, AABB2f winrect, AABB2f fullarea, AABB2f viewport)
	{
		float vpmin = winrect.GetSize().Min();

		float amH = roundf(styleH.areaMargin.Eval(vpmin));
		float twH = roundf(styleH.trackWidth.Eval(vpmin));
		float temH = roundf(styleH.trackEdgeMargin.Eval(vpmin));
		float tmlH = roundf(styleH.trackMinLength.Eval(vpmin));

		float amV = roundf(styleV.areaMargin.Eval(vpmin));
		float twV = roundf(styleV.trackWidth.Eval(vpmin));
		float temV = roundf(styleV.trackEdgeMargin.Eval(vpmin));
		float tmlV = roundf(styleV.trackMinLength.Eval(vpmin));

		htrackwidth = amH * 2 + twH;
		vtrackwidth = amV * 2 + twV;

		AABB2f hscrollrect, vscrollrect;

		if (styleH.otherSide)
		{
			hscrollrect.y0 = winrect.y0 + amH;
			hscrollrect.y1 = hscrollrect.y0 + twH;
		}
		else
		{
			hscrollrect.y1 = winrect.y1 - amH;
			hscrollrect.y0 = hscrollrect.y1 - twH;
		}
		if (styleV.otherSide)
		{
			vscrollrect.x0 = winrect.x0 + amV;
			vscrollrect.x1 = vscrollrect.x0 + twV;
		}
		else
		{
			vscrollrect.x1 = winrect.x1 - amV;
			vscrollrect.x0 = vscrollrect.x1 - twV;
		}

		if (styleV.otherSide)
		{
			hscrollrect.x1 = winrect.x1 - amH;
			hscrollrect.x0 = vscrollrect.x1 + amH;
		}
		else
		{
			hscrollrect.x0 = winrect.x0 + amH;
			hscrollrect.x1 = vscrollrect.x0 - amH;
		}
		if (styleH.otherSide)
		{
			vscrollrect.y0 = hscrollrect.y1 + amH;
			vscrollrect.y1 = winrect.y1 - amH;
		}
		else
		{
			vscrollrect.y0 = winrect.y0 + amH;
			vscrollrect.y1 = hscrollrect.y0 - amH;
		}

		htracklen = hscrollrect.GetWidth();
		vtracklen = vscrollrect.GetHeight();

		float qx0 = invlerpc(fullarea.x0, fullarea.x1, viewport.x0);
		float qx1 = invlerpc(fullarea.x0, fullarea.x1, viewport.x1);
		float qy0 = invlerpc(fullarea.y0, fullarea.y1, viewport.y0);
		float qy1 = invlerpc(fullarea.y0, fullarea.y1, viewport.y1);

		hsliderect = hscrollrect;
		vsliderect = vscrollrect;

		hsliderect.x0 = lerp(hscrollrect.x0, hscrollrect.x1, qx0);
		hsliderect.x1 = lerp(hscrollrect.x0, hscrollrect.x1, qx1);
		vsliderect.y0 = lerp(vscrollrect.y0, vscrollrect.y1, qy0);
		vsliderect.y1 = lerp(vscrollrect.y0, vscrollrect.y1, qy1);

		MinAndClamp(hsliderect.x0, hsliderect.x1, tmlH, { winrect.x0, winrect.x1 });
		MinAndClamp(vsliderect.y0, vsliderect.y1, tmlV, { winrect.y0, winrect.y1 });

		AABB2f hitrect = hsliderect.ShrinkBy(temH);
		AABB2f vitrect = vsliderect.ShrinkBy(temV);

		hedgelrect = hedgerrect = hitrect;
		vedgetrect = vedgebrect = vitrect;

		hedgelrect.x1 = hedgelrect.x0 + hedgelrect.GetHeight();
		hedgerrect.x0 = hedgerrect.x1 - hedgerrect.GetHeight();
		vedgetrect.y1 = vedgetrect.y0 + vedgetrect.GetWidth();
		vedgebrect.y0 = vedgebrect.y1 - vedgebrect.GetWidth();
	}

	static void MinAndClamp(float& tmin, float& tmax, float minlen, Rangef winrange)
	{
		if (tmax < tmin)
			tmin = tmax = (tmin + tmax) * 0.5f;
		float curlen = tmax - tmin;
		if (curlen >= minlen)
			return;
		float hldiff = (minlen - curlen) * 0.5f;
		tmin -= hldiff;
		tmax += hldiff;
		if (tmin < winrange.min)
			tmin = winrange.min, tmax = winrange.min + minlen;
		if (tmax > winrange.max)
			tmax = winrange.max, tmin = winrange.max - minlen;
		if (tmin < winrange.min)
			tmin = winrange.min;
	}
};

bool ViewportEditor::OnEvent(Event& e, AABB2f winrect, AABB2f fullarea, AABB2f& viewport)
{
	ViewportEditorLayout layout(styleH, styleV, winrect, fullarea, viewport);

	float diff;
	bool edited = false;
	state.InitOnEvent(e);
#define START \
	e.context->CaptureMouse(e.current); \
	dragstartcursor = e.position; \
	e.StopPropagation(); \
//
#define HSAVE \
	origvp.x0 = viewport.x0; \
	origvp.x1 = viewport.x1; \
//
#define VSAVE \
	origvp.y0 = viewport.y0; \
	origvp.y1 = viewport.y1; \
//
#define ENDMOVE \
	edited = true; \
	e.StopPropagation(); \
//
	// horizontal
	{
		diff = divf_safe(e.position.x - dragstartcursor.x, layout.htracklen) * fullarea.GetWidth();
		switch (state.DragOnEvent(HSlide, layout.hsliderect, e))
		{
		case SubUIDragState::Stop: e.context->ReleaseMouse(); break;
		case SubUIDragState::Start: START; HSAVE; break;
		case SubUIDragState::Move: viewport.x0 = origvp.x0 + diff; viewport.x1 = origvp.x1 + diff; ENDMOVE; break;
		}
		switch (state.DragOnEvent(HEdgeL, layout.hedgelrect, e))
		{
		case SubUIDragState::Stop: e.context->ReleaseMouse(); break;
		case SubUIDragState::Start: START; HSAVE; break;
		case SubUIDragState::Move: viewport.x0 = origvp.x0 + diff; ENDMOVE; break;
		}
		switch (state.DragOnEvent(HEdgeR, layout.hedgerrect, e))
		{
		case SubUIDragState::Stop: e.context->ReleaseMouse(); break;
		case SubUIDragState::Start: START; HSAVE; break;
		case SubUIDragState::Move: viewport.x1 = origvp.x1 + diff; ENDMOVE; break;
		}
	}
	// vertical
	{
		diff = divf_safe(e.position.y - dragstartcursor.y, layout.vtracklen) * fullarea.GetHeight();
		switch (state.DragOnEvent(VSlide, layout.vsliderect, e))
		{
		case SubUIDragState::Stop: e.context->ReleaseMouse(); break;
		case SubUIDragState::Start: START; VSAVE; break;
		case SubUIDragState::Move: viewport.y0 = origvp.y0 + diff; viewport.y1 = origvp.y1 + diff; ENDMOVE; break;
		}
		switch (state.DragOnEvent(VEdgeT, layout.vedgetrect, e))
		{
		case SubUIDragState::Stop: e.context->ReleaseMouse(); break;
		case SubUIDragState::Start: START; VSAVE; break;
		case SubUIDragState::Move: viewport.y0 = origvp.y0 + diff; ENDMOVE; break;
		}
		switch (state.DragOnEvent(VEdgeB, layout.vedgebrect, e))
		{
		case SubUIDragState::Stop: e.context->ReleaseMouse(); break;
		case SubUIDragState::Start: START; VSAVE; break;
		case SubUIDragState::Move: viewport.y1 = origvp.y1 + diff; ENDMOVE; break;
		}
	}
#undef ENDMOVE
#undef VSAVE
#undef HSAVE
#undef START
	state.FinalizeOnEvent(e);
	bool ish = styleH.otherSide ? e.position.y < winrect.y0 + layout.htrackwidth : e.position.y > winrect.y1 - layout.htrackwidth;
	bool isv = styleV.otherSide ? e.position.x < winrect.x0 + layout.vtrackwidth : e.position.x > winrect.x1 - layout.vtrackwidth;
	if (e.type == ui::EventType::MouseScroll)
	{
		ViewportEditorScrollMode vesm = config.baseScrollMode;
		if (config.hScrollMode != ViewportEditorScrollMode::Inherit)
		{
			if (ish)
				vesm = config.hScrollMode;
		}
		if (config.vScrollMode != ViewportEditorScrollMode::Inherit)
		{
			if (isv)
				vesm = config.vScrollMode;
		}

		if (vesm == ViewportEditorScrollMode::Scroll)
		{
			Vec2f d = e.delta;
			d.x = sign(d.x), d.y = sign(d.y);
			if (ish) d = { d.y, d.x };
			if (e.GetModifierKeys() & MK_Ctrl) d = { d.y, d.x };
			d.x *= divf_safe(fullarea.GetWidth() * config.hScrollAmountPx, layout.htracklen);
			d.y *= divf_safe(fullarea.GetHeight() * config.vScrollAmountPx, layout.vtracklen);
			viewport -= d;
		}
		else if (vesm == ViewportEditorScrollMode::Zoom)
		{
			Vec2f vpcpos = viewport.Lerp(winrect.InverseLerp(e.position));
			viewport -= vpcpos;
			viewport = viewport * (e.delta.y < 0 ? config.zoomScrollAmount : 1.f / config.zoomScrollAmount);
			viewport += vpcpos;
		}
	}
	if (e.type == ui::EventType::ButtonDown && e.GetButton() == ui::MouseButton::Middle)
	{
		e.context->CaptureMouse(e.current);
		dragstartcursor = e.position;
		dragstartvpcur = viewport.Lerp(winrect.InverseLerp(dragstartcursor));
		origvp = viewport;
		isMMBDown = true;
		isOnHScroll = ish && !isv;
		isOnVScroll = isv && !ish;
		isZooming = !!(e.GetModifierKeys() & MK_Ctrl) ^ !!(config.middleMouseBtnFlags & config.MMB_Flip);
	}
	if (e.type == ui::EventType::MouseMove && isMMBDown)
	{
		if (config.middleMouseBtnFlags & config.MMB_Enable)
		{
			viewport = origvp;
			if (!isZooming)
			{
				float diffx = divf_safe(e.position.x - dragstartcursor.x, winrect.GetWidth()) * origvp.GetWidth();
				float diffy = divf_safe(e.position.y - dragstartcursor.y, winrect.GetHeight()) * origvp.GetHeight();
				viewport.x0 -= diffx, viewport.x1 -= diffx;
				viewport.y0 -= diffy, viewport.y1 -= diffy;
			}
			else
			{
				viewport -= dragstartvpcur;
				viewport = viewport * powf(config.zoomDragAmount, e.position.y - dragstartcursor.y);
				viewport += dragstartvpcur;
				if (isOnHScroll)
					viewport.y0 = origvp.y0, viewport.y1 = origvp.y1;
				if (isOnVScroll)
					viewport.x0 = origvp.x0, viewport.x1 = origvp.x1;
			}
		}
	}
	if (e.type == ui::EventType::ButtonUp && e.GetButton() == ui::MouseButton::Middle)
	{
		e.context->ReleaseMouse();
		isMMBDown = false;
	}
	return edited;
}

void ViewportEditor::Draw(AABB2f winrect, AABB2f fullarea, AABB2f viewport)
{
	ViewportEditorLayout layout(styleH, styleV, winrect, fullarea, viewport);

	PaintInfo pi;

	if (styleH.trackPainter)
	{
		pi.rect = layout.hsliderect;
		pi.state = (state.IsPressed(HSlide) ? PS_Down : 0) | (state.IsHovered(HSlide) ? PS_Hover : 0);
		styleH.trackPainter->Paint(pi);
	}

	if (styleV.trackPainter)
	{
		pi.rect = layout.vsliderect;
		pi.state = (state.IsPressed(VSlide) ? PS_Down : 0) | (state.IsHovered(VSlide) ? PS_Hover : 0);
		styleV.trackPainter->Paint(pi);
	}

	if (styleH.edge1Painter)
	{
		pi.rect = layout.hedgelrect;
		pi.state = (state.IsPressed(HEdgeL) ? PS_Down : 0) | (state.IsHovered(HEdgeL) ? PS_Hover : 0);
		styleH.edge1Painter->Paint(pi);
	}

	if (styleH.edge2Painter)
	{
		pi.rect = layout.hedgerrect;
		pi.state = (state.IsPressed(HEdgeR) ? PS_Down : 0) | (state.IsHovered(HEdgeR) ? PS_Hover : 0);
		styleH.edge2Painter->Paint(pi);
	}

	if (styleV.edge1Painter)
	{
		pi.rect = layout.vedgetrect;
		pi.state = (state.IsPressed(VEdgeT) ? PS_Down : 0) | (state.IsHovered(VEdgeT) ? PS_Hover : 0);
		styleV.edge1Painter->Paint(pi);
	}

	if (styleV.edge2Painter)
	{
		pi.rect = layout.vedgebrect;
		pi.state = (state.IsPressed(VEdgeB) ? PS_Down : 0) | (state.IsHovered(VEdgeB) ? PS_Hover : 0);
		styleV.edge2Painter->Paint(pi);
	}
}


} // ui
