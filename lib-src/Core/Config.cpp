
#include "Config.h"

#include "FileSystem.h"
#include "Logging.h"
#include "SerializationJSON.h"


namespace ui {
namespace cfg {

LogCategory LOG_TWEAKABLE("Tweakable");


ITweakable::ITweakable(TweakableSet& TS, StringView typeName) : _type(typeName), _set(&TS)
{
}

ITweakable::~ITweakable()
{
	Unregister();
}

void ITweakable::Register(StringView subType)
{
	Unregister();

	_subType = subType;

	auto& RT = _set->_tweakables;
	_pos = RT.Size();
	RT.Append(this);

	if (_set->_db)
		_set->_db->OnRegisterTweakable(this);
}

void ITweakable::Unregister()
{
	if (_pos == UINT32_MAX)
		return;

	auto& RT = _set->_tweakables;
	if (RT.UnorderedRemoveAt(_pos))
		RT[_pos]->_pos = _pos;
	_pos = UINT32_MAX;
}

void ITweakable::Save()
{
	if (_set && _set->_db)
		_set->_db->OnSaveTweakable(this);
}


static void CfgFileProcessTweakable(IObjectIterator& oi, ITweakable* T)
{
	oi.BeginObject(to_string(T->_type, ":", T->_subType).c_str(), "Tweakable");

	T->Serialize(oi);

	oi.EndObject();
}

ConfigDB_JSONFile::ConfigDB_JSONFile(TweakableSet& TS) : _set(&TS)
{
	_set->_db = this;
}

ConfigDB_JSONFile::~ConfigDB_JSONFile()
{
	_set->_db = nullptr;
}

bool ConfigDB_JSONFile::Load(const TweakableFilter& filter)
{
	auto frr = ui::ReadTextFile(filePath);
	if (!frr.data)
		return false;
	ui::JSONUnserializerObjectIterator r;
	if (!r.Parse(frr.data->GetStringView()))
		return false;

	if (auto* T = filter.specificTweakable)
	{
		if (T->_set != _set)
			return false;
		CfgFileProcessTweakable(r, T);
		T->_dirty = false;
	}
	else
	{
		for (ITweakable* T : _set->_tweakables)
		{
			if (!filter.Matches(T))
				continue;
			CfgFileProcessTweakable(r, T);
			T->_dirty = false;
		}
	}
	return true;
}

bool ConfigDB_JSONFile::Save()
{
	ui::JSONSerializerObjectIterator w;

	for (ITweakable* T : _set->_tweakables)
	{
		CfgFileProcessTweakable(w, T);
	}

	if (ui::CreateMissingParentDirectories(filePath) &&
		ui::WriteTextFile(filePath, w.GetData()))
	{
		for (ITweakable* T : _set->_tweakables)
			T->_dirty = false;
		return true;
	}
	return false;
}

bool ConfigDB_JSONFile::IsAnyDirty()
{
	for (ITweakable* T : _set->_tweakables)
		if (T->_dirty)
			return true;
	return false;
}

void ConfigDB_JSONFile::OnRegisterTweakable(ITweakable* T)
{
	auto frr = ui::ReadTextFile(filePath);
	if (!frr.data)
		return;
	ui::JSONUnserializerObjectIterator r;
	if (!r.Parse(frr.data->GetStringView()))
		return;
	CfgFileProcessTweakable(r, T);
	T->_dirty = false;
}

void ConfigDB_JSONFile::OnSaveTweakable(ITweakable*)
{
	Save();
}


ConfigDB_JSONFiles::ConfigDB_JSONFiles(TweakableSet& TS) : _set(&TS)
{
	_set->_db = this;
}

ConfigDB_JSONFiles::~ConfigDB_JSONFiles()
{
	_set->_db = nullptr;
}

bool ConfigDB_JSONFiles::LoadTweakable(ITweakable* T)
{
	if (CanLogInfo(LOG_TWEAKABLE))
		LogInfo(LOG_TWEAKABLE, "loading tweakable %s:%s", to_string(T->_type).c_str(), to_string(T->_subType).c_str());

	return _LoadTweakableFrom(T, _GetTweakableFilePath(T, false))
		|| _LoadTweakableFrom(T, _GetTweakableFilePath(T, true));
}

bool ConfigDB_JSONFiles::SaveTweakable(ITweakable* T)
{
	if (CanLogInfo(LOG_TWEAKABLE))
		LogInfo(LOG_TWEAKABLE, "saving tweakable %s:%s", to_string(T->_type).c_str(), to_string(T->_subType).c_str());

	auto path = _GetTweakableFilePath(T, false);
	ui::JSONSerializerObjectIterator w;
	T->Serialize(w);
	if (ui::CreateMissingParentDirectories(path) &&
		ui::WriteTextFile(path, w.GetData()))
	{
		T->_dirty = false;
		return true;
	}
	return false;
}

void ConfigDB_JSONFiles::LoadFiltered(const TweakableFilter& filter)
{
	for (ITweakable* T : _set->_tweakables)
	{
		if (!filter.Matches(T))
			continue;
		LoadTweakable(T);
	}
}

void ConfigDB_JSONFiles::SaveFiltered(const TweakableFilter& filter)
{
	for (ITweakable* T : _set->_tweakables)
	{
		if (!filter.Matches(T))
			continue;
		SaveTweakable(T);
	}
}

void ConfigDB_JSONFiles::LoadAll()
{
	for (ITweakable* T : _set->_tweakables)
		LoadTweakable(T);
}

void ConfigDB_JSONFiles::SaveAll()
{
	for (ITweakable* T : _set->_tweakables)
		SaveTweakable(T);
}

void ConfigDB_JSONFiles::SaveMissing(const TweakableFilter* filter)
{
	for (ITweakable* T : _set->_tweakables)
	{
		if (filter && !filter->Matches(T))
			continue;
		if (ui::GetFileAttributes(_GetTweakableFilePath(T, false)) == 0)
			SaveTweakable(T);
	}
}

void ConfigDB_JSONFiles::OnRegisterTweakable(ITweakable* T)
{
	LoadTweakable(T);
}

void ConfigDB_JSONFiles::OnSaveTweakable(ITweakable* T)
{
	SaveTweakable(T);
}

bool ConfigDB_JSONFiles::_LoadTweakableFrom(ITweakable* T, StringView path)
{
	auto frr = ui::ReadTextFile(path);
	if (!frr.data)
		return false;

	ui::JSONUnserializerObjectIterator r;
	if (!r.Parse(frr.data->GetStringView()))
		return false;

	T->Serialize(r);
	T->_dirty = false;
	return true;
}

std::string ConfigDB_JSONFiles::_GetTweakableFilePath(ITweakable* T, bool def)
{
	std::string ret = basePath;
	ret += "/";
	ret += T->_type;
	if (!def && T->_subType.NotEmpty())
	{
		ret += "/";
		ret += T->_subType;
	}
	ret += extension;
	return ret;
}

} // cfg
} // ui
