
#pragma once
#include "pch.h"

#include "DataDesc.h"


struct ImageEditorWindowNode : ui::Buildable
{
	void OnEnable() override
	{
		GetNativeWindow()->SetTitle("Image Resource Editor");
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
