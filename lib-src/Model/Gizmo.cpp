
#include "Gizmo.h"

#include "../Render/RHI.h"
#include "../Render/Primitives.h"

#include "../Core/3DCamera.h"

#include <algorithm>


namespace ui {

struct SDFRet
{
	float dist;
	GizmoAction part;
};

static float ConeSDF(const Vec3f& p, const Vec3f& slope)
{
	float q = sqrtf(p.x * p.x + p.y * p.y);
	return slope.x * q + slope.y * p.z;
}

static float TorusSDF(const Vec3f& p, float radius, const Vec3f& axis)
{
	Vec3f pflat = p - p * Vec3Dot(p, axis);
	float dist2d = pflat.Length() - radius;
	return Vec3f(dist2d, Vec3Dot(p, axis)).Length();
}

static float BoxSDF(const Vec3f& p, const Vec3f& bbmin, const Vec3f& bbmax)
{
	Vec3f c = p;
	if (c.x < bbmin.x) c.x = bbmin.x; else if (c.x > bbmax.x) c.x = bbmax.x;
	if (c.y < bbmin.y) c.y = bbmin.y; else if (c.y > bbmax.y) c.y = bbmax.y;
	if (c.z < bbmin.z) c.z = bbmin.z; else if (c.z > bbmax.z) c.z = bbmax.z;
	return (c - p).Length();
}

static float CubeSDF(const Vec3f& p, const Vec3f& center, float halfExt)
{
	Vec3f h = { halfExt, halfExt, halfExt };
	return BoxSDF(p, center - h, center + h);
}

static SDFRet MovingGizmoSDF(const Vec3f& pos)
{
	auto coneSlope = Vec3f{ 2, 1 }.Normalized();

	float coneDistX = max(ConeSDF({ pos.y, pos.z, pos.x - 1.1f }, coneSlope), 0.9f - pos.x);
	float coneDistY = max(ConeSDF({ pos.z, pos.x, pos.y - 1.1f }, coneSlope), 0.9f - pos.y);
	float coneDistZ = max(ConeSDF({ pos.x, pos.y, pos.z - 1.1f }, coneSlope), 0.9f - pos.z);

	float axisDistX = BoxSDF(pos, { 0.15f, 0, 0 }, { 0.9f, 0, 0 }) - 0.05f;
	float axisDistY = BoxSDF(pos, { 0, 0.15f, 0 }, { 0, 0.9f, 0 }) - 0.05f;
	float axisDistZ = BoxSDF(pos, { 0, 0, 0.15f }, { 0, 0, 0.9f }) - 0.05f;

	float planeDistX = BoxSDF(pos, { 0, 0.6f, 0.6f }, { 0, 0.9f, 0.9f });
	float planeDistY = BoxSDF(pos, { 0.6f, 0, 0.6f }, { 0.9f, 0, 0.9f });
	float planeDistZ = BoxSDF(pos, { 0.6f, 0.6f, 0 }, { 0.9f, 0.9f, 0 });

	float sphereDistC = pos.Length() - 0.15f;

	SDFRet ret = { FLT_MAX, GizmoAction::None };
	if (ret.dist > coneDistX) ret = { coneDistX, GizmoAction::MoveXAxis };
	if (ret.dist > coneDistY) ret = { coneDistY, GizmoAction::MoveYAxis };
	if (ret.dist > coneDistZ) ret = { coneDistZ, GizmoAction::MoveZAxis };
	if (ret.dist > axisDistX) ret = { axisDistX, GizmoAction::MoveXAxis };
	if (ret.dist > axisDistY) ret = { axisDistY, GizmoAction::MoveYAxis };
	if (ret.dist > axisDistZ) ret = { axisDistZ, GizmoAction::MoveZAxis };
	if (ret.dist > planeDistX) ret = { planeDistX, GizmoAction::MoveXPlane };
	if (ret.dist > planeDistY) ret = { planeDistY, GizmoAction::MoveYPlane };
	if (ret.dist > planeDistZ) ret = { planeDistZ, GizmoAction::MoveZPlane };
	if (ret.dist > sphereDistC) ret = { sphereDistC, GizmoAction::MoveScreenPlane };
	return ret;
}

static SDFRet RotatingGizmoSDF(const Vec3f& pos)
{
	float donutDistX = TorusSDF(pos, 0.82f, Vec3f(1, 0, 0)) - 0.1f;
	float donutDistY = TorusSDF(pos, 0.82f, Vec3f(0, 1, 0)) - 0.1f;
	float donutDistZ = TorusSDF(pos, 0.82f, Vec3f(0, 0, 1)) - 0.1f;

	float sphereDistC = pos.Length() - 0.8f;

	SDFRet ret = { FLT_MAX, GizmoAction::None };
	if (ret.dist > donutDistX) ret = { donutDistX, GizmoAction::RotateXAxis };
	if (ret.dist > donutDistY) ret = { donutDistY, GizmoAction::RotateYAxis };
	if (ret.dist > donutDistZ) ret = { donutDistZ, GizmoAction::RotateZAxis };
	if (ret.dist > sphereDistC) ret = { sphereDistC, GizmoAction::RotateTrackpad };
	return ret;
}

static GizmoAction RotatingGizmoScreenSpace(Point2f cursorScr, Point2f centerScr, float sizeScr)
{
	float dist = (cursorScr - centerScr).Length();
	if (dist <= sizeScr + 2 && dist >= sizeScr - 2)
		return GizmoAction::RotateScreenAxis;

	return GizmoAction::None;
}

static SDFRet ScalingGizmoSDF(const Vec3f& pos)
{
	float cubeDistX = CubeSDF(pos, { 0.95f, 0, 0 }, 0.05f);
	float cubeDistY = CubeSDF(pos, { 0, 0.95f, 0 }, 0.05f);
	float cubeDistZ = CubeSDF(pos, { 0, 0, 0.95f }, 0.05f);

	float axisDistX = BoxSDF(pos, { 0, 0, 0 }, { 0.9f, 0, 0 }) - 0.05f;
	float axisDistY = BoxSDF(pos, { 0, 0, 0 }, { 0, 0.9f, 0 }) - 0.05f;
	float axisDistZ = BoxSDF(pos, { 0, 0, 0 }, { 0, 0, 0.9f }) - 0.05f;

	float planeDistX = BoxSDF(pos, { 0, 0.6f, 0.6f }, { 0, 0.9f, 0.9f });
	float planeDistY = BoxSDF(pos, { 0.6f, 0, 0.6f }, { 0.9f, 0, 0.9f });
	float planeDistZ = BoxSDF(pos, { 0.6f, 0.6f, 0 }, { 0.9f, 0.9f, 0 });

	float cubeDistC = CubeSDF(pos, { 0, 0, 0 }, 0.1f);

	SDFRet ret = { FLT_MAX, GizmoAction::None };
	if (ret.dist > cubeDistX) ret = { cubeDistX, GizmoAction::ScaleXAxis };
	if (ret.dist > cubeDistY) ret = { cubeDistY, GizmoAction::ScaleYAxis };
	if (ret.dist > cubeDistZ) ret = { cubeDistZ, GizmoAction::ScaleZAxis };
	if (ret.dist > axisDistX) ret = { axisDistX, GizmoAction::ScaleXAxis };
	if (ret.dist > axisDistY) ret = { axisDistY, GizmoAction::ScaleYAxis };
	if (ret.dist > axisDistZ) ret = { axisDistZ, GizmoAction::ScaleZAxis };
	if (ret.dist > planeDistX) ret = { planeDistX, GizmoAction::ScaleXPlane };
	if (ret.dist > planeDistY) ret = { planeDistY, GizmoAction::ScaleYPlane };
	if (ret.dist > planeDistZ) ret = { planeDistZ, GizmoAction::ScaleZPlane };
	//if (ret.dist > cubeDistC) ret = { cubeDistC, GizmoAction::ScaleUniform };
	return ret;
}

static GizmoAction ScalingGizmoScreenSpace(Point2f cursorScr, Point2f centerScr, float sizeScr)
{
	float dist = (cursorScr - centerScr).Length();
	if (dist <= sizeScr && dist >= sizeScr * 0.2f)
		return GizmoAction::ScaleUniform;

	return GizmoAction::None;
}

static GizmoAction GetIntersectingPart(GizmoType type, const Ray3f& ray, Point2f cursorScr, Point2f centerScr, float sizeScr)
{
	Vec3f pos = ray.origin;

	for (int i = 0; i < 64; i++)
	{
		SDFRet nearest;
		switch (type)
		{
		case GizmoType::Move: nearest = MovingGizmoSDF(pos); break;
		case GizmoType::Rotate: nearest = RotatingGizmoSDF(pos); break;
		case GizmoType::Scale: nearest = ScalingGizmoSDF(pos); break;
		default: nearest = { FLT_MAX, GizmoAction::None };
		}
		if (nearest.dist < 0.001f)
			return nearest.part;

		pos += ray.direction * nearest.dist;
	}

	switch (type)
	{
	case GizmoType::Rotate:
		return RotatingGizmoScreenSpace(cursorScr, centerScr, sizeScr);
	case GizmoType::Scale:
		return ScalingGizmoScreenSpace(cursorScr, centerScr, sizeScr);
	}

	return GizmoAction::None;
}

static Mat4f GetTransformBasis(const Transform3Df& src, bool isWorldSpace)
{
	if (isWorldSpace)
	{
		return Mat4f::Translate(src.position);
	}
	return src.ToMatrix();
}

static float ScaleFactor(Point2f centerWinPos, Point2f cursorWinPos, Point2f newWinPos)
{
	float origDist = (cursorWinPos - centerWinPos).Length();
	float newDist = (newWinPos - centerWinPos).Length();
	return newDist / origDist;
}

static bool IsMoveAction(GizmoAction a)
{
	return (int(a) & GAF_Mode_MASK) == GAF_Mode_Move;
}

static bool IsRotateAction(GizmoAction a)
{
	return (int(a) & GAF_Mode_MASK) == GAF_Mode_Rotate;
}

static bool IsScaleAction(GizmoAction a)
{
	return (int(a) & GAF_Mode_MASK) == GAF_Mode_Scale;
}

static void ChangeAxis(GizmoAction& ioAction, bool& ioWorldSpace, int neededAxis, bool neededPlaneMode)
{
	auto axis = int(ioAction) & GAF_Axis_MASK;
	auto shape = int(ioAction) & GAF_Shape_MASK;
	auto mode = int(ioAction) & GAF_Mode_MASK;

	if (shape == GAF_Shape_Trackpad)
		return; // no transition paths to/from this

	if (mode == GAF_Mode_Rotate)
		neededPlaneMode = false; // not supported

	int neededShape = neededPlaneMode ? GAF_Shape_Plane : GAF_Shape_Axis;

	if (axis == neededAxis && shape == neededShape)
	{
		if (ioWorldSpace)
			ioWorldSpace = false; // world -> local space
		else
		{
			// back to uniform
			axis = GAF_Axis_ScreenOrUniform;
			shape = GAF_Shape_ScreenOrUniform;
		}
	}
	else
	{
		axis = neededAxis;
		shape = neededShape;
		ioWorldSpace = true;
	}

	ioAction = GizmoAction(axis | shape | mode);
}

static Vec3f GetAxis(GizmoAction action)
{
	Vec3f axis = {};
	switch (int(action) & GAF_Axis_MASK)
	{
	case GAF_Axis_X: axis = { 1, 0, 0 }; break;
	case GAF_Axis_Y: axis = { 0, 1, 0 }; break;
	case GAF_Axis_Z: axis = { 0, 0, 1 }; break;
	}
	return axis;
}

static void Snap(float& delta, float snapDist)
{
	delta = roundf(delta / snapDist) * snapDist;
}

void Gizmo::Start(GizmoAction action, Point2f cursorPoint, const CameraBase& cam, const IGizmoEditable& editable)
{
	if (_selectedPart == GizmoAction::None)
	{
		_origXF = editable.GetGizmoLocation();

		_origCursorWinPos = cursorPoint;
		auto pointWP = cam.WorldToWindowPoint(_origXF.position);
		_origCenterWinPos = pointWP;
		_totalMovedWinVec = {};
		_totalAngleDiff = 0;

		_origData.Clear();
		DataWriter dw(_origData);
		editable.Backup(dw);
	}

	_selectedPart = action;
	_curXFWorldSpace = settings.isWorldSpace;
}

static bool IsButtonUsed(Gizmo& G, MouseButton btn)
{
	if (G._selectedPart != GizmoAction::None)
		if (btn == MouseButton::Right)
			return true;
	return btn == MouseButton::Left;
}

bool Gizmo::OnEvent(Event& e, const CameraBase& cam, const IGizmoEditable& editable)
{
	if (e.IsPropagationStopped())
		return false;

	// consume button releases whose corresponding presses happened during a gizmo action
	if (e.type == EventType::ButtonDown && (_selectedPart != GizmoAction::None || _hoveredPart != GizmoAction::None) && IsButtonUsed(*this, e.GetButton()))
	{
		_buttonPresses |= 1 << int(e.GetButton());
		e.StopPropagation();
	}
	if (e.type == EventType::ButtonUp && (_buttonPresses & (1 << int(e.GetButton()))))
	{
		_buttonPresses &= ~(1 << int(e.GetButton()));
		e.StopPropagation();
	}

	auto& editableNC = const_cast<IGizmoEditable&>(editable);
	if (e.type == EventType::MouseMove)
	{
		_lastCursorPos = e.position;
		if (_selectedPart == GizmoAction::None)
		{
			if (settings.visible)
			{
				auto wp = _combinedXF.GetTranslation();
				_hoveredPart = GetIntersectingPart(
					settings.type,
					cam.GetLocalRayEP(e, _combinedXF.Inverted()),
					_lastCursorPos,
					cam.WorldToWindowPoint(wp),
					fabsf(cam.WorldToWindowSize(_finalSize, wp)));
			}
			else
			{
				_hoveredPart = GizmoAction::None;
			}
		}
		else
		{
			bool slowMotion = (e.GetModifierKeys() & MK_Shift) != 0;
			bool snapping = (e.GetModifierKeys() & MK_Ctrl) != 0;

			Point2f delta = e.delta;

			Vec3f axis = GetAxis(_selectedPart);
			Mat4f xfBasis = GetTransformBasis(_origXF, _curXFWorldSpace);

			if (IsMoveAction(_selectedPart))
			{
				_totalMovedWinVec += delta * (slowMotion ? settings.edit.moveSlowdownFactor : 1);

				float snapDist = slowMotion ? settings.edit.moveSlowSnapDist : settings.edit.moveSnapDist;

				Ray3f vray = cam.GetViewRayWP(_origCenterWinPos + _totalMovedWinVec);

				Vec3f worldOrigPos = xfBasis.GetTranslation();
				Vec3f worldAxis = xfBasis.TransformDirection(axis).Normalized();
				if (_selectedPart == GizmoAction::MoveScreenPlane)
				{
					worldAxis = cam.GetInverseViewMatrix().TransformDirection({ 0, 0, 1 }).Normalized();
				}

				Vec3f viewAxis = cam.GetViewMatrix().TransformDirection(worldAxis).Normalized();
				Vec3f viewOrigPos = cam.GetViewMatrix().TransformPoint(worldOrigPos);

				if (_totalMovedWinVec.x == 0 && _totalMovedWinVec.y == 0)
				{
					DataReader dr(_origData);
					editableNC.Transform(dr, nullptr, 0);
					return true;
				}
				if ((int(_selectedPart) & GAF_Shape_MASK) == GAF_Shape_Axis)
				{
					// generate a plane
					Vec3f vcam2center = viewOrigPos.Normalized();
					Vec3f rt = Vec3Cross(vcam2center, viewAxis);
					Vec3f pn = Vec3Cross(rt, viewAxis);
					auto rpir = RayPlaneIntersect(vray.origin, vray.direction, { pn.x, pn.y, pn.z, Vec3Dot(pn, viewOrigPos) });
					if (rpir.angcos)
					{
						Vec3f isp = vray.GetPoint(rpir.dist);
						float diff = Vec3Dot(viewAxis, isp) - Vec3Dot(viewAxis, viewOrigPos);
						//printf("tmwv=%.2f;%.2f   isp=%.3f;%.3f;%.3f   dist=%.3f\n", _totalMovedWinVec.x, _totalMovedWinVec.y, isp.x, isp.y, isp.z, diff);
						if (snapping)
							Snap(diff, snapDist);

						auto xf = Mat4f::Translate(worldAxis * diff);

						DataReader dr(_origData);
						editableNC.Transform(dr, &xf, GPC_Move);
						return true;
					}
				}
				else
				{
					auto rpir = RayPlaneIntersect(vray.origin, vray.direction, { viewAxis.x, viewAxis.y, viewAxis.z, Vec3Dot(viewAxis, viewOrigPos) });
					if (rpir.angcos)
					{
						Vec3f worldTg1, worldTg2, viewTg1, viewTg2;
						if (_selectedPart == GizmoAction::MoveScreenPlane)
						{
							viewTg1 = { 1, 0, 0 };
							viewTg2 = { 0, 1, 0 };
							worldTg1 = cam.GetInverseViewMatrix().TransformDirection(viewTg1).Normalized();
							worldTg2 = cam.GetInverseViewMatrix().TransformDirection(viewTg2).Normalized();
						}
						else
						{
							Vec3f tg1(axis.z, axis.x, axis.y);
							Vec3f tg2(axis.y, axis.z, axis.x);
							worldTg1 = xfBasis.TransformDirection(tg1).Normalized();
							worldTg2 = xfBasis.TransformDirection(tg2).Normalized();
							viewTg1 = cam.GetViewMatrix().TransformDirection(worldTg1).Normalized();
							viewTg2 = cam.GetViewMatrix().TransformDirection(worldTg2).Normalized();
						}

						Vec3f diff = vray.GetPoint(rpir.dist) - viewOrigPos;
						float diffX = Vec3Dot(diff, viewTg1);
						float diffY = Vec3Dot(diff, viewTg2);
						if (snapping)
						{
							Snap(diffX, snapDist);
							Snap(diffY, snapDist);
						}
						diff = worldTg1 * diffX + worldTg2 * diffY;

						auto xf = Mat4f::Translate(diff);

						DataReader dr(_origData);
						editableNC.Transform(dr, &xf, GPC_Move);
						return true;
					}
				}
			}
			else if (IsRotateAction(_selectedPart))
			{
				float snapAngle = slowMotion ? settings.edit.rotateSlowSnapAngleDeg : settings.edit.rotateSnapAngleDeg;
				Mat4f xfBasisW = GetTransformBasis(_origXF, true);

				Mat4f fullXF;
				if ((int(_selectedPart) & GAF_Shape_MASK) == GAF_Shape_Trackpad)
				{
					_totalMovedWinVec += delta * (slowMotion ? settings.edit.rotateSlowdownFactor : 1);

					auto angles = _totalMovedWinVec;
					if (snapping)
					{
						Snap(angles.x, snapAngle);
						Snap(angles.y, snapAngle);
					}

					auto rx = Mat4f::RotateAxisAngle(cam.GetInverseViewMatrix().TransformDirection({ 1, 0, 0 }).Normalized(), angles.y);
					auto ry = Mat4f::RotateAxisAngle(cam.GetInverseViewMatrix().TransformDirection({ 0, 1, 0 }).Normalized(), angles.x);
					auto xf = rx * ry;

					fullXF = xfBasisW.Inverted() * xf * xfBasisW;
				}
				else
				{
					auto prevWV = _origCursorWinPos + _totalMovedWinVec - _origCenterWinPos;
					_totalMovedWinVec += delta;
					auto nextWV = _origCursorWinPos + _totalMovedWinVec - _origCenterWinPos;

					float prevAngle = atan2f(prevWV.y, prevWV.x);
					float nextAngle = atan2f(nextWV.y, nextWV.x);
					float angleDiff = nextAngle - prevAngle;
					angleDiff = fmodf(angleDiff, 2 * 3.14159f);
					if (angleDiff < -3.14159f)
						angleDiff += 2 * 3.14159f;
					else if (angleDiff > 3.14159f)
						angleDiff -= 2 * 3.14159f;
					angleDiff *= 180 / 3.14159f;

					if (slowMotion)
						angleDiff *= settings.edit.rotateSlowdownFactor;

					_totalAngleDiff += angleDiff;

					Vec3f worldAxis = xfBasis.TransformDirection(axis).Normalized();
					if (_selectedPart == GizmoAction::RotateScreenAxis)
					{
						worldAxis = cam.GetInverseViewMatrix().TransformDirection({ 0, 0, 1 }).Normalized();
					}
					if (cam.GetViewMatrix().TransformDirection(worldAxis).z < 0)
						worldAxis = -worldAxis;

					float angle = _totalAngleDiff;
					if (snapping)
						Snap(angle, snapAngle);

					auto xf = Mat4f::RotateAxisAngle(worldAxis, angle);
					fullXF = xfBasisW.Inverted() * xf * xfBasisW;
				}

				DataReader dr(_origData);
				editableNC.Transform(dr, &fullXF, GPC_Move | GPC_Rotate);
				return true;
			}
			else if (IsScaleAction(_selectedPart))
			{
				_totalMovedWinVec += delta * (slowMotion ? settings.edit.scaleSlowdownFactor : 1);

				float scaleFactor = ScaleFactor(_origCenterWinPos, _origCursorWinPos, _origCursorWinPos + _totalMovedWinVec);
				if (snapping)
					Snap(scaleFactor, slowMotion ? settings.edit.scaleSlowSnapDist : settings.edit.scaleSnapDist);

				Mat4f xf;
				switch (int(_selectedPart) & GAF_Shape_MASK)
				{
				case GAF_Shape_Axis:
					xf = Mat4f::Scale(Vec3Lerp(Vec3f(1), Vec3f(1) * scaleFactor, axis));
					break;
				case GAF_Shape_Plane:
					xf = Mat4f::Scale(Vec3Lerp(Vec3f(1) * scaleFactor, Vec3f(1), axis));
					break;
				case GAF_Shape_ScreenOrUniform:
					xf = Mat4f::Scale(scaleFactor);
					break;
				default:
					xf = Mat4f::Identity();
					break;
				}

				auto fullXF = xfBasis.Inverted() * xf * xfBasis;

				DataReader dr(_origData);
				editableNC.Transform(dr, &fullXF, GPC_Move | GPC_Scale);
				return true;
			}
		}
	}
	else if (e.type == EventType::ButtonDown && e.GetButton() == MouseButton::Left)
	{
		if (_selectedPart != GizmoAction::None)
		{
			_selectedPart = GizmoAction::None;
			e.StopPropagation();
		}
		else if (_hoveredPart != GizmoAction::None)
		{
			Vec3f cam2center = (_combinedXF.GetTranslation() - cam.GetCameraEyePos()).Normalized();
			float dotX = fabsf(Vec3Dot((_combinedXF.TransformPoint({ 1, 0, 0 }) - _combinedXF.GetTranslation()).Normalized(), cam2center));
			float dotY = fabsf(Vec3Dot((_combinedXF.TransformPoint({ 0, 1, 0 }) - _combinedXF.GetTranslation()).Normalized(), cam2center));
			float dotZ = fabsf(Vec3Dot((_combinedXF.TransformPoint({ 0, 0, 1 }) - _combinedXF.GetTranslation()).Normalized(), cam2center));

			float visX = clamp(invlerp(0.99f, 0.97f, dotX), 0.0f, 1.0f);
			float visY = clamp(invlerp(0.99f, 0.97f, dotY), 0.0f, 1.0f);
			float visZ = clamp(invlerp(0.99f, 0.97f, dotZ), 0.0f, 1.0f);
			float visPX = clamp(invlerp(0.05f, 0.21f, dotX), 0.0f, 1.0f);
			float visPY = clamp(invlerp(0.05f, 0.21f, dotY), 0.0f, 1.0f);
			float visPZ = clamp(invlerp(0.05f, 0.21f, dotZ), 0.0f, 1.0f);

			if ((int(_hoveredPart) & GAF_Mode_MASK) != GAF_Mode_Rotate)
			{
				switch (int(_hoveredPart) & (GAF_Axis_MASK | GAF_Shape_MASK))
				{
				case GAF_Axis_X | GAF_Shape_Axis: if (visX <= 0) return false; break;
				case GAF_Axis_Y | GAF_Shape_Axis: if (visY <= 0) return false; break;
				case GAF_Axis_Z | GAF_Shape_Axis: if (visZ <= 0) return false; break;
				case GAF_Axis_X | GAF_Shape_Plane: if (visPX <= 0) return false; break;
				case GAF_Axis_Y | GAF_Shape_Plane: if (visPY <= 0) return false; break;
				case GAF_Axis_Z | GAF_Shape_Plane: if (visPZ <= 0) return false; break;
				}
			}

			Start(_hoveredPart, e.position, cam, editable);

			e.StopPropagation();
		}
	}
	else if (e.type == EventType::ButtonDown && e.GetButton() == MouseButton::Right && _selectedPart != GizmoAction::None && settings.visible)
	{
		_selectedPart = GizmoAction::None;
		DataReader dr(_origData);
		editableNC.Transform(dr, nullptr, 0);
		return true;
	}
	else if (e.type == EventType::ButtonUp && e.GetButton() == MouseButton::Left)
	{
		if (_selectedPart != GizmoAction::None)
		{
			_selectedPart = GizmoAction::None;
			e.StopPropagation();
		}
	}
	else if (e.type == EventType::KeyDown && (ModKeyCheck(e.GetModifierKeys(), 0) || ModKeyCheck(e.GetModifierKeys(), MK_Shift)))
	{
		bool anySelPart = _selectedPart != GizmoAction::None;
		u8 modeKeyMask = GizmoKeyDetect::Start | (anySelPart ? GizmoKeyDetect::ModeSwitch : 0);
		u8 axisKeyMask = anySelPart ? GizmoKeyDetect::AxisSwitch : 0;

		switch (e.shortCode)
		{
		case KSC_Escape:
			if (anySelPart && (settings.detectsKeys & GizmoKeyDetect::Exit))
			{
				_selectedPart = GizmoAction::None;
				DataReader dr(_origData);
				editableNC.Transform(dr, nullptr, 0);

				e.StopPropagation();
				return true;
			}
			break;

		case KSC_G:
			if (settings.detectsKeys & modeKeyMask)
			{
				Start(GizmoAction::MoveScreenPlane, _lastCursorPos, cam, editable);
				e.StopPropagation();
				return true;
			}
			break;
		case KSC_R:
			if (settings.detectsKeys & modeKeyMask)
			{
				Start(GizmoAction::RotateScreenAxis, _lastCursorPos, cam, editable);
				e.StopPropagation();
				return true;
			}
			break;
		case KSC_S:
			if (settings.detectsKeys & modeKeyMask)
			{
				Start(GizmoAction::ScaleUniform, _lastCursorPos, cam, editable);
				e.StopPropagation();
				return true;
			}
			break;

		case KSC_X:
			if (settings.detectsKeys & axisKeyMask)
			{
				ChangeAxis(_selectedPart, _curXFWorldSpace, GAF_Axis_X, (e.GetModifierKeys() & MK_Shift) != 0);
				e.StopPropagation();
				return true;
			}
			break;
		case KSC_Y:
			if (settings.detectsKeys & axisKeyMask)
			{
				ChangeAxis(_selectedPart, _curXFWorldSpace, GAF_Axis_Y, (e.GetModifierKeys() & MK_Shift) != 0);
				e.StopPropagation();
				return true;
			}
			break;
		case KSC_Z:
			if (settings.detectsKeys & axisKeyMask)
			{
				ChangeAxis(_selectedPart, _curXFWorldSpace, GAF_Axis_Z, (e.GetModifierKeys() & MK_Shift) != 0);
				e.StopPropagation();
				return true;
			}
			break;
		}
	}
	return false;
}

static Color4b GetColor(GizmoAction hoverAction, GizmoAction curr, float alpha)
{
	bool hovered = hoverAction == curr;

	if ((int(curr) & GAF_Shape_MASK) == GAF_Shape_Plane)
		alpha *= 0.5f;

	switch (int(curr) & GAF_Axis_MASK)
	{
	case GAF_Axis_X: return Color4f::Hex(hovered ? "f13c30" : "b04139", alpha);
	case GAF_Axis_Y: return Color4f::Hex(hovered ? "40c113" : "478e2e", alpha);
	case GAF_Axis_Z: return Color4f::Hex(hovered ? "0d71e3" : "2e5b8e", alpha);
	default: return Color4f::Hex(hovered ? "e6e6e6" : "9c9c9c", alpha);
	}
}

static Color4b GetColor(GizmoAction action)
{
	return GetColor(action, action, 1.0f);
}

static void Gizmo_Render_Move(GizmoAction hoverPart, const Mat4f& transform, const CameraBase& cam)
{
	using namespace gfx;

	constexpr prim::PlaneSettings PS = {};
	constexpr uint16_t planeVC = PS.CalcVertexCount();
	constexpr uint16_t planeIC = PS.CalcIndexCount();
	Vertex_PF3CB4 planeVerts[planeVC];
	uint16_t planeIndices[planeIC];
	prim::GeneratePlane(PS, planeVerts, planeIndices);

	constexpr uint16_t planeFrameIC = 5;
	uint16_t planeFrameIndices[planeFrameIC] = { 0, 1, 3, 2, 0 };

	constexpr prim::ConeSettings CS = {};
	constexpr uint16_t coneVC = CS.CalcVertexCount();
	constexpr uint16_t coneIC = CS.CalcIndexCount();
	Vertex_PF3CB4 coneVerts[coneVC];
	uint16_t coneIndices[coneIC];
	prim::GenerateCone(CS, coneVerts, coneIndices);

	constexpr prim::BoxSphereSettings SS = {};
	constexpr uint16_t sphereVC = SS.CalcVertexCount();
	constexpr uint16_t sphereIC = SS.CalcIndexCount();
	Vertex_PF3CB4 sphereVerts[sphereVC];
	uint16_t sphereIndices[sphereIC];
	prim::GenerateBoxSphere(SS, sphereVerts, sphereIndices);

	Vertex_PF3CB4 tmpVerts[2] = {};

	Vec3f cam2center = (transform.GetTranslation() - cam.GetCameraEyePos()).Normalized();
	float dotX = fabsf(Vec3Dot((transform.TransformPoint({ 1, 0, 0 }) - transform.GetTranslation()).Normalized(), cam2center));
	float dotY = fabsf(Vec3Dot((transform.TransformPoint({ 0, 1, 0 }) - transform.GetTranslation()).Normalized(), cam2center));
	float dotZ = fabsf(Vec3Dot((transform.TransformPoint({ 0, 0, 1 }) - transform.GetTranslation()).Normalized(), cam2center));

	float visX = clamp(invlerp(0.99f, 0.97f, dotX), 0.0f, 1.0f);
	float visY = clamp(invlerp(0.99f, 0.97f, dotY), 0.0f, 1.0f);
	float visZ = clamp(invlerp(0.99f, 0.97f, dotZ), 0.0f, 1.0f);
	float visPX = clamp(invlerp(0.05f, 0.21f, dotX), 0.0f, 1.0f);
	float visPY = clamp(invlerp(0.05f, 0.21f, dotY), 0.0f, 1.0f);
	float visPZ = clamp(invlerp(0.05f, 0.21f, dotZ), 0.0f, 1.0f);

	constexpr int NUM_PARTS = 10;
	Vec3f centers[NUM_PARTS] =
	{
		{ 0, 0.15f, 0.15f }, // x plane
		{ 0.15f, 0, 0.15f }, // y plane
		{ 0.15f, 0.15f, 0 }, // z plane
		{ 0.55f, 0, 0 }, // x line
		{ 0, 0.55f, 0 }, // y line
		{ 0, 0, 0.55f }, // z line
		{ 0.95f, 0, 0 }, // x cone
		{ 0, 0.95f, 0 }, // y cone
		{ 0, 0, 0.95f }, // z cone
		{ 0, 0, 0 }, // center sphere
	};
	float distances[NUM_PARTS];
	Mat4f matWorldView = transform * cam.GetViewMatrix();
	for (int i = 0; i < NUM_PARTS; i++)
	{
		distances[i] = matWorldView.TransformPoint(centers[i]).z;
	}

	int sortedParts[NUM_PARTS] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 };
	std::sort(std::begin(sortedParts), std::end(sortedParts), [&distances](int a, int b) { return distances[a] > distances[b]; });

	constexpr unsigned DF_PRIMARY = DF_AlphaBlended | DF_ZTestOff | DF_ZWriteOff;

	for (int i = 0; i < NUM_PARTS; i++)
	{
		switch (sortedParts[i])
		{
		case 0:
			SetRenderState(DF_PRIMARY);
			prim::SetVertexColor(planeVerts, planeVC, GetColor(hoverPart, GizmoAction::MoveXPlane, visPX));
			Mat4f mtxXPlane = Mat4f::Scale(0.15f, 0.15f, 1.0f) * Mat4f::Translate(0.75f, 0.75f, 0) * Mat4f::RotateY(90) * transform;
			DrawIndexed(mtxXPlane, PT_Triangles, VF_Color, planeVerts, planeVC, planeIndices, planeIC);
			DrawIndexed(mtxXPlane, PT_LineStrip, VF_Color, planeVerts, planeVC, planeFrameIndices, planeFrameIC);
			break;

		case 1:
			SetRenderState(DF_PRIMARY);
			prim::SetVertexColor(planeVerts, planeVC, GetColor(hoverPart, GizmoAction::MoveYPlane, visPY));
			Mat4f mtxYPlane = Mat4f::Scale(0.15f, 0.15f, 1.0f) * Mat4f::Translate(0.75f, 0.75f, 0) * Mat4f::RotateX(-90) * transform;
			DrawIndexed(mtxYPlane, PT_Triangles, VF_Color, planeVerts, planeVC, planeIndices, planeIC);
			DrawIndexed(mtxYPlane, PT_LineStrip, VF_Color, planeVerts, planeVC, planeFrameIndices, planeFrameIC);
			break;

		case 2:
			SetRenderState(DF_PRIMARY);
			prim::SetVertexColor(planeVerts, planeVC, GetColor(hoverPart, GizmoAction::MoveZPlane, visPZ));
			Mat4f mtxZPlane = Mat4f::Scale(0.15f, 0.15f, 1.0f) * Mat4f::Translate(0.75f, 0.75f, 0) * transform;
			DrawIndexed(mtxZPlane, PT_Triangles, VF_Color, planeVerts, planeVC, planeIndices, planeIC);
			DrawIndexed(mtxZPlane, PT_LineStrip, VF_Color, planeVerts, planeVC, planeFrameIndices, planeFrameIC);
			break;

		case 3:
			SetRenderState(DF_PRIMARY);
			tmpVerts[0].pos = { 0.15f, 0, 0 };
			tmpVerts[1].pos = { 0.9f, 0, 0 };
			tmpVerts[0].col = tmpVerts[1].col = GetColor(hoverPart, GizmoAction::MoveXAxis, visX);
			Draw(transform, PT_Lines, VF_Color, tmpVerts, 2);
			break;

		case 4:
			SetRenderState(DF_PRIMARY);
			tmpVerts[0].pos = { 0, 0.15f, 0 };
			tmpVerts[1].pos = { 0, 0.9f, 0 };
			tmpVerts[0].col = tmpVerts[1].col = GetColor(hoverPart, GizmoAction::MoveYAxis, visY);
			Draw(transform, PT_Lines, VF_Color, tmpVerts, 2);
			break;

		case 5:
			SetRenderState(DF_PRIMARY);
			tmpVerts[0].pos = { 0, 0, 0.15f };
			tmpVerts[1].pos = { 0, 0, 0.9f };
			tmpVerts[0].col = tmpVerts[1].col = GetColor(hoverPart, GizmoAction::MoveZAxis, visZ);
			Draw(transform, PT_Lines, VF_Color, tmpVerts, 2);
			break;

		case 6:
			SetRenderState(DF_PRIMARY | DF_Cull);
			prim::SetVertexColor(coneVerts, coneVC, GetColor(hoverPart, GizmoAction::MoveXAxis, visX));
			DrawIndexed(Mat4f::Scale(0.1f, 0.1f, 0.2f) * Mat4f::Translate(0, 0, 0.9f) * Mat4f::RotateY(-90) * transform,
				PT_Triangles, VF_Color, coneVerts, coneVC, coneIndices, coneIC);
			break;

		case 7:
			SetRenderState(DF_PRIMARY | DF_Cull);
			prim::SetVertexColor(coneVerts, coneVC, GetColor(hoverPart, GizmoAction::MoveYAxis, visY));
			DrawIndexed(Mat4f::Scale(0.1f, 0.1f, 0.2f) * Mat4f::Translate(0, 0, 0.9f) * Mat4f::RotateX(90) * transform,
				PT_Triangles, VF_Color, coneVerts, coneVC, coneIndices, coneIC);
			break;

		case 8:
			SetRenderState(DF_PRIMARY | DF_Cull);
			prim::SetVertexColor(coneVerts, coneVC, GetColor(hoverPart, GizmoAction::MoveZAxis, visZ));
			DrawIndexed(Mat4f::Scale(0.1f, 0.1f, 0.2f) * Mat4f::Translate(0, 0, 0.9f) * transform,
				PT_Triangles, VF_Color, coneVerts, coneVC, coneIndices, coneIC);
			break;

		case 9:
			SetRenderState(DF_PRIMARY | DF_Cull);
			prim::SetVertexColor(sphereVerts, sphereVC, GetColor(hoverPart, GizmoAction::MoveScreenPlane, 1.0f));
			DrawIndexed(Mat4f::Scale(0.15f) * transform,
				PT_Triangles, VF_Color, sphereVerts, sphereVC, sphereIndices, sphereIC);
			break;
		}
	}
}

static void RenderCircleCulled(const Mat4f& transform, Vec3f axis, Color4b color, const CameraBase& cam, Vec3f darkZoneCenter, float darkZoneRadius)
{
	using namespace gfx;

	constexpr int NUMSEG = 64;

	Vec3f axis1(axis.y, axis.z, axis.x);
	Vec3f axis2(axis.z, axis.x, axis.y);

	Point2f dzcWin = cam.WorldToWindowPoint(darkZoneCenter);
	float dzrWin = fabsf(cam.WorldToWindowSize(darkZoneRadius, darkZoneCenter));
	float dzz = fabsf(cam.GetViewMatrix().TransformPoint(darkZoneCenter).z);

	Vertex_PF3CB4 circleVerts[NUMSEG * 2];
	int realnum = 0;
	for (int i = 0; i < NUMSEG; i++)
	{
		float a1 = i * 2 * 3.14159f / NUMSEG;
		float a2 = (i + 1) * 2 * 3.14159f / NUMSEG;
		Vec3f dir1 = axis1 * cosf(a1) + axis2 * sinf(a1);
		Vec3f dir2 = axis1 * cosf(a2) + axis2 * sinf(a2);

		Vec3f wp1 = transform.TransformPoint(dir1);
		Vec3f wp2 = transform.TransformPoint(dir2);
		if (((cam.WorldToWindowPoint(wp1) - dzcWin).Length() < dzrWin && fabsf(cam.GetViewMatrix().TransformPoint(wp1).z) > dzz) ||
			((cam.WorldToWindowPoint(wp2) - dzcWin).Length() < dzrWin && fabsf(cam.GetViewMatrix().TransformPoint(wp2).z) > dzz))
			continue;

		circleVerts[realnum].pos = dir1;
		circleVerts[realnum].col = color;
		realnum++;

		circleVerts[realnum].pos = dir2;
		circleVerts[realnum].col = color;
		realnum++;
	}

	Draw(transform, PT_Lines, VF_Color, circleVerts, realnum);
}

static void Gizmo_Render_Rotate(GizmoAction hoverPart, const Mat4f& transform, float size, const CameraBase& cam)
{
	using namespace gfx;

	auto worldPoint = transform.GetTranslation();
	auto wsp = cam.WorldToWindowPoint(worldPoint);
	float sizeScale = fabsf(cam.WorldToWindowSize(1.0f, worldPoint));

	// 2D mode
	{
		auto prevRect = End3DMode();

		draw::AACircleLineCol(wsp, sizeScale * size, 1, Color4f(1, hoverPart == GizmoAction::RotateScreenAxis ? 0.5f : 0.2f));
		draw::AACircleLineCol(wsp, sizeScale * size * 0.4f, sizeScale * size * 0.8f, Color4f(1, hoverPart == GizmoAction::RotateTrackpad ? 0.3f : 0.1f));

		Begin3DMode(prevRect);
		SetViewMatrix(cam.GetViewMatrix());
		SetProjectionMatrix(cam.GetProjectionMatrix());
	}

	SetRenderState(DF_AlphaBlended | DF_ZTestOff | DF_ZWriteOff);

	float q = 0.82f;
	float dzr = size * 0.8f;
	RenderCircleCulled(transform, { q, 0, 0 }, GetColor(hoverPart, GizmoAction::RotateXAxis, 1), cam, worldPoint, dzr);
	RenderCircleCulled(transform, { 0, q, 0 }, GetColor(hoverPart, GizmoAction::RotateYAxis, 1), cam, worldPoint, dzr);
	RenderCircleCulled(transform, { 0, 0, q }, GetColor(hoverPart, GizmoAction::RotateZAxis, 1), cam, worldPoint, dzr);
}

static void Gizmo_Render_Scale(GizmoAction hoverPart, const Mat4f& transform, float size, const CameraBase& cam)
{
	using namespace gfx;

	constexpr prim::PlaneSettings PS = {};
	constexpr uint16_t planeVC = PS.CalcVertexCount();
	constexpr uint16_t planeIC = PS.CalcIndexCount();
	Vertex_PF3CB4 planeVerts[planeVC];
	uint16_t planeIndices[planeIC];
	prim::GeneratePlane(PS, planeVerts, planeIndices);

	constexpr uint16_t planeFrameIC = 5;
	uint16_t planeFrameIndices[planeFrameIC] = { 0, 1, 3, 2, 0 };

	constexpr prim::BoxSettings BS = {};
	constexpr uint16_t boxVC = BS.CalcVertexCount();
	constexpr uint16_t boxIC = BS.CalcIndexCount();
	Vertex_PF3CB4 boxVerts[boxVC];
	uint16_t boxIndices[boxIC];
	prim::GenerateBox(BS, boxVerts, boxIndices);

	Vertex_PF3CB4 tmpVerts[2] = {};

	Vec3f cam2center = (transform.GetTranslation() - cam.GetCameraEyePos()).Normalized();
	float dotX = fabsf(Vec3Dot((transform.TransformPoint({ 1, 0, 0 }) - transform.GetTranslation()).Normalized(), cam2center));
	float dotY = fabsf(Vec3Dot((transform.TransformPoint({ 0, 1, 0 }) - transform.GetTranslation()).Normalized(), cam2center));
	float dotZ = fabsf(Vec3Dot((transform.TransformPoint({ 0, 0, 1 }) - transform.GetTranslation()).Normalized(), cam2center));

	float visX = clamp(invlerp(0.99f, 0.97f, dotX), 0.0f, 1.0f);
	float visY = clamp(invlerp(0.99f, 0.97f, dotY), 0.0f, 1.0f);
	float visZ = clamp(invlerp(0.99f, 0.97f, dotZ), 0.0f, 1.0f);
	float visPX = clamp(invlerp(0.05f, 0.21f, dotX), 0.0f, 1.0f);
	float visPY = clamp(invlerp(0.05f, 0.21f, dotY), 0.0f, 1.0f);
	float visPZ = clamp(invlerp(0.05f, 0.21f, dotZ), 0.0f, 1.0f);

	constexpr int NUM_PARTS = 9;
	Vec3f centers[NUM_PARTS] =
	{
		{ 0, 0.15f, 0.15f }, // x plane
		{ 0.15f, 0, 0.15f }, // y plane
		{ 0.15f, 0.15f, 0 }, // z plane
		{ 0.45f, 0, 0 }, // x line
		{ 0, 0.45f, 0 }, // y line
		{ 0, 0, 0.45f }, // z line
		{ 0.95f, 0, 0 }, // x cube
		{ 0, 0.95f, 0 }, // y cube
		{ 0, 0, 0.95f }, // z cube
		//{ 0, 0, 0 }, // center cube
	};
	float distances[NUM_PARTS];
	Mat4f matWorldView = transform * cam.GetViewMatrix();
	for (int i = 0; i < NUM_PARTS; i++)
	{
		distances[i] = matWorldView.TransformPoint(centers[i]).z;
	}

	int sortedParts[NUM_PARTS] = { 0, 1, 2, 3, 4, 5, 6, 7, 8 };
	std::sort(std::begin(sortedParts), std::end(sortedParts), [&distances](int a, int b) { return distances[a] > distances[b]; });

	constexpr unsigned DF_PRIMARY = DF_AlphaBlended | DF_ZTestOff | DF_ZWriteOff;

	// uniform scale area
	{
		auto prevRect = End3DMode();

		auto worldPoint = transform.GetTranslation();
		auto wsp = cam.WorldToWindowPoint(worldPoint);
		float sizeScale = fabsf(cam.WorldToWindowSize(1.0f, worldPoint));

		draw::AACircleLineCol(wsp, sizeScale * size * 0.6f, sizeScale * size * 0.8f, Color4f(1, hoverPart == GizmoAction::ScaleUniform ? 0.2f : 0.1f));
		draw::AACircleLineCol(wsp, sizeScale * size, 1, Color4f(1, hoverPart == GizmoAction::ScaleUniform ? 0.5f : 0.2f));
		draw::AACircleLineCol(wsp, sizeScale * size * 0.2f, 1, Color4f(1, hoverPart == GizmoAction::ScaleUniform ? 0.5f : 0.2f));

		Begin3DMode(prevRect);
		SetViewMatrix(cam.GetViewMatrix());
		SetProjectionMatrix(cam.GetProjectionMatrix());
	}

	for (int i = 0; i < NUM_PARTS; i++)
	{
		switch (sortedParts[i])
		{
		case 0:
			SetRenderState(DF_PRIMARY);
			prim::SetVertexColor(planeVerts, planeVC, GetColor(hoverPart, GizmoAction::ScaleXPlane, visPX));
			Mat4f mtxXPlane = Mat4f::Scale(0.15f, 0.15f, 1.0f) * Mat4f::Translate(0.75f, 0.75f, 0) * Mat4f::RotateY(90) * transform;
			DrawIndexed(mtxXPlane, PT_Triangles, VF_Color, planeVerts, planeVC, planeIndices, planeIC);
			DrawIndexed(mtxXPlane, PT_LineStrip, VF_Color, planeVerts, planeVC, planeFrameIndices, planeFrameIC);
			break;

		case 1:
			SetRenderState(DF_PRIMARY);
			prim::SetVertexColor(planeVerts, planeVC, GetColor(hoverPart, GizmoAction::ScaleYPlane, visPY));
			Mat4f mtxYPlane = Mat4f::Scale(0.15f, 0.15f, 1.0f) * Mat4f::Translate(0.75f, 0.75f, 0) * Mat4f::RotateX(-90) * transform;
			DrawIndexed(mtxYPlane, PT_Triangles, VF_Color, planeVerts, planeVC, planeIndices, planeIC);
			DrawIndexed(mtxYPlane, PT_LineStrip, VF_Color, planeVerts, planeVC, planeFrameIndices, planeFrameIC);
			break;

		case 2:
			SetRenderState(DF_PRIMARY);
			prim::SetVertexColor(planeVerts, planeVC, GetColor(hoverPart, GizmoAction::ScaleZPlane, visPZ));
			Mat4f mtxZPlane = Mat4f::Scale(0.15f, 0.15f, 1.0f) * Mat4f::Translate(0.75f, 0.75f, 0) * transform;
			DrawIndexed(mtxZPlane, PT_Triangles, VF_Color, planeVerts, planeVC, planeIndices, planeIC);
			DrawIndexed(mtxZPlane, PT_LineStrip, VF_Color, planeVerts, planeVC, planeFrameIndices, planeFrameIC);
			break;

		case 3:
			SetRenderState(DF_PRIMARY);
			tmpVerts[1].pos = { 0.9f, 0, 0 };
			tmpVerts[0].col = tmpVerts[1].col = GetColor(hoverPart, GizmoAction::ScaleXAxis, visX);
			Draw(transform, PT_Lines, VF_Color, tmpVerts, 2);
			break;

		case 4:
			SetRenderState(DF_PRIMARY);
			tmpVerts[1].pos = { 0, 0.9f, 0 };
			tmpVerts[0].col = tmpVerts[1].col = GetColor(hoverPart, GizmoAction::ScaleYAxis, visY);
			Draw(transform, PT_Lines, VF_Color, tmpVerts, 2);
			break;

		case 5:
			SetRenderState(DF_PRIMARY);
			tmpVerts[1].pos = { 0, 0, 0.9f };
			tmpVerts[0].col = tmpVerts[1].col = GetColor(hoverPart, GizmoAction::ScaleZAxis, visZ);
			Draw(transform, PT_Lines, VF_Color, tmpVerts, 2);
			break;

		case 6:
			SetRenderState(DF_PRIMARY | DF_Cull);
			prim::SetVertexColor(boxVerts, boxVC, GetColor(hoverPart, GizmoAction::ScaleXAxis, visX));
			DrawIndexed(Mat4f::Scale(0.05f) * Mat4f::Translate(0, 0, 0.95f) * Mat4f::RotateY(-90) * transform,
				PT_Triangles, VF_Color, boxVerts, boxVC, boxIndices, boxIC);
			break;

		case 7:
			SetRenderState(DF_PRIMARY | DF_Cull);
			prim::SetVertexColor(boxVerts, boxVC, GetColor(hoverPart, GizmoAction::ScaleYAxis, visY));
			DrawIndexed(Mat4f::Scale(0.05f) * Mat4f::Translate(0, 0, 0.95f) * Mat4f::RotateX(90) * transform,
				PT_Triangles, VF_Color, boxVerts, boxVC, boxIndices, boxIC);
			break;

		case 8:
			SetRenderState(DF_PRIMARY | DF_Cull);
			prim::SetVertexColor(boxVerts, boxVC, GetColor(hoverPart, GizmoAction::ScaleZAxis, visZ));
			DrawIndexed(Mat4f::Scale(0.05f) * Mat4f::Translate(0, 0, 0.95f) * transform,
				PT_Triangles, VF_Color, boxVerts, boxVC, boxIndices, boxIC);
			break;
#if 0
		case 9:
			SetRenderState(DF_PRIMARY | DF_Cull);
			prim::SetVertexColor(boxVerts, boxVC, GetColor(hoverPart, GizmoAction::ScaleUniform, visZ));
			DrawIndexed(Mat4f::Scale(0.1f) * transform,
				PT_Triangles, VF_Color, boxVerts, boxVC, boxIndices, boxIC);
			break;
#endif
		}
	}
}

static void RenderInfiniteLine(Color4b col, Vec3f origin, Vec3f dir)
{
	using namespace gfx;

	constexpr int COUNT = 35;
	Vertex_PF3CB4 tmpVerts[COUNT];
	for (int i = 0; i < COUNT; i++)
	{
		float off = i - (COUNT / 2);
		float sign = off == 0 ? 0 : off > 0 ? 1 : -1;
		float absoff = fabsf(off);
		tmpVerts[i].pos = origin + dir * absoff * sign * powf(off, 10.0f);
		tmpVerts[i].col = col;
	}

	Draw(Mat4f::Identity(), PT_LineStrip, VF_Color, tmpVerts, COUNT);
}

static void RenderInfiniteAxisLine(int axisFlag, Mat4f basis)
{
	auto action = GizmoAction(axisFlag);
	Vec3f axis = GetAxis(action);
	RenderInfiniteLine(
		GetColor(action),
		basis.GetTranslation(),
		basis.TransformDirection(axis).Normalized());
}

static int GetOtherAxis(GizmoAction action, bool second)
{
	switch (int(action) & GAF_Axis_MASK)
	{
	case GAF_Axis_X: return second ? GAF_Axis_Y : GAF_Axis_Z;
	case GAF_Axis_Y: return second ? GAF_Axis_Z : GAF_Axis_X;
	case GAF_Axis_Z: return second ? GAF_Axis_X : GAF_Axis_Y;
	default: return 0;
	}
}

void Gizmo::Render(const CameraBase& cam, const IGizmoEditable& editable)
{
	using namespace gfx;

	float size = settings.size;
	auto xf = editable.GetGizmoLocation();
	if (settings.sizeMode != GizmoSizeMode::Scene)
	{
		const auto& pm = cam.GetProjectionMatrix();
		float fovQ = pm.m[1][1];
		float q = max(0.00001f, fabsf(cam.GetViewMatrix().TransformPoint(xf.position).z / fovQ));
		size *= q;
		if (settings.sizeMode == GizmoSizeMode::ViewPixels)
			size /= cam.GetWindowRect().GetHeight();
	}
	_finalSize = size;
	_combinedXF = Mat4f::Scale(size) * GetTransformBasis(xf, settings.isWorldSpace);

	if (_selectedPart != GizmoAction::None)
	{
		Mat4f xfBasis = GetTransformBasis(_origXF, _curXFWorldSpace);

		SetRenderState(DF_AlphaBlended | DF_ZTestOff | DF_ZWriteOff);
		SetTexture(nullptr);

		switch (int(_selectedPart) & GAF_Mode_MASK)
		{
		case GAF_Mode_Rotate:
			RenderCircleCulled(xfBasis, GetAxis(_selectedPart) * size, GetColor(_selectedPart), cam, {}, -1);
			// pass-through
		case GAF_Mode_Move:
		case GAF_Mode_Scale:
			switch (int(_selectedPart) & GAF_Shape_MASK)
			{
			case GAF_Shape_Axis:
				RenderInfiniteAxisLine(int(_selectedPart), xfBasis);
				break;
			case GAF_Shape_Plane: {
				RenderInfiniteAxisLine(GetOtherAxis(_selectedPart, false), xfBasis);
				RenderInfiniteAxisLine(GetOtherAxis(_selectedPart, true), xfBasis);
				break; }
			}
			break;
		}
		return;
	}

	if (!settings.visible)
		return;

	SetTexture(nullptr);

	switch (settings.type)
	{
	case GizmoType::Move:
		Gizmo_Render_Move(_hoveredPart, _combinedXF, cam);
		break;
	case GizmoType::Rotate:
		Gizmo_Render_Rotate(_hoveredPart, _combinedXF, size, cam);
		break;
	case GizmoType::Scale:
		Gizmo_Render_Scale(_hoveredPart, _combinedXF, size, cam);
		break;
	}
}

} // ui
