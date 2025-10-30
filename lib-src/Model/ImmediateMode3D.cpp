
#include "ImmediateMode3D.h"

#include "Layout.h"


namespace ui {
namespace imm {

bool imEditQuat(Quat& val, QuatEditMode mode, const DragConfig& cfg, Range<float> range, NumberFormatSettings fmt)
{
	if (mode == QuatEditMode::Raw)
		return imEditFloatVec(&val.x, ui::axesXYZW, cfg, range, fmt);
	else if (mode == QuatEditMode::EulerXYZ)
	{
		ui::PushScope<imVectorEditGroup<Vec3f>> group;
		group->SetValue(val.ToEulerAnglesXYZ());
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
		group->SetValue(val.ToEulerAnglesZYX());
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
