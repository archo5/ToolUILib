
#include "Gradient.h"

#include "Core/3DMath.h"
#include "Curve_QuadSpline.h"


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
	"cubic_ce",
	"cubic_te",
	nullptr,
};

const char* Gradient::InterpolationTypeNames[] =
{
	"Linear",
	"Smoothstep",
	"Cubic (const.ex.)",
	"Cubic (tg.ex.)",
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


static QLIFSplinePoint CubicMakePointA(Gradient::AlphaKP pp, Gradient::AlphaKP cp, Gradient::AlphaKP np, bool extrapolate)
{
	QLIFSplinePoint ret;
	ret.time = cp.pos;
	ret.value = cp.alpha;
	if (isfinite(pp.alpha))
	{
		ret.velocity = divf_safe(np.alpha - pp.alpha, np.pos - pp.pos);
	}
	else if (extrapolate)
	{
		ret.velocity = divf_safe(np.alpha - cp.alpha, np.pos - cp.pos);
	}
	else
	{
		ret.velocity = 0;
	}
	return ret;
}

static QLIFSplinePoint CubicMakePointB(Gradient::AlphaKP pp, Gradient::AlphaKP cp, Gradient::AlphaKP np, bool extrapolate)
{
	QLIFSplinePoint ret;
	ret.time = cp.pos;
	ret.value = cp.alpha;
	if (isfinite(np.alpha))
	{
		ret.velocity = divf_safe(np.alpha - pp.alpha, np.pos - pp.pos);
	}
	else if (extrapolate)
	{
		ret.velocity = divf_safe(cp.alpha - pp.alpha, cp.pos - pp.pos);
	}
	else
	{
		ret.velocity = 0;
	}
	return ret;
}

static float CubicInterpolate(QLIFSplinePoint a, QLIFSplinePoint b, float pos)
{
	float dist = b.time - a.time;
	float t = invlerpc(a.time, b.time, pos);
	float t2 = t * t;
	float t3 = t2 * t;
	a.velocity *= dist;
	b.velocity *= dist;
	return a.value * (2 * t3 - 3 * t2 + 1) + a.velocity * (t3 - 2 * t2 + t) + b.value * (-2 * t3 + 3 * t2) + b.velocity * (t3 - t2);
}


Color4f Gradient::Sample(float pos) const
{
	using IT = InterpolationType;
	bool cex = interpolationType == IT::Cubic_TE;

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
			if (interpolationType == IT::Smoothstep)
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
				if (interpolationType == IT::Cubic_CE || interpolationType == IT::Cubic_TE)
				{
					AlphaKP pkpr, pkpg, pkpb, pkpa;
					AlphaKP kp0r, kp0g, kp0b, kp0a;
					AlphaKP kp1r, kp1g, kp1b, kp1a;
					AlphaKP nkpr, nkpg, nkpb, nkpa;

					if (i0 == 0)
						pkpr = pkpg = pkpb = pkpa = AlphaKP{ -1, INFINITY };
					else
					{
						auto& c = colors[i0 - 1];
						Color4f p = c.color;
						if (colorCorrection == ColorCorrection::Gamma2_2)
							p = p.Power(2.2f);
						pkpr = { c.pos, p.r };
						pkpg = { c.pos, p.g };
						pkpb = { c.pos, p.b };
						pkpa = { c.pos, p.a };
					}

					{
						auto& c = colors[i0];
						kp0r = { c.pos, a.r };
						kp0g = { c.pos, a.g };
						kp0b = { c.pos, a.b };
						kp0a = { c.pos, a.a };
					}

					{
						auto& c = colors[i1];
						kp1r = { c.pos, b.r };
						kp1g = { c.pos, b.g };
						kp1b = { c.pos, b.b };
						kp1a = { c.pos, b.a };
					}

					if (i1 + 1 == colors.Size())
						nkpr = nkpg = nkpb = nkpa = AlphaKP{ -1, INFINITY };
					else
					{
						auto& c = colors[i1 + 1];
						Color4f n = c.color;
						if (colorCorrection == ColorCorrection::Gamma2_2)
							n = n.Power(2.2f);
						nkpr = { c.pos, n.r };
						nkpg = { c.pos, n.g };
						nkpb = { c.pos, n.b };
						nkpa = { c.pos, n.a };
					}

					QLIFSplinePoint p0r = CubicMakePointA(pkpr, kp0r, kp1r, cex);
					QLIFSplinePoint p0g = CubicMakePointA(pkpg, kp0g, kp1g, cex);
					QLIFSplinePoint p0b = CubicMakePointA(pkpb, kp0b, kp1b, cex);
					QLIFSplinePoint p0a = CubicMakePointA(pkpa, kp0a, kp1a, cex);

					QLIFSplinePoint p1r = CubicMakePointB(kp0r, kp1r, nkpr, cex);
					QLIFSplinePoint p1g = CubicMakePointB(kp0g, kp1g, nkpg, cex);
					QLIFSplinePoint p1b = CubicMakePointB(kp0b, kp1b, nkpb, cex);
					QLIFSplinePoint p1a = CubicMakePointB(kp0a, kp1a, nkpa, cex);

					col.r = CubicInterpolate(p0r, p1r, pos);
					col.g = CubicInterpolate(p0g, p1g, pos);
					col.b = CubicInterpolate(p0b, p1b, pos);
					col.a = CubicInterpolate(p0a, p1a, pos);
				}
				else
				{
					col = Color4fLerp(a, b, q);
				}
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
				if (interpolationType == IT::Smoothstep)
					q = q * q * (3.0f - 2.0f * q);

				if (interpolationType == IT::Cubic_CE || interpolationType == IT::Cubic_TE)
				{
					AlphaKP pkp = i0 == 0 ? AlphaKP{ -1, INFINITY } : alphas[i0 - 1];
					AlphaKP nkp = i1 + 1 == alphas.Size() ? AlphaKP{ -1, INFINITY } : alphas[i1 + 1];
					QLIFSplinePoint p0 = CubicMakePointA(pkp, alphas[i0], alphas[i1], cex);
					QLIFSplinePoint p1 = CubicMakePointB(alphas[i0], alphas[i1], nkp, cex);
					col.a = CubicInterpolate(p0, p1, pos);
				}
				else
				{
					col.a = lerp(alphas[i0].alpha, alphas[i1].alpha, q);
				}
			}
		}
	}

	return col;
}


} // ui
