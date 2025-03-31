
#pragma once

#include "3DMath.h"

#include "../Model/InputDefs.h"


namespace ui {

struct Event;

struct CameraBase
{
	Mat4f _mtxView = Mat4f::Identity();
	Mat4f _mtxInvView = Mat4f::Identity();
	Mat4f _mtxProj = Mat4f::Identity();
	Mat4f _mtxInvProj = Mat4f::Identity();
	Mat4f _mtxViewProj = Mat4f::Identity();
	Mat4f _mtxInvViewProj = Mat4f::Identity();
	AABB2f _windowRect = { -1, -1, 0, 0 };

	void _UpdateViewProjMatrix();

	UI_FORCEINLINE const Mat4f& GetViewMatrix() const { return _mtxView; }
	UI_FORCEINLINE const Mat4f& GetInverseViewMatrix() const { return _mtxInvView; }
	void SetViewMatrix(const Mat4f& m);

	UI_FORCEINLINE const Mat4f& GetProjectionMatrix() const { return _mtxProj; }
	UI_FORCEINLINE const Mat4f& GetInverseProjectionMatrix() const { return _mtxInvProj; }
	void SetProjectionMatrix(const Mat4f& m);

	UI_FORCEINLINE const Mat4f& GetViewProjectionMatrix() const { return _mtxViewProj; }
	UI_FORCEINLINE const Mat4f& GetInverseViewProjectionMatrix() const { return _mtxInvViewProj; }

	UI_FORCEINLINE const AABB2f& GetWindowRect() const { return _windowRect; }
	void SetWindowRect(const AABB2f& rect);

	// position found in the inverse view matrix
	virtual Vec3f GetCameraPosition() const { return _mtxInvView.GetTranslation(); }
	virtual Vec3f GetCameraRightDir() const { return Vec3f(_mtxInvView.v00, _mtxInvView.v10, _mtxInvView.v20).Normalized(); }
	virtual Vec3f GetCameraUpDir() const { return Vec3f(_mtxInvView.v01, _mtxInvView.v11, _mtxInvView.v21).Normalized(); }
	virtual Vec3f GetCameraForwardDir() const { return Vec3f(_mtxInvView.v02, _mtxInvView.v12, _mtxInvView.v22).Normalized(); }
	// the location of the "eye" (source of rays)
	virtual Vec3f GetCameraEyePos() const
	{
		if (_mtxProj.v32)
			return GetCameraPosition(); // perspective projection, can get exact eye pos
		return GetCameraEyePosEstimate();
	}
	// https://terathon.com/gdc07_lengyel.pdf (slide 6)
	Vec3f GetCameraEyePosEstimate(float z = -10000) const { return _mtxInvViewProj.TransformPoint({ 0, 0, z }); }

	Point2f WindowToNormalizedPoint(Point2f p) const;
	Point2f NormalizedToWindowPoint(Point2f p) const;
	Point2f WorldToNormalizedPoint(const Vec3f& p) const;
	Point2f WorldToWindowPoint(const Vec3f& p) const;

	float WorldToWindowSize(float size, const Vec3f& refp) const;
	float WindowToWorldSize(float size, const Vec3f& refp) const;

	Ray3f GetRayNP(Point2f p) const;
	Ray3f GetViewRayNP(Point2f p) const;
	Ray3f GetLocalRayNP(Point2f p, const Mat4f& world2local) const;
	Ray3f GetRayWP(Point2f p) const;
	Ray3f GetViewRayWP(Point2f p) const;
	Ray3f GetLocalRayWP(Point2f p, const Mat4f& world2local) const;
	Ray3f GetRayEP(const Event& e) const;
	Ray3f GetViewRayEP(const Event& e) const;
	Ray3f GetLocalRayEP(const Event& e, const Mat4f& world2local) const;
};

struct OrbitCamera : CameraBase
{
	OrbitCamera(bool rh = false);
	// returns true if the camera was modified
	bool OnEvent(Event& e, const AABB2f* rect = nullptr);
	void SerializeState(IObjectIterator& oi, const FieldInfo& FI);

	void Rotate(float dx, float dy);
	void Pan(float dx, float dy);
	void Zoom(float delta);

	void ResetState();

	void _UpdateViewMatrix();

	// state/settings
	Vec3f pivot = {};
	float yaw = 45;
	float pitch = 45;
	float distance = 1;

	// settings
	bool rightHanded = false;
	float minPitch = -85;
	float maxPitch = 85;
	float rotationSpeed = 0.5f; // degrees per pixel
	float distanceScale = 1.2f; // scale per scroll
	MouseButton rotateButton = MouseButton::Left;
	MouseButton panButton = MouseButton::Middle;

	// state
	bool rotating = false;
	bool panning = false;
};

} // ui
