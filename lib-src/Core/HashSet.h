
#pragma once

#include "HashTableBase.h"
#include "ObjectIterationCore.h"


namespace ui {

template <class K>
struct HashSetDataStorage
{
	using Self = HashSetDataStorage;
	using EntryRef = const K&;

	using ConstIterator = const K*;
	using Iterator = K*;

	K* keys = nullptr;
	size_t count = 0;
	size_t capacity = 0;

	UI_FORCEINLINE Iterator Begin() { return keys; }
	UI_FORCEINLINE Iterator End() { return keys + count; }
	UI_FORCEINLINE ConstIterator Begin() const { return keys; }
	UI_FORCEINLINE ConstIterator End() const { return keys + count; }

	void DestructAll()
	{
		if (!std::is_trivially_destructible_v<K>)
		{
			for (size_t i = 0; i < count; i++)
				keys[i].~K();
		}
		count = 0;
	}

	void DestructEntry(size_t pos)
	{
		keys[pos].~K();
	}

	void MoveEntry(size_t to, size_t from)
	{
		keys[to].~K();
		new (&keys[to]) K(keys[from]);
	}

	void FreeMemory()
	{
		free(keys);
		keys = nullptr;

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
	}

	void MoveFrom(Self&& o)
	{
		keys = o.keys;
		count = o.count;
		capacity = o.capacity;

		o.keys = nullptr;
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
			free(keys);
			keys = newKeys;
		}

		capacity = cap;
	}

	void SetKey(size_t pos, const K& key)
	{
		if (pos != count)
			keys[pos].~K();
		else
			count++;
		new (&keys[pos]) K(key);
	}

	UI_FORCEINLINE K& GetKeyAt(size_t pos) { return keys[pos]; }
	UI_FORCEINLINE const K& GetKeyAt(size_t pos) const { return keys[pos]; }
};

template <class K, class HEC = HashEqualityComparer<K>>
struct HashSet : HashTableExtBase<K, HEC, HashSetDataStorage<K>>
{
	using Base = HashTableExtBase<K, HEC, HashSetDataStorage<K>>;

	using Key = K;

	using HashTableExtBase<K, HEC, HashSetDataStorage<K>>::HashTableExtBase;

	HashSet() {}
	HashSet(const std::initializer_list<K>& keys)
	{
		Base::Reserve(keys.size());
		for (const K& key : keys)
			Insert(key);
	}

	K& At(size_t pos)
	{
		assert(pos < this->_storage.count);
		return this->_storage.GetKeyAt(pos);
	}
	UI_FORCEINLINE const K& At(size_t pos) const { return const_cast<HashSet*>(this)->At(pos); }

	bool Insert(const K& key)
	{
		size_t ipos = Base::_FindInsertPos(key);
		bool inserted = ipos == this->_storage.count;
		if (inserted)
			this->_storage.SetKey(ipos, key);
		return inserted;
	}

	K FindFirstIntersecting(const HashSet& other, const K& def = {}) const
	{
		for (const K& key : *this)
			if (other.Contains(key))
				return key;
		return def;
	}

	HashSet IntersectWith(const HashSet& other) const
	{
		HashSet ret;
		for (const K& key : *this)
			if (other.Contains(key))
				ret.Insert(key);
		return ret;
	}

	void OnSerialize(IObjectIterator& oi, const FieldInfo& FI)
	{
		size_t n = oi.BeginArray(this->Size(), FI);
		if (oi.IsUnserializer())
		{
			this->Clear();
			this->Reserve(n);
			while (oi.HasMoreArrayElements())
			{
				K k{};
				OnField(oi, {}, k);
				this->Insert(k);
			}
		}
		else
		{
			for (K& k : *this)
				OnField(oi, {}, k);
		}
		oi.EndArray();
	}
};

} // ui
