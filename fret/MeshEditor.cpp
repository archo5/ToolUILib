
#include "pch.h"
#include "MeshEditor.h"

#include "../Render/RHI.h"


void MeshEditorWindowNode::Build()
{
	auto& sp1 = ui::Push<ui::SplitPane>();
	{
		auto& sp2 = ui::Push<ui::SplitPane>();
		{
			ui::Push<ui::Panel>();
			if (ddiSrc.dataDesc && ddiSrc.dataDesc->curInst)
			{
				auto& view3d = ui::Push<ui::View3D>();
				view3d.GetStyle().SetBackgroundPainter(ui::CheckerboardPainter::Get());
				view3d.HandleEvent() = [this](ui::Event& e) { orbitCamera.OnEvent(e); };
				view3d.onRender = [this](ui::UIRect r) { OnRender3D(r); };
				{
					ui::Push<ui::EdgeSliceLayoutElement>();

					auto tmpl = ui::EdgeSliceLayoutElement::GetSlotTemplate();
					tmpl->edge = ui::Edge::Left;
					ui::Push<ui::SizeConstraintElement>().SetWidth(200);
					ui::Push<ui::StackTopDownLayoutElement>();
					ui::Push<ui::ListBoxFrame>();
					ui::Push<ui::StackTopDownLayoutElement>();
					{
						ui::imm::PropEditBool("Alpha blend", alphaBlend);
						ui::imm::PropEditBool("Cull", cull);
						ui::imm::PropEditBool("Use texture", useTexture);
						ui::imm::PropEditBool("Draw wireframe", drawWireframe);
						ui::imm::PropEditColor("Wire color", wireColor);
					}
					ui::Pop();
					ui::Pop();
					ui::Pop();
					ui::Pop();

					tmpl->edge = ui::Edge::Right;
					ui::Push<ui::SizeConstraintElement>().SetWidth(200);
					ui::Push<ui::StackTopDownLayoutElement>();
					{
						if (ui::imm::Button("Load mesh", { ui::Enable(ddiSrc.dataDesc && ddiSrc.dataDesc->curInst) }))
						{
							ReloadMesh();
						}
					}
					ui::Pop();
					ui::Pop();

					ui::Pop();
				}
				ui::Pop();
			}
			ui::Pop();

			ui::Push<ui::StackTopDownLayoutElement>();
			if (structDef)
			{
				mesh->script.EditUI();
			}
			ui::Pop();
		}
		ui::Pop();
		sp2.SetDirection(true);
		sp2.SetSplits({ 0.6f });

		ui::Push<ui::FillerElement>();
		if (ddiSrc.dataDesc)
		{
			auto& tv = ui::Make<ui::TableView>();
			tv.SetDataSource(&ddiSrc);
			tv.SetSelectionStorage(&ddiSrc);
			tv.SetSelectionMode(ui::SelectionMode::Single);
			ddiSrc.refilter = true;
			tv.CalculateColumnWidths();
			tv.HandleEvent(ui::EventType::SelectionChange) = [this, &tv](ui::Event& e) { e.current->Rebuild(); };
			tv.HandleEvent(ui::EventType::Click) = [this, &tv](ui::Event& e)
			{
				if (e.numRepeats == 2 && tv.GetHoverRow() != SIZE_MAX && ddiSrc.dataDesc && ddiSrc.dataDesc->curInst)
					ReloadMesh();
			};
		}
		ui::Pop();
	}
	ui::Pop();
	sp1.SetDirection(false);
	sp1.SetSplits({ 0.6f });
}

void MeshEditorWindowNode::ReloadMesh()
{
	VariableSource vs;
	vs.desc = ddiSrc.dataDesc;
	vs.root = ddiSrc.dataDesc->curInst;
	lastMesh = mesh->script.RunScript(ddiSrc.dataDesc->curInst->file->dataSource, &vs);
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

void MeshEditorWindowNode::OnRender3D(ui::UIRect rect)
{
	using namespace ui::rhi;

	ClearDepthOnly();
	SetProjectionMatrix(ui::Mat4f::PerspectiveFOVLH(90, rect.GetWidth() / rect.GetHeight(), 0.01f, 100000));
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
	DrawIndexed(ui::Mat4f::Translate(0, 0, -1), PT_Triangles, VF_Color, verts, 4, indices, 6);
	DrawIndexed(ui::Mat4f::Translate(0, 0, -1) * ui::Mat4f::RotateX(90), PT_Triangles, VF_Color, verts, 4, indices, 6);
	DrawIndexed(ui::Mat4f::Translate(0, 0, -1) * ui::Mat4f::RotateY(-90), PT_Triangles, VF_Color, verts, 4, indices, 6);

	cachedImgs.resize(lastMesh.primitives.size());
	for (auto& P : lastMesh.primitives)
	{
		auto& CI = cachedImgs[&P - lastMesh.primitives.data()];
		Texture2D* tex = nullptr;
		if (useTexture && P.texInstID > 0 && ddiSrc.dataDesc)
		{
			auto* SI = ddiSrc.dataDesc->FindInstanceByID(P.texInstID);
			if (SI && SI->def->resource.type == DDStructResourceType::Image)
			{
				auto ii = ddiSrc.dataDesc->GetInstanceImage(*SI);
				tex = CI.GetImage(ii)->GetInternalExclusive();
			}
		}
		SetTexture(tex);
		unsigned flags = 0;
		if (cull)
			flags |= DF_Cull;
		if (alphaBlend)
			flags |= DF_AlphaBlended;
		SetRenderState(flags);
		RenderMesh(P);
	}
	if (drawWireframe)
	{
		SetTexture(nullptr);
		SetRenderState(DF_Wireframe | DF_ForceColor | DF_AlphaBlended);
		SetForcedColor(wireColor);
		for (auto& P : lastMesh.primitives)
			RenderMesh(P);
	}
}

void MeshEditorWindowNode::RenderMesh(MSPrimitive& P)
{
	using namespace ui::rhi;

	if (P.indices.empty())
	{
		Draw(
			ui::Mat4f::Identity(),
			ConvPrimitiveType(P.type),
			VF_Normal | VF_Texcoord | VF_Color,
			P.convVerts.data(),
			P.convVerts.size());
	}
	else
	{
		DrawIndexed(
			ui::Mat4f::Identity(),
			ConvPrimitiveType(P.type),
			VF_Normal | VF_Texcoord | VF_Color,
			P.convVerts.data(),
			P.convVerts.size(),
			P.indices.data(),
			P.indices.size(),
			true);
	}
}
