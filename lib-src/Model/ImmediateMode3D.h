
#pragma once
#include "ImmediateMode.h"

#include "../Core/3DMath.h"


namespace ui {

inline bool imEditVec3f(Vec3f& val, ModInitList mods = {}, const DragConfig& cfg = {}, Rangef range = All{}, const char* fmt = "%g")
{
	return imEditFloatVec(&val.x, axesXYZ, mods, cfg, range, fmt);
}

enum class QuatEditMode : u8
{
	Raw,
	EulerXYZ,
	EulerZYX,
};

bool imEditQuat(
	Quat& val,
	QuatEditMode mode = QuatEditMode::Raw,
	ModInitList mods = {},
	const DragConfig& cfg = {},
	Range<float> range = All{},
	const char* fmt = "%g");

} // ui
