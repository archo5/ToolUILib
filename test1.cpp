
#include "uicore.h"


struct Button : UINode
{
	void Render(UIContext* ctx) override
	{
		ctx->PushBox();
		ctx->Text(txt);
		ctx->Pop();
	}
	void OnEvent(UIEvent& e) override
	{
		if (e.type == UIEventType::Click)
		{
			puts("button was clicked");
		}
	}

	void SetText(const char* s)
	{
		txt = s;
	//	GetContext()->SetDirty(this); // TODO: do we want this?
	}

	const char* txt = "<todo>";
};

struct Checkbox : UINode
{
	void Render(UIContext* ctx) override
	{
		ctx->PushBox();
		ctx->Text(value ? "X" : "O");
		ctx->Pop();
	}
	void OnEvent(UIEvent& e) override
	{
		if (e.type == UIEventType::Click)
		{
			puts("checkbox was toggled");
			value = !value;
			e.context->OnChange(this);
		}
	}

	void SetValue(bool v)
	{
		value = v;
	}
	bool GetValue() const { return value; }

	bool value = false;
};

struct OpenClose : UINode
{
	void Render(UIContext* ctx) override
	{
		ctx->PushBox();

		cb = ctx->Make<Checkbox>();
		printf("open:%s\n", open ? "Y":"n");
		cb->SetValue(open);

		openBtn = ctx->Make<Button>();
		openBtn->SetText("Open");

		closeBtn = ctx->Make<Button>();
		closeBtn->SetText("Close");

		if (open)
		{
			ctx->PushBox();
			ctx->Text("It is open!");
			ctx->Pop();
		}

		ctx->Pop();
	}
	void OnEvent(UIEvent& e) override
	{
		if (e.type == UIEventType::Click)
		{
			if (e.GetTargetNode() == openBtn)
				open = true;
			if (e.GetTargetNode() == closeBtn)
				open = false;
		}
		if (e.type == UIEventType::Change)
		{
			if (e.GetTargetNode() == cb)
			{
				open = cb->GetValue();
				e.context->AddToRenderStack(this);
			}
		}
	}

	bool open = false;

	Checkbox* cb;
	Button* openBtn;
	Button* closeBtn;
};


int main()
{
	UIContext ctx;

	puts("\n---- render a full UI ----\n");
	ctx.Build<OpenClose>();
	dumpallocinfo();
	dumptree(ctx.rootNode);

	puts("\n---- render it again (and expect reuse) ----\n");
	ctx.Build<OpenClose>();
	dumpallocinfo();
	dumptree(ctx.rootNode);

	puts("\n---- paint ----\n");
	ctx.rootNode->OnPaint();

	puts("\n---- pretend a click happened (OnClick checkbox, then ProcessNodeRenderStack) ----\n");
	ctx.OnClick(ctx.rootNode->firstChild->firstChild->firstChild->firstChild);
	ctx.ProcessNodeRenderStack();
	dumpallocinfo();

	puts("\n---- paint again (changed state) ----\n");
	ctx.rootNode->OnPaint();

	puts("\n---- pretend a click happened 2 (OnClick checkbox, then ProcessNodeRenderStack) ----\n");
	ctx.OnClick(ctx.rootNode->firstChild->firstChild->firstChild->firstChild);
	ctx.ProcessNodeRenderStack();
	dumpallocinfo();

	puts("\n---- paint again 2 (changed state) ----\n");
	ctx.rootNode->OnPaint();

	puts("\n---- delete the whole UI ----\n");
	ctx.Free();
	printf("end:\n");
	dumpallocinfo();
	return 0;
}

