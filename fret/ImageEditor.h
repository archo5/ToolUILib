
#pragma once
#include "pch.h"

#include "DataDesc.h"


struct ImageEditorWindowNode : ui::Buildable
{
	void OnInit() override
	{
		GetNativeWindow()->SetTitle("Image Resource Editor");
		GetNativeWindow()->SetSize(1200, 800);
	}
	void Build(ui::UIContainer* ctx) override;
	void Setup(DataDesc* desc)
	{
		ddiSrc.dataDesc = desc;
	}
	void SetStruct(DDStruct* s)
	{
		ddiSrc.filterStruct = s;
		structDef = s;
		if (!s->resource.image)
			s->resource.image = new DDRsrcImage;
		image = s->resource.image;
		GetNativeWindow()->SetTitle(("Image Resource Editor - " + s->name).c_str());
	}

	DDStruct* structDef = nullptr;
	DDRsrcImage* image = nullptr;

	DataDescInstanceSource ddiSrc;
	CachedImage cachedImg;
};
