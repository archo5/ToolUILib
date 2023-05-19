
#include "ImmediateMode3D.h"


namespace ui {
namespace imm {

bool EditQuat(Quat& val, QuatEditMode mode, ModInitList mods, const DragConfig& cfg, Range<float> range, const char* fmt)
{
	if (mode == QuatEditMode::Raw)
		return EditFloatVec(&val.x, "XYZW", mods, cfg, range, fmt);
	else if (mode == QuatEditMode::EulerZYX)
	{
		Vec3f angles = val.ToEulerAnglesZYX();
		if (EditFloatVec(&angles.x, "XYZ", mods, cfg, range, fmt))
		{
			val = Quat::RotateEulerAnglesZYX(angles);
			return true;
		}
	}
	return false;
}


bool PropEditQuat(const char* label, Quat& val, QuatEditMode mode, ModInitList mods, const DragConfig& cfg, Range<float> range, const char* fmt)
{
	LabeledProperty::Scope ps(label);
	for (auto& mod : mods)
		mod->ApplyToLabel(ps.label);
	return EditQuat(val, mode, mods, cfg, range, fmt);
}

} // imm
} // ui
