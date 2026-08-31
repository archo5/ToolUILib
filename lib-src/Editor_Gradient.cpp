
#include "Editor_Gradient.h"

#include "Model/Theme.h"
#include "Model/System.h"
#include "Layout_Stack.h"
#include "Model/ImmediateMode.h"
#include "Model/Graphics.h"
#include "Model/Menu.h"


namespace ui {


void GradientSliderStyle::Serialize(ThemeData& td, IObjectIterator& oi)
{
	OnField(oi, "gradientHeight", gradientHeight);
	OnFieldBorderBox(oi, "gradientMargin", gradientMargin);
	OnField(oi, "keypointOffset", keypointOffset);
	OnField(oi, "keypointSize", keypointSize);
	OnFieldBorderBox(oi, "keypointMargin", keypointMargin);
	OnFieldPainter(oi, td, "backgroundPainter", backgroundPainter);
	OnFieldPainter(oi, td, "trackFramePainter", trackFramePainter);
	OnFieldPainter(oi, td, "keypointFramePainter", keypointFramePainter);
}


i32 GradientSlider::FindKP(Vec2f pos)
{
	if (!gradient)
		return 0;

	if (!GetFinalRect().Contains(pos))
		return 0;

	float kpheight = style.keypointSize.y;
	AABB2f track = GetFinalRect();
	track.y0 += kpheight + style.keypointOffset;
	if (gradient->separateAlpha)
		track.y1 -= kpheight + style.keypointOffset;
	track = track.ShrinkBy(style.gradientMargin);
	track.x0 = roundf(track.x0);
	track.x1 = roundf(track.x1);

	float hkpw = style.keypointSize.x / 2;
	if (pos.y < track.y0)
	{
		float closestdist = FLT_MAX;
		i32 closestpoint = 0;
		float y = track.y0 - style.keypointOffset;
		for (size_t i = 0; i < gradient->colors.Size(); i++)
		{
			float x = lerp(track.x0, track.x1, gradient->colors[i].pos);
			AABB2f kbox = { x - hkpw, y - kpheight, x + hkpw, y };
			kbox = kbox.ExtendBy(style.keypointMargin);
			if (kbox.Contains(pos))
			{
				float dist = fabsf(pos.x - x);
				if (dist < closestdist)
				{
					closestdist = dist;
					closestpoint = i + 1;
				}
			}
		}
		return closestpoint;
	}

	if (gradient->separateAlpha && pos.y > track.y1)
	{
		float closestdist = FLT_MAX;
		i32 closestpoint = 0;
		float y = track.y1 + style.keypointOffset;
		for (size_t i = 0; i < gradient->alphas.Size(); i++)
		{
			float x = lerp(track.x0, track.x1, gradient->alphas[i].pos);
			AABB2f kbox = { x - hkpw, y, x + hkpw, y + kpheight };
			kbox = kbox.ExtendBy(style.keypointMargin);
			if (kbox.Contains(pos))
			{
				float dist = fabsf(pos.x - x);
				if (dist < closestdist)
				{
					closestdist = dist;
					closestpoint = -i32(i + 1);
				}
			}
		}
		return closestpoint;
	}

	return 0;
}


static StaticID<GradientSliderStyle> sid_gradient_slider_style("gradient_slider_style");
void GradientSlider::OnReset()
{
	UIObjectNoChildren::OnReset();

	style = *GetCurrentTheme()->GetStruct(sid_gradient_slider_style);
	gradient = nullptr;
}

static StaticID_ImageSet sid_bgr_checkerboard("bgr-checkerboard");
void GradientSlider::OnPaint(const UIPaintContext& ctx)
{
	ContentPaintAdvice cpa;
	if (style.backgroundPainter)
		cpa = style.backgroundPainter->Paint(this);

	if (!gradient)
		return;

	draw::ImageSetHandle checkerboard = GetCurrentTheme()->GetImageSet(sid_bgr_checkerboard);
	float kpheight = style.keypointSize.y;

	// track
	PaintInfo pinfo(this);
	AABB2f rect = pinfo.rect;
	AABB2f track = rect;
	track.y0 += kpheight + style.keypointOffset;
	if (gradient->separateAlpha)
		track.y1 -= kpheight + style.keypointOffset;
	track = track.ShrinkBy(style.gradientMargin);
	track.x0 = roundf(track.x0);
	track.x1 = roundf(track.x1);
	if (style.trackFramePainter)
	{
		pinfo.rect = track;
		style.trackFramePainter->Paint(pinfo);
	}
	if (checkerboard)
		checkerboard->Draw(track);

	// track fill
	for (float x = track.x0; x <= track.x1; x++)
	{
		draw::RectCol(x, track.y0, x + 1, track.y1, gradient->Sample(invlerpc(track.x0, track.x1, x)));
	}

	float hkpw = style.keypointSize.x / 2;
	// color keypoints
	{
		float y = track.y0 - style.keypointOffset;
		for (auto& ckp : gradient->colors)
		{
			float x = lerp(track.x0, track.x1, ckp.pos);

			draw::AALineCol(x, y, x, y + style.keypointOffset, 1, {});

			AABB2f kbox = { x - hkpw, y - kpheight, x + hkpw, y };
			if (style.keypointFramePainter)
			{
				pinfo.rect = kbox.ExtendBy(style.keypointMargin);
				pinfo.state = 0, pinfo.checkState = 0;
				i32 curkp = i32(gradient->colors.GetElementIndex(ckp) + 1);
				if (curkp == hoveredkp)
					pinfo.state |= PS_Hover;
				if (curkp == pressedkp)
					pinfo.state |= PS_Down;
				if (curkp == gradient->curkp)
					pinfo.state |= PS_Checked, pinfo.checkState = 1;
				style.keypointFramePainter->Paint(pinfo);
			}
			Color4f col = ckp.color;
			if (gradient->separateAlpha)
				col.a = 1;
			else if (col.a < 1 && checkerboard)
				checkerboard->Draw(kbox);

			Vec2f boxpts[4] = { kbox.GetP00(), kbox.GetP01(), kbox.GetP11(), kbox.GetP10() };
			draw::AAPolyCol(boxpts, col);
		}
	}

	// alpha keypoints
	if (gradient->separateAlpha)
	{
		float y = track.y1 + style.keypointOffset;
		for (auto& akp : gradient->alphas)
		{
			float x = lerp(track.x0, track.x1, akp.pos);

			draw::AALineCol(x, y, x, y - style.keypointOffset, 1, {});

			AABB2f kbox = { x - hkpw, y, x + hkpw, y + kpheight };
			if (style.keypointFramePainter)
			{
				pinfo.rect = kbox.ExtendBy(style.keypointMargin);
				pinfo.state = 0, pinfo.checkState = 0;
				i32 curkp = -i32(gradient->alphas.GetElementIndex(akp) + 1);
				if (curkp == hoveredkp)
					pinfo.state |= PS_Hover;
				if (curkp == pressedkp)
					pinfo.state |= PS_Down;
				if (curkp == gradient->curkp)
					pinfo.state |= PS_Checked, pinfo.checkState = 1;
				style.keypointFramePainter->Paint(pinfo);
			}

			Vec2f boxpts[4] = { kbox.GetP00(), kbox.GetP01(), kbox.GetP11(), kbox.GetP10() };
			draw::AAPolyCol(boxpts, Color4f(akp.alpha, 1));
		}
	}
}

void GradientSlider::OnEvent(Event& e)
{
	if (e.type == EventType::ButtonDown && e.GetButton() == MouseButton::Left)
	{
		e.context->CaptureMouse(this);
		pressedkp = hoveredkp;

		_mxoff = 0;
		if (gradient)
		{
			gradient->curkp = hoveredkp;
			if (hoveredkp > 0)
				_mxoff = KPPToUIX(gradient->colors[hoveredkp - 1].pos) - e.position.x;
			else if (hoveredkp < 0)
				_mxoff = KPPToUIX(gradient->alphas[-hoveredkp - 1].pos) - e.position.x;
		}

		e.context->OnChange(this);
	}
	if (e.type == EventType::ButtonUp && e.GetButton() == MouseButton::Left)
	{
		e.context->ReleaseMouse();
		pressedkp = 0;
		if (!IsInputDisabled())
			e.context->OnCommit(this);
	}
	if (e.type == EventType::MouseMove)
	{
		if (pressedkp && !IsInputDisabled() && gradient)
		{
			float kpp = UIXToKPP(e.position.x + _mxoff);
			if (pressedkp > 0)
			{
				i32 i = pressedkp - 1;
				gradient->colors[i].pos = kpp;
				Gradient::SortPoint(gradient->colors, i);
				gradient->curkp = hoveredkp = pressedkp = i + 1;
			}
			else
			{
				i32 i = -pressedkp - 1;
				gradient->alphas[i].pos = kpp;
				Gradient::SortPoint(gradient->alphas, i);
				gradient->curkp = hoveredkp = pressedkp = -i - 1;
			}
			e.context->OnChange(this);
		}
		else
			hoveredkp = FindKP(e.position);
	}
	if (e.type == EventType::ContextMenu)
	{
		if (!gradient)
			return;
		MenuItemCollection& cm = ContextMenu::Get();
		cm.AddNext("Delete point", hoveredkp == 0) = [this]()
		{
			if (!hoveredkp)
				return;
			if (hoveredkp > 0)
				gradient->colors.RemoveAt(hoveredkp - 1);
			else
				gradient->alphas.RemoveAt(-hoveredkp - 1);
			if (gradient->curkp == hoveredkp)
				gradient->curkp = 0;
			pressedkp = 0;
			hoveredkp = 0;
			system->eventSystem.OnChange(this);
		};

		AABB2f track = GetFinalRect();
		float kpheight = style.keypointSize.y;
		track.y0 += kpheight + style.keypointOffset;
		if (gradient->separateAlpha)
			track.y1 -= kpheight + style.keypointOffset;
		float pos = UIXToKPP(e.position.x);

		if (e.position.y < track.y1)
		{
			cm.AddNext(Format("Add color point at %.3f", pos)) = [this, pos]()
			{
				TmpEdit<bool> te(gradient->separateAlpha, false);
				Color4f col = gradient->Sample(pos);
				i32 i = i32(gradient->colors.Size());
				gradient->colors.Append({ pos, col });
				Gradient::SortPoint(gradient->colors, i);
				system->eventSystem.OnChange(this);
			};
		}
		else if (e.position.y > track.y0 && gradient->separateAlpha)
		{
			cm.AddNext(Format("Add alpha point at %.3f", pos)) = [this, pos]()
			{
				float alpha = gradient->Sample(pos).a;
				i32 i = i32(gradient->alphas.Size());
				gradient->alphas.Append({ pos, alpha });
				Gradient::SortPoint(gradient->alphas, i);
				system->eventSystem.OnChange(this);
			};
		}
	}
}

Rangef GradientSlider::TrackUIRange() const
{
	auto rect = GetFinalRect();
	rect.x0 += style.gradientMargin.x0;
	rect.x1 -= style.gradientMargin.x1;
	rect.x0 = roundf(rect.x0);
	rect.x1 = roundf(rect.x1);
	return { rect.x0, rect.x1 };
}

float GradientSlider::UIXToKPP(float uix) const
{
	auto tur = TrackUIRange();
	return invlerpc(tur.min, tur.max, uix);
}

float GradientSlider::KPPToUIX(float kpp) const
{
	auto tur = TrackUIRange();
	return lerp(tur.min, tur.max, kpp);
}


void GradientEditor::Build()
{
	bool chg = false;
	Push<StackTopDownLayoutElement>();
	{
		Push<StackExpandLTRLayoutElement>();
		if (gradient)
		{
			chg |= imDropdownMenuList(gradient->colorSpace, UI_BUILD_ALLOC(ui::CStrArrayOptionList)(NullTerminated{}, Gradient::ColorSpaceNames));
			if (gradient->colorSpace == Gradient::ColorSpace::HSV)
				chg |= imDropdownMenuList(gradient->hueMode, UI_BUILD_ALLOC(ui::CStrArrayOptionList)(NullTerminated{}, Gradient::HueModeNames));
			chg |= imDropdownMenuList(gradient->colorCorrection, UI_BUILD_ALLOC(ui::CStrArrayOptionList)(NullTerminated{}, Gradient::ColorCorrectionNames));
			chg |= imDropdownMenuList(gradient->interpolationType, UI_BUILD_ALLOC(ui::CStrArrayOptionList)(NullTerminated{}, Gradient::InterpolationTypeNames));
			chg |= imEditBool(gradient->separateAlpha, "Separate alpha", imm::ButtonStateToggleSkin());
			StdText("Approx.steps:");
			chg |= imEditInt(gradient->approxSteps, {}, { 0, 65535 });
		}
		Pop();

		Make<GradientSlider>().Init(gradient).HandleEvent(EventType::Change) = [this](Event&)
		{
			Rebuild();
			system->eventSystem.OnChange(this);
		};

		if (gradient)
		{
			imLabel("Current point"), chg |= imEditInt(gradient->curkp, {}, { -i32(gradient->alphas.Size()), i32(gradient->colors.Size()) });
			if (gradient->curkp < 0 && gradient->curkp >= -i32(gradient->alphas.Size()))
			{
				i32 i = -gradient->curkp - 1;
				if (imLabel("Position"), chg |= imEditFloat(gradient->alphas[i].pos, 0.01f, { 0.f, 1.f }))
				{
					Gradient::SortPoint(gradient->alphas, i);
					gradient->curkp = -(i + 1);
				}
				imLabel("Alpha"), chg |= imEditFloat(gradient->alphas[i].alpha, 0.01f, { 0, 1 });
			}
			else if (gradient->curkp > 0 && gradient->curkp <= i32(gradient->colors.Size()))
			{
				i32 i = gradient->curkp - 1;
				if (imLabel("Position"), chg |= imEditFloat(gradient->colors[i].pos, 0.01f, { 0.f, 1.f }))
				{
					Gradient::SortPoint(gradient->colors, i);
					gradient->curkp = i + 1;
				}

				auto& colpk = Make<ColorPicker>();
				colpk.SetColor(gradient->colors[i].color);
				colpk.HandleEvent(EventType::Change) = [&colpk, this](Event&)
				{
					gradient->colors[gradient->curkp - 1].color = colpk.GetColor().GetRGBA();
					system->eventSystem.OnChange(this);
				};
			}
		}
	}
	Pop();
	if (chg)
		system->eventSystem.OnChange(this);
}


} // ui
