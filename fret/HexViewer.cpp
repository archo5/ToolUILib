
#include "pch.h"
#include "HexViewer.h"


void HighlightSettings::Load(const char* key, NamedTextSerializeReader& r)
{
	r.BeginDict(key);

	excludeZeroes = r.ReadBool("excludeZeroes");
	enableFloat32 = r.ReadBool("enableFloat32");
	minFloat32 = r.ReadFloat("minFloat32");
	maxFloat32 = r.ReadFloat("maxFloat32");
	enableInt16 = r.ReadBool("enableInt16");
	minInt16 = r.ReadInt("minInt16");
	maxInt16 = r.ReadInt("maxInt16");
	enableInt32 = r.ReadBool("enableInt32");
	minInt32 = r.ReadInt("minInt32");
	maxInt32 = r.ReadInt("maxInt32");
	minASCIIChars = r.ReadUInt("minASCIIChars");

	r.EndDict();
}

void HighlightSettings::Save(const char* key, NamedTextSerializeWriter& w)
{
	w.BeginDict(key);
	
	w.WriteBool("excludeZeroes", excludeZeroes);
	w.WriteBool("enableFloat32", enableFloat32);
	w.WriteFloat("minFloat32", minFloat32);
	w.WriteFloat("maxFloat32", maxFloat32);
	w.WriteBool("enableInt16", enableInt16);
	w.WriteInt("minInt16", minInt16);
	w.WriteInt("maxInt16", maxInt16);
	w.WriteBool("enableInt32", enableInt32);
	w.WriteInt("minInt32", minInt32);
	w.WriteInt("maxInt32", maxInt32);
	w.WriteInt("minASCIIChars", minASCIIChars);

	w.EndDict();
}

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


static bool IsASCII(uint8_t v)
{
	return v >= 0x20 && v < 0x7f;
}

static void Highlight(HighlightSettings* hs, DataDesc* desc, DDFile* file, uint64_t basePos, ByteColors* outColors, uint8_t* bytes, size_t numBytes)
{
	for (auto& M : file->markerData.markers)
	{
		uint64_t start = M.at;
		uint64_t end = M.GetEnd();
		size_t ss = start > basePos ? start - basePos : 0;
		size_t se = end > basePos ? end - basePos : 0;
		if (ss > numBytes)
			ss = numBytes;
		if (se > numBytes)
			se = numBytes;
		for (size_t i = ss; i < se; i++)
		{
			if (unsigned flags = M.ContainInfo(basePos + i))
			{
				auto mc = M.GetColor();
				auto& oc = outColors[i];
				oc.asciiColor.BlendOver(mc);
				oc.hexColor.BlendOver(mc);
				Color4f mc2 = { sqrtf(mc.r), sqrtf(mc.g), sqrtf(mc.b), sqrtf(mc.a) };
				if (flags & 2)
					oc.leftBracketColor.BlendOver(mc2);
				if (flags & 4)
					oc.rightBracketColor.BlendOver(mc2);
			}
		}
	}

	for (auto& SI : desc->instances)
	{
		if (SI.file != file)
			continue;
		if (SI.off >= int64_t(basePos) && SI.off < int64_t(basePos + numBytes))
		{
			outColors[SI.off - basePos].leftBracketColor.BlendOver(&SI == &desc->instances[desc->curInst] ? colorCurInst : colorInst);
		}
	}

	// auto highlights
	if (hs->enableFloat32 || hs->enableInt32)
	{
		for (size_t i = 0; i + 4 < numBytes; i++)
		{
			if (outColors[i].hexColor.a ||
				outColors[i + 1].hexColor.a ||
				outColors[i + 2].hexColor.a ||
				outColors[i + 3].hexColor.a)
				continue;
			if (hs->excludeZeroes && bytes[i] == 0 && bytes[i + 1] == 0 && bytes[i + 2] == 0 && bytes[i + 3] == 0)
				continue;

			if (hs->enableFloat32)
			{
				float v;
				memcpy(&v, &bytes[i], 4);
				if ((v >= hs->minFloat32 && v <= hs->maxFloat32) || (v >= -hs->maxFloat32 && v <= -hs->minFloat32))
				{
					for (int j = 0; j < 4; j++)
						outColors[i + j].hexColor.BlendOver(colorFloat32);
				}
			}

			if (hs->enableInt32)
			{
				int32_t v;
				memcpy(&v, &bytes[i], 4);
				if (v >= hs->minInt32 && v <= hs->maxInt32)
				{
					for (int j = 0; j < 4; j++)
						outColors[i + j].hexColor.BlendOver(colorInt32);
				}
			}
		}
	}

	if (hs->enableInt16)
	{
		for (size_t i = 0; i + 2 < numBytes; i++)
		{
			if (outColors[i].hexColor.a ||
				outColors[i + 1].hexColor.a)
				continue;
			if (hs->excludeZeroes && bytes[i] == 0 && bytes[i + 1] == 0)
				continue;

			int16_t v;
			memcpy(&v, &bytes[i], 2);
			if (v >= hs->minInt16 && v <= hs->maxInt16)
			{
				outColors[i].hexColor.BlendOver(colorInt32);
				outColors[i + 1].hexColor.BlendOver(colorInt32);
			}
		}
	}

	if (hs->minASCIIChars > 0)
	{
		size_t start = SIZE_MAX;
		bool prev = false;
		for (size_t i = 0; i < numBytes; i++)
		{
			bool cur = outColors[i].asciiColor.a == 0 && IsASCII(bytes[i]);
			if (cur && !prev)
				start = i;
			else if (prev && !cur && i - start >= hs->minASCIIChars)
			{
				for (size_t j = start; j < i; j++)
					outColors[j].asciiColor.BlendOver(colorASCII);
			}
			prev = cur;
		}
		if (prev && numBytes - start >= hs->minASCIIChars)
		{
			for (size_t j = start; j < numBytes; j++)
				outColors[j].asciiColor.BlendOver(colorASCII);
		}
	}
}


void HexViewer::OnEvent(UIEvent& e)
{
	int W = state->byteWidth;

	if (e.type == UIEventType::ButtonDown)
	{
		if (e.GetButton() == UIMouseButton::Left)
		{
			state->mouseDown = true;
			state->selectionStart = state->selectionEnd = state->hoverByte;
			RerenderNode();
		}
	}
	else if (e.type == UIEventType::ButtonUp)
	{
		if (e.GetButton() == UIMouseButton::Left)
			state->mouseDown = false;
	}
	else if (e.type == UIEventType::MouseMove)
	{
		float fh = GetFontHeight() + 4;
		float x = finalRectC.x0 + 2 + GetTextWidth("0") * 8;
		float y = finalRectC.y0 + fh;
		float x2 = x + 20 * W + 10;

		state->hoverSection = -1;
		state->hoverByte = UINT64_MAX;
		if (e.y >= y && e.x >= x && e.x < x + W * 20)
		{
			state->hoverSection = 0;
			int xpos = std::min(std::max(0, int((e.x - x) / 20)), W - 1);
			int ypos = (e.y - y) / fh;
			state->hoverByte = GetBasePos() + xpos + ypos * W;
		}
		else if (e.y >= y && e.x >= x2 && e.x < x2 + W * 10)
		{
			state->hoverSection = 1;
			int xpos = std::min(std::max(0, int((e.x - x2) / 10)), W - 1);
			int ypos = (e.y - y) / fh;
			state->hoverByte = GetBasePos() + xpos + ypos * W;
		}
		if (state->mouseDown)
		{
			state->selectionEnd = state->hoverByte;
		}
		RerenderNode();
	}
	else if (e.type == UIEventType::MouseLeave)
	{
		state->hoverSection = -1;
		state->hoverByte = UINT64_MAX;
		RerenderNode();
	}
	else if (e.type == UIEventType::MouseScroll)
	{
		int64_t diff = round(e.dy / 40) * 16;
		if (diff > 0 && diff > state->basePos)
			state->basePos = 0;
		else
			state->basePos -= diff;
		state->basePos = std::min(file->dataSource->GetSize() - 1, state->basePos);
		RerenderNode();
	}
}

void HexViewer::OnPaint()
{
	int W = state->byteWidth;

	auto minSel = std::min(state->selectionStart, state->selectionEnd);
	auto maxSel = std::max(state->selectionStart, state->selectionEnd);

	uint8_t buf[256 * 64];
	static ByteColors bcol[256 * 64];
	memset(&bcol, 0, sizeof(bcol));
	size_t sz = file->dataSource->Read(GetBasePos(), W * 64, buf);

	float fh = GetFontHeight() + 4;
	float x = finalRectC.x0 + 2 + GetTextWidth("0") * 8;
	float y = finalRectC.y0 + fh * 2;
	float x2 = x + 20 * W + 10;

	Highlight(highlightSettings, dataDesc, file, state->basePos, &bcol[0], buf, sz);

	GL::SetTexture(0);
	GL::BatchRenderer br;
	br.Begin();
	for (size_t i = 0; i < sz; i++)
	{
		uint8_t v = buf[i];
		auto pos = GetBasePos() + i;

		Color4f col = bcol[i].hexColor;// highlighter->GetByteTypeBin(GetBasePos(), buf, i, sz);
		if (pos >= minSel && pos <= maxSel)
			col.BlendOver(state->colorSelect);
		if (state->hoverByte == pos)
			col.BlendOver(state->colorHover);
		if (col.a > 0)
		{
			float xoff = (i % W) * 20;
			float yoff = (i / W) * fh;
			br.SetColor(col.r, col.g, col.b, col.a);
			br.Quad(x + xoff - 2, y + yoff - fh + 4, x + xoff + 18, y + yoff + 3, 0, 0, 1, 1);
		}

		col = bcol[i].asciiColor;// highlighter->GetByteTypeASCII(GetBasePos(), buf, i, sz);
		if (pos >= minSel && pos <= maxSel)
			col.BlendOver(state->colorSelect);
		if (state->hoverByte == pos)
			col.BlendOver(state->colorHover);
		if (col.a > 0)
		{
			float xoff = (i % W) * 10;
			float yoff = (i / W) * fh;
			br.SetColor(col.r, col.g, col.b, col.a);
			br.Quad(x2 + xoff - 2, y + yoff - fh + 4, x2 + xoff + 8, y + yoff + 3, 0, 0, 1, 1);
		}

		col = bcol[i].leftBracketColor;
		if (col.a > 0)
		{
			float xoff = (i % W) * 20;
			float yoff = (i / W) * fh;
			br.SetColor(col.r, col.g, col.b, col.a);
			br.Quad(x + xoff - 2, y + yoff - fh + 5, x + xoff - 1, y + yoff + 2, 0, 0, 1, 1);
			br.Quad(x + xoff - 2, y + yoff - fh + 4, x + xoff + 4, y + yoff - fh + 5, 0, 0, 1, 1);
			br.Quad(x + xoff - 2, y + yoff + 2, x + xoff + 4, y + yoff + 3, 0, 0, 1, 1);
		}

		col = bcol[i].rightBracketColor;
		if (col.a > 0)
		{
			float xoff = (i % W) * 20;
			float yoff = (i / W) * fh;
			br.SetColor(col.r, col.g, col.b, col.a);
			br.Quad(x + xoff + 17, y + yoff - fh + 5, x + xoff + 18, y + yoff + 2, 0, 0, 1, 1);
			br.Quad(x + xoff + 12, y + yoff - fh + 4, x + xoff + 18, y + yoff - fh + 5, 0, 0, 1, 1);
			br.Quad(x + xoff + 12, y + yoff + 2, x + xoff + 18, y + yoff + 3, 0, 0, 1, 1);
		}
	}
	br.End();

	auto size = file->dataSource->GetSize();
	for (int i = 0; i < 64; i++)
	{
		if (GetBasePos() + i * W >= size)
			break;
		char str[16];
		snprintf(str, 16, "%" PRIX64, GetBasePos() + i * W);
		float w = GetTextWidth(str);
		DrawTextLine(x - w - 10, y + i * fh, str, 1, 1, 1, 0.5f);
	}

	for (int i = 0; i < W; i++)
	{
		char str[3];
		str[0] = "0123456789ABCDEF"[i >> 4];
		str[1] = "0123456789ABCDEF"[i & 0xf];
		str[2] = 0;
		float xoff = (i % W) * 20;
		DrawTextLine(x + xoff, y - fh, str, 1, 1, 1, 0.5f);
	}

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
