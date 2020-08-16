
#include "pch.h"

#include <deque>


struct SequenceEditorsTest : ui::Node
{
	static constexpr bool Persistent = true;

	void Render(UIContainer* ctx) override
	{
		ctx->PushBox() + ui::StackingDirection(style::StackingDirection::LeftToRight);

		if (ui::imm::Button(ctx, "Reset"))
		{
			vectordata = { 1, 2, 3, 4 };
			listdata = { 1, 2, 3, 4 };
			dequedata = { 1, 2, 3, 4 };
		}
		if (ui::imm::Button(ctx, "100 values"))
			AdjustSizeAll(100);
		if (ui::imm::Button(ctx, "1000 values"))
			AdjustSizeAll(1000);
		if (ui::imm::Button(ctx, "10000 values"))
			AdjustSizeAll(10000);

		ctx->Pop();

		ctx->PushBox() + ui::StackingDirection(style::StackingDirection::LeftToRight);

		ctx->PushBox() + ui::Width(style::Coord::Percent(25));
		ctx->Text("std::vector<int>:");
		SeqEdit(ctx, Allocate<ui::StdSequence<decltype(vectordata)>>(vectordata));
		ctx->Pop();

		ctx->PushBox() + ui::Width(style::Coord::Percent(25));
		ctx->Text("std::list<int>:");
		SeqEdit(ctx, Allocate<ui::StdSequence<decltype(listdata)>>(listdata));
		ctx->Pop();

		ctx->PushBox() + ui::Width(style::Coord::Percent(25));
		ctx->Text("std::deque<int>:");
		SeqEdit(ctx, Allocate<ui::StdSequence<decltype(dequedata)>>(dequedata));
		ctx->Pop();

		ctx->PushBox() + ui::Width(style::Coord::Percent(25));
		ctx->Text("int[5]:");
		SeqEdit(ctx, Allocate<ui::BufferSequence<int, uint8_t>>(bufdata, buflen));
		ctx->Pop();

		ctx->Pop();
	}

	void SeqEdit(UIContainer* ctx, ui::ISequence* seq)
	{
		ctx->Make<ui::SequenceEditor>()->SetSequence(seq).itemUICallback = [](UIContainer* ctx, ui::SequenceEditor* se, ui::ISequenceIterator* it)
		{
			ui::imm::PropEditInt(ctx, "\bvalue", se->GetSequence()->GetValue<int>(it));
		};
	}

	void AdjustSizeAll(size_t size)
	{
		AdjustSize(vectordata, size);
		AdjustSize(listdata, size);
		AdjustSize(dequedata, size);
	}
	template <class Cont> void AdjustSize(Cont& C, size_t size)
	{
		while (C.size() > size)
			C.pop_back();
		while (C.size() < size)
			C.push_back(C.size() + 1);
	}

	std::vector<int> vectordata{ 1, 2, 3, 4 };
	std::list<int> listdata{ 1, 2, 3, 4 };
	std::deque<int> dequedata{ 1, 2, 3, 4 };
	int bufdata[5] = { 1, 2, 3, 4, 9999 };
	uint8_t buflen = 4;
};
void Test_SequenceEditors(UIContainer* ctx)
{
	ctx->Make<SequenceEditorsTest>();
}

