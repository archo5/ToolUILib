
#pragma once

#include "DrawableImage.h"

#include "../Core/Array.h"
#include "../Core/Image.h"
#include "../Core/Math.h"
#include "../Core/Memory.h"
#include "../Core/RefCounted.h"


namespace ui {
namespace draw {

namespace _ {

void InitResources();
void FreeResources();
void OnBeginDrawFrame();
void OnEndDrawFrame();
void Flush();

} // internals

void LineCol(float x0, float y0, float x1, float y1, float w, Color4b col, bool midpixel = true);
void AALineCol(float x0, float y0, float x1, float y1, float w, Color4b col, bool midpixel = true);
void LineCol(const ArrayView<Point2f>& points, float w, Color4b col, bool closed, bool midpixel = true);
void AALineCol(const ArrayView<Point2f>& points, float w, Color4b col, bool closed, bool midpixel = true);
void CircleLineCol(Point2f center, float rad, float w, Color4b col, bool midpixel = true);
void AACircleLineCol(Point2f center, float rad, float w, Color4b col, bool midpixel = true);
void RectCol(float x0, float y0, float x1, float y1, Color4b col);
void RectGradH(float x0, float y0, float x1, float y1, Color4b a, Color4b b);
void RectTex(float x0, float y0, float x1, float y1, IImage* tex);
void RectTex(const AABB2f& r, IImage* tex);
void RectTex(float x0, float y0, float x1, float y1, IImage* tex, float u0, float v0, float u1, float v1);
void RectTex(const AABB2f& r, IImage* tex, const AABB2f& uv);
void RectColTex(float x0, float y0, float x1, float y1, Color4b col, IImage* tex);
void RectColTex(const AABB2f& r, Color4b col, IImage* tex);
void RectColTex(float x0, float y0, float x1, float y1, Color4b col, IImage* tex, float u0, float v0, float u1, float v1);
void RectColTex9Slice(const AABB2f& outer, const AABB2f& inner, Color4b col, IImage* tex, const AABB2f& texouter, const AABB2f& texinner);
void RectCutoutCol(const AABB2f& rect, const AABB2f& cutout, Color4b col);

typedef void VertexTransformFunction(void* userdata, Vertex* vertices, size_t count);
struct VertexTransformCallback
{
	void* userdata = nullptr;
	VertexTransformFunction* func = nullptr;

	UI_FORCEINLINE void Call(Vertex* vertices, size_t count)
	{
		if (func)
			func(userdata, vertices, count);
	}
};
// returns the previous callback
VertexTransformCallback SetVertexTransformCallback(VertexTransformCallback cb);

void ApplyScissor();

ScaleOffset2D GetScissorRectResolutionTransform();
ScaleOffset2D SetScissorRectResolutionTransform(ScaleOffset2D nsrrt);
ScaleOffset2D AddScissorRectResolutionTransform(ScaleOffset2D nsrrt);

void PushScissorRectRaw(const AABB2i& screen, const AABB2f& virt);
bool PushScissorRect(const AABB2f& rect);
bool PushScissorRectIfNotEmpty(const AABB2f& rect);
void PopScissorRect();
void _ResetScissorRectStack(int x0, int y0, int x1, int y1);
AABB2f GetCurrentScissorRectF();

} // draw
} // ui
