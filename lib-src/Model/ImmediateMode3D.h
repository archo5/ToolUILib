
#pragma once
#include "ImmediateMode.h"

#include "../Core/3DMath.h"


namespace ui {
namespace imm {

inline bool imEditVec3f(Vec3f& val, const DragConfig& cfg = {}, Rangef range = All{}, NumberFormatSettings fmt = {})
{
	bool chg = (imLabel("\bX", LabeledProperty::OneElement), imEditFloat(val.x, cfg, range, fmt));
	chg |= (imLabel("\bY", LabeledProperty::OneElement), imEditFloat(val.y, cfg, range, fmt));
	return chg | (imLabel("\bZ", LabeledProperty::OneElement), imEditFloat(val.z, cfg, range, fmt));
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
	const DragConfig& cfg = {},
	Range<float> range = All{},
	NumberFormatSettings fmt = {});

} // imm
} // ui
