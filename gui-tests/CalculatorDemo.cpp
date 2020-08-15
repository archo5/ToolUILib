
#include "pch.h"


static const char* calcOpNames[] = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", ".", "+", "-", "*", "/" };
static UIRect CalcBoxButton(int x, int y, int w = 1, int h = 1)
{
	return { x / 4.f, y / 5.f, (x + w) / 4.f, (y + h) / 5.f };
}
static UIRect calcOpAnchors[] =
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
struct Calculator : ui::Node
{
	void Render(UIContainer* ctx) override
	{
		*this + ui::Width(style::Coord::Percent(100));
		*this + ui::Height(style::Coord::Percent(100));

		auto& inputs = ctx->Make<ui::Textbox>()->SetText(operation);
		//auto& inputs = ctx->PushBox();
		inputs + ui::MakeOverlay();
		auto* rap_inputs = Allocate<style::RectAnchoredPlacement>();
		rap_inputs->anchor = { 0, 0, 1, 0.1f };
		inputs + ui::SetPlacement(rap_inputs);
		//ctx->Pop();

		auto* rap_result = Allocate<style::RectAnchoredPlacement>();
		rap_result->anchor = { 0, 0.1f, 1, 0.2f };
		*ctx->Push<ui::Panel>() + ui::MakeOverlay() + ui::SetPlacement(rap_result);
		ctx->Text("=" + ToString(Calculate()));
		ctx->Pop();

		auto& buttons = ctx->PushBox();
		buttons + ui::MakeOverlay();
		auto* rap_buttons = Allocate<style::RectAnchoredPlacement>();
		rap_buttons->anchor = { 0, 0.2f, 1, 1 };
		buttons.GetStyle().SetPlacement(rap_buttons);

		for (int i = 0; i < 15; i++)
		{
			auto* rap = Allocate<style::RectAnchoredPlacement>();
			rap->anchor = calcOpAnchors[i];
			if (ui::imm::Button(ctx, calcOpNames[i], { ui::SetPlacement(rap) }))
			{
				AddChar(calcOpNames[i][0]);
			}
		}

		// =
		{
			auto* rap = Allocate<style::RectAnchoredPlacement>();
			rap->anchor = CalcBoxButton(2, 4);
			if (ui::imm::Button(ctx, "=", { ui::SetPlacement(rap) }))
			{
				operation = ToString(Calculate());
			}
		}

		// backspace
		{
			auto* rap = Allocate<style::RectAnchoredPlacement>();
			rap->anchor = CalcBoxButton(2, 0);
			if (ui::imm::Button(ctx, "<", { ui::SetPlacement(rap) }))
			{
				if (!operation.empty())
					operation.pop_back();
			}
		}

		ctx->Pop();
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
		Rerender();
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
void Demo_Calculator(UIContainer* ctx)
{
	ctx->Make<Calculator>();
}

