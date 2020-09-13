
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
