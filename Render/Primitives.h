
#pragma once
#include "../Core/Image.h"
#include "../Core/3DMath.h"


namespace ui {


struct Vertex_PF3CB4
{
	Vec3f pos;
	Color4b col;
};


namespace prim {


inline void SetVertexColor(Vertex_PF3CB4* verts, uint16_t count, ui::Color4b col)
{
	for (uint16_t i = 0; i < count; i++)
		verts[i].col = col;
}


struct PlaneSettings
{
	uint16_t numFacesX = 1;
	uint16_t numFacesY = 1;

	constexpr uint16_t CalcVertexCount() const
	{
		return (numFacesX + 1) * (numFacesY + 1);
	}
	constexpr unsigned CalcIndexCount() const
	{
		return numFacesX * numFacesY * 6;
	}
};
void GeneratePlane(const PlaneSettings& S, Vertex_PF3CB4* outVerts, uint16_t* outIndices);


struct BoxSettings
{
	uint16_t numFacesX = 1;
	uint16_t numFacesY = 1;
	uint16_t numFacesZ = 1;

	constexpr uint16_t CalcVertexCount() const
	{
		return (numFacesX + 1) * (numFacesY + 1) * 2
			+ (numFacesY + 1) * (numFacesZ + 1) * 2
			+ (numFacesZ + 1) * (numFacesX + 1) * 2;
	}
	constexpr unsigned CalcIndexCount() const
	{
		return (numFacesX * numFacesY + numFacesY * numFacesZ + numFacesZ * numFacesX) * 2 * 6;
	}
};
void GenerateBox(const BoxSettings& S, Vertex_PF3CB4* outVerts, uint16_t* outIndices);


struct ConeSettings
{
	uint16_t numFacesH = 32; // 3+, covers 360 degrees

	constexpr uint16_t CalcVertexCount() const
	{
		return numFacesH + 1;
	}
	constexpr unsigned CalcIndexCount() const
	{
		return (numFacesH * 2 - 2) * 3;
	}
};
void GenerateCone(const ConeSettings& S, Vertex_PF3CB4* outVerts, uint16_t* outIndices);


struct UVSphereSettings
{
	uint16_t numFacesH = 32; // 3+, covers 360 degrees
	uint16_t numFacesV = 16; // 2+, covers 180 degrees

	constexpr uint16_t CalcVertexCount() const
	{
		// top: 1
		// bottom: 1
		// every middle strip (count = numFacesV - 1): numFacesH
		return (numFacesV - 1) * numFacesH + 2;
	}
	constexpr unsigned CalcIndexCount() const
	{
		// top: numFacesH
		// bottom: numFacesH
		// every middle strip (count = numFacesV - 2): numFacesH * 2
		return 3 * (numFacesH * 2 * (numFacesV - 1));
	}
};
void GenerateUVSphere(const UVSphereSettings& S, Vertex_PF3CB4* outVerts, uint16_t* outIndices);


struct BoxSphereSettings
{
	uint16_t numFacesQ = 4; // covers one third of one corner (center to corner for one plane) with Q * Q faces

	constexpr uint16_t CalcVertexCount() const
	{
		unsigned edge = numFacesQ * 2 + 1;
		return edge * edge * 6;
	}
	constexpr uint16_t CalcIndexCount() const
	{
		return numFacesQ * numFacesQ * 6 * 4 * 6;
	}
};
void GenerateBoxSphere(const BoxSphereSettings& S, Vertex_PF3CB4* outVerts, uint16_t* outIndices);


} // prim
} // ui
