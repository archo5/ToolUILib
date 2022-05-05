
#pragma once
#include "pch.h"

#include "DataDesc.h"


struct MeshEditorWindowNode : ui::Buildable
{
	void OnEnable() override
	{
		GetNativeWindow()->SetTitle("Mesh Resource Editor");
		GetNativeWindow()->SetInnerSize(1200, 800);
	}
	void Build() override;
	void Setup(DataDesc* desc)
	{
		ddiSrc.dataDesc = desc;
	}
	void SetStruct(DDStruct* s)
	{
		ddiSrc.filterStruct = s;
		structDef = s;
		if (!s->resource.mesh)
			s->resource.mesh = new DDRsrcMesh;
		mesh = s->resource.mesh;
		GetNativeWindow()->SetTitle(("Mesh Resource Editor - " + s->name).c_str());
	}

	void ReloadMesh();
	void OnRender3D(ui::UIRect rect);
	void RenderMesh(MSPrimitive& P);

	DDStruct* structDef = nullptr;
	DDRsrcMesh* mesh = nullptr;

	DataDescInstanceSource ddiSrc;
	std::vector<CachedImage> cachedImgs;

	MSData lastMesh;
	ui::OrbitCamera orbitCamera;

	bool alphaBlend = false;
	bool cull = false;
	bool useTexture = true;
	bool drawWireframe = false;
	ui::Color4b wireColor = { 0, 255, 0 };
};
