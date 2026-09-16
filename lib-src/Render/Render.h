
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

UI_API void InitResources();
UI_API void FreeResources();
UI_API void OnBeginDrawFrame();
UI_API void OnEndDrawFrame();
UI_API void Flush();
UI_API void RestoreStates();

} // _

UI_API void LineCol(float x0, float y0, float x1, float y1, float w, Color4b col, bool midpixel = true);
UI_API void AALineCol(float x0, float y0, float x1, float y1, float w, Color4b col, bool midpixel = true);
UI_API void LineCol(const ArrayView<Point2f>& points, float w, Color4b col, bool closed, bool midpixel = true);
UI_API void AALineCol(const ArrayView<Point2f>& points, float w, Color4b col, bool closed, bool midpixel = true);
UI_API void PolyCol(const ArrayView<Point2f>& points, Color4b col, bool midpixel = true);
UI_API void AAPolyCol(const ArrayView<Point2f>& points, Color4b col, bool midpixel = true);
UI_API void CircleLineCol(Point2f center, float rad, float w, Color4b col, bool midpixel = true);
UI_API void AACircleLineCol(Point2f center, float rad, float w, Color4b col, bool midpixel = true);
UI_API void CircleCol(Point2f center, float rad, Color4b col, bool midpixel = true);
UI_API void AACircleCol(Point2f center, float rad, Color4b col, bool midpixel = true);
UI_API void RectCol(float x0, float y0, float x1, float y1, Color4b col);
UI_API void RectCol(const AABB2f& r, Color4b col);
UI_API void RectGradH(float x0, float y0, float x1, float y1, Color4b a, Color4b b);
UI_API void RectGradV(float x0, float y0, float x1, float y1, Color4b a, Color4b b);
UI_API void RectTex(float x0, float y0, float x1, float y1, IImage* tex);
UI_API void RectTex(const AABB2f& r, IImage* tex);
UI_API void RectTex(float x0, float y0, float x1, float y1, IImage* tex, float u0, float v0, float u1, float v1);
UI_API void RectTex(const AABB2f& r, IImage* tex, const AABB2f& uv);
UI_API void RectColTex(float x0, float y0, float x1, float y1, Color4b col, IImage* tex);
UI_API void RectColTex(const AABB2f& r, Color4b col, IImage* tex);
UI_API void RectColTex(float x0, float y0, float x1, float y1, Color4b col, IImage* tex, float u0, float v0, float u1, float v1);
UI_API void RectColTex(const AABB2f& r, Color4b col, IImage* tex, const AABB2f& uv);
UI_API void RectColTex9Slice(const AABB2f& outer, const AABB2f& inner, Color4b col, IImage* tex, const AABB2f& texouter, const AABB2f& texinner);
UI_API void RectCutoutCol(const AABB2f& rect, const AABB2f& cutout, Color4b col);

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
UI_API VertexTransformCallback SetVertexTransformCallback(VertexTransformCallback cb);

UI_API void ApplyScissor();

UI_API ScaleOffset2D GetScissorRectResolutionTransform();
UI_API ScaleOffset2D SetScissorRectResolutionTransform(ScaleOffset2D nsrrt);
UI_API ScaleOffset2D AddScissorRectResolutionTransform(ScaleOffset2D nsrrt);

UI_API void PushScissorRectRaw(const AABB2i& screen, const AABB2f& virt);
UI_API bool PushScissorRect(const AABB2f& rect);
UI_API bool PushScissorRectIfNotEmpty(const AABB2f& rect);
UI_API void PopScissorRect();
UI_API void _ResetScissorRectStack(int x0, int y0, int x1, int y1);
UI_API AABB2f GetCurrentScissorRectF();

} // draw
} // ui
