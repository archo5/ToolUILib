
#include "pch.h"

#include "../lib-src/Core/3DMath.h"
#include "../lib-src/Core/3DCamera.h"
#include "../lib-src/Model/Gizmo.h"
#include "../lib-src/Render/RHI.h"
#include "../lib-src/Render/Primitives.h"


struct The3DViewTest : ui::Buildable
{
	struct VertPC
	{
		float x, y, z;
		ui::Color4b col;
	};
	void Build() override
	{
		ui::Push<ui::FrameElement>().SetDefaultFrameStyle(ui::DefaultFrameStyle::GroupBox);
		{
			auto& v = ui::Push<ui::View3D>();
			v.SetFlag(ui::UIObject_DB_CaptureMouseOnLeftClick, true);
			v.HandleEvent() = [this](ui::Event& e)
			{
				if (e.type == ui::EventType::MouseMove)
					mousePos = e.position;
				camera.OnEvent(e);
			};
			v.onRender = [this](ui::UIRect r) { Render3DView(r); };
			v.onPaintOverlay = [this](ui::UIRect r) { RenderOverlay(r); };
			{
				auto tmpl = ui::Push<ui::PlacementLayoutElement>().GetSlotTemplate();

				auto* leftCorner = UI_BUILD_ALLOC(ui::PointAnchoredPlacement)();
				leftCorner->SetAnchorAndPivot({ 0, 0 });
				tmpl->placement = leftCorner;
				tmpl->measure = false;
				ui::Push<ui::StackTopDownLayoutElement>();

				ui::Text("Overlay text");
				ui::Make<ui::ColorBlock>().SetColor({ 100, 0, 200, 255 });
				ui::MakeWithText<ui::Button>("Reset")
					+ ui::AddEventHandler(ui::EventType::Activate, [this](ui::Event&)
				{
					bool rh = camera.rightHanded;
					camera = {};
					camera.rightHanded = rh;
				});
				ui::imm::EditBool(camera.rightHanded, "Right-handed");

				ui::Pop();
				ui::Pop();
			}
			ui::Pop();
		}
		ui::Pop();
	}
	void RenderOverlay(const ui::UIRect& rect)
	{
		float y = rect.y0 + 100;
		auto* font = ui::GetFontByFamily(ui::FONT_FAMILY_SANS_SERIF);

		auto fwd = camera.GetCameraForwardDir();
		ui::draw::TextLine(font, 12, rect.x0, y,
			ui::Format("Dir (Z): %g;%g;%g", fwd.x, fwd.y, fwd.z),
			ui::Color4b::White());
		y += 12;

		auto up = camera.GetCameraUpDir();
		ui::draw::TextLine(font, 12, rect.x0, y,
			ui::Format("Up (Y): %g;%g;%g", up.x, up.y, up.z),
			ui::Color4b::White());
		y += 12;

		auto rt = camera.GetCameraRightDir();
		ui::draw::TextLine(font, 12, rect.x0, y,
			ui::Format("Right (X): %g;%g;%g", rt.x, rt.y, rt.z),
			ui::Color4b::White());
		y += 12;

		auto pos = camera.GetCameraPosition();
		ui::draw::TextLine(font, 12, rect.x0, y,
			ui::Format("Position: %g;%g;%g", pos.x, pos.y, pos.z),
			ui::Color4b::White());
		y += 12;

		auto epos = camera.GetCameraEyePos();
		ui::draw::TextLine(font, 12, rect.x0, y,
			ui::Format("Eye pos.: %g;%g;%g", epos.x, epos.y, epos.z),
			ui::Color4b::White());
		y += 12;
	}
	void Render3DView(const ui::UIRect& rect)
	{
		using namespace ui::gfx;

		camera.SetWindowRect(rect);
		camera.SetProjectionMatrix(camera.rightHanded
			? ui::Mat4f::PerspectiveFOVRH(90, rect.GetAspectRatio(), 0.01f, 1000)
			: ui::Mat4f::PerspectiveFOVLH(90, rect.GetAspectRatio(), 0.01f, 1000));

		Clear(16, 15, 14, 255);
		SetProjectionMatrix(camera.GetProjectionMatrix());
		SetViewMatrix(camera.GetViewMatrix());
		SetForcedColor(ui::Color4f(0.5f));
		VertPC verts[] =
		{
			{ -1, -1, 0, { 100, 150, 200, 255 } },
			{ 1, -1, 0, { 100, 0, 200, 255 } },
			{ -1, 1, 0, { 200, 150, 0, 255 } },
			{ 1, 1, 0, { 150, 50, 0, 255 } },
		};
		uint16_t indices[] = { 0, 1, 2, 1, 3, 2 };
		DrawIndexed(ui::Mat4f::Translate(0, 0, -1), PT_Triangles, VF_Color, verts, 4, indices, 6);
		DrawIndexed(ui::Mat4f::Translate(0, 0, -1) * ui::Mat4f::RotateX(90), PT_Triangles, VF_Color, verts, 4, indices, 6);
		DrawIndexed(ui::Mat4f::Translate(0, 0, -1) * ui::Mat4f::RotateY(-90), PT_Triangles, VF_Color, verts, 4, indices, 6);

		{
			constexpr ui::prim::PlaneSettings S = { 2, 3 };
			constexpr auto vc = S.CalcVertexCount();
			constexpr auto ic = S.CalcIndexCount();
			ui::Vertex_PF3CB4 verts[vc];
			uint16_t idcs[ic];
			ui::prim::GeneratePlane(S, verts, idcs);
			DrawPrim(verts, vc, idcs, ic, ui::Color4f(0.5f, 0.2f, 0.8f, 0.7f), ui::Mat4f::Scale(0.1f) * ui::Mat4f::Translate(0.4f, 0, 0));
		}

		{
			constexpr ui::prim::BoxSettings S = { 2, 3, 4 };
			constexpr auto vc = S.CalcVertexCount();
			constexpr auto ic = S.CalcIndexCount();
			ui::Vertex_PF3CB4 verts[vc];
			uint16_t idcs[ic];
			ui::prim::GenerateBox(S, verts, idcs);
			DrawPrim(verts, vc, idcs, ic, ui::Color4f(0.2f, 0.5f, 0.8f, 0.7f), ui::Mat4f::Scale(0.1f) * ui::Mat4f::Translate(0.2f, 0, 0));
		}

		{
			constexpr ui::prim::ConeSettings S = { 31 };
			constexpr auto vc = S.CalcVertexCount();
			constexpr auto ic = S.CalcIndexCount();
			ui::Vertex_PF3CB4 verts[vc];
			uint16_t idcs[ic];
			ui::prim::GenerateCone(S, verts, idcs);
			DrawPrim(verts, vc, idcs, ic, ui::Color4f(0.2f, 0.8f, 0.5f, 0.7f), ui::Mat4f::Scale(0.1f) * ui::Mat4f::Translate(0.0f, 0, 0));
		}

		{
			constexpr ui::prim::UVSphereSettings S = { 31, 13 };
			constexpr auto vc = S.CalcVertexCount();
			constexpr auto ic = S.CalcIndexCount();
			ui::Vertex_PF3CB4 verts[vc];
			uint16_t idcs[ic];
			ui::prim::GenerateUVSphere(S, verts, idcs);
			DrawPrim(verts, vc, idcs, ic, ui::Color4f(0.5f, 0.8f, 0.2f, 0.7f), ui::Mat4f::Scale(0.1f) * ui::Mat4f::Translate(-0.2f, 0, 0));
		}

		{
			constexpr ui::prim::BoxSphereSettings S = { 5 };
			constexpr auto vc = S.CalcVertexCount();
			constexpr auto ic = S.CalcIndexCount();
			ui::Vertex_PF3CB4 verts[vc];
			uint16_t idcs[ic];
			ui::prim::GenerateBoxSphere(S, verts, idcs);
			DrawPrim(verts, vc, idcs, ic, ui::Color4f(0.8f, 0.5f, 0.2f, 0.7f), ui::Mat4f::Scale(0.1f) * ui::Mat4f::Translate(-0.4f, 0, 0));
		}

		{
			constexpr ui::prim::UVCapsuleSettings S = { 31, 5 };
			constexpr auto vc = S.CalcVertexCount();
			constexpr auto ic = S.CalcIndexCount();
			ui::Vertex_PF3CB4 verts[vc];
			uint16_t idcs[ic];
			ui::prim::GenerateUVCapsule(S, verts, idcs, { 0, 0, -1 }, { 0, 0, 1 }, { 1, 0, 0 }, 1);
			DrawPrim(verts, vc, idcs, ic, ui::Color4f(0.5f, 0.8f, 0.2f, 0.7f), ui::Mat4f::Scale(0.1f) * ui::Mat4f::Translate(-0.6f, 0, 0));
		}

		SetRenderState(DF_ZTestOff | DF_ZWriteOff);
		auto ray = camera.GetRayWP(mousePos);
		auto rpir = RayPlaneIntersect(ray.origin, ray.direction, { 0, 0, 1, -1 });
		ui::Vec3f isp = ray.GetPoint(rpir.dist);
		DrawIndexed(ui::Mat4f::Scale(0.1f, 0.1f, 0.1f) * ui::Mat4f::Translate(isp), PT_Triangles, VF_Color, verts, 4, indices, 6);
	}
	void DrawPrim(ui::Vertex_PF3CB4* verts, uint16_t vc, uint16_t* idcs, unsigned ic, const ui::Color4b& col, const ui::Mat4f& m)
	{
		using namespace ui::gfx;

		ui::prim::SetVertexColor(verts, vc, col);

		SetRenderState(DF_AlphaBlended | DF_Cull);
		DrawIndexed(m, PT_Triangles, VF_Color, verts, vc, idcs, ic);

		SetRenderState(DF_Wireframe | DF_ForceColor);
		DrawIndexed(m, PT_Triangles, VF_Color, verts, vc, idcs, ic);
	}

	ui::OrbitCamera camera;
	ui::Point2f mousePos = {};
};
void Test_3DView()
{
	ui::Make<The3DViewTest>();
}


namespace ui {

struct GizmoContainer;
struct GizmoPersistentObject : IPersistentObject, IGizmoEditable
{
	Gizmo gizmo;
	Buildable* _buildable = nullptr;
	GizmoContainer* _cont = nullptr;
	Array<char> _savedData;
	Transform3Df _savedXF;
	Optional<Mat4f> editMatrix;
	u8 editPtnChanges = 0;
	bool edited = false;

	~GizmoPersistentObject();

	// CreateUIObject
	void _InitReset() {}
	// IPersistentObject
	void PO_ResetConfiguration() override
	{
		gizmo.settings = {};
	}
	// IGizmoEditable
	void Backup(DataWriter& data) const override
	{
		data._data.AppendRange(_savedData);
	}
	void Transform(DataReader& data, const Mat4f* xf, u8 ptnChanges) override
	{
		_savedData = data._data;
		if (xf)
			editMatrix = *xf;
		else
			editMatrix = {};
		editPtnChanges = ptnChanges;
		edited = true;
		if (_buildable)
			_buildable->Rebuild();
	}
	Transform3Df GetGizmoLocation() const override
	{
		return _savedXF;
	}
};

struct GizmoContainer
{
	Array<GizmoPersistentObject*> _gizmos;

	void OnEvent(Event& e, const CameraBase& cam)
	{
		for (auto* gizmo : _gizmos)
			gizmo->gizmo.OnEvent(e, cam, *gizmo);
	}
	void Render(const CameraBase& cam)
	{
		for (auto* gizmo : _gizmos)
			gizmo->gizmo.Render(cam, *gizmo);
	}
};

GizmoPersistentObject::~GizmoPersistentObject()
{
	if (_cont)
		_cont->_gizmos.RemoveFirstOf(this);
}

namespace imm {

bool EditTransform(GizmoContainer& gc, const IGizmoEditable& ge, const GizmoSettings& settings = {})
{
	auto& go = New<GizmoPersistentObject>();
	go.gizmo.settings = settings;
	if (!go._cont)
	{
		go._cont = &gc;
		gc._gizmos.Append(&go);
		go._buildable = GetCurrentBuildable();
	}

	bool edited = go.edited;
	if (edited)
	{
		DataReader dr(go._savedData);
		const_cast<IGizmoEditable&>(ge).Transform(dr, go.editMatrix.GetValuePtrOrNull(), go.editPtnChanges);
		GetCurrentBuildable()->Rebuild();
		go.edited = false;
	}
	else
	{
		go._savedData.Clear();
		DataWriter dw(go._savedData);
		ge.Backup(dw);
		go._savedXF = ge.GetGizmoLocation();
	}

	return edited;
}

} // imm

} // ui

struct GizmoTest : ui::Buildable
{
	ui::OrbitCamera camera;
	bool useImmediate = true;
	ui::GizmoSettings gizmoSettings;
	ui::GizmoContainer gizmoCont;
	ui::Gizmo gizmo;
	ui::Mat4f xf = ui::Mat4f::Translate(0.01f, 0.02f, 0.03f);
	float fov = 90;

	struct VertPC
	{
		float x, y, z;
		ui::Color4b col;
	};
	void Build() override
	{
		ui::Push<ui::FrameElement>().SetDefaultFrameStyle(ui::DefaultFrameStyle::GroupBox);
		{
			auto& v = ui::Push<ui::View3D>();
			v.SetFlag(ui::UIObject_DB_CaptureMouseOnLeftClick, true);
			v.HandleEvent() = [this](ui::Event& e)
			{
				if (e.type == ui::EventType::ButtonDown)
					e.context->SetKeyboardFocus(e.current);
				if (useImmediate)
				{
					gizmoCont.OnEvent(e, camera);
				}
				else
				{
					if (gizmo.OnEvent(e, camera, ui::GizmoEditableMat4f(xf)))
						Rebuild();
				}
				camera.OnEvent(e);
			};
			v.onRender = [this](ui::UIRect r) { Render3DView(r); };
			{
				auto tmpl = ui::Push<ui::PlacementLayoutElement>().GetSlotTemplate();

				auto* leftTop = UI_BUILD_ALLOC(ui::PointAnchoredPlacement)();
				leftTop->SetAnchorAndPivot({ 0, 0 });
				tmpl->placement = leftTop;
				tmpl->measure = false;
				ui::Push<ui::SizeConstraintElement>().SetWidth(120);
				ui::Push<ui::FrameElement>().SetDefaultFrameStyle(ui::DefaultFrameStyle::GroupBox);
				ui::Push<ui::StackTopDownLayoutElement>();
				{
					if (ui::imm::Button("Add test offset"))
					{
						xf = xf * ui::Mat4f::Translate(100, 200, 300);
						camera.pivot += ui::Vec3f(100, 200, 300);
						camera._UpdateViewMatrix();
						camera._UpdateViewProjMatrix();
					}
					ui::MakeWithText<ui::Header>("Camera");
					ui::imm::PropEditFloat("FOV", fov, {}, {}, { 1.0f, 179.0f });

					{
						ui::LabeledProperty::Scope ps;
						ui::MakeWithText<ui::Header>("Object");
						if (ui::imm::Button("Reset"))
							xf = ui::Mat4f::Translate(0.01f, 0.02f, 0.03f);
					}

					auto pos = xf.TransformPoint({ 0, 0, 0 });
					ui::MakeWithTextf<ui::LabelFrame>("pos=%g;%g;%g", pos.x, pos.y, pos.z);
				}
				ui::Pop();
				ui::Pop();
				ui::Pop();

				auto* rightTop = UI_BUILD_ALLOC(ui::PointAnchoredPlacement)();
				rightTop->SetAnchorAndPivot({ 1, 0 });
				tmpl->placement = rightTop;
				tmpl->measure = false;
				ui::Push<ui::SizeConstraintElement>().SetWidth(180);
				ui::Push<ui::FrameElement>().SetDefaultFrameStyle(ui::DefaultFrameStyle::GroupBox);
				ui::Push<ui::StackTopDownLayoutElement>();
				{
					ui::imm::EditBool(useImmediate, "Use immediate");
					ui::MakeWithText<ui::Header>("Gizmo");
					ui::imm::PropEditFloat("Size", gizmoSettings.size, {}, {}, { 0.001f, 200.0f });
					ui::imm::Label("Size mode"), ui::imm::DropdownMenuList(gizmoSettings.sizeMode, UI_BUILD_ALLOC(ui::ZeroSepCStrOptionList)("Scene\0View normalized (Y)\0View pixels\0"));
					{
						ui::LabeledProperty::Scope ps("Type");
						ui::imm::RadioButton(gizmoSettings.type, ui::GizmoType::Move, "M", {}, ui::imm::ButtonStateToggleSkin());
						ui::imm::RadioButton(gizmoSettings.type, ui::GizmoType::Rotate, "R", {}, ui::imm::ButtonStateToggleSkin());
						ui::imm::RadioButton(gizmoSettings.type, ui::GizmoType::Scale, "S", {}, ui::imm::ButtonStateToggleSkin());
					}
					{
						ui::LabeledProperty::Scope ps("Space");
						ui::imm::RadioButton(gizmoSettings.isWorldSpace, false, "Local", {}, ui::imm::ButtonStateToggleSkin());
						ui::imm::RadioButton(gizmoSettings.isWorldSpace, true, "World", {}, ui::imm::ButtonStateToggleSkin());
					}

					if (useImmediate)
					{
						ui::imm::EditTransform(gizmoCont, ui::GizmoEditableMat4f(xf), gizmoSettings);
					}
				}
				ui::Pop();
				ui::Pop();
				ui::Pop();

				ui::Pop();
			}
			ui::Pop();
		}
		ui::Pop();
	}
	void Render3DView(const ui::UIRect& rect)
	{
		using namespace ui::gfx;

		camera.SetWindowRect(rect);
		camera.SetProjectionMatrix(ui::Mat4f::PerspectiveFOVLH(fov, rect.GetAspectRatio(), 0.01f, 1000));

		Clear(16, 15, 14, 255);
		SetProjectionMatrix(camera.GetProjectionMatrix());
		SetViewMatrix(camera.GetViewMatrix());
		VertPC verts[] =
		{
			{ -1, -1, 0, { 100, 150, 200, 255 } },
			{ 1, -1, 0, { 100, 0, 200, 255 } },
			{ -1, 1, 0, { 200, 150, 0, 255 } },
			{ 1, 1, 0, { 150, 50, 0, 255 } },
		};
		uint16_t indices[] = { 0, 1, 2, 1, 3, 2 };
		DrawIndexed(ui::Mat4f::Translate(0, 0, -1), PT_Triangles, VF_Color, verts, 4, indices, 6);
		DrawIndexed(ui::Mat4f::Translate(0, 0, -1) * ui::Mat4f::RotateX(90), PT_Triangles, VF_Color, verts, 4, indices, 6);
		DrawIndexed(ui::Mat4f::Translate(0, 0, -1) * ui::Mat4f::RotateY(-90), PT_Triangles, VF_Color, verts, 4, indices, 6);

		RenderObject(ui::Mat4f::Scale(0.1f) * xf);

		if (useImmediate)
		{
			gizmoCont.Render(camera);
		}
		else
		{
			gizmo.settings = gizmoSettings;
			gizmo.Render(camera, ui::GizmoEditableMat4f(xf));
		}
	}

	void RenderObject(const ui::Mat4f& mtx)
	{
		using namespace ui::gfx;

		SetRenderState(DF_Cull);

		{
			constexpr ui::prim::BoxSettings S = {};
			constexpr auto vc = S.CalcVertexCount();
			constexpr auto ic = S.CalcIndexCount();
			ui::Vertex_PF3CB4 verts[vc];
			uint16_t idcs[ic];
			ui::prim::GenerateBox(S, verts, idcs);
			ui::prim::SetVertexColor(verts, vc, ui::Color4f(0.1f, 1));
			DrawIndexed(mtx, PT_Triangles, VF_Color, verts, vc, idcs, ic);
		}

		{
			constexpr ui::prim::ConeSettings S = { 32 };
			constexpr auto vc = S.CalcVertexCount();
			constexpr auto ic = S.CalcIndexCount();
			ui::Vertex_PF3CB4 verts[vc];
			uint16_t idcs[ic];
			ui::prim::GenerateCone(S, verts, idcs);
			ui::prim::SetVertexColor(verts, vc, ui::Color4f(0.2f, 0, 0, 1));
			DrawIndexed(ui::Mat4f::Translate(0, 0, 1) * ui::Mat4f::RotateY(-90) * mtx, PT_Triangles, VF_Color, verts, vc, idcs, ic);
			ui::prim::SetVertexColor(verts, vc, ui::Color4f(0, 0.2f, 0, 1));
			DrawIndexed(ui::Mat4f::Translate(0, 0, 1) * ui::Mat4f::RotateX(90) * mtx, PT_Triangles, VF_Color, verts, vc, idcs, ic);
			ui::prim::SetVertexColor(verts, vc, ui::Color4f(0, 0, 0.2f, 1));
			DrawIndexed(ui::Mat4f::Translate(0, 0, 1) * mtx, PT_Triangles, VF_Color, verts, vc, idcs, ic);
		}
	}
};
void Test_Gizmo()
{
	ui::Make<GizmoTest>();
}


struct QuaternionTest : ui::Buildable
{
	ui::OrbitCamera camera;
	float fov = 45;
	ui::Vec3f angles;
	bool zyx = false;
	bool useMtx = false;

	ui::Quat GetQuat()
	{
		if (!zyx)
			return ui::Quat::RotateEulerAnglesXYZ(angles);
		else
			return ui::Quat::RotateEulerAnglesZYX(angles);
	}
	ui::Mat4f GetMtx()
	{
		if (useMtx)
		{
			if (!zyx)
			{
				return ui::Mat4f::RotateX(angles.x)
					* ui::Mat4f::RotateY(angles.y)
					* ui::Mat4f::RotateZ(angles.z);
			}
			else
			{
				return ui::Mat4f::RotateZ(angles.z)
					* ui::Mat4f::RotateY(angles.y)
					* ui::Mat4f::RotateX(angles.x);
			}
		}
		return ui::Mat4f::Rotate(GetQuat());
	}

	struct VertPC
	{
		float x, y, z;
		ui::Color4b col;
	};
	void Build() override
	{
		ui::Push<ui::FrameElement>().SetDefaultFrameStyle(ui::DefaultFrameStyle::GroupBox);
		{
			auto& v = ui::Push<ui::View3D>();
			v.SetFlag(ui::UIObject_DB_CaptureMouseOnLeftClick, true);
			v.HandleEvent() = [this](ui::Event& e)
			{
				if (e.type == ui::EventType::ButtonDown)
					e.context->SetKeyboardFocus(e.current);
				camera.OnEvent(e);
			};
			v.onRender = [this](ui::UIRect r) { Render3DView(r); };
			{
				auto tmpl = ui::Push<ui::PlacementLayoutElement>().GetSlotTemplate();

				auto* leftTop = UI_BUILD_ALLOC(ui::PointAnchoredPlacement)();
				leftTop->SetAnchorAndPivot({ 0, 0 });
				tmpl->placement = leftTop;
				tmpl->measure = false;
				ui::Push<ui::SizeConstraintElement>().SetWidth(200);
				ui::Push<ui::FrameElement>().SetDefaultFrameStyle(ui::DefaultFrameStyle::GroupBox);
				ui::Push<ui::StackTopDownLayoutElement>();
				{
					ui::MakeWithText<ui::Header>("Camera");
					ui::imm::PropEditFloat("FOV", fov, {}, {}, { 1.0f, 179.0f });
					ui::imm::PropEditFloatVec("R", &angles.x, ui::imm::XYZ);
					ui::imm::Label("Mode"), ui::imm::DropdownMenuList(zyx, UI_BUILD_ALLOC(ui::ZeroSepCStrOptionList)("XYZ\0" "ZYX\0"));
					ui::imm::PropEditBool("Use mtx", useMtx);
				}
				ui::Pop();
				ui::Pop();
				ui::Pop();

				auto* rightTop = UI_BUILD_ALLOC(ui::PointAnchoredPlacement)();
				rightTop->SetAnchorAndPivot({ 1, 0 });
				tmpl->placement = rightTop;
				tmpl->measure = false;
				ui::Push<ui::SizeConstraintElement>().SetWidth(240);
				ui::Push<ui::FrameElement>().SetDefaultFrameStyle(ui::DefaultFrameStyle::GroupBox);
				ui::Push<ui::StackTopDownLayoutElement>();
				{
					auto q1 = GetQuat();
					ui::Textf("q1=%g;%g;%g;%g", q1.x, q1.y, q1.z, q1.w);
					auto m1 = GetMtx();
					auto q2 = m1.GetRotationQuaternion();
					ui::Textf("q2=%g;%g;%g;%g", q2.x, q2.y, q2.z, q2.w);
					auto a2 = zyx ? q2.ToEulerAnglesZYX() : q2.ToEulerAnglesXYZ();
					ui::Textf("a2=%g;%g;%g", a2.x, a2.y, a2.z);
					auto q3 = zyx ? ui::Quat::RotateEulerAnglesZYX(a2) : ui::Quat::RotateEulerAnglesXYZ(a2);
					ui::Textf("q3=%g;%g;%g;%g", q3.x, q3.y, q3.z, q3.w);
				}
				ui::Pop();
				ui::Pop();
				ui::Pop();

				ui::Pop();
			}
			ui::Pop();
		}
		ui::Pop();
	}
	void Render3DView(const ui::UIRect& rect)
	{
		using namespace ui::gfx;

		camera.SetWindowRect(rect);
		camera.SetProjectionMatrix(ui::Mat4f::PerspectiveFOVLH(fov, rect.GetAspectRatio(), 0.01f, 1000));

		Clear(16, 15, 14, 255);
		SetProjectionMatrix(camera.GetProjectionMatrix());
		SetViewMatrix(camera.GetViewMatrix());
		VertPC verts[] =
		{
			{ -1, -1, 0, { 100, 150, 200, 255 } },
			{ 1, -1, 0, { 100, 0, 200, 255 } },
			{ -1, 1, 0, { 200, 150, 0, 255 } },
			{ 1, 1, 0, { 150, 50, 0, 255 } },
		};
		uint16_t indices[] = { 0, 1, 2, 1, 3, 2 };
		DrawIndexed(ui::Mat4f::Translate(0, 0, -1), PT_Triangles, VF_Color, verts, 4, indices, 6);
		DrawIndexed(ui::Mat4f::Translate(0, 0, -1) * ui::Mat4f::RotateX(90), PT_Triangles, VF_Color, verts, 4, indices, 6);
		DrawIndexed(ui::Mat4f::Translate(0, 0, -1) * ui::Mat4f::RotateY(-90), PT_Triangles, VF_Color, verts, 4, indices, 6);

		RenderObject(ui::Mat4f::Scale(0.1f) * GetMtx());
	}

	void RenderObject(const ui::Mat4f& mtx)
	{
		using namespace ui::gfx;

		SetRenderState(DF_Cull);

		{
			constexpr ui::prim::BoxSettings S = {};
			constexpr auto vc = S.CalcVertexCount();
			constexpr auto ic = S.CalcIndexCount();
			ui::Vertex_PF3CB4 verts[vc];
			uint16_t idcs[ic];
			ui::prim::GenerateBox(S, verts, idcs);
			ui::prim::SetVertexColor(verts, vc, ui::Color4f(0.1f, 1));
			DrawIndexed(mtx, PT_Triangles, VF_Color, verts, vc, idcs, ic);
		}

		{
			constexpr ui::prim::ConeSettings S = { 32 };
			constexpr auto vc = S.CalcVertexCount();
			constexpr auto ic = S.CalcIndexCount();
			ui::Vertex_PF3CB4 verts[vc];
			uint16_t idcs[ic];
			ui::prim::GenerateCone(S, verts, idcs);
			ui::prim::SetVertexColor(verts, vc, ui::Color4f(0.2f, 0, 0, 1));
			DrawIndexed(ui::Mat4f::Translate(0, 0, 1) * ui::Mat4f::RotateY(-90) * mtx, PT_Triangles, VF_Color, verts, vc, idcs, ic);
			ui::prim::SetVertexColor(verts, vc, ui::Color4f(0, 0.2f, 0, 1));
			DrawIndexed(ui::Mat4f::Translate(0, 0, 1) * ui::Mat4f::RotateX(90) * mtx, PT_Triangles, VF_Color, verts, vc, idcs, ic);
			ui::prim::SetVertexColor(verts, vc, ui::Color4f(0, 0, 0.2f, 1));
			DrawIndexed(ui::Mat4f::Translate(0, 0, 1) * mtx, PT_Triangles, VF_Color, verts, vc, idcs, ic);
		}
	}
};
void Test_Quaternion()
{
	ui::Make<QuaternionTest>();
}
