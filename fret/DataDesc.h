
#pragma once
#include "pch.h"
#include "FileReaders.h"
#include "MathExpr.h"
#include "ImageParsers.h"
#include "Markers.h"
#include "DataDescStruct.h"


extern ui::Color4f colorFloat32;
extern ui::Color4f colorInt16;
extern ui::Color4f colorInt32;
extern ui::Color4f colorNearFileSize32;
extern ui::Color4f colorNearFileSize64;
extern ui::Color4f colorASCII;
extern ui::Color4f colorInst;
extern ui::Color4f colorCurInst;


struct DDFile
{
	uint64_t id = UINT64_MAX;
	std::string name;
	std::string path;
	struct IDataSource* dataSource = nullptr;
	MarkerData markerData;
	MarkerDataSource mdSrc;
};


extern ui::DataCategoryTag DCT_Struct[1];
extern ui::DataCategoryTag DCT_CurStructInst[1];
struct DataDesc
{
	struct Image
	{
		DDFile* file = nullptr;
		int64_t offImage = 0;
		int64_t offPalette = 0;
		uint32_t width = 0;
		uint32_t height = 0;
		std::string format;
		std::string notes;
		bool userCreated = true;
	};

	// data
	std::vector<DDFile*> files;
	std::unordered_map<std::string, DDStruct*> structs;
	std::vector<DDStructInst*> instances;
	std::vector<Image> images;

	// ID allocation
	uint64_t fileIDAlloc = 0;
	int64_t instIDAlloc = 0;

	// ui state
	int editMode = 0;
	DDStructInst* curInst = nullptr;
	uint32_t curImage = 0;
	uint32_t curField = 0;

	void EditStructuralItems();
	void EditInstance();
	void EditStruct();
	void EditField();

	void EditImageItems();

	DDStructInst* AddInstance(const DDStructInst& src);
	void DeleteInstance(DDStructInst* inst);
	void SetCurrentInstance(DDStructInst* inst);
	void _OnDeleteInstance(DDStructInst* inst);
	void ExpandAllInstances(DDFile* filterFile = nullptr);
	void DeleteAllInstances(DDFile* filterFile = nullptr, DDStruct* filterStruct = nullptr);
	DataDesc::Image GetInstanceImage(const DDStructInst& SI);

	~DataDesc();
	void Clear();
	DDFile* CreateNewFile();
	DDFile* FindFileByID(uint64_t id);
	DDStruct* CreateNewStruct(const std::string& name);
	DDStruct* FindStructByName(const std::string& name);
	DDStructInst* FindInstanceByID(int64_t id);

	void DeleteImage(size_t id);
	size_t DuplicateImage(size_t id);

	void Load(const char* key, NamedTextSerializeReader& r);
	void Save(const char* key, NamedTextSerializeWriter& w);
};

struct DataDescInstanceSource : ui::TableDataSource, ui::ISelectionStorage
{
	size_t GetNumRows() override;
	size_t GetNumCols() override;
	std::string GetRowName(size_t row) override;
	std::string GetColName(size_t col) override;
	std::string GetText(size_t row, size_t col) override;

	void ClearSelection() override;
	bool GetSelectionState(uintptr_t item) override;
	void SetSelectionState(uintptr_t item, bool sel) override;

	void Edit();

	void _Refilter();

	std::vector<size_t> _indices;
	bool refilter = true;

	DataDesc* dataDesc = nullptr;

	bool filterStructEnable = true;
	bool filterStructFollow = true;
	DDStruct* filterStruct = nullptr;

	bool filterHideStructsEnable = true;
	std::unordered_set<DDStruct*> filterHideStructs;

	bool filterFileEnable = true;
	bool filterFileFollow = true;
	DDFile* filterFile = nullptr;

	uint32_t showBytes = 0;

	CreationReason filterCreationReason = CreationReason::Query;
};

struct DataDescImageSource : ui::TableDataSource, ui::ISelectionStorage
{
	size_t GetNumRows() override;
	size_t GetNumCols() override;
	std::string GetRowName(size_t row) override;
	std::string GetColName(size_t col) override;
	std::string GetText(size_t row, size_t col) override;

	void ClearSelection() override;
	bool GetSelectionState(uintptr_t item) override;
	void SetSelectionState(uintptr_t item, bool sel) override;

	void Edit();

	void _Refilter();

	std::vector<size_t> _indices;
	bool refilter = true;

	DataDesc* dataDesc = nullptr;
	DDFile* filterFile = nullptr;
	bool filterUserCreated = false;
};

struct CachedImage
{
	ui::draw::ImageHandle GetImage(const DataDesc::Image& imgDesc)
	{
		if (curImg)
		{
			if (imgDesc.file == curImgDesc.file &&
				imgDesc.offImage == curImgDesc.offImage &&
				imgDesc.offPalette == curImgDesc.offPalette &&
				imgDesc.format == curImgDesc.format &&
				imgDesc.width == curImgDesc.width &&
				imgDesc.height == curImgDesc.height)
			{
				return curImg;
			}
		}

		curImg = CreateImageFrom(imgDesc.file->dataSource, imgDesc.format.c_str(), { imgDesc.offImage, imgDesc.offPalette, imgDesc.width, imgDesc.height });
		curImgDesc = imgDesc;
		return curImg;
	}

	ui::draw::ImageHandle curImg;
	DataDesc::Image curImgDesc;
};
