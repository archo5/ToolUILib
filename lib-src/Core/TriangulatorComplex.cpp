
// license:
// - use for whatever for free
// - no promises that this will work for you
// - add a link to author or original file in your copy
// - report bugs and reliability improvements

#include "TriangulatorComplex.h"

#include "HashMap.h"
#include "HashSet.h"

#include "FileSystem.h"
#include "SerializationJSON.h"
#define SF(x) #x, x


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

	void OnSerialize(IObjectIterator& oi, const FieldInfo& FI)
	{
		oi.BeginObject(FI, "TCVertInfoNode");
		ui::OnField(oi, SF(next));
		ui::OnField(oi, SF(polyID));
		ui::OnField(oi, SF(edgeID));
		ui::OnField(oi, SF(edgeQ));
		oi.EndObject();
	}
};

struct TCVert
{
	Vec2f pos;
	u32 infochain;

	void OnSerialize(IObjectIterator& oi, const FieldInfo& FI)
	{
		oi.BeginObject(FI, "TCVert");
		ui::OnField(oi, SF(pos));
		ui::OnField(oi, SF(infochain));
		oi.EndObject();
	}
};

struct TCEdge
{
	u32 v0, v1;

	void OnSerialize(IObjectIterator& oi, const FieldInfo& FI)
	{
		oi.BeginObject(FI, "TCEdge");
		ui::OnField(oi, SF(v0));
		ui::OnField(oi, SF(v1));
		oi.EndObject();
	}
};

struct TCSplit
{
	float q;
	u32 vert;

	void OnSerialize(IObjectIterator& oi, const FieldInfo& FI)
	{
		oi.BeginObject(FI, "TCSplit");
		ui::OnField(oi, SF(q));
		ui::OnField(oi, SF(vert));
		oi.EndObject();
	}
};

struct TCOrigEdge
{
	u32 v0, v1;
	u32 polyID;
	u32 edgeID;
	// TODO figure out something better if possible
	Array<TCSplit> splits;

	void OnSerialize(IObjectIterator& oi, const FieldInfo& FI)
	{
		oi.BeginObject(FI, "TCOrigEdge");
		ui::OnField(oi, SF(v0));
		ui::OnField(oi, SF(v1));
		ui::OnField(oi, SF(polyID));
		ui::OnField(oi, SF(edgeID));
		ui::OnField(oi, SF(splits));
		oi.EndObject();
	}

	void SortSplits()
	{
		qsort(splits.Data(), splits.Size(), sizeof(*splits.Data()), [](const void* a, const void* b) -> int
		{
			return int(sign(static_cast<const TCSplit*>(a)->q - static_cast<const TCSplit*>(b)->q));
		});
	}
};

struct Counter
{
	u32 count = 0;

	void OnSerialize(IObjectIterator& oi, const FieldInfo& FI)
	{
		oi.BeginObject(FI, "Counter");
		ui::OnField(oi, SF(count));
		oi.EndObject();
	}
};

struct TCTriangle
{
	u32 a, b, c;

	void OnSerialize(IObjectIterator& oi, const FieldInfo& FI)
	{
		oi.BeginObject(FI, "TCTriangle");
		ui::OnField(oi, SF(a));
		ui::OnField(oi, SF(b));
		ui::OnField(oi, SF(c));
		oi.EndObject();
	}

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

struct CutterSharedBase
{
	Array<TCVert> vertices;
	Array<TCVertInfoNode> vertinfos;
	HashMap<Vec2f, u32> pos2vert;
	Array<TCOrigEdge> origedges;

	u32 nextPolyID = 0;

	void ResetBase()
	{
		vertices.Clear();
		vertinfos.Clear();
		pos2vert.Clear();
		origedges.Clear();
		nextPolyID = 0;
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

	void AddPathOrPolygon(ArrayView<Vec2f> pverts, bool closed)
	{
		u32 pid = nextPolyID++;

		// introduce the data
		u32 origv0 = InsertVertex(pverts.First(), pid, 0, 0);
		u32 vprev = origv0;
		for (size_t i = 0; i + !closed < pverts.Size(); i++)
		{
			u32 v0 = vprev;
			u32 v1 = origv0;
			if (i + 1 < pverts.Size())
				v1 = InsertVertex(pverts.NextWrap(i), pid, (i + 1) % pverts.Size(), 0);
			origedges.Append({ v0, v1, pid, u32(i) });
			vprev = v1;
		}
	}

	static bool IsSameEdge(u32 a0, u32 a1, u32 b0, u32 b1)
	{
		return (a0 == b0 && a1 == b1) || (a0 == b1 && a1 == b0);
	}

	static Vec2f PointLineDistanceQ(Vec2f p, Vec2f A, Vec2f B)
	{
		if (A == B)
			return { (p - A).Length(), 0.f };
		Vec2f dir = (B - A).Normalized();
		Vec2f tng = dir.Perp();
		float dirp = Vec2Dot(dir, p);
		float dirA = Vec2Dot(dir, A);
		float dirB = Vec2Dot(dir, B);
		float tngp = Vec2Dot(tng, p);
		float tngA = Vec2Dot(tng, A);
		float dirpc = clamp(dirp, dirA, dirB);
		return { Vec2f(dirpc - dirp, tngp - tngA).Length(), invlerpc(dirA, dirB, dirp) };
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
		size_t numinitverts = vertices.Size();

		// generate virtual splits
		for (size_t i = 0; i < origedges.Size(); i++)
		{
			auto& ei = origedges[i];
			Vec2f ei0pos = vertices[ei.v0].pos;
			Vec2f ei1pos = vertices[ei.v1].pos;
			//Vec2f eid = (ei1pos - ei0pos).Normalized();

			//for (u32 op : origpoints)
			for (u32 op = 0; op < numinitverts; op++)
			{
				if (ei.v0 == op || ei.v1 == op)
					continue;
				Vec2f oppos = vertices[op].pos;
				Vec2f dq = PointLineDistanceQ(oppos, ei0pos, ei1pos);
				if (dq.x < 0.0001f && dq.y > 0.0001f && dq.y < 0.9999f)
				{
					u32 vagain = InsertVertex(oppos, ei.polyID, ei.edgeID, dq.y);
					assert(op == vagain);
					ei.splits.Append({ dq.y, op });
				}
			}

			for (size_t j = i + 1; j < origedges.Size(); j++)
			{
				auto& ej = origedges[j];
				if (ei.v0 == ej.v0 || ei.v0 == ej.v1 || ei.v1 == ej.v0 || ei.v1 == ej.v1)
					continue;
				bool alreadySplitting = false;
				for (auto& split : ei.splits)
				{
					if (split.vert == ej.v0 || split.vert == ej.v1)
					{
						alreadySplitting = true;
						break;
					}
				}
				if (alreadySplitting)
					continue;
				Vec2f ej0pos = vertices[ej.v0].pos;
				Vec2f ej1pos = vertices[ej.v1].pos;
#if 0
				LineSegmentOverlapInfo lso;
				if (AreLineSegmentsOverlapping(ei0pos, ei1pos, eid, ej0pos, ej1pos, &lso))
				{
					// overlap point 0
					{
						Vec2f isp = ui::Vec2fLerp(ei0pos, ei1pos, lso.aq0);
						if (lso.aq0 > 0 && lso.aq0 < 1)
							ei.splits.Append({ lso.aq0, InsertVertex(isp, ei.polyID, ei.edgeID, lso.aq0) });
						if (lso.bq0 > 0 && lso.bq0 < 1)
							ej.splits.Append({ lso.bq0, InsertVertex(isp, ej.polyID, ej.edgeID, lso.bq0) });
					}
					// overlap point 1
					{
						Vec2f isp = ui::Vec2fLerp(ei0pos, ei1pos, lso.aq1);
						if (lso.aq1 > 0 && lso.aq1 < 1)
							ei.splits.Append({ lso.aq1, InsertVertex(isp, ei.polyID, ei.edgeID, lso.aq1) });
						if (lso.bq1 > 0 && lso.bq1 < 1)
							ej.splits.Append({ lso.bq1, InsertVertex(isp, ej.polyID, ej.edgeID, lso.bq1) });
					}
					continue;
				}
#endif
				Vec2f isq;
				if (LineSegmentIntersectionQ_EE(ei0pos, ei1pos, ej0pos, ej1pos, isq))
				{
					Vec2f isp = ui::Vec2fLerp(ei0pos, ei1pos, isq.x);
					u32 v0 = InsertVertex(isp, ei.polyID, ei.edgeID, isq.x);
					// adding metadata from the other edge to the same vertex
					u32 v0again = InsertVertex(isp, ej.polyID, ej.edgeID, isq.y);
					assert(v0 == v0again);
					ei.splits.Append({ isq.x, v0 });
					ej.splits.Append({ isq.y, v0 });
				}
			}
		}
	}
};

struct TriangulatorComplex : CutterSharedBase
{
	struct CandidateSplit
	{
		u32 v0, v1;
		float distsq;

		void OnSerialize(IObjectIterator& oi, const FieldInfo& FI)
		{
			oi.BeginObject(FI, "TriangulatorComplex::CandidateSplit");
			ui::OnField(oi, SF(v0));
			ui::OnField(oi, SF(v1));
			ui::OnField(oi, SF(distsq));
			oi.EndObject();
		}
	};

	TriangulatorComplex_InsideFunc* insideFunc = &DefaultInsideFunc;
	void* infnUserdata = nullptr;

	//Array<u32> origpoints;
	HashMap<u32, Counter> refvertidxset; // indices + reference counts of referenced vertices
	Array<TCEdge> edges;
	Array<CandidateSplit> csplits;
	Array<TCEdge> spledges;
	// TODO figure out something better if possible
	HashMap<u32, Array<TCEdge>> vert2edge;
	HashSet<TCTriangle, TCTriangle::HashEqualityComparer> proctris;
	Array<TCTriangle> triangles;

	void Reset()
	{
		insideFunc = nullptr;
		infnUserdata = nullptr;
		ResetBase();
		//origpoints.Clear();
		refvertidxset.Clear();
		edges.Clear();
		csplits.Clear();
		spledges.Clear();
		vert2edge.Clear();
		proctris.Clear();
		triangles.Clear();
	}

	void OnSerialize(IObjectIterator& oi)
	{
		// optimized for extensibility, not speed
		OnField(oi, SF(vertices));
		OnField(oi, SF(vertinfos));
		OnField(oi, SF(pos2vert));
		OnField(oi, SF(origedges));
		//OnField(oi, SF(origpoints));
		OnField(oi, SF(refvertidxset));
		OnField(oi, SF(edges));
		OnField(oi, SF(csplits));
		OnField(oi, SF(spledges));
		OnField(oi, SF(vert2edge));
		OnField(oi, SF(proctris));
		OnField(oi, SF(triangles));
		OnField(oi, SF(nextPolyID));
	}

	static bool DefaultInsideFunc(void* userdata, Vec2f point)
	{
		return true;
	}

	UI_FORCEINLINE bool Inside(Vec2f point)
	{
		return insideFunc(infnUserdata, point);
	}

	void AddPoints(ArrayView<Vec2f> points)
	{
		u32 pid = nextPolyID++;

		for (size_t i = 0; i < points.Size(); i++)
		{
			u32 v = InsertVertex(points[i], pid, u32(i), 0);
			InsertReferencedVertex(v);
			//origpoints.Append(v);
		}
	}

	void InsertReferencedVertex(u32 v)
	{
		refvertidxset[v].count++;
	}

	void InstantiateSplits()
	{
		// sort virtual splits and turn them into real ones
		for (auto& e : origedges)
		{
			e.SortSplits();

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
					bool exists = false;
					for (auto& E : edges)
					{
						if (IsSameEdge(E.v0, E.v1, prevvpos, nextvpos))
						{
							exists = true;
							break;
						}
					}
					if (!exists)
						edges.Append({ prevvpos, nextvpos });
				}

				prevvpos = nextvpos;
			}
		}
	}

	struct LineSegmentOverlapInfo
	{
		float aq0, aq1;
		float bq0, bq1;
	};
	static bool AreLineSegmentsOverlapping(Vec2f a0, Vec2f a1, Vec2f ad, Vec2f b0, Vec2f b1, LineSegmentOverlapInfo* overlapinfo = nullptr)
	{
		//Vec2f ad = (a1 - a0).Normalized();
		Vec2f bd = (b1 - b0).Normalized();
		float dot = Vec2Dot(ad, bd);
		if (fabsf(dot) < 0.999999f)
			return false;

		Vec2f dir = (ad + bd * sign(dot)).Normalized();
		Vec2f tng = dir.Perp();
		if (fabsf(Vec2Dot(tng, a0 - b0)) > 0.00001f)
			return false;

		float ap0 = Vec2Dot(dir, a0);
		float ap1 = Vec2Dot(dir, a1);
		Rangef ar = Rangef::Exact(ap0);
		ar.Include(ap1);

		float bp0 = Vec2Dot(dir, b0);
		float bp1 = Vec2Dot(dir, b1);
		Rangef br = Rangef::Exact(bp0);
		br.Include(bp1);
		if (!ar.Overlaps(br))
			return false;

		float ovr = min(ar.max - br.min, br.max - ar.min);
		if (ovr <= 0.00001f)
			return false;

		if (overlapinfo)
		{
			float op0 = min(ar.max, br.max);
			float op1 = max(ar.min, br.min);
			overlapinfo->aq0 = invlerpc(ap0, ap1, op0);
			overlapinfo->bq0 = invlerpc(bp0, bp1, op0);
			overlapinfo->aq1 = invlerpc(ap0, ap1, op1);
			overlapinfo->bq1 = invlerpc(bp0, bp1, op1);
		}
		return true;
	}

	bool IntersectsAnyOrigEdge(u32 v0, u32 v1)
	{
		Vec2f p0 = vertices[v0].pos;
		Vec2f p1 = vertices[v1].pos;
		Vec2f pd = (p1 - p0).Normalized();

		for (auto& oe : origedges)
		{
			if (IsSameEdge(oe.v0, oe.v1, v0, v1))
				return true;

			bool connected = false;
			if (oe.v0 == v0 || oe.v0 == v1 || oe.v1 == v0 || oe.v1 == v1)
				connected = true;
			if (!connected)
			{
				for (auto& oes : oe.splits)
				{
					if (oes.vert == v0 || oes.vert == v1)
					{
						connected = true;
						break;
					}
				}
			}
			Vec2f oep0 = vertices[oe.v0].pos;
			Vec2f oep1 = vertices[oe.v1].pos;
			if (connected)
			{
				// more complex overlap test (area of triangle)
				Vec2f vecfe = p1 - p0;
				Vec2f vecoe = oep1 - oep0;
				float area2x = Vec2Cross(vecfe, vecoe);
				if (fabsf(area2x) < 1e-6f)
					if (AreLineSegmentsOverlapping(p0, p1, pd, oep0, oep1))
						return true;
				continue;
			}

			if (AreLineSegmentsOverlapping(p0, p1, pd, oep0, oep1))
				return true;

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
		Vec2f pd = (p1 - p0).Normalized();

		for (auto& se : spledges)
		{
			if (IsSameEdge(se.v0, se.v1, v0, v1))
				return true;
			Vec2f sep0 = vertices[se.v0].pos;
			Vec2f sep1 = vertices[se.v1].pos;
			if (AreLineSegmentsOverlapping(p0, p1, pd, sep0, sep1))
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
		InstantiateSplits();

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
					u32 ovA = AE.v0 == E.v0 ? AE.v1 : AE.v0;
					u32 ovB = BE.v0 == E.v1 ? BE.v1 : BE.v0;
					if (ovA == ovB)
					{
						u32 ovid = ovA;

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

bool TriangulatorComplex_LoadFromFile(TriangulatorComplex* TC, StringView path)
{
	auto frr = ReadTextFile(path);
	if (!frr.data)
		return false;
	JSONUnserializerObjectIterator u;
	if (!u.Parse(frr.data->GetStringView()))
		return false;
	TC->OnSerialize(u);
	return true;
}

bool TriangulatorComplex_SaveToFile(TriangulatorComplex* TC, StringView path)
{
	JSONSerializerObjectIterator s;
	TC->OnSerialize(s);
	return WriteTextFile(path, s.GetData());
}

void TriangulatorComplex_SaveToFileAutoPath(TriangulatorComplex* TC)
{
	char path[48];
	static u32 inc = 0;
	snprintf(path, 32, "TC_%" PRIu64 "_%" PRIu32 ".json", GetTimeUnixMS(), ++inc);
	TriangulatorComplex_SaveToFile(TC, path);
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
	TC->AddPathOrPolygon(verts, true);
}

void TriangulatorComplex_AddPoints(TriangulatorComplex* TC, ArrayView<Vec2f> points)
{
	TC->AddPoints(points);
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



struct PCEdge
{
	u32 v0, v1;
	bool lin, rin; // left/right is inside
};

struct PolygonCutterImpl : PolygonCutter, CutterSharedBase
{
	Array<i8> polyCats;
	Array<Array<Vec2f>> polygons;
	Array<PCEdge> edges;
	HashMap<u32, Array<PCEdge*>> vert2edge;

	u8 mode = 0;
	bool keepInnerEdges = true;

	void Reset(u8 mode, bool keepInnerEdges)
	{
		ResetBase();

		polyCats.Clear();
		polygons.Clear();
		edges.Clear();
		vert2edge.Clear();

		this->mode = mode;
		this->keepInnerEdges = keepInnerEdges;
	}

	bool IsInsidePolygon(i8 cat, Vec2f point)
	{
		for (size_t i = 0; i < polygons.Size(); i++)
		{
			if (polyCats[i] != cat)
				continue;
			if (TriangulatorComplexUtil_PointInOrNearPolygon(point, polygons[i], 0.0001f))
				return true;
		}
		return false;
	}

	bool NotInsidePolygon(i8 cat, Vec2f point)
	{
		for (size_t i = 0; i < polygons.Size(); i++)
		{
			if (polyCats[i] != cat)
				continue;
			if (TriangulatorComplexUtil_PointInButNotNearPolygon(point, polygons[i], 0.0001f))
				return false;
		}
		return true;
	}

	bool Inside(Vec2f point, u32 assocEdgePolyID)
	{
		// skip all polygon edges if combining - all are inside
		if (mode == Mode_Combine)
		{
			if (assocEdgePolyID < polyCats.Size() && polyCats[assocEdgePolyID] >= 0)
				return true;
			for (size_t i = 0; i < polygons.Size(); i++)
			{
				if (polyCats[i] < 0)
					continue;
				if (TriangulatorComplexUtil_PointInOrNearPolygon(point, polygons[i], 0.0001f))
					return true;
			}
			return false;
		}
		else if (mode == Mode_Subtract)
			return NotInsidePolygon(1, point) && IsInsidePolygon(0, point);
		else if (mode == Mode_Intersect)
			return IsInsidePolygon(0, point) && IsInsidePolygon(1, point);

		return false;
	}

	void InstantiateSplits()
	{
		// sort virtual splits and turn them into real ones
		for (auto& e : origedges)
		{
			e.SortSplits();

			u32 prevvpos = e.v0;
			for (size_t i = 0; i <= e.splits.Size(); i++)
			{
				u32 nextvpos = e.v1;
				if (i < e.splits.Size())
				{
					nextvpos = e.splits[i].vert;
				}

				Vec2f midpoint = (vertices[prevvpos].pos + vertices[nextvpos].pos) * 0.5f;
				if (Inside(midpoint, e.polyID))
				{
					bool exists = false;
					for (auto& E : edges)
					{
						if (IsSameEdge(E.v0, E.v1, prevvpos, nextvpos))
						{
							exists = true;
							break;
						}
					}
					if (!exists)
					{
						Vec2f dir = (vertices[nextvpos].pos - vertices[prevvpos].pos).Normalized();
						Vec2f lfpt = midpoint + dir.Perp2() * 0.0002f;
						Vec2f rtpt = midpoint + dir.Perp() * 0.0002f;
						bool lin = Inside(lfpt, UINT32_MAX);
						bool rin = Inside(rtpt, UINT32_MAX);
						edges.Append({ prevvpos, nextvpos, lin, rin });
					}
				}

				prevvpos = nextvpos;
			}
		}
	}

	u32 FindFirstInnerEdge()
	{
		for (u32 i = 0; i < edges.Size(); i++)
			if (edges[i].lin && edges[i].rin)
				return i;
		return UINT32_MAX;
	}

	u32 FindFirstRelevantEdge()
	{
		for (u32 i = 0; i < edges.Size(); i++)
			if (edges[i].lin || edges[i].rin)
				return i;
		return UINT32_MAX;
	}

	u32 FindNextEdge(u32 curvert, u32 fromedge)
	{
		auto& E0 = edges[fromedge];
		u32 fromvert = curvert == E0.v0 ? E0.v1 : E0.v0;
		float fromangle = (vertices[fromvert].pos - vertices[curvert].pos).Angle2();
		if (auto* vedges = vert2edge.GetValuePtr(curvert))
		{
			if (vedges->Size() < 2)
				return UINT32_MAX;
			float minangle = FLT_MAX;
			u32 minedge = UINT32_MAX;
			for (auto* E : *vedges)
			{
				if (E == &edges[fromedge]) // came from this edge, can't go back
					continue;
				if (!E->lin && !E->rin) // edge downgraded to no longer used
					continue;
				u32 nextvert = E->v0 == curvert ? E->v1 : E->v0;
				float fwdangle = (vertices[nextvert].pos - vertices[curvert].pos).Angle2();
				float angle = ui::AngleNormalize360(fwdangle - fromangle);
				if (angle <= 0)
					angle += 360;
				if (angle < minangle)
				{
					minedge = E - edges.Data();
					minangle = angle;
				}
			}
			return minedge;
		}
		return UINT32_MAX;
	}

	void DowngradeEdge(u32 ei)
	{
		auto& E = edges[ei];
		if (E.lin)
		{
			E.lin = false;
			return;
		}
		E.rin = false;
	}

	void EmitPolygonAndDowngradeEdgeLoop(u32 ei, PolygonOutput& output)
	{
		size_t origvcount = output.verts.Size();
		output.polygons.Append({ origvcount, origvcount });
		auto& E0 = edges[ei];
		output.verts.Append({ vertices[E0.v0].pos, vertices[E0.v0].infochain });
		u32 lastvert = E0.v0;
		u32 curvert = E0.v1;
		u32 fromedge = ei;
		DowngradeEdge(fromedge);
		while (curvert != lastvert)
		{
			output.polygons.Last().lastVertex++;
			output.verts.Append({ vertices[curvert].pos, vertices[curvert].infochain });
			u32 nextedge = FindNextEdge(curvert, fromedge);
			if (nextedge == UINT32_MAX)
			{
				output.verts.Resize(origvcount);
				output.polygons.RemoveLast();
				return;
			}
			DowngradeEdge(nextedge);
			auto& EN = edges[nextedge];
			u32 nextvert = EN.v0 == curvert ? EN.v1 : EN.v0;
			curvert = nextvert;
			fromedge = nextedge;
		}
	}

	void Cut(PolygonOutput& output)
	{
		GenerateSplits();
		InstantiateSplits();

		// remove non-participating edges
		for (size_t i = 0; i < edges.Size(); i++)
		{
			auto& E = edges[i];
			bool keep = true;
			if (!E.lin && !E.rin)
				keep = false;
			else if (!keepInnerEdges && E.lin && E.rin)
				keep = false;
			if (!keep)
				edges.UnorderedRemoveAt(i--);
		}

		// index edges
		for (auto& E : edges)
		{
			vert2edge[E.v0].Append(&E);
			vert2edge[E.v1].Append(&E);
		}

		output.verts.Clear();
		output.polygons.Clear();

		// first traverse inner edges using same direction nearest angle rule
		for (;;)
		{
			size_t ei = FindFirstInnerEdge();
			if (ei == SIZE_MAX)
				break;

			EmitPolygonAndDowngradeEdgeLoop(ei, output);
		}

		// once no inner edges exist, walk the remaining polygon(s)
		for (;;)
		{
			size_t ei = FindFirstRelevantEdge();
			if (ei == SIZE_MAX)
				break;

			EmitPolygonAndDowngradeEdgeLoop(ei, output);
		}

		// copy over metadata
		output.infos.Clear();
		output.infos.Reserve(vertinfos.Size());
		for (auto& info : vertinfos)
		{
			output.infos.Append({ info.next, info.polyID, info.edgeID, info.edgeQ });
		}
	}
};

PolygonCutter* PolygonCutter::Create()
{
	return new PolygonCutterImpl;
}

void PolygonCutter::Destroy()
{
	delete static_cast<PolygonCutterImpl*>(this);
}

void PolygonCutter::Reset(u8 mode, bool keepInnerEdges)
{
	static_cast<PolygonCutterImpl*>(this)->Reset(mode, keepInnerEdges);
}

void PolygonCutter::AddPolygonA(ArrayView<Vec2f> poly)
{
	auto* PCI = static_cast<PolygonCutterImpl*>(this);
	PCI->polyCats.Append(0);
	PCI->polygons.Append(poly);
	PCI->AddPathOrPolygon(poly, true);
}

void PolygonCutter::AddPolygonB(ArrayView<Vec2f> poly)
{
	auto* PCI = static_cast<PolygonCutterImpl*>(this);
	PCI->polyCats.Append(1);
	PCI->polygons.Append(poly);
	PCI->AddPathOrPolygon(poly, true);
}

void PolygonCutter::AddCuttingPath(ArrayView<Vec2f> path)
{
	auto* PCI = static_cast<PolygonCutterImpl*>(this);
	PCI->polyCats.Append(-1);
	PCI->polygons.Append({});
	PCI->AddPathOrPolygon(path, false);
}

void PolygonCutter::Cut(PolygonOutput& output)
{
	static_cast<PolygonCutterImpl*>(this)->Cut(output);
}

} // ui
