
#include "ImmediateMode3D.h"


namespace ui {
namespace imm {

bool EditQuat(Quat& val, QuatEditMode mode, ModInitList mods, const DragConfig& cfg, Range<float> range, const char* fmt)
{
	if (mode == QuatEditMode::Raw)
		return EditFloatVec(&val.x, ui::imm::XYZW, mods, cfg, range, fmt);
	else if (mode == QuatEditMode::EulerXYZ)
	{
		Vec3f angles = val.ToEulerAnglesXYZ();
		if (EditFloatVec(&angles.x, ui::imm::XYZ, mods, cfg, range, fmt))
		{
			val = Quat::RotateEulerAnglesXYZ(angles);
			return true;
		}
	}
	else if (mode == QuatEditMode::EulerZYX)
	{
		Vec3f angles = val.ToEulerAnglesZYX();
		if (EditFloatVec(&angles.x, ui::imm::XYZ, mods, cfg, range, fmt))
		{
			val = Quat::RotateEulerAnglesZYX(angles);
			return true;
		}
	}
	return false;
}

} // imm
} // ui
