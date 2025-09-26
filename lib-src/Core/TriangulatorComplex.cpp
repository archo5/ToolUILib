
// license:
// - use for whatever for free
// - no promises that this will work for you
// - add a link to author or original file in your copy
// - report bugs and reliability improvements

#include "TriangulatorComplex.h"

#include "HashMap.h"
#include "HashSet.h"


// this triangulator attempts to support arbitrary inside/outside logic reliably
// it is expected that the resulting triangulation may not be optimal, it may be improved using other methods separately
// it also does not attempt (and is unable) to remove all types of redundant inputs - which can again be done separately


namespace ui {

struct TCVertInfoNode
{
	u32 next = TriangulatorComplex_NodeID_NULL;
	u32 polyID;
	u32 edgeID;
	float edgeQ; // original vertex: edgeID = vertexID, edgeQ = 0
};

struct TCVert
{
	Vec2f pos;
	u32 infochain;
};

struct TCEdge
{
	u32 v0, v1;
};

struct TCSplit
{
	float q;
	u32 vert;
};

struct TCOrigEdge
{
	u32 v0, v1;
	u32 polyID;
	u32 edgeID;
	// TODO figure out something better if possible
	Array<TCSplit> splits;
};

struct Counter
{
	u32 count = 0;
};

struct TCTriangle
{
	u32 a, b, c;

	static TCTriangle Make(u32 a, u32 b, u32 c)
	{
		u32 vmin = min(a, min(b, c));
		u32 vmax = max(a, max(b, c));
		u32 vmid = a != vmin && a != vmax ? a :
			b != vmin && b != vmax ? b : c;
		return { vmin, vmid, vmax };
	}

	struct HashEqualityComparer
	{
		UI_FORCEINLINE static bool AreEqual(const TCTriangle& a, const TCTriangle& b)
		{
			return a.a == b.a && a.b == b.b && a.c == b.c;
		}
		UI_FORCEINLINE static size_t GetHash(const TCTriangle& v)
		{
			return v.a | (v.b << 11) | (v.c << 22);
		}
	};
};

struct TriangulatorComplex
{
	struct CandidateSplit
	{
		u32 v0, v1;
		float distsq;
	};

	TriangulatorComplex_InsideFunc* insideFunc = &DefaultInsideFunc;
	void* infnUserdata = nullptr;

	Array<TCVert> vertices;
	Array<TCVertInfoNode> vertinfos;
	HashMap<Vec2f, u32> pos2vert;
	Array<TCOrigEdge> origedges;
	HashMap<u32, Counter> refvertidxset; // indices + reference counts of referenced vertices
	Array<TCEdge> edges;
	Array<CandidateSplit> csplits;
	Array<TCEdge> spledges;
	// TODO figure out something better if possible
	HashMap<u32, Array<TCEdge>> vert2edge;
	HashSet<TCTriangle, TCTriangle::HashEqualityComparer> proctris;
	Array<TCTriangle> triangles;

	u32 nextPolyID = 0;

	void Reset()
	{
		insideFunc = nullptr;
		infnUserdata = nullptr;
		vertices.Clear();
		vertinfos.Clear();
		pos2vert.Clear();
		origedges.Clear();
		refvertidxset.Clear();
		edges.Clear();
		csplits.Clear();
		spledges.Clear();
		vert2edge.Clear();
		proctris.Clear();
		triangles.Clear();
		nextPolyID = 0;
	}

	static bool DefaultInsideFunc(void* userdata, Vec2f point)
	{
		return true;
	}

	UI_FORCEINLINE bool Inside(Vec2f point)
	{
		return insideFunc(infnUserdata, point);
	}

	u32 InsertVertex(Vec2f pos, u32 pid, u32 eid, float edgeQ)
	{
		if (u32* pvid = pos2vert.GetValuePtr(pos))
		{
			u32 vid = *pvid;
			u32& previnfo = vertices[vid].infochain;
			u32 nextinfo = u32(vertinfos.Size());
			vertinfos.Append({ previnfo, pid, eid, edgeQ });
			previnfo = nextinfo;
			return vid;
		}
		else
		{
			u32 vid = u32(vertices.Size());
			pos2vert.Insert(pos, vid);
			vertices.Append({ pos, u32(vertinfos.Size()) });
			vertinfos.Append({ TriangulatorComplex_NodeID_NULL, pid, eid, edgeQ });
			return vid;
		}
	}

	void AddPolygon(ArrayView<Vec2f> pverts)
	{
		u32 pid = nextPolyID++;

		// introduce the data
		u32 origv0 = InsertVertex(pverts.First(), pid, 0, 0);
		u32 vprev = origv0;
		for (size_t i = 0; i < pverts.Size(); i++)
		{
			u32 v0 = vprev;
			u32 v1 = origv0;
			if (i + 1 < pverts.Size())
				v1 = InsertVertex(pverts.NextWrap(i), pid, (i + 1) % pverts.Size(), 0);
			origedges.Append({ v0, v1, pid, u32(i) });
			vprev = v1;
		}
	}

	static bool LineSegmentIntersectionQ_EE(Vec2f a0, Vec2f a1, Vec2f b0, Vec2f b1, Vec2f& isq)
	{
		float denom = (a0.x - a1.x) * (b0.y - b1.y) - (a0.y - a1.y) * (b0.x - b1.x);
		if (denom == 0)
			return false;
		float qanum = (a0.x - b0.x) * (b0.y - b1.y) - (a0.y - b0.y) * (b0.x - b1.x);
		float qbnum = (a0.x - b0.x) * (a0.y - a1.y) - (a0.y - b0.y) * (a0.x - a1.x);
		if (denom < 0)
		{
			qanum = -qanum;
			qbnum = -qbnum;
			denom = -denom;
		}
		// exclude endpoint match
		if (qanum <= 0 || qanum >= denom ||
			qbnum <= 0 || qbnum >= denom)
			return false;
		isq = { qanum / denom, qbnum / denom };
		return true;
	}

	void GenerateSplits()
	{
		// generate virtual splits
		for (size_t i = 0; i < origedges.Size(); i++)
		{
			auto& ei = origedges[i];
			auto& ei0 = vertices[ei.v0];
			auto& ei1 = vertices[ei.v1];
			for (size_t j = i + 1; j < origedges.Size(); j++)
			{
				auto& ej = origedges[j];
				auto& ej0 = vertices[ej.v0];
				auto& ej1 = vertices[ej.v1];

				Vec2f isq;
				if (LineSegmentIntersectionQ_EE(ei0.pos, ei1.pos, ej0.pos, ej1.pos, isq))
				{
					Vec2f isp = ui::Vec2fLerp(ei0.pos, ei1.pos, isq.x);
					u32 v0 = InsertVertex(isp, ei.polyID, ei.edgeID, isq.x);
					// adding metadata from the other edge to the same vertex
					u32 v0again = InsertVertex(isp, ej.polyID, ej.edgeID, isq.y);
					assert(v0 == v0again);
					ei.splits.Append({ isq.x, v0 });
					ej.splits.Append({ isq.y, v0 });
				}
			}
		}

		// sort virtual splits and turn them into real ones
		for (auto& e : origedges)
		{
			qsort(e.splits.Data(), e.splits.Size(), sizeof(*e.splits.Data()), [](const void* a, const void* b) -> int
			{
				return int(sign(static_cast<const TCSplit*>(a)->q - static_cast<const TCSplit*>(b)->q));
			});

			u32 prevvpos = e.v0;
			for (size_t i = 0; i <= e.splits.Size(); i++)
			{
				u32 nextvpos = e.v1;
				if (i < e.splits.Size())
				{
					nextvpos = e.splits[i].vert;
				}

				Vec2f midpoint = (vertices[prevvpos].pos + vertices[nextvpos].pos) * 0.5f;
				if (Inside(midpoint))
				{
					edges.Append({ prevvpos, nextvpos });
				}

				prevvpos = nextvpos;
			}
		}
	}

	void InsertReferencedVertex(u32 v)
	{
		refvertidxset[v].count++;
	}

	static bool IsSameEdge(u32 a0, u32 a1, u32 b0, u32 b1)
	{
		return (a0 == b0 && a1 == b1) || (a0 == b1 && a1 == b0);
	}

	static bool AreLineSegmentsOverlapping(Vec2f a0, Vec2f a1, Vec2f b0, Vec2f b1)
	{
		Vec2f ad = (a1 - a0).Normalized();
		Vec2f bd = (b1 - b0).Normalized();
		float dot = Vec2Dot(ad, bd);
		if (fabsf(dot) < 0.999999f)
			return false;

		Vec2f dir = (ad + bd * sign(dot)).Normalized();
		Vec2f tng = dir.Perp();
		if (fabsf(Vec2Dot(tng, a0 - b0)) > 0.00001f)
			return false;

		Rangef ar = Rangef::Exact(Vec2Dot(dir, a0));
		ar.Include(Vec2Dot(dir, a1));
		Rangef br = Rangef::Exact(Vec2Dot(dir, b0));
		br.Include(Vec2Dot(dir, b1));
		if (!ar.Overlaps(br))
			return false;

		float ovr = min(ar.max - br.min, br.max - ar.min);
		return ovr > 0.00001f;
	}

	bool IntersectsAnyOrigEdge(u32 v0, u32 v1)
	{
		Vec2f p0 = vertices[v0].pos;
		Vec2f p1 = vertices[v1].pos;

		for (auto& oe : origedges)
		{
			if (IsSameEdge(oe.v0, oe.v1, v0, v1))
				return true;
			Vec2f oep0 = vertices[oe.v0].pos;
			Vec2f oep1 = vertices[oe.v1].pos;
			if (AreLineSegmentsOverlapping(p0, p1, oep0, oep1))
				return true;
			if (oe.v0 == v0 || oe.v0 == v1 || oe.v1 == v0 || oe.v1 == v1)
				continue;
			Vec2f dummy;
			if (LineSegmentIntersectionQ_EE(p0, p1, oep0, oep1, dummy))
				return true;
		}
		return false;
	}

	bool IntersectsAnySplitEdge(u32 v0, u32 v1)
	{
		Vec2f p0 = vertices[v0].pos;
		Vec2f p1 = vertices[v1].pos;

		for (auto& se : spledges)
		{
			if (IsSameEdge(se.v0, se.v1, v0, v1))
				return true;
			Vec2f sep0 = vertices[se.v0].pos;
			Vec2f sep1 = vertices[se.v1].pos;
			if (AreLineSegmentsOverlapping(p0, p1, sep0, sep1))
				return true;
			if (se.v0 == v0 || se.v0 == v1 || se.v1 == v0 || se.v1 == v1)
				continue;
			Vec2f dummy;
			if (LineSegmentIntersectionQ_EE(p0, p1, sep0, sep1, dummy))
				return true;
		}
		return false;
	}

	static bool PointInTriangle(Vec2f p, Vec2f t0, Vec2f t1, Vec2f t2)
	{
		Vec2f d20 = t2 - t0;
		Vec2f d10 = t1 - t0;
		Vec2f tp0 = p - t0;

		float cc = Vec2Dot(d20, d20);
		float bc = Vec2Dot(d10, d20);
		float pc = Vec2Dot(d20, tp0);
		float bb = Vec2Dot(d10, d10);
		float pb = Vec2Dot(d10, tp0);

		float denom = cc * bb - bc * bc;
		float u = (bb * pc - bc * pb) / denom;
		float v = (cc * pb - bc * pc) / denom;

		return u > 0 && v > 0 && 1 - u - v > 0;
	}

	static float PointLineDistance(Vec2f p, Vec2f A, Vec2f B, Vec2f* onp = nullptr)
	{
		if (A == B)
		{
			if (onp)
				*onp = A;
			return (p - A).Length();
		}
		Vec2f dir = (B - A).Normalized();
		Vec2f tng = dir.Perp();
		float dirp = Vec2Dot(dir, p);
		float dirA = Vec2Dot(dir, A);
		float dirB = Vec2Dot(dir, B);
		float tngp = Vec2Dot(tng, p);
		float tngA = Vec2Dot(tng, A);
		float dirpc = clamp(dirp, dirA, dirB);
		if (onp)
			*onp = dir * dirpc + tng * tngA;
		return Vec2f(dirpc - dirp, tngp - tngA).Length();
	}

	void Triangulate()
	{
		GenerateSplits();

		// find referenced vertices
		for (auto& e : edges)
		{
			InsertReferencedVertex(e.v0);
			InsertReferencedVertex(e.v1);
		}

		// generate candidate splits
		for (size_t i = 0; i < refvertidxset.Size(); i++)
		{
			for (size_t j = i + 1; j < refvertidxset.Size(); j++)
			{
				u32 vi = refvertidxset._storage.GetKeyAt(i);
				u32 vj = refvertidxset._storage.GetKeyAt(j);
				float dsq = (vertices[vi].pos - vertices[vj].pos).LengthSq();
				csplits.Append({ vi, vj, dsq });
			}
		}
		qsort(csplits.Data(), csplits.Size(), sizeof(CandidateSplit), [](const void* a, const void* b) -> int
		{
			return int(sign(static_cast<const CandidateSplit*>(a)->distsq - static_cast<const CandidateSplit*>(b)->distsq));
		});

		for (auto& cs : csplits)
		{
			Vec2f p0 = vertices[cs.v0].pos;
			Vec2f p1 = vertices[cs.v1].pos;

			Vec2f midpoint = (p0 + p1) * 0.5f;
			if (!Inside(midpoint))
				continue;

			if (IntersectsAnyOrigEdge(cs.v0, cs.v1))
				continue;

			if (IntersectsAnySplitEdge(cs.v0, cs.v1))
				continue;

			spledges.Append({ cs.v0, cs.v1 });
		}

		edges.AppendRange(spledges);
		for (auto& e : edges)
		{
			vert2edge[e.v0].Append(e);
			vert2edge[e.v1].Append(e);
		}

		for (auto& E : edges)
		{
			// find all shared verts
			auto* aedges = vert2edge.GetValuePtr(E.v0);
			auto* bedges = vert2edge.GetValuePtr(E.v1);
			for (auto& AE : *aedges)
			{
				if (AE.v1 == E.v1)
					continue; // same edge as E
				for (auto& BE : *bedges)
				{
					if (BE.v0 == E.v0)
						continue; // same edge as E

					// shared third vertex
					if (AE.v0 == BE.v0 || AE.v1 == BE.v0 || AE.v0 == BE.v1 || AE.v1 == BE.v1)
					{
						u32 ovid = AE.v0 == E.v0 ? AE.v1 : AE.v0;

						Vec2f tp0 = vertices[E.v0].pos;
						Vec2f tp1 = vertices[E.v1].pos;
						Vec2f tp2 = vertices[ovid].pos;

						// check if near-0 area
						float sarea2 = Vec2Cross(tp0 - tp2, tp1 - tp2);
						if (fabsf(sarea2) < 0.0001f)
							continue;

						TCTriangle T = TCTriangle::Make(E.v0, E.v1, ovid);
						if (!proctris.Insert(T))
							continue; // already inserted

						bool anyisect = false;
						for (auto& IE : edges)
						{
							//if (IE == E || IE == AE || IE == BE)
							if (&IE == &E || &IE == &AE || &IE == &BE)
								continue;
							Vec2f mid = (vertices[IE.v0].pos + vertices[IE.v1].pos) * 0.5f;
							if (PointInTriangle(mid, tp0, tp1, tp2) &&
								// skip edge hits
								PointLineDistance(mid, tp0, tp1) >= 0.001f &&
								PointLineDistance(mid, tp1, tp2) >= 0.001f &&
								PointLineDistance(mid, tp2, tp0) >= 0.001f)
							{
								anyisect = true;
								break;
							}
						}
						if (anyisect)
							continue;

						// check if triangle (midpoint) is inside the face
						Vec2f mid = (tp0 + tp1 + tp2) * (1.f / 3.f);
						if (!Inside(mid))
							continue;

						triangles.Append(T);
					}
				}
			}
		}
	}
};

TriangulatorComplex* TriangulatorComplex_Create()
{
	return new TriangulatorComplex;
}

void TriangulatorComplex_Destroy(TriangulatorComplex* TC)
{
	delete TC;
}

void TriangulatorComplex_Reset(TriangulatorComplex* TC)
{
	TC->Reset();
}

void TriangulatorComplex_SetInsideFunc(TriangulatorComplex* TC, void* userdata, TriangulatorComplex_InsideFunc* func)
{
	if (func)
	{
		TC->insideFunc = func;
		TC->infnUserdata = userdata;
	}
	else
	{
		TC->insideFunc = &TriangulatorComplex::DefaultInsideFunc;
		TC->infnUserdata = nullptr;
	}
}

void TriangulatorComplex_AddPolygon(TriangulatorComplex* TC, ArrayView<Vec2f> verts)
{
	TC->AddPolygon(verts);
}

void TriangulatorComplex_Triangulate(TriangulatorComplex* TC)
{
	TC->Triangulate();
}

u32 TriangulatorComplex_GetVertexCount(TriangulatorComplex* TC)
{
	return u32(TC->vertices.Size());
}

bool TriangulatorComplex_GetVertexInfo(TriangulatorComplex* TC, u32 vid, Vec2f& outpos, u32& outinfochainfirstnode)
{
	if (vid >= TC->vertices.Size())
		return false;
	auto& v = TC->vertices[vid];
	outpos = v.pos;
	outinfochainfirstnode = v.infochain;
	return true;
}

bool TriangulatorComplex_GetVertexInfoNode(TriangulatorComplex* TC, u32 nodeid, TriangulatorComplex_VertexInfoNode& node)
{
	if (nodeid >= TC->vertinfos.Size())
		return false;
	auto& vi = TC->vertinfos[nodeid];
	node.nextnode = vi.next;
	node.polyID = vi.polyID;
	node.edgeID = vi.edgeID;
	node.edgeQ = vi.edgeQ;
	return true;
}

u32 TriangulatorComplex_GetTriangleCount(TriangulatorComplex* TC)
{
	return u32(TC->triangles.Size());
}

bool TriangulatorComplex_GetTriangle(TriangulatorComplex* TC, u32 tid, u32 outverts[3])
{
	if (tid >= TC->triangles.Size())
		return false;
	auto& T = TC->triangles[tid];
	Vec2f tap = TC->vertices[T.a].pos;
	Vec2f tbp = TC->vertices[T.b].pos;
	Vec2f tcp = TC->vertices[T.c].pos;
	float sarea2 = Vec2Cross(tbp - tap, tcp - tap);
	outverts[0] = T.a;
	outverts[1] = T.b;
	outverts[2] = T.c;
	if (sarea2 < 0)
		std::swap(outverts[1], outverts[2]);
	return true;
}

bool TriangulatorComplexUtil_PointInPolygon(Vec2f point, ArrayView<Vec2f> verts)
{
	size_t i, j;
	bool in = false;
	for (i = 0, j = verts.Size() - 1; i < verts.Size(); j = i++)
	{
		if (((verts[i].y > point.y) != (verts[j].y > point.y)) &&
			(point.x < (verts[j].x - verts[i].x) * (point.y - verts[i].y) / (verts[j].y - verts[i].y) + verts[i].x))
		{
			in ^= true;
		}
	}
	return in;
}

bool TriangulatorComplexUtil_PointNearPolygonEdge(Vec2f point, ArrayView<Vec2f> verts, float dist)
{
	for (size_t i = 0; i < verts.Size(); i++)
	{
		if (TriangulatorComplex::PointLineDistance(point, verts[i], verts.NextWrap(i)) <= dist)
			return true;
	}
	return false;
}

bool TriangulatorComplexUtil_PointInOrNearPolygon(Vec2f point, ArrayView<Vec2f> verts, float dist)
{
	return TriangulatorComplexUtil_PointInPolygon(point, verts) || TriangulatorComplexUtil_PointNearPolygonEdge(point, verts, dist);
}

bool TriangulatorComplexUtil_PointInButNotNearPolygon(Vec2f point, ArrayView<Vec2f> verts, float dist)
{
	return TriangulatorComplexUtil_PointInPolygon(point, verts) && !TriangulatorComplexUtil_PointNearPolygonEdge(point, verts, dist);
}

} // ui
