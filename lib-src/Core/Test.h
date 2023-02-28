
#pragma once

#include <stdio.h>
#include <stdlib.h>


static void Impl_AssertEqual(bool xref, bool x, int line, const char* sxref, const char* sx)
{
	if (xref != x)
	{
		printf("%d: ERROR (%s exp. %s): %s is not %s\n", line, sx, sxref, x ? "true" : "false", xref ? "true" : "false");
	}
}
#define ASSERT_EQUAL(xref, x) Impl_AssertEqual(xref, x, __LINE__, #xref, #x)

static void Impl_AssertNear(float a, float b, int line, const char* sa, const char* sb)
{
	if (fabsf(float(a) - float(b)) > 0.0001f)
	{
		printf("%d: ERROR (%s near %s): %g is not near %g\n", line, sa, sb, a, b);
	}
}
#define ASSERT_NEAR(a, b) Impl_AssertNear(a, b, __LINE__, #a, #b)

struct ITestBase
{
	virtual void RunTest() = 0;
};

struct TestStorage
{
	struct Test
	{
		const char* cat;
		const char* name;
		const char* catdotname;
		ITestBase* test;
		int order;
		int num;
	};

	Test* tests = nullptr;
	size_t capTests = 0;
	size_t numTests = 0;

	static TestStorage& Get()
	{
		static TestStorage inst;
		return inst;
	}

	void Run()
	{
		for (size_t i = 0; i < numTests; i++)
		{
			Test& T = tests[i];
			T.test->RunTest();
		}
	}

	void Add(const char* cat, const char* name, const char* catdotname, ITestBase* test, int order)
	{
		if (!tests)
		{
			tests = new Test[32];
			capTests = 32;
		}
		else if (numTests == capTests)
		{
			auto* ntests = new Test[capTests * 2];
			memcpy(ntests, tests, sizeof(Test) * numTests);
			delete[] tests;
			tests = ntests;
		}
		Test& T = tests[numTests++];
		T.cat = cat;
		T.name = name;
		T.catdotname = catdotname;
		T.test = test;
		T.order = order;
		T.num = int(numTests);
	}
};

struct RegisterTest
{
	RegisterTest(const char* cat, const char* name, const char* catdotname, ITestBase* test, int order)
	{
		TestStorage::Get().Add(cat, name, catdotname, test, order);
	}
};

#define DEFINE_TEST_CATEGORY(cat, order) \
	static const int g_testcat_order_##cat = int(order);
	

#define DEFINE_TEST(cat, name) \
	struct Test_##cat##_##name : ::ui::ITestBase \
	{ \
		RegisterTest _rt { #cat, #name, #cat "." #name, this, ::ui::g_testcat_order_##cat }; \
		void RunTest() override; \
	}; \
	static Test_##cat##_##name g_test_##cat##_##name; \
	void Test_##cat##_##name::RunTest()
