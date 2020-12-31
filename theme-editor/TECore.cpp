
#include "TECore.h"


DataCategoryTag DCT_NodePreviewInvalidated[1];
DataCategoryTag DCT_ChangeActiveImage[1];


void Color4bLoad(const char* key, Color4b& col, NamedTextSerializeReader& nts)
{
	nts.BeginDict(key);
	col.r = nts.ReadInt("r");
	col.g = nts.ReadInt("g");
	col.b = nts.ReadInt("b");
	col.a = nts.ReadInt("a");
	nts.EndDict();
}

void Color4bSave(const char* key, Color4b& col, JSONLinearWriter& nts)
{
	nts.BeginDict(key);
	nts.WriteInt("r", col.r);
	nts.WriteInt("g", col.g);
	nts.WriteInt("b", col.b);
	nts.WriteInt("a", col.a);
	nts.EndDict();
}

void PointFloatLoad(const char* key, Point<float>& pt, NamedTextSerializeReader& nts)
{
	nts.BeginDict(key);
	pt.x = nts.ReadFloat("x");
	pt.y = nts.ReadFloat("y");
	nts.EndDict();
}

void PointFloatSave(const char* key, Point<float>& pt, JSONLinearWriter& nts)
{
	nts.BeginDict(key);
	nts.WriteFloat("x", pt.x);
	nts.WriteFloat("y", pt.y);
	nts.EndDict();
}

void AABBFloatLoad(const char* key, AABB<float>& rect, NamedTextSerializeReader& nts)
{
	nts.BeginDict(key);
	rect.x0 = nts.ReadFloat("x0");
	rect.y0 = nts.ReadFloat("y0");
	rect.x1 = nts.ReadFloat("x1");
	rect.y1 = nts.ReadFloat("y1");
	nts.EndDict();
}

void AABBFloatSave(const char* key, AABB<float>& rect, JSONLinearWriter& nts)
{
	nts.BeginDict(key);
	nts.WriteFloat("x0", rect.x0);
	nts.WriteFloat("y0", rect.y0);
	nts.WriteFloat("x1", rect.x1);
	nts.WriteFloat("y1", rect.y1);
	nts.EndDict();
}


AbsRect SubRect::Resolve(const AbsRect& frame)
{
	return
	{
		lerp(frame.x0, frame.x1, anchors.x0) + offsets.x0,
		lerp(frame.y0, frame.y1, anchors.y0) + offsets.y0,
		lerp(frame.x0, frame.x1, anchors.x1) + offsets.x1,
		lerp(frame.y0, frame.y1, anchors.y1) + offsets.y1,
	};
}

void SubRect::Load(const char* key, NamedTextSerializeReader& nts)
{
	nts.BeginDict(key);
	AABBFloatLoad("anchors", anchors, nts);
	AABBFloatLoad("offsets", offsets, nts);
	nts.EndDict();
}

void SubRect::Save(const char* key, JSONLinearWriter& nts)
{
	nts.BeginDict(key);
	AABBFloatSave("anchors", anchors, nts);
	AABBFloatSave("offsets", offsets, nts);
	nts.EndDict();
}

void SubRect::OnSerialize(IObjectIterator& oi, const FieldInfo& FI)
{
	oi.BeginObject(FI, "SubRect");
	OnField(oi, "anchors", anchors);
	OnField(oi, "offsets", offsets);
	oi.EndObject();
}


Point<float> SubPos::Resolve(const AbsRect& frame)
{
	return
	{
		lerp(frame.x0, frame.x1, anchor.x) + offset.x,
		lerp(frame.y0, frame.y1, anchor.y) + offset.y,
	};
}

void SubPos::Load(const char* key, NamedTextSerializeReader& nts)
{
	nts.BeginDict(key);
	PointFloatLoad("anchor", anchor, nts);
	PointFloatLoad("offset", offset, nts);
	nts.EndDict();
}

void SubPos::Save(const char* key, JSONLinearWriter& nts)
{
	nts.BeginDict(key);
	PointFloatSave("anchor", anchor, nts);
	PointFloatSave("offset", offset, nts);
	nts.EndDict();
}

void SubPos::OnSerialize(IObjectIterator& oi, const FieldInfo& FI)
{
	oi.BeginObject(FI, "SubPos");
	OnField(oi, "anchor", anchor);
	OnField(oi, "offset", offset);
	oi.EndObject();
}


void CornerRadiuses::Load(const char* key, NamedTextSerializeReader& nts)
{
	nts.BeginDict(key);
	uniform = nts.ReadBool("uniform", true);
	r = nts.ReadFloat("r");
	r00 = nts.ReadFloat("r00");
	r10 = nts.ReadFloat("r10");
	r01 = nts.ReadFloat("r01");
	r11 = nts.ReadFloat("r11");
	nts.EndDict();
}

void CornerRadiuses::Save(const char* key, JSONLinearWriter& nts)
{
	nts.BeginDict(key);
	nts.WriteBool("uniform", uniform);
	nts.WriteFloat("r", r);
	nts.WriteFloat("r00", r00);
	nts.WriteFloat("r10", r10);
	nts.WriteFloat("r01", r01);
	nts.WriteFloat("r11", r11);
	nts.EndDict();
}

void CornerRadiuses::OnSerialize(IObjectIterator& oi, const FieldInfo& FI)
{
	oi.BeginObject(FI, "CornerRadiuses");
	OnField(oi, "uniform", uniform);
	OnField(oi, "r", r);
	OnField(oi, "r00", r00);
	OnField(oi, "r10", r10);
	OnField(oi, "r01", r01);
	OnField(oi, "r11", r11);
	oi.EndObject();
}

float EvalAARoundedRectMask(float x, float y, const AbsRect& rr, const CornerRadiuses& cr)
{
	if (x < rr.x0 + cr.r00 &&
		y < rr.y0 + cr.r00)
		return AACircle(x, y, rr.x0 + cr.r00, rr.y0 + cr.r00, cr.r00);
	if (x > rr.x1 - cr.r10 &&
		y < rr.y0 + cr.r10)
		return AACircle(x, y, rr.x1 - cr.r10, rr.y0 + cr.r10, cr.r10);
	if (x < rr.x0 + cr.r01 &&
		y > rr.y1 - cr.r01)
		return AACircle(x, y, rr.x0 + cr.r01, rr.y1 - cr.r01, cr.r01);
	if (x > rr.x1 - cr.r11 &&
		y > rr.y1 - cr.r11)
		return AACircle(x, y, rr.x1 - cr.r11, rr.y1 - cr.r11, cr.r11);
	return AARect(x, y, rr);
}


void TE_NamedColor::Load(NamedTextSerializeReader& nts)
{
	nts.BeginDict("NamedColor");
	name = nts.ReadString("name");
	Color4bLoad("color", color, nts);
	nts.EndDict();
}

void TE_NamedColor::Save(JSONLinearWriter& nts)
{
	nts.BeginDict("NamedColor");
	nts.WriteString("name", name);
	Color4bSave("color", color, nts);
	nts.EndDict();
}

void TE_NamedColor::OnSerialize(IObjectIterator& oi, const FieldInfo& FI)
{
	oi.BeginObject(FI, "NamedColor");
	OnField(oi, "name", name);
	OnField(oi, "color", color);
	oi.EndObject();
}


std::vector<std::shared_ptr<TE_NamedColor>>* g_namedColors;

void EditNCRef(UIContainer* ctx, std::weak_ptr<TE_NamedColor>& ncref)
{
	// TODO internal selection with color previews
	auto ncr = ncref.lock();
	if (imm::Button(ctx, ncr ? ncr->name.c_str() : "<none>"))
	{
		MenuItemCollection mic;
		mic.Add("<none>", false, false, -1) = [&ncref]() { ncref = {}; };
		for (const auto& nc : *g_namedColors)
		{
			mic.Add(nc->name) = [&ncref, &nc]() { ncref = nc; };
		}
		Menu(mic.Finalize()).Show(ctx->GetCurrentNode());
	}
}

Color4b TE_ColorRef::_GetOrigResolvedColor(const TE_Overrides* ovr)
{
	if (useRef)
	{
		if (auto ptr = ncref.lock())
		{
			if (ovr)
			{
				for (auto& oc : ovr->colorOverrides)
				{
					if (auto ptr2 = oc.ncref.lock())
						if (ptr == ptr2)
							return oc.color;
				}
			}
			return ptr->color;
		}
	}
	return color;
}

void TE_ColorRef::Resolve(const TE_RenderContext& rc, const TE_Overrides* ovr)
{
	resolvedColor = _GetOrigResolvedColor(ovr);
	if (rc.gamma)
		resolvedColor = resolvedColor.Power(2.2f);
}

void TE_ColorRef::Load(const char* key, NamedTextSerializeReader& nts)
{
	nts.BeginDict(key);
	useRef = nts.ReadBool("useRef", true);
	auto name = nts.ReadString("name");
	ncref = {};
	for (auto& nc : *g_namedColors)
	{
		if (nc->name == name)
			ncref = nc;
	}
	Color4bLoad("color", color, nts);
	nts.EndDict();
}

void TE_ColorRef::Save(const char* key, JSONLinearWriter& nts)
{
	nts.BeginDict(key);
	nts.WriteBool("useRef", useRef);
	auto r = ncref.lock();
	nts.WriteString("name", r ? r->name : "");
	Color4bSave("color", color, nts);
	nts.EndDict();
}

void TE_ColorRef::OnSerialize(IObjectIterator& oi, const FieldInfo& FI)
{
	oi.BeginObject(FI, "ColorRef");

	OnField(oi, "useRef", useRef);
	if (!oi.HasField("useRef"))
		useRef = true;

	auto r = ncref.lock();
	std::string name = r ? r->name : "";
	OnField(oi, "name", name);
	if (oi.IsUnserializer())
	{
		ncref = {};
		for (auto& nc : *g_namedColors)
		{
			if (nc->name == name)
				ncref = nc;
		}
	}

	OnField(oi, "color", color);
	oi.EndObject();
}

void TE_ColorRef::UI(UIContainer* ctx)
{
	Property::Scope ps(ctx, "\bColor");
	imm::EditBool(ctx, useRef, useRef ? "N" : "C", {}, imm::ButtonStateToggleSkin());
	if (useRef)
	{
		EditNCRef(ctx, ncref);
	}
	else
	{
		imm::EditColor(ctx, color);
	}
}
