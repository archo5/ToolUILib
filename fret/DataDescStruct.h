
#pragma once
#include "pch.h"

#include "FileReaders.h"
#include "MathExpr.h"
#include "MeshScript.h"


using CacheVersion = uint32_t;
constexpr int64_t F_NO_VALUE = -1;


struct DDParam
{
	std::string name;
	int64_t intVal;
};
struct DDCondition
{
	std::string field;
	std::string value;
};
struct DDCompArg
{
	std::string name;
	std::string src;
	int64_t intVal;
};
struct DDField
{
	std::string type;
	std::string name;

	MathExprObj valueExpr;

	int64_t off = 0;
	MathExprObj offExpr;

	int64_t count = 1;
	std::string countSrc;
	bool countIsMaxSize = false;
	bool individualComputedOffsets = false;
	bool readUntil0 = false;
	std::vector<DDCompArg> structArgs;
	MathExprObj condition;
	MathExprObj elementCondition;

	bool IsComputed() const
	{
		return !valueExpr.expr.empty() || !offExpr.expr.empty();
	}
	bool IsOne() const { return count == 1 && countSrc.empty(); }
};
enum class DDStructResourceType
{
	None,
	Image,
	Mesh,
};
bool EditImageFormat(const char* label, std::string& format);
struct DDRsrcImage
{
	struct FormatOverride
	{
		std::string format;
		MathExprObj condition;
	};
	std::string format;
	std::vector<FormatOverride> formatOverrides;
	MathExprObj imgOff;
	MathExprObj palOff;
	MathExprObj width;
	MathExprObj height;
};
struct DDRsrcMesh
{
	MeshScript script;
};
struct DDStructResource
{
	DDStructResourceType type = DDStructResourceType::None;
	DDRsrcImage* image = nullptr;
	DDRsrcMesh* mesh = nullptr;

	void Load(NamedTextSerializeReader& r);
	void Save(NamedTextSerializeWriter& w);
};
struct DDStruct
{
	std::string name;
	bool serialized = false;
	std::vector<DDParam> params;
	std::vector<DDField> fields;
	int64_t size = 0;
	std::string sizeSrc;
	DDStructResource resource;

	CacheVersion editVersionS = 1;

	void OnEdit()
	{
		editVersionS++;
	}
	size_t GetFieldCount() const { return fields.size(); }
	size_t FindFieldByName(ui::StringView name);
	void Load(NamedTextSerializeReader& r);
	void Save(NamedTextSerializeWriter& w);
};
struct DDArg
{
	std::string name;
	int64_t intVal;
};
enum class CreationReason : uint8_t
{
	UserDefined,
	ManualExpand,
	AutoExpand,
	Query,
};
enum class OptionalBool : int8_t
{
	Unknown = -1,
	False = 0,
	True = 1,
};
struct DDReadFieldValue
{
	std::string preview;
	int64_t off;
	int64_t intVal;
};
struct DDReadField
{
	std::vector<DDReadFieldValue> values;
	std::string preview;
	int64_t origOff = F_NO_VALUE;
	int64_t off = F_NO_VALUE;
	int64_t count = F_NO_VALUE;
	int64_t totalSize = F_NO_VALUE;
	int64_t readOff = F_NO_VALUE;
	bool present;
};
struct DDStructInst
{
	int64_t id = -1;
	DataDesc* desc = nullptr;
	DDStruct* def = nullptr;
	struct DDFile* file = nullptr;
	int64_t off = 0;
	std::string notes;
	CreationReason creationReason = CreationReason::UserDefined;
	bool allowAutoExpand = true;
	bool remainingCountIsSize = false;
	int64_t remainingCount = 1;
	bool sizeOverrideEnable = false;
	int64_t sizeOverrideValue = 0;
	std::vector<DDArg> args;

	CacheVersion editVersionSI = 1;
	mutable CacheVersion cacheSizeVersionSI = 0;
	mutable CacheVersion cacheSizeVersionS = 0;
	mutable int64_t cachedSize = F_NO_VALUE;
	mutable CacheVersion cacheFieldsVersionSI = 0;
	mutable CacheVersion cacheFieldsVersionS = 0;
	mutable int64_t cachedReadOff = F_NO_VALUE;
	mutable std::vector<DDReadField> cachedFields;

	std::string GetFieldDescLazy(size_t i, bool* incomplete = nullptr) const;
	int64_t GetSize(bool lazy = false) const;
	bool IsFieldPresent(size_t i) const;
	OptionalBool IsFieldPresent(size_t i, bool lazy) const;
	const std::string& GetFieldPreview(size_t i, bool lazy = false) const;
	const std::string& GetFieldValuePreview(size_t i, size_t n = 0) const;
	int64_t GetFieldIntValue(size_t i, size_t n = 0) const;
	int64_t GetFieldOffset(size_t i, bool lazy = false) const;
	int64_t GetFieldValueOffset(size_t i, size_t n = 0, bool lazy = false) const;
	int64_t GetFieldElementCount(size_t i, bool lazy = false) const;
	int64_t GetFieldTotalSize(size_t i, bool lazy = false) const;
	int64_t GetCompArgValue(const DDCompArg& arg) const;
	DDStructInst* CreateFieldInstances(size_t i, size_t upToN, CreationReason cr, std::function<bool(DDStructInst*)> oneach = {}) const;
	OptionalBool CanCreateNextInstance(bool lazy = false, bool loose = false) const;
	std::string GetNextInstanceInfo(bool lazy = false) const;
	DDStructInst* CreateNextInstance(CreationReason cr) const;

	void _CheckFieldCache() const;
	void _EnumerateFields(size_t untilNum, bool lazy = false) const;
	int64_t _CalcSize(bool lazy) const;
	int64_t _CalcFieldElementCount(size_t i) const;
	bool _CanReadMoreFields(size_t i) const;
	bool _ReadFieldValues(size_t i, size_t n) const;

	void OnEdit()
	{
		editVersionSI++;
	}
};
