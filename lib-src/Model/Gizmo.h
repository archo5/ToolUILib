
#pragma once
#include "Graphics.h"
#include "../Core/SerializationBasic.h"


namespace ui {

enum GizmoPotentialChangeMask
{
	GPC_Move = 1 << 0,
	GPC_Rotate = 1 << 1,
	GPC_Scale = 1 << 2,
};

struct IGizmoEditable
{
	virtual void Backup(DataWriter& data) const = 0;
	virtual void Transform(DataReader& data, const Mat4f* xf, u8 ptnChanges) = 0;
	virtual Transform3Df GetGizmoLocation() const = 0;
};

struct GizmoEditablePosVec3f : IGizmoEditable
{
	Vec3f& pos;

	GizmoEditablePosVec3f(Vec3f& p) : pos(p) {}
	void Backup(DataWriter& data) const override
	{
		data << pos;
	}
	void Transform(DataReader& data, const Mat4f* xf, u8 ptnChanges) override
	{
		data << pos;
		if (xf && (ptnChanges && GPC_Move))
			pos = xf->TransformPoint(pos);
	}
	Transform3Df GetGizmoLocation() const override
	{
		return { pos };
	}
};

struct GizmoEditableMat4f : IGizmoEditable
{
	Mat4f& mat4f;

	GizmoEditableMat4f(Mat4f& m) : mat4f(m) {}
	void Backup(DataWriter& data) const override
	{
		data << mat4f;
	}
	void Transform(DataReader& data, const Mat4f* xf, u8 ptnChanges) override
	{
		data << mat4f;
		if (xf)
			mat4f = mat4f * *xf;
	}
	Transform3Df GetGizmoLocation() const override
	{
		return Transform3Df::FromMatrixLossy(mat4f);
	}
};

struct GizmoEditableTransform3Df : IGizmoEditable
{
	Transform3Df& xform;

	GizmoEditableTransform3Df(Transform3Df& x) : xform(x) {}
	void Backup(DataWriter& data) const override
	{
		data << xform;
	}
	void Transform(DataReader& data, const Mat4f* xf, u8 ptnChanges) override
	{
		data << xform;
		if (xf)
		{
			auto nxf = xform * *xf;
			if (ptnChanges & GPC_Move)
				xform.position = nxf.position;
			if (ptnChanges & GPC_Rotate)
				xform.rotation = nxf.rotation;
		}
	}
	Transform3Df GetGizmoLocation() const override
	{
		return xform;
	}
};

struct GizmoEditableTransformScale3Df : IGizmoEditable
{
	TransformScale3Df& xform;

	GizmoEditableTransformScale3Df(TransformScale3Df& x) : xform(x) {}
	void Backup(DataWriter& data) const override
	{
		data << xform;
	}
	void Transform(DataReader& data, const Mat4f* xf, u8 ptnChanges) override
	{
		data << xform;
		if (xf)
		{
			auto nxf = xform.TransformLossy(*xf);
			if (ptnChanges & GPC_Move)
				xform.position = nxf.position;
			if (ptnChanges & GPC_Rotate)
				xform.rotation = nxf.rotation;
			if (ptnChanges & GPC_Scale)
				xform.scale = nxf.scale;
		}
	}
	Transform3Df GetGizmoLocation() const override
	{
		return xform.WithoutScale();
	}
};

enum class GizmoType : uint8_t
{
	Move,
	Rotate,
	Scale,
};

enum class GizmoSizeMode
{
	Scene,
	ViewNormalizedY,
	ViewPixels,
};

enum GizmoActionFlags
{
	GAF_Axis_X = 0,
	GAF_Axis_Y = 1,
	GAF_Axis_Z = 2,
	GAF_Axis_ScreenOrUniform = 3,
	GAF_Axis_MASK = 0x3,

	GAF_Shape_Axis = 0 << 2,
	GAF_Shape_Plane = 1 << 2,
	GAF_Shape_Trackpad = 2 << 2,
	GAF_Shape_ScreenOrUniform = 3 << 2,
	GAF_Shape_MASK = 0x3 << 2,

	GAF_ScreenOrUniform = 0xf, // short

	GAF_Mode_Move = 1 << 4,
	GAF_Mode_Rotate = 2 << 4,
	GAF_Mode_Scale = 3 << 4,
	GAF_Mode_MASK = 0x3 << 4,
};

enum class GizmoAction : uint8_t
{
	None = 0,

	MoveXAxis = GAF_Axis_X | GAF_Shape_Axis | GAF_Mode_Move,
	MoveYAxis = GAF_Axis_Y | GAF_Shape_Axis | GAF_Mode_Move,
	MoveZAxis = GAF_Axis_Z | GAF_Shape_Axis | GAF_Mode_Move,
	MoveXPlane = GAF_Axis_X | GAF_Shape_Plane | GAF_Mode_Move,
	MoveYPlane = GAF_Axis_Y | GAF_Shape_Plane | GAF_Mode_Move,
	MoveZPlane = GAF_Axis_Z | GAF_Shape_Plane | GAF_Mode_Move,
	MoveScreenPlane = GAF_ScreenOrUniform | GAF_Mode_Move,

	ScaleXAxis = GAF_Axis_X | GAF_Shape_Axis | GAF_Mode_Scale,
	ScaleYAxis = GAF_Axis_Y | GAF_Shape_Axis | GAF_Mode_Scale,
	ScaleZAxis = GAF_Axis_Z | GAF_Shape_Axis | GAF_Mode_Scale,
	ScaleXPlane = GAF_Axis_X | GAF_Shape_Plane | GAF_Mode_Scale,
	ScaleYPlane = GAF_Axis_Y | GAF_Shape_Plane | GAF_Mode_Scale,
	ScaleZPlane = GAF_Axis_Z | GAF_Shape_Plane | GAF_Mode_Scale,
	ScaleUniform = GAF_ScreenOrUniform | GAF_Mode_Scale,

	RotateXAxis = GAF_Axis_X | GAF_Shape_Axis | GAF_Mode_Rotate,
	RotateYAxis = GAF_Axis_Y | GAF_Shape_Axis | GAF_Mode_Rotate,
	RotateZAxis = GAF_Axis_Z | GAF_Shape_Axis | GAF_Mode_Rotate,
	RotateScreenAxis = GAF_ScreenOrUniform | GAF_Mode_Rotate,
	RotateTrackpad = GAF_Axis_ScreenOrUniform | GAF_Shape_Trackpad | GAF_Mode_Rotate,
};

struct GizmoKeyDetect
{
	static constexpr const u8
		None = 0,
		Start = 1 << 0,
		Exit = 1 << 1,
		ModeSwitch = 1 << 2,
		AxisSwitch = 1 << 3,
		All = 0xf;
};

struct GizmoSettings
{
	struct Edit
	{
		float moveSlowdownFactor = 0.1f;
		float moveSnapDist = 1;
		float moveSlowSnapDist = 0.1f;

		float rotateSlowdownFactor = 1.0f / 15.0f;
		float rotateSnapAngleDeg = 15;
		float rotateSlowSnapAngleDeg = 1;

		float scaleSlowdownFactor = 0.1f;
		float scaleSnapDist = 0.1f;
		float scaleSlowSnapDist = 0.01f;
	}
	edit;

	GizmoType type = GizmoType::Move;
	bool isWorldSpace = true;
	bool visible = true;
	u8 detectsKeys = GizmoKeyDetect::All;
	float size = 100.0f;
	GizmoSizeMode sizeMode = GizmoSizeMode::ViewPixels;
};

struct CameraBase;

struct Gizmo
{
	GizmoSettings settings;

	Mat4f _combinedXF = Mat4f::Identity();
	float _finalSize = 1;

	GizmoAction _selectedPart = GizmoAction::None;
	GizmoAction _hoveredPart = GizmoAction::None;
	bool _curXFWorldSpace = true;
	u8 _buttonPresses = 0;
	Point2f _lastCursorPos = {};

	Point2f _origCursorWinPos = {};
	Point2f _totalMovedWinVec = {};
	float _totalAngleDiff = 0;
	Point2f _origCenterWinPos = {};
	Array<char> _origData;
	Transform3Df _origXF;

	void Start(GizmoAction action, Point2f cursorPoint, const CameraBase& cam, const IGizmoEditable& editable);
	// returns true if the event was processed (editable was modified or transforming started)
	bool OnEvent(Event& e, const CameraBase& cam, const IGizmoEditable& editable);
	void Render(const CameraBase& cam, const IGizmoEditable& editable);
};

} // ui
