
#include "pch.h"
#include "HexViewer.h"


void HighlightSettings::EditUI(UIContainer* ctx)
{
	ui::imm::PropEditBool(ctx, "Exclude zeroes", excludeZeroes);

	ui::Property::Begin(ctx, "float32");
	ui::imm::PropEditBool(ctx, nullptr, enableFloat32);
	ui::imm::PropEditFloat(ctx, "\bMin", minFloat32, {}, 0.01f);
	ui::imm::PropEditFloat(ctx, "\bMax", maxFloat32);
	ui::Property::End(ctx);

	ui::Property::Begin(ctx, "int16");
	ui::imm::PropEditBool(ctx, nullptr, enableInt16);
	ui::imm::PropEditInt(ctx, "\bMin", minInt16);
	ui::imm::PropEditInt(ctx, "\bMax", maxInt16);
	ui::Property::End(ctx);

	ui::Property::Begin(ctx, "int32");
	ui::imm::PropEditBool(ctx, nullptr, enableInt32);
	ui::imm::PropEditInt(ctx, "\bMin", minInt32);
	ui::imm::PropEditInt(ctx, "\bMax", maxInt32);
	ui::Property::End(ctx);

	ui::Property::Begin(ctx, "ASCII");
	ui::imm::PropEditInt(ctx, "\bMin chars", minASCIIChars, {}, 1, 1, 128);
	ui::Property::End(ctx);
}


Color4f Highlighter::GetByteTypeBin(uint64_t basePos, uint8_t* buf, int at, int sz)
{
	Color4f col(0);
	if (IsInt16InRange(basePos, buf, at, sz)) col.BlendOver(colorInt16);
	if (IsInt16InRange(basePos, buf, at - 1, sz)) col.BlendOver(colorInt16);
	if (IsFloat32InRange(basePos, buf, at, sz)) col.BlendOver(colorFloat32);
	if (IsFloat32InRange(basePos, buf, at - 1, sz)) col.BlendOver(colorFloat32);
	if (IsFloat32InRange(basePos, buf, at - 2, sz)) col.BlendOver(colorFloat32);
	if (IsFloat32InRange(basePos, buf, at - 3, sz)) col.BlendOver(colorFloat32);
	if (IsInt32InRange(basePos, buf, at, sz)) col.BlendOver(colorInt32);
	if (IsInt32InRange(basePos, buf, at - 1, sz)) col.BlendOver(colorInt32);
	if (IsInt32InRange(basePos, buf, at - 2, sz)) col.BlendOver(colorInt32);
	if (IsInt32InRange(basePos, buf, at - 3, sz)) col.BlendOver(colorInt32);
	return col;
}

Color4f Highlighter::GetByteTypeASCII(uint64_t basePos, uint8_t* buf, int at, int sz)
{
	Color4f col(0);
	if (IsASCIIInRange(basePos, buf, at, sz)) col.BlendOver(colorASCII);
	return col;
}

bool Highlighter::IsInt16InRange(uint64_t basePos, uint8_t* buf, int at, int sz)
{
	if (!enableInt16 || at < 0 || sz - at < 2)
		return false;
	if (markerData->IsMarked(basePos + at, 2))
		return false;
	if (excludeZeroes && buf[at] == 0 && buf[at + 1] == 0)
		return false;
	int16_t v;
	memcpy(&v, &buf[at], 2);
	return v >= minInt16 && v <= maxInt16;
}

bool Highlighter::IsInt32InRange(uint64_t basePos, uint8_t* buf, int at, int sz)
{
	if (!enableInt32 || at < 0 || sz - at < 4)
		return false;
	if (markerData->IsMarked(basePos + at, 4))
		return false;
	if (excludeZeroes && buf[at] == 0 && buf[at + 1] == 0 && buf[at + 2] == 0 && buf[at + 3] == 0)
		return false;
	int32_t v;
	memcpy(&v, &buf[at], 4);
	return v >= minInt32 && v <= maxInt32;
}

bool Highlighter::IsFloat32InRange(uint64_t basePos, uint8_t* buf, int at, int sz)
{
	if (!enableFloat32 || at < 0 || sz - at < 4)
		return false;
	if (markerData->IsMarked(basePos + at, 4))
		return false;
	if (excludeZeroes && buf[at] == 0 && buf[at + 1] == 0 && buf[at + 2] == 0 && buf[at + 3] == 0)
		return false;
	float v;
	memcpy(&v, &buf[at], 4);
	return (v >= minFloat32 && v <= maxFloat32) || (v >= -maxFloat32 && v <= -minFloat32);
}

static bool IsASCII(uint8_t v)
{
	return v >= 0x20 && v < 0x7f;
}

bool Highlighter::IsASCIIInRange(uint64_t basePos, uint8_t* buf, int at, int sz)
{
	if (!IsASCII(buf[at]))
		return false;
	int min = at;
	int max = at;
	while (min - 1 >= 0 && max - min < minASCIIChars && IsASCII(buf[min - 1]))
		min--;
	while (max + 1 < sz && max - min < minASCIIChars && IsASCII(buf[max + 1]))
		max++;
	return max - min >= minASCIIChars;
}


void HexViewer::OnEvent(UIEvent& e)
{
	int W = *byteWidth;

	if (e.type == UIEventType::MouseMove)
	{
		float fh = GetFontHeight() + 4;
		float x = finalRectC.x0 + 2;
		float y = finalRectC.y0;
		float x2 = x + 20 * W + 10;

		hoverSection = -1;
		hoverByte = UINT64_MAX;
		if (e.x >= x && e.x < x + W * 20)
		{
			hoverSection = 0;
			int xpos = std::min(std::max(0, int((e.x - x) / 20)), W - 1);
			int ypos = (e.y - y) / fh;
			hoverByte = GetBasePos() + xpos + ypos * W;
		}
		else if (e.x >= x2 && e.x < x2 + W * 10)
		{
			hoverSection = 1;
			int xpos = std::min(std::max(0, int((e.x - x2) / 10)), W - 1);
			int ypos = (e.y - y) / fh;
			hoverByte = GetBasePos() + xpos + ypos * W;
		}
	}
	else if (e.type == UIEventType::MouseLeave)
	{
		hoverSection = -1;
		hoverByte = UINT64_MAX;
	}
	else if (e.type == UIEventType::ButtonDown && e.GetButton() == UIMouseButton::Right)
	{
		uint64_t pos = hoverByte;

		char txt_pos[64];
		snprintf(txt_pos, 32, "@ %" PRIu64 " (0x%" PRIX64 ")", pos, pos);

		char txt_int16[32];
		GetInt16Text(txt_int16, 32, pos, true);
		char txt_uint16[32];
		GetInt16Text(txt_uint16, 32, pos, false);
		char txt_int32[32];
		GetInt32Text(txt_int32, 32, pos, true);
		char txt_uint32[32];
		GetInt32Text(txt_uint32, 32, pos, false);
		char txt_float32[32];
		GetFloat32Text(txt_float32, 32, pos);

		ui::MenuItem items[] =
		{
			ui::MenuItem(txt_pos, {}, true),
			ui::MenuItem("Mark int16", txt_int16).Func([this, pos]() { highlighter->markerData->AddMarker(DT_I16, pos, pos + 2); }),
			ui::MenuItem("Mark uint16", txt_uint16).Func([this, pos]() { highlighter->markerData->AddMarker(DT_U16, pos, pos + 2); }),
			ui::MenuItem("Mark int32", txt_int32).Func([this, pos]() { highlighter->markerData->AddMarker(DT_I32, pos, pos + 4); }),
			ui::MenuItem("Mark uint32", txt_uint32).Func([this, pos]() { highlighter->markerData->AddMarker(DT_U32, pos, pos + 4); }),
			ui::MenuItem("Mark float32", txt_float32).Func([this, pos]() { highlighter->markerData->AddMarker(DT_F32, pos, pos + 4); }),
		};
		ui::Menu menu(items);
		menu.Show(this);
	}
	else if (e.type == UIEventType::MouseScroll)
	{
		int64_t diff = round(e.dy / 40) * 16;
		if (diff > 0 && diff > *basePos)
			*basePos = 0;
		else
			*basePos -= diff;
		*basePos = std::min(dataSource->GetSize() - 1, *basePos);
		RerenderNode();
	}
}

void HexViewer::OnPaint()
{
	int W = *byteWidth;

	uint8_t buf[256 * 64];
	size_t sz = dataSource->Read(GetBasePos(), W * 64, buf);

	float fh = GetFontHeight() + 4;
	float x = finalRectC.x0 + 2;
	float y = finalRectC.y0 + fh;
	float x2 = x + 20 * W + 10;

	GL::SetTexture(0);
	GL::BatchRenderer br;
	br.Begin();
	for (size_t i = 0; i < sz; i++)
	{
		uint8_t v = buf[i];
		Color4f col = highlighter->GetByteTypeBin(GetBasePos(), buf, i, sz);
		auto mc = highlighter->markerData->GetMarkedColor(GetBasePos() + i);
		if (hoverByte == GetBasePos() + i)
			col.BlendOver(colorHover);
		if (mc.a > 0)
			col.BlendOver(mc);
		if (col.a > 0)
		{
			float xoff = (i % W) * 20;
			float yoff = (i / W) * fh;
			br.SetColor(col.r, col.g, col.b, col.a);
			br.Quad(x + xoff - 2, y + yoff - fh + 4, x + xoff + 18, y + yoff + 3, 0, 0, 1, 1);
		}
		col = highlighter->GetByteTypeASCII(GetBasePos(), buf, i, sz);
		if (hoverByte == GetBasePos() + i)
			col.BlendOver(colorHover);
		if (mc.a > 0)
			col.BlendOver(mc);
		if (col.a > 0)
		{
			float xoff = (i % W) * 10;
			float yoff = (i / W) * fh;
			br.SetColor(col.r, col.g, col.b, col.a);
			br.Quad(x2 + xoff - 2, y + yoff - fh + 4, x2 + xoff + 8, y + yoff + 3, 0, 0, 1, 1);
		}
	}
	br.End();

	for (size_t i = 0; i < sz; i++)
	{
		uint8_t v = buf[i];
		char str[3];
		str[0] = "0123456789ABCDEF"[v >> 4];
		str[1] = "0123456789ABCDEF"[v & 0xf];
		str[2] = 0;
		float xoff = (i % W) * 20;
		float yoff = (i / W) * fh;
		DrawTextLine(x + xoff, y + yoff, str, 1, 1, 1);
		str[1] = IsASCII(v) ? v : '.';
		DrawTextLine(x2 + xoff / 2, y + yoff, str + 1, 1, 1, 1);
	}
}
