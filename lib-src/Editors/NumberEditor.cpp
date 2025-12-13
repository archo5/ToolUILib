
#include "NumberEditor.h"

#include "../Elements/Textbox.h"
#include "../Core/MathExpr.h"
#include "../Model/ImmediateMode.h"
#include "../Render/RenderText.h"


namespace ui {

NumberEditorBase::~NumberEditorBase()
{
	if (_activeTextbox)
	{
		_activeTextbox->DetachParent();
		DeleteUIObject(_activeTextbox);
	}
}

void NumberEditorBase::OnReset()
{
	FrameElement::OnReset();

	flags |= UIObject_DB_CaptureMouseOnLeftClick;
	if (!_activeTextbox)
		flags |= UIObject_IsFocusable;
	SetDefaultFrameStyle(DefaultFrameStyle::Button);

	dragConfig = {};
}

void NumberEditorBase::OnPaint(const UIPaintContext& ctx)
{
	auto cpa = PaintFrame();

	auto* font = frameStyle.font.GetFont();
	int size = frameStyle.font.size;
	auto r = GetContentRect();

	if (!label.empty())
	{
		float lblw = min(GetTextWidth(font, size, label), r.GetWidth() / 3.f);
		auto cliprect = GetFinalRect();
		cliprect.x1 = r.x0 + lblw;
		draw::TextLine(font, size, r.x0, r.y0, label, Color4b(255, 127), TextBaseline::Top, &cliprect);
		r.x0 = cliprect.x1;
	}
	if (!_activeTextbox)
	{
		auto text = ValueToString(false);

		auto textColor = cpa.HasTextColor() ? cpa.GetTextColor() : Color4b::White(); // TODO to theme
		draw::TextLine(font, size, r.GetCenterX(), r.y0, text, textColor, TextHAlign::Center, TextBaseline::Top, &GetFinalRect());
	}
	PaintChildren(ctx, cpa);
}

static void NEB_CreateTextbox(NumberEditorBase& neb)
{
	if (neb._activeTextbox)
		return;
	neb._activeTextbox = new Textbox;
	neb._activeTextbox->SetText(neb.ValueToString(true));
	neb.AppendChild(neb._activeTextbox);
	neb.SetFlag(UIObject_IsFocusable, false);
}

static void NEB_DestroyTextbox(NumberEditorBase& neb)
{
	neb._activeTextbox->DetachParent();
	DeleteUIObject(neb._activeTextbox); // TODO: delay the deletion - it works for now but is accessing deleted memory
	neb._activeTextbox = nullptr;
	neb.SetFlag(UIObject_IsFocusable, true);
}

void NumberEditorBase::OnEvent(Event& e)
{
	if (_activeTextbox && e.target == _activeTextbox)
	{
		if (e.type == EventType::LostFocus || e.type == EventType::Commit)
		{
			if (!IsInputDisabled() && SetValueFromString(StringView(_activeTextbox->GetText()).trim()))
			{
				e.context->OnChange(this);
				e.context->OnCommit(this);
			}
			NEB_DestroyTextbox(*this); // TODO: delay the deletion - it works for now but is accessing deleted memory
			e.StopPropagation(); // don't let the commit event through
		}
		if (e.type == EventType::Change)
			e.StopPropagation(); // don't let the change event through
		if (e.type == EventType::KeyDown && e.shortCode == ui::KSC_Escape)
		{
			NEB_DestroyTextbox(*this); // TODO: delay the deletion - it works for now but is accessing deleted memory
			e.StopPropagation();
		}
	}
	if (e.type == EventType::GotFocus && e.arg0 && !IsInputDisabled())
	{
		NEB_CreateTextbox(*this);
		e.context->SetKeyboardFocus(_activeTextbox);
		e.StopPropagation();
	}
	if (e.type == EventType::ButtonDown && e.GetButton() == MouseButton::Left)
	{
		_dragged = false;
		_anyChg = false;
		e.context->SetKeyboardFocus(this);
	}
	if (e.type == EventType::ButtonUp && e.GetButton() == MouseButton::Left && !IsInputDisabled())
	{
		if (!_dragged)
		{
			NEB_CreateTextbox(*this);
			e.context->SetKeyboardFocus(_activeTextbox);
		}
		else
		{
			if (_anyChg)
				e.context->OnCommit(this);
		}
	}
	if (e.type == EventType::Click)
	{
		e.StopPropagation();
	}
	if (e.type == EventType::MouseMove && e.target->IsPressed() && !IsInputDisabled())
	{
		if (e.delta.x != 0)
		{
			float speed = dragConfig.GetSpeed(e.GetModifierKeys());
			float snap = dragConfig.GetSnap(e.GetModifierKeys());
			float diff = e.delta.x * speed;

			_dragged = true;
			if (OnDragEdit(diff, snap))
			{
				_anyChg = true;
				e.context->OnChange(this);
			}
		}
		e.StopPropagation();
	}
	if (e.type == EventType::SetCursor && !IsInputDisabled())
	{
		e.context->SetDefaultCursor(DefaultCursor::ResizeHorizontal);
		e.StopPropagation();
	}
}

void NumberEditorBase::_AttachToFrameContents(FrameContents* owner)
{
	FrameElement::_AttachToFrameContents(owner);

	if (_activeTextbox)
		AppendChild(_activeTextbox);
}

EstSizeRange NumberEditorBase::CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type)
{
	float minWidth = frameStyle.font.size * 2 + frameStyle.padding.x0 + frameStyle.padding.x1;
	return FrameElement::CalcEstimatedWidth(containerSize, type).WithSoftMin(minWidth);
}

EstSizeRange NumberEditorBase::CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type)
{
	float minHeight = frameStyle.font.size + frameStyle.padding.y0 + frameStyle.padding.y1;
	return FrameElement::CalcEstimatedHeight(containerSize, type).WithSoftMin(minHeight);
}

void NumberEditorBase::OnLayout(const UIRect& rect, LayoutInfo info)
{
	AABB2f tmppad = frameStyle.padding;
	if (!label.empty())
	{
		auto* font = frameStyle.font.GetFont();
		int size = frameStyle.font.size;
		float lblw = min(GetTextWidth(font, size, label), GetContentRect().GetWidth() / 3.f);
		tmppad.x0 += lblw;
	}
	TmpEdit<AABB2f> pad(frameStyle.padding, tmppad);
	FrameElement::OnLayout(rect, info);
}

Optional<double> NumberEditorBase::ParseMathExpr(StringView str)
{
	MathExpr me;
	if (!me.Compile(str))
		return {};

	return me.Evaluate();
}

} // ui
