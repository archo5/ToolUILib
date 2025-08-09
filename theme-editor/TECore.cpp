
#include "TECore.h"


MulticastDelegate<struct TE_Node*> OnNodePreviewInvalidated;
MulticastDelegate<> OnActiveImageChanged;


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

void SubRect::OnSerialize(IObjectIterator& oi, const FieldInfo& FI)
{
	oi.BeginObject(FI, "SubRect");
	OnField(oi, "anchors", anchors);
	OnField(oi, "offsets", offsets);
	oi.EndObject();
}


Point2f SubPos::Resolve(const AbsRect& frame)
{
	return
	{
		lerp(frame.x0, frame.x1, anchor.x) + offset.x,
		lerp(frame.y0, frame.y1, anchor.y) + offset.y,
	};
}

void SubPos::OnSerialize(IObjectIterator& oi, const FieldInfo& FI)
{
	oi.BeginObject(FI, "SubPos");
	OnField(oi, "anchor", anchor);
	OnField(oi, "offset", offset);
	oi.EndObject();
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


void TE_NamedColor::OnSerialize(IObjectIterator& oi, const FieldInfo& FI)
{
	oi.BeginObject(FI, "NamedColor");
	OnField(oi, "name", name);
	OnField(oi, "color", color);
	oi.EndObject();
}


Array<std::shared_ptr<TE_NamedColor>>* g_namedColors;

void EditNCRef(std::weak_ptr<TE_NamedColor>& ncref)
{
	// TODO internal selection with color previews
	auto ncr = ncref.lock();
	if (imm::Button(ncr ? ncr->name.c_str() : "<none>"))
	{
		MenuItemCollection mic;
		mic.Add("<none>", false, false, -1) = [&ncref]() { ncref = {}; };
		for (const auto& nc : *g_namedColors)
		{
			mic.Add(nc->name) = [&ncref, &nc]() { ncref = nc; };
		}
		Menu(mic.Finalize()).Show(ui::GetCurrentBuildable());
	}
}

void OnField(IObjectIterator& oi, const FieldInfo& FI, std::weak_ptr<TE_NamedColor>& val)
{
	auto r = val.lock();
	std::string name = r ? r->name : "";
	OnField(oi, FI, name);
	if (oi.IsUnserializer())
	{
		val = {};
		for (auto& nc : *oi.GetUnserializeStorage<TE_IUnserializeStorage>()->curNamedColors)
		{
			if (nc->name == name)
				val = nc;
		}
	}
}


void TE_ColorOverride::OnSerialize(IObjectIterator& oi, const FieldInfo& FI)
{
	oi.BeginObject(FI, "ColorOverride");
	OnField(oi, "ncref", ncref);
	OnField(oi, "color", color);
	oi.EndObject();
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

void TE_ColorRef::OnSerialize(IObjectIterator& oi, const FieldInfo& FI)
{
	oi.BeginObject(FI, "ColorRef");

	OnField(oi, "useRef", useRef);
	if (!oi.HasField("useRef"))
		useRef = true;

	OnField(oi, "ncref", ncref);

	OnField(oi, "color", color);
	oi.EndObject();
}

void TE_ColorRef::UI()
{
	LabeledProperty::Scope ps("\bColor");
	imm::EditBool(useRef, useRef ? "N" : "C", {}, imm::ButtonStateToggleSkin());
	if (useRef)
	{
		EditNCRef(ncref);
	}
	else
	{
		imEditColor(color);
	}
}
