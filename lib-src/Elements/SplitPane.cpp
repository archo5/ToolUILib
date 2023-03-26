
#include "SplitPane.h"

#include "../Model/Theme.h"
#include "../Model/System.h"


namespace ui {

void SeparatorLineStyle::Serialize(ThemeData& td, IObjectIterator& oi)
{
	OnField(oi, "size", size);
	OnFieldPainter(oi, td, "backgroundPainter", backgroundPainter);
}

static StaticID<SeparatorLineStyle> sid_seplinestyle_hor("split_pane_separator_horizontal");
static StaticID<SeparatorLineStyle> sid_seplinestyle_vert("split_pane_separator_vertical");
void SplitPane::OnReset()
{
	UIObject::OnReset();

	vertSepStyle = *GetCurrentTheme()->GetStruct(sid_seplinestyle_vert);
	horSepStyle = *GetCurrentTheme()->GetStruct(sid_seplinestyle_hor);

	_verticalSplit = false;
	_children.Clear();
}

static float SplitQToX(SplitPane* sp, float split)
{
	float hw = sp->vertSepStyle.size / 2;
	return lerp(sp->GetFinalRect().x0 + hw, sp->GetFinalRect().x1 - hw, split);
}

static float SplitQToY(SplitPane* sp, float split)
{
	float hh = sp->horSepStyle.size / 2;
	return lerp(sp->GetFinalRect().y0 + hh, sp->GetFinalRect().y1 - hh, split);
}

static float SplitXToQ(SplitPane* sp, float c)
{
	float hw = sp->vertSepStyle.size / 2;
	return sp->GetFinalRect().GetWidth() > hw * 2 ? (c - sp->GetFinalRect().x0 - hw) / (sp->GetFinalRect().GetWidth() - hw * 2) : 0;
}

static float SplitYToQ(SplitPane* sp, float c)
{
	float hh = sp->horSepStyle.size / 2;
	return sp->GetFinalRect().GetHeight() > hh * 2 ? (c - sp->GetFinalRect().y0 - hh) / (sp->GetFinalRect().GetHeight() - hh * 2) : 0;
}

static UIRect GetSplitRectH(SplitPane* sp, int which)
{
	float split = sp->_splits[which];
	float w = sp->vertSepStyle.size;
	float hw = w / 2;
	float x = roundf(lerp(sp->GetFinalRect().x0 + hw, sp->GetFinalRect().x1 - hw, split) - hw);
	return { x, sp->GetFinalRect().y0, x + roundf(w), sp->GetFinalRect().y1 };
}

static UIRect GetSplitRectV(SplitPane* sp, int which)
{
	float split = sp->_splits[which];
	float h = sp->horSepStyle.size;
	float hh = h / 2;
	float y = roundf(lerp(sp->GetFinalRect().y0 + hh, sp->GetFinalRect().y1 - hh, split) - hh);
	return { sp->GetFinalRect().x0, y, sp->GetFinalRect().x1, y + roundf(h) };
}

static float SplitWidthAsQ(SplitPane* sp)
{
	if (!sp->_verticalSplit)
	{
		if (sp->GetFinalRect().GetWidth() == 0)
			return 0;
		float w = sp->vertSepStyle.size;
		return w / max(w, sp->GetFinalRect().GetWidth() - w);
	}
	else
	{
		if (sp->GetFinalRect().GetHeight() == 0)
			return 0;
		float w = sp->horSepStyle.size;
		return w / max(w, sp->GetFinalRect().GetHeight() - w);
	}
}

static void CheckSplits(SplitPane* sp, size_t needElement = SIZE_MAX)
{
	size_t cc = sp->_children.size() - 1;
	if (cc == SIZE_MAX)
		cc = 0;
	if (needElement != SIZE_MAX)
		cc = max(cc, needElement + 1);
	size_t oldsize = sp->_splits.size();
	if (oldsize < cc)
	{
		// rescale to fit
		float scale = cc ? cc / (float)oldsize : 1.0f;
		for (auto& s : sp->_splits)
			s *= scale;

		sp->_splits.Resize(cc);
		for (size_t i = oldsize; i < cc; i++)
			sp->_splits[i] = (i + 1.0f) / (cc + 1.0f);
	}
	else if (oldsize > cc)
	{
		sp->_splits.Resize(cc);
		// rescale to fit
		float scale = cc ? oldsize / (float)cc : 1.0f;
		for (auto& s : sp->_splits)
			s *= scale;
	}
}

static void ClampSplit(SplitPane* sp, size_t which)
{
	// TODO config option
	float minPartSizePx = 4;

	bool v = sp->_verticalSplit;
	auto r = sp->GetFinalRect();

	// no layout yet or broken layout
	if (sp->_verticalSplit)
	{
		if (r.y0 >= r.y1)
			return;
	}
	else
	{
		if (r.x0 >= r.x1)
			return;
	}

	float minPos = !v ? r.x0 : r.y0;
	float maxPos = !v ? r.x1 : r.y1;

	if (which > 0)
	{
		if (!v)
			minPos = SplitQToX(sp, sp->_splits[which - 1]) + sp->vertSepStyle.size / 2;
		else
			minPos = SplitQToY(sp, sp->_splits[which - 1]) + sp->horSepStyle.size / 2;
	}
	if (which + 1 < sp->_splits.size())
	{
		if (!v)
			maxPos = SplitQToX(sp, sp->_splits[which + 1]) - sp->vertSepStyle.size / 2;
		else
			maxPos = SplitQToY(sp, sp->_splits[which + 1]) - sp->horSepStyle.size / 2;
	}

	minPos += minPartSizePx;
	maxPos -= minPartSizePx;

	auto& S = sp->_splits[which];
	if (!v)
		S = SplitXToQ(sp, clamp(SplitQToX(sp, S), minPos, maxPos));
	else
		S = SplitYToQ(sp, clamp(SplitQToY(sp, S), minPos, maxPos));
}

void SplitPane::OnPaint(const UIPaintContext& ctx)
{
	if (_children.NotEmpty())
	{
		size_t split = 0;
		if (!_verticalSplit)
		{
			float splitWidth = vertSepStyle.size;
			float prevEdge = GetFinalRect().x0;
			for (auto* ch : _children)
			{
				UIRect r = GetFinalRect();
				auto sr = split < _splits.size() ? GetSplitRectH(this, split) : UIRect{ r.x1, 0, r.x1, 0 };
				split++;
				r.x0 = prevEdge;
				r.x1 = sr.x0;

				if (draw::PushScissorRectIfNotEmpty(r))
				{
					ch->Paint(ctx);
					draw::PopScissorRect();
				}

				prevEdge = sr.x1;
			}
		}
		else
		{
			float splitHeight = horSepStyle.size;
			float prevEdge = GetFinalRect().y0;
			for (auto* ch : _children)
			{
				UIRect r = GetFinalRect();
				auto sr = split < _splits.size() ? GetSplitRectV(this, split) : UIRect{ 0, r.y1, 0, r.y1 };
				split++;
				r.y0 = prevEdge;
				r.y1 = sr.y0;

				if (draw::PushScissorRectIfNotEmpty(r))
				{
					ch->Paint(ctx);
					draw::PopScissorRect();
				}

				prevEdge = sr.y1;
			}
		}
	}

	PaintInfo info(this);
	if (!_verticalSplit)
	{
		if (vertSepStyle.backgroundPainter)
		{
			for (size_t i = 0; i < _splits.size(); i++)
			{
				auto ii = (uint16_t)i;
				info.rect = GetSplitRectH(this, ii);
				info.state &= ~(PS_Hover | PS_Down);
				if (_splitUI.IsHovered(ii))
					info.state |= PS_Hover;
				if (_splitUI.IsPressed(ii))
					info.state |= PS_Down;
				vertSepStyle.backgroundPainter->Paint(info);
			}
		}
	}
	else
	{
		if (horSepStyle.backgroundPainter)
		{
			for (size_t i = 0; i < _splits.size(); i++)
			{
				auto ii = (uint16_t)i;
				info.rect = GetSplitRectV(this, ii);
				info.state &= ~(PS_Hover | PS_Down);
				if (_splitUI.IsHovered(ii))
					info.state |= PS_Hover;
				if (_splitUI.IsPressed(ii))
					info.state |= PS_Down;
				horSepStyle.backgroundPainter->Paint(info);
			}
		}
	}
}

void SplitPane::OnEvent(Event& e)
{
	// event while building, understanding of children is incomplete
	if (UIContainer::GetCurrent() &&
		GetCurrentBuildable())
		return;

	CheckSplits(this);

	_splitUI.InitOnEvent(e);
	for (size_t i = 0; i < _splits.size(); i++)
	{
		if (!_verticalSplit)
		{
			switch (_splitUI.DragOnEvent(uint16_t(i), GetSplitRectH(this, i), e))
			{
			case SubUIDragState::Start:
				e.context->CaptureMouse(this);
				_dragOff = SplitQToX(this, _splits[i]) - e.position.x;
				break;
			case SubUIDragState::Move:
				_splits[i] = SplitXToQ(this, e.position.x + _dragOff);
				ClampSplit(this, i);
				_OnChangeStyle();
				{
					Event ev(e.context, this, EventType::Change);
					ev.arg0 = i;
					e.context->BubblingEvent(ev);
				}
				break;
			case SubUIDragState::Stop:
				if (e.context->GetMouseCapture() == this)
					e.context->ReleaseMouse();
				{
					Event ev(e.context, this, EventType::Commit);
					ev.arg0 = i;
					e.context->BubblingEvent(ev);
				}
				break;
			}
		}
		else
		{
			switch (_splitUI.DragOnEvent(uint16_t(i), GetSplitRectV(this, i), e))
			{
			case SubUIDragState::Start:
				e.context->CaptureMouse(this);
				_dragOff = SplitQToY(this, _splits[i]) - e.position.y;
				break;
			case SubUIDragState::Move:
				_splits[i] = SplitYToQ(this, e.position.y + _dragOff);
				ClampSplit(this, i);
				_OnChangeStyle();
				{
					Event ev(e.context, this, EventType::Change);
					ev.arg0 = i;
					e.context->BubblingEvent(ev);
				}
				break;
			case SubUIDragState::Stop:
				if (e.context->GetMouseCapture() == this)
					e.context->ReleaseMouse();
				{
					Event ev(e.context, this, EventType::Commit);
					ev.arg0 = i;
					e.context->BubblingEvent(ev);
				}
				break;
			}
		}
	}
	if (e.type == EventType::SetCursor && _splitUI.IsAnyHovered())
	{
		e.context->SetDefaultCursor(_verticalSplit ? DefaultCursor::ResizeRow : DefaultCursor::ResizeCol);
		e.StopPropagation();
	}
	_splitUI.FinalizeOnEvent(e);
}

Rangef SplitPane::CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type)
{
	return Rangef::Exact(containerSize.x);
}

Rangef SplitPane::CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type)
{
	return Rangef::Exact(containerSize.y);
}

void SplitPane::OnLayout(const UIRect& rect, LayoutInfo info)
{
	_finalRect = rect;

	CheckSplits(this);
	for (size_t i = 0; i < _splits.size(); i++)
		ClampSplit(this, i);

	size_t split = 0;
	if (!_verticalSplit)
	{
		float splitWidth = vertSepStyle.size;
		float prevEdge = GetFinalRect().x0;
		for (auto* ch : _children)
		{
			UIRect r = GetFinalRect();
			auto sr = split < _splits.size() ? GetSplitRectH(this, split) : UIRect{ r.x1, 0, r.x1, 0 };
			split++;
			r.x0 = prevEdge;
			r.x1 = sr.x0;
			ch->PerformLayout(r, info);
			prevEdge = sr.x1;
		}
	}
	else
	{
		float splitHeight = horSepStyle.size;
		float prevEdge = GetFinalRect().y0;
		for (auto* ch : _children)
		{
			UIRect r = GetFinalRect();
			auto sr = split < _splits.size() ? GetSplitRectV(this, split) : UIRect{ 0, r.y1, 0, r.y1 };
			split++;
			r.y0 = prevEdge;
			r.y1 = sr.y0;
			ch->PerformLayout(r, info);
			prevEdge = sr.y1;
		}
	}
}

void SplitPane::SlotIterator_Init(UIObjectIteratorData& data)
{
	data.data0 = 0;
}

UIObject* SplitPane::SlotIterator_GetNext(UIObjectIteratorData& data)
{
	if (data.data0 >= _children.size())
		return nullptr;
	return _children[data.data0++];
}

void SplitPane::RemoveChildImpl(UIObject* ch)
{
	for (size_t i = 0; i < _children.size(); i++)
	{
		if (_children[i] == ch)
		{
			_children.RemoveAt(i);
			break;
		}
	}
}

void SplitPane::DetachChildren(bool recursive)
{
	for (size_t i = 0; i < _children.size(); i++)
	{
		auto* ch = _children[i];

		if (recursive)
			ch->DetachChildren(true);

		// if ch->system != 0 then system != 0 but the latter should be a more predictable branch
		if (system)
			ch->_DetachFromTree();

		ch->parent = nullptr;
	}
	_children.Clear();
}

void SplitPane::AppendChild(UIObject* obj)
{
	obj->DetachParent();

	obj->parent = this;
	_children.Append(obj);

	if (system)
		obj->_AttachToFrameContents(system);
}

UIObject* SplitPane::FindObjectAtPoint(Point2f pos)
{
	for (size_t i = _children.size(); i > 0; )
	{
		i--;
		if (_children[i]->Contains(pos))
			if (auto* o = _children[i]->FindObjectAtPoint(pos))
				return o;
	}
	if (!(flags & UIObject_HitTestPassthrough) && Contains(pos))
		return this;
	return nullptr;
}

void SplitPane::_AttachToFrameContents(FrameContents* owner)
{
	UIObject::_AttachToFrameContents(owner);

	for (auto* ch : _children)
		ch->_AttachToFrameContents(owner);
}

void SplitPane::_DetachFromFrameContents()
{
	for (auto* ch : _children)
		ch->_DetachFromFrameContents();

	UIObject::_DetachFromFrameContents();
}

SplitPane& SplitPane::SetDirection(Direction d)
{
	_verticalSplit = d == Direction::Vertical;
	_OnChangeStyle();
	return *this;
}

SplitPane& SplitPane::SetSplitPos(int which, float pos)
{
	if (which < 0)
		return *this;
	CheckSplits(this, which);
	_splits[which] = pos;
	ClampSplit(this, which);
	_OnChangeStyle();
	return *this;
}

float SplitPane::GetSplitPos(int which) const
{
	if (which < 0 || size_t(which) >= _splits.size())
		return 0;
	return _splits[which];
}

SplitPane& SplitPane::SetSplits(const float* splits, size_t numSplits)
{
	_splits.AssignMany(splits, numSplits);
	for (size_t i = 0; i < _splits.size(); i++)
		ClampSplit(this, i);
	_OnChangeStyle();
	return *this;
}

SplitPane& SplitPane::Init(Direction d, float* splits, size_t numSplits)
{
	SetDirection(d);
	SetSplits(splits, numSplits);
	HandleEvent(this, EventType::Change) = [this, splits](ui::Event& e)
	{
		splits[e.arg0] = GetSplitPos(e.arg0);
	};
	return *this;
}

} // ui
