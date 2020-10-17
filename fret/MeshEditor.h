
#pragma once
#include "pch.h"

#include "DataDesc.h"


struct MeshEditorWindowNode : ui::Node
{
	void OnInit() override
	{
		GetNativeWindow()->SetTitle("Mesh Resource Editor");
		GetNativeWindow()->SetSize(1200, 800);
	}
	void Render(UIContainer* ctx) override;
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

	void OnRender3D(UIRect rect);

	DDStruct* structDef = nullptr;
	DDRsrcMesh* mesh = nullptr;

	DataDescInstanceSource ddiSrc;
	CachedImage cachedImg;

	MSData lastMesh;
	ui::OrbitCamera orbitCamera;
};
