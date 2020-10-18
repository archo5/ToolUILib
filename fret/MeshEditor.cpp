
#include "pch.h"
#include "MeshEditor.h"

#include "../Render/OpenGL.h"


void MeshEditorWindowNode::Render(UIContainer* ctx)
{
	auto* sp1 = ctx->Push<ui::SplitPane>();
	{
		auto* sp2 = ctx->Push<ui::SplitPane>();
		{
			ctx->Push<ui::Panel>();
			if (ddiSrc.dataDesc && ddiSrc.dataDesc->curInst)
			{
				auto* view3d = ctx->Push<ui::View3D>();
				*view3d + ui::Width(style::Coord::Percent(100));
				*view3d + ui::Height(style::Coord::Percent(100));
				auto bgr = ui::Theme::current->GetImage(ui::ThemeImage::CheckerboardBackground);
				view3d->GetStyle().SetPaintFunc([bgr](const style::PaintInfo& info)
				{
					auto r = info.rect;

					ui::draw::RectTex(r.x0, r.y0, r.x1, r.y1, bgr->_texture, 0, 0, r.GetWidth() / bgr->GetWidth(), r.GetHeight() / bgr->GetHeight());
				});
				view3d->HandleEvent() = [this](UIEvent& e) { orbitCamera.OnEvent(e); };
				view3d->onRender = [this, view3d]() { OnRender3D(view3d->GetContentRect()); };
				{
					if (ui::imm::Button(ctx, "Load mesh", { ui::Enable(ddiSrc.dataDesc && ddiSrc.dataDesc->curInst) }))
					{
						VariableSource vs;
						vs.desc = ddiSrc.dataDesc;
						vs.root = ddiSrc.dataDesc->curInst;
						lastMesh = mesh->script.RunScript(ddiSrc.dataDesc->curInst->file->dataSource, &vs);
					}
				}
				ctx->Pop();
			}
			ctx->Pop();

			ctx->PushBox();
			if (structDef)
			{
				mesh->script.EditUI(ctx);
			}
			ctx->Pop();
		}
		ctx->Pop();
		sp2->SetDirection(true);
		sp2->SetSplits({ 0.6f });

		ctx->PushBox();
		if (ddiSrc.dataDesc)
		{
			auto* tv = ctx->Make<ui::TableView>();
			*tv + ui::Layout(style::layouts::EdgeSlice()) + ui::Height(style::Coord::Percent(100));
			tv->SetDataSource(&ddiSrc);
			tv->SetSelectionStorage(&ddiSrc);
			tv->SetSelectionMode(ui::SelectionMode::Single);
			ddiSrc.refilter = true;
			tv->CalculateColumnWidths();
			tv->HandleEvent(UIEventType::SelectionChange) = [this, tv](UIEvent& e) { e.current->RerenderNode(); };
		}
		ctx->Pop();
	}
	ctx->Pop();
	sp1->SetDirection(false);
	sp1->SetSplits({ 0.6f });
}

static ui::rhi::PrimitiveType ConvPrimitiveType(MSPrimType type)
{
	using namespace ui::rhi;
	switch (type)
	{
	default:
	case MSPrimType::Points: return PT_Points;
	case MSPrimType::Lines: return PT_Lines;
	case MSPrimType::LineStrip: return PT_LineStrip;
	case MSPrimType::Triangles: return PT_Triangles;
	case MSPrimType::TriangleStrip: return PT_TriangleStrip;
	}
}

void MeshEditorWindowNode::OnRender3D(UIRect rect)
{
	using namespace ui::rhi;

	ClearDepthOnly();
	SetPerspectiveMatrix(Mat4f::PerspectiveFOVLH(90, rect.GetWidth() / rect.GetHeight(), 0.01f, 100000));
	SetViewMatrix(orbitCamera.GetViewMatrix());

	struct VertPC
	{
		float x, y, z;
		ui::Color4b col;
	};
	VertPC verts[] =
	{
		{ -1, -1, 0, { 100, 150, 200, 255 } },
		{ 1, -1, 0, { 100, 0, 200, 255 } },
		{ -1, 1, 0, { 200, 150, 0, 255 } },
		{ 1, 1, 0, { 150, 50, 0, 255 } },
	};
	uint16_t indices[] = { 0, 1, 2, 1, 3, 2 };
	DrawIndexed(Mat4f::Translate(0, 0, -1), PT_Triangles, VF_Color, verts, 4, indices, 6);
	DrawIndexed(Mat4f::Translate(0, 0, -1) * Mat4f::RotateX(90), PT_Triangles, VF_Color, verts, 4, indices, 6);
	DrawIndexed(Mat4f::Translate(0, 0, -1) * Mat4f::RotateY(-90), PT_Triangles, VF_Color, verts, 4, indices, 6);

	for (auto& P : lastMesh.primitives)
	{
		if (P.indices.empty())
		{
			Draw(
				Mat4f::Identity(),
				ConvPrimitiveType(P.type),
				VF_Normal | VF_Texcoord | VF_Color,
				P.convVerts.data(),
				P.convVerts.size());
		}
		else
		{
			DrawIndexed(
				Mat4f::Identity(),
				ConvPrimitiveType(P.type),
				VF_Normal | VF_Texcoord | VF_Color,
				P.convVerts.data(),
				P.convVerts.size(),
				P.indices.data(),
				P.indices.size(),
				true);
		}
	}
}
