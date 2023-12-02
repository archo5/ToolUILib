
#pragma once

#include "StrCatView.h"


namespace ui {

size_t StrCatView::_Size(FragType t, FragData d)
{
	switch (t)
	{
	case FragType::Empty:
	default:
		return 0;
	case FragType::NullTerm:
		return strlen(d.nullTerm);
	case FragType::PtrSize:
		return d.ptrSize.size;
	case FragType::CatView:
		return d.catView->Size();
	}
}

void StrCatView::_Append(std::string& dest, FragType t, FragData d)
{
	switch (t)
	{
	case FragType::Empty:
		break;
	case FragType::NullTerm:
		dest += d.nullTerm;
		break;
	case FragType::PtrSize:
		dest.append(d.ptrSize.ptr, d.ptrSize.size);
		break;
	case FragType::CatView:
		d.catView->AppendTo(dest);
		break;
	}
}

#if UI_BUILD_TESTS
#include "Test.h"
DEFINE_TEST_CATEGORY(StrCatView, 50);
DEFINE_TEST(StrCatView, Basics)
{
	ASSERT_EQUAL(true, SCV().ltype == SCV::FragType::Empty);
	ASSERT_EQUAL(true, SCV().rtype == SCV::FragType::Empty);

	const char* nullterm = "test";
	ASSERT_EQUAL(true, SCV(nullterm).ltype == SCV::FragType::NullTerm);
	ASSERT_EQUAL(true, SCV(nullterm).rtype == SCV::FragType::Empty);
	ASSERT_EQUAL(true, SCV(nullterm).ldata.nullTerm == nullterm);
}

DEFINE_TEST(StrCatView, Sizes)
{
	ASSERT_EQUAL(true, SCV().Size() == 0);
	const char* nullterm = "test";
	ASSERT_EQUAL(true, SCV(nullterm).Size() == 4);
	ASSERT_EQUAL(true, SCV(nullterm, 3).Size() == 3);
	ASSERT_EQUAL(true, (SCV() + SCV()).Size() == 0);
	ASSERT_EQUAL(true, (SCV(nullterm) + SCV()).Size() == 4);
	ASSERT_EQUAL(true, (SCV() + SCV(nullterm)).Size() == 4);
	ASSERT_EQUAL(true, (SCV(nullterm) + SCV(nullterm)).Size() == 8);
}

DEFINE_TEST(StrCatView, Outputs)
{
	ASSERT_EQUAL(true, SCV().Str() == "");
	const char* nullterm = "test";
	ASSERT_EQUAL(true, SCV(nullterm).Str() == "test");
	ASSERT_EQUAL(true, SCV(nullterm, 3).Str() == "tes");
	ASSERT_EQUAL(true, (SCV() + SCV()).Str() == "");
	ASSERT_EQUAL(true, (SCV(nullterm) + SCV()).Str() == "test");
	ASSERT_EQUAL(true, (SCV() + SCV(nullterm)).Str() == "test");
	ASSERT_EQUAL(true, (SCV(nullterm) + SCV(nullterm)).Str() == "testtest");
}
#endif

} // ui
