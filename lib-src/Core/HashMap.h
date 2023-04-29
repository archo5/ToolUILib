
#pragma once

#include "HashTableBase.h"


namespace ui {

template <class K, class V>
struct HashTableDataStorage_SeparateArrays
{
	using Self = HashTableDataStorage_SeparateArrays;

	struct EntryRef
	{
		K& key;
		V& value;

		EntryRef* operator -> () { return this; }
		const EntryRef* operator -> () const { return this; }
	};
	struct ConstIterator
	{
		const Self* _h;
		size_t _pos;

		bool operator == (const ConstIterator& o) const { return _pos == o._pos; }
		bool operator != (const ConstIterator& o) const { return _pos != o._pos; }
		ConstIterator& operator ++ () { _pos++; return *this; }
		EntryRef operator * () const { return { _h->keys[_pos], _h->values[_pos] }; }
		EntryRef operator -> () const { return { _h->keys[_pos], _h->values[_pos] }; }
		bool is_valid() const { return _pos < _h->count; }
	};
	struct Iterator
	{
		Self* _h;
		size_t _pos;

		bool operator == (const Iterator& o) const { return _pos == o._pos; }
		bool operator != (const Iterator& o) const { return _pos != o._pos; }
		Iterator& operator ++ () { _pos++; return *this; }
		EntryRef operator * () const { return { _h->keys[_pos], _h->values[_pos] }; }
		EntryRef operator -> () const { return { _h->keys[_pos], _h->values[_pos] }; }
		bool is_valid() const { return _pos < _h->count; }
	};

	K* keys = nullptr;
	V* values = nullptr;
	size_t count = 0;
	size_t capacity = 0;

	UI_FORCEINLINE Iterator Begin() { return { this, 0 }; }
	UI_FORCEINLINE Iterator End() { return { this, count }; }
	UI_FORCEINLINE ConstIterator Begin() const { return { this, 0 }; }
	UI_FORCEINLINE ConstIterator End() const { return { this, count }; }

	void DestructAll()
	{
		if (!std::is_trivially_destructible_v<K>)
		{
			for (size_t i = 0; i < count; i++)
				keys[i].~K();
		}
		if (!std::is_trivially_destructible_v<V>)
		{
			for (size_t i = 0; i < count; i++)
				values[i].~V();
		}
		count = 0;
	}

	void DestructEntry(size_t pos)
	{
		keys[pos].~K();

		values[pos].~V();
	}

	void MoveEntry(size_t to, size_t from)
	{
		keys[to].~K();
		new (&keys[to]) K(keys[from]);

		values[to].~V();
		new (&values[to]) V(values[from]);
	}

	void FreeMemory()
	{
		if (keys)
		{
			free(keys);
			keys = nullptr;
		}

		if (values)
		{
			free(values);
			values = nullptr;
		}

		capacity = 0;
	}

	void InitCopyFrom(const Self& o)
	{
		count = o.count;

		if (std::is_standard_layout_v<K> && std::is_trivially_copy_constructible_v<K>)
		{
			memcpy(keys, o.keys, sizeof(K) * count);
		}
		else
		{
			for (size_t i = 0; i < count; i++)
				new (&keys[i]) K(o.keys[i]);
		}

		if (std::is_standard_layout_v<V> && std::is_trivially_copy_constructible_v<V>)
		{
			memcpy(values, o.values, sizeof(V) * count);
		}
		else
		{
			for (size_t i = 0; i < count; i++)
				new (&values[i]) V(o.values[i]);
		}
	}

	void MoveFrom(Self&& o)
	{
		keys = o.keys;
		values = o.values;
		count = o.count;
		capacity = o.capacity;

		o.keys = nullptr;
		o.values = nullptr;
		o.count = 0;
		o.capacity = 0;
	}

	void Reserve(size_t cap)
	{
		if (std::is_standard_layout_v<K> && std::is_trivially_copy_constructible_v<K>)
		{
			keys = (K*)realloc(keys, sizeof(K) * cap);
		}
		else
		{
			K* newKeys = (K*)malloc(sizeof(K) * cap);
			for (size_t i = 0; i < count; i++)
			{
				new (&newKeys[i]) K(Move(keys[i]));
				keys[i].~K();
			}
			if (keys)
				free(keys);
			keys = newKeys;
		}

		if (std::is_standard_layout_v<V> && std::is_trivially_copy_constructible_v<V>)
		{
			values = (V*)realloc(values, sizeof(V) * cap);
		}
		else
		{
			V* newValues = (V*)malloc(sizeof(V) * cap);
			for (size_t i = 0; i < count; i++)
			{
				new (&newValues[i]) V(Move(values[i]));
				values[i].~V();
			}
			if (values)
				free(values);
			values = newValues;
		}

		capacity = cap;
	}

	void SetKeyDefValueIfMissing(size_t pos, const K& key)
	{
		if (pos != count)
			return;
		count++;
		new (&keys[pos]) K(key);
		new (&values[pos]) V;
	}

	void SetKeyValue(size_t pos, const K& key, const V& value)
	{
		if (pos != count)
		{
			keys[pos].~K();
			values[pos].~V();
		}
		else
			count++;
		new (&keys[pos]) K(key);
		new (&values[pos]) V(value);
	}

	UI_FORCEINLINE K& GetKeyAt(size_t pos) { return keys[pos]; }
	UI_FORCEINLINE const K& GetKeyAt(size_t pos) const { return keys[pos]; }
	UI_FORCEINLINE V& GetValueAt(size_t pos) { return values[pos]; }
	UI_FORCEINLINE const V& GetValueAt(size_t pos) const { return values[pos]; }
};

template <class K, class V, class HEC = HashEqualityComparer<K>>
struct HashMap : HashTableExtBase<K, HEC, HashTableDataStorage_SeparateArrays<K, V>>
{
	using Base = HashTableExtBase<K, HEC, HashTableDataStorage_SeparateArrays<K, V>>;

	using Key = K;
	using Value = V;

	using HashTableExtBase<K, HEC, HashTableDataStorage_SeparateArrays<K, V>>::HashTableExtBase;

	UI_FORCEINLINE V GetValueOrDefault(const K& key, const V& def = {}) const
	{
		auto pos = Base::_FindPos(key, SIZE_MAX);
		return pos != SIZE_MAX ? this->_storage.GetValueAt(pos) : def;
	}

	UI_FORCEINLINE const V* GetValuePtr(const K& key) const { return const_cast<HashMap*>(this)->GetValuePtr(key); }
	UI_FORCEINLINE V* GetValuePtr(const K& key)
	{
		auto pos = Base::_FindPos(key, SIZE_MAX);
		return pos == SIZE_MAX ? nullptr : &this->_storage.GetValueAt(pos);
	}

	typename Base::Iterator Insert(const K& key, const V& value, bool* inserted = nullptr)
	{
		size_t ipos = Base::_FindInsertPos(key);
		if (inserted)
			*inserted = ipos == this->_storage.count;
		this->_storage.SetKeyValue(ipos, key, value);
		return { &this->_storage, ipos };
	}

	V& operator [] (const K& key)
	{
		size_t ipos = Base::_FindInsertPos(key);
		this->_storage.SetKeyDefValueIfMissing(ipos, key);
		return this->_storage.GetValueAt(ipos);
	}
};

} // ui
