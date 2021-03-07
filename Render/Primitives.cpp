
#include "Primitives.h"


namespace ui {
namespace prim {


static float Range1F(uint16_t i, uint16_t n)
{
	return float(i) / n;
}

static float Range2F(uint16_t i, uint16_t n)
{
	return float(i) / n * 2.0f - 1.0f;
}

static_assert(PlaneSettings{ 2, 3 }.CalcVertexCount() == 12, "[plane] bad vertex count calculation");
static_assert(PlaneSettings{ 2, 3 }.CalcIndexCount() == 36, "[plane] bad index count calculation");
void GeneratePlane(const PlaneSettings& S, Vertex_PF3CB4* outVerts, uint16_t* outIndices)
{
	uint16_t nfx = S.numFacesX;
	uint16_t nfy = S.numFacesY;

	for (uint16_t y = 0; y <= nfy; y++)
	{
		for (uint16_t x = 0; x <= nfx; x++)
		{
			outVerts++->pos = { Range2F(x, nfx), Range2F(y, nfy), 0 };
		}
	}

	uint16_t vw = nfx + 1;

	for (uint16_t y = 0; y < nfy; y++)
	{
		for (uint16_t x = 0; x < nfx; x++)
		{
			uint16_t a = x + y * vw;
			uint16_t b = a + 1;
			uint16_t c = b + vw;
			uint16_t d = a + vw;

			*outIndices++ = a;
			*outIndices++ = d;
			*outIndices++ = c;
			*outIndices++ = c;
			*outIndices++ = b;
			*outIndices++ = a;
		}
	}
}

static_assert(BoxSettings{ 2, 3, 4 }.CalcVertexCount() == (12 + 20 + 15) * 2, "[box] bad vertex count calculation");
static_assert(BoxSettings{ 2, 3, 4 }.CalcIndexCount() == (36 + 72 + 48) * 2, "[box] bad index count calculation");
void GenerateBox(const BoxSettings& S, Vertex_PF3CB4* outVerts, uint16_t* outIndices)
{
	PlaneSettings XY = { S.numFacesX, S.numFacesY };
	PlaneSettings YZ = { S.numFacesY, S.numFacesZ };
	PlaneSettings ZX = { S.numFacesZ, S.numFacesX };
	uint16_t vcXY = XY.CalcVertexCount();
	uint16_t vcYZ = YZ.CalcVertexCount();
	uint16_t vcZX = ZX.CalcVertexCount();
	unsigned icXY = XY.CalcIndexCount();
	unsigned icYZ = YZ.CalcIndexCount();
	unsigned icZX = ZX.CalcIndexCount();
	uint16_t vertOff = 0;

	// X+
	GeneratePlane(YZ, outVerts, outIndices);
	for (uint16_t i = 0; i < vcYZ; i++)
	{
		auto& v = outVerts[i].pos;
		v = { 1, v.x, v.y };
	}
	for (unsigned i = 0; i < icYZ; i++)
		outIndices[i] += vertOff;
	vertOff += vcYZ;
	outVerts += vcYZ;
	outIndices += icYZ;
	// X-
	GeneratePlane(YZ, outVerts, outIndices);
	for (uint16_t i = 0; i < vcYZ; i++)
	{
		auto& v = outVerts[i].pos;
		v = { -1, -v.x, v.y };
	}
	for (unsigned i = 0; i < icYZ; i++)
		outIndices[i] += vertOff;
	vertOff += vcYZ;
	outVerts += vcYZ;
	outIndices += icYZ;
	// Y+
	GeneratePlane(ZX, outVerts, outIndices);
	for (uint16_t i = 0; i < vcZX; i++)
	{
		auto& v = outVerts[i].pos;
		v = { v.y, 1, v.x };
	}
	for (unsigned i = 0; i < icZX; i++)
		outIndices[i] += vertOff;
	vertOff += vcZX;
	outVerts += vcZX;
	outIndices += icZX;
	// Y-
	GeneratePlane(ZX, outVerts, outIndices);
	for (uint16_t i = 0; i < vcZX; i++)
	{
		auto& v = outVerts[i].pos;
		v = { v.y , -1, -v.x };
	}
	for (unsigned i = 0; i < icZX; i++)
		outIndices[i] += vertOff;
	vertOff += vcZX;
	outVerts += vcZX;
	outIndices += icZX;
	// Z+
	GeneratePlane(XY, outVerts, outIndices);
	for (uint16_t i = 0; i < vcXY; i++)
	{
		auto& v = outVerts[i].pos;
		v = { v.x, v.y, 1 };
	}
	for (unsigned i = 0; i < icXY; i++)
		outIndices[i] += vertOff;
	vertOff += vcXY;
	outVerts += vcXY;
	outIndices += icXY;
	// Z-
	GeneratePlane(XY, outVerts, outIndices);
	for (uint16_t i = 0; i < vcXY; i++)
	{
		auto& v = outVerts[i].pos;
		v = { -v.x, v.y, -1 };
	}
	for (unsigned i = 0; i < icXY; i++)
		outIndices[i] += vertOff;
	vertOff += vcXY;
	outVerts += vcXY;
	outIndices += icXY;
}

static_assert(ConeSettings{ 3 }.CalcVertexCount() == 4, "[cone] bad vertex count calculation");
static_assert(ConeSettings{ 3 }.CalcIndexCount() == 12, "[cone] bad index count calculation");
void GenerateCone(const ConeSettings& S, Vertex_PF3CB4* outVerts, uint16_t* outIndices)
{
	uint16_t nfh = S.numFacesH;

	for (uint16_t i = 0; i < nfh; i++)
	{
		float a = Range1F(i, nfh) * 3.14159f * 2;
		outVerts++->pos = { sinf(a), cosf(a), 0.0f };
	}
	outVerts++->pos = { 0, 0, 1 };

	for (uint16_t i = 0; i < nfh; i++)
	{
		*outIndices++ = i;
		*outIndices++ = (i + 1) % nfh;
		*outIndices++ = nfh;
	}

	for (uint16_t i = 2; i < nfh; i++)
	{
		*outIndices++ = 0;
		*outIndices++ = i;
		*outIndices++ = i - 1;
	}
}

static_assert(UVSphereSettings{ 4, 3 }.CalcVertexCount() == 10, "[uvsphere] bad vertex count calculation");
static_assert(UVSphereSettings{ 4, 3 }.CalcIndexCount() == (4 + 4 + 8) * 3, "[uvsphere] bad index count calculation");
void GenerateUVSphere(const UVSphereSettings& S, Vertex_PF3CB4* outVerts, uint16_t* outIndices)
{
	uint16_t nfh = S.numFacesH;
	uint16_t nfv = S.numFacesV;
	uint16_t evp = nfh * (nfv - 1); // extreme vertex positions (top, bottom)

	outVerts[evp].pos = { 0, 0, 1 };
	outVerts[evp + 1].pos = { 0, 0, -1 };

	for (uint16_t v = 1; v < nfv; v++)
	{
		float angVert = Range1F(v, nfv) * 3.14159f;
		float sinAngVert = sinf(angVert);
		float z = cosf(angVert);
		for (uint16_t h = 0; h < nfh; h++)
		{
			float angHor = Range1F(h, nfh) * 3.14159f * 2;
			float x = cosf(angHor) * sinAngVert;
			float y = sinf(angHor) * sinAngVert;
			outVerts++->pos = { x, y, z };
		}
	}

	// top row
	for (uint16_t h = 0; h < nfh; h++)
	{
		*outIndices++ = evp;
		*outIndices++ = (h + 1) % nfh;
		*outIndices++ = h;
	}

	// middle rows
	for (uint16_t v = 0; v + 2 < nfv; v++)
	{
		for (uint16_t h = 0; h < nfh; h++)
		{
			uint16_t h1 = (h + 1) % nfh;
			uint16_t a = nfh * v + h;
			uint16_t b = nfh * v + h1;
			uint16_t c = b + nfh;
			uint16_t d = a + nfh;

			*outIndices++ = a;
			*outIndices++ = b;
			*outIndices++ = c;
			*outIndices++ = c;
			*outIndices++ = d;
			*outIndices++ = a;
		}
	}

	// bottom row
	uint16_t lastRow = (nfv - 2) * nfh;
	for (uint16_t h = 0; h < nfh; h++)
	{
		*outIndices++ = h + lastRow;
		*outIndices++ = (h + 1) % nfh + lastRow;
		*outIndices++ = evp + 1;
	}
}

static_assert(BoxSettings{ 6, 6, 6 }.CalcVertexCount() == BoxSphereSettings{ 3 }.CalcVertexCount(), "[boxsphere] bad vertex count calculation");
static_assert(BoxSettings{ 6, 6, 6 }.CalcIndexCount() == BoxSphereSettings{ 3 }.CalcIndexCount(), "[boxsphere] bad index count calculation");
void GenerateBoxSphere(const BoxSphereSettings& S, Vertex_PF3CB4* outVerts, uint16_t* outIndices)
{
	uint16_t nf = S.numFacesQ * 2;
	BoxSettings BS = { nf, nf, nf };
	GenerateBox(BS, outVerts, outIndices);

	uint16_t vc = BS.CalcVertexCount();
	for (uint16_t i = 0; i < vc; i++)
	{
		auto& p = outVerts[i].pos;
		p = p.Normalized();
	}
}


} // prim
} // ui
