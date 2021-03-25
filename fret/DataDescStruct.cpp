
#include "pch.h"
#include "DataDescStruct.h"
#include "DataDesc.h"


struct BuiltinTypeInfo
{
	uint8_t size;
	bool separated;
	int64_t(*get_int64)(const void* src);
	void(*append_to_str)(const void* src, std::string& out);
};

static void AddChar(std::string& out, char c)
{
	if (c >= 0x20 && c < 0x7f)
		out.push_back(c);
	else
	{
		out.push_back('\\');
		out.push_back('x');
		out.push_back("0123456789abcdef"[c >> 4]);
		out.push_back("0123456789abcdef"[c & 15]);
	}
}

static std::unordered_map<std::string, BuiltinTypeInfo> g_builtinTypes
{
	{ "pad", { 1, false, [](const void* src) -> int64_t { return 0; }, [](const void* src, std::string& out) {} } },
	{ "char", { 1, false, [](const void* src) -> int64_t { return *(const char*)src; }, [](const void* src, std::string& out) { AddChar(out, *(const char*)src); } } },
	{ "i8", { 1, true, [](const void* src) -> int64_t { return *(const int8_t*)src; }, [](const void* src, std::string& out) { char bfr[32]; snprintf(bfr, 32, "%" PRId8, *(const int8_t*)src); out += bfr; } } },
	{ "u8", { 1, true, [](const void* src) -> int64_t { return *(const uint8_t*)src; }, [](const void* src, std::string& out) { char bfr[32]; snprintf(bfr, 32, "%" PRIu8, *(const uint8_t*)src); out += bfr; } } },
	{ "i16", { 2, true, [](const void* src) -> int64_t { return *(const int16_t*)src; }, [](const void* src, std::string& out) { char bfr[32]; snprintf(bfr, 32, "%" PRId16, *(const int16_t*)src); out += bfr; } } },
	{ "u16", { 2, true, [](const void* src) -> int64_t { return *(const uint16_t*)src; }, [](const void* src, std::string& out) { char bfr[32]; snprintf(bfr, 32, "%" PRIu16, *(const uint16_t*)src); out += bfr; } } },
	{ "i32", { 4, true, [](const void* src) -> int64_t { return *(const int32_t*)src; }, [](const void* src, std::string& out) { char bfr[32]; snprintf(bfr, 32, "%" PRId32, *(const int32_t*)src); out += bfr; } } },
	{ "u32", { 4, true, [](const void* src) -> int64_t { return *(const uint32_t*)src; }, [](const void* src, std::string& out) { char bfr[32]; snprintf(bfr, 32, "%" PRIu32, *(const uint32_t*)src); out += bfr; } } },
	{ "f32", { 4, true, [](const void* src) -> int64_t { return *(const float*)src; }, [](const void* src, std::string& out) { char bfr[32]; snprintf(bfr, 32, "%g", *(const float*)src); out += bfr; } } },
};


void DDStructResource::Load(NamedTextSerializeReader& r)
{
	auto rsrcType = r.ReadString("type");
	if (rsrcType == "image")
		type = DDStructResourceType::Image;
	if (rsrcType == "mesh")
		type = DDStructResourceType::Mesh;

	if (r.BeginDict("image"))
	{
		image = new DDRsrcImage;
		image->format = r.ReadString("format");

		r.BeginArray("formatOverrides");
		for (auto E2 : r.GetCurrentRange())
		{
			r.BeginEntry(E2);
			r.BeginDict("");

			DDRsrcImage::FormatOverride FO;
			FO.format = r.ReadString("format");
			FO.condition.SetExpr(r.ReadString("condition"));

			image->formatOverrides.push_back(FO);

			r.EndDict();
			r.EndEntry();
		}
		r.EndArray();

		image->imgOff.SetExpr(r.ReadString("imgOff"));
		image->palOff.SetExpr(r.ReadString("palOff"));
		image->width.SetExpr(r.ReadString("width"));
		image->height.SetExpr(r.ReadString("height"));
	}
	r.EndDict();

	if (r.BeginDict("mesh"))
	{
		mesh = new DDRsrcMesh;
		mesh->script.Load(r);
	}
	r.EndDict();
}

void DDStructResource::Save(NamedTextSerializeWriter& w)
{
	switch (type)
	{
	case DDStructResourceType::Image:
		w.WriteString("type", "image");
		break;
	case DDStructResourceType::Mesh:
		w.WriteString("type", "mesh");
		break;
	}

	if (image)
	{
		w.BeginDict("image");

		w.WriteString("format", image->format);

		w.BeginArray("formatOverrides");
		for (auto& FO : image->formatOverrides)
		{
			w.BeginDict("");
			w.WriteString("format", FO.format);
			w.WriteString("condition", FO.condition.expr);
			w.EndDict();
		}
		w.EndArray();

		w.WriteString("imgOff", image->imgOff.expr);
		w.WriteString("palOff", image->palOff.expr);
		w.WriteString("width", image->width.expr);
		w.WriteString("height", image->height.expr);

		w.EndDict();
	}

	if (mesh)
	{
		w.BeginDict("mesh");
		mesh->script.Save(w);
		w.EndDict();
	}
}

size_t DDStruct::FindFieldByName(ui::StringView name)
{
	for (size_t i = 0; i < fields.size(); i++)
		if (ui::StringView(fields[i].name) == name)
			return i;
	return SIZE_MAX;
}

void DDStruct::Load(NamedTextSerializeReader& r)
{
	serialized = r.ReadBool("serialized");

	r.BeginArray("params");
	for (auto E2 : r.GetCurrentRange())
	{
		r.BeginEntry(E2);
		r.BeginDict("");

		DDParam P;
		P.name = r.ReadString("name");
		P.intVal = r.ReadInt64("intVal");
		params.push_back(P);

		r.EndDict();
		r.EndEntry();
	}
	r.EndArray();

	r.BeginArray("fields");
	for (auto E2 : r.GetCurrentRange())
	{
		r.BeginEntry(E2);
		r.BeginDict("");

		DDField F;
		F.type = r.ReadString("type");
		F.name = r.ReadString("name");
		F.valueExpr.SetExpr(r.ReadString("valueExpr"));
		F.off = r.ReadInt64("off");
		F.offExpr.SetExpr(r.ReadString("offExpr"));
		F.count = r.ReadInt64("count");
		F.countSrc = r.ReadString("countSrc");
		F.countIsMaxSize = r.ReadBool("countIsMaxSize");
		F.individualComputedOffsets = r.ReadBool("individualComputedOffsets");
		F.readUntil0 = r.ReadBool("readUntil0");

		r.BeginArray("structArgs");
		for (auto E3 : r.GetCurrentRange())
		{
			r.BeginEntry(E3);
			r.BeginDict("");

			DDCompArg A;
			A.name = r.ReadString("name");
			A.src = r.ReadString("src");
			A.intVal = r.ReadInt64("intVal");
			F.structArgs.push_back(A);

			r.EndDict();
			r.EndEntry();
		}
		r.EndArray();

		F.condition.SetExpr(r.ReadString("condition"));
		F.elementCondition.SetExpr(r.ReadString("elementCondition"));

		fields.push_back(F);

		r.EndDict();
		r.EndEntry();
	}
	r.EndArray();

	r.BeginDict("resource");
	resource.Load(r);
	r.EndDict();

	size = r.ReadInt64("size");
	sizeSrc = r.ReadString("sizeSrc");
}

void DDStruct::Save(NamedTextSerializeWriter& w)
{
	w.WriteString("name", name);
	w.WriteBool("serialized", serialized);

	w.BeginArray("params");
	for (DDParam& P : params)
	{
		w.BeginDict("");
		w.WriteString("name", P.name);
		w.WriteInt("intVal", P.intVal);
		w.EndDict();
	}
	w.EndArray();

	w.BeginArray("fields");
	for (const DDField& F : fields)
	{
		w.BeginDict("");
		w.WriteString("type", F.type);
		w.WriteString("name", F.name);
		w.WriteString("valueExpr", F.valueExpr.expr);
		w.WriteInt("off", F.off);
		w.WriteString("offExpr", F.offExpr.expr);
		w.WriteInt("count", F.count);
		w.WriteString("countSrc", F.countSrc);
		w.WriteBool("countIsMaxSize", F.countIsMaxSize);
		w.WriteBool("individualComputedOffsets", F.individualComputedOffsets);
		w.WriteBool("readUntil0", F.readUntil0);

		w.BeginArray("structArgs");
		for (const DDCompArg& A : F.structArgs)
		{
			w.BeginDict("");
			w.WriteString("name", A.name);
			w.WriteString("src", A.src);
			w.WriteInt("intVal", A.intVal);
			w.EndDict();
		}
		w.EndArray();

		w.WriteString("condition", F.condition.expr);
		w.WriteString("elementCondition", F.elementCondition.expr);

		w.EndDict();
	}
	w.EndArray();

	if (resource.type != DDStructResourceType::None)
	{
		w.BeginDict("resource");
		resource.Save(w);
		w.EndDict();
	}

	w.WriteInt("size", size);
	w.WriteString("sizeSrc", sizeSrc);
}


std::string DDStructInst::GetFieldDescLazy(size_t i, bool* incomplete) const
{
	_EnumerateFields(i + 1, true);

	if (i >= cachedFields.size() && incomplete)
		*incomplete = true;

	auto& F = def->fields[i];

	std::string ret = F.name;
	ret += ": ";
	ret += F.type;

	if (F.countIsMaxSize || F.countSrc.size() || F.count != 1)
	{
		ret += "[";
		if (F.countIsMaxSize)
			ret += "up to ";
		int64_t elcount = GetFieldElementCount(i, true);
		if (elcount != F_NO_VALUE)
			ret += std::to_string(elcount);
		else
		{
			ret += "?";
			if (incomplete)
				*incomplete = true;
		}
		ret += "]";
	}

	ret += "@";
	int64_t off = GetFieldOffset(i, true);
	if (off != F_NO_VALUE)
		ret += std::to_string(off);
	else
	{
		ret += "?";
		if (incomplete)
			*incomplete = true;
	}

	ret += "=";
	ret += GetFieldPreview(i, true);

	return ret;
}

int64_t DDStructInst::GetSize(bool lazy) const
{
	if (cachedSize == F_NO_VALUE ||
		cacheSizeVersionSI != editVersionSI ||
		cacheSizeVersionS != def->editVersionS)
	{
		cachedSize = _CalcSize(lazy);
		cacheSizeVersionSI = editVersionSI;
		cacheSizeVersionS = def->editVersionS;
	}
	return cachedSize;
}

bool DDStructInst::IsFieldPresent(size_t i) const
{
	_EnumerateFields(i + 1);
	return cachedFields[i].present;
}

OptionalBool DDStructInst::IsFieldPresent(size_t i, bool lazy) const
{
	_EnumerateFields(i + 1, lazy);
	if (i < cachedFields.size())
		return cachedFields[i].present ? OptionalBool::True : OptionalBool::False;
	return OptionalBool::Unknown;
}

std::string g_emptyString;
std::string g_notLoadedStr = "<?>";
std::string g_notPresentStr = "<not present>";
#define MAX_PREVIEW_LENGTH 24
const std::string& DDStructInst::GetFieldPreview(size_t i, bool lazy) const
{
	_EnumerateFields(i + 1, lazy);
	if (lazy && i >= cachedFields.size())
		return g_notLoadedStr;
	auto& CF = cachedFields[i];
	if (!CF.present)
		return g_notPresentStr;
	if (CF.preview.empty())
	{
		// complex offset
		if (GetFieldOffset(i, lazy) == F_NO_VALUE)
			return g_notLoadedStr;

		bool separated = true;
		{
			auto it = g_builtinTypes.find(def->fields[i].type);
			if (it != g_builtinTypes.end())
				separated = it->second.separated;
		}
		for (size_t n = 0; CF.preview.size() < MAX_PREVIEW_LENGTH; n++)
		{
			const auto& fvp = GetFieldValuePreview(i, n);
			if (fvp.empty())
				break;
			if (n && separated)
				CF.preview += ", ";
			CF.preview += fvp;
		}
		if (CF.preview.size() > MAX_PREVIEW_LENGTH || CF.preview.empty())
		{
			CF.preview += "...";
		}
	}
	return CF.preview;
}

const std::string& DDStructInst::GetFieldValuePreview(size_t i, size_t n) const
{
	if (_ReadFieldValues(i, n + 1))
	{
		return cachedFields[i].values[n].preview;
	}
	return g_emptyString;
}

int64_t DDStructInst::GetFieldIntValue(size_t i, size_t n) const
{
	if (_ReadFieldValues(i, n + 1))
	{
		return cachedFields[i].values[n].intVal;
	}
	return 0;
}

int64_t DDStructInst::GetFieldOffset(size_t i, bool lazy) const
{
	_EnumerateFields(i + 1, lazy);
	if (lazy && i >= cachedFields.size())
		return F_NO_VALUE;
	auto& CF = cachedFields[i];
	if (CF.off == F_NO_VALUE && !lazy)
	{
		auto& F = def->fields[i];
		if (F.IsComputed())
		{
			if (F.valueExpr.expr.empty() && F.offExpr.inst)
			{
				PredefinedConstant constants[] =
				{
					{ "i", 0 },
					{ "orig", CF.origOff },
				};
				VariableSource vs;
				{
					vs.desc = desc;
					vs.root = this;
					vs.constants = constants;
					vs.constantCount = sizeof(constants) / sizeof(constants[0]);
				}
				CF.off = F.offExpr.inst->Evaluate(&vs);
			}
		}
		else
		{
			CF.off = 0;
			puts("invalid state, expected enumeration to provide offset");
		}
	}
	return CF.off;
}

int64_t DDStructInst::GetFieldValueOffset(size_t i, size_t n, bool lazy) const
{
	int64_t off = GetFieldOffset(i, lazy);
	if (off == F_NO_VALUE || n == 0)
		return off;
	if (!lazy)
		_ReadFieldValues(i, n + 1);
	if (i < cachedFields.size() &&
		n < cachedFields[i].values.size())
		return cachedFields[i].values[n].off;
	return F_NO_VALUE;
}

int64_t DDStructInst::GetFieldElementCount(size_t i, bool lazy) const
{
	_EnumerateFields(i + 1, lazy);
	if (lazy && i >= cachedFields.size())
		return F_NO_VALUE;
	auto& CF = cachedFields[i];
	if (CF.count == F_NO_VALUE)
		CF.count = CF.present ? _CalcFieldElementCount(i) : 0;
	return CF.count;
}

static uint64_t GetFixedTypeSize(DataDesc* desc, const std::string& type)
{
	auto it = g_builtinTypes.find(type);
	if (it != g_builtinTypes.end())
		return it->second.size;
	auto sit = desc->structs.find(type);
	if (sit != desc->structs.end() && !sit->second->serialized)
		return sit->second->size;
	return UINT64_MAX;
}

int64_t DDStructInst::GetFieldTotalSize(size_t i, bool lazy) const
{
	_EnumerateFields(i + 1);
	if (lazy && i >= cachedFields.size())
		return F_NO_VALUE;
	auto& CF = cachedFields[i];
	if (CF.totalSize == F_NO_VALUE)
	{
		auto& F = def->fields[i];
		if (!F.valueExpr.expr.empty())
			return CF.totalSize = 0;
		auto fs = GetFixedTypeSize(desc, F.type);
		if (fs != UINT64_MAX)
		{
			int64_t numElements = GetFieldElementCount(i);
			if (F.readUntil0)
			{
				_ReadFieldValues(i, numElements);
				if (numElements > CF.values.size())
					numElements = CF.values.size();
			}
			if (F.countIsMaxSize)
				numElements /= fs;
			CF.totalSize = fs * numElements;
		}
		else if (!lazy) // serialized struct, need to create instances
		{
			int64_t totalSize = 0;
			CreateFieldInstances(i, SIZE_MAX, CreationReason::Query, [&totalSize](DDStructInst* SI)
			{
				totalSize += SI->GetSize();
				return true;
			});
			CF.totalSize = totalSize;
		}
	}
	return CF.totalSize;
}

int64_t DDStructInst::GetCompArgValue(const DDCompArg& arg) const
{
	if (arg.src.empty())
		return arg.intVal;

	// search fields
	// TODO: previous only?
	size_t at = def->FindFieldByName(arg.src);
	if (at != SIZE_MAX)
		return GetFieldIntValue(at) + arg.intVal;

	// search arguments
	for (auto& ia : args)
	{
		if (arg.src == ia.name)
			return ia.intVal + arg.intVal;
	}

	// search parameter default values
	for (auto& P : def->params)
	{
		if (arg.src == P.name)
			return P.intVal + arg.intVal;
	}

	return 0;
}

DDStructInst* DDStructInst::CreateFieldInstances(size_t i, size_t upToN, CreationReason cr, std::function<bool(DDStructInst*)> oneach) const
{
	auto& F = def->fields[i];

	if (!F.valueExpr.expr.empty())
		return nullptr;

	int64_t numElements = GetFieldElementCount(i);
	if (F.IsComputed() && F.individualComputedOffsets)
	{
		_EnumerateFields(i + 1);
		DDStructInst* newInst = nullptr;
		DDStructInst newSI = { -1, desc, desc->structs.find(F.type)->second, file, 0, "", cr };
		for (size_t n = 0; n < numElements; n++)
		{
			// compute the offset
			{
				PredefinedConstant constants[] =
				{
					{ "i", n },
					{ "orig", cachedFields[i].origOff },
				};
				VariableSource vs;
				{
					vs.desc = desc;
					vs.root = this;
					vs.constants = constants;
					vs.constantCount = sizeof(constants) / sizeof(constants[0]);
				}
				newSI.off = F.offExpr.Evaluate(vs);
				if (newSI.off >= file->dataSource->GetSize())
					continue;
			}

			if (F.elementCondition.inst)
			{
				PredefinedConstant constants[] =
				{
					{ "i", n },
					{ "off", newSI.off },
					{ "orig", cachedFields[i].origOff },
				};
				VariableSource vs;
				{
					vs.desc = desc;
					vs.root = this;
					vs.constants = constants;
					vs.constantCount = sizeof(constants) / sizeof(constants[0]);
				}
				if (!F.elementCondition.Evaluate(vs))
					continue;
			}

			size_t prevSize = desc->instances.size();
			newInst = desc->AddInstance(newSI);
			if (prevSize != desc->instances.size())
			{
				for (auto& SA : F.structArgs)
				{
					newInst->args.push_back({ SA.name, GetCompArgValue(SA) });
				}
			}

			if (oneach && !oneach(newInst))
				return newInst;
			if (n == upToN)
				return newInst;
		}
		return nullptr;
	}
	else
	{
		size_t n = 0;
		DDStructInst newSI = { -1, desc, desc->structs.find(F.type)->second, file, GetFieldOffset(i), "", cr };
		newSI.remainingCountIsSize = F.countIsMaxSize;
		newSI.remainingCount = numElements;
		size_t prevSize = desc->instances.size();
		DDStructInst* newInst = desc->AddInstance(newSI);
		if (prevSize != desc->instances.size())
		{
			for (auto& SA : F.structArgs)
			{
				newInst->args.push_back({ SA.name, GetCompArgValue(SA) });
			}
		}

		if (oneach && !oneach(newInst))
			return newInst;

		while (n <= upToN && n < numElements && newInst && newInst->CanCreateNextInstance() == OptionalBool::True)
		{
			newInst = newInst->CreateNextInstance(cr);

			if (newInst && oneach && !oneach(newInst))
				return newInst;

			n++;
		}
		return n == upToN ? newInst : nullptr;
	}
}

OptionalBool DDStructInst::CanCreateNextInstance(bool lazy, bool loose) const
{
	int64_t structSize = GetSize(lazy);
	if (structSize == F_NO_VALUE)
		return OptionalBool::Unknown;
	int64_t remSizeSub = remainingCountIsSize ? (sizeOverrideEnable ? sizeOverrideValue : structSize) : 1;
	int64_t diff = remainingCount - remSizeSub;
	return (loose ? diff != 0 : diff > 0) ? OptionalBool::True : OptionalBool::False;
}

std::string DDStructInst::GetNextInstanceInfo(bool lazy) const
{
	int64_t size = GetSize(lazy);
	int64_t remSizeSub = remainingCountIsSize ? (sizeOverrideEnable ? sizeOverrideValue : size) : 1;

	return ui::Format("@%" PRId64 ", after %" PRId64 ", rem. %s: %" PRId64,
		off + size,
		sizeOverrideEnable ? sizeOverrideValue : size,
		remainingCountIsSize ? "size" : "count",
		remainingCount - remSizeSub);
}

DDStructInst* DDStructInst::CreateNextInstance(CreationReason cr) const
{
	int64_t structSize = GetSize();
	if (structSize == 0)
	{
		puts("SIZE=0 - ABORTED");
		return nullptr;
	}
	int64_t remSizeSub = remainingCountIsSize ? (sizeOverrideEnable ? sizeOverrideValue : structSize) : 1;
	if (remainingCount - remSizeSub > 0)
	{
		DDStructInst SIcopy = *this;
		SIcopy.off += sizeOverrideEnable ? sizeOverrideValue : structSize;
		SIcopy.remainingCount -= remSizeSub;
		SIcopy.sizeOverrideEnable = false;
		SIcopy.sizeOverrideValue = 0;
		SIcopy.creationReason = cr;
		return desc->AddInstance(SIcopy);
	}
	return nullptr;
}

void DDStructInst::_CheckFieldCache() const
{
	if (cacheFieldsVersionSI != editVersionSI ||
		cacheFieldsVersionS != def->editVersionS)
	{
		cachedFields.clear();
		cachedReadOff = off;
		cacheFieldsVersionSI = editVersionSI;
		cacheFieldsVersionS = def->editVersionS;
	}
}

void DDStructInst::_EnumerateFields(size_t untilNum, bool lazy) const
{
	if (untilNum > def->fields.size())
		untilNum = def->fields.size();

	_CheckFieldCache();

	while (cachedFields.size() < untilNum)
	{
		size_t i = cachedFields.size();
		const auto& F = def->fields[i];

		cachedFields.emplace_back();
		DDReadField& rf = cachedFields.back();
		InParseVariableSource vs;
		{
			vs.root = this;
			vs.untilField = i;
		};
		rf.present = F.condition.expr.empty() || F.condition.Evaluate(vs) != 0;
		rf.origOff = cachedReadOff;
		if (!F.valueExpr.expr.empty())
		{
			rf.off = 0;
			rf.totalSize = 0;
			rf.count = 1;
		}

		if (!F.IsComputed())
			rf.off = cachedReadOff;

		if (rf.present && !F.IsComputed())
		{
			if (def->serialized)
			{
				int64_t fts = GetFieldTotalSize(i, lazy);
				if (lazy && fts == F_NO_VALUE)
					break; // too complicated to do
				cachedReadOff += fts;
			}
			else
			{
				rf.off += F.off;
			}
		}
	}
}

int64_t DDStructInst::_CalcSize(bool lazy) const
{
	if (def->sizeSrc.size())
	{
		if (lazy)
			return F_NO_VALUE;
#if 0
		for (auto& ca : F.structArgs)
			if (ca.name == def->sizeSrc)
				return GetCompArgValue(SI, F, rfs, ca) + def->size;
#endif

		for (size_t i = 0; i < def->fields.size(); i++)
			if (def->fields[i].name == def->sizeSrc)
				return GetFieldIntValue(i) + def->size;

#if 1
		for (auto& arg : args)
			if (arg.name == def->sizeSrc)
				return arg.intVal + def->size;
#endif

		return def->size;
	}
	else if (def->serialized)
	{
		if (lazy)
			return F_NO_VALUE;
		_EnumerateFields(def->fields.size());
		return cachedReadOff - off;
	}
	else
		return def->size;
}

int64_t DDStructInst::_CalcFieldElementCount(size_t i) const
{
	auto& F = def->fields[i];

	if (!F.valueExpr.expr.empty())
		return 1;

	if (F.countSrc.empty())
		return F.count;

	if (F.countSrc == ":file-size")
		return file->dataSource->GetSize() + F.count;

	// search previous fields
	for (size_t at = 0; at < i; at++)
	{
		auto& field = def->fields[at];
		if (F.countSrc == field.name)
			return GetFieldIntValue(at) + F.count;
	}

	// search arguments
	for (auto& ia : args)
	{
		if (F.countSrc == ia.name)
			return ia.intVal + F.count;
	}

	// search parameter default values
	for (auto& P : def->params)
	{
		if (F.countSrc == P.name)
			return P.intVal + F.count;
	}

	return 0;
}

bool DDStructInst::_CanReadMoreFields(size_t i) const
{
	auto& F = def->fields[i];
	auto& CF = cachedFields[i];
	if (F.countIsMaxSize)
		return CF.count > CF.readOff - CF.off;
	else
		return CF.count > CF.values.size();
}

bool DDStructInst::_ReadFieldValues(size_t i, size_t n) const
{
	_EnumerateFields(i + 1);
	auto& CF = cachedFields[i];
	if (CF.values.size() >= n)
		return true;

	auto& F = def->fields[i];
	if (!F.valueExpr.expr.empty())
	{
		if (CF.values.empty())
		{
			PredefinedConstant constants[] =
			{
				{ "orig", CF.origOff },
			};
			VariableSource vs;
			{
				vs.desc = desc;
				vs.root = this;
				vs.constants = constants;
				vs.constantCount = sizeof(constants) / sizeof(constants[0]);
			}
			int64_t value = F.valueExpr.Evaluate(vs);
			CF.values.emplace_back();
			auto& CFV = CF.values.back();
			CFV.intVal = value;
			CFV.off = 0;
			CFV.preview = std::to_string(value);
		}
		return CF.values.size() >= n;
	}

	auto it = g_builtinTypes.find(F.type);
	if (it == g_builtinTypes.end())
		return false;

	if (CF.readOff == F_NO_VALUE)
		CF.readOff = GetFieldOffset(i);

	GetFieldElementCount(i);

	auto& BTI = it->second;
	while (CF.values.size() < n && _CanReadMoreFields(i))
	{
		if (F.readUntil0 && CF.values.size() && CF.values.back().intVal == 0)
			break;

		CF.values.emplace_back();
		auto& CFV = CF.values.back();

		CFV.off = CF.readOff;
		__declspec(align(8)) char bfr[8];
		file->dataSource->Read(CF.readOff, BTI.size, bfr);
		CFV.intVal = BTI.get_int64(bfr);
		BTI.append_to_str(bfr, CFV.preview);

		CF.readOff += BTI.size;
	}
	return CF.values.size() >= n;
}
