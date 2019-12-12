
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <typeinfo>
#include <new>


struct UIEvent;
struct UIElement; // physical item
struct UINode; // logical item
struct UIContext;


size_t numAllocs = 0, numNew = 0, numDelete = 0;
void* operator new(size_t s)
{
	numAllocs++;
	numNew++;
	return malloc(s);
}
void operator delete(void* p)
{
	numAllocs--;
	numDelete++;
	free(p);
}


enum class UIEventType
{
	Click,
	Change,
};

struct UIEvent
{
	UIContext* context;
	UIElement* targetElement;
	UINode* targetNode;
	UIElement* currentElement;
	UINode* currentNode;

	UIEventType type;

	// TODO data

	bool handled;
};


struct UIElement
{
	virtual ~UIElement() {}
	virtual void Reset() {}

	void dump() { printf("    [=%p ]=%p ^=%p <=%p >=%p\n", firstChild, lastChild, parent, prev, next); fflush(stdout); }

	UIElement* firstChild = nullptr;
	UIElement* lastChild = nullptr;
	UIElement* parent = nullptr;
	UIElement* prev = nullptr;
	UIElement* next = nullptr;
	UINode* ownerNode = nullptr;
};

struct UIRootElement : UIElement
{
};

struct UITextElement : UIElement
{
	void SetText(const char*)
	{
	}
};

struct UIBoxElement : UIElement
{
};


struct UINode
{
	virtual ~UINode() {}
	virtual void Render(UIContext* ctx) = 0;
	virtual void OnEvent(UIEvent& e) {}

	UINode* firstChild = nullptr;
	UINode* lastChild = nullptr;
	UINode* parent = nullptr;
	UINode* prev = nullptr;
	UINode* next = nullptr;
	UIElement* container = nullptr;
};

template<class T> void Node_AddChild(T* node, T* ch)
{
	assert(ch->parent == nullptr);
	ch->parent = node;
	if (!node->firstChild)
	{
		node->firstChild = node->lastChild = ch;
	}
	else
	{
		ch->prev = node->lastChild;
		node->lastChild->next = ch;
		node->lastChild = ch;
	}
}

struct UIContext
{
	void Free()
	{
		if (rootElement)
		{
			elementStack[0] = rootElement;
			elementStackSize = 1;
			ProcessElementDeleteStack();
			rootElement = nullptr;
		}
		if (rootNode)
		{
			nodeStack[0] = rootNode;
			nodeStackSize = 1;
			ProcessNodeDeleteStack();
			rootNode = nullptr;
		}
	}
	void ProcessNodeDeleteStack()
	{
		while (nodeStackSize > 0)
		{
			auto* cur = nodeStack[--nodeStackSize];

			for (auto* n = cur->firstChild; n != nullptr; n = n->next)
				nodeStack[nodeStackSize++] = n;

			delete cur;
		}
	}
	void ProcessElementDeleteStack(int first = 0)
	{
		while (elementStackSize > first)
		{
			auto* cur = elementStack[--elementStackSize];

			for (auto* n = cur->firstChild; n != nullptr; n = n->next)
				elementStack[elementStackSize++] = n;

			printf("--- DELETING %p\n", cur);
			delete cur;
		}
	}
	void DeleteElementsStartingFrom(UIElement* el)
	{
		int first = elementStackSize;
		for (auto* e = el; e; e = e->next)
			elementStack[elementStackSize++] = e;
		ProcessElementDeleteStack(first);
	}
	template<class T> T* AllocIfDifferent(UINode* node)
	{
		if (node && typeid(*node) == typeid(T))
			return static_cast<T*>(node);
		return new T();
	}
	template<class T> T* AllocIfDifferent(UIElement* el)
	{
		if (el && typeid(*el) == typeid(T))
		{
			el->Reset();
			return static_cast<T*>(el);
		}
		return new T();
	}
	template<class T> T* Make()
	{
		T* out = new T;
		out->container = elementStack[elementStackSize - 1];
		Node_AddChild<UINode>(currentNode, out);
		nodeStack[nodeStackSize++] = out;
		assert(nodeStackSize < 128);
		return out;
	}
	template<class T> void Build()
	{
		if (!rootElement)
			rootElement = new UIRootElement();
		rootNode = AllocIfDifferent<T>(rootNode);
		rootNode->container = rootElement;
		nodeStack[0] = rootNode;
		nodeStackSize = 1;
		ProcessNodeRenderStack();
	}
	void ProcessNodeRenderStack()
	{
		while (nodeStackSize > 0)
		{
			// find beginning of nodes with the same container
			int origSize = nodeStackSize;
			int start = nodeStackSize - 1;
			auto* cont = nodeStack[start]->container;
			while (start > 0 && nodeStack[start - 1]->container == cont)
				start--;

			elementStack[0] = cont;
			elemChildStack[0] = cont->firstChild;
			elementStackSize = 1;

			for (int i = start; i < nodeStackSize; i++)
			{
				currentNode = nodeStack[i];
				printf("rendering %s\n", typeid(*currentNode).name());
				currentNode->Render(this);

				if (elementStackSize > 1)
				{
					printf("WARNING: elements not popped: %d\n", elementStackSize);
					while (elementStackSize > 1)
					{
						Pop();
					}
				}
			}

			elementStackSize--; // root

			// cut out the processed nodes (TODO)
			memcpy(&nodeStack[start], &nodeStack[origSize], sizeof(UINode*) * (nodeStackSize - origSize));
			nodeStackSize -= origSize - start;
		}
		currentNode = nullptr;
	}

	void Pop()
	{
		assert(elementStackSize > 1);
		if (elemChildStack[elementStackSize - 1])
		{
			// remove leftover children
			DeleteElementsStartingFrom(elemChildStack[elementStackSize - 1]);
		}
		elementStackSize--;
		printf("  pop [%d] %s\n", elementStackSize, typeid(*elementStack[elementStackSize]).name());
	}
	template<class T> T* Push()
	{
		printf("%p\n", elemChildStack[elementStackSize - 1]);
		if (elemChildStack[elementStackSize - 1])
			printf("%s\n", typeid(*elemChildStack[elementStackSize - 1]).name());
		T* el = AllocIfDifferent<T>(elemChildStack[elementStackSize - 1]);
		printf("  push [%d] %s\n", elementStackSize, typeid(*el).name());
		if (el == elemChildStack[elementStackSize - 1])
		{
			// continue the match streak
			puts("/// match streak ///");
			elemChildStack[elementStackSize - 1] = el->next;
		}
		else
		{
			elementStack[elementStackSize - 1]->dump();
			if (elemChildStack[elementStackSize - 1])
			{
				// delete all nodes starting from this child, the match streak is gone
				elementStack[elementStackSize - 1]->lastChild = elemChildStack[elementStackSize - 1]->prev;
				DeleteElementsStartingFrom(elemChildStack[elementStackSize - 1]);
			}
			puts(">>> new element >>>");
			Node_AddChild<UIElement>(elementStack[elementStackSize - 1], el);
			elemChildStack[elementStackSize - 1] = nullptr;
		}
		el->ownerNode = currentNode;
		elementStack[elementStackSize] = el;
		elemChildStack[elementStackSize] = el->firstChild;
		elementStackSize++;
		return el;
	}
	UIBoxElement* PushBox() { return Push<UIBoxElement>(); }
	UITextElement* Text(const char* s)
	{
		auto* T = Push<UITextElement>();
		T->SetText(s);
		Pop();
		return T;
	}

	void OnChange(UINode* n)
	{
	}

	UINode* rootNode = nullptr;
	UIRootElement* rootElement = nullptr;
	int debugpad1 = 0;
	UIElement* elementStack[128];
	UIElement* elemChildStack[128];
	int debugpad2 = 0;
	UINode* nodeStack[128];
	UINode* currentNode;
	int elementStackSize = 0;
	int nodeStackSize = 0;
};


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
		if (e.type == UIEventType::Click && e.targetNode == this)
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
		if (e.type == UIEventType::Click && e.targetNode == this)
		{
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
		cb->SetValue(open);

		openBtn = ctx->Make<Button>();
		openBtn->SetText("Open");

		closeBtn = ctx->Make<Button>();
		closeBtn->SetText("Close");

		ctx->Pop();
	}
	void OnEvent(UIEvent& e) override
	{
		if (e.type == UIEventType::Click)
		{
			if (e.targetNode == openBtn)
				open = true;
			if (e.targetNode == closeBtn)
				open = false;
		}
		if (e.type == UIEventType::Change)
		{
			if (e.targetNode == cb)
				open = cb->GetValue();
		}
	}

	bool open = false;

	Checkbox* cb;
	Button* openBtn;
	Button* closeBtn;
};


void dumpallocinfo()
{
	printf("- allocs:%u new:%u delete:%u\n", (unsigned) numAllocs, (unsigned) numNew, (unsigned) numDelete);
}

int main()
{
	UIContext ctx;
	ctx.Build<OpenClose>();
	dumpallocinfo();
	ctx.Build<OpenClose>();
	dumpallocinfo();
	ctx.Free();
	printf("end:\n");
	dumpallocinfo();
	return 0;
}

