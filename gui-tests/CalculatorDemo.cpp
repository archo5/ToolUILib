
#include "pch.h"


static const char* calcOpNames[] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", ".", "+", "-", "*", "/" };
static ui::UIRect CalcBoxButton(int x, int y, int w = 1, int h = 1)
{
	return { x / 4.f, y / 5.f, (x + w) / 4.f, (y + h) / 5.f };
}
static ui::UIRect calcOpAnchors[] =
{
	// numbers
	CalcBoxButton(0, 4),
	CalcBoxButton(0, 3),
	CalcBoxButton(1, 3),
	CalcBoxButton(2, 3),
	CalcBoxButton(0, 2),
	CalcBoxButton(1, 2),
	CalcBoxButton(2, 2),
	CalcBoxButton(0, 1),
	CalcBoxButton(1, 1),
	CalcBoxButton(2, 1),

	CalcBoxButton(1, 4),

	CalcBoxButton(3, 3, 1, 2),
	CalcBoxButton(3, 2),
	CalcBoxButton(3, 1),
	CalcBoxButton(3, 0),
};
struct Calculator : ui::Buildable
{
	void Build() override
	{
		TEMP_LAYOUT_MODE = FILLER;
		auto& ple = ui::Push<ui::PlacementLayoutElement>();
		auto tmpl = ple.GetSlotTemplate();

		auto* rap_inputs = Allocate<ui::RectAnchoredPlacement>();
		rap_inputs->anchor = { 0, 0, 1, 0.1f };
		tmpl->placement = rap_inputs;
		ui::Make<ui::Textbox>().SetText(operation);

		auto* rap_result = Allocate<ui::RectAnchoredPlacement>();
		rap_result->anchor = { 0, 0.1f, 1, 0.2f };
		tmpl->placement = rap_result;
		ui::Push<ui::Panel>();
		ui::Text("=" + ToString(Calculate()));
		ui::Pop();

		auto* rap_buttons = Allocate<ui::RectAnchoredPlacement>();
		rap_buttons->anchor = { 0, 0.2f, 1, 1 };
		tmpl->placement = rap_buttons;
		auto& btnple = ui::Push<ui::PlacementLayoutElement>();
		tmpl.Revert();
		tmpl = ple.GetSlotTemplate();

		for (int i = 0; i < 15; i++)
		{
			auto* rap = Allocate<ui::RectAnchoredPlacement>();
			rap->anchor = calcOpAnchors[i];
			tmpl->placement = rap;
			if (ui::imm::Button(calcOpNames[i]))
			{
				AddChar(calcOpNames[i][0]);
			}
		}

		// =
		{
			auto* rap = Allocate<ui::RectAnchoredPlacement>();
			rap->anchor = CalcBoxButton(2, 4);
			tmpl->placement = rap;
			if (ui::imm::Button("="))
			{
				operation = ToString(Calculate());
			}
		}

		// backspace
		{
			auto* rap = Allocate<ui::RectAnchoredPlacement>();
			rap->anchor = CalcBoxButton(2, 0);
			tmpl->placement = rap;
			if (ui::imm::Button("<"))
			{
				if (!operation.empty())
					operation.pop_back();
			}
		}

		ui::Pop();
		ui::Pop();
	}

	void AddChar(char ch)
	{
		if (ch == '.')
		{
			// must be preceded by a number without a dot
			if (operation.empty())
				return;
			for (size_t i = operation.size(); i > 0; )
			{
				i--;

				if (operation[i] == '.')
					return;
				if (!isdigit(operation[i]))
					break;
			}
		}
		else if (ch == '+' || ch == '-' || ch == '*' || ch == '/')
		{
			// must be preceded by a number or a dot
			if (operation.empty())
				return;
			if (!isdigit(operation.back()) && operation.back() != '.')
				return;
		}
		operation.push_back(ch);
		Rebuild();
	}

	int precedence(char op)
	{
		if (op == '*' || op == '/')
			return 2;
		if (op == '+' || op == '-')
			return 1;
		return 0;
	}
	void Apply(std::vector<double>& numqueue, char op)
	{
		double b = numqueue.back();
		numqueue.pop_back();
		double& a = numqueue.back();
		switch (op)
		{
		case '+': a += b; break;
		case '-': a -= b; break;
		case '*': a *= b; break;
		case '/': a /= b; break;
		}
	}
	double Calculate()
	{
		std::vector<double> numqueue;
		std::vector<char> opstack;
		bool lastwasop = false;
		for (size_t i = 0; i < operation.size(); i++)
		{
			char c = operation[i];
			if (isdigit(c))
			{
				char* end = nullptr;
				numqueue.push_back(std::strtod(operation.c_str() + i, &end));
				i = end - operation.c_str() - 1;
				lastwasop = false;
			}
			else
			{
				while (!opstack.empty() && precedence(opstack.back()) > precedence(c))
				{
					Apply(numqueue, opstack.back());
					opstack.pop_back();
				}
				opstack.push_back(c);
				lastwasop = true;
			}
		}
		if (lastwasop)
			opstack.pop_back();
		while (!opstack.empty() && numqueue.size() >= 2)
		{
			Apply(numqueue, opstack.back());
			opstack.pop_back();
		}
		return !numqueue.empty() ? numqueue.back() : 0;
	}

	std::string ToString(double val)
	{
		std::string s = std::to_string(val);
		while (!s.empty() && s.back() == '0')
			s.pop_back();
		if (s.empty())
			s.push_back('0');
		else if (s.back() == '.')
			s.pop_back();
		return s;
	}

	std::string operation;
};
void Demo_Calculator()
{
	ui::Make<Calculator>();
}

