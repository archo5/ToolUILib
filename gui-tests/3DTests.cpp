
#include "pch.h"

#include "../Core/3DMath.h"
#include "../Model/Gizmo.h"
#include "../Render/RHI.h"
#include "../Render/Primitives.h"


struct The3DViewTest : ui::Buildable
{
	struct VertPC
	{
		float x, y, z;
		ui::Color4b col;
	};
	void Build() override
	{
		ui::Push<ui::Panel>()
			+ ui::SetMargin(0)
			+ ui::SetHeight(ui::Coord::Percent(100));
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
			v + ui::SetHeight(ui::Coord::Percent(100));
			{
				auto* leftCorner = ui::BuildAlloc<ui::PointAnchoredPlacement>();
				leftCorner->SetAnchorAndPivot({ 0, 0 });
				ui::PushBox() + ui::SetPlacement(leftCorner);

				ui::Text("Overlay text");
				ui::Make<ui::ColorBlock>().SetColor({ 100, 0, 200, 255 });
				ui::MakeWithText<ui::Button>("Reset")
					+ ui::AddEventHandler(ui::EventType::Activate, [this](ui::Event&)
				{
					bool rh = camera.rightHanded;
					camera = {};
					camera.rightHanded = rh;
				});
				ui::imm::PropEditBool("\bRight-handed", camera.rightHanded);

				ui::Pop();
			}
			ui::Pop();
		}
		ui::Pop();
	}
	void Render3DView(const ui::UIRect& rect)
	{
		using namespace ui::rhi;

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

		SetRenderState(DF_ZTestOff | DF_ZWriteOff);
		auto ray = camera.GetRayWP(mousePos);
		auto rpir = RayPlaneIntersect(ray.origin, ray.direction, { 0, 0, 1, -1 });
		ui::Vec3f isp = ray.GetPoint(rpir.dist);
		DrawIndexed(ui::Mat4f::Scale(0.1f, 0.1f, 0.1f) * ui::Mat4f::Translate(isp), PT_Triangles, VF_Color, verts, 4, indices, 6);
	}
	void DrawPrim(ui::Vertex_PF3CB4* verts, uint16_t vc, uint16_t* idcs, unsigned ic, const ui::Color4b& col, const ui::Mat4f& m)
	{
		using namespace ui::rhi;

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


struct GizmoTest : ui::Buildable
{
	ui::OrbitCamera camera;
	ui::Gizmo gizmo;
	float gizmoSize = 100;
	ui::GizmoSizeMode gizmoSizeMode = ui::GizmoSizeMode::ViewPixels;
	ui::Mat4f xf = ui::Mat4f::Translate(0.01f, 0.02f, 0.03f);
	float fov = 90;

	struct VertPC
	{
		float x, y, z;
		ui::Color4b col;
	};
	void Build() override
	{
		ui::Push<ui::Panel>()
			+ ui::SetMargin(0)
			+ ui::SetHeight(ui::Coord::Percent(100));
		{
			auto& v = ui::Push<ui::View3D>();
			v.SetFlag(ui::UIObject_DB_CaptureMouseOnLeftClick, true);
			v.HandleEvent() = [this](ui::Event& e)
			{
				if (e.type == ui::EventType::ButtonDown)
					e.context->SetKeyboardFocus(e.current);
				if (gizmo.OnEvent(e, camera, ui::GizmoEditableMat4f(xf)))
					Rebuild();
				camera.OnEvent(e);
			};
			v.onRender = [this](ui::UIRect r) { Render3DView(r); };
			v + ui::SetHeight(ui::Coord::Percent(100));
			{
				auto* leftTop = Allocate<ui::PointAnchoredPlacement>();
				leftTop->SetAnchorAndPivot({ 0, 0 });
				ui::Push<ui::Panel>() + ui::SetWidth(120) + ui::SetPlacement(leftTop);
				{
					ui::MakeWithText<ui::Header>("Camera");
					ui::imm::PropEditFloat("FOV", fov, {}, {}, { 1.0f, 179.0f });

					{
						ui::LabeledProperty::Scope ps;
						ui::MakeWithText<ui::Header>("Object");
						if (ui::imm::Button("Reset"))
							xf = ui::Mat4f::Translate(0.01f, 0.02f, 0.03f);
					}

					auto pos = xf.TransformPoint({ 0, 0, 0 });
					ui::Textf("pos=%g;%g;%g", pos.x, pos.y, pos.z) + ui::SetPadding(5);
				}
				ui::Pop();

				auto* rightTop = Allocate<ui::PointAnchoredPlacement>();
				rightTop->SetAnchorAndPivot({ 1, 0 });
				ui::Push<ui::Panel>() + ui::SetWidth(180) + ui::SetPlacement(rightTop);
				{
					ui::MakeWithText<ui::Header>("Gizmo");
					ui::imm::PropEditFloat("Size", gizmoSize, {}, {}, { 0.001f, 200.0f });
					ui::imm::PropDropdownMenuList("Size mode", gizmoSizeMode, Allocate<ui::ZeroSepCStrOptionList>("Scene\0View normalized (Y)\0View pixels\0"));
					{
						ui::LabeledProperty::Scope ps("Type");
						ui::imm::RadioButton(gizmo.type, ui::GizmoType::Move, "M", {}, ui::imm::ButtonStateToggleSkin());
						ui::imm::RadioButton(gizmo.type, ui::GizmoType::Rotate, "R", {}, ui::imm::ButtonStateToggleSkin());
						ui::imm::RadioButton(gizmo.type, ui::GizmoType::Scale, "S", {}, ui::imm::ButtonStateToggleSkin());
					}
					{
						ui::LabeledProperty::Scope ps("Space");
						ui::imm::RadioButton(gizmo.isWorldSpace, false, "Local", {}, ui::imm::ButtonStateToggleSkin());
						ui::imm::RadioButton(gizmo.isWorldSpace, true, "World", {}, ui::imm::ButtonStateToggleSkin());
					}
				}
				ui::Pop();
			}
			ui::Pop();
		}
		ui::Pop();
	}
	void Render3DView(const ui::UIRect& rect)
	{
		using namespace ui::rhi;

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

		gizmo.SetTransform(xf.RemoveScale());
		gizmo.Render(camera, gizmoSize, gizmoSizeMode);
	}

	void RenderObject(const ui::Mat4f& mtx)
	{
		using namespace ui::rhi;

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
	bool useMtx = false;

	ui::Quat GetQuat()
	{
		return ui::Quat::RotateEulerAnglesZYX(angles);
	}
	ui::Mat4f GetMtx()
	{
		if (useMtx)
		{
			return ui::Mat4f::RotateZ(angles.z)
				* ui::Mat4f::RotateY(angles.y)
				* ui::Mat4f::RotateX(angles.x);
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
		ui::Push<ui::Panel>()
			+ ui::SetMargin(0)
			+ ui::SetHeight(ui::Coord::Percent(100));
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
			v + ui::SetHeight(ui::Coord::Percent(100));
			{
				auto* leftTop = Allocate<ui::PointAnchoredPlacement>();
				leftTop->SetAnchorAndPivot({ 0, 0 });
				ui::Push<ui::Panel>() + ui::SetWidth(200) + ui::SetPlacement(leftTop);
				{
					ui::MakeWithText<ui::Header>("Camera");
					ui::imm::PropEditFloat("FOV", fov, {}, {}, { 1.0f, 179.0f });
					ui::imm::PropEditFloatVec("R", &angles.x);
					ui::imm::PropEditBool("Use mtx", useMtx);
				}
				ui::Pop();

				auto* rightTop = Allocate<ui::PointAnchoredPlacement>();
				rightTop->SetAnchorAndPivot({ 1, 0 });
				ui::Push<ui::Panel>() + ui::SetWidth(240) + ui::SetPlacement(rightTop);
				{
					auto q1 = GetQuat();
					ui::Textf("q1=%g;%g;%g;%g", q1.x, q1.y, q1.z, q1.w);
					auto m1 = GetMtx();
					auto q2 = m1.GetRotationQuaternion();
					ui::Textf("q2=%g;%g;%g;%g", q2.x, q2.y, q2.z, q2.w);
					auto a2 = q2.ToEulerAnglesZYX();
					ui::Textf("a2=%g;%g;%g", a2.x, a2.y, a2.z);
					auto q3 = ui::Quat::RotateEulerAnglesZYX(a2);
					ui::Textf("q3=%g;%g;%g;%g", q3.x, q3.y, q3.z, q3.w);
				}
				ui::Pop();
			}
			ui::Pop();
		}
		ui::Pop();
	}
	void Render3DView(const ui::UIRect& rect)
	{
		using namespace ui::rhi;

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
		using namespace ui::rhi;

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
