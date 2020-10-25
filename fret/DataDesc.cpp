
#include "pch.h"
#include "DataDesc.h"
#include "FileReaders.h"
#include "ImageParsers.h"


ui::DataCategoryTag DCT_Struct[1];
ui::DataCategoryTag DCT_CurStructInst[1];


bool EditImageFormat(UIContainer* ctx, const char* label, std::string& format)
{
	if (ui::imm::PropButton(ctx, label, format.c_str()))
	{
		std::vector<ui::MenuItem> imgFormats;
		StringView prevCat;
		for (size_t i = 0, count = GetImageFormatCount(); i < count; i++)
		{
			StringView cat = GetImageFormatCategory(i);
			if (cat != prevCat)
			{
				prevCat = cat;
				imgFormats.push_back(ui::MenuItem(cat, {}, true));
			}
			StringView name = GetImageFormatName(i);
			imgFormats.push_back(ui::MenuItem(name).Func([&format, name]() { format.assign(name.data(), name.size()); }));
		}
		ui::Menu(imgFormats).Show(ctx->GetCurrentNode());
		return true;
	}
	return false;
}


static bool advancedAccess = false;
static MathExprObj testQuery;
void DataDesc::EditStructuralItems(UIContainer* ctx)
{
	ctx->Push<ui::Panel>();

	ui::imm::PropEditBool(ctx, "Advanced access", advancedAccess);
	if (advancedAccess)
	{
		ui::imm::PropEditString(ctx, "\bTQ:", testQuery.expr.c_str(), [](const char* v) { testQuery.SetExpr(v); });
		if (testQuery.inst)
		{
			char bfr[128];
			VariableSource vs;
			{
				vs.desc = this;
				vs.root = curInst;
			}
			snprintf(bfr, 128, "Value: %" PRId64, testQuery.inst->Evaluate(&vs));
			ctx->Text(bfr);
		}

		auto id = curInst ? curInst->id : -1LL;
		ui::imm::PropEditInt(ctx, "Current instance ID", id, {}, 1);
		if (!curInst || curInst->id != id)
			SetCurrentInstance(FindInstanceByID(id));

		if (curInst)
		{
			ctx->PushBox() + ui::Layout(style::layouts::StackExpand()) + ui::StackingDirection(style::StackingDirection::LeftToRight);
			if (ui::imm::Button(ctx, "Drop cache"))
			{
				curInst->cachedFields.clear();
				curInst->cachedReadOff = curInst->off;
				curInst->cachedSize = F_NO_VALUE;
			}
			if (ui::imm::Button(ctx, "Edit inst"))
			{
				curInst->OnEdit();
			}
			if (ui::imm::Button(ctx, "Edit struct"))
			{
				if (curInst->def)
					curInst->def->OnEdit();
			}
			ctx->Pop();
		}
	}

	ctx->PushBox() + ui::StackingDirection(style::StackingDirection::LeftToRight);
	ctx->Text("Edit:") + ui::Padding(5);
	ui::imm::RadioButton(ctx, editMode, 0, "instance", {}, ui::imm::ButtonStateToggleSkin());
	ui::imm::RadioButton(ctx, editMode, 1, "struct", {}, ui::imm::ButtonStateToggleSkin());
	ui::imm::RadioButton(ctx, editMode, 2, "field", {}, ui::imm::ButtonStateToggleSkin());
	ctx->Pop();

	if (editMode == 0)
	{
		EditInstance(ctx);
	}
	if (editMode == 1)
	{
		EditStruct(ctx);
	}
	if (editMode == 2)
	{
		EditField(ctx);
	}

	ctx->Pop();
}

static const char* CreationReasonToString(CreationReason cr)
{
	switch (cr)
	{
	case CreationReason::UserDefined: return "user-defined";
	case CreationReason::ManualExpand: return "manual expand";
	case CreationReason::AutoExpand: return "auto expand";
	case CreationReason::Query: return "query";
	default: return "unknown";
	}
}

static const char* CreationReasonToStringShort(CreationReason cr)
{
	switch (cr)
	{
	case CreationReason::UserDefined: return "U";
	case CreationReason::ManualExpand: return "ME";
	case CreationReason::AutoExpand: return "AE";
	case CreationReason::Query: return "Q";
	default: return "?";
	}
}

static bool EditCreationReason(UIContainer* ctx, const char* label, CreationReason& cr)
{
	return ui::imm::PropDropdownMenuList(ctx, label, cr,
		ctx->GetCurrentNode()->Allocate<ui::ZeroSepCStrOptionList>(
			"user-defined\0manual expand\0auto expand\0query\0"));
}

void DataDesc::EditInstance(UIContainer* ctx)
{
	if (auto* SI = curInst)
	{
		bool del = false;

		if (ui::imm::Button(ctx, "Delete"))
		{
			del = true;
		}
		ui::imm::PropEditString(ctx, "Notes", SI->notes.c_str(), [&SI](const char* s) { SI->notes = s; });
		ui::imm::PropEditBool(ctx, "Allow auto expand", SI->allowAutoExpand);
		ui::imm::PropEditInt(ctx, "Offset", SI->off);
		if (ui::imm::PropButton(ctx, "Edit struct:", SI->def->name.c_str()))
		{
			editMode = 1;
			ctx->GetCurrentNode()->Rerender();
		}

		if (advancedAccess)
		{
			EditCreationReason(ctx, "Creation reason", SI->creationReason);
			ui::imm::PropEditBool(ctx, "Use remaining size", SI->remainingCountIsSize);
			ui::imm::PropEditInt(ctx, SI->remainingCountIsSize ? "Remaining size" : "Remaining count", SI->remainingCount);

			ui::Property::Begin(ctx);
			auto& lbl = ui::Property::Label(ctx, "Size override");
			ui::imm::EditBool(ctx, SI->sizeOverrideEnable, nullptr);
			ui::imm::EditInt(ctx, &lbl, SI->sizeOverrideValue);
			ui::Property::End(ctx);
		}

		ctx->Text("Arguments") + ui::Padding(5);
		ctx->Push<ui::Panel>();

		auto* argSeq = ctx->GetCurrentNode()->Allocate<ui::StdSequence<decltype(SI->args)>>(SI->args);
		auto* argEditor = ctx->Make<ui::SequenceEditor>();
		argEditor->SetSequence(argSeq);
		argEditor->itemUICallback = [this](UIContainer* ctx, ui::SequenceEditor* ed, size_t idx, void* ptr)
		{
			auto& A = *static_cast<DDArg*>(ptr);

			ui::imm::PropEditString(ctx, "\bName", A.name.c_str(), [&A](const char* v) { A.name = v; });
			ui::imm::PropEditInt(ctx, "\bValue", A.intVal);
		};

		if (ui::imm::Button(ctx, "Add"))
		{
			SI->args.push_back({ "unnamed", 0 });
			ctx->GetCurrentNode()->Rerender();
		}
		ctx->Pop();

		if (SI->def)
		{
			auto& S = *SI->def;
			struct Data : ui::TableDataSource
			{
				enum Columns
				{
					COL_Name,
					COL_Type,
					COL_Offset,
					COL_Preview,

					COL__COUNT,
				};

				size_t GetNumRows() override { return SI->def->GetFieldCount(); }
				size_t GetNumCols() override { return COL__COUNT; }
				std::string GetRowName(size_t row) override { return std::to_string(row + 1); }
				std::string GetColName(size_t col) override
				{
					if (col == COL_Name) return "Name";
					if (col == COL_Type) return "Type";
					if (col == COL_Offset) return "Offset";
					if (col == COL_Preview) return "Preview";
					return "";
				}
				std::string GetText(size_t row, size_t col) override
				{
					switch (col)
					{
					case COL_Name: return SI->def->fields[row].name;
					case COL_Type: return SI->def->fields[row].type + "[" + std::to_string(SI->def->fields[row].count) + "]";
					case COL_Offset: return std::to_string(SI->GetFieldOffset(row, true));
					case COL_Preview: return SI->GetFieldPreview(row, true);
					default: return "";
					}
				}

				DDStructInst* SI;
			};
			auto* data = ctx->GetCurrentNode()->Allocate<Data>();
			data->SI = SI;

			char bfr[256];
			auto size = SI->GetSize(true);
			if (SI->CanCreateNextInstance(true, true) == OptionalBool::True)
			{
				int64_t remSizeSub = SI->remainingCountIsSize ? (SI->sizeOverrideEnable ? SI->sizeOverrideValue : size) : 1;

				snprintf(bfr, 256, "Next instance (%s)", SI->GetNextInstanceInfo().c_str());
				if (SI->remainingCount - remSizeSub && ui::imm::Button(ctx, bfr, { ui::Enable(SI->remainingCount - remSizeSub > 0) }))
				{
					SetCurrentInstance(SI->CreateNextInstance(CreationReason::ManualExpand));
				}
			}

			ctx->PushBox() + ui::StackingDirection(style::StackingDirection::LeftToRight);
			{
				if (size != F_NO_VALUE)
				{
					snprintf(bfr, 256, "Data (size=%" PRId64 ")", size);
					ctx->Text(bfr) + ui::Padding(5);
				}
				else
				{
					ctx->Text("Data (size=") + ui::Padding(5);
					{
						if (ui::imm::Button(ctx, "?"))
							SI->GetSize();
					}
					ctx->Text(")") + ui::Padding(5);
				}
			}
			ctx->Pop();

			bool incomplete = size == F_NO_VALUE;
			for (size_t i = 0; i < S.fields.size(); i++)
			{
				auto desc = SI->GetFieldDescLazy(i, &incomplete);
				ctx->PushBox() + ui::Layout(style::layouts::StackExpand()) + ui::StackingDirection(style::StackingDirection::LeftToRight);
				ctx->Text(desc) + ui::Padding(5);
				
				if (FindStructByName(S.fields[i].type) && SI->IsFieldPresent(i, true) == OptionalBool::True)
				{
					if (ui::imm::Button(ctx, "View"))
					{
						// TODO indices
						if (auto* inst = SI->CreateFieldInstances(i, 0, CreationReason::ManualExpand))
							SetCurrentInstance(inst);
					}
				}
				ctx->Pop();
			}
			auto* tv = ctx->Make<ui::TableView>();
			*tv + ui::Height(200);
			tv->enableRowHeader = false;
			tv->SetDataSource(data);
			tv->CalculateColumnWidths();

			if (incomplete && ui::imm::Button(ctx, "Load completely"))
			{
				for (size_t i = 0; i < S.fields.size(); i++)
				{
					SI->GetFieldElementCount(i);
					SI->GetFieldOffset(i);
					SI->GetFieldPreview(i);
				}
				SI->GetSize();
			}

			if (S.resource.type == DDStructResourceType::Image)
			{
				if (ui::imm::Button(ctx, "Open image in tab", { ui::Enable(!!S.resource.image) }))
				{
					images.push_back(GetInstanceImage(*SI));

					curImage = images.size() - 1;
					ctx->GetCurrentNode()->SendUserEvent(GlobalEvent_OpenImage, images.size() - 1);
				}
				if (ui::imm::Button(ctx, "Open image in editor", { ui::Enable(!!S.resource.image) }))
				{
					ctx->GetCurrentNode()->SendUserEvent(GlobalEvent_OpenImageRsrcEditor, uintptr_t(SI->def));
				}
			}
		}

		if (del)
		{
			DeleteInstance(curInst);
		}
	}
}

struct RenameDialog : ui::NativeDialogWindow
{
	RenameDialog(StringView name)
	{
		newName.assign(name.data(), name.size());
		SetTitle(("Rename struct: " + newName).c_str());
		SetSize(400, 16 * 3 + 24 * 2);
		SetStyle(ui::WindowStyle::WS_TitleBar);
	}
	void OnClose() override
	{
		rename = false;
		SetVisible(false);
	}
	void OnRender(UIContainer* ctx) override
	{
		ctx->PushBox() + ui::Layout(style::layouts::EdgeSlice()) + ui::Padding(16);
		ui::imm::PropEditString(ctx, "New name:", newName.c_str(), [this](const char* s) { newName = s; });

		*ctx->Make<ui::BoxElement>() + ui::Height(16);

		ui::Property::Begin(ctx);
		if (ui::imm::Button(ctx, "OK", { ui::Height(30) }))
		{
			rename = true;
			SetVisible(false);
		}
		*ctx->Make<ui::BoxElement>() + ui::Width(16);
		if (ui::imm::Button(ctx, "Cancel", { ui::Height(30) }))
		{
			rename = false;
			SetVisible(false);
		}
		ui::Property::End(ctx);
		ctx->Pop();
	}

	bool rename = false;
	std::string newName;
};

void DataDesc::EditStruct(UIContainer* ctx)
{
	if (auto* SI = curInst)
	{
		ctx->PushBox() + ui::StackingDirection(style::StackingDirection::LeftToRight);
		ctx->Text("Struct:") + ui::Padding(5);
		ctx->Text(SI->def->name) + ui::Padding(5);
		if (ui::imm::Button(ctx, "Rename"))
		{
			RenameDialog rd(SI->def->name);
			for (;;)
			{
				rd.Show();
				if (rd.rename && rd.newName != SI->def->name)
				{
					if (rd.newName.empty())
					{
						// TODO msgbox/rt
						puts("Name empty!");
						continue;
					}
					if (structs.find(rd.newName) != structs.end())
					{
						// TODO msgbox/rt
						puts("Name already used!");
						continue;
					}
					structs[rd.newName] = SI->def;
					structs.erase(SI->def->name);
					SI->def->name = rd.newName;
				}
				break;
			}
		}
		ctx->Pop();

		auto it = structs.find(SI->def->name);
		if (it != structs.end())
		{
			auto& S = *it->second;
			ui::imm::PropEditBool(ctx, "Is serialized?", S.serialized);

			ui::imm::PropEditInt(ctx, "Size", S.size);
			ui::imm::PropEditString(ctx, "Size source", S.sizeSrc.c_str(), [&S](const char* v) { S.sizeSrc = v; });

			ctx->Text("Parameters") + ui::Padding(5);
			ctx->Push<ui::Panel>();

			auto* paramSeq = ctx->GetCurrentNode()->Allocate<ui::StdSequence<decltype(S.params)>>(S.params);
			auto* paramEditor = ctx->Make<ui::SequenceEditor>();
			paramEditor->SetSequence(paramSeq);
			paramEditor->itemUICallback = [this](UIContainer* ctx, ui::SequenceEditor* ed, size_t idx, void* ptr)
			{
				auto& P = *static_cast<DDParam*>(ptr);

				ui::imm::PropEditString(ctx, "\bName", P.name.c_str(), [&P](const char* v) { P.name = v; });
				ui::imm::PropEditInt(ctx, "\bValue", P.intVal);
			};

			if (ui::imm::Button(ctx, "Add"))
			{
				S.params.push_back({ "unnamed", 0 });
				ctx->GetCurrentNode()->Rerender();
			}
			ctx->Pop();

			ctx->Text("Fields") + ui::Padding(5);
			ctx->Push<ui::Panel>();

			auto* fieldSeq = ctx->GetCurrentNode()->Allocate<ui::StdSequence<decltype(S.fields)>>(S.fields);
			auto* fieldEditor = ctx->Make<ui::SequenceEditor>();
			fieldEditor->SetSequence(fieldSeq);
			auto* N = ctx->GetCurrentNode(); // TODO remove workaround
			fieldEditor->itemUICallback = [this, &S, N](UIContainer* ctx, ui::SequenceEditor* ed, size_t idx, void* ptr)
			{
				auto& F = *static_cast<DDField*>(ptr);

				char info[128];
				int cc = snprintf(info, 128, "%s: %s[%s%s%s%" PRId64 "]",
					F.name.c_str(),
					F.type.c_str(),
					F.countIsMaxSize ? "up to " : "",
					F.countSrc.c_str(),
					F.count >= 0 && F.countSrc.size() ? "+" : "",
					F.count);
				if (!S.serialized)
					snprintf(info + cc, 128 - cc, " @%" PRId64, F.off);
				*ctx->MakeWithText<ui::BoxElement>(info) + ui::Padding(5);

				if (ui::imm::Button(ctx, "Edit", { ui::Width(50) }))
				{
					editMode = 2;
					curField = idx;
					N->Rerender();
				}
			};

			if (ui::imm::Button(ctx, "Add"))
			{
				S.fields.push_back({ "i32", "unnamed" });
				editMode = 2;
				curField = S.fields.size() - 1;
				ctx->GetCurrentNode()->Rerender();
			}
			ctx->Pop();

			ctx->Text("Resource") + ui::Padding(5);
			ui::Property::Begin(ctx);
			ctx->Text("Type:") + ui::Padding(5);
			ui::imm::RadioButton(ctx, S.resource.type, DDStructResourceType::None, "None", {}, ui::imm::ButtonStateToggleSkin());
			ui::imm::RadioButton(ctx, S.resource.type, DDStructResourceType::Image, "Image", {}, ui::imm::ButtonStateToggleSkin());
			ui::imm::RadioButton(ctx, S.resource.type, DDStructResourceType::Mesh, "Mesh", {}, ui::imm::ButtonStateToggleSkin());
			ui::Property::End(ctx);

			if (S.resource.type == DDStructResourceType::Image)
			{
				if (ui::imm::Button(ctx, "Edit image"))
				{
					if (!S.resource.image)
						S.resource.image = new DDRsrcImage;
					ctx->GetCurrentNode()->SendUserEvent(GlobalEvent_OpenImageRsrcEditor, uintptr_t(SI->def));
				}
			}
			if (S.resource.type == DDStructResourceType::Mesh)
			{
				if (ui::imm::Button(ctx, "Edit mesh"))
				{
					if (!S.resource.mesh)
						S.resource.mesh = new DDRsrcMesh;
					ctx->GetCurrentNode()->SendUserEvent(GlobalEvent_OpenMeshRsrcEditor, uintptr_t(SI->def));
				}
			}
		}
		else
		{
			ctx->Text("-- NOT FOUND --") + ui::Padding(10);
		}
	}
}

void DataDesc::EditField(UIContainer* ctx)
{
	if (curInst)
	{
		auto it = structs.find(curInst->def->name);
		if (it != structs.end())
		{
			auto& S = *it->second;
			if (curField < S.fields.size())
			{
				auto& F = S.fields[curField];
				ui::imm::PropEditString(ctx, "Value expr.", F.valueExpr.expr.c_str(), [&F](const char* v) { F.valueExpr.SetExpr(v); });
				if (!S.serialized)
					ui::imm::PropEditInt(ctx, "Offset", F.off);
				ui::imm::PropEditString(ctx, "Off.expr.", F.offExpr.expr.c_str(), [&F](const char* v) { F.offExpr.SetExpr(v); });
				ui::imm::PropEditString(ctx, "Name", F.name.c_str(), [&F](const char* s) { F.name = s; });
				ui::imm::PropEditString(ctx, "Type", F.type.c_str(), [&F](const char* s) { F.type = s; });
				ui::imm::PropEditInt(ctx, "Count", F.count, {}, 1);
				ui::imm::PropEditString(ctx, "Count source", F.countSrc.c_str(), [&F](const char* s) { F.countSrc = s; });
				ui::imm::PropEditBool(ctx, "Count is max. size", F.countIsMaxSize);
				ui::imm::PropEditBool(ctx, "Individual computed offsets", F.individualComputedOffsets);
				ui::imm::PropEditBool(ctx, "Read until 0", F.readUntil0);

				ctx->Text("Struct arguments") + ui::Padding(5);
				ctx->Push<ui::Panel>();
				for (size_t i = 0; i < F.structArgs.size(); i++)
				{
					auto& SA = F.structArgs[i];
					ctx->PushBox() + ui::Layout(style::layouts::StackExpand()) + ui::StackingDirection(style::StackingDirection::LeftToRight);
					ui::imm::PropEditString(ctx, "\bName", SA.name.c_str(), [&SA](const char* v) { SA.name = v; });
					ui::imm::PropEditString(ctx, "\bSource", SA.src.c_str(), [&SA](const char* v) { SA.src = v; });
					ui::imm::PropEditInt(ctx, "\bOffset", SA.intVal);
					if (ui::imm::Button(ctx, "X", { ui::Width(20) }))
					{
						F.structArgs.erase(F.structArgs.begin() + i);
					}
					ctx->Pop();
				}
				if (ui::imm::Button(ctx, "Add"))
				{
					F.structArgs.push_back({ "unnamed", "", 0 });
				}
				ctx->Pop();

				ui::imm::PropEditString(ctx, "Condition", F.condition.expr.c_str(), [&F](const char* v) { F.condition.SetExpr(v); });
				ui::imm::PropEditString(ctx, "Elem.cond.",
					F.elementCondition.expr.c_str(),
					[&F](const char* v) { F.elementCondition.SetExpr(v); },
					{ ui::Enable(F.individualComputedOffsets) });
			}
		}
	}
}

void DataDesc::EditImageItems(UIContainer* ctx)
{
	if (curImage < images.size())
	{
		ctx->Push<ui::Panel>();

		auto& I = images[curImage];

		bool chg = false;
		chg |= ui::imm::PropEditString(ctx, "Notes", I.notes.c_str(), [&I](const char* s) { I.notes = s; });
		chg |= EditImageFormat(ctx, "Format", I.format);
		chg |= ui::imm::PropEditInt(ctx, "Image offset", I.offImage);
		chg |= ui::imm::PropEditInt(ctx, "Palette offset", I.offPalette);
		chg |= ui::imm::PropEditInt(ctx, "Width", I.width);
		chg |= ui::imm::PropEditInt(ctx, "Height", I.height);
		if (chg)
			ctx->GetCurrentNode()->Rerender();

		ctx->Pop();
	}
}


DDStructInst* DataDesc::AddInstance(const DDStructInst& src)
{
	printf("trying to create %s @ %" PRId64 "\n", src.def->name.c_str(), src.off);
	for (size_t i = 0, num = instances.size(); i < num; i++)
	{
		auto* I = instances[i];
		if (I->off == src.off && I->def == src.def && I->file == src.file)
		{
			I->creationReason = min(I->creationReason, src.creationReason);
			I->remainingCount = src.remainingCount;
			I->remainingCountIsSize = src.remainingCountIsSize;
			return I;
		}
	}
	auto* copy = new DDStructInst(src);
	copy->id = instIDAlloc++;
	copy->OnEdit();
	instances.push_back(copy);
	return copy;
}

void DataDesc::DeleteInstance(DDStructInst* inst)
{
	delete inst;
	_OnDeleteInstance(inst);
	instances.erase(std::remove_if(instances.begin(), instances.end(), [inst](DDStructInst* SI) { return inst == SI; }), instances.end());
}

void DataDesc::SetCurrentInstance(DDStructInst* inst)
{
	curInst = inst;
	ui::Notify(DCT_CurStructInst, nullptr);
}

void DataDesc::_OnDeleteInstance(DDStructInst* inst)
{
	if (curInst == inst)
		SetCurrentInstance(nullptr);
}

void DataDesc::ExpandAllInstances(DDFile* filterFile)
{
	for (size_t i = 0; i < instances.size(); i++)
	{
		if (filterFile && instances[i]->file != filterFile)
			continue;
		auto* SI = instances[i];
		if (!SI->allowAutoExpand)
			continue;
		auto& S = *SI->def;

		SI->CreateNextInstance(CreationReason::AutoExpand);

		for (size_t j = 0, jn = S.fields.size(); j < jn; j++)
			if (SI->IsFieldPresent(j) && structs.count(S.fields[j].type))
				SI->CreateFieldInstances(j, SIZE_MAX, CreationReason::AutoExpand);
	}
}

void DataDesc::DeleteAllInstances(DDFile* filterFile, DDStruct* filterStruct)
{
	instances.erase(std::remove_if(instances.begin(), instances.end(), [this, filterFile, filterStruct](DDStructInst* SI)
	{
		if (SI->creationReason <= CreationReason::ManualExpand)
			return false;
		if (filterFile && SI->file != filterFile)
			return false;
		if (filterStruct && SI->def != filterStruct)
			return false;
		_OnDeleteInstance(SI);
		delete SI;
		return true;
	}), instances.end());
}

DataDesc::Image DataDesc::GetInstanceImage(const DDStructInst& SI)
{
	VariableSource vs;
	{
		vs.desc = this;
		vs.root = &SI;
	}
	auto& II = *SI.def->resource.image;
	Image img;
	img.file = SI.file;
	img.offImage = II.imgOff.Evaluate(vs);
	img.offPalette = II.palOff.Evaluate(vs);
	img.width = II.width.Evaluate(vs);
	img.height = II.height.Evaluate(vs);
	img.format = II.format;

	for (auto& FO : II.formatOverrides)
	{
		if (FO.condition.Evaluate(vs))
		{
			img.format = FO.format;
			break;
		}
	}
	img.userCreated = false;
	return img;
}

DataDesc::~DataDesc()
{
	Clear();
}

void DataDesc::Clear()
{
	for (auto* F : files)
		delete F;
	files.clear();
	fileIDAlloc = 0;

	for (auto& sp : structs)
		delete sp.second;
	structs.clear();

	instances.clear();

	images.clear();
}

DDFile* DataDesc::CreateNewFile()
{
	auto* F = new DDFile;
	F->id = fileIDAlloc++;
	F->mdSrc.data = &F->markerData;
	F->mdSrc.dataSource = F->dataSource;
	files.push_back(F);
	return F;
}

DDFile* DataDesc::FindFileByID(uint64_t id)
{
	for (auto* F : files)
		if (F->id == id)
			return F;
	return nullptr;
}

DDStruct* DataDesc::CreateNewStruct(const std::string& name)
{
	assert(structs.count(name) == 0);
	auto* S = new DDStruct;
	S->name = name;
	structs[name] = S;
	return S;
}

DDStruct* DataDesc::FindStructByName(const std::string& name)
{
	auto it = structs.find(name);
	if (it != structs.end())
		return it->second;
	return nullptr;
}

DDStructInst* DataDesc::FindInstanceByID(int64_t id)
{
	for (auto* SI : instances)
		if (SI->id == id)
			return SI;
	return nullptr;
}

void DataDesc::DeleteImage(size_t id)
{
	images.erase(images.begin() + id);
	if (curImage == id)
		curImage = 0;
}

size_t DataDesc::DuplicateImage(size_t id)
{
	images.push_back(images[id]);
	return images.size() - 1;
}

void DataDesc::Load(const char* key, NamedTextSerializeReader& r)
{
	r.BeginDict(key);

	r.BeginArray("files");
	for (auto E : r.GetCurrentRange())
	{
		r.BeginEntry(E);
		r.BeginDict("");

		auto* F = CreateNewFile();
		F->id = r.ReadUInt64("id");
		F->name = r.ReadString("name");
		F->path = r.ReadString("path");
		F->markerData.Load("markerData", r);

		r.EndDict();
		r.EndEntry();
	}
	r.EndArray();

	fileIDAlloc = r.ReadUInt64("fileIDAlloc");
	instIDAlloc = r.ReadUInt64("instIDAlloc");

	r.BeginArray("structs");
	for (auto E : r.GetCurrentRange())
	{
		r.BeginEntry(E);
		r.BeginDict("");

		auto name = r.ReadString("name");
		auto* S = CreateNewStruct(name);
		S->Load(r);

		r.EndDict();
		r.EndEntry();
	}
	r.EndArray();

	r.BeginArray("instances");
	for (auto E : r.GetCurrentRange())
	{
		r.BeginEntry(E);
		r.BeginDict("");

		auto* SI = new DDStructInst;
		auto id = r.ReadUInt64("id", UINT64_MAX);
		SI->id = id == UINT64_MAX ? instIDAlloc++ : id;
		SI->desc = this;
		SI->def = FindStructByName(r.ReadString("struct"));
		SI->file = FindFileByID(r.ReadUInt64("file"));
		SI->off = r.ReadInt64("off");
		SI->notes = r.ReadString("notes");
		SI->creationReason = (CreationReason)r.ReadInt("creationReason",
			int(r.ReadBool("userCreated") ? CreationReason::UserDefined : CreationReason::AutoExpand));
		SI->allowAutoExpand = r.ReadBool("allowAutoExpand", true);
		SI->remainingCountIsSize = r.ReadBool("remainingCountIsSize");
		SI->remainingCount = r.ReadInt64("remainingCount");
		SI->sizeOverrideEnable = r.ReadBool("sizeOverrideEnable");
		SI->sizeOverrideValue = r.ReadInt64("sizeOverrideValue");

		r.BeginArray("args");
		for (auto E2 : r.GetCurrentRange())
		{
			r.BeginEntry(E2);
			r.BeginDict("");

			DDArg A;
			A.name = r.ReadString("name");
			A.intVal = r.ReadInt64("intVal");
			SI->args.push_back(A);

			r.EndDict();
			r.EndEntry();
		}
		r.EndArray();
		instances.push_back(SI);

		r.EndDict();
		r.EndEntry();
	}
	r.EndArray();

	r.BeginArray("images");
	for (auto E : r.GetCurrentRange())
	{
		r.BeginEntry(E);
		r.BeginDict("");

		Image I;
		I.file = FindFileByID(r.ReadUInt64("file"));
		I.offImage = r.ReadInt64("offImage");
		I.offPalette = r.ReadInt64("offPalette");
		I.width = r.ReadUInt("width");
		I.height = r.ReadUInt("height");
		I.format = r.ReadString("format");
		I.notes = r.ReadString("notes");
		I.userCreated = r.ReadBool("userCreated");
		images.push_back(I);

		r.EndDict();
		r.EndEntry();
	}
	r.EndArray();

	editMode = r.ReadInt("editMode");
	auto curInstID = r.ReadInt64("curInst", -1);
	SetCurrentInstance(curInstID == -1 ? nullptr : FindInstanceByID(curInstID));
	curImage = r.ReadUInt("curImage");
	curField = r.ReadUInt("curField");

	r.EndDict();
}

void DataDesc::Save(const char* key, NamedTextSerializeWriter& w)
{
	w.BeginDict(key);

	w.BeginArray("files");
	for (auto* F : files)
	{
		w.BeginDict("");

		w.WriteInt("id", F->id);
		w.WriteString("name", F->name);
		w.WriteString("path", F->path);
		F->markerData.Save("markerData", w);

		w.EndDict();
	}
	w.EndArray();

	w.WriteInt("fileIDAlloc", fileIDAlloc);
	w.WriteInt("instIDAlloc", instIDAlloc);

	w.BeginArray("structs");
	std::vector<std::string> structNames;
	for (const auto& sp : structs)
		structNames.push_back(sp.first);
	std::sort(structNames.begin(), structNames.end());
	for (const auto& sname : structNames)
	{
		DDStruct* S = structs.find(sname)->second;

		w.BeginDict("");

		S->Save(w);

		w.EndDict();
	}
	w.EndArray();

	w.BeginArray("instances");
	for (const DDStructInst* SI : instances)
	{
		if (SI->creationReason >= CreationReason::AutoExpand)
			continue;

		w.BeginDict("");

		w.WriteInt("id", SI->id);
		w.WriteString("struct", SI->def->name);
		w.WriteInt("file", SI->file->id);
		w.WriteInt("off", SI->off);
		w.WriteString("notes", SI->notes);
		w.WriteInt("creationReason", int(SI->creationReason));
		w.WriteBool("allowAutoExpand", SI->allowAutoExpand);
		w.WriteBool("remainingCountIsSize", SI->remainingCountIsSize);
		w.WriteInt("remainingCount", SI->remainingCount);
		w.WriteBool("sizeOverrideEnable", SI->sizeOverrideEnable);
		w.WriteInt("sizeOverrideValue", SI->sizeOverrideValue);

		w.BeginArray("args");
		for (const DDArg& A : SI->args)
		{
			w.BeginDict("");
			w.WriteString("name", A.name);
			w.WriteInt("intVal", A.intVal);
			w.EndDict();
		}
		w.EndArray();

		w.EndDict();
	}
	w.EndArray();

	w.BeginArray("images");
	for (const Image& I : images)
	{
		w.BeginDict("");

		w.WriteInt("file", I.file->id);
		w.WriteInt("offImage", I.offImage);
		w.WriteInt("offPalette", I.offPalette);
		w.WriteInt("width", I.width);
		w.WriteInt("height", I.height);
		w.WriteString("format", I.format);
		w.WriteString("notes", I.notes);
		w.WriteBool("userCreated", I.userCreated);

		w.EndDict();
	}
	w.EndArray();

	w.WriteInt("editMode", editMode);
	w.WriteInt("curInst", curInst ? curInst->id : -1LL);
	w.WriteInt("curImage", curImage);
	w.WriteInt("curField", curField);

	w.EndDict();
}


enum COLS_DDI
{
	DDI_COL_ID,
	DDI_COL_IID,
	DDI_COL_CR,
	DDI_COL_File,
	DDI_COL_Offset,
	DDI_COL_Struct,
	DDI_COL_Bytes,

	DDI_COL_FirstField,
	DDI_COL_HEADER_SIZE = DDI_COL_FirstField,
};

size_t DataDescInstanceSource::GetNumRows()
{
	_Refilter();
	return _indices.size();
}

size_t DataDescInstanceSource::GetNumCols()
{
	size_t ncols = filterStructEnable && filterStruct ? DDI_COL_HEADER_SIZE + filterStruct->fields.size() : DDI_COL_HEADER_SIZE;
	if (filterFileEnable && filterFile)
		ncols--;
	if (filterStructEnable && filterStruct)
		ncols--;
	if (showBytes == 0)
		ncols--;
	return ncols;
}

std::string DataDescInstanceSource::GetRowName(size_t row)
{
	return std::to_string(row);
}

std::string DataDescInstanceSource::GetColName(size_t col)
{
	if (filterFileEnable && filterFile && col >= DDI_COL_File)
		col++;
	if (filterStructEnable && filterStruct && col >= DDI_COL_Struct)
		col++;
	if (showBytes == 0 && col >= DDI_COL_Bytes)
		col++;
	switch (col)
	{
	case DDI_COL_ID: return "ID";
	case DDI_COL_IID: return "IID";
	case DDI_COL_CR: return "CR";
	case DDI_COL_File: return "File";
	case DDI_COL_Offset: return "Offset";
	case DDI_COL_Struct: return "Struct";
	case DDI_COL_Bytes: return "Bytes";
	default: return filterStruct->fields[col - DDI_COL_FirstField].name;
	}
}

std::string DataDescInstanceSource::GetText(size_t row, size_t col)
{
	if (filterFileEnable && filterFile && col >= DDI_COL_File)
		col++;
	if (filterStructEnable && filterStruct && col >= DDI_COL_Struct)
		col++;
	if (showBytes == 0 && col >= DDI_COL_Bytes)
		col++;
	switch (col)
	{
	case DDI_COL_ID: return std::to_string(_indices[row]);
	case DDI_COL_IID: return std::to_string(dataDesc->instances[_indices[row]]->id);
	case DDI_COL_CR: return CreationReasonToStringShort(dataDesc->instances[_indices[row]]->creationReason);
	case DDI_COL_File: return dataDesc->instances[_indices[row]]->file->name;
	case DDI_COL_Offset: return std::to_string(dataDesc->instances[_indices[row]]->off);
	case DDI_COL_Struct: return dataDesc->instances[_indices[row]]->def->name;
	case DDI_COL_Bytes: {
		uint32_t nbytes = std::min(showBytes, 128U);
		uint8_t buf[128];
		auto* inst = dataDesc->instances[_indices[row]];
		inst->file->dataSource->Read(inst->off, nbytes, buf);
		std::string text;
		for (uint32_t i = 0; i < nbytes; i++)
		{
			if (i)
				text += " ";
			char tbf[32];
			snprintf(tbf, 32, "%02X", buf[i]);
			text += tbf;
		}
		return text;
	} break;
	default: return dataDesc->instances[_indices[row]]->GetFieldPreview(col - DDI_COL_FirstField);
	}
}

void DataDescInstanceSource::ClearSelection()
{
	dataDesc->SetCurrentInstance(nullptr);
}

bool DataDescInstanceSource::GetSelectionState(uintptr_t item)
{
	return dataDesc->curInst == dataDesc->instances[_indices[item]];
}

void DataDescInstanceSource::SetSelectionState(uintptr_t item, bool sel)
{
	if (sel)
		dataDesc->SetCurrentInstance(dataDesc->instances[_indices[item]]);
	else if (GetSelectionState(item))
		dataDesc->SetCurrentInstance(nullptr);
}

struct StructOptions : ui::OptionList
{
	DataDesc* desc;

	StructOptions(DataDesc* d) : desc(d) {}

	void IterateElements(size_t from, size_t count, std::function<ElementFunc>&& fn) override
	{
		std::vector<DDStruct*> structs;
		structs.reserve(desc->structs.size() + 1);
		structs.push_back(nullptr);
		for (auto& kvp : desc->structs)
			structs.push_back(kvp.second);
		std::sort(structs.begin() + 1, structs.end(), [](const DDStruct* a, const DDStruct* b) { return a->name < b->name; });
		
		for (size_t i = from; i < from + count && i < structs.size(); i++)
			fn(structs[i], uintptr_t(structs[i]));
	}
	void BuildElement(UIContainer* ctx, const void* ptr, uintptr_t id, bool list) override
	{
		auto* s = static_cast<const DDStruct*>(ptr);
		ctx->Text(s ? s->name : "<none>");
	}
};

struct FileOptions : ui::OptionList
{
	DataDesc* desc;

	FileOptions(DataDesc* d) : desc(d) {}

	void IterateElements(size_t from, size_t count, std::function<ElementFunc>&& fn) override
	{
		std::vector<DDFile*> files = desc->files;
		std::sort(files.begin(), files.end(), [](const DDFile* a, const DDFile* b) { return a->name < b->name; });
		files.insert(files.begin(), nullptr);

		for (size_t i = from; i < from + count && i < files.size(); i++)
			fn(files[i], uintptr_t(files[i]));
	}
	void BuildElement(UIContainer* ctx, const void* ptr, uintptr_t id, bool list) override
	{
		auto* s = static_cast<const DDFile*>(ptr);
		ctx->Text(s ? s->name : "<none>");
	}
};

struct StructsDropdownMenu : ui::DropdownMenu
{
	DataDesc* dataDesc = nullptr;
	std::unordered_set<DDStruct*>* structSet = nullptr;

	void OnBuildButtonContents(UIContainer* ctx) override
	{
		if (structSet->empty())
			ctx->Text("<none>");
		else if (structSet->size() == dataDesc->structs.size())
			ctx->Text("<all>");
		else if (structSet->size() == 1)
			ctx->Text((*structSet->begin())->name);
		else
			ctx->Text("<many>");
	}

	void OnBuildMenuContents(UIContainer* ctx) override
	{
		std::vector<DDStruct*> structs;
		for (auto& kvp : dataDesc->structs)
			structs.push_back(kvp.second);
		std::sort(structs.begin(), structs.end(), [](const DDStruct* A, const DDStruct* B) { return A->name < B->name; });

		for (DDStruct* S : structs)
		{
			if (ui::imm::CheckboxRaw(ctx, structSet->count(S) > 0, S->name.c_str()))
			{
				if (structSet->count(S))
					structSet->erase(S);
				else
					structSet->insert(S);
			}
		}
	}
};

void DataDescInstanceSource::Edit(UIContainer* ctx)
{
	ui::Property::Begin(ctx, "Filter by struct");
	if (ui::imm::EditBool(ctx, filterStructEnable, nullptr))
		refilter = true;
	if (ui::imm::DropdownMenuList(ctx, filterStruct, ctx->GetCurrentNode()->Allocate<StructOptions>(dataDesc)))
		refilter = true;
	ui::imm::PropEditInt(ctx, "\bBytes", showBytes, {}, 1, 0, 128);
	ui::Property::End(ctx);

	if (!filterStructEnable || !filterStruct)
	{
		ui::Property::Scope ps(ctx, "Hide structs");
		if (ui::imm::EditBool(ctx, filterHideStructsEnable, nullptr))
			refilter = true;
		auto* ddm = ctx->Make<StructsDropdownMenu>();
		ddm->dataDesc = dataDesc;
		ddm->structSet = &filterHideStructs;
		ddm->HandleEvent(UIEventType::IMChange) = [this](UIEvent& e)
		{
			refilter = true;
			e.current->RerenderContainerNode();
		};
	}

	ui::Property::Begin(ctx, "Filter by file");
	if (ui::imm::EditBool(ctx, filterFileEnable, nullptr))
		refilter = true;
	if (ui::imm::DropdownMenuList(ctx, filterFile, ctx->GetCurrentNode()->Allocate<FileOptions>(dataDesc)))
		refilter = true;
	ui::imm::PropEditBool(ctx, "\bFollow", filterFileFollow);
	ui::Property::End(ctx);

	if (EditCreationReason(ctx, "Filter by creation reason", filterCreationReason))
		refilter = true;
}

void DataDescInstanceSource::_Refilter()
{
	if (!refilter)
		return;

	_indices.clear();
	_indices.reserve(dataDesc->instances.size());
	for (size_t i = 0; i < dataDesc->instances.size(); i++)
	{
		auto* I = dataDesc->instances[i];
		if (filterStructEnable && filterStruct && filterStruct != I->def)
			continue;
		else if (filterHideStructsEnable && filterHideStructs.count(I->def))
			continue;
		if (filterFileEnable && filterFile && filterFile != I->file)
			continue;
		if (I->creationReason > filterCreationReason)
			continue;
		_indices.push_back(i);
	}

	refilter = false;
}


enum COLS_DDIMG
{
	DDIMG_COL_ID,
	DDIMG_COL_User,
	DDIMG_COL_File,
	DDIMG_COL_ImgOff,
	DDIMG_COL_PalOff,
	DDIMG_COL_Format,
	DDIMG_COL_Width,
	DDIMG_COL_Height,

	DDIMG_COL_HEADER_SIZE,
};

size_t DataDescImageSource::GetNumRows()
{
	_Refilter();
	return _indices.size();
}

size_t DataDescImageSource::GetNumCols()
{
	size_t ncols = DDIMG_COL_HEADER_SIZE;
	if (filterFile)
		ncols--;
	return ncols;
}

std::string DataDescImageSource::GetRowName(size_t row)
{
	return std::to_string(row);
}

std::string DataDescImageSource::GetColName(size_t col)
{
	if (filterFile && col >= DDIMG_COL_File)
		col++;
	switch (col)
	{
	case DDIMG_COL_ID: return "ID";
	case DDIMG_COL_User: return "User";
	case DDIMG_COL_File: return "File";
	case DDIMG_COL_ImgOff: return "Image Offset";
	case DDIMG_COL_PalOff: return "Palette Offset";
	case DDIMG_COL_Format: return "Format";
	case DDIMG_COL_Width: return "Width";
	case DDIMG_COL_Height: return "Height";
	default: return "???";
	}
}

std::string DataDescImageSource::GetText(size_t row, size_t col)
{
	if (filterFile && col >= DDIMG_COL_File)
		col++;
	switch (col)
	{
	case DDIMG_COL_ID: return std::to_string(_indices[row]);
	case DDIMG_COL_User: return dataDesc->images[_indices[row]].userCreated ? "+" : "";
	case DDIMG_COL_File: return dataDesc->images[_indices[row]].file->name;
	case DDIMG_COL_ImgOff: return std::to_string(dataDesc->images[_indices[row]].offImage);
	case DDIMG_COL_PalOff: return std::to_string(dataDesc->images[_indices[row]].offPalette);
	case DDIMG_COL_Format: return dataDesc->images[_indices[row]].format;
	case DDIMG_COL_Width: return std::to_string(dataDesc->images[_indices[row]].width);
	case DDIMG_COL_Height: return std::to_string(dataDesc->images[_indices[row]].height);
	default: return "???";
	}
}

void DataDescImageSource::ClearSelection()
{
	dataDesc->curImage = UINT32_MAX;
}

bool DataDescImageSource::GetSelectionState(uintptr_t item)
{
	return dataDesc->curImage == _indices[item];
}

void DataDescImageSource::SetSelectionState(uintptr_t item, bool sel)
{
	if (sel)
		dataDesc->curImage = _indices[item];
	else if (GetSelectionState(item))
		dataDesc->curImage = UINT32_MAX;
}

void DataDescImageSource::Edit(UIContainer* ctx)
{
	if (ui::imm::PropDropdownMenuList(ctx, "Filter by file", filterFile, ctx->GetCurrentNode()->Allocate<FileOptions>(dataDesc)))
		refilter = true;
	if (ui::imm::PropEditBool(ctx, "Show user created only", filterUserCreated))
		refilter = true;
}

void DataDescImageSource::_Refilter()
{
	if (!refilter)
		return;

	_indices.clear();
	_indices.reserve(dataDesc->images.size());
	for (size_t i = 0; i < dataDesc->images.size(); i++)
	{
		auto& I = dataDesc->images[i];
		if (filterFile && filterFile != I.file)
			continue;
		if (filterUserCreated && !I.userCreated)
			continue;
		_indices.push_back(i);
	}

	refilter = false;
}
