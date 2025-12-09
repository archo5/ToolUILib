
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

TriangulatorComplex* TriangulatorComplex_Create();
void TriangulatorComplex_Destroy(TriangulatorComplex* TC);
void TriangulatorComplex_Reset(TriangulatorComplex* TC);

// debugging features
bool TriangulatorComplex_LoadFromFile(TriangulatorComplex* TC, StringView path);
bool TriangulatorComplex_SaveToFile(TriangulatorComplex* TC, StringView path);
void TriangulatorComplex_SaveToFileAutoPath(TriangulatorComplex* TC);

// expectations from this function:
// - consistently return the same output for the same `point`
// - return the same result anywhere inside polygons implicitly created from the intersections of the polygons passed in via AddPolygon
// - if near a border edge (whatever makes sense for "near" given the scale of the input data), return true
typedef bool TriangulatorComplex_InsideFunc(void* userdata, Vec2f point);
void TriangulatorComplex_SetInsideFunc(TriangulatorComplex* TC, void* userdata, TriangulatorComplex_InsideFunc* func);

void TriangulatorComplex_AddPolygon(TriangulatorComplex* TC, ArrayView<Vec2f> verts);
void TriangulatorComplex_AddPoints(TriangulatorComplex* TC, ArrayView<Vec2f> points);

void TriangulatorComplex_Triangulate(TriangulatorComplex* TC);

u32 TriangulatorComplex_GetVertexCount(TriangulatorComplex* TC);
bool TriangulatorComplex_GetVertexInfo(TriangulatorComplex* TC, u32 vid, Vec2f& outpos, u32& outinfochainfirstnode);
static constexpr const u32 TriangulatorComplex_NodeID_NULL = -1;
struct TriangulatorComplex_VertexInfoNode
{
	u32 nextnode;
	u32 polyID;
	u32 edgeID;
	float edgeQ;
};
bool TriangulatorComplex_GetVertexInfoNode(TriangulatorComplex* TC, u32 nodeid, TriangulatorComplex_VertexInfoNode& node);
u32 TriangulatorComplex_GetTriangleCount(TriangulatorComplex* TC);
bool TriangulatorComplex_GetTriangle(TriangulatorComplex* TC, u32 tid, u32 outverts[3]);

bool TriangulatorComplexUtil_PointInPolygon(Vec2f point, ArrayView<Vec2f> verts);
bool TriangulatorComplexUtil_PointNearPolygonEdge(Vec2f point, ArrayView<Vec2f> verts, float dist);
bool TriangulatorComplexUtil_PointInOrNearPolygon(Vec2f point, ArrayView<Vec2f> verts, float dist);
bool TriangulatorComplexUtil_PointInButNotNearPolygon(Vec2f point, ArrayView<Vec2f> verts, float dist);


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


struct PolygonCutter
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
