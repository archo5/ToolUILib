
#include "Controls.h"
#include "Layout.h"
#include "System.h"
#include "Theme.h"


namespace ui {

Panel::Panel()
{
	styleProps = Theme::current->panel;
}


Button::Button()
{
	styleProps = Theme::current->button;
}

void Button::OnEvent(UIEvent& e)
{
	if (e.type == UIEventType::ButtonDown)
	{
		system->eventSystem.SetKeyboardFocus(this);
	}
	if (e.type == UIEventType::Activate)
	{
		if (IsInputDisabled())
		{
			e.handled = true;
			return;
		}
		if (onClick)
			onClick();
	}
}


void CheckableBase::OnPaint()
{
	style::PaintInfo info(this);
	if (IsSelected())
		info.state |= style::PS_Checked;
	styleProps->paint_func(info);

	PaintChildren();
}

void CheckableBase::OnEvent(UIEvent& e)
{
	if (e.type == UIEventType::ButtonDown)
	{
		system->eventSystem.SetKeyboardFocus(this);
	}
	if (e.type == UIEventType::Activate)
	{
		if (IsInputDisabled())
		{
			e.handled = true;
			return;
		}
		OnSelect();
		if (onChange)
			onChange();
		e.context->OnChange(this);
	}
}


CheckboxBase::CheckboxBase()
{
	styleProps = Theme::current->checkbox;
}


RadioButtonBase::RadioButtonBase()
{
	styleProps = Theme::current->radioButton;
}


ListBox::ListBox()
{
	styleProps = Theme::current->listBox;
}


void SelectableBase::OnPaint()
{
	UIElement::OnPaint();
}

void SelectableBase::OnEvent(UIEvent& e)
{
	UIElement::OnEvent(e);
}


Selectable::Selectable()
{
	styleProps = Theme::current->button;
}


ProgressBar::ProgressBar()
{
	styleProps = Theme::current->progressBarBase;
	completionBarStyle = Theme::current->progressBarCompletion;
}

void ProgressBar::OnPaint()
{
	styleProps->paint_func(this);

	style::PaintInfo cinfo(this);
	cinfo.rect = cinfo.rect.ShrinkBy(GetMarginRect(completionBarStyle, cinfo.rect.GetWidth()));
	cinfo.rect.x1 = lerp(cinfo.rect.x0, cinfo.rect.x1, progress);
	completionBarStyle->paint_func(cinfo);

	PaintChildren();
}


Slider::Slider()
{
	styleProps = Theme::current->sliderHBase;
	trackStyle = Theme::current->sliderHTrack;
	trackFillStyle = Theme::current->sliderHTrackFill;
	thumbStyle = Theme::current->sliderHThumb;
}

void Slider::OnPaint()
{
	styleProps->paint_func(this);

	style::PaintInfo trkinfo(this);
	float w = trkinfo.rect.GetWidth();
	trkinfo.rect = trkinfo.rect.ShrinkBy(GetMarginRect(trackStyle, w));
	trackStyle->paint_func(trkinfo);

	if (valuePtr)
	{
		trkinfo.rect = trkinfo.rect.ShrinkBy(GetPaddingRect(trackStyle, w));
		trkinfo.rect.x1 = lerp(trkinfo.rect.x0, trkinfo.rect.x1, ValueToQ(*valuePtr));
		auto prethumb = trkinfo.rect;

		trkinfo.rect = trkinfo.rect.ExtendBy(GetPaddingRect(trackFillStyle, w));
		trackFillStyle->paint_func(trkinfo);

		prethumb.x0 = prethumb.x1;
		trkinfo.rect = prethumb.ExtendBy(GetPaddingRect(thumbStyle, w));
		thumbStyle->paint_func(trkinfo);
	}

	PaintChildren();
}

void Slider::OnEvent(UIEvent& e)
{
	if (e.type == UIEventType::ButtonDown && e.GetButton() == UIMouseButton::Left)
	{
		_mxoff = 0;
		// TODO if inside, calc offset
	}
	if (e.type == UIEventType::MouseMove && IsClicked())
	{
		if (valuePtr)
			*valuePtr = PosToValue(e.x + _mxoff);
	}
}

Slider* Slider::Init(float* vp, double vmin, double vmax, double step)
{
	valuePtr = vp;
	minValue = vmin;
	maxValue = vmax;
	trackStep = step;
	return this;
}

float Slider::PosToQ(float x)
{
	auto rect = GetPaddingRect();
	float w = rect.GetWidth();
	rect = rect.ShrinkBy(GetMarginRect(trackStyle, w));
	rect = rect.ShrinkBy(GetPaddingRect(trackStyle, w));
	float fw = rect.GetWidth();
	return fw ? (x - rect.x0) / fw : 0;
}

double Slider::QToValue(float q)
{
	if (q < 0)
		q = 0;
	else if (q > 1)
		q = 1;

	double v = minValue * (1 - q) + maxValue * q;
	if (trackStep)
	{
		v = round((v - minValue) / trackStep) * trackStep + minValue;

		if (v > maxValue + 0.001 * (maxValue - minValue))
			v -= trackStep;
		if (v < minValue)
			v = minValue;
	}

	return v;
}

float Slider::ValueToQ(double v)
{
	double diff = maxValue - minValue;
	return diff ? float((v - minValue) / diff) : 0.0f;
}


ScrollArea::ScrollArea()
{
	trackVStyle = Theme::current->scrollVTrack;
	thumbVStyle = Theme::current->scrollVThumb;
}

void ScrollArea::OnPaint()
{
	styleProps->paint_func(this);

	PaintChildren();

	float w = GetContentRect().GetWidth();
	style::PaintInfo vsinfo(this);
	vsinfo.rect.x0 = vsinfo.rect.x1 - ResolveUnits(trackVStyle->width, w);
	vsinfo.state &= ~style::PS_Down;
	vsinfo.state &= ~style::PS_Hover;
	trackVStyle->paint_func(vsinfo);
	vsinfo.rect = vsinfo.rect.ShrinkBy(GetPaddingRect(trackVStyle, w));
	vsinfo.rect.y0 += 10;
	vsinfo.rect.y1 -= 10;
	thumbVStyle->paint_func(vsinfo);
}

void ScrollArea::OnEvent(UIEvent& e)
{
	if (e.type == UIEventType::MouseScroll)
	{
		yoff -= e.dy / 4;
	}
}

void ScrollArea::OnLayout(const UIRect& rect)
{
	UIRect r = rect;
	r.y0 -= yoff;
	r.y1 -= yoff;

	UIElement::OnLayout(r);

	finalRectC.y0 += yoff;
	finalRectC.y1 += yoff;
	finalRectCP.y0 += yoff;
	finalRectCP.y1 += yoff;
	finalRectCPB.y0 += yoff;
	finalRectCPB.y1 += yoff;
}


TabGroup::TabGroup()
{
	styleProps = Theme::current->tabGroup;
}

void TabGroup::OnInit()
{
	_curButton = 0;
	_curPanel = 0;
}

void TabGroup::OnPaint()
{
	styleProps->paint_func(this);

	for (UIObject* ch = firstChild; ch; ch = ch->next)
	{
		if (auto* p = dynamic_cast<TabPanel*>(ch))
			if (p->id != active)
				continue;
		ch->OnPaint();
	}
}


TabList::TabList()
{
	styleProps = Theme::current->tabList;
}

void TabList::OnPaint()
{
	styleProps->paint_func(this);

	if (auto* g = FindParentOfType<TabGroup>())
	{
		for (UIObject* ch = firstChild; ch; ch = ch->next)
		{
			if (auto* p = dynamic_cast<TabButton*>(ch))
				if (p->id == g->active)
					continue;
			ch->OnPaint();
		}
	}
	else
		PaintChildren();
}


TabButton::TabButton()
{
	styleProps = Theme::current->tabButton;
}

void TabButton::OnInit()
{
	if (auto* g = FindParentOfType<TabGroup>())
	{
		id = g->_curButton++;
		if (id == g->active)
			g->_activeBtn = this;
	}
}

void TabButton::OnDestroy()
{
	if (auto* g = FindParentOfType<TabGroup>())
		if (g->_activeBtn == this)
			g->_activeBtn = nullptr;
}

void TabButton::OnPaint()
{
	style::PaintInfo info(this);
	if (FindParentOfType<TabGroup>()->active == id)
		info.state |= style::PS_Checked;
	styleProps->paint_func(info);
	PaintChildren();
}

void TabButton::OnEvent(UIEvent& e)
{
	if ((e.type == UIEventType::Activate || e.type == UIEventType::ButtonDown) && IsChildOrSame(e.GetTargetNode()))
	{
		if (auto* g = FindParentOfType<TabGroup>())
		{
			g->active = id;
			g->_activeBtn = this;
			g->RerenderNode();
		}
	}
}


TabPanel::TabPanel()
{
	styleProps = Theme::current->tabPanel;
}

void TabPanel::OnInit()
{
	if (auto* g = FindParentOfType<TabGroup>())
		id = g->_curPanel++;
}

void TabPanel::OnPaint()
{
	styleProps->paint_func(this);

	if (auto* g = FindParentOfType<TabGroup>())
		if (g->_activeBtn)
			g->_activeBtn->Paint();

	PaintChildren();
}


Textbox::Textbox()
{
	styleProps = Theme::current->textBoxBase;
}

void Textbox::OnPaint()
{
	// background
	styleProps->paint_func(this);

	{
		auto r = GetContentRect();
		DrawTextLine(r.x0, r.y1 - (r.y1 - r.y0 - GetFontHeight()) / 2, text.c_str(), 1, 1, 1);

		if (IsFocused())
		{
			if (startCursor != endCursor)
			{
				int minpos = startCursor < endCursor ? startCursor : endCursor;
				int maxpos = startCursor > endCursor ? startCursor : endCursor;
				float x0 = GetTextWidth(text.c_str(), minpos);
				float x1 = GetTextWidth(text.c_str(), maxpos);

				GL::SetTexture(0);
				GL::BatchRenderer br;
				br.Begin();
				br.SetColor(0.5f, 0.7f, 0.9f, 0.4f);
				br.Quad(r.x0 + x0, r.y0, r.x0 + x1, r.y1, 0, 0, 1, 1);
				br.End();
			}

			if (showCaretState)
			{
				float x = GetTextWidth(text.c_str(), endCursor);
				GL::SetTexture(0);
				GL::BatchRenderer br;
				br.Begin();
				br.SetColor(1, 1, 1);
				br.Quad(r.x0 + x, r.y0, r.x0 + x + 1, r.y1, 0, 0, 1, 1);
				br.End();
			}
		}
	}

	PaintChildren();
}

void Textbox::OnEvent(UIEvent& e)
{
	if (e.type == UIEventType::ButtonDown)
	{
		if (e.GetButton() == UIMouseButton::Left)
			startCursor = endCursor = _FindCursorPos(e.x);
		system->eventSystem.SetKeyboardFocus(this);
	}
	else if (e.type == UIEventType::ButtonUp)
	{
		if (e.GetButton() == UIMouseButton::Left)
			endCursor = _FindCursorPos(e.x);
	}
	else if (e.type == UIEventType::MouseMove)
	{
		if (IsClicked(0))
			endCursor = _FindCursorPos(e.x);
	}
	else if (e.type == UIEventType::Timer)
	{
		showCaretState = !showCaretState;
		if (IsFocused())
			system->eventSystem.SetTimer(this, 0.5f);
	}
	else if (e.type == UIEventType::GotFocus)
	{
		showCaretState = true;
		system->eventSystem.SetTimer(this, 0.5f);
	}
	else if (e.type == UIEventType::LostFocus)
	{
		system->eventSystem.OnCommit(this);
	}
	else if (e.type == UIEventType::KeyAction)
	{
		switch (e.GetKeyAction())
		{
		case UIKeyAction::Enter:
			system->eventSystem.SetKeyboardFocus(nullptr);
			break;
		case UIKeyAction::Backspace:
			if (IsLongSelection())
				EraseSelection();
			else if (endCursor > 0)
				text.erase(startCursor = --endCursor, 1); // TODO unicode
			system->eventSystem.OnChange(this);
			break;
		case UIKeyAction::Delete:
			if (IsLongSelection())
				EraseSelection();
			else if (endCursor + 1 < text.size())
				text.erase(endCursor, 1); // TODO unicode
			system->eventSystem.OnChange(this);
			break;

		case UIKeyAction::Left:
			if (IsLongSelection())
				startCursor = endCursor = startCursor < endCursor ? startCursor : endCursor;
			else if (endCursor > 0)
				startCursor = --endCursor;
			break;
		case UIKeyAction::Right:
			if (IsLongSelection())
				startCursor = endCursor = startCursor > endCursor ? startCursor : endCursor;
			else if (endCursor < text.size())
				startCursor = ++endCursor;
			break;
		case UIKeyAction::Home:
			startCursor = endCursor = 0;
			break;
		case UIKeyAction::End:
			startCursor = endCursor = text.size();
			break;

		case UIKeyAction::SelectAll:
			startCursor = 0;
			endCursor = text.size();
			break;
		}
	}
	else if (e.type == UIEventType::TextInput)
	{
		char ch[5];
		if (e.GetUTF32Char() >= 32 && e.GetUTF8Text(ch))
			EnterText(ch);
	}
}

void Textbox::EnterText(const char* str)
{
	EraseSelection();
	size_t num = strlen(str);
	text.insert(endCursor, str, num);
	startCursor = endCursor += num;
	system->eventSystem.OnChange(this);
}

void Textbox::EraseSelection()
{
	if (IsLongSelection())
	{
		int min = startCursor < endCursor ? startCursor : endCursor;
		int max = startCursor > endCursor ? startCursor : endCursor;
		text.erase(min, max - min);
		startCursor = endCursor = min;
	}
}

int Textbox::_FindCursorPos(float vpx)
{
	auto r = GetContentRect();
	// TODO kerning
	float x = r.x0;
	for (size_t i = 0; i < text.size(); i++)
	{
		float lw = GetTextWidth(&text[i], 1);
		if (vpx < x + lw * 0.5f)
			return i;
		x += lw;
	}
	return text.size();
}


CollapsibleTreeNode::CollapsibleTreeNode()
{
	styleProps = Theme::current->collapsibleTreeNode;
}

void CollapsibleTreeNode::OnPaint()
{
	style::PaintInfo info(this);
	info.state &= ~style::PS_Hover;
	if (_hovered)
		info.state |= style::PS_Hover;
	if (open)
		info.state |= style::PS_Checked;
	styleProps->paint_func(info);
	PaintChildren();
}

void CollapsibleTreeNode::OnEvent(UIEvent& e)
{
	if (e.type == UIEventType::MouseEnter || e.type == UIEventType::MouseMove)
	{
		auto r = GetPaddingRect();
		float h = GetFontHeight();
		_hovered = e.x >= r.x0 && e.x < r.x0 + h && e.y >= r.y0 && e.y < r.y0 + h;
	}
	if (e.type == UIEventType::MouseLeave)
	{
		_hovered = false;
	}
	if (e.type == UIEventType::ButtonDown && e.GetButton() == UIMouseButton::Left && _hovered)
	{
		open = !open;
		e.GetTargetNode()->Rerender();
	}
}


Table::Table()
{
	styleProps = Theme::current->tableBase;
	cellStyle = Theme::current->tableCell;
	rowHeaderStyle = Theme::current->tableRowHeader;
	colHeaderStyle = Theme::current->tableColHeader;
}

void Table::OnPaint()
{
	styleProps->paint_func(this);

	int rhw = 80;
	int chh = 20;
	int w = 100;
	int h = 20;
	
	int nc = _dataSource->GetNumCols();
	int nr = _dataSource->GetNumRows();

	style::PaintInfo info(this);

	auto RC = GetContentRect();

	// backgrounds
	for (int r = 0; r < nr; r++)
	{
		info.rect =
		{
			RC.x0,
			RC.y0 + chh + h * r,
			RC.x0 + rhw,
			RC.y0 + chh + h * (r + 1),
		};
		rowHeaderStyle->paint_func(info);
	}
	for (int c = 0; c < nc; c++)
	{
		info.rect =
		{
			RC.x0 + rhw + w * c,
			RC.y0,
			RC.x0 + rhw + w * (c + 1),
			RC.y0 + chh,
		};
		colHeaderStyle->paint_func(info);
	}
	for (int r = 0; r < nr; r++)
	{
		for (int c = 0; c < nc; c++)
		{
			info.rect =
			{
				RC.x0 + rhw + w * c,
				RC.y0 + chh + h * r,
				RC.x0 + rhw + w * (c + 1),
				RC.y0 + chh + h * (r + 1),
			};
			cellStyle->paint_func(info);
		}
	}

	// text
	for (int r = 0; r < nr; r++)
	{
		UIRect rect =
		{
			RC.x0,
			RC.y0 + chh + h * r,
			RC.x0 + rhw,
			RC.y0 + chh + h * (r + 1),
		};
		DrawTextLine(rect.x0, (rect.y0 + rect.y1 + GetFontHeight()) / 2, _dataSource->GetRowName(r).c_str(), 1, 1, 1);
	}
	for (int c = 0; c < nc; c++)
	{
		UIRect rect =
		{
			RC.x0 + rhw + w * c,
			RC.y0,
			RC.x0 + rhw + w * (c + 1),
			RC.y0 + chh,
		};
		DrawTextLine(rect.x0, (rect.y0 + rect.y1 + GetFontHeight()) / 2, _dataSource->GetColName(c).c_str(), 1, 1, 1);
	}
	for (int r = 0; r < nr; r++)
	{
		for (int c = 0; c < nc; c++)
		{
			UIRect rect =
			{
				RC.x0 + rhw + w * c,
				RC.y0 + chh + h * r,
				RC.x0 + rhw + w * (c + 1),
				RC.y0 + chh + h * (r + 1),
			};
			DrawTextLine(rect.x0, (rect.y0 + rect.y1 + GetFontHeight()) / 2, _dataSource->GetText(r, c).c_str(), 1, 1, 1);
		}
	}

	PaintChildren();
}

void Table::OnEvent(UIEvent& e)
{
}

void Table::Render(UIContainer* ctx)
{
}

void Table::SetDataSource(TableDataSource* src)
{
	_dataSource = src;
	Rerender();
}

} // ui
