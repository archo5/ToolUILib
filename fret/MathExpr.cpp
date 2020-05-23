
#include "pch.h"
#include "MathExpr.h"


enum class MEOperation : uint8_t
{
	Unknown,

	Add,
	Subtract,
	Multiply,
	Divide,
	Modulo,

	And,
	Or,
	Xor,
	LShift,
	RShift,
};

struct ValueNode
{
	virtual int64_t Eval(IVariableSource*) const = 0;
	virtual void Dump(int level) const = 0;
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
};

struct ConstantNode : ValueNode
{
	int64_t Eval(IVariableSource*) const override { return value; }
	void Dump(int level) const override { DMPLEV(level); fprintf(stderr, "value = %" PRId64 "\n", value); }

	int64_t value = 0;
};

struct UnaryOpNode : ValueNode
{
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
};

struct BitwiseInvertNode : UnaryOpNode
{
	int64_t Do(int64_t a) const override { return ~a; }
	const char* Name() const override { return "invert"; }
};

struct BinaryOpNode : ValueNode
{
	virtual int64_t Do(int64_t a, int64_t b) const = 0;
	int64_t Eval(IVariableSource* vs) const override { return Do(srcA->Eval(vs), srcB->Eval(vs)); }

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

struct StructQueryNode
{
	virtual std::vector<DataDesc::StructInst*> Query(IVariableSource* vs) = 0;
	virtual void Dump(int level) const = 0;
	void DumpConditions(int level) const
	{
		for (auto& C : conditions)
		{
			fprintf(stderr, " \"%s\"=\"%s\"", C.field.c_str(), C.value.c_str());
		}
		if (which)
		{
			fprintf(stderr, " #=\n");
			which->Dump(level + 1);
		}
		else
			fprintf(stderr, "\n");
	}

	std::vector<DataDesc::Condition> conditions;
	ValueNode* which = nullptr;
};

struct ErrorQueryNode : StructQueryNode
{
	std::vector<DataDesc::StructInst*> Query(IVariableSource* vs) override { return {}; }
	void Dump(int level) const override { DMPLEV(level); fprintf(stderr, "ERROR\n"); }
};

struct RootQueryNode : StructQueryNode
{
	std::vector<DataDesc::StructInst*> Query(IVariableSource* vs) override
	{
		int64_t nth = which ? which->Eval(vs) : 0;
		return vs->RootQuery(typeName, conditions, which ? &nth : 0);
	}
	void Dump(int level) const override
	{
		DMPLEV(level);
		fprintf(stderr, "root (struct) query \"%s\"", typeName.c_str());
		DumpConditions(level);
	}

	std::string typeName;
};

struct SubQueryNode : StructQueryNode
{
	std::vector<DataDesc::StructInst*> Query(IVariableSource* vs) override
	{
		int64_t nth = which ? which->Eval(vs) : 0;
		return vs->Subquery(query ? query->Query(vs) : vs->GetInitialSet(), name, conditions, which ? &nth : 0);
	}
	void Dump(int level) const override
	{
		DMPLEV(level);
		fprintf(stderr, "subfield query \"%s\"", name.c_str());
		DumpConditions(level);
		if (query)
			query->Dump(level + 1);
	}

	StructQueryNode* query = nullptr;
	std::string name;
};

struct MemberFieldNode : ValueNode
{
	int64_t Eval(IVariableSource* vs) const override
	{
		std::vector<DataDesc::StructInst*> insts = query ? query->Query(vs) : vs->GetInitialSet();
		int64_t ret = 0;
		if (insts.size() > 0 && vs->GetVariable(insts[0], name, ret))
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

	StructQueryNode* query = nullptr;
	std::string name;
	bool isOffset = false;
};

struct CompiledMathExpr
{
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

	PEXPR_START = '(',
	PEXPR_END = ')',
	SEXPR_START = '[',
	SEXPR_END = ']',
	OP_OFFSET = '@',
	OP_MEMBER = '.',
	OP_READ = '$',
};

struct Token
{
	METokenType type;
	StringView text;
};

static StringView operatorStrings[] =
{
	"(", ")", "[", "]", "@", ".", "$",
	"+", "-", "*", "/", "%",
	"&", "|", "^", "<<", ">>", "~",
};
static METokenType operatorTypes[] =
{
	PEXPR_START, PEXPR_END, SEXPR_START, SEXPR_END, OP_OFFSET, OP_MEMBER, OP_READ,
	OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD,
	OP_AND, OP_OR, OP_XOR, OP_LSH, OP_RSH, OP_INV,
};

static bool IsNameChar(char c)
{
	return IsAlphaNum(c) || c == '_';
}
static bool IsFirstNameChar(char c)
{
	return IsAlpha(c) || c == '_' || c == '"' || c == '#';
}

struct Compiler
{
	struct Range
	{
		size_t from, to;
	};

	Token ParseOne()
	{
		it = it.ltrim();
		if (it.empty())
			return { END, it };

		for (int i = 0; i < 12; i++)
		{
			if (it.starts_with(operatorStrings[i]))
			{
				size_t sz = operatorStrings[i].size();
				auto text = it.substr(0, sz);
				it = it.substr(sz);
				return { operatorTypes[i], text };
			}
		}

		if (it.first_char_is(IsDigit))
		{
			StringView num = it.take_while(IsDigit);
			return { CONSTANT, num };
		}

		if (it.first_char_is(IsFirstNameChar))
		{
			bool isStruct = it.first() == '#';
			if (isStruct)
				it.take_char();
			if (it.first_char_is(IsFirstNameChar) && it.first() != '#')
			{
				bool isQuoted = it.first() == '"';
				if (isQuoted)
					it.take_char(); // eat the first '"'
				StringView name = isQuoted ? it.take_while([](char c) { return c != '"'; }) : it.take_while(IsNameChar);
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
				puts("ERROR: BAD STRUCT NAME");
		}

		puts("ERROR!");
		return { END };
	}

	int GetOpScore(size_t at, Range r)
	{
		Token& T = tokens[at];
		if (T.type == OP_OR) return 4;
		if (T.type == OP_XOR) return 6;
		if (T.type == OP_AND) return 8;
		if (T.type == OP_LSH || T.type == OP_RSH)
			return 10;
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
		static char exprStack[MAX];
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
				for (char c : tokens[r.from].text)
					val = val * 10 + (c - '0');
				N->value = val;
				return N;
			}
		}

		if (tokens[r.from].type == PEXPR_START && tokens[r.to - 1].type == PEXPR_END)
		{
			r.from++;
			r.to--;
		}

		size_t lastWeakestOp = FindLastWeakestOp(r);
		if (lastWeakestOp == r.to)
		{
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
		}
		N->srcA = A;
		N->srcB = B;
		return N;
	}

	StructQueryNode* ParseQuery(Range r)
	{
		if (r.from == r.to)
			return nullptr;

		if (tokens[r.to - 1].type != OP_MEMBER)
		{
			puts("ERROR: BAD PREQUERY");
			return new ErrorQueryNode;
		}
		r.to--;

		if (r.from == r.to)
		{
			puts("ERROR: BAD PREQUERY");
			return new ErrorQueryNode;
		}

		// TODO parse conditions

		if (tokens[r.to - 1].type == FIELD_NAME)
		{
			auto* N = new SubQueryNode;
			auto name = tokens[r.to - 1].text;
			N->name.assign(name.data(), name.size());
			r.to--;
			N->query = ParseQuery(r);
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
			N->typeName.assign(name.data(), name.size());
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

	StringView it;
	std::vector<Token> tokens;
	ValueNode* root;
};


MathExpr::~MathExpr()
{
	delete _impl;
}

void MathExpr::Compile(const char* expr)
{
	delete _impl;

	fprintf(stderr, "Parse: %s\n", expr);
	Compiler c;
	c.it = expr;
	c.Parse();
	c.Dump();
	_impl = new CompiledMathExpr;
	_impl->root = c.root;
}

int64_t MathExpr::Evaluate(IVariableSource* vsrc)
{
	if (!_impl || !_impl->root)
		return 0;
	return _impl->root->Eval(vsrc);
}


#if 1
struct MathExprTest
{
	MathExprTest()
	{
		struct TVS : IVariableSource
		{
			bool GetVariable(DataDesc::StructInst* inst, const std::string& field, int64_t& outVal) override
			{
				printf("GET VARIABLE %s\n", field.c_str());
				outVal = 42;
				return true;
			}
			std::vector<DataDesc::StructInst*> GetInitialSet() override
			{
				puts("GET INITIAL SET");
				return { nullptr };
			}
			std::vector<DataDesc::StructInst*> Subquery(const std::vector<DataDesc::StructInst*>& src, const std::string& field, const std::vector<DataDesc::Condition>& conditions, int64_t* nth) override
			{
				printf("SUBQUERY %s\n", field.c_str());
				return { nullptr };
			}
			std::vector<DataDesc::StructInst*> RootQuery(const std::string& typeName, const std::vector<DataDesc::Condition>& conditions, int64_t* nth) override
			{
				printf("ROOT QUERY %s\n", typeName.c_str());
				return { nullptr };
			}
		} tvs;
		MathExpr e;
		e.Compile("15 + 3 * 2"); printf("= %" PRId64 "\n", e.Evaluate(&tvs));
		e.Compile("(15 + 3) * 2"); printf("= %" PRId64 "\n", e.Evaluate(&tvs));
		e.Compile("(15 + field) * 2"); printf("= %" PRId64 "\n", e.Evaluate(&tvs));
		e.Compile("(15 + @field) * 2"); printf("= %" PRId64 "\n", e.Evaluate(&tvs));
		e.Compile("(15 + @#potato.\"field\") * 2"); printf("= %" PRId64 "\n", e.Evaluate(&tvs));
		e.Compile("(15 + @#potato.field.grain) * 2"); printf("= %" PRId64 "\n", e.Evaluate(&tvs));

		exit(0);
	}
}
gMathExprTest;
#endif
