
#include "ImmediateMode3D.h"


namespace ui {
namespace imm {

bool imEditQuat(Quat& val, QuatEditMode mode, ModInitList mods, const DragConfig& cfg, Range<float> range, const char* fmt)
{
	if (mode == QuatEditMode::Raw)
		return imEditFloatVec(&val.x, ui::axesXYZW, mods, cfg, range, fmt);
	else if (mode == QuatEditMode::EulerXYZ)
	{
		Vec3f angles = val.ToEulerAnglesXYZ();
		if (imEditVec3f(angles, mods, cfg, range, fmt))
		{
			val = Quat::RotateEulerAnglesXYZ(angles);
			return true;
		}
	}
	else if (mode == QuatEditMode::EulerZYX)
	{
		Vec3f angles = val.ToEulerAnglesZYX();
		if (imEditVec3f(angles, mods, cfg, range, fmt))
		{
			val = Quat::RotateEulerAnglesZYX(angles);
			return true;
		}
	}
	return false;
}

} // imm
} // ui
