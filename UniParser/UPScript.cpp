
#include <string>
#include <vector>
#include <unordered_map>
#include <inttypes.h>

#include "UPScript.h"


namespace uniparser {

static bool IsIdent(char c) { return IsAlphaNum(c) || c == '_'; }

size_t AlignUp(size_t x, size_t d) { return (x + d - 1) / d * d; }


enum OpType
{
	OP_ABORT,
	OP_CALL,
	OP_RETURN,

	OP_READ32,

	OP_DUMP_CODE_PREFIX,
	OP_DUMP_BYTES,
	OP_DUMP_S32,
	OP_DUMP_U32,
};

enum AbortReason
{
	ABORT_NONE,
	ABORT_UNRECOGNIZED_FILE,
};

enum VarType
{
	VT_UNKNOWN,
	VT_S32,
	VT_U32,
};

static size_t VarTypeToSize(VarType t)
{
	switch (t)
	{
	case VT_S32:
	case VT_U32:
		return 4;
	default:
		return 0;
	}
}

static const char* VarTypeToName(VarType t)
{
	switch (t)
	{
	case VT_S32: return "s32";
	case VT_U32: return "u32";
	default: return "<unknown>";
	}
}

struct Op
{
	OpType type;
	uint32_t arg0;
	uint32_t arg1;
};

struct Var
{
	StringView name;
	VarType type;
	uint32_t off;
};

struct Function
{
	StringView name;
	std::vector<Op> ops;
	std::vector<Var> args;
	size_t stackSize = 0;
};

struct Script
{
	std::string text;
	std::vector<std::string> strings;
	std::vector<Function> functions;
	uint32_t mainFuncID = UINT32_MAX;
};

struct StackFrame
{
	Function* F;
	size_t curInstr;
	size_t stackBase;
};

static const char NULLS[8] = {};

struct Context
{
	Script* S;
	StringView curRange;
	StringView fullRange;
	uint8_t abortReason = ABORT_NONE;

	std::vector<uint8_t> stack;
	std::vector<StackFrame> callStack;

	Context(Script* s) : S(s) {}
	void PushFunc(uint32_t fid)
	{
		auto* F = &S->functions[fid];
		if (stack.size() + F->stackSize < stack.capacity())
			stack.reserve(stack.capacity() * 2 + F->stackSize);
		callStack.push_back({ F, 0, stack.size() });
		stack.resize(stack.size() + F->stackSize);
	}
	void ExecuteOneOp()
	{
		auto& F = callStack.back();
		auto& op = F.F->ops[F.curInstr++];
		switch (op.type)
		{
		case OP_ABORT:
			abortReason = op.arg0;
			callStack.clear();
			break;

		case OP_CALL:
			PushFunc(op.arg0);
			break;

		case OP_RETURN:
			callStack.pop_back();
			break;

		case OP_READ32:
			memcpy(&stack[F.stackBase + op.arg0], curRange.size() >= 4 ? curRange.data() : NULLS, 4);
			curRange = curRange.substr(4);
			break;

		case OP_DUMP_BYTES:
			fprintf(stdout, "\n");
			for (size_t i = 0; i < (op.arg1 + 15) / 16; i++)
			{
				fprintf(stdout, "  %08X ", unsigned(i << 4));
				size_t end = std::min((i + 1) * 16, op.arg1);
				for (size_t j = i * 16; j < end; j++)
					fprintf(stdout, " %02X", stack[F.stackBase + op.arg0 + j]);
				if (op.arg1 > 16)
				{
					for (size_t j = end; j < (i + 1) * 16; j++)
						fprintf(stdout, "   ");
				}
				fprintf(stdout, "  ");
				for (size_t j = i * 16; j < end; j++)
				{
					uint8_t ch = stack[F.stackBase + op.arg0 + j];
					if (ch < 32 || ch == 127)
						ch = '.';
					fprintf(stdout, "%c", ch);
				}
				if (op.arg1 > 16)
				{
					for (size_t j = end; j < (i + 1) * 16; j++)
						fprintf(stdout, " ");
				}
				fprintf(stdout, "\n");
			}
			break;

		case OP_DUMP_CODE_PREFIX:
			fwrite(&S->text[op.arg0], 1, op.arg1, stdout);
			fprintf(stdout, " = ");
			break;

		case OP_DUMP_S32: {
			int32_t val;
			memcpy(&val, &stack[F.stackBase + op.arg0], 4);
			fprintf(stdout, "%" PRId32 " (hex=%08X)\n", val, val);
		} break;

		case OP_DUMP_U32: {
			uint32_t val;
			memcpy(&val, &stack[F.stackBase + op.arg0], 4);
			fprintf(stdout, "%" PRIu32 " (hex=%08X)\n", val, val);
		} break;
		}
	}
	void Execute(StringView data)
	{
		curRange = fullRange = data;
		stack.clear();
		PushFunc(S->mainFuncID);
		while (callStack.size())
		{
			ExecuteOneOp();
		}
	}
};


Script* Load(StringView text)
{
	auto* S = new Script;
	S->text.assign(text.data(), text.size());
	S->functions.emplace_back();
	text = S->text;

	Function* curfn = nullptr;
	std::vector<Var> stackVars;
	std::unordered_map<StringView, uint32_t> funcNameMap;
	bool argstage = false;

	bool hasErrors = false;

	auto FindVariable = [&](StringView name) -> Var
	{
		for (auto& V : stackVars)
			if (V.name == name)
				return V;

		hasErrors = true;
		fprintf(stderr, "variable not found: %.*s\n", int(name.size()), name.data());
		return { name, VT_UNKNOWN, 0 };
	};
	auto AllocVariable = [&](StringView name, VarType type, bool canExist = true) -> Var
	{
		for (auto& V : stackVars)
		{
			if (V.name == name)
			{
				if (!canExist)
				{
					hasErrors = true;
					fprintf(stderr, "variable already defined: %.*s\n", int(name.size()), name.data());
				}
				else if (type != V.type)
				{
					hasErrors = true;
					fprintf(stderr, "expected variable '%.*s' type doesn't match defined - needed %s, found %s",
						int(name.size()), name.data(), VarTypeToName(type), VarTypeToName(V.type));
				}
				return { name, type, V.off };
			}
		}
		size_t size = VarTypeToSize(type);
		curfn->stackSize = AlignUp(curfn->stackSize, size);
		size_t off = curfn->stackSize;
		curfn->stackSize += size;
		Var v = { name, type, off };
		stackVars.push_back(v);
		if (argstage)
			stackVars.push_back(v);
		return v;
	};

	while (!text.empty() && !hasErrors)
	{
		StringView line = text.take_while([](char c) { return c != '\n'; });
		text.take_char();

		line.take_while(IsSpace);
		if (line.first_char_is([](char c) { return c == '#'; }))
			continue; // comment

		StringView cmd = line.take_while(IsIdent);
		if (curfn)
		{
			if (argstage && cmd != "arg")
				argstage = false;

			if (cmd == "arg" || cmd == "var")
			{
				if (argstage || cmd == "var")
				{
					line.take_while(IsSpace);
					auto name = line.take_while(IsIdent);

					Var v = AllocVariable(name, VT_U32, false);
					if (argstage)
						curfn->args.push_back(v);
				}
				else
				{
					hasErrors = true;
					fprintf(stderr, "attempted to define arg after beginning\n");
				}
				continue;
			}
			if (cmd == "endfunc")
			{
				curfn->ops.push_back({ OP_RETURN, 0 });
				curfn->stackSize = AlignUp(curfn->stackSize, 16);
				curfn = nullptr;
				stackVars.clear();
				continue;
			}

			if (cmd == "abort")
			{
				uint32_t reason = 0;
				line.take_while(IsSpace);
				auto name = line.take_while(IsIdent);
				if (name == "unrecognized")
					reason = ABORT_UNRECOGNIZED_FILE;
				if (reason)
					curfn->ops.push_back({ OP_ABORT, reason });
				else
				{
					hasErrors = true;
					fprintf(stderr, "unknown abort reason: %.*s\n", int(name.size()), name.data());
				}
				continue;
			}
			if (cmd == "return")
			{
				curfn->ops.push_back({ OP_RETURN });
			}
			if (cmd == "call")
			{
				line.take_while(IsSpace);
				auto name = line.take_while(IsIdent);
				auto it = funcNameMap.find(name);
				if (it != funcNameMap.end())
				{
					curfn->ops.push_back({ OP_CALL, it->second });
				}
				else
				{
					hasErrors = true;
					fprintf(stderr, "function not found: %.*s\n", int(name.size()), name.data());
				}
				continue;
			}

			if (cmd == "readu32") // read 32 bits of data into a 32-bit unsigned variable
			{
				line.take_while(IsSpace);
				curfn->ops.push_back({ OP_READ32, AllocVariable(line.take_while(IsIdent), VT_U32).off });
				continue;
			}
			if (cmd == "dump") // dump the text and value of expression according to its type
			{
				line.take_while(IsSpace);
				curfn->ops.push_back({ OP_DUMP_CODE_PREFIX, uint32_t(line.data() - S->text.data()), line.size() });
				auto v = FindVariable(line.take_while(IsIdent));
				switch (v.type)
				{
				case VT_S32: curfn->ops.push_back({ OP_DUMP_S32, v.off }); break;
				case VT_U32: curfn->ops.push_back({ OP_DUMP_U32, v.off }); break;
				}
				continue;
			}
			if (cmd == "dumpb")
			{
				line.take_while(IsSpace);
				curfn->ops.push_back({ OP_DUMP_CODE_PREFIX, uint32_t(line.data() - S->text.data()), line.size() });
				auto v = FindVariable(line.take_while(IsIdent));
				curfn->ops.push_back({ OP_DUMP_BYTES, v.off, VarTypeToSize(v.type) });
				continue;
			}
		}
		else
		{
			if (cmd == "func")
			{
				line.take_while(IsSpace);
				auto name = line.take_while(IsIdent);

				funcNameMap[name] = S->functions.size();
				if (name == "main")
					S->mainFuncID = S->functions.size();
				S->functions.emplace_back();
				curfn = &S->functions.back();
				curfn->name = name;
				argstage = true;
				continue;
			}
		}

		hasErrors = true;
		fprintf(stderr, "unrecognized command: %.*s\n", int(cmd.size()), cmd.data());
	}

	if (S->mainFuncID == UINT32_MAX)
	{
		hasErrors = true;
		fprintf(stderr, "main function not defined\n");
	}

	if (hasErrors)
	{
		delete S;
		return nullptr;
	}
	return S;
}

void Free(Script* S)
{
	delete S;
}

void RunOnFile(Script* S, StringView file)
{
	Function F;

	Context ctx(S);
	ctx.Execute(file);
}

}
