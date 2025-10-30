
#pragma once


namespace ui {

struct DragConfig
{
	// all speed values are in units/px
	// snap 0 = snap off

	float snap = 1.0f;
	float speed = 0.1f;
	float slowdownSnap = 0.01f;
	float slowdownSpeed = 0.001f;
	float boostSnap = 1.0f;
	float boostSpeed = 10.0f;

	bool relativeSpeed = false;

	// ModifierKeyFlags
	uint8_t snapOffKey = MK_Alt;
	uint8_t slowdownKey = MK_Ctrl;
	uint8_t boostKey = MK_Shift;

	DragConfig() {}
	DragConfig(float q, bool relSpeed = false) :
		snap(q),
		speed(q * 0.1f),
		slowdownSnap(q * 0.01f),
		slowdownSpeed(q * 0.001f),
		boostSnap(q),
		boostSpeed(q * 10.0f),
		relativeSpeed(relSpeed)
	{}

	float GetSpeed(uint8_t modifierKeys) const
	{
		if (modifierKeys & slowdownKey)
			return slowdownSpeed;
		if (modifierKeys & boostKey)
			return boostSpeed;
		return speed;
	}
	float GetSnap(uint8_t modifierKeys) const
	{
		if (modifierKeys & snapOffKey)
			return 0;
		if (modifierKeys & slowdownKey)
			return slowdownSnap;
		if (modifierKeys & boostKey)
			return boostSnap;
		return snap;
	}
};

struct NumberFormatSettings
{
	i8 digits = -1;
	//bool beautify : 1; -- TODO add actual support
	bool shortFloat : 1;
	bool hex : 1;

	NumberFormatSettings() : /*beautify(true),*/ shortFloat(true), hex(false) {}

	static NumberFormatSettings DecimalDigits(i8 num)
	{
		NumberFormatSettings nfs;
		nfs.digits = num;
		nfs.shortFloat = false;
		return nfs;
	}
};

} // ui
