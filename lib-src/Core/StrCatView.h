
#pragma once

#include "Platform.h"
#include "String.h"


namespace ui {

// similar to llvm::Twine

struct StrCatView
{
	enum class FragType : u8
	{
		Empty,
		NullTerm,
		PtrSize,
		CatView,
	};
	union FragData
	{
		const char* nullTerm;
		struct
		{
			const char* ptr;
			size_t size;
		}
		ptrSize;
		const StrCatView* catView;
	};

	FragData ldata, rdata;
	FragType ltype, rtype;

	StrCatView& operator = (const StrCatView&) = delete;

	StrCatView() : ltype(FragType::Empty), rtype(FragType::Empty) {}
	StrCatView(const char* s) : ltype(FragType::NullTerm), rtype(FragType::Empty) { ldata.nullTerm = s; }
	StrCatView(const char* s, size_t l) : ltype(FragType::PtrSize), rtype(FragType::Empty)
	{
		ldata.ptrSize.ptr = s;
		ldata.ptrSize.size = l;
	}
	StrCatView(StringView sv) : ltype(FragType::PtrSize), rtype(FragType::Empty)
	{
		ldata.ptrSize.ptr = sv._data;
		ldata.ptrSize.size = sv._size;
	}

	StrCatView operator + (const StrCatView& o) const
	{
		StrCatView ret;
		ret.ltype = FragType::CatView;
		ret.rtype = FragType::CatView;
		ret.ldata.catView = this;
		ret.rdata.catView = &o;
		return ret;
	}

	static size_t _Size(FragType t, FragData d);
	static void _Append(std::string& dest, FragType t, FragData d);

	size_t Size() const
	{
		return _Size(ltype, ldata) + _Size(rtype, rdata);
	}

	void AppendTo(std::string& dest) const
	{
		_Append(dest, ltype, ldata);
		_Append(dest, rtype, rdata);
	}

	std::string Str() const
	{
		std::string ret;
		ret.reserve(Size());
		AppendTo(ret);
		return ret;
	}
};

using SCV = StrCatView;

} // ui
