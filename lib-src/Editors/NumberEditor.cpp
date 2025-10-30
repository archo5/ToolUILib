
#include "NumberEditor.h"

#include "../Elements/Textbox.h"
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

	flags |= UIObject_DB_FocusOnLeftClick | UIObject_DB_CaptureMouseOnLeftClick;
	SetDefaultFrameStyle(DefaultFrameStyle::Button);

	dragConfig = {};
}

void NumberEditorBase::OnPaint(const UIPaintContext& ctx)
{
	auto cpa = PaintFrame();
	if (!_activeTextbox)
	{
		auto* font = frameStyle.font.GetFont();
		int size = frameStyle.font.size;

		auto text = ValueToString(false);

		auto textColor = cpa.HasTextColor() ? cpa.GetTextColor() : Color4b::White(); // TODO to theme
		auto r = GetContentRect();
		draw::TextLine(font, size, r.GetCenterX(), r.y0, text, textColor, TextHAlign::Center, TextBaseline::Top, &GetFinalRect());
	}
	PaintChildren(ctx, cpa);
}

void NumberEditorBase::OnEvent(Event& e)
{
	if (_activeTextbox && e.target == _activeTextbox)
	{
		if (e.type == EventType::LostFocus || e.type == EventType::Commit)
		{
			SetValueFromString(_activeTextbox->GetText());
			_activeTextbox->DetachParent();
			DeleteUIObject(_activeTextbox);
			_activeTextbox = nullptr;
		}
	}
	if (e.type == EventType::ButtonDown && e.GetButton() == MouseButton::Left)
	{
		dragged = false;
	}
	if (e.type == EventType::ButtonUp && e.GetButton() == MouseButton::Left)
	{
		if (!dragged)
		{
			_activeTextbox = new Textbox;
			_activeTextbox->SetText(ValueToString(true));
			AppendChild(_activeTextbox);
			e.context->SetKeyboardFocus(_activeTextbox);
		}
	}
	if (e.type == EventType::MouseMove && e.target->IsPressed() && e.delta.x != 0)
	{
		float speed = dragConfig.GetSpeed(e.GetModifierKeys());
		float snap = dragConfig.GetSnap(e.GetModifierKeys());
		float diff = e.delta.x * speed;

		OnDragEdit(diff, snap);
		dragged = true;
	}
	if (e.type == EventType::SetCursor)
	{
		e.context->SetDefaultCursor(DefaultCursor::ResizeHorizontal);
		e.StopPropagation();
	}
}

Rangef NumberEditorBase::CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type)
{
	float minWidth = frameStyle.font.size * 2 + frameStyle.padding.x0 + frameStyle.padding.x1;
	return FrameElement::CalcEstimatedWidth(containerSize, type).Intersect(Rangef::AtLeast(minWidth));
}

Rangef NumberEditorBase::CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type)
{
	float minHeight = frameStyle.font.size + frameStyle.padding.y0 + frameStyle.padding.y1;
	return FrameElement::CalcEstimatedHeight(containerSize, type).Intersect(Rangef::AtLeast(minHeight));
}

} // ui
