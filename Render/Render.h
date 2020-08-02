
#pragma once

#include <stdint.h>
#include "../Core/Math.h"
#include "../Core/Image.h"
#include "OpenGL.h"


struct Sprite
{
	int ox0, oy0, ox1, oy1;
	int bx0, by0, bx1, by1;
};

enum EThemeElement
{
	TE_ButtonNormal,
	TE_ButtonHover,
	TE_ButtonPressed,
	TE_ButtonDisabled,
	TE_ButtonDownDisabled,

	TE_Panel,

	TE_TextboxNormal,
	TE_TextboxDisabled,

	TE_CheckBgrNormal,
	TE_CheckBgrHover,
	TE_CheckBgrPressed,
	TE_CheckBgrDisabled,

	TE_CheckMark,
	TE_CheckMarkDisabled,
	TE_CheckInd,
	TE_CheckIndDisabled,

	TE_RadioBgrNormal,
	TE_RadioBgrHover,
	TE_RadioBgrPressed,
	TE_RadioBgrDisabled,
	TE_RadioMark,
	TE_RadioMarkDisabled,

	TE_Selector32,
	TE_Selector16,
	TE_Selector12,
	TE_Outline,

	TE_TabNormal,
	TE_TabHover,
	TE_TabSelected,
	TE_TabPanel,

	TE_TreeTickClosedNormal,
	TE_TreeTickClosedHover,
	TE_TreeTickOpenNormal,
	TE_TreeTickOpenHover,

	TE__COUNT,
};


namespace GL {

struct BatchRenderer
{
	void Begin();
	void End();

	void SetColor(float r, float g, float b, float a = 1);
	void SetColor(ui::Color4b c);
	void Pos(float x, float y);
	void Quad(float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1);
	void Line(float x0, float y0, float x1, float y1, float w = 1);

	ui::Color4b col;
};

} // GL


namespace ui {
namespace draw {

void LineCol(float x0, float y0, float x1, float y1, float w, Color4b col);
void RectCol(float x0, float y0, float x1, float y1, Color4b col);
void RectGradH(float x0, float y0, float x1, float y1, Color4b a, Color4b b);
void RectTex(float x0, float y0, float x1, float y1, rhi::Texture2D* tex);
void RectTex(float x0, float y0, float x1, float y1, rhi::Texture2D* tex, float u0, float v0, float u1, float v1);

} // draw
} // ui


void InitFont();
float GetTextWidth(const char* text, size_t num = SIZE_MAX);
float GetFontHeight();
void DrawTextLine(float x, float y, const char* text, float r, float g, float b, float a = 1);

extern ui::rhi::Texture2D* g_themeTexture;
void InitTheme();
AABB<float> GetThemeElementBorderWidths(EThemeElement e);
void DrawThemeElement(EThemeElement e, float x0, float y0, float x1, float y1);
