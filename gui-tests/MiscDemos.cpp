
#include "pch.h"
#include <set>


struct Settings
{
	int introInt = 5;
	float otherInt = 7.5f;
	bool extAdvHidden = true;
};

struct SetPropEntry
{
	ui::StringView section;
	ui::StringView group;
	ui::StringView searchStr;
	void(*editFunc)(Settings&);
};


static const SetPropEntry entries[] =
{
	{ "General", "Intro", "Integer", [](Settings& S) { ui::imm::PropEditInt("Integer", S.introInt); } },
	{ "General", "Misc", "Other", [](Settings& S) { ui::imm::PropEditFloat("Other", S.otherInt); } },
	{ "Extended", "Advanced", "Hidden", [](Settings& S) { ui::imm::PropEditBool("Hidden", S.extAdvHidden); } },
};


struct SettingsWindowDemo : ui::Buildable
{
	std::string search = "";
	std::string currentSection = "General";
	Settings settings;

	void Build() override
	{
		auto& sp = ui::Push<ui::SplitPane>();

		ui::Push<ui::ListBox>();

		ui::imm::EditString(search.c_str(), [this](const char* v) { search = v; }, { ui::TextboxPlaceholder("Search") });

		std::unordered_set<ui::StringView> sections;
		for (auto& entry : entries)
		{
			if (!SearchMatch(entry.searchStr))
				continue;
			if (sections.insert(entry.section).second)
			{
				ui::MakeWithText<ui::Selectable>(entry.section).Init(entry.section == currentSection)
					+ ui::AddEventHandler(ui::EventType::Activate, [this, &entry](ui::Event&)
				{
					currentSection.assign(entry.section.data(), entry.section.size());
					Rebuild();
				});
			}
		}

		ui::Pop();

		ui::PushBox();

		ui::StringView lastGroup;
		for (auto& entry : entries)
		{
			if (entry.section != currentSection)
				continue;
			if (!SearchMatch(entry.searchStr))
				continue;

			if (entry.group != lastGroup)
			{
				if (lastGroup.size())
					ui::Pop();

				if (entry.group.size())
				{
					ui::Text(entry.group);
					ui::Push<ui::ListBox>();
				}
				lastGroup = entry.group;
			}

			entry.editFunc(settings);
		}
		if (lastGroup.size())
			ui::Pop();

		ui::Pop();

		ui::Pop();
		sp.SetSplits({ 0.4f });
	}

	bool SearchMatch(ui::StringView s)
	{
		if (search.empty())
			return true;

		return s.find_first_at(search) != SIZE_MAX;
	}
};
void Demo_SettingsWindow()
{
	ui::Make<SettingsWindowDemo>();
}
