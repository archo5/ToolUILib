
#pragma once

#include "Core/Image.h"


namespace ui {


struct Gradient
{
	enum class ColorSpace : u8
	{
		RGB,
		HSV,
	};
	static const char* ColorSpaceKeys[];
	static const char* ColorSpaceNames[];
	enum class ColorCorrection : u8
	{
		None,
		Gamma2_2,
	};
	static const char* ColorCorrectionKeys[];
	static const char* ColorCorrectionNames[];
	enum class HueMode : u8
	{
		Near,
		Far,
		Forward,
		Backwards,
	};
	static const char* HueModeKeys[];
	static const char* HueModeNames[];
	enum class InterpolationType : u8
	{
		Linear,
		Smoothstep,
	};
	static const char* InterpolationTypeKeys[];
	static const char* InterpolationTypeNames[];

	struct ColorKP
	{
		float pos = 0;
		Color4f color;

		void OnSerialize(IObjectIterator& oi, const FieldInfo& fi)
		{
			oi.BeginObject(fi, "Gradient::ColorKP");
			OnField(oi, "pos", pos);
			OnField(oi, "color", color);
			oi.EndObject();
		}
	};
	struct AlphaKP
	{
		float pos = 0;
		float alpha = 1;

		void OnSerialize(IObjectIterator& oi, const FieldInfo& fi)
		{
			oi.BeginObject(fi, "Gradient::AlphaKP");
			OnField(oi, "pos", pos);
			OnField(oi, "alpha", alpha);
			oi.EndObject();
		}
	};

	ColorSpace colorSpace = ColorSpace::RGB;
	ColorCorrection colorCorrection = ColorCorrection::Gamma2_2;
	HueMode hueMode = HueMode::Near;
	InterpolationType interpolationType = InterpolationType::Smoothstep;
	bool separateAlpha = false;

	Array<ColorKP> colors;
	Array<AlphaKP> alphas;

	u32 approxSteps = 256;

	// > 0  ==>  color
	// < 0  ==>  alpha
	i32 curkp = 0;

	static Gradient ColorToColor(Color4f a, Color4f b);
	static Gradient BlackToWhite() { return ColorToColor({ 0, 0, 0 }, { 1, 1, 1 }); }
	static Gradient WhiteToBlack() { return ColorToColor({ 1, 1, 1 }, { 0, 0, 0 }); }

	inline void Serialize(IObjectIterator& oi, const FieldInfo& fi);

	Color4f Sample(float pos) const;

	template <class TCol> Array<TCol> GetApproxT() const
	{
		Array<TCol> ret;
		ret.Resize(approxSteps);
		if (approxSteps == 1)
			ret[0] = Sample(0.5f);
		else
			for (u32 i = 0; i < approxSteps; i++)
				ret[i] = Sample(i / float(approxSteps - 1));
		return ret;
	}
	Array<Color4f> GetApproxF() const { return GetApproxT<Color4f>(); }
	Array<Color4b> GetApproxB() const { return GetApproxT<Color4b>(); }

	template <class TKP> static void SortPoint(Array<TKP>& arr, i32& i)
	{
		while (i > 0 && arr[i - 1].pos > arr[i].pos)
			std::swap(arr[i - 1], arr[i]), i--;
		while (i + 1 < i32(arr.Size()) && arr[i + 1].pos < arr[i].pos)
			std::swap(arr[i + 1], arr[i]), i++;
	}
};

template <> struct EnumKeys<Gradient::ColorSpace> : EnumKeysStringList<Gradient::ColorSpace, Gradient::ColorSpaceKeys> {};
template <> struct EnumKeys<Gradient::ColorCorrection> : EnumKeysStringList<Gradient::ColorCorrection, Gradient::ColorCorrectionKeys> {};
template <> struct EnumKeys<Gradient::HueMode> : EnumKeysStringList<Gradient::HueMode, Gradient::HueModeKeys> {};
template <> struct EnumKeys<Gradient::InterpolationType> : EnumKeysStringList<Gradient::InterpolationType, Gradient::InterpolationTypeKeys> {};

void Gradient::Serialize(IObjectIterator& oi, const FieldInfo& fi)
{
	oi.BeginObject(fi, "Gradient");

	OnFieldEnumString(oi, "colorSpace", colorSpace);
	OnFieldEnumString(oi, "colorCorrection", colorCorrection);
	OnFieldEnumString(oi, "hueMode", hueMode);
	OnFieldEnumString(oi, "interpolationType", interpolationType);
	OnField(oi, "separateAlpha", separateAlpha);
	OnField(oi, "colors", colors);
	OnField(oi, "alphas", alphas);
	OnField(oi, "approxSteps", approxSteps);

	oi.EndObject();
}


} // ui
