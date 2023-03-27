
#include "Image.h"


namespace ui {

Color4f Color4f::HSV(float h, float s, float v, float a)
{
	h = fmodf(h, 1);
	h *= 6;
	float c = v * s;
	float x = c * (1 - fabsf(fmodf(h, 2) - 1));
	float m = v - c;
	if (h < 1) return { c + m, x + m, m, a };
	if (h < 2) return { x + m, c + m, m, a };
	if (h < 3) return { m, c + m, x + m, a };
	if (h < 4) return { m, x + m, c + m, a };
	if (h < 5) return { x + m, m, c + m, a };
	return { c + m, m, x + m, a };
}

void Color4f::GetHex(char buf[7])
{
	uint8_t r = GetRed8(), g = GetGreen8(), b = GetBlue8();
	buf[0] = hexchar(r >> 4);
	buf[1] = hexchar(r & 0xf);
	buf[2] = hexchar(g >> 4);
	buf[3] = hexchar(g & 0xf);
	buf[4] = hexchar(b >> 4);
	buf[5] = hexchar(b & 0xf);
}

float Color4f::GetHue()
{
	float maxv = GetValue();
	float minv = min(r, min(g, b));
	float c = maxv - minv;
	if (c == 0)
		return 0;
	if (maxv == r)
		return fmodf(((g - b) / c) / 6 + 1, 1);
	if (maxv == g)
		return (2 + (b - r) / c) / 6;
	return (4 + (r - g) / c) / 6;
}


Canvas& Canvas::operator = (const Canvas& c)
{
	SetSize(c.GetWidth(), c.GetHeight());
	memcpy(_pixels, c._pixels, _width * _height * 4);
	return *this;
}

Canvas& Canvas::operator = (Canvas&& c)
{
	delete _pixels;
	_width = c._width;
	_height = c._height;
	_pixels = c._pixels;
	c._width = 0;
	c._height = 0;
	c._pixels = nullptr;
	return *this;
}

void Canvas::SetSize(uint32_t w, uint32_t h)
{
	if (_width == w && _height == h)
		return;
	delete[] _pixels;
	_width = w;
	_height = h;
	_pixels = new uint32_t[w * h];
}


SimpleMaskBlurGen::Input SimpleMaskBlurGen::Input::Normalized() const
{
	auto ret = *this;
	u32 hms = min(ret.boxSizeX, ret.boxSizeY) / 2;
	ret.cornerLT = min(ret.cornerLT, hms);
	ret.cornerLB = min(ret.cornerLB, hms);
	ret.cornerRT = min(ret.cornerRT, hms);
	ret.cornerRB = min(ret.cornerRB, hms);
	return ret;
}

SimpleMaskBlurGen::Input SimpleMaskBlurGen::Input::Canonicalized() const
{
	auto ret = Normalized();
	u32 mrgInL = max(ret.cornerLT, ret.cornerLB);
	u32 mrgInR = max(ret.cornerRT, ret.cornerRB);
	u32 mrgInT = max(ret.cornerLT, ret.cornerRT);
	u32 mrgInB = max(ret.cornerLB, ret.cornerRB);
	u32 r = ret.blurSize;
	if (ret.boxSizeX > r + mrgInL + mrgInR)
		ret.boxSizeX = r + mrgInL + mrgInR + 1;
	if (ret.boxSizeY > r + mrgInT + mrgInB)
		ret.boxSizeY = r + mrgInT + mrgInB + 1;
	return ret;
}

bool SimpleMaskBlurGen::OutputWillDiffer(const Input& a, const Input& b)
{
	if (a.blurSize != b.blurSize ||
		a.cornerLT != b.cornerLT ||
		a.cornerLB != b.cornerLB ||
		a.cornerRT != b.cornerRT ||
		a.cornerRB != b.cornerRB)
		return true; // key attributes differ

	if (a.boxSizeX == b.boxSizeX && a.boxSizeY == b.boxSizeY)
		return false; // same exact size

	// at this point, key attributes match but sizes differ

	u32 mrgInL = max(a.cornerLT, a.cornerLB);
	u32 mrgInR = max(a.cornerRT, a.cornerRB);
	u32 mrgInT = max(a.cornerLT, a.cornerRT);
	u32 mrgInB = max(a.cornerLB, a.cornerRB);
	u32 r = a.blurSize;

	if ((a.boxSizeX <= r + mrgInL + mrgInR) != (b.boxSizeX <= r + mrgInL + mrgInR) ||
		(a.boxSizeY <= r + mrgInT + mrgInB) != (b.boxSizeY <= r + mrgInT + mrgInB))
		return true;
	if (a.boxSizeX <= r + mrgInL + mrgInR && a.boxSizeX != b.boxSizeX ||
		a.boxSizeY <= r + mrgInT + mrgInB && a.boxSizeY != b.boxSizeY)
		return true;

	return false;
}

static int aacircle(int x, int y, int cx, int cy, int radius)
{
	int r2 = radius * radius;
	int d2 = (x - cx) * (x - cx) + (y - cy) * (y - cy);
	float q = float(radius) - sqrtf(float(d2)) + 1.0f;
	return q < 0 ? 0 : q > 1 ? 255 : int(q * 255);
}

struct RRect
{
	int x0, y0, x1, y1, corner00, corner01, corner10, corner11;
};

static int dorectmask(int x, int y, const RRect& rr)
{
	if (x < rr.x0 || x > rr.x1 || y < rr.y0 || y > rr.y1)
		return 0;
	if (x < rr.x0 + rr.corner00 &&
		y < rr.y0 + rr.corner00)
		return aacircle(x, y, rr.x0 + rr.corner00, rr.y0 + rr.corner00, rr.corner00);
	if (x > rr.x1 - rr.corner10 &&
		y < rr.y0 + rr.corner10)
		return aacircle(x, y, rr.x1 - rr.corner10, rr.y0 + rr.corner10, rr.corner10);
	if (x < rr.x0 + rr.corner01 &&
		y > rr.y1 - rr.corner01)
		return aacircle(x, y, rr.x0 + rr.corner01, rr.y1 - rr.corner01, rr.corner01);
	if (x > rr.x1 - rr.corner11 &&
		y > rr.y1 - rr.corner11)
		return aacircle(x, y, rr.x1 - rr.corner11, rr.y1 - rr.corner11, rr.corner11);
	return 255;
}

static void blur(u8* scratch, u8* src, u32 count, u32 incr, int off, float* kernel, u32 ksize)
{
	// blur to scratch
	for (u32 i = 0; i < count; i++)
	{
		float total = 0;
		for (u32 k = 0; k < ksize; k++)
		{
			int sp = int(i + k) - off;
			if (sp < 0 || sp >= count)
				continue; // assume 0
			total += src[sp * incr] * kernel[k];
		}
		scratch[i] = min(int(roundf(total)), 255);
	}

	// copy back
	for (u32 i = 0; i < count; i++)
		src[i * incr] = scratch[i];
}

void SimpleMaskBlurGen::Generate(const Input& uinput, Output& output, Canvas& outCanvas)
{
	auto input = uinput.Normalized();

	u32 mrgInL = max(input.cornerLT, input.cornerLB);
	u32 mrgInR = max(input.cornerRT, input.cornerRB);
	u32 mrgInT = max(input.cornerLT, input.cornerRT);
	u32 mrgInB = max(input.cornerLB, input.cornerRB);

	bool specialX = input.boxSizeX <= input.blurSize + mrgInL + mrgInR;
	bool specialY = input.boxSizeY <= input.blurSize + mrgInT + mrgInB;

	u32 w = (specialX ? input.boxSizeX : 1 + input.blurSize + mrgInL + mrgInR) + input.blurSize + 2;
	u32 h = (specialY ? input.boxSizeY : 1 + input.blurSize + mrgInT + mrgInB) + input.blurSize + 2;

	// TODO: simplified version without inner overlap (sample a gradient based on distance from border)?
	{
		u32 imgmem = w * h;
		u32 scratchmem = max(w, h);
		int halfext = int(ceil(input.blurSize / 2.0f));
		u32 kernelsize = halfext * 2 + 1;
		u32 allmem = imgmem + scratchmem;
		allmem = (allmem + 3 % 4);
		u32 f32kerneloffset = allmem;
		allmem += kernelsize * 4;
		u8* img = (u8*)malloc(allmem);
		u8* scratch = img + imgmem;
		float* f32kernel = (float*)(img + f32kerneloffset);

		// rasterize the shape
		memset(img, 0, imgmem);
		u32 p0 = input.blurSize / 2 + 1;
		RRect rrect =
		{
			int(p0), int(p0), int(w - p0 - 1), int(h - p0 - 1),
			int(input.cornerLT), int(input.cornerLB), int(input.cornerRT), int(input.cornerRB),
		};
		for (u32 y = p0; y < h - p0; y++)
		{
			for (u32 x = p0; x < w - p0; x++)
			{
				img[x + y * w] = dorectmask(x, y, rrect);
			}
		}

		if (input.blurSize > 0)
		{
			// generate the blur kernel (https://en.wikipedia.org/wiki/Gaussian_blur#Mathematics)
			float sigma = input.blurSize / 4.0f;
			float sigma_sq = sigma * sigma;
			float ksum = 0;
			for (u32 i = 0; i < kernelsize; i++)
			{
				float x = float(int(i) - halfext);
				float G_x = expf(-(x * x) / (2 * sigma_sq)) / sqrtf(2 * PI * sigma_sq);
				if (G_x < 0)
					G_x = 0;
				f32kernel[i] = G_x;
				ksum += G_x;
			}
			for (u32 i = 0; i < kernelsize; i++)
				f32kernel[i] /= ksum;

			// x blur
			for (u32 y = 1; y + 1 < h; y++)
			{
				blur(scratch, img + y * w + 1, w - 2, 1, halfext, f32kernel, kernelsize);
			}

			// y blur
			for (u32 x = 1; x + 1 < w; x++)
			{
				blur(scratch, img + x + w, h - 2, w, halfext, f32kernel, kernelsize);
			}
		}

		// copy into canvas as alpha (rgb=255)
		outCanvas.SetSize(w, h);
		u32* px = outCanvas.GetPixels();
		for (u32 y = 0; y < h; y++)
		{
			for (u32 x = 0; x < w; x++)
			{
				px[x + y * w] = 0xffffff + (img[x + y * w] << 24);
			}
		}

		free(img);

		// calculate the outputs
		output.innerOffset = { float(mrgInL + halfext) + 0.5f, float(mrgInT + halfext) + 0.5f, float(mrgInR + halfext) + 0.5f, float(mrgInB + halfext) + 0.5f };
		output.outerOffset = { float(p0) - 0.5f, float(p0) - 0.5f, float(p0) - 0.5f, float(p0) - 0.5f };
		output.outerUV = { 0.5f / w, 0.5f / h, 1 - 0.5f / w, 1 - 0.5f / h };

		if (specialX)
		{
			output.innerOffset.x0 = -output.outerOffset.x0;
			output.innerOffset.x1 = -output.outerOffset.x1;
			output.innerUV.x0 = 0;
			output.innerUV.x1 = 1;
		}
		else
		{
			output.innerUV.x0 = (p0 + output.innerOffset.x0) / w;
			output.innerUV.x1 = 1 - (p0 + output.innerOffset.x1) / w;
		}

		if (specialY)
		{
			output.innerOffset.y0 = -output.outerOffset.y0;
			output.innerOffset.y1 = -output.outerOffset.y1;
			output.innerUV.y0 = 0;
			output.innerUV.y1 = 1;
		}
		else
		{
			output.innerUV.y0 = (p0 + output.innerOffset.y0) / h;
			output.innerUV.y1 = 1 - (p0 + output.innerOffset.y1) / h;
		}
	}
}

} // ui
