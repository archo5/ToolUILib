
#include "pch.h"


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
	{ "General", "Intro", "Integer", [](Settings& S) { ui::imLabel("Integer"), ui::imEditInt(S.introInt); } },
	{ "General", "Misc", "Other", [](Settings& S) { ui::imLabel("Other"), ui::imEditFloat(S.otherInt); } },
	{ "Extended", "Advanced", "Hidden", [](Settings& S) { ui::imLabel("Hidden"), ui::imEditBool(S.extAdvHidden); } },
};


struct SettingsWindowDemo : ui::Buildable
{
	std::string search = "";
	std::string currentSection = "General";
	Settings settings;

	void Build() override
	{
		auto& sp = ui::Push<ui::SplitPane>();

		ui::Push<ui::ListBoxFrame>();
		ui::Push<ui::StackTopDownLayoutElement>();

		ui::imEditString(search).Placeholder("Search");

		ui::HashSet<ui::StringView> sections;
		for (auto& entry : entries)
		{
			if (!SearchMatch(entry.searchStr))
				continue;
			if (sections.Insert(entry.section))
			{
				ui::MakeWithText<ui::Selectable>(entry.section).Init(entry.section == currentSection)
					+ ui::AddEventHandler(ui::EventType::Activate, [this, &entry](ui::Event&)
				{
					currentSection <<= entry.section;
					Rebuild();
				});
			}
		}

		ui::Pop();
		ui::Pop();

		ui::Push<ui::StackTopDownLayoutElement>();

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
				{
					ui::Pop();
					ui::Pop();
				}

				if (entry.group.size())
				{
					ui::Text(entry.group);
					ui::Push<ui::ListBoxFrame>();
					ui::Push<ui::StackTopDownLayoutElement>();
				}
				lastGroup = entry.group;
			}

			entry.editFunc(settings);
		}
		if (lastGroup.size())
		{
			ui::Pop();
			ui::Pop();
		}

		ui::Pop();

		ui::Pop();
		sp.SetSplits({ 0.4f });
	}

	bool SearchMatch(ui::StringView s)
	{
		if (search.empty())
			return true;

		return s.FindFirstAt(search) != SIZE_MAX;
	}
};
void Demo_SettingsWindow()
{
	ui::Make<SettingsWindowDemo>();
}


struct NLRCReceiver : ui::WrapperElement
{
	ui::StackTopDownLayoutElement* layout = ui::CreateUIObject<ui::StackTopDownLayoutElement>();
	ui::LabelFrame* toplabel = ui::CreateUIObject<ui::LabelFrame>();
	ui::TextElement* toplabeltext = ui::CreateUIObject<ui::TextElement>();

	void OnReset() override
	{
		ui::WrapperElement::OnReset();
		AppendChild(layout);
		layout->AppendChild(toplabel);
		toplabel->AppendChild(toplabeltext);
		toplabeltext->SetText("Receiver content");
	}
	~NLRCReceiver()
	{
		ui::DeleteUIObject(toplabeltext);
		ui::DeleteUIObject(toplabel);
		ui::DeleteUIObject(layout);
	}
};
int testnum1 = 5;
float testnum2 = 6;
bool pullout = true;
struct NLRCContents : ui::Buildable
{
	NLRCReceiver* receiver = nullptr;
	void Build() override
	{
		WPush<ui::StackTopDownLayoutElement>();
		ui::StdText("Main content");
		ui::imm::imEditInt(testnum1);
		ui::imm::imEditBool(pullout, "Pull out the content below to a separate column");
		if (pullout)
			ui::PushPremade(receiver->layout);
		ui::StdText("Could be side content");
		ui::imm::imEditFloat(testnum2);
		if (pullout)
			ui::Pop();
		WPop();
	}
};
struct NonLinearRedirectedConstructionDemo : ui::Buildable
{
	NLRCReceiver* receiver = ui::CreateUIObject<NLRCReceiver>();
	~NonLinearRedirectedConstructionDemo()
	{
		ui::DeleteUIObject(receiver);
	}
	void Build() override
	{
		WPush<ui::StackLTRLayoutElement>();
		{
			WPush<ui::SizeConstraintElement>().SetWidth(300);
			WMake<NLRCContents>().receiver = receiver;
			WPop();
		}
		{
			WPush<ui::SizeConstraintElement>().SetWidth(300);
			ui::Add(receiver);
			WPop();
		}
		WPop();
	}
};
void Demo_NonLinearRedirectedConstruction()
{
	ui::Make<NonLinearRedirectedConstructionDemo>();
}
