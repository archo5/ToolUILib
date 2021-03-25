
#include "pch.h"
#include "MathExpr.h"
#include "DataDesc.h"


struct ValueNode
{
	virtual ~ValueNode() {}
	virtual int64_t Eval(IVariableSource*) const = 0;
	virtual void Dump(int level) const = 0;
	virtual std::string GenPyScript() const = 0;
};

static void DMPLEV(int level)
{
	for (int i = 0; i < level; i++)
		fprintf(stderr, "  ");
}

struct ErrorNode : ValueNode
{
	int64_t Eval(IVariableSource*) const override { return 0; }
	void Dump(int level) const override { DMPLEV(level); fprintf(stderr, "ERROR\n"); }
	std::string GenPyScript() const override { return "ERROR"; }
};

struct ConstantNode : ValueNode
{
	int64_t Eval(IVariableSource*) const override { return value; }
	void Dump(int level) const override { DMPLEV(level); fprintf(stderr, "value = %" PRId64 "\n", value); }
	std::string GenPyScript() const override { return "(" + std::to_string(value) + ")"; }

	int64_t value = 0;
};

struct UnaryOpNode : ValueNode
{
	~UnaryOpNode() { delete src; }
	virtual int64_t Do(int64_t a) const = 0;
	int64_t Eval(IVariableSource* vs) const override { return Do(src->Eval(vs)); }

	virtual const char* Name() const = 0;
	void Dump(int level) const override
	{
		DMPLEV(level);
		fprintf(stderr, "%s\n", Name());
		src->Dump(level + 1);
	}

	ValueNode* src = nullptr;
};

struct NegateNode : UnaryOpNode
{
	int64_t Do(int64_t a) const override { return -a; }
	const char* Name() const override { return "negate"; }
	std::string GenPyScript() const override { return "(-" + src->GenPyScript() + ")"; }
};

struct BitwiseInvertNode : UnaryOpNode
{
	int64_t Do(int64_t a) const override { return ~a; }
	const char* Name() const override { return "invert"; }
	std::string GenPyScript() const override { return "(~" + src->GenPyScript() + ")"; }
};

struct BinaryOpNode : ValueNode
{
	~BinaryOpNode() { delete srcA; delete srcB; }
	virtual int64_t Do(int64_t a, int64_t b) const = 0;
	int64_t Eval(IVariableSource* vs) const override { return Do(srcA->Eval(vs), srcB->Eval(vs)); }
	std::string GenPyScript() const override { return "(" + srcA->GenPyScript() + Name() + srcB->GenPyScript() + ")"; }

	virtual const char* Name() const = 0;
	void Dump(int level) const override
	{
		DMPLEV(level);
		fprintf(stderr, "%s\n", Name());
		srcA->Dump(level + 1);
		srcB->Dump(level + 1);
	}

	ValueNode* srcA = nullptr;
	ValueNode* srcB = nullptr;
};

struct AddNode : BinaryOpNode
{
	int64_t Do(int64_t a, int64_t b) const override { return a + b; }
	const char* Name() const override { return "+"; }
};

struct SubNode : BinaryOpNode
{
	int64_t Do(int64_t a, int64_t b) const override { return a - b; }
	const char* Name() const override { return "-"; }
};

struct MulNode : BinaryOpNode
{
	int64_t Do(int64_t a, int64_t b) const override { return a * b; }
	const char* Name() const override { return "*"; }
};

struct DivNode : BinaryOpNode
{
	int64_t Do(int64_t a, int64_t b) const override { return a / b; }
	const char* Name() const override { return "/"; }
};

struct ModNode : BinaryOpNode
{
	int64_t Do(int64_t a, int64_t b) const override { return a % b; }
	const char* Name() const override { return "%"; }
};

struct AndNode : BinaryOpNode
{
	int64_t Do(int64_t a, int64_t b) const override { return a & b; }
	const char* Name() const override { return "&"; }
};

struct OrNode : BinaryOpNode
{
	int64_t Do(int64_t a, int64_t b) const override { return a | b; }
	const char* Name() const override { return "|"; }
};

struct XorNode : BinaryOpNode
{
	int64_t Do(int64_t a, int64_t b) const override { return a ^ b; }
	const char* Name() const override { return "^"; }
};

struct LShiftNode : BinaryOpNode
{
	int64_t Do(int64_t a, int64_t b) const override { return a << b; }
	const char* Name() const override { return "<<"; }
};

struct RShiftNode : BinaryOpNode
{
	int64_t Do(int64_t a, int64_t b) const override { return a >> b; }
	const char* Name() const override { return ">>"; }
};

struct EqualNode : BinaryOpNode
{
	int64_t Do(int64_t a, int64_t b) const override { return a == b; }
	const char* Name() const override { return "=="; }
};

struct NotEqualNode : BinaryOpNode
{
	int64_t Do(int64_t a, int64_t b) const override { return a != b; }
	const char* Name() const override { return "!="; }
};

struct LessThanNode : BinaryOpNode
{
	int64_t Do(int64_t a, int64_t b) const override { return a < b; }
	const char* Name() const override { return "<"; }
};

struct LessEqualNode : BinaryOpNode
{
	int64_t Do(int64_t a, int64_t b) const override { return a <= b; }
	const char* Name() const override { return "<="; }
};

struct GreaterThanNode : BinaryOpNode
{
	int64_t Do(int64_t a, int64_t b) const override { return a > b; }
	const char* Name() const override { return ">"; }
};

struct GreaterEqualNode : BinaryOpNode
{
	int64_t Do(int64_t a, int64_t b) const override { return a >= b; }
	const char* Name() const override { return ">="; }
};

template <class T>
struct ReadNodeBase : ValueNode
{
	~ReadNodeBase() { delete srcOff; }
	int64_t Eval(IVariableSource* vs) const override
	{
		int64_t off = srcOff->Eval(vs);
		T val = 0;
		vs->ReadFile(off, sizeof(val), &val);
		return int64_t(val);
	}

	virtual const char* Name() const = 0;
	void Dump(int level) const override
	{
		DMPLEV(level);
		fprintf(stderr, "read %s\n", Name());
		srcOff->Dump(level + 1);
	}
	std::string GenPyScript() const override { return std::string("vs.read_file(\"") + Name() + "\", " + srcOff->GenPyScript() + ")"; }

	ValueNode* srcOff = nullptr;
};
#define DEFINE_READ_NODE(t, sn) struct ReadNode_##sn : ReadNodeBase<t> { const char* Name() const override { return #sn; } }
DEFINE_READ_NODE(int8_t, i8);
DEFINE_READ_NODE(int16_t, i16);
DEFINE_READ_NODE(int32_t, i32);
DEFINE_READ_NODE(int64_t, i64);
DEFINE_READ_NODE(uint8_t, u8);
DEFINE_READ_NODE(uint16_t, u16);
DEFINE_READ_NODE(uint32_t, u32);
DEFINE_READ_NODE(uint64_t, u64);
#undef DEFINE_READ_NODE

struct ExprCondition
{
	std::string field;
	ValueNode* expected = nullptr;

	virtual ~ExprCondition() { delete expected; }
	ExprCondition() {}
	ExprCondition(ExprCondition&& o) : field(std::move(o.field)), expected(o.expected)
	{
		o.expected = nullptr;
	}
	ExprCondition& operator = (ExprCondition&& o)
	{
		if (this != &o)
		{
			field = std::move(o.field);
			expected = o.expected;
			o.expected = nullptr;
		}
		return *this;
	}
	ExprCondition(const ExprCondition& o) = delete;
	ExprCondition& operator = (const ExprCondition& o) = delete;
};

struct StructQueryNodeFilters
{
	virtual ~StructQueryNodeFilters() { delete which; }
	StructQueryNodeFilters() {}
	StructQueryNodeFilters(StructQueryNodeFilters&& o) :
		conditions(std::move(o.conditions)),
		exprConds(std::move(o.exprConds)),
		which(o.which)
	{
		o.which = nullptr;
	}
	StructQueryNodeFilters& operator = (StructQueryNodeFilters&& o)
	{
		if (this != &o)
		{
			conditions = std::move(o.conditions);
			exprConds = std::move(o.exprConds);
			which = o.which;
			o.which = nullptr;
		}
		return *this;
	}
	StructQueryNodeFilters(const StructQueryNodeFilters& o) = delete;
	StructQueryNodeFilters& operator = (const StructQueryNodeFilters& o) = delete;

	void Dump(int level) const
	{
		for (auto& C : conditions)
		{
			fprintf(stderr, " ?\"%s\"=\"%s\"", C.field.c_str(), C.value.c_str());
		}
		for (auto& C : exprConds)
		{
			fprintf(stderr, " #\"%s\"=\n", C.field.c_str());
			C.expected->Dump(level + 1);
		}
		if (which)
		{
			fprintf(stderr, " #=\n");
			which->Dump(level + 1);
		}
		else
			fprintf(stderr, "\n");
	}
	std::string GenPyScript() const
	{
		std::string ret = "{";
		for (auto& C : conditions)
		{
			ret += '"';
			ret += C.field;
			ret += "\":\"";
			ret += C.value;
			ret += "\",";
		}
		for (auto& C : exprConds)
		{
			ret += "\"#";
			ret += C.field;
			ret += "\":";
			ret += C.expected->GenPyScript();
		}
		if (which)
		{
			ret += "\"#\":";
			ret += which->GenPyScript();
		}
		ret += '}';
		return ret;
	}

	StructQueryFilter Eval(IVariableSource* vs)
	{
		StructQueryFilter sqf = { conditions };
		sqf.returnNth = !!which;
		sqf.nth = which ? which->Eval(vs) : 0;
		sqf.intConds.reserve(exprConds.size());
		for (auto& C : exprConds)
		{
			if (!C.expected)
				continue;
			sqf.intConds.push_back({ C.field, C.expected->Eval(vs) });
		}
		return sqf;
	}

	std::vector<DDCondition> conditions;
	std::vector<ExprCondition> exprConds;
	ValueNode* which = nullptr;
};

struct StructQueryNode
{
	virtual StructQueryResults Query(IVariableSource* vs) = 0;
	virtual void Dump(int level) const = 0;
	virtual std::string GenPyScript() const = 0;

	StructQueryNodeFilters filters;
};

struct ErrorQueryNode : StructQueryNode
{
	StructQueryResults Query(IVariableSource* vs) override { return {}; }
	void Dump(int level) const override { DMPLEV(level); fprintf(stderr, "ERROR\n"); }
	virtual std::string GenPyScript() const override { return "ERROR"; }
};

struct RootQueryNode : StructQueryNode
{
	StructQueryResults Query(IVariableSource* vs) override
	{
		return typeName == "" ? vs->GetInitialSet() : vs->RootQuery(typeName, global, filters.Eval(vs));
	}
	void Dump(int level) const override
	{
		DMPLEV(level);
		fprintf(stderr, "root (struct) query \"%s\"", typeName.c_str());
		filters.Dump(level);
	}
	virtual std::string GenPyScript() const override { return "vs.root_query(\"" + typeName + "\", " + filters.GenPyScript() + ")"; }

	std::string typeName;
	bool global = false;
};

struct SubQueryNode : StructQueryNode
{
	virtual ~SubQueryNode() { delete query; }
	StructQueryResults Query(IVariableSource* vs) override
	{
		return vs->Subquery(query ? query->Query(vs) : vs->GetInitialSet(), name, filters.Eval(vs));
	}
	void Dump(int level) const override
	{
		DMPLEV(level);
		fprintf(stderr, "subfield query \"%s\"", name.c_str());
		filters.Dump(level);
		if (query)
			query->Dump(level + 1);
	}
	virtual std::string GenPyScript() const override
	{
		return "vs.subquery(" + (query ? query->GenPyScript() : "[self]") + ", \"" + name + "\", " + filters.GenPyScript() + ")";
	}

	StructQueryNode* query = nullptr;
	std::string name;
};

struct MemberFieldNode : ValueNode
{
	virtual ~MemberFieldNode() { delete query; }
	int64_t Eval(IVariableSource* vs) const override
	{
		StructQueryResults insts = query ? query->Query(vs) : vs->GetInitialSet();
		int64_t idx = index ? index->Eval(vs) : 0;
		int64_t ret = 0;
		if (insts.size() > 0 && vs->GetVariable(insts[0], name, idx, isOffset, ret))
			return ret;
		return 0;
	}
	void Dump(int level) const override
	{
		DMPLEV(level);
		fprintf(stderr, "member field%s \"%s\"%s\n", isOffset ? " offset" : "", name.c_str(), query ? " with query:" : "");
		if (query)
			query->Dump(level + 1);
	}
	std::string GenPyScript() const override
	{
		return "vs.get_variable("
			+ (query ? query->GenPyScript() : "[self]")
			+ ", \"" + name
			+ "\", " + (index ? index->GenPyScript() : "0")
			+ ", " + (isOffset ? "True" : "False") + ")";
	}

	StructQueryNode* query = nullptr;
	ValueNode* index = nullptr;
	std::string name;
	bool isOffset = false;
};

struct StructOffsetNode : ValueNode
{
	virtual ~StructOffsetNode() { delete query; }
	int64_t Eval(IVariableSource* vs) const override
	{
		StructQueryResults insts = query ? query->Query(vs) : vs->GetInitialSet();
		int64_t ret = 0;
		if (insts.size() > 0 && insts[0])
			return insts[0]->off;
		return 0;
	}
	void Dump(int level) const override
	{
		DMPLEV(level);
		fprintf(stderr, "struct offset with query:\n");
		if (query)
			query->Dump(level + 1);
	}
	std::string GenPyScript() const override { return "bdat.me_structoff(vs, " + (query ? query->GenPyScript() : "[self]") + ")"; }

	StructQueryNode* query = nullptr;
};

struct FieldPreviewEqualsStringNode : ValueNode
{
	virtual ~FieldPreviewEqualsStringNode() { delete query; }
	int64_t Eval(IVariableSource* vs) const override
	{
		StructQueryResults insts = query ? query->Query(vs) : vs->GetInitialSet();
		bool found = false;
		for (auto* inst : insts)
		{
			if (!inst)
				continue;
			size_t fid = inst->def->FindFieldByName(fieldName);
			if (fid == SIZE_MAX)
				continue;
			if ((inst->GetFieldPreview(fid) == text) ^ invert)
			{
				found = true;
				break;
			}
		}
		return found;
	}
	void Dump(int level) const override
	{
		DMPLEV(level);
		fprintf(stderr, "field '%s' preview %sequals string '%s':\n", fieldName.c_str(), invert ? "NOT " : "", text.c_str());
		if (query)
			query->Dump(level + 1);
	}
	std::string GenPyScript() const override
	{
		return "bdat.me_fpeqs(vs, " + (query ? query->GenPyScript() : "[self]") + ", \"" + fieldName + "\", b\"" + text + "\"" + (invert ? "True" : "False") + ")";
	}

	StructQueryNode* query;
	std::string fieldName;
	std::string text;
	bool invert = false;
};

struct InstanceIDNode : ValueNode
{
	virtual ~InstanceIDNode() { delete query; }
	int64_t Eval(IVariableSource* vs) const override
	{
		StructQueryResults insts = query ? query->Query(vs) : vs->GetInitialSet();
		if (insts.empty())
			return 0;
		return insts[0]->id;
	}
	void Dump(int level) const override
	{
		DMPLEV(level);
		fprintf(stderr, "instance id:\n");
		if (query)
			query->Dump(level + 1);
	}
	std::string GenPyScript() const override
	{
		return "TODO";
	}

	StructQueryNode* query;
};


struct CompiledMathExpr
{
	~CompiledMathExpr()
	{
		delete root;
	}

	ValueNode* root;
};


enum METokenType
{
	CONSTANT,
	FIELD_NAME,
	STRUCT_NAME,
	END,

	OP_ADD,
	OP_SUB,
	OP_MUL,
	OP_DIV,
	OP_MOD,

	OP_AND,
	OP_OR,
	OP_XOR,
	OP_LSH,
	OP_RSH,
	OP_INV,

	OP_EQ,
	OP_NE,
	OP_LT,
	OP_LE,
	OP_GT,
	OP_GE,

	PEXPR_START = '(',
	PEXPR_END = ')',
	SEXPR_START = '[',
	SEXPR_END = ']',
	COMMA = ',',
	OP_OFFSET = '@',
	OP_MEMBER = '.',
	OP_FNPFX = '$',
	OP_ASSIGN = '=',
};

struct Token
{
	METokenType type;
	ui::StringView text;
};

static ui::StringView operatorStrings[] =
{
	"(", ")", "[", "]", ",", "@", ".", "$",
	"+", "-", "*", "/", "%",
	"&", "|", "^", "<<", ">>", "~",
	"==", "!=", "<=", ">=", "<", ">", "=",
};
static METokenType operatorTypes[] =
{
	PEXPR_START, PEXPR_END, SEXPR_START, SEXPR_END, COMMA, OP_OFFSET, OP_MEMBER, OP_FNPFX,
	OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD,
	OP_AND, OP_OR, OP_XOR, OP_LSH, OP_RSH, OP_INV,
	OP_EQ, OP_NE, OP_LE, OP_GE, OP_LT, OP_GT, OP_ASSIGN,
};
static_assert(sizeof(operatorStrings) / sizeof(operatorStrings[0]) == sizeof(operatorTypes) / sizeof(operatorTypes[0]), "operator arrays not equal");

static bool IsNameChar(char c)
{
	return ui::IsAlphaNum(c) || c == '_' || c == '#';
}
static bool IsFirstNameChar(char c)
{
	return ui::IsAlpha(c) || c == '_' || c == '"' || c == '#';
}

struct Compiler
{
	struct Range
	{
		size_t from, to;

		bool Empty() const { return from == to; }
		size_t Length() const { return to - from; }
	};

	Token ParseOne()
	{
		it = it.ltrim();
		if (it.empty())
			return { END, it };

		for (int i = 0; i < sizeof(operatorStrings) / sizeof(operatorStrings[0]); i++)
		{
			if (it.starts_with(operatorStrings[i]))
			{
				size_t sz = operatorStrings[i].size();
				auto text = it.substr(0, sz);
				it = it.substr(sz);
				return { operatorTypes[i], text };
			}
		}

		if (it.first_char_is(ui::IsDigit))
		{
			ui::StringView num = it.take_while(ui::IsDigit);
			if (it.first_char_is([](char c) { return c == 'x'; }))
			{
				it.take_char();
				ui::StringView num2 = it.take_while(ui::IsHexDigit);
				num = ui::StringView(num.data(), num2.end() - num.data());
			}
			return { CONSTANT, num };
		}

		if (it.first_char_is(IsFirstNameChar))
		{
			bool isStruct = it.first() == '#';
			if (isStruct)
				it.take_char();
			if (it.first_char_is(IsFirstNameChar))// && it.first() != '#')
			{
				bool isQuoted = it.first() == '"';
				if (isQuoted)
					it.take_char(); // eat the first '"'
				ui::StringView name = isQuoted ? it.take_while([](char c) { return c != '"'; }) : it.take_while(IsNameChar);
				if (isQuoted && it.size())
					it.take_char(); // eat the other '"'
				if (name.empty())
				{
					puts("ERROR: BAD NAME");
				}
				else
					return { isStruct ? STRUCT_NAME : FIELD_NAME, name };
			}
			else
				return { STRUCT_NAME, it.substr(0, 0) };
		}

		puts("ERROR!");
		return { END };
	}

	std::vector<Range> SplitArgs(Range r)
	{
		if (r.from + 2 <= r.to && tokens[r.from].type == PEXPR_START && tokens[r.to - 1].type == PEXPR_END)
		{
			r.from++;
			r.to--;
		}

		std::vector<Range> ranges;
		ranges.push_back(r);

		size_t at = r.to;
		constexpr int MAX = 32;
		char exprStack[MAX] = {};
		int level = 0;
		for (size_t i = r.from; i < r.to; i++)
		{
			if (tokens[i].type == PEXPR_START || tokens[i].type == SEXPR_START)
			{
				if (level >= MAX)
				{
					puts("ERROR: TOO DEEP");
					return {};
				}
				exprStack[level++] = tokens[i].type == PEXPR_START ? PEXPR_END : SEXPR_END;
				continue;
			}
			if (tokens[i].type == PEXPR_END || tokens[i].type == SEXPR_END)
			{
				if (exprStack[--level] != tokens[i].type)
				{
					puts("ERROR: EXITS EXCEED ENTRIES");
					return {};
				}
				continue;
			}
			if (level > 0)
				continue;

			if (tokens[i].type == COMMA)
			{
				auto rb = ranges.back();
				ranges.back().to = i;
				rb.from = i + 1;
				ranges.push_back(rb);
			}
		}

		return ranges;
	}

	int GetOpScore(size_t at, Range r)
	{
		Token& T = tokens[at];
		if (T.type == OP_OR) return 4;
		if (T.type == OP_XOR) return 6;
		if (T.type == OP_AND) return 8;
		if (T.type == OP_EQ || T.type == OP_NE)
			return 10;
		if (T.type == OP_LT || T.type == OP_LE || T.type == OP_GT || T.type == OP_GE)
			return 12;
		if (T.type == OP_LSH || T.type == OP_RSH)
			return 14;
		if (T.type == OP_ADD || T.type == OP_SUB)
			return 20;
		if (T.type == OP_MUL || T.type == OP_DIV || T.type == OP_MOD)
			return 30;
		return 0;
	}

	size_t FindLastWeakestOp(Range r)
	{
		int minScore = 99;
		size_t at = r.to;
		constexpr int MAX = 32;
		char exprStack[MAX] = {};
		int level = 0;
		for (size_t i = r.from; i < r.to; i++)
		{
			if (tokens[i].type == PEXPR_START || tokens[i].type == SEXPR_START)
			{
				if (level >= MAX)
					return r.to;
				exprStack[level++] = tokens[i].type == PEXPR_START ? PEXPR_END : SEXPR_END;
				continue;
			}
			if (tokens[i].type == PEXPR_END || tokens[i].type == SEXPR_END)
			{
				if (exprStack[--level] != tokens[i].type)
					return r.to;
				continue;
			}
			if (level > 0)
				continue;
			int score = GetOpScore(i, r);
			if (!score)
				continue;
			if (score <= minScore)
			{
				minScore = score;
				at = i;
			}
		}
		return at;
	}

	ValueNode* ParseExpr(Range r)
	{
		if (r.from == r.to)
		{
			puts("ERROR: EMPTY EXPR");
			return new ErrorNode;
		}

		if (r.from + 1 == r.to)
		{
			if (tokens[r.from].type == CONSTANT)
			{
				auto* N = new ConstantNode;
				int64_t val = 0;
				ui::StringView it = tokens[r.from].text;
				if (it.starts_with("0x"))
				{
					it = it.substr(2);
					for (char c : it)
						val = val * 16 + ui::gethex(c);
				}
				else
				{
					for (char c : it)
						val = val * 10 + (c - '0');
				}
				N->value = val;
				return N;
			}
		}

		if (tokens[r.from].type == OP_FNPFX)
		{
			if (r.from + 1 == r.to)
			{
				puts("ERROR: NO FN");
				return new ErrorNode;
			}
			if (tokens[r.from + 1].type != FIELD_NAME)
			{
				puts("ERROR: NO FN NAME");
				return new ErrorNode;
			}
			auto name = tokens[r.from + 1].text;
			auto args = SplitArgs({ r.from + 2, r.to });
#define USE_READ_NODE(t) \
			if (name == #t) \
			{ \
				if (args.size() != 1) \
				{ \
					printf("ERROR: %s EXPECTS 1 arg\n", #t); \
					return new ErrorNode; \
				} \
				r.from += 2; \
				auto* N = new ReadNode_##t; \
				N->srcOff = ParseExpr(r); \
				return N; \
			}
			USE_READ_NODE(i8);
			USE_READ_NODE(i16);
			USE_READ_NODE(i32);
			USE_READ_NODE(i64);
			USE_READ_NODE(u8);
			USE_READ_NODE(u16);
			USE_READ_NODE(u32);
			USE_READ_NODE(u64);
#undef USE_READ_NODE
			if (name == "fpeqs" || name == "fpnes")
			{
				if (args.size() != 2 ||
					args[0].Empty() ||
					args[1].Length() != 1 ||
					tokens[args[1].from].type != FIELD_NAME ||
					tokens[args[0].to - 1].type != FIELD_NAME)
				{
					printf("ERROR: %s EXPECTS 2 args: f-query, name\n", name == "fpeqs" ? "fpeqs" : "fpnes");
					return new ErrorNode;
				}
				auto* N = new FieldPreviewEqualsStringNode;
				auto fieldName = tokens[args[0].to - 1].text;
				N->fieldName.assign(fieldName.data(), fieldName.size());
				args[0].to--;
				N->query = ParseQuery(args[0]);
				auto previewValue = tokens[args[1].from].text;
				N->text.assign(previewValue.data(), previewValue.size());
				N->invert = name != "fpeqs";
				return N;
			}
			if (name == "arr")
			{
				if (args.size() != 2 ||
					args[0].Empty() ||
					args[1].Empty() ||
					tokens[args[0].to - 1].type != FIELD_NAME)
				{
					printf("ERROR: arr EXPECTS 2 args: f-query, int\n");
					return new ErrorNode;
				}
				auto* N = new MemberFieldNode;
				auto fieldName = tokens[args[0].to - 1].text;
				N->name.assign(fieldName.data(), fieldName.size());
				args[0].to--;
				N->query = ParseQuery(args[0]);
				N->index = ParseExpr(args[1]);
				return N;
			}
			if (name == "iid")
			{
				if (args.size() != 1 ||
					args[0].Empty())
				{
					printf("ERROR: iid EXPECTS 1 arg: i-query\n");
					return new ErrorNode;
				}
				auto* N = new InstanceIDNode;
				N->query = ParseQuery(args[0], true);
				return N;
			}
		}

		size_t lastWeakestOp = FindLastWeakestOp(r);
		if (lastWeakestOp == r.to)
		{
			if (tokens[r.from].type == PEXPR_START && tokens[r.to - 1].type == PEXPR_END)
			{
				r.from++;
				r.to--;
				return ParseExpr(r);
			}

			if (tokens[r.to - 1].type == FIELD_NAME)
			{
				auto* N = new MemberFieldNode;
				auto name = tokens[r.to - 1].text;
				N->name.assign(name.data(), name.size());
				r.to--;
				if (tokens[r.from].type == OP_OFFSET)
				{
					r.from++;
					N->isOffset++;
				}
				N->query = ParseQuery(r);
				return N;
			}
			if (tokens[r.from].type == OP_OFFSET)
			{
				auto* N = new StructOffsetNode;
				r.from++;
				N->query = ParseQuery(r, true);
				return N;
			}
			puts("ERROR: CANNOT PARSE");
			return new ErrorNode;
		}

		ValueNode* A = ParseExpr({ r.from, lastWeakestOp });
		ValueNode* B = ParseExpr({ lastWeakestOp + 1, r.to });

		BinaryOpNode* N = nullptr;
		switch (tokens[lastWeakestOp].type)
		{
		case OP_ADD: N = new AddNode; break;
		case OP_SUB: N = new SubNode; break;
		case OP_MUL: N = new MulNode; break;
		case OP_DIV: N = new DivNode; break;
		case OP_MOD: N = new ModNode; break;

		case OP_AND: N = new AndNode; break;
		case OP_OR: N = new OrNode; break;
		case OP_XOR: N = new XorNode; break;
		case OP_LSH: N = new LShiftNode; break;
		case OP_RSH: N = new RShiftNode; break;

		case OP_EQ: N = new EqualNode; break;
		case OP_NE: N = new NotEqualNode; break;
		case OP_LT: N = new LessThanNode; break;
		case OP_LE: N = new LessEqualNode; break;
		case OP_GT: N = new GreaterThanNode; break;
		case OP_GE: N = new GreaterEqualNode; break;
		}
		N->srcA = A;
		N->srcB = B;
		return N;
	}

	size_t FindNextComma(Range r)
	{
		constexpr int MAX = 32;
		char exprStack[MAX] = {};
		int level = 0;
		for (size_t i = r.from; i < r.to; i++)
		{
			if (tokens[i].type == PEXPR_START || tokens[i].type == SEXPR_START)
			{
				if (level >= MAX)
					return r.to;
				exprStack[level++] = tokens[i].type == PEXPR_START ? PEXPR_END : SEXPR_END;
				continue;
			}
			if (tokens[i].type == PEXPR_END || tokens[i].type == SEXPR_END)
			{
				if (exprStack[--level] != tokens[i].type)
					return r.to;
				continue;
			}
			if (level > 0)
				continue;
			if (tokens[i].type == COMMA)
				return i;
		}
		return r.to;
	}

	void ParseCondition(Range& r, StructQueryNodeFilters& qnfs)
	{
		size_t comma = FindNextComma(r);

		if (tokens[r.from].type == FIELD_NAME)
		{
			if (r.from + 3 == comma && tokens[r.from + 1].type == OP_ASSIGN && tokens[r.from + 2].type == FIELD_NAME)
			{
				DDCondition C;
				C.field.assign(tokens[r.from].text.data(), tokens[r.from].text.size());
				C.value.assign(tokens[r.from + 2].text.data(), tokens[r.from + 2].text.size());
				qnfs.conditions.push_back(C);
			}
			else
			{
				puts("ERROR: BAD COND");
			}
		}
		else if (tokens[r.from].type == STRUCT_NAME)
		{
			if (r.from + 3 <= comma && tokens[r.from + 1].type == OP_ASSIGN)
			{
				if (tokens[r.from].text == "")
				{
					qnfs.which = ParseExpr({ r.from + 2, comma });
				}
				else
				{
					ExprCondition C;
					C.field.assign(tokens[r.from].text.data(), tokens[r.from].text.size());
					C.expected = ParseExpr({ r.from + 2, comma });
					qnfs.exprConds.push_back(std::move(C));
				}
			}
			else
			{
				puts("ERROR: BAD INDEX");
			}
		}
		else
		{
			puts("ERROR: UNKNOWN FILTER");
		}

		r.from = comma + 1;
	}

	void ParseConditions(Range r, StructQueryNodeFilters& qnfs)
	{
		while (r.from < r.to)
		{
			ParseCondition(r, qnfs);
			if (r.from < r.to && tokens[r.from].type == COMMA)
				r.from++;
		}
	}

	StructQueryNode* ParseQuery(Range r, bool isFirstPrequery = false)
	{
		if (r.from == r.to)
			return nullptr;

		if (!isFirstPrequery)
		{
			if (tokens[r.to - 1].type != OP_MEMBER)
			{
				puts("ERROR: BAD PREQUERY");
				return new ErrorQueryNode;
			}
			r.to--;
		}

		if (r.from == r.to)
		{
			puts("ERROR: BAD PREQUERY");
			return new ErrorQueryNode;
		}

		StructQueryNodeFilters qnfs;
		if (tokens[r.to - 1].type == SEXPR_END)
		{
			constexpr int MAX = 32;
			char exprStack[MAX] = {};
			int level = 0;
			exprStack[level++] = SEXPR_START;
			size_t cond = r.to;
			for (size_t i = r.to - 1; i > r.from; )
			{
				i--;
				cond = i;
				auto& T = tokens[i];
				if (T.type == SEXPR_START || T.type == PEXPR_START)
				{
					if (exprStack[level - 1])
					{
						level--;
						if (level == 0)
							break;
					}
					else
					{
						puts("ERROR: BAD CONDITION");
						return new ErrorQueryNode;
					}
				}
				else if (T.type == SEXPR_END || T.type == PEXPR_END)
				{
					if (level >= MAX)
					{
						puts("ERROR: TOO MANY BRACES");
						return new ErrorQueryNode;
					}
					exprStack[level++] = T.type;
				}
			}
			if (cond == r.from)
			{
				puts("ERROR: BAD COND");
				return new ErrorQueryNode;
			}
			ParseConditions({ cond + 1, r.to - 1 }, qnfs);
			r.to = cond;
		}

		if (tokens[r.to - 1].type == FIELD_NAME)
		{
			auto* N = new SubQueryNode;
			auto name = tokens[r.to - 1].text;
			N->name.assign(name.data(), name.size());
			r.to--;
			N->query = ParseQuery(r);
			N->filters = std::move(qnfs);
			return N;
		}

		if (tokens[r.to - 1].type == STRUCT_NAME)
		{
			if (r.from + 1 != r.to)
			{
				puts("ERROR: STRUCT NAME AFTER START OF QUERY");
				return new ErrorQueryNode;
			}

			auto* N = new RootQueryNode;
			auto name = tokens[r.to - 1].text;
			if (name.starts_with("#"))
			{
				N->global = true;
				name = name.substr(1);
			}
			N->typeName.assign(name.data(), name.size());
			N->filters = std::move(qnfs);
			return N;
		}

		puts("ERROR: ?");
		return new ErrorQueryNode;
	}

	void Parse()
	{
		// tokens
		for (;;)
		{
			Token t = ParseOne();
			if (t.type == END)
				break;
			tokens.push_back(t);
		}

		root = ParseExpr({ 0, tokens.size() });
	}

	void Dump()
	{
		fprintf(stderr, "tokens:");
		for (auto& T : tokens)
		{
			fprintf(stderr, "   %.*s", int(T.text.size()), T.text.data());
		}
		fprintf(stderr, "\n");

		fprintf(stderr, "tree:\n");
		root->Dump(0);
	}

	ui::StringView it;
	std::vector<Token> tokens;
	ValueNode* root;
};


static bool Matches(const StructQueryFilter& filter, DataDesc* desc, const DDStructInst* inst)
{
	if (filter.conditions.empty() && filter.intConds.empty())
		return true;
	for (const auto& C : filter.conditions)
	{
		size_t fid = inst->def->FindFieldByName(C.field);
		if (fid == SIZE_MAX)
			return false;
		if (inst->GetFieldPreview(fid) != C.value)
			return false;
	}
	for (const auto& C : filter.intConds)
	{
		size_t fid = inst->def->FindFieldByName(C.field);
		if (fid == SIZE_MAX)
			return false;
		if (inst->GetFieldIntValue(fid) != C.value)
			return false;
	}
	return true;
}

static StructQueryResults GetNth(StructQueryResults& res, int64_t nth)
{
	if (nth < 0 || nth >= res.size())
		return {};
	// TODO n = 0 fast path?
	std::sort(res.begin(), res.end(), [](const DDStructInst* A, const DDStructInst* B) { return A->off < B->off; });
	return { res[nth] };
}

bool VariableSource::GetVariable(const DDStructInst* inst, const std::string& field, int64_t pos, bool offset, int64_t& outVal)
{
	if (pos < 0)
		return false;

	for (size_t i = 0; i < constantCount; i++)
	{
		if (constants[i].name == field)
		{
			outVal = offset ? 0 : constants[i].value;
			return true;
		}
	}

	for (const auto& arg : inst->args)
	{
		if (arg.name == field)
		{
			outVal = offset ? 0 : arg.intVal;
			return true;
		}
	}

	size_t fid = inst->def->FindFieldByName(field);
	if (fid != SIZE_MAX)
	{
		if (offset)
			outVal = inst->GetFieldValueOffset(fid, pos);
		else
			outVal = inst->GetFieldIntValue(fid, pos);
		return true;
	}
	return false;
}

StructQueryResults VariableSource::GetInitialSet()
{
	return { root };
}

StructQueryResults VariableSource::Subquery(const StructQueryResults& src, const std::string& field, const StructQueryFilter& filter)
{
	StructQueryResults res;
	for (auto* SI : src)
	{
		size_t fid = SI->def->FindFieldByName(field);
		if (fid == SIZE_MAX)
			continue;

		auto* str = desc->FindStructByName(SI->def->fields[fid].type);
		if (!str)
			continue;

		if (!SI->IsFieldPresent(fid))
			continue;

		int64_t i = 0;
		SI->CreateFieldInstances(fid, SIZE_MAX, CreationReason::Query, [&](DDStructInst* ch)
		{
			if (Matches(filter, desc, ch))
			{
				if (filter.returnNth)
				{
					if (i++ == filter.nth)
					{
						res.push_back(ch);
						return false;
					}
				}
				else
				{
					res.push_back(ch);
				}
			}
			return true;
		});
	}
	return res;
}

StructQueryResults VariableSource::RootQuery(const std::string& typeName, bool global, const StructQueryFilter& filter)
{
	StructQueryResults res;
	auto* F = root->file;
	auto* S = desc->FindStructByName(typeName);
	if (!S)
		return res;
	for (auto* SI : desc->instances)
	{
		if (SI->def != S || (!global && SI->file != F))
			continue;
		if (!Matches(filter, desc, SI))
			continue;

		res.push_back(SI);
	}
	if (filter.returnNth)
		res = GetNth(res, filter.nth);
	return res;
}

size_t VariableSource::ReadFile(int64_t off, size_t size, void* outbuf)
{
	return root->file->dataSource->Read(off, size, outbuf);
}


bool InParseVariableSource::GetVariable(const DDStructInst* inst, const std::string& field, int64_t pos, bool offset, int64_t& outVal)
{
	if (pos < 0)
		return false;

	for (const auto& arg : inst->args)
	{
		if (arg.name == field)
		{
			outVal = offset ? 0 : arg.intVal;
			return true;
		}
	}

	size_t fid = inst->def->FindFieldByName(field);
	if (fid < untilField)
	{
		if (offset)
			outVal = inst->GetFieldValueOffset(fid, pos);
		else
			outVal = inst->GetFieldIntValue(fid, pos);
		return true;
	}
	return false;
}

StructQueryResults InParseVariableSource::GetInitialSet()
{
	return { root };
}

StructQueryResults InParseVariableSource::Subquery(const StructQueryResults& src, const std::string& field, const StructQueryFilter& filter)
{
	return {};
}

StructQueryResults InParseVariableSource::RootQuery(const std::string& typeName, bool global, const StructQueryFilter& filter)
{
	return {};
}

size_t InParseVariableSource::ReadFile(int64_t off, size_t size, void* outbuf)
{
	return root->file->dataSource->Read(off, size, outbuf);
}


MathExpr::~MathExpr()
{
	delete _impl;
}

void MathExpr::Compile(const char* expr)
{
	delete _impl;

	//fprintf(stderr, "Parse: %s\n", expr);
	Compiler c;
	c.it = expr;
	c.Parse();
	//c.Dump();
	_impl = new CompiledMathExpr;
	_impl->root = c.root;
}

int64_t MathExpr::Evaluate(IVariableSource* vsrc)
{
	if (!_impl || !_impl->root)
		return 0;
	return _impl->root->Eval(vsrc);
}

std::string MathExpr::GenPyScript()
{
	if (!_impl || !_impl->root)
		return "0";
	return _impl->root->GenPyScript();
}


#if 0
struct MathExprTest
{
	MathExprTest()
	{
		Test();
		exit(0);
	}
	void Test()
	{
		struct TVS : IVariableSource
		{
			bool GetVariable(const DDStructInst* inst, const std::string& field, int64_t pos, bool offset, int64_t& outVal) override
			{
				printf("GET VARIABLE%s %s[%" PRId64 "]\n", offset ? " OFFSET" : "", field.c_str(), pos);
				if (pos < 0)
					return false;
				outVal = 42 + pos;
				return true;
			}
			StructQueryResults GetInitialSet() override
			{
				puts("GET INITIAL SET");
				return { nullptr };
			}
			StructQueryResults Subquery(const StructQueryResults& src, const std::string& field, const StructQueryFilter& filter) override
			{
				printf("SUBQUERY %s\n", field.c_str());
				return { nullptr };
			}
			StructQueryResults RootQuery(const std::string& typeName, const StructQueryFilter& filter) override
			{
				printf("ROOT QUERY %s\n", typeName.c_str());
				return { nullptr };
			}
			size_t ReadFile(int64_t off, size_t size, void* outbuf)
			{
				printf("READ FILE @ %" PRId64 "[%zu]\n", off, size);
				memset(outbuf, 0, size);
				return size;
			}
		} tvs;
		MathExpr e;
		auto TEST = [&e, &tvs](const char* name)
		{
			printf("--- %s\n", name);
			e.Compile(name);
			printf("= %" PRId64 "\n", e.Evaluate(&tvs));
		};
		puts("\n\n- BASIC EXPRS -");
		TEST("15 + 3 * 2");
		TEST("(15 + 3) * 2");
		TEST("(1<2)|(3>4)");
		puts("\n\n- FIELD EXPRS -");
		TEST("(15 + field) * 2");
		TEST("(@# + field) * 2");
		TEST("(15 + @field) * 2");
		TEST("(15 + @\"field\") * 2");
		TEST("(15 + @#potato) * 2");
		TEST("(15 + @#\"potato\") * 2");
		TEST("(15 + @#potato.\"field\") * 2");
		TEST("(15 + @#potato.field.grain) * 2");
		TEST("(15 + @#potato[field=value]) * 2");
		TEST("(15 + @#potato[#=3]) * 2");
		TEST("(15 + @#potato[#=3+5, field=\"value\"]) * 2");
		puts("\n\n- FN EXPRS -");
		TEST("$i32 4"); // read i32 at 4
		TEST("$i32 @field"); // read i32 at field offset
		TEST("$u8 #a[#=3].b");
		TEST("$fpeqs field, name"); // compare field preview to name
		TEST("$fpeqs(#struct.field, \"name\")");
		TEST("$arr(#struct.field, 3 + 5)"); // read array at 3+5
	}
}
gMathExprTest;
#endif
