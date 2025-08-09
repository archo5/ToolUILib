
#pragma once
#include "ImmediateMode.h"

#include "../Core/3DMath.h"


namespace ui {
namespace imm {

enum class QuatEditMode : u8
{
	Raw,
	EulerXYZ,
	EulerZYX,
};

bool EditQuat(
	Quat& val,
	QuatEditMode mode = QuatEditMode::Raw,
	ModInitList mods = {},
	const DragConfig& cfg = {},
	Range<float> range = All{},
	const char* fmt = "%g");

} // imm
} // ui
