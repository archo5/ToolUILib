
#pragma once
#include <assert.h>
#include <stddef.h>
#include <type_traits>
#include <new>

template <class K, class V, class Hasher = std::hash<K>, class Equal = std::equal_to<K>>
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
	struct Iterator
	{
		const HashMap* _h;
		size_t _pos;

		bool operator == (const Iterator& o) { return _pos == o._pos; }
		bool operator != (const Iterator& o) { return _pos != o._pos; }
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
		reserve(capacity);
	}
	HashMap(const HashMap& o)
	{
		reserve(o._count);
		for (EntryRef e : o)
			insert(e.key, e.value);
	}
	HashMap(HashMap&& o)
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
	~HashMap()
	{
		_destruct_all();
		free(_hashTable);
		free(_hashes);
		free(_keys);
		free(_values);
	}

	__forceinline size_t size() const { return _count; }
	__forceinline size_t capacity() const { return _capacity; }
	__forceinline Iterator begin() const { return { this, 0 }; }
	__forceinline Iterator end() const { return { this, _count }; }

	void clear()
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
	void reserve(size_t capacity)
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

	__forceinline size_t _advance(size_t pos) const
	{
		return (pos + 1) & (_hashCap - 1);
	}
	__forceinline size_t _find_htidx(const K& key) const
	{
		if (!_count)
			return SIZE_MAX;
		Hasher hh;
		Equal eq;
		H hash = hh(key);
		size_t start = hash & (_hashCap - 1);
		size_t i = start;
		for (;;)
		{
			size_t idx = _hashTable[i];
			if (idx == NO_VALUE)
				return SIZE_MAX;
			if (idx != REMOVED && eq(key, _keys[idx]))
				return i;
			i = _advance(i);
			if (i == start)
				break;
		}
		return SIZE_MAX;
	}
	__forceinline size_t _find_pos(const K& key, size_t def) const
	{
		size_t i = _find_htidx(key);
		return i != SIZE_MAX ? _hashTable[i] : def;
	}
	size_t _insert_pos(const K& key, H hash, size_t def)
	{
		Equal eq;
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
			if (idx != REMOVED && eq(key, _keys[idx]))
				return idx;
			i = _advance(i);

			// error
			assert(i != start);
			if (i == start)
				return 0;
		}
	}
	__forceinline Iterator find(const K& key) const
	{
		return { this, _find_pos(key, _count) };
	}
	__forceinline const V& get(const K& key, const V& def = {}) const
	{
		auto pos = _find_pos(key, SIZE_MAX);
		return pos != SIZE_MAX ? _values[pos] : def;
	}
	__forceinline bool contains(const K& key) const
	{
		return _find_pos(key, SIZE_MAX) != SIZE_MAX;
	}

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

		Hasher hh;
		H hash = hh(key);
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
	Iterator insert(const K& key, const V& value, bool* inserted = nullptr)
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

	bool erase(const K& key)
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
