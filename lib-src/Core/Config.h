
#pragma once

#include "Logging.h"
#include "ObjectIteration.h"
#include "String.h"


namespace ui {
namespace cfg {

extern LogCategory LOG_TWEAKABLE;

struct ITweakable;

struct ITweakableSetDatabase
{
	virtual void OnRegisterTweakable(ITweakable*) = 0;
	virtual void OnSaveTweakable(ITweakable*) = 0;
};

struct TweakableSet
{
	Array<ITweakable*> _tweakables;
	ITweakableSetDatabase* _db = nullptr;
};
#define UI_CFG_TWEAKABLE_SET_DECLARE(name) ::ui::cfg::TweakableSet& name();
#define UI_CFG_TWEAKABLE_SET_DEFINE(name) ::ui::cfg::TweakableSet& name() { static ::ui::cfg::TweakableSet name##_inst; return name##_inst; }

struct ITweakable
{
	StringView _type;
	StringView _subType;
	TweakableSet* _set = nullptr;
	u32 _pos = UINT32_MAX;
	bool _dirty = false;

	ITweakable(TweakableSet& TS, StringView typeName);
	~ITweakable();
	void Register(StringView subType = {});
	void Unregister();
	void SetDirty() { _dirty = true; }
	void Save();

	virtual void Serialize(IObjectIterator& oi) = 0;
};

template <class T>
struct Tweakable : ITweakable, T
{
	Tweakable(TweakableSet& TS) : ITweakable(TS, T::Type)
	{
		Register("");
	}
	void Serialize(IObjectIterator& oi) override
	{
		if (oi.IsUnserializer())
			static_cast<T&>(*this) = {};
		T::Serialize(oi);
	}
};

struct TweakableFilter
{
	bool dirtyOnly = false;
	bool filterByType = false;
	StringView type;
	ITweakable* specificTweakable = nullptr;

	// prevent linear fieldless construction
	UI_FORCEINLINE TweakableFilter() {}

	bool Matches(ITweakable* T) const
	{
		if (dirtyOnly && !T->_dirty)
			return false;
		if (filterByType && type != T->_type)
			return false;
		if (specificTweakable && T != specificTweakable)
			return false;
		return true;
	}
};

struct ConfigDB_JSONFile : ITweakableSetDatabase
{
	TweakableSet* _set;

	std::string filePath;

	ConfigDB_JSONFile(TweakableSet& TS);
	~ConfigDB_JSONFile();

	bool Load(const TweakableFilter& filter = {});
	bool Save();
	bool IsAnyDirty();

	// ITweakableSetDatabase
	void OnRegisterTweakable(ITweakable* T) override;
	void OnSaveTweakable(ITweakable* T) override;
};

struct ConfigDB_JSONFiles : ITweakableSetDatabase
{
	TweakableSet* _set;

	std::string basePath;
	std::string extension = ".json";

	ConfigDB_JSONFiles(TweakableSet& TS);
	~ConfigDB_JSONFiles();

	bool LoadTweakable(ITweakable* T);
	bool SaveTweakable(ITweakable* T);
	void LoadFiltered(const TweakableFilter& filter);
	void SaveFiltered(const TweakableFilter& filter);
	void LoadAll();
	void SaveAll();
	void SaveMissing(const TweakableFilter* filter = nullptr);

	// ITweakableSetDatabase
	void OnRegisterTweakable(ITweakable* T) override;
	void OnSaveTweakable(ITweakable* T) override;

	bool _LoadTweakableFrom(ITweakable* T, StringView path);
	std::string _GetTweakableFilePath(ITweakable* T, bool def);
};

} // cfg

using namespace cfg;
} // ui
