
// license:
// - use for whatever for free
// - no promises that this will work for you
// - add a link to author or original file in your copy
// - report bugs and reliability improvements

#pragma once

#include "Math.h"
#include "Memory.h"


namespace ui {

struct TriangulatorComplex;

UI_API TriangulatorComplex* TriangulatorComplex_Create();
UI_API void TriangulatorComplex_Destroy(TriangulatorComplex* TC);
UI_API void TriangulatorComplex_Reset(TriangulatorComplex* TC);

// debugging features
UI_API bool TriangulatorComplex_LoadFromFile(TriangulatorComplex* TC, StringView path);
UI_API bool TriangulatorComplex_SaveToFile(TriangulatorComplex* TC, StringView path);
UI_API void TriangulatorComplex_SaveToFileAutoPath(TriangulatorComplex* TC);

// expectations from this function:
// - consistently return the same output for the same `point`
// - return the same result anywhere inside polygons implicitly created from the intersections of the polygons passed in via AddPolygon
// - if near a border edge (whatever makes sense for "near" given the scale of the input data), return true
typedef bool TriangulatorComplex_InsideFunc(void* userdata, Vec2f point);
UI_API void TriangulatorComplex_SetInsideFunc(TriangulatorComplex* TC, void* userdata, TriangulatorComplex_InsideFunc* func);

UI_API void TriangulatorComplex_AddPolygon(TriangulatorComplex* TC, ArrayView<Vec2f> verts);
UI_API void TriangulatorComplex_AddPoints(TriangulatorComplex* TC, ArrayView<Vec2f> points);

UI_API void TriangulatorComplex_Triangulate(TriangulatorComplex* TC);

UI_API u32 TriangulatorComplex_GetVertexCount(TriangulatorComplex* TC);
UI_API bool TriangulatorComplex_GetVertexInfo(TriangulatorComplex* TC, u32 vid, Vec2f& outpos, u32& outinfochainfirstnode);
static constexpr const u32 TriangulatorComplex_NodeID_NULL = -1;
struct TriangulatorComplex_VertexInfoNode
{
	u32 nextnode;
	u32 polyID;
	u32 edgeID;
	float edgeQ;
};
UI_API bool TriangulatorComplex_GetVertexInfoNode(TriangulatorComplex* TC, u32 nodeid, TriangulatorComplex_VertexInfoNode& node);
UI_API u32 TriangulatorComplex_GetTriangleCount(TriangulatorComplex* TC);
UI_API bool TriangulatorComplex_GetTriangle(TriangulatorComplex* TC, u32 tid, u32 outverts[3]);

UI_API bool TriangulatorComplexUtil_PointInPolygon(Vec2f point, ArrayView<Vec2f> verts);
UI_API bool TriangulatorComplexUtil_PointNearPolygonEdge(Vec2f point, ArrayView<Vec2f> verts, float dist);
UI_API bool TriangulatorComplexUtil_PointInOrNearPolygon(Vec2f point, ArrayView<Vec2f> verts, float dist);
UI_API bool TriangulatorComplexUtil_PointInButNotNearPolygon(Vec2f point, ArrayView<Vec2f> verts, float dist);


struct PolygonOutput
{
	static constexpr const u32 NodeID_NULL = -1;
	struct VertexInfoNode
	{
		u32 nextNodeID;
		u32 polyID;
		u32 edgeID;
		float edgeQ;
	};
	struct Vertex
	{
		Vec2f pos;
		u32 nodeID;
	};
	struct Polygon
	{
		u32 firstVertex;
		u32 lastVertex;
	};

	Array<Vertex> verts;
	Array<VertexInfoNode> infos;
	Array<Polygon> polygons;
};


struct UI_API PolygonCutter
{
	static PolygonCutter* Create();
	void Destroy();
	enum
	{
		Mode_Combine = 0, // A and B are treated equally
		Mode_Subtract = 1,
		Mode_Intersect = 2,
	};
	void Reset(u8 mode, bool keepInnerEdges);
	void AddPolygonA(ArrayView<Vec2f> poly);
	void AddPolygonB(ArrayView<Vec2f> poly);
	void AddCuttingPath(ArrayView<Vec2f> path);
	void Cut(PolygonOutput& output);
};

} // ui
