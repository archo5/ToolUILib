
#include "3DCamera.h"

#include "../Model/Events.h"


namespace ui {

void CameraBase::_UpdateViewProjMatrix()
{
	_mtxViewProj = _mtxView * _mtxProj;
	_mtxInvViewProj = _mtxViewProj.Inverted();
}

void CameraBase::SetViewMatrix(const Mat4f& m)
{
	_mtxView = m;
	_mtxInvView = m.Inverted();
	_UpdateViewProjMatrix();
}

void CameraBase::SetProjectionMatrix(const Mat4f& m)
{
	_mtxProj = m;
	_mtxInvProj = m.Inverted();
	_UpdateViewProjMatrix();
}

void CameraBase::SetWindowRect(const AABB2f& rect)
{
	_windowRect = rect;
}

Point2f CameraBase::WindowToNormalizedPoint(Point2f p) const
{
	auto& cr = _windowRect;
	return
	{
		lerp(-1, 1, invlerp(cr.x0, cr.x1, p.x)),
		lerp(1, -1, invlerp(cr.y0, cr.y1, p.y)),
	};
}

Point2f CameraBase::NormalizedToWindowPoint(Point2f p) const
{
	auto& cr = _windowRect;
	return
	{
		lerp(cr.x0, cr.x1, invlerp(-1, 1, p.x)),
		lerp(cr.y0, cr.y1, invlerp(1, -1, p.y)),
	};
}

Point2f CameraBase::WorldToNormalizedPoint(const Vec3f& p) const
{
	auto np = GetViewProjectionMatrix().TransformPoint(p);
	return { np.x, np.y };
}

Point2f CameraBase::WorldToWindowPoint(const Vec3f& p) const
{
	return NormalizedToWindowPoint(WorldToNormalizedPoint(p));
}

float CameraBase::WorldToWindowSize(float size, const Vec3f& refp) const
{
	float dist = GetViewMatrix().TransformPoint(refp).z;
	return size * GetWindowRect().GetHeight() * GetProjectionMatrix().m[1][1] * 0.5f / dist;
}

float CameraBase::WindowToWorldSize(float size, const Vec3f& refp) const
{
	return size / WorldToWindowSize(1.0f, refp);
}

Ray3f CameraBase::GetRayNP(Point2f p) const
{
	return GetCameraRay(GetInverseViewProjectionMatrix(), p.x, p.y);
}

Ray3f CameraBase::GetViewRayNP(Point2f p) const
{
	return GetCameraRay(GetInverseProjectionMatrix(), p.x, p.y);
}

Ray3f CameraBase::GetLocalRayNP(Point2f p, const Mat4f& world2local) const
{
	return GetCameraRay(_mtxInvProj * (_mtxInvView * world2local), p.x, p.y);
}

Ray3f CameraBase::GetRayWP(Point2f p) const
{
	return GetRayNP(WindowToNormalizedPoint(p));
}

Ray3f CameraBase::GetViewRayWP(Point2f p) const
{
	return GetViewRayNP(WindowToNormalizedPoint(p));
}

Ray3f CameraBase::GetLocalRayWP(Point2f p, const Mat4f& world2local) const
{
	return GetLocalRayNP(WindowToNormalizedPoint(p), world2local);
}

Ray3f CameraBase::GetRayEP(const Event& e) const
{
	return GetRayNP(WindowToNormalizedPoint(e.position));
}

Ray3f CameraBase::GetViewRayEP(const Event& e) const
{
	return GetViewRayNP(WindowToNormalizedPoint(e.position));
}

Ray3f CameraBase::GetLocalRayEP(const Event& e, const Mat4f& world2local) const
{
	return GetLocalRayNP(WindowToNormalizedPoint(e.position), world2local);
}


OrbitCamera::OrbitCamera(bool rh) : rightHanded(rh)
{
	_UpdateViewMatrix();
}

// avoids bringing in a whole header just for one function
AABB2f UIObject_GetFinalRect(UIObject* obj);

bool OrbitCamera::OnEvent(Event& e, const AABB2f* rect)
{
	if (e.IsPropagationStopped())
		return false;
	if (e.type == EventType::ButtonDown && (!rect || rect->Contains(e.topLevelPosition)))
	{
		if (e.GetButton() == rotateButton)
		{
			rotating = true;
			e.StopPropagation();
		}
		else if (e.GetButton() == panButton)
		{
			panning = true;
			e.StopPropagation();
		}
	}
	if (e.type == EventType::ButtonUp)
	{
		if (e.GetButton() == rotateButton)
		{
			rotating = false;
		}
		else if (e.GetButton() == panButton)
		{
			panning = false;
		}
	}
	if (e.type == EventType::MouseMove)
	{
		if (rotating)
			Rotate(e.delta.x * rotationSpeed, e.delta.y * rotationSpeed);
		if (panning)
		{
			auto& r = rect ? *rect : UIObject_GetFinalRect(e.current);
			Pan(e.delta.x / r.GetWidth(), e.delta.y / r.GetHeight());
		}
		return rotating || panning;
	}
	if (e.type == EventType::MouseScroll)
	{
		Zoom(e.delta.y == 0 ? 0 : e.delta.y < 0 ? 1 : -1);
		return true;
	}
	return false;
}

void OrbitCamera::SerializeState(IObjectIterator& oi, const FieldInfo& FI)
{
	oi.BeginObject(FI, "OrbitCamera(state)");

	OnField(oi, "pivot", pivot);
	OnField(oi, "yaw", yaw);
	OnField(oi, "pitch", pitch);
	OnField(oi, "distance", distance);

	oi.EndObject();
}

void OrbitCamera::Rotate(float dx, float dy)
{
	yaw += dx * (rightHanded ? -1 : 1);
	pitch += dy;
	pitch = clamp(pitch, minPitch, maxPitch);

	_UpdateViewMatrix();
}

void OrbitCamera::Pan(float dx, float dy)
{
	float scale = distance * 2;
	dx *= scale / GetProjectionMatrix().m[0][0];
	dy *= scale / GetProjectionMatrix().m[1][1];
	auto vm = GetViewMatrix();
	Vec3f right = { vm.v00, vm.v01, vm.v02 };
	Vec3f up = { vm.v10, vm.v11, vm.v12 };
	pivot -= right * dx - up * dy;

	_UpdateViewMatrix();
}

void OrbitCamera::Zoom(float delta)
{
	distance *= powf(distanceScale, delta);

	_UpdateViewMatrix();
}

void OrbitCamera::ResetState()
{
	pivot = {};
	yaw = 45;
	pitch = 45;
	distance = 1;

	_UpdateViewMatrix();
}

void OrbitCamera::_UpdateViewMatrix()
{
	float cp = cosf(pitch * DEG2RAD);
	float sp = sinf(pitch * DEG2RAD);
	float cy = cosf(yaw * DEG2RAD);
	float sy = sinf(yaw * DEG2RAD);
	Vec3f dir = { cy * cp, sy * cp, sp };
	Vec3f up = { cy * -sp, sy * -sp, cp };
	Vec3f pos = pivot + dir * distance;
	Mat4f vm = rightHanded
		? Mat4f::LookAtRH(pos, pivot, up)
		: Mat4f::LookAtLH(pos, pivot, up);

	SetViewMatrix(vm);
}

} // ui
