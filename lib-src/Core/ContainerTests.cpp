
#include "Array.h"
#include "HashMap.h"

#include <stdarg.h>
#include <stdio.h>
#include <vector>
#include <unordered_map>
#ifdef _MSC_VER
#pragma warning(disable:4996)
#endif

namespace ui {
double hqtime();
} // ui
using namespace ui;

struct Test
{
	Test(const char* name, bool cmp, bool skip_measuring = false) : comparison(cmp), printed(skip_measuring)
	{
		if (!cmp)
		{
			char nbuf[256] = {};
			strcpy(nbuf, name);
			if (skip_measuring)
				strcat(nbuf, " -- test only");
			size_t len = strlen(nbuf);
			while (len < 60)
				nbuf[len++] = ' ';
			nbuf[len] = 0;
			printf("%s", nbuf);
		}
		start = hqtime();
	}
	void RestartMeasure()
	{
		start = hqtime();
	}
	void EndMeasure()
	{
		if (!printed)
		{
			static double prev_time_ms;
			double time_ms = (hqtime() - start) * 1000;
			printf(" %s=%.5f ms", comparison ? "std" : "custom", time_ms);
			if (!comparison)
				prev_time_ms = time_ms;
			else
			{
				printf(" as percent=%g%%", prev_time_ms / time_ms * 100);
			}
			printed = true;
		}
	}
	~Test()
	{
		EndMeasure();
	}
	double start = 0;
	bool comparison = false;
	bool printed = false;
};

#define TEST(name) Test _test(name, false)
#define TEST_ONLY(name) Test _test(name, false, true)
#define RESTART_MEASURING _test.RestartMeasure()
#define VALIDATE _test.EndMeasure()
#define END_MEASURING _test.EndMeasure()
#define END_TEST_GROUP puts("")

#define BACKSPACE10 "\x8\x8\x8\x8\x8\x8\x8\x8\x8\x8"
#define ERASE10 BACKSPACE10 "          " BACKSPACE10

static void TestError(const char* what, int line, const char* fmt, ...)
{
	printf("[ERROR] Check failed - \"%s\" (line %d)\nInfo: ", what, line);
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
	puts("");
}
#define CHECK(truth, fmt, ...) do { if (!(truth)) TestError(#truth, __LINE__, fmt, __VA_ARGS__); } while(0)
#define CHECK_INT_EQ(what, to) CHECK(int(what) == int(to), "%d", int(what))

template <int& at>
struct InstCounter
{
	InstCounter() { at++; }
	InstCounter(const InstCounter&) { at++; }
	~InstCounter() { at--; }
};
static int g_numKeys;
static int g_numVals;
struct KeyIC : InstCounter<g_numKeys>
{
	int num = 0;

	KeyIC(int i) : num(i) {}
	bool operator == (const KeyIC& o) const { return num == o.num; }
};
struct ValueIC : InstCounter<g_numVals>
{
	int num = -999999;

	ValueIC() {}
	ValueIC(int i) : num(i) {}
};
struct KeyICHasher
{
	size_t operator () (const KeyIC& k) const
	{
		return k.num;
	};
	static UI_FORCEINLINE size_t GetHash(const KeyIC& v)
	{
		return v.num;
	}
	static UI_FORCEINLINE bool AreEqual(const KeyIC& a, const KeyIC& b)
	{
		return a == b;
	}
};

using HashMapIC = HashMap<KeyIC, ValueIC, KeyICHasher>;
using unordered_map_IC = std::unordered_map<KeyIC, ValueIC, KeyICHasher>;

template <class HT>
static void check_integrity(HT& ht)
{
	assert(ht.Size() <= ht.Capacity());
	assert(ht.Size() <= ht._hashCap);
	assert(ht.Capacity() <= ht._hashCap);
	for (size_t i = 0; i < ht.Size(); i++)
	{
		size_t start = ht._hashes[i] % ht._hashCap;
		size_t j = start;
		for (;;)
		{
			size_t idx = ht._hashTable[j];
			assert(idx < ht.Size() || idx == HT::REMOVED);
			if (idx == i)
				break;
			j = ht._advance(j);
			assert(j != start);
		}
	}
}

#define TEST_CMP Test _test(" vector:", true)
static int arr0 = 0;
static void ArrayTests()
{
	// test basic functionality
	{
		// - initial state
		CHECK_INT_EQ(arr0, 0);
		Array<InstCounter<arr0>> arr;
		CHECK_INT_EQ(arr0, 0);
		CHECK_INT_EQ(arr.Size(), 0);
		CHECK_INT_EQ(arr.Capacity(), 0);
		// - append & remove all
		arr.Append({});
		CHECK_INT_EQ(arr0, 1);
		CHECK_INT_EQ(arr.Size(), 1);
		CHECK_INT_EQ(arr.Capacity(), 1);
		arr.Clear();
		CHECK_INT_EQ(arr0, 0);
		CHECK_INT_EQ(arr.Size(), 0);
		CHECK_INT_EQ(arr.Capacity(), 1);
		// - gradually append
		arr.Append({});
		CHECK_INT_EQ(arr0, 1);
		CHECK_INT_EQ(arr.Size(), 1);
		CHECK_INT_EQ(arr.Capacity(), 1);
		arr.Append({});
		CHECK_INT_EQ(arr0, 2);
		CHECK_INT_EQ(arr.Size(), 2);
		CHECK_INT_EQ(arr.Capacity(), 3);
		arr.Append({});
		CHECK_INT_EQ(arr0, 3);
		CHECK_INT_EQ(arr.Size(), 3);
		CHECK_INT_EQ(arr.Capacity(), 3);
		arr.Append({});
		CHECK_INT_EQ(arr0, 4);
		CHECK_INT_EQ(arr.Size(), 4);
		CHECK_INT_EQ(arr.Capacity(), 7);
		// - copy ctor
		{
			auto arr2(arr);
			CHECK_INT_EQ(arr0, 8);
		}
		CHECK_INT_EQ(arr0, 4);
		// - copy assign
		{
			auto arr2 = arr;
			CHECK_INT_EQ(arr0, 8);
		}
		CHECK_INT_EQ(arr0, 4);
		// - move ctor
		{
			auto arr2(std::move(arr));
			CHECK_INT_EQ(arr0, 4);
			arr = std::move(arr2);
			CHECK_INT_EQ(arr0, 4);
		}
		CHECK_INT_EQ(arr0, 4);
	}
	// - dtor
	CHECK_INT_EQ(arr0, 0);


	{TEST("ctor/dtor (int) x1000");
	for (int i = 0; i < 1000; i++)
		Array<int> v;
	}

	{TEST_CMP;
	for (int i = 0; i < 1000; i++)
		std::vector<int> v;
	}
	END_TEST_GROUP;


	for (int cap = 1; cap <= 1024; cap *= 2)
	{
		char bfr[128];
		snprintf(bfr, 128, "ctor/dtor [cap=%d] (int) x1000", cap);
		{TEST(bfr);
		for (int i = 0; i < 1000; i++)
			Array<int> v(cap);
		}

		{TEST_CMP;
		for (int i = 0; i < 1000; i++)
		{
			std::vector<int> v;
			v.reserve(cap);
		}
		}
		END_TEST_GROUP;
	}
}
#undef TEST_CMP

#define TEST_CMP Test _test(" unordered_map:", true)
static void HashMapTests()
{
	{TEST("ctor/dtor (int, int) x100");
	for (int i = 0; i < 100; i++)
		HashMap<int, int> v;
	}

	{TEST_CMP;
	for (int i = 0; i < 100; i++)
		std::unordered_map<int, int> v;
	}
	END_TEST_GROUP;


	for (int cap = 1; cap <= 1024; cap *= 2)
	{
		char bfr[128];
		snprintf(bfr, 128, "ctor/dtor [cap=%d] (int, int)", cap);
		{TEST(bfr);
		for (int i = 0; i < 100; i++)
			HashMap<int, int> v(cap);
		}

		{TEST_CMP;
		for (int i = 0; i < 100; i++)
		{
			std::unordered_map<int, int> v;
			v.reserve(cap);
		}
		}
		END_TEST_GROUP;
	}


	{TEST("find in empty (int, int) x100");
	HashMap<int, int> v;
	unsigned testval = 0;
	auto end = v.end();
	RESTART_MEASURING;
	for (int i = 0; i < 100; i++)
		testval += v.Find(i) != end;
	END_MEASURING;
	printf("%10u" ERASE10, testval);
	}

	{TEST_CMP;
	std::unordered_map<int, int> v;
	unsigned testval = 0;
	auto end = v.end();
	RESTART_MEASURING;
	for (int i = 0; i < 100; i++)
		testval += v.find(i) != end;
	END_MEASURING;
	printf("%10u" ERASE10, testval);
	}
	END_TEST_GROUP;


	{TEST_ONLY("insert (int, int) x100");
	HashMap<int, int> v;
	for (int i = 0; i < 100; i++)
	{
		bool inserted = false;
		auto it = v.Insert(i, i + 1000, &inserted);
		assert(inserted);
		assert(v.size() == i + 1);
		assert(v.Capacity() >= size_t(i + 1));
		assert(v._hashCap >= size_t(i + 1));
		check_integrity(v);
		assert(v.begin() != v.end());
		assert(!(v.begin() == v.end()));
		assert(it != v.end());
		assert(it->key == i);
		assert(it->value == i + 1000);
		auto it2 = v.Find(i);
		assert(it == it2);
		assert(!(it != it2));
		assert(it2->key == i);
		assert(it2->value == i + 1000);
		for (int j = 0; j < i; j++)
		{
			auto it3 = v.Find(j);
			assert(it3 != v.end());
			assert(it3 != it2);
			assert(it3 != it);
			assert(it3->key == j);
			assert(it3->value == j + 1000);
		}
		v.Reserve(3);
		check_integrity(v);
		inserted = true;
		v.Insert(i, i + 1000, &inserted);
		assert(!inserted);
		check_integrity(v);
	}
	int at = 0;
	for (auto e : v)
	{
		assert(e.key == at && e.value == at + 1000);
		at++;
	}
	v.Clear();
	assert(v.size() == 0);
	}
	END_TEST_GROUP;


	{TEST_ONLY("op[] insert (int, int) x100");
	HashMap<int, int> v;
	for (int i = 0; i < 100; i++)
	{
		v[i] = i + 1000;
		assert(v.size() == i + 1);
		assert(v.Capacity() >= size_t(i + 1));
		assert(v._hashCap >= size_t(i + 1));
		assert(v.begin() != v.end());
		assert(!(v.begin() == v.end()));
		auto it2 = v.Find(i);
		assert(it2->key == i);
		assert(it2->value == i + 1000);
		for (int j = 0; j < i; j++)
		{
			auto it3 = v.Find(j);
			assert(it3 != v.end());
			assert(it3 != it2);
			assert(it3->key == j);
			assert(it3->value == j + 1000);
		}
		v[i] = i + 1000;
		assert(v.size() == i + 1);
		bool inserted = true;
		v.Insert(i, i + 1000, &inserted);
		assert(!inserted);
	}
	int at = 0;
	for (auto e : v)
	{
		assert(e.key == at && e.value == at + 1000);
		at++;
	}
	}
	END_TEST_GROUP;


	{TEST("insert (int, int) x10");
	HashMap<int, int> v;
	RESTART_MEASURING;
	for (int i = 0; i < 10; i++)
		v.Insert(i, i + 1000);
	END_MEASURING;
	}

	{TEST_CMP;
	std::unordered_map<int, int> v;
	RESTART_MEASURING;
	for (int i = 0; i < 10; i++)
		v.insert({ i, i + 1000 });
	END_MEASURING;
	}
	END_TEST_GROUP;


	{TEST("insert (int, int) x100");
	HashMap<int, int> v;
	RESTART_MEASURING;
	for (int i = 0; i < 100; i++)
		v.Insert(i, i + 1000);
	END_MEASURING;
	}

	{TEST_CMP;
	std::unordered_map<int, int> v;
	RESTART_MEASURING;
	for (int i = 0; i < 100; i++)
		v.insert({ i, i + 1000 });
	END_MEASURING;
	}
	END_TEST_GROUP;


	{TEST("insert (int, int) x1000");
	HashMap<int, int> v;
	RESTART_MEASURING;
	for (int i = 0; i < 1000; i++)
		v.Insert(i, i + 1000);
	END_MEASURING;
	}

	{TEST_CMP;
	std::unordered_map<int, int> v;
	RESTART_MEASURING;
	for (int i = 0; i < 1000; i++)
		v.insert({ i, i + 1000 });
	END_MEASURING;
	}
	END_TEST_GROUP;


	{TEST("op[] insert (int, int) x100");
	HashMap<int, int> v;
	RESTART_MEASURING;
	for (int i = 0; i < 100; i++)
		v[i] = i + 1000;
	END_MEASURING;
	}

	{TEST_CMP;
	std::unordered_map<int, int> v;
	RESTART_MEASURING;
	for (int i = 0; i < 100; i++)
		v[i] = i + 1000;
	END_MEASURING;
	}
	END_TEST_GROUP;


	{TEST("insert (int, int) x100 [ins], x100 [ovr], x100 [ovr]");
	HashMap<int, int> v;
	RESTART_MEASURING;
	for (int n = 0; n < 3; n++)
		for (int i = 0; i < 100; i++)
			v.Insert(i, i + 1000);
	END_MEASURING;
	}

	{TEST_CMP;
	std::unordered_map<int, int> v;
	RESTART_MEASURING;
	for (int n = 0; n < 3; n++)
		for (int i = 0; i < 100; i++)
			v.insert({ i, i + 1000 });
	END_MEASURING;
	}
	END_TEST_GROUP;


	{TEST("contain check successfully in 10 (int, int) x1'000'000");
	HashMap<int, int> v;
	for (int i = 0; i < 10; i++)
		v.Insert(i, i + 1000);
	unsigned testval = 0;
	RESTART_MEASURING;
	for (int i = 0; i < 1000000; i++)
		testval += v.Contains(i % 10);
	END_MEASURING;
	printf("%10u" ERASE10, testval);
	}

	{TEST_CMP;
	std::unordered_map<int, int> v;
	for (int i = 0; i < 10; i++)
		v.insert({ i, i + 1000 });
	unsigned testval = 0;
	auto end = v.end();
	RESTART_MEASURING;
	for (int i = 0; i < 1000000; i++)
		testval += v.find(i % 10) != end; // ~2x faster than count() on MSVC
	END_MEASURING;
	printf("%10u" ERASE10, testval);
	}
	END_TEST_GROUP;


	{TEST("find successfully in 10 (int, int) x1'000'000");
	HashMap<int, int> v;
	for (int i = 0; i < 10; i++)
		v.Insert(i, i + 1000);
	unsigned testval = 0;
	auto end = v.end();
	RESTART_MEASURING;
	for (int i = 0; i < 1000000; i++)
		testval += v.Find(i % 10) != end;
	END_MEASURING;
	printf("%10u" ERASE10, testval);
	VALIDATE;
	for (int i = 0; i < 10; i++)
	{
		auto it = v.Find(i);
		assert(it != v.end());
		assert(it._pos == i);
		assert(it->key == i);
		assert(it->value == i + 1000);
	}
	}

	{TEST_CMP;
	std::unordered_map<int, int> v;
	for (int i = 0; i < 10; i++)
		v.insert({ i, i + 1000 });
	unsigned testval = 0;
	auto end = v.end();
	RESTART_MEASURING;
	for (int i = 0; i < 1000000; i++)
		testval += v.find(i % 10) != end;
	END_MEASURING;
	printf("%10u" ERASE10, testval);
	}
	END_TEST_GROUP;


	{TEST("contain check successfully in 100 (int, int) x1'000'000");
	HashMap<int, int> v;
	for (int i = 0; i < 100; i++)
		v.Insert(i, i + 1000);
	unsigned testval = 0;
	RESTART_MEASURING;
	for (int i = 0; i < 1000000; i++)
		testval += v.Contains(i % 100);
	END_MEASURING;
	printf("%10u" ERASE10, testval);
	}

	{TEST_CMP;
	std::unordered_map<int, int> v;
	for (int i = 0; i < 100; i++)
		v.insert({ i, i + 1000 });
	unsigned testval = 0;
	auto end = v.end();
	RESTART_MEASURING;
	for (int i = 0; i < 1000000; i++)
		testval += v.find(i % 100) != end; // ~2x faster than count() on MSVC
	END_MEASURING;
	printf("%10u" ERASE10, testval);
	}
	END_TEST_GROUP;


	{TEST("find successfully in 100 (int, int) x1'000'000");
	HashMap<int, int> v;
	for (int i = 0; i < 100; i++)
		v.Insert(i, i + 1000);
	unsigned testval = 0;
	auto end = v.end();
	RESTART_MEASURING;
	for (int i = 0; i < 1000000; i++)
		testval += v.Find(i % 100) != end;
	END_MEASURING;
	printf("%10u" ERASE10, testval);
	VALIDATE;
	for (int i = 0; i < 100; i++)
	{
		auto it = v.Find(i);
		assert(it != v.end());
		assert(it._pos == i);
		assert(it->key == i);
		assert(it->value == i + 1000);
	}
	}

	{TEST_CMP;
	std::unordered_map<int, int> v;
	for (int i = 0; i < 100; i++)
		v.insert({ i, i + 1000 });
	unsigned testval = 0;
	auto end = v.end();
	RESTART_MEASURING;
	for (int i = 0; i < 1000000; i++)
		testval += v.find(i % 100) != end;
	END_MEASURING;
	printf("%10u" ERASE10, testval);
	}
	END_TEST_GROUP;


	{TEST("contain check successfully in 1000 (int, int) x1'000'000");
	HashMap<int, int> v;
	for (int i = 0; i < 1000; i++)
		v.Insert(i, i + 1000);
	unsigned testval = 0;
	RESTART_MEASURING;
	for (int i = 0; i < 1000000; i++)
		testval += v.Contains(i % 1000);
	END_MEASURING;
	printf("%10u" ERASE10, testval);
	}

	{TEST_CMP;
	std::unordered_map<int, int> v;
	for (int i = 0; i < 1000; i++)
		v.insert({ i, i + 1000 });
	unsigned testval = 0;
	auto end = v.end();
	RESTART_MEASURING;
	for (int i = 0; i < 1000000; i++)
		testval += v.find(i % 1000) != end; // ~2x faster than count() on MSVC
	END_MEASURING;
	printf("%10u" ERASE10, testval);
	}
	END_TEST_GROUP;


	{TEST("find successfully in 1000 (int, int) x1'000'000");
	HashMap<int, int> v;
	for (int i = 0; i < 1000; i++)
		v.Insert(i, i + 1000);
	unsigned testval = 0;
	RESTART_MEASURING;
	for (int i = 0; i < 1000000; i++)
		testval += v.Find(i % 1000)->value;
	END_MEASURING;
	printf("%10u" ERASE10, testval);
	}

	{TEST_CMP;
	std::unordered_map<int, int> v;
	for (int i = 0; i < 1000; i++)
		v.insert({ i, i + 1000 });
	unsigned testval = 0;
	RESTART_MEASURING;
	for (int i = 0; i < 1000000; i++)
		testval += v.find(i % 1000)->second;
	END_MEASURING;
	printf("%10u" ERASE10, testval);
	}
	END_TEST_GROUP;


	{TEST_ONLY("insert -> erase (int, int) x100");
	HashMap<int, int> v;
	for (int i = 0; i < 100; i++)
		v.Insert(i, i + 1000);
	assert(v.size() == 100);
	check_integrity(v);
	for (int i = 0; i < 100; i++)
	{
		bool ret = v.Remove(i);
		assert(ret);
		assert(v.size() == 100 - i - 1);
		check_integrity(v);
		for (int j = i + 1; j < 100; j++)
		{
			assert(v.Contains(j));
			auto it = v.Find(j);
			assert(it.is_valid());
		}
	}
	assert(v.size() == 0);
	}
	END_TEST_GROUP;


	{TEST("insert -> erase (int, int) x100");
	HashMap<int, int> v;
	for (int i = 0; i < 100; i++)
		v.Insert(i, i + 1000);
	RESTART_MEASURING;
	for (int i = 0; i < 100; i++)
		v.Remove(i);
	END_MEASURING;
	}

	{TEST_CMP;
	std::unordered_map<int, int> v;
	for (int i = 0; i < 100; i++)
		v.insert({ i, i + 1000 });
	RESTART_MEASURING;
	for (int i = 0; i < 100; i++)
		v.erase(i);
	END_MEASURING;
	}
	END_TEST_GROUP;


	{TEST("find in empty post-erase (int, int) x100");
	HashMap<int, int> v;
	for (int i = 0; i < 100; i++)
		v.Insert(i, i + 1000);
	for (int i = 0; i < 100; i++)
		v.Remove(i);
	unsigned testval = 0;
	auto end = v.end();
	RESTART_MEASURING;
	for (int i = 0; i < 100; i++)
		testval += v.Find(i) != end;
	END_MEASURING;
	printf("%10u" ERASE10, testval);
	}

	{TEST_CMP;
	std::unordered_map<int, int> v;
	for (int i = 0; i < 100; i++)
		v.insert({ i, i + 1000 });
	for (int i = 0; i < 100; i++)
		v.erase(i);
	unsigned testval = 0;
	auto end = v.end();
	RESTART_MEASURING;
	for (int i = 0; i < 100; i++)
		testval += v.find(i) != end;
	END_MEASURING;
	printf("%10u" ERASE10, testval);
	}
	END_TEST_GROUP;




	{TEST("ctor/dtor (K/V InstCounters) x100");
	for (int i = 0; i < 100; i++)
		HashMapIC v;
	VALIDATE;
	assert(g_numKeys == 0 && g_numVals == 0);
	}

	{TEST_CMP;
	for (int i = 0; i < 100; i++)
		unordered_map_IC v;
	}
	END_TEST_GROUP;


	for (int cap = 1; cap <= 1024; cap *= 2)
	{
		char bfr[128];
		snprintf(bfr, 128, "ctor/dtor [cap=%d] (K/V InstCounters)", cap);
		{TEST(bfr);
		for (int i = 0; i < 100; i++)
			HashMapIC v(cap);
		VALIDATE;
		assert(g_numKeys == 0 && g_numVals == 0);
		}

		{TEST_CMP;
		for (int i = 0; i < 100; i++)
		{
			unordered_map_IC v;
			v.reserve(cap);
		}
		}
		END_TEST_GROUP;
	}


	{TEST("find in empty (K/V InstCounters) x100");
	HashMapIC v;
	unsigned testval = 0;
	auto end = v.end();
	RESTART_MEASURING;
	for (int i = 0; i < 100; i++)
		testval += v.Find(i) != end;
	END_MEASURING;
	printf("%10u" ERASE10, testval);
	VALIDATE;
	assert(g_numKeys == 0 && g_numVals == 0);
	}

	{TEST_CMP;
	unordered_map_IC v;
	unsigned testval = 0;
	auto end = v.end();
	RESTART_MEASURING;
	for (int i = 0; i < 100; i++)
		testval += v.find(i) != end;
	END_MEASURING;
	printf("%10u" ERASE10, testval);
	}
	END_TEST_GROUP;


	{TEST_ONLY("insert (K/V InstCounters) x100");
	HashMapIC v;
	for (int i = 0; i < 100; i++)
	{
		bool inserted = false;
		auto it = v.Insert(i, i + 1000, &inserted);
		assert(inserted);
		assert(g_numKeys == i + 1 && g_numVals == i + 1);
		assert(it->key.num == i);
		assert(it->value.num == i + 1000);
		auto it2 = v.Find(i);
		assert(it == it2);
		assert(!(it != it2));
		assert(it2->key.num == i);
		assert(it2->value.num == i + 1000);
		for (int j = 0; j < i; j++)
		{
			auto it3 = v.Find(j);
			assert(it3 != v.end());
			assert(it3->key.num == j);
			assert(it3->value.num == j + 1000);
		}
		inserted = true;
		v.Insert(i, i + 1000, &inserted);
		assert(!inserted);
		assert(g_numKeys == i + 1 && g_numVals == i + 1);
	}
	assert(g_numKeys == 100 && g_numVals == 100);
	int at = 0;
	for (auto e : v)
	{
		assert(e.key.num == at && e.value.num == at + 1000);
		at++;
	}
	v.Clear();
	assert(v.size() == 0);
	assert(g_numKeys == 0 && g_numVals == 0);
	}
	END_TEST_GROUP;


	{TEST_ONLY("op[] insert (K/V InstCounters) x100");
	HashMapIC v;
	for (int i = 0; i < 100; i++)
	{
		v[i] = i + 1000;
		assert(g_numKeys == i + 1 && g_numVals == i + 1);
		auto it2 = v.Find(i);
		assert(it2->key.num == i);
		assert(it2->value.num == i + 1000);
		for (int j = 0; j < i; j++)
		{
			auto it3 = v.Find(j);
			assert(it3 != v.end());
			assert(it3 != it2);
			assert(it3->key.num == j);
			assert(it3->value.num == j + 1000);
		}
		v[i] = i + 1000;
		assert(v.size() == i + 1);
		bool inserted = true;
		v.Insert(i, i + 1000, &inserted);
		assert(!inserted);
		assert(g_numKeys == i + 1 && g_numVals == i + 1);
	}
	int at = 0;
	for (auto e : v)
	{
		assert(e.key.num == at && e.value.num == at + 1000);
		at++;
	}
	}
	assert(g_numKeys == 0 && g_numVals == 0);
	END_TEST_GROUP;


	{TEST("insert (K/V InstCounters) x100");
	HashMapIC v;
	RESTART_MEASURING;
	for (int i = 0; i < 100; i++)
		v.Insert(i, i + 1000);
	END_MEASURING;
	}

	{TEST_CMP;
	unordered_map_IC v;
	RESTART_MEASURING;
	for (int i = 0; i < 100; i++)
		v.insert({ i, i + 1000 });
	END_MEASURING;
	}
	END_TEST_GROUP;


	{TEST("op[] insert (K/V InstCounters) x100");
	HashMapIC v;
	RESTART_MEASURING;
	for (int i = 0; i < 100; i++)
		v[i] = i + 1000;
	END_MEASURING;
	}

	{TEST_CMP;
	unordered_map_IC v;
	RESTART_MEASURING;
	for (int i = 0; i < 100; i++)
		v[i] = i + 1000;
	END_MEASURING;
	}
	END_TEST_GROUP;


	{TEST("insert (K/V InstCounters) x100 [ins], x100 [ovr], x100 [ovr]");
	HashMapIC v;
	RESTART_MEASURING;
	for (int n = 0; n < 3; n++)
		for (int i = 0; i < 100; i++)
			v.Insert(i, i + 1000);
	END_MEASURING;
	}

	{TEST_CMP;
	unordered_map_IC v;
	RESTART_MEASURING;
	for (int n = 0; n < 3; n++)
		for (int i = 0; i < 100; i++)
			v.insert({ i, i + 1000 });
	END_MEASURING;
	}
	END_TEST_GROUP;


	{TEST("find successfully in 100 (K/V InstCounters) x100'000");
	HashMapIC v;
	for (int i = 0; i < 100; i++)
		v.Insert(i, i + 1000);
	unsigned testval = 0;
	auto end = v.end();
	RESTART_MEASURING;
	for (int i = 0; i < 100000; i++)
		testval += v.Find(i % 100) != end;
	END_MEASURING;
	printf("%10u" ERASE10, testval);
	VALIDATE;
	for (int i = 0; i < 100; i++)
	{
		auto it = v.Find(i);
		assert(it != v.end());
		assert(it._pos == i);
		assert(it->key.num == i);
		assert(it->value.num == i + 1000);
	}
	}

	{TEST_CMP;
	unordered_map_IC v;
	for (int i = 0; i < 100; i++)
		v.insert({ i, i + 1000 });
	unsigned testval = 0;
	auto end = v.end();
	RESTART_MEASURING;
	for (int i = 0; i < 100000; i++)
		testval += v.find(i % 100) != end;
	END_MEASURING;
	printf("%10u" ERASE10, testval);
	}
	END_TEST_GROUP;


	{TEST_ONLY("insert -> erase (K/V InstCounters) x100");
	HashMapIC v;
	for (int i = 0; i < 100; i++)
		v.Insert(i, i + 1000);
	assert(v.size() == 100);
	assert(g_numKeys == 100 && g_numVals == 100);
	check_integrity(v);
	for (int i = 0; i < 100; i++)
	{
		bool ret = v.Remove(i);
		assert(ret);
		assert(v.size() == 100 - i - 1);
		check_integrity(v);
		for (int j = i + 1; j < 100; j++)
		{
			assert(v.Contains(j));
			auto it = v.Find(j);
			assert(it.is_valid());
		}
	}
	assert(v.size() == 0);
	assert(g_numKeys == 0 && g_numVals == 0);
	}
	END_TEST_GROUP;


	{TEST("insert -> erase (K/V InstCounters) x100");
	HashMapIC v;
	for (int i = 0; i < 100; i++)
		v.Insert(i, i + 1000);
	RESTART_MEASURING;
	for (int i = 0; i < 100; i++)
		v.Remove(i);
	END_MEASURING;
	}

	{TEST_CMP;
	unordered_map_IC v;
	for (int i = 0; i < 100; i++)
		v.insert({ i, i + 1000 });
	RESTART_MEASURING;
	for (int i = 0; i < 100; i++)
		v.erase(i);
	END_MEASURING;
	}
	END_TEST_GROUP;
}
#undef TEST_CMP

struct Init
{
	Init()
	{
		ArrayTests();
		HashMapTests();
		exit(0);
	}
};
//static Init init;

void IncludeContainerTests() {}
