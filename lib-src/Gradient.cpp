
#include "Gradient.h"

#include "Core/3DMath.h"


namespace ui {


const char* Gradient::ColorSpaceKeys[] =
{
	"rgb",
	"hsv",
	nullptr,
};

const char* Gradient::ColorSpaceNames[] =
{
	"RGB",
	"HSV",
	nullptr,
};


const char* Gradient::ColorCorrectionKeys[] =
{
	"none",
	"gamma2.2",
	nullptr,
};

const char* Gradient::ColorCorrectionNames[] =
{
	"None",
	"Gamma=2.2",
	nullptr,
};


const char* Gradient::HueModeKeys[] =
{
	"near",
	"far",
	"forward",
	"backwards",
	nullptr,
};

const char* Gradient::HueModeNames[] =
{
	"Near",
	"Far",
	"Forward (CW)",
	"Backwards (CCW)",
	nullptr,
};


const char* Gradient::InterpolationTypeKeys[] =
{
	"linear",
	"smoothstep",
	nullptr,
};

const char* Gradient::InterpolationTypeNames[] =
{
	"Linear",
	"Smoothstep",
	nullptr,
};


Gradient Gradient::ColorToColor(Color4f a, Color4f b)
{
	Gradient ret;
	ret.colors.Append({ 0, a });
	ret.colors.Append({ 1, b });
	return ret;
}

// Curve_QuadSpline.cpp
size_t FindCurveSection(const float* timevalues, size_t stride, size_t count, float x);

Color4f Gradient::Sample(float pos) const
{
	Color4f col;

	if (colors.Size() == 1)
		col = colors.First().color;
	else if (colors.NotEmpty())
	{
		size_t sec = FindCurveSection(&colors.Data()->pos, sizeof(ColorKP), colors.Size(), pos);
		if (sec == 0)
			col = colors.First().color;
		else if (sec == colors.Size())
			col = colors.Last().color;
		else
		{
			size_t i0 = sec - 1;
			size_t i1 = sec;
			float q = invlerpc(colors[i0].pos, colors[i1].pos, pos);
			if (interpolationType == InterpolationType::Smoothstep)
				q = q * q * (3.0f - 2.0f * q);

			Color4f a = colors[i0].color;
			Color4f b = colors[i1].color;
			if (colorCorrection == ColorCorrection::Gamma2_2)
			{
				a = a.Power(2.2f);
				b = b.Power(2.2f);
			}

			if (colorSpace == ColorSpace::HSV)
			{
				float ah = a.GetHue();
				float as = a.GetSaturation();
				float av = a.GetValue();

				float bh = b.GetHue();
				float bs = b.GetSaturation();
				float bv = b.GetValue();

				if (hueMode == HueMode::Near)
				{
					if (fabsf(bh - ah) > 0.5f)
						bh += sign(ah - bh);
				}
				else if (hueMode == HueMode::Far)
				{
					if (fabsf(bh - ah) < 0.5f)
						bh += sign(ah - bh);
				}
				else if (hueMode == HueMode::Forward)
				{
					if (bh < ah)
						bh += 1;
				}
				else if (hueMode == HueMode::Backwards)
				{
					if (bh > ah)
						bh -= 1;
				}

				float ch = lerp(ah, bh, q);
				float cs = lerp(as, bs, q);
				float cv = lerp(av, bv, q);
				float ca = lerp(a.a, b.a, q);

				col = Color4f::HSV(ch, cs, cv, ca);
			}
			else
			{
				col = Color4fLerp(a, b, q);
			}

			if (colorCorrection == ColorCorrection::Gamma2_2)
			{
				col = col.Power(1.f / 2.2f);
			}
		}
	}

	if (separateAlpha)
	{
		if (alphas.Size() == 1)
			col.a = alphas.First().alpha;
		else if (alphas.NotEmpty())
		{
			size_t sec = FindCurveSection(&alphas.Data()->pos, sizeof(AlphaKP), alphas.Size(), pos);
			if (sec == 0)
				col.a = alphas.First().alpha;
			else if (sec == alphas.Size())
				col.a = alphas.Last().alpha;
			else
			{
				size_t i0 = sec - 1;
				size_t i1 = sec;
				float q = invlerpc(alphas[i0].pos, alphas[i1].pos, pos);
				if (interpolationType == InterpolationType::Smoothstep)
					q = q * q * (3.0f - 2.0f * q);
				col.a = lerp(alphas[i0].alpha, alphas[i1].alpha, q);
			}
		}
	}

	return col;
}


} // ui
