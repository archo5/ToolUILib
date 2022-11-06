
#pragma once

#include "Platform.h"
#include "HashUtils.h"

#include <assert.h>
#include <stddef.h>


namespace ui {

struct HashTableBase
{
	using H = size_t;
	using Hash = H;

	static constexpr size_t NO_VALUE = SIZE_MAX;
	static constexpr size_t REMOVED = SIZE_MAX - 1;

	size_t* _hashTable = nullptr;
	size_t _hashCap = 0;
	H* _hashes = nullptr;
	size_t _removed = 0;

	UI_FORCEINLINE size_t _advance(size_t pos) const
	{
		return (pos + 1) & (_hashCap - 1);
	}

	static size_t _next_po2(size_t v)
	{
		v--;
		v |= v >> 1;
		v |= v >> 2;
		v |= v >> 4;
		v |= v >> 8;
		v |= v >> 16;
		v++;
		return v;
	}
};

template <class K, class HEC, class HTDS>
struct HashTableExtBase : HashTableBase
{
	using Storage = HTDS;

	using EntryRef = typename Storage::EntryRef;
	using Iterator = typename Storage::Iterator;
	using ConstIterator = typename Storage::ConstIterator;

	HTDS _storage;

	HashTableExtBase(size_t capacity = 0)
	{
		Reserve(capacity);
	}
	HashTableExtBase(const HashTableExtBase& o)
	{
		Reserve(o._storage.count);
		for (EntryRef e : o)
			Insert(e.key, e.value);
	}
	HashTableExtBase(HashTableExtBase&& o)
	{
		_MoveFrom(std::move(o));
	}
	HashTableExtBase& operator = (const HashTableExtBase& o)
	{
		if (this == &o)
			return *this;
		Clear();
		Reserve(o._storage.count);
		for (EntryRef e : o)
			Insert(e.key, e.value);
		return *this;
	}
	HashTableExtBase& operator = (HashTableExtBase&& o)
	{
		if (this == &o)
			return *this;
		FreeMemory();
		_MoveFrom(std::move(o));
		return *this;
	}
	~HashTableExtBase()
	{
		FreeMemory();
	}

	UI_FORCEINLINE bool empty() const { return _storage.count == 0; }
	UI_FORCEINLINE size_t size() const { return _storage.count; }
	UI_FORCEINLINE size_t capacity() const { return _storage.capacity; }
	UI_FORCEINLINE Iterator begin() { return { &_storage, 0 }; }
	UI_FORCEINLINE Iterator end() { return { &_storage, _storage.count }; }
	UI_FORCEINLINE ConstIterator begin() const { return { &_storage, 0 }; }
	UI_FORCEINLINE ConstIterator end() const { return { &_storage, _storage.count }; }

	UI_FORCEINLINE bool IsEmpty() const { return _storage.count == 0; }
	UI_FORCEINLINE size_t Size() const { return _storage.count; }
	UI_FORCEINLINE size_t Capacity() const { return _storage.capacity; }

	UI_FORCEINLINE bool Contains(const K& key) const { return _FindPos(key, SIZE_MAX) != SIZE_MAX; }

	// TODO needed?
	UI_FORCEINLINE Iterator Find(const K& key) { return { &_storage, _FindPos(key, _storage.count) }; }
	UI_FORCEINLINE ConstIterator Find(const K& key) const { return { &_storage, _FindPos(key, _storage.count) }; }

	void _MoveFrom(HashTableExtBase&& o)
	{
		_hashTable = o._hashTable;
		_hashCap = o._hashCap;
		_hashes = o._hashes;

		o._hashTable = nullptr;
		o._hashCap = 0;
		o._hashes = nullptr;

		_storage.MoveFrom(std::move(o._storage));
	}

	void FreeMemory()
	{
		_storage.DestructAll();
		_storage.FreeMemory();

		free(_hashTable);
		_hashTable = nullptr;

		free(_hashes);
		_hashes = nullptr;

		_hashCap = 0;
	}

	void Clear()
	{
		_storage.DestructAll();
		for (size_t i = 0; i < _hashCap; i++)
			_hashTable[i] = NO_VALUE;
	}

	void _Rehash(size_t hashCap)
	{
		if (hashCap != _hashCap)
		{
			// no realloc since we don't need the old data
			free(_hashTable);
			_hashTable = (size_t*)malloc(sizeof(size_t) * hashCap);
		}
		_hashCap = hashCap;

		// mark all slots as unused
		for (size_t i = 0; i < hashCap; i++)
			_hashTable[i] = NO_VALUE;

		// reinsert used elements
		for (size_t n = 0; n < _storage.count; n++)
		{
			size_t start = _hashes[n] & (hashCap - 1);
			size_t i = start;
			for (;;)
			{
				size_t idx = _hashTable[i];
				if (idx == NO_VALUE)
				{
					_hashTable[i] = n;
					break;
				}
				i = _advance(i);

				// error
				assert(i != start);
				if (i == start)
					break;
			}
		}
		_removed = 0;
	}

	void Reserve(size_t newcap)
	{
		if (newcap <= _storage.capacity)
			return;

		size_t hashCap = _next_po2(newcap + newcap / 4);
		_Rehash(hashCap);
		_hashes = (H*)realloc(_hashes, sizeof(H) * newcap);

		_storage.Reserve(newcap);
	}

	size_t _FindHashTableIndex(const K& key) const
	{
		if (!_storage.count)
			return SIZE_MAX;
		H hash = HEC::GetHash(key);
		size_t start = hash & (_hashCap - 1);
		size_t i = start;
		for (;;)
		{
			size_t idx = _hashTable[i];
			if (idx == NO_VALUE)
				return SIZE_MAX;
			if (idx != REMOVED && HEC::AreEqual(key, _storage.GetKeyAt(idx)))
				return i;
			i = _advance(i);
			if (i == start)
				break;
		}
		return SIZE_MAX;
	}
	inline size_t _FindPos(const K& key, size_t def) const
	{
		size_t i = _FindHashTableIndex(key);
		return i != SIZE_MAX ? _hashTable[i] : def;
	}

	size_t _FindInsertPos(const K& key)
	{
		if (_storage.count == _storage.capacity)
		{
			Reserve(_storage.capacity * 2 + 1);
		}

		H hash = HEC::GetHash(key);
		size_t start = hash & (_hashCap - 1);
		size_t i = start;
		for (;;)
		{
			size_t idx = _hashTable[i];
			if (idx == NO_VALUE)
			{
				size_t pos = _storage.count;
				_hashTable[i] = pos;
				_hashes[pos] = hash;
				return pos;
			}
			if (idx != REMOVED && HEC::AreEqual(key, _storage.GetKeyAt(idx)))
				return idx;
			i = _advance(i);

			// error
			assert(i != start);
			if (i == start)
				return 0;
		}
	}

	bool Remove(const K& key)
	{
		size_t htidx = _FindHashTableIndex(key);
		if (htidx == SIZE_MAX)
			return false;

		size_t pos = _hashTable[htidx];
		_hashTable[htidx] = REMOVED;
		size_t lastPos = _storage.count - 1;
		if (pos != lastPos)
		{
			size_t newidx = _FindHashTableIndex(_storage.GetKeyAt(lastPos));
			assert(newidx < _hashCap);
			_storage.MoveEntry(pos, lastPos);
			_hashes[pos] = _hashes[lastPos];
			_hashTable[newidx] = pos;
		}
		_storage.DestructEntry(lastPos);
		_storage.count--;
		_removed++;
		if (_removed > _storage.count / 2)
			_Rehash(_hashCap);
		return true;
	}
};

} // ui
