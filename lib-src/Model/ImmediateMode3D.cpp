
#include "ImmediateMode3D.h"

#include "Layout.h"
#include "../Editors/NumberEditor.h"


namespace ui {
namespace imm {

// have seen errors as large as 0.0003 from the roundtrip so leaving just 3 decimal digits seems to work
// maybe sometime in the future there might be value from more precision but not seeing it now
static void RemoveDecimalErrorAngle(float& v)
{
	char bfr[32];
	snprintf(bfr, sizeof(bfr), "%.3f", v); // FLT_DIG - 1
	sscanf(bfr, "%g", &v);
}

static Vec3f RemoveDecimalErrorAngleVec3(Vec3f v)
{
	RemoveDecimalErrorAngle(v.x);
	RemoveDecimalErrorAngle(v.y);
	RemoveDecimalErrorAngle(v.z);
	return v;
}

bool imEditQuat(Quat& val, QuatEditMode mode, const DragConfig& cfg, Range<float> range, NumberFormatSettings fmt)
{
	if (mode == QuatEditMode::Raw)
		return imEditFloatVec(&val.x, ui::axesXYZW, cfg, range, fmt);
	else if (mode == QuatEditMode::EulerXYZ)
	{
		ui::PushScope<imVectorEditGroup<Vec3f>> group;
		group->SetValue(RemoveDecimalErrorAngleVec3(val.ToEulerAnglesXYZ()));
		ui::PushScope<StackExpandLTRLayoutElement> layout;
		if (imEditVec3f(group->value, cfg, range, fmt))
		{
			val = Quat::RotateEulerAnglesXYZ(group->value);
			return true;
		}
	}
	else if (mode == QuatEditMode::EulerZYX)
	{
		ui::PushScope<imVectorEditGroup<Vec3f>> group;
		group->SetValue(RemoveDecimalErrorAngleVec3(val.ToEulerAnglesZYX()));
		ui::PushScope<StackExpandLTRLayoutElement> layout;
		if (imEditVec3f(group->value, cfg, range, fmt))
		{
			val = Quat::RotateEulerAnglesZYX(group->value);
			return true;
		}
	}
	return false;
}

} // imm
} // ui
