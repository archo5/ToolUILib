
#pragma once

#include "Platform.h"
#include "HashUtils.h"

#include <assert.h>
#include <stddef.h>


namespace ui {

template <class K, class V, class HEC = HashEqualityComparer<K>>
struct HashMap
{
	using H = size_t;
	using Hash = H;
	using Key = K;
	using Value = V;
	static constexpr size_t NO_VALUE = SIZE_MAX;
	static constexpr size_t REMOVED = SIZE_MAX - 1;

	struct EntryRef
	{
		K& key;
		V& value;

		EntryRef* operator -> () { return this; }
		const EntryRef* operator -> () const { return this; }
	};
	struct ConstIterator
	{
		const HashMap* _h;
		size_t _pos;

		bool operator == (const ConstIterator& o) const { return _pos == o._pos; }
		bool operator != (const ConstIterator& o) const { return _pos != o._pos; }
		ConstIterator& operator ++ () { _pos++; return *this; }
		EntryRef operator * () const { return { _h->_keys[_pos], _h->_values[_pos] }; }
		EntryRef operator -> () const { return { _h->_keys[_pos], _h->_values[_pos] }; }
		bool is_valid() const { return _pos < _h->_count; }
	};
	struct Iterator
	{
		HashMap* _h;
		size_t _pos;

		bool operator == (const Iterator& o) const { return _pos == o._pos; }
		bool operator != (const Iterator& o) const { return _pos != o._pos; }
		Iterator& operator ++ () { _pos++; return *this; }
		EntryRef operator * () const { return { _h->_keys[_pos], _h->_values[_pos] }; }
		EntryRef operator -> () const { return { _h->_keys[_pos], _h->_values[_pos] }; }
		bool is_valid() const { return _pos < _h->_count; }
	};

	size_t* _hashTable = nullptr;
	size_t _hashCap = 0;
	H* _hashes = nullptr;
	K* _keys = nullptr;
	V* _values = nullptr;
	size_t _count = 0;
	size_t _capacity = 0;
	size_t _removed = 0;

	HashMap(size_t capacity = 0) : _capacity(capacity), _hashCap(capacity + capacity / 4)
	{
		Reserve(capacity);
	}
	HashMap(const HashMap& o)
	{
		Reserve(o._count);
		for (EntryRef e : o)
			insert(e.key, e.value);
	}
	HashMap(HashMap&& o)
	{
		_move_from(o);
	}
	HashMap& operator = (const HashMap& o)
	{
		if (this == &o)
			return *this;
		Clear();
		Reserve(o._count);
		for (EntryRef e : o)
			insert(e.key, e.value);
		return *this;
	}
	HashMap& operator = (HashMap&& o)
	{
		if (this == &o)
			return *this;
		_destruct_free();
		_move_from(o);
		return *this;
	}
	~HashMap()
	{
		_destruct_free();
	}

	UI_FORCEINLINE bool empty() const { return _count == 0; }
	UI_FORCEINLINE size_t size() const { return _count; }
	UI_FORCEINLINE size_t capacity() const { return _capacity; }
	UI_FORCEINLINE Iterator begin() { return { this, 0 }; }
	UI_FORCEINLINE Iterator end() { return { this, _count }; }
	UI_FORCEINLINE ConstIterator begin() const { return { this, 0 }; }
	UI_FORCEINLINE ConstIterator end() const { return { this, _count }; }

	UI_FORCEINLINE bool IsEmpty() const { return _count == 0; }
	UI_FORCEINLINE size_t Size() const { return _count; }
	UI_FORCEINLINE size_t Capacity() const { return _capacity; }

	void _move_from(HashMap& o)
	{
		_hashTable = o._hashTable;
		_hashCap = o._hashCap;
		_hashes = o._hashes;
		_keys = o._keys;
		_values = o._values;
		_count = o._count;
		_capacity = o._capacity;

		o._hashTable = nullptr;
		o._hashCap = 0;
		o._hashes = nullptr;
		o._keys = nullptr;
		o._values = nullptr;
		o._count = 0;
		o._capacity = 0;
	}
	void dealloc()
	{
		_destruct_free();
		_hashTable = nullptr;
		_hashCap = 0;
		_hashes = nullptr;
		_keys = nullptr;
		_values = nullptr;
		_count = 0;
		_capacity = 0;
	}

	void _destruct_free()
	{
		_destruct_all();
		free(_hashTable);
		free(_hashes);
		free(_keys);
		free(_values);
	}

	UI_FORCEINLINE void clear() { Clear(); }
	void Clear()
	{
		_destruct_all();
		for (size_t i = 0; i < _hashCap; i++)
			_hashTable[i] = NO_VALUE;
		_count = 0;
	}

	void _destruct_all()
	{
		if (!std::is_trivially_destructible_v<K>)
		{
			for (size_t i = 0; i < _count; i++)
				_keys[i].~K();
		}
		if (!std::is_trivially_destructible_v<V>)
		{
			for (size_t i = 0; i < _count; i++)
				_values[i].~V();
		}
	}

	void _rehash(size_t hashCap)
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
		for (size_t n = 0; n < _count; n++)
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
	UI_FORCEINLINE void reserve(size_t capacity) { Reserve(capacity); }
	void Reserve(size_t capacity)
	{
		if (capacity <= _capacity)
			return;
		if (capacity < 32)
			capacity = 32;
		size_t hashCap = _next_po2(capacity + capacity / 4);
		_rehash(hashCap);
		_hashes = (H*)realloc(_hashes, sizeof(H) * capacity);
		if (std::is_standard_layout_v<K> && std::is_trivially_copy_constructible_v<K> &&
			std::is_standard_layout_v<V> && std::is_trivially_copy_constructible_v<V>)
		{
			_keys = (K*)realloc(_keys, sizeof(K) * capacity);
			_values = (V*)realloc(_values, sizeof(V) * capacity);
		}
		else
		{
			K* keys = (K*)malloc(sizeof(K) * capacity);
			V* values = (V*)malloc(sizeof(V) * capacity);
			for (size_t i = 0; i < _count; i++)
			{
				new (&keys[i]) K(std::move(_keys[i]));
				_keys[i].~K();
				new (&values[i]) V(std::move(_values[i]));
				_values[i].~V();
			}
			free(_keys);
			free(_values);
			_keys = keys;
			_values = values;
		}
		_capacity = capacity;
	}

	UI_FORCEINLINE size_t _advance(size_t pos) const
	{
		return (pos + 1) & (_hashCap - 1);
	}
	UI_FORCEINLINE size_t _find_htidx(const K& key) const
	{
		if (!_count)
			return SIZE_MAX;
		H hash = HEC::GetHash(key);
		size_t start = hash & (_hashCap - 1);
		size_t i = start;
		for (;;)
		{
			size_t idx = _hashTable[i];
			if (idx == NO_VALUE)
				return SIZE_MAX;
			if (idx != REMOVED && HEC::AreEqual(key, _keys[idx]))
				return i;
			i = _advance(i);
			if (i == start)
				break;
		}
		return SIZE_MAX;
	}
	UI_FORCEINLINE size_t _find_pos(const K& key, size_t def) const
	{
		size_t i = _find_htidx(key);
		return i != SIZE_MAX ? _hashTable[i] : def;
	}
	size_t _insert_pos(const K& key, H hash, size_t def)
	{
		size_t start = hash & (_hashCap - 1);
		size_t i = start;
		for (;;)
		{
			size_t idx = _hashTable[i];
			if (idx == NO_VALUE)
			{
				_hashTable[i] = def;
				return def;
			}
			if (idx != REMOVED && HEC::AreEqual(key, _keys[idx]))
				return idx;
			i = _advance(i);

			// error
			assert(i != start);
			if (i == start)
				return 0;
		}
	}
	UI_FORCEINLINE Iterator find(const K& key) { return { this, _find_pos(key, _count) }; }
	UI_FORCEINLINE Iterator Find(const K& key) { return { this, _find_pos(key, _count) }; }
	UI_FORCEINLINE ConstIterator find(const K& key) const { return { this, _find_pos(key, _count) }; }
	UI_FORCEINLINE ConstIterator Find(const K& key) const { return { this, _find_pos(key, _count) }; }

	UI_FORCEINLINE V get(const K& key, const V& def = {}) const { return GetValueOrDefault(key, def); }
	UI_FORCEINLINE V GetValueOrDefault(const K& key, const V& def = {}) const
	{
		auto pos = _find_pos(key, SIZE_MAX);
		return pos != SIZE_MAX ? _values[pos] : def;
	}

	UI_FORCEINLINE const V* GetValuePtr(const K& key) const { return const_cast<HashMap*>(this)->GetValuePtr(key); }
	UI_FORCEINLINE V* GetValuePtr(const K& key)
	{
		auto pos = _find_pos(key, SIZE_MAX);
		return pos == SIZE_MAX ? nullptr : &_values[pos];
	}

	UI_FORCEINLINE bool Contains(const K& key) const { return _find_pos(key, SIZE_MAX) != SIZE_MAX; }

	struct InsertResult
	{
		size_t pos;
		bool inserted;
	};
	InsertResult _insert_alloc(const K& key)
	{
		if (_count == _capacity)
		{
			reserve(_capacity * 2 + 1);
		}

		H hash = HEC::GetHash(key);
		size_t pos = _insert_pos(key, hash, _count);
		bool inserted = pos == _count;
		if (pos == _count)
		{
			new (&_keys[pos]) K(key);
			_hashes[pos] = hash;
			_count++;
		}
		return { pos, inserted };
	}
	UI_FORCEINLINE Iterator insert(const K& key, const V& value, bool* inserted = nullptr) { return Insert(key, value, inserted); }
	Iterator Insert(const K& key, const V& value, bool* inserted = nullptr)
	{
		InsertResult res = _insert_alloc(key);
		if (!res.inserted)
			_values[res.pos].~V();
		new (&_values[res.pos]) V(value);
		if (inserted)
			*inserted = res.inserted;
		return { this, res.pos };
	}

	V& operator [] (const K& key)
	{
		InsertResult res = _insert_alloc(key);
		if (res.inserted)
			new (&_values[res.pos]) V;
		return _values[res.pos];
	}

	UI_FORCEINLINE bool erase(const K& key) { return Remove(key); }
	bool Remove(const K& key)
	{
		size_t idx = _find_htidx(key);
		if (idx == SIZE_MAX)
			return false;

		size_t pos = _hashTable[idx];
		_hashTable[idx] = REMOVED;
		_count--;
		if (pos != _count)
		{
			size_t newidx = _find_htidx(_keys[_count]);
			assert(newidx < _hashCap);
			_keys[pos].~K();
			new (&_keys[pos]) K(_keys[_count]);
			_values[pos].~V();
			new (&_values[pos]) V(_values[_count]);
			_hashes[pos] = _hashes[_count];
			_hashTable[newidx] = pos;
		}
		_keys[_count].~K();
		_values[_count].~V();
		_removed++;
		if (_removed > _count / 2)
			_rehash(_hashCap);
		return true;
	}
};

} // ui
