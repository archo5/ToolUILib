
#pragma once
#include "../GUI.h"


using namespace ui;


extern DataCategoryTag DCT_NodePreviewInvalidated[1];
extern DataCategoryTag DCT_ChangeActiveImage[1];


void Color4bLoad(const char* key, Color4b& col, NamedTextSerializeReader& nts);
void Color4bSave(const char* key, Color4b& col, NamedTextSerializeWriter& nts);
void PointFloatLoad(const char* key, Point<float>& pt, NamedTextSerializeReader& nts);
void PointFloatSave(const char* key, Point<float>& pt, NamedTextSerializeWriter& nts);
void AABBFloatLoad(const char* key, AABB<float>& rect, NamedTextSerializeReader& nts);
void AABBFloatSave(const char* key, AABB<float>& rect, NamedTextSerializeWriter& nts);


using AbsRect = AABB<float>;

struct SubRect
{
	AABB<float> anchors = {};
	AABB<float> offsets = {};

	AbsRect Resolve(const AbsRect& frame);
	void Load(const char* key, NamedTextSerializeReader& nts);
	void Save(const char* key, NamedTextSerializeWriter& nts);
};

struct SubPos
{
	Point<float> anchor = {};
	Point<float> offset = {};

	Point<float> Resolve(const AbsRect& frame);
	void Load(const char* key, NamedTextSerializeReader& nts);
	void Save(const char* key, NamedTextSerializeWriter& nts);
};

struct CornerRadiuses
{
	CornerRadiuses Eval()
	{
		if (uniform)
			return { uniform, r, r, r, r, r };
		return *this;
	}
	void Load(const char* key, NamedTextSerializeReader& nts);
	void Save(const char* key, NamedTextSerializeWriter& nts);

	bool uniform = true;
	float r = 0;
	float r00 = 0, r10 = 0, r01 = 0, r11 = 0;
};

float EvalAARoundedRectMask(float x, float y, const AbsRect& rr, const CornerRadiuses& cr);


struct TE_NamedColor
{
	std::string name;
	Color4b color;

	TE_NamedColor() {}
	TE_NamedColor(const std::string& n, Color4b c) : name(n), color(c) {}

	void Load(NamedTextSerializeReader& nts);
	void Save(NamedTextSerializeWriter& nts);
};

extern std::vector<std::shared_ptr<TE_NamedColor>>* g_namedColors;

void EditNCRef(UIContainer* ctx, std::weak_ptr<TE_NamedColor>& ncref);


struct TE_ColorOverride
{
	std::weak_ptr<TE_NamedColor> ncref;
	Color4b color;
};

struct TE_Overrides
{
	std::vector<TE_ColorOverride> colorOverrides;
};


struct TE_RenderContext
{
	unsigned width;
	unsigned height;
	AbsRect frame;
	bool gamma;
};

struct TE_IRenderContextProvider
{
	virtual const TE_RenderContext& GetRenderContext() = 0;
};

struct TE_ColorRef
{
	bool useRef = true;
	std::weak_ptr<TE_NamedColor> ncref;
	Color4b color;
	Color4f resolvedColor;

	Color4b _GetOrigResolvedColor(const TE_Overrides* ovr);
	void Resolve(const TE_RenderContext& rc, const TE_Overrides* ovr);

	void Set(std::weak_ptr<TE_NamedColor> ncr)
	{
		useRef = true;
		ncref = ncr;
	}
	void Set(Color4b c)
	{
		useRef = false;
		color = c;
	}

	void Load(const char* key, NamedTextSerializeReader& nts);
	void Save(const char* key, NamedTextSerializeWriter& nts);
	void UI(UIContainer* ctx);
};


inline float Clamp01(float x)
{
	return x < 0 ? 0 : x > 1 ? 1 : x;
}

inline float AARect(float x, float y, const AbsRect& rect)
{
	return Clamp01(x - rect.x0 + 0.5f)
		* Clamp01(rect.x1 - x + 0.5f)
		* Clamp01(y - rect.y0 + 0.5f)
		* Clamp01(rect.y1 - y + 0.5f);
}

inline float AACircle(float x, float y, float cx, float cy, float radius)
{
	float r2 = radius * radius;
	float d2 = (x - cx) * (x - cx) + (y - cy) * (y - cy);
	float q = radius - sqrtf(d2) + 0.5f;
	return Clamp01(q);
}
