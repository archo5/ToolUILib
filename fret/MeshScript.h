
#pragma once
#include "pch.h"
#include "../Editors/TreeEditor.h"

#include "FileReaders.h"
#include "MathExpr.h"


extern ui::DataCategoryTag DCT_MeshScriptChanged[1];


using Vec2f = Point<float>;

enum class MSPrimType : uint8_t
{
	Points,
	Lines,
	Triangles,
	Quads,
};

enum class MSVDDest : uint8_t
{
	Position,
	Normal,
	Texcoord,
	Color,
};

enum class MSVDType : uint8_t
{
	I8,
	U8,
	I16,
	U16,
	I32,
	U32,
	I64,
	U64,
	F32,
	F64,
};

struct MSPrimitive
{
	std::vector<Vec3f> positions;
	std::vector<Vec3f> normals;
	std::vector<Vec2f> texcoords;
	std::vector<ui::Color4f> colors;
	std::vector<uint32_t> indices;
	MSPrimType type;
};

struct MSData
{
	std::vector<MSPrimitive> primitives;
};

struct MSError
{
	std::weak_ptr<struct MSNode> node;
	std::string error;
};

struct MSContext
{
	MSData* data;
	IDataSource* src;
	IVariableSource* vs;

	std::weak_ptr<struct MSNode> curNode;
	std::vector<MSError> errors;

	void Error(std::string&& text)
	{
		errors.push_back({ curNode, std::move(text) });
	}
};

struct MSNode
{
	using Ptr = std::shared_ptr<MSNode>;

	std::vector<Ptr> children;

	virtual ~MSNode() {}
	MSNode* Clone();

	virtual void Do(MSContext& C) = 0;
	virtual const char* GetName() = 0;
	virtual MSNode* CloneBase() = 0;
	virtual void InlineEditUI(UIContainer* ctx) = 0;
	virtual void FullEditUI(UIContainer* ctx) = 0;
};

#define MSN_NODE(name) \
	const char* GetName() override { return #name; } \
	MSNode* CloneBase() override { return new MSN_##name(*this); }

struct MeshScript : ui::ITree
{
	std::vector<MSNode::Ptr> roots;
	std::weak_ptr<MSNode> selected;

	~MeshScript();
	void Clear();
	MSData RunScript(IDataSource* src, IVariableSource* instCtx);
	void EditUI(UIContainer* ctx);

	struct NodeLoc
	{
		std::vector<MSNode::Ptr>* arr;
		size_t idx;

		MSNode::Ptr Get() const { return arr->at(idx); }
	};
	NodeLoc FindNode(ui::TreePathRef path);

	void IterateChildren(ui::TreePathRef path, IterationFunc&& fn) override;
	bool HasChildren(ui::TreePathRef path) override;
	size_t GetChildCount(ui::TreePathRef path) override;

	uint32_t lastVer = ui::ContextMenu::Get().GetVersion();
	void FillItemContextMenu(ui::MenuItemCollection& mic, ui::TreePathRef path) override;
	void FillListContextMenu(ui::MenuItemCollection& mic) override;
	void GenerateInsertMenu(ui::MenuItemCollection& mic, ui::TreePathRef path);
	void Insert(ui::TreePathRef path, MSNode* node);

	void ClearSelection() override;
	bool GetSelectionState(ui::TreePathRef path) override;
	void SetSelectionState(ui::TreePathRef path, bool sel) override;

	void Remove(ui::TreePathRef path) override;
	void Duplicate(ui::TreePathRef path) override;
	void MoveTo(ui::TreePathRef node, ui::TreePathRef dest) override;
};

struct MSN_NewPrimitive : MSNode
{
	MSN_NODE(NewPrimitive);

	void Do(MSContext& C) override;
	void InlineEditUI(UIContainer* ctx) override;
	void FullEditUI(UIContainer* ctx) override;

	MSPrimType type = MSPrimType::Points;
};

struct MSN_VertexData : MSNode
{
	MSN_NODE(VertexData);

	void Do(MSContext& C) override;
	void InlineEditUI(UIContainer* ctx) override;
	void FullEditUI(UIContainer* ctx) override;

	MSVDDest dest = MSVDDest::Position;
	MSVDType type = MSVDType::F32;
	int ncomp = 3;
	MathExprObj count;
	MathExprObj stride;
	MathExprObj attrOff;
};
