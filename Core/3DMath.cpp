
#include "3DMath.h"


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
	auto z = -dir.Normalized();
	auto x = Vec3Cross(up, z).Normalized();
	auto y = Vec3Cross(z, x);

	return
	{
		x.x, y.x, z.x, 0,
		x.y, y.y, z.y, 0,
		x.z, y.z, z.z, 0,
		Vec3Dot(x, pos), Vec3Dot(y, pos), Vec3Dot(z, pos), 1,
	};
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


#if 0
#include <stdio.h>
#include <stdlib.h>
#define ASSERT_NEAR(a, b) if (fabsf(float(a) - float(b)) > 0.0001f) \
	printf("%d: ERROR (" #a " near " #b "): %g is not near %g\n", __LINE__, float(a), float(b))
struct Test3DMath
{
	Test3DMath()
	{
		TestMatrixInvert();
		TestRayPlaneIntersect();
		exit(0);
	}

	void TestMatrixInvert()
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

	void TestRayPlaneIntersect()
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
}
g_tests;
#endif
