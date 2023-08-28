
#include "3DMath.h"


namespace ui {

Quat Quat::Normalized() const
{
	float lsq = x * x + y * y + z * z + w * w;
	if (lsq == 0)
		return { 0, 0, 0, 0 };
	float il = 1.0f / sqrtf(lsq);
	return { x * il, y * il, z * il, w * il };
}

Vec3f Quat::Axis() const
{
	return Vec3f(x, y, z).Normalized();
}

float Quat::Angle() const
{
	return acosf(clamp(w, -1.f, 1.f)) * (2 * RAD2DEG);
}

Quat Quat::operator * (const Quat& o) const
{
	const Quat& a = *this;
	const Quat& b = o;
	return
	{
		a.x * b.w + a.y * b.z - a.z * b.y + a.w * b.x,
		-a.x * b.z + a.y * b.w + a.z * b.x + a.w * b.y,
		a.x * b.y - a.y * b.x + a.z * b.w + a.w * b.z,
		-a.x * b.x - a.y * b.y - a.z * b.z + a.w * b.w,
	};
}

Vec3f Quat::ToEulerAnglesZYX() const
{
	float sinX = 2 * (w * x + y * z);
	float cosX = 1 - 2 * (x * x + y * y);
	float angleX = atan2f(sinX, cosX);

	float sinY = 2 * (w * y - z * x);
	float angleY = asinf(clamp(sinY, -1.0f, 1.0f));

	float sinZ = 2 * (w * z + x * y);
	float cosZ = 1 - 2 * (y * y + z * z);
	float angleZ = atan2f(sinZ, cosZ);

	return Vec3f(angleX, angleY, angleZ) * RAD2DEG;
}

Vec3f Quat::Rotate(Vec3f v) const
{
#if 0
	Quat p(v.x, v.y, v.z, 0);
	Quat r = Inverted() * p * *this;
	return { r.x, r.y, r.z };
#else
	Vec3f myvec(-x, -y, -z);
	return 2 * Vec3Dot(v, myvec) * myvec + (w * w - Vec3Dot(myvec, myvec)) * v + 2 * w * Vec3Cross(myvec, v);
#endif
}

Quat Quat::RotateAxisAngle(const Vec3f& axis, float angle)
{
	float har = angle * DEG2RAD * 0.5f;
	float c = cosf(har);
	float s = sinf(har);
	return { s * axis.x, s * axis.y, s * axis.z, c };
}

Quat Quat::RotateBetweenNormalDirections(const Vec3f& a, const Vec3f& b)
{
	return RotateAxisAngle(Vec3Cross(b, a).Normalized(), acosf(clamp(Vec3Dot(a, b), -1.f, 1.f)) * RAD2DEG);
}

Quat Quat::RotateBetweenDirections(const Vec3f& a, const Vec3f& b)
{
	return RotateBetweenNormalDirections(a.Normalized(), b.Normalized());
}

Quat Quat::RotateX(float angle)
{
	return RotateAxisAngle({ 1, 0, 0 }, angle);
}

Quat Quat::RotateY(float angle)
{
	return RotateAxisAngle({ 0, 1, 0 }, angle);
}

Quat Quat::RotateZ(float angle)
{
	return RotateAxisAngle({ 0, 0, 1 }, angle);
}

Quat Quat::RotateEulerAnglesXYZ(const Vec3f& angles)
{
	return RotateX(angles.x) * RotateY(angles.y) * RotateZ(angles.z);
}

Quat Quat::RotateEulerAnglesZYX(const Vec3f& angles)
{
	return RotateZ(angles.z) * RotateY(angles.y) * RotateX(angles.x);
}


Mat4f Mat4f::Translate(float x, float y, float z)
{
	return
	{
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		x, y, z, 1,
	};
}

Mat4f Mat4f::Scale(float x, float y, float z)
{
	return
	{
		x, 0, 0, 0,
		0, y, 0, 0,
		0, 0, z, 0,
		0, 0, 0, 1,
	};
}

Mat4f Mat4f::RotateX(float a)
{
	a *= 3.14159f / 180;
	float c = cosf(a);
	float s = sinf(a);
	return
	{
		1, 0, 0, 0,
		0, c, -s, 0,
		0, s, c, 0,
		0, 0, 0, 1,
	};
}

Mat4f Mat4f::RotateY(float a)
{
	a *= 3.14159f / 180;
	float c = cosf(a);
	float s = sinf(a);
	return
	{
		c, 0, s, 0,
		0, 1, 0, 0,
		-s, 0, c, 0,
		0, 0, 0, 1,
	};
}

Mat4f Mat4f::RotateZ(float a)
{
	a *= 3.14159f / 180;
	float c = cosf(a);
	float s = sinf(a);
	return
	{
		c, -s, 0, 0,
		s, c, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1,
	};
}

Mat4f Mat4f::RotateAxisAngle(const Vec3f& axis, float a)
{
	a *= 3.14159f / 180;
	float c = cosf(a);
	float s = sinf(a);
	float ic = 1 - c;
	float x = axis.x, y = axis.y, z = axis.z;
	return
	{
		c + x * x * ic, x * y * ic - z * s, x * z * ic + y * s, 0,
		y * x * ic + z * s, c + y * y * ic, y * z * ic - x * s, 0,
		z * x * ic - y * s, z * y * ic + x * s, c + z * z * ic, 0,
		0, 0, 0, 1,
	};
}

Mat4f Mat4f::Rotate(const Quat& q)
{
	float xx = q.x * q.x;
	float yy = q.y * q.y;
	float zz = q.z * q.z;
	float xy = q.x * q.y;
	float xz = q.x * q.z;
	float yz = q.y * q.z;
	float xw = q.x * q.w;
	float yw = q.y * q.w;
	float zw = q.z * q.w;

	float m00 = 1 - 2 * (yy + zz);
	float m11 = 1 - 2 * (xx + zz);
	float m22 = 1 - 2 * (xx + yy);

	float m10 = 2 * (xy - zw);
	float m01 = 2 * (xy + zw);

	float m20 = 2 * (xz + yw);
	float m02 = 2 * (xz - yw);

	float m21 = 2 * (yz - xw);
	float m12 = 2 * (yz + xw);

	return
	{
		m00, m10, m20, 0,
		m01, m11, m21, 0,
		m02, m12, m22, 0,
		0, 0, 0, 1,
	};
}

Mat4f Mat4f::LookAtDirLH(const Vec3f& pos, const Vec3f& dir, const Vec3f& up)
{
	auto z = dir.Normalized();
	auto x = Vec3Cross(up, z).Normalized();
	auto y = Vec3Cross(z, x);

	return
	{
		x.x, y.x, z.x, 0,
		x.y, y.y, z.y, 0,
		x.z, y.z, z.z, 0,
		-Vec3Dot(x, pos), -Vec3Dot(y, pos), -Vec3Dot(z, pos), 1,
	};
}

Mat4f Mat4f::LookAtDirRH(const Vec3f& pos, const Vec3f& dir, const Vec3f& up)
{
	return LookAtDirLH(pos, -dir, up);
}

Mat4f Mat4f::PerspectiveExtLH(float xscale, float yscale, float znear, float zfar)
{
	return
	{
		xscale, 0, 0, 0,
		0, yscale, 0, 0,
		0, 0, zfar / (zfar - znear), 1,
		0, 0, znear * zfar / (znear - zfar), 0,
	};
}

Mat4f Mat4f::PerspectiveExtRH(float xscale, float yscale, float znear, float zfar)
{
	return
	{
		xscale, 0, 0, 0,
		0, yscale, 0, 0,
		0, 0, zfar / (znear - zfar), -1,
		0, 0, znear * zfar / (znear - zfar), 0,
	};
}

bool Mat4f::InvertTo(Mat4f& inv) const
{
	// https://stackoverflow.com/a/44446912
	const Mat4f& m = *this;
	float A2323 = m.v22 * m.v33 - m.v23 * m.v32;
	float A1323 = m.v21 * m.v33 - m.v23 * m.v31;
	float A1223 = m.v21 * m.v32 - m.v22 * m.v31;
	float A0323 = m.v20 * m.v33 - m.v23 * m.v30;
	float A0223 = m.v20 * m.v32 - m.v22 * m.v30;
	float A0123 = m.v20 * m.v31 - m.v21 * m.v30;
	float A2313 = m.v12 * m.v33 - m.v13 * m.v32;
	float A1313 = m.v11 * m.v33 - m.v13 * m.v31;
	float A1213 = m.v11 * m.v32 - m.v12 * m.v31;
	float A2312 = m.v12 * m.v23 - m.v13 * m.v22;
	float A1312 = m.v11 * m.v23 - m.v13 * m.v21;
	float A1212 = m.v11 * m.v22 - m.v12 * m.v21;
	float A0313 = m.v10 * m.v33 - m.v13 * m.v30;
	float A0213 = m.v10 * m.v32 - m.v12 * m.v30;
	float A0312 = m.v10 * m.v23 - m.v13 * m.v20;
	float A0212 = m.v10 * m.v22 - m.v12 * m.v20;
	float A0113 = m.v10 * m.v31 - m.v11 * m.v30;
	float A0112 = m.v10 * m.v21 - m.v11 * m.v20;

	float det = m.v00 * (m.v11 * A2323 - m.v12 * A1323 + m.v13 * A1223)
		- m.v01 * (m.v10 * A2323 - m.v12 * A0323 + m.v13 * A0223)
		+ m.v02 * (m.v10 * A1323 - m.v11 * A0323 + m.v13 * A0123)
		- m.v03 * (m.v10 * A1223 - m.v11 * A0223 + m.v12 * A0123);
	if (det == 0)
		return false;
	det = 1 / det;

	inv.v00 = det * (m.v11 * A2323 - m.v12 * A1323 + m.v13 * A1223);
	inv.v01 = det * -(m.v01 * A2323 - m.v02 * A1323 + m.v03 * A1223);
	inv.v02 = det * (m.v01 * A2313 - m.v02 * A1313 + m.v03 * A1213);
	inv.v03 = det * -(m.v01 * A2312 - m.v02 * A1312 + m.v03 * A1212);
	inv.v10 = det * -(m.v10 * A2323 - m.v12 * A0323 + m.v13 * A0223);
	inv.v11 = det * (m.v00 * A2323 - m.v02 * A0323 + m.v03 * A0223);
	inv.v12 = det * -(m.v00 * A2313 - m.v02 * A0313 + m.v03 * A0213);
	inv.v13 = det * (m.v00 * A2312 - m.v02 * A0312 + m.v03 * A0212);
	inv.v20 = det * (m.v10 * A1323 - m.v11 * A0323 + m.v13 * A0123);
	inv.v21 = det * -(m.v00 * A1323 - m.v01 * A0323 + m.v03 * A0123);
	inv.v22 = det * (m.v00 * A1313 - m.v01 * A0313 + m.v03 * A0113);
	inv.v23 = det * -(m.v00 * A1312 - m.v01 * A0312 + m.v03 * A0112);
	inv.v30 = det * -(m.v10 * A1223 - m.v11 * A0223 + m.v12 * A0123);
	inv.v31 = det * (m.v00 * A1223 - m.v01 * A0223 + m.v02 * A0123);
	inv.v32 = det * -(m.v00 * A1213 - m.v01 * A0213 + m.v02 * A0113);
	inv.v33 = det * (m.v00 * A1212 - m.v01 * A0212 + m.v02 * A0112);
	return true;
}

Vec3f Mat4f::GetScale() const
{
	return Vec3f(
		Vec3f(v00, v10, v20).Length(),
		Vec3f(v10, v11, v12).Length(),
		Vec3f(v20, v21, v22).Length());
}

Mat4f Mat4f::GetRotationMatrix() const
{
	return RemoveTranslation().RemoveScale();
}

Quat Mat4f::GetRotationQuaternion() const
{
	auto mr = GetRotationMatrix();
	Quat q;
	q.x = sqrtf(max(0.0f, 1 + mr.m[0][0] - mr.m[1][1] - mr.m[2][2])) * 0.5f;
	q.y = sqrtf(max(0.0f, 1 - mr.m[0][0] + mr.m[1][1] - mr.m[2][2])) * 0.5f;
	q.z = sqrtf(max(0.0f, 1 - mr.m[0][0] - mr.m[1][1] + mr.m[2][2])) * 0.5f;
	q.w = sqrtf(max(0.0f, 1 + mr.m[0][0] + mr.m[1][1] + mr.m[2][2])) * 0.5f;
	q.x = copysignf(q.x, mr.m[2][1] - mr.m[1][2]);
	q.y = copysignf(q.y, mr.m[0][2] - mr.m[2][0]);
	q.z = copysignf(q.z, mr.m[1][0] - mr.m[0][1]);
	return q;
}

Mat4f Mat4f::RemoveTranslation() const
{
	Mat4f copy = *this;
	copy.v03 = copy.v13 = copy.v23 = 0;
	return copy;
}

Mat4f Mat4f::RemoveScale() const
{
	Mat4f copy = *this;
	for (int i = 0; i < 3; i++)
	{
		Vec3f tmp(copy.m[i][0], copy.m[i][1], copy.m[i][2]);
		tmp = tmp.Normalized();
		copy.m[i][0] = tmp.x;
		copy.m[i][1] = tmp.y;
		copy.m[i][2] = tmp.z;
	}
	return copy;
}


Transform3Df Transform3Df::FromMatrixLossy(const Mat4f& m)
{
	return { m.GetTranslation(), m.GetRotationQuaternion() };
}

void Transform3Df::OnSerialize(IObjectIterator& oi, const FieldInfo& FI)
{
	oi.BeginObject(FI, "Transform3D");
	OnField(oi, "position", position);
	OnField(oi, "rotation", rotation);
	oi.EndObject();
}

Transform3Df TransformLerp(const Transform3Df& a, const Transform3Df& b, float q)
{
	Transform3Df o;
	o.position = ui::Vec3Lerp(a.position, b.position, q);
	o.rotation = ui::QuatNLerp(a.rotation, b.rotation, q);
	return o;
}


TransformScale3Df TransformScale3Df::FromMatrixLossy(const Mat4f& m)
{
	return { m.GetTranslation(), m.GetRotationQuaternion(), m.GetScale() };
}

void TransformScale3Df::OnSerialize(IObjectIterator& oi, const FieldInfo& FI)
{
	oi.BeginObject(FI, "TransformScale3D");
	OnField(oi, "position", position);
	OnField(oi, "rotation", rotation);
	OnField(oi, "scale", scale);
	oi.EndObject();
}

Mat4f TransformScale3Df::ToMatrix() const
{
	return Mat4f::Scale(scale)
		* Mat4f::Rotate(rotation)
		* Mat4f::Translate(position);
}

TransformScale3Df TransformScale3Df::InvertedLossy() const
{
	return FromMatrixLossy(ToMatrix().Inverted());
}

TransformScale3Df TransformScale3Df::TransformLossy(const Mat4f& other) const
{
	return FromMatrixLossy(ToMatrix() * other);
}

TransformScale3Df TransformScale3Df::TransformLossy(const TransformScale3Df& other) const
{
	return TransformLossy(other.ToMatrix());
}

TransformScale3Df TransformLerp(const TransformScale3Df& a, const TransformScale3Df& b, float q)
{
	TransformScale3Df o;
	o.position = ui::Vec3Lerp(a.position, b.position, q);
	o.rotation = ui::QuatNLerp(a.rotation, b.rotation, q);
	o.scale = ui::Vec3Lerp(a.scale, b.scale, q);
	return o;
}


#if UI_BUILD_TESTS
#include "Test.h"

DEFINE_TEST_CATEGORY(3DMath, 1000);

DEFINE_TEST(3DMath, MatrixInvert)
{
	puts("TestMatrixInvert");

	Mat4f id = Mat4f::Identity();

	Mat4f m = Mat4f::LookAtLH({ 5, 5, 5 }, { 1, 2, 3 }, { 0, 0, 1 });
	Mat4f mi = m.Inverted();
	Mat4f mxmi = m * mi;
	for (int i = 0; i < 16; i++)
	{
		ASSERT_NEAR(mxmi.a[i], id.a[i]);
	}

	m = Mat4f::PerspectiveFOVRH(45, 1.333f, 0.123f, 456.f);
	mi = m.Inverted();
	mxmi = m * mi;
	for (int i = 0; i < 16; i++)
	{
		ASSERT_NEAR(mxmi.a[i], id.a[i]);
	}
}

DEFINE_TEST(3DMath, TransformInvert)
{
	puts("TestTransformInvert");

	Transform3Df a({ 1, 2, 3 }, Quat::RotateX(90));
	Transform3Df inv = a.Inverted();
	Transform3Df exp({ -1, 3, -2 }, Quat::RotateX(-90));
	ASSERT_NEAR(inv.position.x, exp.position.x);
	ASSERT_NEAR(inv.position.y, exp.position.y);
	ASSERT_NEAR(inv.position.z, exp.position.z);
	ASSERT_NEAR(inv.rotation.x, exp.rotation.x);
	ASSERT_NEAR(inv.rotation.y, exp.rotation.y);
	ASSERT_NEAR(inv.rotation.z, exp.rotation.z);
	ASSERT_NEAR(inv.rotation.w, exp.rotation.w);
	auto rt468 = inv.TransformPoint(a.TransformPoint({ 4, 6, 8 }));
	ASSERT_NEAR(rt468.x, 4);
	ASSERT_NEAR(rt468.y, 6);
	ASSERT_NEAR(rt468.z, 8);
	auto zt = a * inv;
	ASSERT_NEAR(zt.position.x, 0);
	ASSERT_NEAR(zt.position.y, 0);
	ASSERT_NEAR(zt.position.z, 0);
	ASSERT_NEAR(zt.rotation.x, 0);
	ASSERT_NEAR(zt.rotation.y, 0);
	ASSERT_NEAR(zt.rotation.z, 0);
	ASSERT_NEAR(zt.rotation.w, 1);
}

DEFINE_TEST(3DMath, TransformConsistency)
{
	puts("TestTransformConsistency");

	// quat / matrix rotation
	{
		auto m1 = Mat4f::Rotate(Quat::RotateAxisAngle({ 1, 0, 0 }, 90));
		auto m2 = Mat4f::RotateX(90);
		for (int i = 0; i < 16; i++)
		{
			ASSERT_NEAR(m1.a[i], m2.a[i]);
		}
	}

	// quat / matrix vector transform
	{
		Vec3f v(1, 2, 3);
		auto q1 = Quat::RotateAxisAngle({ 1, 0, 0 }, 90);
		auto m2 = Mat4f::RotateX(90);
		auto xfv1 = q1.Rotate(v);
		auto xfv2 = m2.TransformDirection(v);
		ASSERT_NEAR(xfv1.x, xfv2.x);
		ASSERT_NEAR(xfv1.y, xfv2.y);
		ASSERT_NEAR(xfv1.z, xfv2.z);
	}
}

DEFINE_TEST(3DMath, RayPlaneIntersect)
{
	puts("TestRayPlaneIntersect");

	RayPlaneIntersectResult rpir;
	rpir = RayPlaneIntersect({ 0, 0, 1 }, { 0, 0, -1 }, { 0, 0, 1, 0 });
	ASSERT_NEAR(rpir.dist, 1);
	ASSERT_NEAR(rpir.angcos, 1);

	rpir = RayPlaneIntersect({ 0, 0, 1 }, { 0, 0, 1 }, { 0, 0, 1, 0 });
	ASSERT_NEAR(rpir.dist, -1);
	ASSERT_NEAR(rpir.angcos, -1);

	rpir = RayPlaneIntersect({ 0, 0, 1 }, { 0, 1, 0 }, { 0, 0, 1, 0 });
	ASSERT_NEAR(rpir.dist, 0);
	ASSERT_NEAR(rpir.angcos, 0);

	rpir = RayPlaneIntersect({ 0, 0, 1 }, Vec3f{ 0, sqrtf(3), -1 }.Normalized(), { 0, 0, 1, 0 });
	ASSERT_NEAR(rpir.dist, 2);
	ASSERT_NEAR(rpir.angcos, cosf(3.14159f / 3));
}
#endif

} // ui
