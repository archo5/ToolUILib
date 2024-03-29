
#include "MathExpr.h"

#include "Array.h"
#include "Logging.h"
#include "Math.h"
#include "String.h"


namespace ui {

LogCategory LOG_MATHEXPR("MathExpr");

struct DefaultErrorOutput : IMathExprErrorOutput
{
	void OnError(int pos, const char* error)
	{
		LogError(LOG_MATHEXPR, "MathExpr compilation error: [%d] %s", pos, error);
	}
};
static DefaultErrorOutput g_defErrOut;

bool IMathExprDataSource::IsNameEqualTo(const char* name, const char* name2)
{
	return StringView(name).EqualToCI(name2);
}


struct DummyMathExprDataSource : IMathExprDataSource
{
};
static DummyMathExprDataSource g_dummyEval;

struct MathExprData
{
	enum Instr
	{
		PushConst, // push constant on stack, increment push marker
		PushVar, // push variable on stack [1-byte index] in variable ID array
		PushVar16, // push variable on stack [2-byte index] in variable ID array
		FuncCall, // call a function [#args, 1-byte ID], replacing arguments with the return value
		FuncCall16, // call a function [#args, 2-byte ID], replacing arguments with the return value
		Return, // finish the evaluation and return the topmost value on stack
		// unary ops
		Negate,
		// binary ops
		Add,
		Sub,
		Mul,
		Div,
		Mod, // also func
		Pow, // also func
		// binary cmp ops
		IfEQ,
		IfNE,
		IfGT,
		IfGE,
		IfLT,
		IfLE,
		// one arg functions
		Abs,
		Sign,

		Round,
		Floor,
		Ceil,
		Int,
		Frac,

		Sqrt,
		LogE,
		Log2,
		Log10,
		ExpE,
		Exp2,

		Sin,
		Cos,
		Tan,
		ArcSin,
		ArcCos,
		ArcTan,
		// two arg functions
		ArcTan2,
		Min,
		Max,
		// three arg functions
		Clamp,
		Lerp,
		InvLerp,
		LerpC,
		InvLerpC,
	};

	float* constants = nullptr;
	uint8_t* code = nullptr;
	float* varMem = nullptr;
	uint16_t* varIDs = nullptr;

	float* stack = nullptr;

	size_t numConstants;
	size_t numCodeBytes;
	size_t numVars;
	size_t stackSize;

	float Eval(IMathExprDataSource* src)
	{
		// load variables
		{
			float* curVarMem = varMem;
			for (size_t i = 0; i < numVars; i++)
			{
				*curVarMem++ = src->GetVariable(varIDs[i]);
			}
		}

		float* stackLast = stack - 1;
		float* curConst = constants;
		uint8_t* ip = code;
		for (;;)
		{
			switch (*ip++)
			{
			case PushConst:
				*++stackLast = *curConst++;
				break;
			case PushVar:
				*++stackLast = varMem[*ip++];
				break;
			case PushVar16:
				*++stackLast = varMem[ip[0] | (uint16_t(ip[1]) << 8)];
				ip += 2;
				break;

			case FuncCall: {
				uint8_t numArgs = *ip++;
				uint16_t id = *ip++;
				stackLast -= numArgs - 1;
				*stackLast = src->CallFunction(id, stackLast, numArgs);
				break; }
			case FuncCall16: {
				uint8_t numArgs = *ip++;
				uint16_t id = *ip++;
				id |= uint16_t(*ip++) << 8;
				stackLast -= numArgs - 1;
				*stackLast = src->CallFunction(id, stackLast, numArgs);
				break; }
			case Return:
				return *stackLast;

			case Negate: *stackLast = -*stackLast; break;

			case Add: stackLast--; *stackLast += stackLast[1]; break;
			case Sub: stackLast--; *stackLast -= stackLast[1]; break;
			case Mul: stackLast--; *stackLast *= stackLast[1]; break;
			case Div:
				stackLast--;
				if (stackLast[1] == 0)
					return 0;
				*stackLast /= stackLast[1];
				break;
			case Mod:
				stackLast--;
				if (stackLast[1] == 0)
					return 0;
				*stackLast = fmodf(stackLast[0], stackLast[1]);
				break;
			case Pow:
				stackLast--;
				*stackLast = powf(stackLast[0], stackLast[1]);
				break;

			case IfEQ: stackLast--; *stackLast = stackLast[0] == stackLast[1]; break;
			case IfNE: stackLast--; *stackLast = stackLast[0] != stackLast[1]; break;
			case IfGT: stackLast--; *stackLast = stackLast[0] > stackLast[1]; break;
			case IfGE: stackLast--; *stackLast = stackLast[0] >= stackLast[1]; break;
			case IfLT: stackLast--; *stackLast = stackLast[0] < stackLast[1]; break;
			case IfLE: stackLast--; *stackLast = stackLast[0] <= stackLast[1]; break;

			case Abs: *stackLast = fabsf(*stackLast); break;
			case Sign: *stackLast = sign(*stackLast); break;

			case Round: *stackLast = roundf(*stackLast); break;
			case Floor: *stackLast = floorf(*stackLast); break;
			case Ceil: *stackLast = ceilf(*stackLast); break;
			case Int: *stackLast = float(int(*stackLast)); break;
			case Frac: {
				double dummy;
				*stackLast = modf(*stackLast, &dummy);
				break; }

			case Sqrt: *stackLast = sqrtf(*stackLast); break;
			case LogE: *stackLast = logf(*stackLast); break;
			case Log2: *stackLast = log2f(*stackLast); break;
			case Log10: *stackLast = log10f(*stackLast); break;
			case ExpE: *stackLast = expf(*stackLast); break;
			case Exp2: *stackLast = exp2f(*stackLast); break;

			case Sin: *stackLast = sinf(*stackLast); break;
			case Cos: *stackLast = cosf(*stackLast); break;
			case Tan: *stackLast = tanf(*stackLast); break;
			case ArcSin: *stackLast = asinf(*stackLast); break;
			case ArcCos: *stackLast = acosf(*stackLast); break;
			case ArcTan: *stackLast = atanf(*stackLast); break;

			case ArcTan2:
				stackLast--;
				*stackLast = atan2f(stackLast[0], stackLast[1]);
				break;
			case Min:
				stackLast--;
				*stackLast = min(stackLast[0], stackLast[1]);
				break;
			case Max:
				stackLast--;
				*stackLast = max(stackLast[0], stackLast[1]);
				break;

			case Clamp:
				stackLast -= 2;
				*stackLast = clamp(stackLast[0], stackLast[1], stackLast[2]);
				break;
			case Lerp:
				stackLast -= 2;
				*stackLast = lerp(stackLast[0], stackLast[1], stackLast[2]);
				break;
			case InvLerp:
				stackLast -= 2;
				*stackLast = invlerp(stackLast[0], stackLast[1], stackLast[2]);
				break;
			case LerpC:
				stackLast -= 2;
				*stackLast = lerp(stackLast[0], stackLast[1], clamp(stackLast[2], 0.0f, 1.0f));
				break;
			case InvLerpC:
				stackLast -= 2;
				*stackLast = clamp(invlerp(stackLast[0], stackLast[1], stackLast[2]), 0.0f, 1.0f);
				break;
			}
		}
	}

	~MathExprData()
	{
		delete[] stack;
		delete[] varIDs;
		delete[] varMem;
		delete[] code;
		delete[] constants;
	}

	MathExprData(
		const Array<float>& arg_constants,
		const Array<uint8_t>& arg_instructions,
		const Array<uint16_t>& arg_variables,
		size_t arg_maxTempStackSize)
	{
		constants = new float[arg_constants.size()];
		memcpy(constants, arg_constants.data(), sizeof(*constants) * arg_constants.size());
		numConstants = arg_constants.size();

		code = new uint8_t[arg_instructions.size()];
		memcpy(code, arg_instructions.data(), sizeof(*code) * arg_instructions.size());
		numCodeBytes = arg_instructions.size();

		varMem = new float[arg_variables.size()];
		varIDs = new uint16_t[arg_variables.size()];
		memcpy(varIDs, arg_variables.data(), sizeof(*varIDs) * arg_variables.size());
		numVars = arg_variables.size();

		stack = new float[arg_maxTempStackSize];
		stackSize = arg_maxTempStackSize;
	}

	MathExprData(const MathExprData& o)
	{
		numConstants = o.numConstants;
		constants = new float[numConstants];
		memcpy(constants, o.constants, sizeof(*constants) * numConstants);

		numCodeBytes = o.numCodeBytes;
		code = new uint8_t[numCodeBytes];
		memcpy(code, o.code, sizeof(*code) * numCodeBytes);

		numVars = o.numVars;
		varMem = new float[numVars];
		varIDs = new uint16_t[numVars];
		memcpy(varIDs, o.varIDs, sizeof(*varIDs) * numVars);

		stackSize = o.stackSize;
		stack = new float[stackSize];
	}

	MathExprData* Clone()
	{
		return new MathExprData(*this);
	}

	struct Compiler
	{
		enum TokenType
		{
			TTUnknown,
			TTOperator,
			TTComma,
			TTLParen,
			// end of a value:
			TTRParen,
			TTNumber,
			TTName,
		};
		enum
		{
			TMP_LParen = 255,
		};
		enum BuiltInFuncFlags
		{
			BIF_Degrees = 0x80,
		};
		static bool HasDegreeArg(uint8_t op)
		{
			op &= ~BIF_Degrees;
			return op == Sin || op == Cos || op == Tan;
		}

		struct Op
		{
			uint8_t op;
			uint8_t precedence;
			uint8_t realOp = UINT8_MAX;
			uint8_t expArgCount = UINT8_MAX;
			bool canPushArg = true;
			uint16_t realArgCount = 0;
			IMathExprDataSource::ID funcID = UINT16_MAX;
			StringView refText;
		};
		struct Scope
		{
			uint32_t numArgs = 0;
		};

		Array<float> constants;
		Array<uint8_t> instructions;
		Array<uint16_t> foundVars;
		Array<Op> opStack;
		int constStreak = 0;
		size_t maxTempStackSize = 0;
		size_t curTempStackSize = 0;

		const char* start;
		IMathExprDataSource* src;
		IMathExprErrorOutput* errOut;

		MathExprData* CreateExpr()
		{
			LogDebug(LOG_MATHEXPR, ">> EXPR: const=%d instrB=%d vars=%d stack=%d\n",
				int(constants.size()), int(instructions.size()), int(foundVars.size()), int(maxTempStackSize));

			return new MathExprData(constants, instructions, foundVars, maxTempStackSize);
		}

		int EPos(const StringView& sv)
		{
			return int(sv.data() - start + 1);
		}

		bool Error(const StringView& sv, const char* fmt, ...)
		{
			char bfr[256];
			va_list args;
			va_start(args, fmt);
			vsnprintf(bfr, 256, fmt, args);
			va_end(args);
			errOut->OnError(EPos(sv), bfr);
			return false;
		}

		void TSSPush()
		{
			if (maxTempStackSize == curTempStackSize)
				maxTempStackSize++;
			curTempStackSize++;
		}
		void TSSOp(size_t numArgs)
		{
			if (numArgs == 0)
				TSSPush();
			else
				curTempStackSize -= numArgs - 1;
		}

		void LastScopePushValue()
		{
			for (size_t i = opStack.size(); i > 0; )
			{
				i--;
				if (opStack[i].op != TMP_LParen)
					continue;
				if (opStack[i].canPushArg)
				{
					opStack[i].realArgCount++;
					opStack[i].canPushArg = false;
				}
				break;
			}
		}

		void LastScopeNextValue()
		{
			for (size_t i = opStack.size(); i > 0; )
			{
				i--;
				if (opStack[i].op != TMP_LParen)
					continue;
				opStack[i].canPushArg = true;
				break;
			}
		}

		void PushValue(float v)
		{
			if (CanLogDebug(LOG_MATHEXPR))
				LogDebug(LOG_MATHEXPR, "> PushConst %g\n", v);

			constStreak++;
			constants.Append(v);
			instructions.Append(PushConst);
			TSSPush();
			LastScopePushValue();
		}

		void CommitOp(const Op& op)
		{
			if (op.realOp == UINT8_MAX) // if op is not real
				return;

			if (CanLogDebug(LOG_MATHEXPR))
			{
				LogDebug(LOG_MATHEXPR, "> Op[%d]%.*s expargs=%d realargs=%d\n",
					op.realOp, int(op.refText.size()), op.refText.data(), op.expArgCount, op.realArgCount);
			}

			assert(curTempStackSize >= op.realArgCount);
			TSSOp(op.realArgCount);
			if (op.realOp == FuncCall)
			{
				constStreak = 0;
				instructions.Append(op.funcID > UINT8_MAX ? FuncCall16 : FuncCall);
				instructions.Append(uint8_t(op.realArgCount));
				instructions.Append(op.funcID & 0xff);
				if (op.funcID > UINT8_MAX)
					instructions.Append(op.funcID >> 8);
			}
			else
			{
				if (constStreak >= op.realArgCount)
				{
					EvalConstOp(op.realOp, op.realArgCount);
				}
				else
				{
					constStreak = 0;

					bool degArg = HasDegreeArg(op.realOp);
					if ((op.realOp & BIF_Degrees) != 0 && degArg)
					{
						constants.Append(DEG2RAD);
						instructions.Append(PushConst);
						instructions.Append(Mul);
					}

					instructions.Append(op.realOp & ~BIF_Degrees);

					if ((op.realOp & BIF_Degrees) != 0 && !degArg)
					{
						constants.Append(RAD2DEG);
						instructions.Append(PushConst);
						instructions.Append(Mul);
					}
				}
			}
		}

		void EvalConstOp(uint8_t op, uint16_t realArgCount)
		{
			constStreak -= realArgCount;
			for (uint16_t i = 0; i < realArgCount; i++)
				instructions.RemoveLast();

			const float* cd = constants.data() + constants.size() - realArgCount;
			float ret = 0;
			bool deg = (op & BIF_Degrees) != 0;
			float ARGMUL = deg ? DEG2RAD : 1;
			float RETMUL = deg ? RAD2DEG : 1;
			switch (op & ~BIF_Degrees)
			{
			case Negate: ret = -cd[0]; break;

			case Add: ret = cd[0] + cd[1]; break;
			case Sub: ret = cd[0] - cd[1]; break;
			case Mul: ret = cd[0] * cd[1]; break;
			case Div:
				if (cd[1])
					ret = cd[0] / cd[1];
				else
					ret = 0;
				break;
			case Mod:
				if (cd[1])
					ret = fmodf(cd[0], cd[1]);
				else
					ret = 0;
				break;
			case Pow:
				ret = powf(cd[0], cd[1]);
				break;
				
			case IfEQ: ret = cd[0] == cd[1] ? 1.0f : 0.0f; break;
			case IfNE: ret = cd[0] != cd[1] ? 1.0f : 0.0f; break;
			case IfGT: ret = cd[0] > cd[1] ? 1.0f : 0.0f; break;
			case IfGE: ret = cd[0] >= cd[1] ? 1.0f : 0.0f; break;
			case IfLT: ret = cd[0] < cd[1] ? 1.0f : 0.0f; break;
			case IfLE: ret = cd[0] <= cd[1] ? 1.0f : 0.0f; break;

			case Abs: ret = fabsf(cd[0]); break;
			case Sign: ret = sign(cd[0]); break;

			case Round: ret = roundf(cd[0]); break;
			case Floor: ret = floorf(cd[0]); break;
			case Ceil: ret = ceilf(cd[0]); break;
			case Int: ret = float(int(cd[0])); break;
			case Frac: {
				double dummy;
				ret = modf(cd[0], &dummy);
				break; }

			case Sqrt: ret = sqrtf(cd[0]); break;
			case LogE: ret = logf(cd[0]); break;
			case Log2: ret = log2f(cd[0]); break;
			case Log10: ret = log10f(cd[0]); break;
			case ExpE: ret = expf(cd[0]); break;
			case Exp2: ret = exp2f(cd[0]); break;

			case Sin: ret = sinf(cd[0] * ARGMUL); break;
			case Cos: ret = cosf(cd[0] * ARGMUL); break;
			case Tan: ret = tanf(cd[0] * ARGMUL); break;
			case ArcSin: ret = asinf(cd[0]) * RETMUL; break;
			case ArcCos: ret = acosf(cd[0]) * RETMUL; break;
			case ArcTan: ret = atanf(cd[0]) * RETMUL; break;

			case ArcTan2: ret = atan2f(cd[0], cd[1]) * RETMUL; break;
			case Min: ret = min(cd[0], cd[1]); break;
			case Max: ret = max(cd[0], cd[1]); break;
				
			case Clamp: ret = clamp(cd[0], cd[1], cd[2]); break;
			case Lerp: ret = lerp(cd[0], cd[1], cd[2]); break;
			case InvLerp: ret = invlerp(cd[0], cd[1], cd[2]); break;
			case LerpC: ret = lerp(cd[0], cd[1], clamp(cd[2], 0.0f, 1.0f)); break;
			case InvLerpC: ret = clamp(invlerp(cd[0], cd[1], cd[2]), 0.0f, 1.0f); break;

			default:
				assert(!"unknown op");
				break;
			}

			for (uint16_t i = 0; i < realArgCount; i++)
				constants.RemoveLast();

			constStreak++;
			constants.Append(ret);
			instructions.Append(PushConst);
		}

		void FinishScope()
		{
			while (opStack.NotEmpty() && opStack.Last().op != TMP_LParen)
			{
				CommitOp(opStack.Last());
				opStack.RemoveLast();
			}
		}

		uint8_t GetOpPrecedence(uint8_t op)
		{
			switch (op)
			{
			case TMP_LParen: return 1;
			case Add: return 2;
			case Sub: return 2;
			case Mul: return 3;
			case Div: return 3;
			case Mod: return 3;
			case Pow: return 4;
			case Negate: return 5;
			default: return 0;
			}
		}

		void AddOpToStack(uint8_t op, uint8_t expArgs)
		{
			uint8_t curPrecedence = GetOpPrecedence(op);

			while (opStack.NotEmpty())
			{
				auto top = opStack.Last();
				if (top.precedence < curPrecedence || top.op == Negate) // unary ops can't be committed by other unary ops
					break;

				CommitOp(top);
				opStack.RemoveLast();
			}

			opStack.Append({ op, curPrecedence, op, expArgs });
		}

		void AddFuncCallToStack(uint8_t realOp, uint8_t expArgs, IMathExprDataSource::ID funcID, StringView refText)
		{
			uint8_t op = TMP_LParen;
			uint8_t curPrecedence = GetOpPrecedence(op);
			Op v = { op, curPrecedence, realOp, expArgs };
			v.funcID = funcID;
			v.refText = refText;
			opStack.Append(v);
		}

		struct BuiltInFunc
		{
			StringView name;
			uint8_t numArgs;
			uint8_t op;
		};
		bool TryBuiltInFunc(StringView name)
		{
			static const BuiltInFunc builtInFuncs[] =
			{
				{ "mod", 2, Mod },
				{ "pow", 2, Pow },

				{ "ifeq", 2, IfEQ },
				{ "ifne", 2, IfNE },
				{ "ifgt", 2, IfGT },
				{ "ifge", 2, IfGE },
				{ "iflt", 2, IfLT },
				{ "ifle", 2, IfLE },

				{ "abs", 1, Abs },
				{ "sign", 1, Sign },

				{ "round", 1, Round },
				{ "floor", 1, Floor },
				{ "ceil", 1, Ceil },
				{ "int", 1, Int },
				{ "frac", 1, Frac },

				{ "sqrt", 1, Sqrt },
				{ "log", 1, LogE },
				{ "log2", 1, Log2 },
				{ "log10", 1, Log10 },
				{ "exp", 1, ExpE },
				{ "exp2", 1, Exp2 },

				{ "sin", 1, Sin },
				{ "cos", 1, Cos },
				{ "tan", 1, Tan },
				{ "sind", 1, Sin | BIF_Degrees },
				{ "cosd", 1, Cos | BIF_Degrees },
				{ "tand", 1, Tan | BIF_Degrees },

				{ "asin", 1, ArcSin },
				{ "arcsin", 1, ArcSin },
				{ "acos", 1, ArcCos },
				{ "arccos", 1, ArcCos },
				{ "atan", 1, ArcTan },
				{ "arctan", 1, ArcTan },
				{ "asind", 1, ArcSin | BIF_Degrees },
				{ "arcsind", 1, ArcSin | BIF_Degrees },
				{ "acosd", 1, ArcCos | BIF_Degrees },
				{ "arccosd", 1, ArcCos | BIF_Degrees },
				{ "atand", 1, ArcTan | BIF_Degrees },
				{ "arctand", 1, ArcTan | BIF_Degrees },

				{ "atan2", 2, ArcTan2 },
				{ "arctan2", 2, ArcTan2 },
				{ "atan2d", 2, ArcTan2 | BIF_Degrees },
				{ "arctan2d", 2, ArcTan2 | BIF_Degrees },

				{ "min", 2, Min },
				{ "max", 2, Max },

				{ "clamp", 3, Clamp },
				{ "lerp", 3, Lerp },
				{ "invlerp", 3, InvLerp },
				{ "lerpc", 3, LerpC },
				{ "invlerpc", 3, InvLerpC },
			};

			for (const auto& fn : builtInFuncs)
			{
				if (name.EqualToCI(fn.name))
				{
					AddFuncCallToStack(fn.op, fn.numArgs, UINT16_MAX, name);

					return true;
				}
			}
			return false;
		}

		bool TryBuiltInConst(StringView name)
		{
			if (name.EqualToCI("pi")) { PushValue(PI); return true; }
			if (name.EqualToCI("e")) { PushValue(E); return true; }
			if (name.EqualToCI("deg2rad")) { PushValue(DEG2RAD); return true; }
			if (name.EqualToCI("rad2deg")) { PushValue(RAD2DEG); return true; }
			return false;
		}

		TokenType ClassifyChar(char ch)
		{
			if (ch == '(')
				return TTLParen;
			if (ch == ')')
				return TTRParen;
			if (ch == ',')
				return TTComma;
			if (ch == '+' ||
				ch == '-' ||
				ch == '*' ||
				ch == '/' ||
				ch == '%' ||
				ch == '^')
				return TTOperator;
			if (IsDigit(ch))
				return TTNumber;
			if (IsAlpha(ch) || ch == '_')
				return TTName;
			return TTUnknown;
		}

		bool ReadConstant(StringView& it, float& vout)
		{
			double bv = 0;
			while (it.FirstCharIs(IsDigit))
			{
				bv *= 10;
				bv += it.take_char() - '0';
			}

			if (it.IsEmpty() || it.First() != '.')
			{
				vout = float(bv);
				return true;
			}
			it.take_char(); // '.'

			double decimalpos = 1;
			while (it.FirstCharIs(IsDigit))
			{
				decimalpos *= 0.1;
				bv += decimalpos * (it.take_char() - '0');
			}

			if (it.size() < 3 || it[0] != 'e' || (it[1] != '+' && it[1] != '-') || !IsDigit(it[2]))
			{
				vout = float(bv);
				return true;
			}

			it.take_char(); // 'e'
			double expsign = it.take_char() == '-' ? -1.0f : 1.0f;
			double exp = 0;
			while (it.FirstCharIs(IsDigit))
			{
				exp *= 10;
				exp += it.take_char() - '0';
			}
			bv *= pow(10, exp * expsign);
			vout = float(bv);
			return true;
		}

		bool Compile(StringView it)
		{
			TokenType prevTokenType = TTUnknown;
			for (;;)
			{
				it = it.ltrim();
				if (it.IsEmpty())
					break;

				char ch = it.First();
				TokenType curTokenType = ClassifyChar(ch);

				if (curTokenType == TTNumber)
				{
					float v = 0;
					if (!ReadConstant(it, v))
						return false;
					PushValue(v);
				}
				else if (curTokenType == TTName)
				{
					// variable or function
					StringView name = it.take_while([](char c) { return IsAlphaNum(c) || c == '_'; });
					it = it.ltrim();
					if (it.FirstCharIs([](char c) { return c == '('; }))
					{
						it.take_char();
						curTokenType = TTLParen;

						// function
						if (!TryBuiltInFunc(name))
						{
							int expArgs = -1;
							IMathExprDataSource::ID fid = IMathExprDataSource::NOT_FOUND;

							char bfr[256];
							if (name.size() < sizeof(bfr) - 1)
							{
								memcpy(bfr, name.data(), name.size());
								bfr[name.size()] = 0;

								fid = src->FindFunction(bfr, expArgs);
							}
							if (fid != IMathExprDataSource::NOT_FOUND)
							{
								AddFuncCallToStack(FuncCall, expArgs >= 0 ? expArgs : UINT8_MAX, fid, name);
							}
							else
								return Error(name, "function not found: %.*s", int(name.size()), name.data());
						}
					}
					else
					{
						// variable
						if (!TryBuiltInConst(name))
						{
							IMathExprDataSource::ID vid = IMathExprDataSource::NOT_FOUND;

							char bfr[256];
							if (name.size() < sizeof(bfr) - 1)
							{
								memcpy(bfr, name.data(), name.size());
								bfr[name.size()] = 0;

								vid = src->FindVariable(bfr);
							}
							if (vid != IMathExprDataSource::NOT_FOUND)
							{
								size_t at = SIZE_MAX;
								for (size_t i = 0; i < foundVars.size(); i++)
								{
									if (foundVars[i] == vid)
									{
										at = i;
										break;
									}
								}
								if (at == SIZE_MAX)
								{
									at = foundVars.size();
									foundVars.Append(vid);
								}
								constStreak = 0;
								if (at > UINT8_MAX)
								{
									instructions.Append(PushVar16);
									instructions.Append(at & 0xff);
									instructions.Append((at >> 8) & 0xff);
								}
								else
								{
									instructions.Append(PushVar);
									instructions.Append(at & 0xff);
								}
								TSSPush();
								LastScopePushValue();
							}
							else
								return Error(name, "variable not found: %.*s", int(name.size()), name.data());
						}
					}
				}
				else if (curTokenType == TTLParen)
				{
					if (prevTokenType == TTName || prevTokenType == TTNumber)
						return Error(it, "unexpected left parenthesis");

					uint8_t curPrecedence = GetOpPrecedence(TMP_LParen);
					opStack.Append({ TMP_LParen, curPrecedence, TMP_LParen, 1 });
					opStack.Last().refText = it.substr(0, 1);
					it.take_char();
				}
				else if (curTokenType == TTRParen)
				{
					if (prevTokenType == TTUnknown)
						return Error(it, "no matching left parenthesis found");
					if (prevTokenType == TTComma || prevTokenType == TTOperator)
						return Error(it, "unexpected end of expression");

					FinishScope();

					if (opStack.IsEmpty() || opStack.Last().op != TMP_LParen)
						return Error(it, "no matching left parenthesis found");

					auto& op = opStack.Last();
					if (op.expArgCount < UINT8_MAX && op.expArgCount != op.realArgCount)
					{
						if (op.realOp == TMP_LParen)
							return Error(op.refText, "empty subexpression");
						else
							return Error(op.refText, "unexpected argument count (expected %d, got %d)", op.expArgCount, op.realArgCount);
					}

					CommitOp(op);
					opStack.RemoveLast();
					LastScopePushValue();
					it.take_char();
				}
				else if (curTokenType == TTComma)
				{
					if (prevTokenType == TTUnknown || prevTokenType == TTComma || prevTokenType == TTLParen || prevTokenType == TTOperator)
						return Error(it, "unexpected end of expression");

					FinishScope();
					LastScopeNextValue();

					if (opStack.IsEmpty())
						return Error(it, "unexpected top-level comma");

					it.take_char();
				}
				else if (curTokenType == TTOperator)
				{
					StringView dbgText = it.substr(0, 1);
					it.take_char();

					// missing left operand
					if (prevTokenType == TTUnknown || prevTokenType == TTComma || prevTokenType == TTLParen || prevTokenType == TTOperator)
					{
						if (ch == '+')
						{
							// just ignore it
						}
						else if (ch == '-')
						{
							it = it.ltrim();
							// check if it's a sign
							if (it.FirstCharIs(IsDigit))
							{
								curTokenType = TTNumber;

								float v = 0;
								if (!ReadConstant(it, v))
									return false;
								PushValue(-v);
							}
							else
							{
								// negate operator
								AddOpToStack(Negate, 1);
								opStack.Last().realArgCount = 1;
								opStack.Last().refText = dbgText;
								LastScopePushValue();
							}
						}
						else
							return Error(dbgText, "unexpected binary operator");

						goto end_current;
					}

					Instr op;
					switch (ch)
					{
					case '+': op = Add; break;
					case '-': op = Sub; break;
					case '*': op = Mul; break;
					case '/': op = Div; break;
					case '%': op = Mod; break;
					case '^': op = Pow; break;
					default:
						return Error(dbgText, "internal error (bad op)");
					}
					AddOpToStack(op, 2);
					opStack.Last().realArgCount = 2;
					opStack.Last().refText = dbgText;
					LastScopePushValue();
				}
				else
					return Error(it, "unexpected character");

			end_current:
				prevTokenType = curTokenType;
			}

			if (prevTokenType == TTUnknown)
				return Error(it, "empty expression");

			if (prevTokenType == TTOperator)
				return Error(it, "unexpected end of expression");

			FinishScope();

			if (opStack.NotEmpty())
				return Error(opStack.Last().refText, "no matching right parenthesis found");

			instructions.Append(Return);

			return true;
		}
	};

	static MathExprData* Compile(const char* str, IMathExprDataSource* src, IMathExprErrorOutput* errOut)
	{
		Compiler C;
		C.start = str;
		C.src = src ? src : &g_dummyEval;
		C.errOut = errOut ? errOut : &g_defErrOut;
		if (C.Compile(str))
		{
			return C.CreateExpr();
		}
		return nullptr;
	}
};

MathExpr::MathExpr() : _data(nullptr)
{
}

MathExpr::MathExpr(const MathExpr& o)
{
	_data = o._data ? static_cast<MathExprData*>(o._data)->Clone() : nullptr;
}

MathExpr::MathExpr(MathExpr&& o) : _data(o._data)
{
	o._data = nullptr;
}

MathExpr::~MathExpr()
{
	delete static_cast<MathExprData*>(_data);
	_data = nullptr;
}

MathExpr& MathExpr::operator = (const MathExpr& o)
{
	if (this != &o)
	{
		delete static_cast<MathExprData*>(_data);
		_data = o._data ? static_cast<MathExprData*>(o._data)->Clone() : nullptr;
	}
	return *this;
}

MathExpr& MathExpr::operator = (MathExpr&& o)
{
	if (this != &o)
	{
		delete static_cast<MathExprData*>(_data);
		_data = o._data;
		o._data = nullptr;
	}
	return *this;
}

bool MathExpr::Compile(const char* str, IMathExprDataSource* src, IMathExprErrorOutput* errOut)
{
	delete static_cast<MathExprData*>(_data);
	_data = MathExprData::Compile(str, src, errOut);
	return !!_data;
}

float MathExpr::Evaluate(IMathExprDataSource* src)
{
	if (!_data)
		return 0;

	return static_cast<MathExprData*>(_data)->Eval(src ? src : &g_dummyEval);
}

bool MathExpr::IsConstant() const
{
	if (!_data)
		return true;
	auto& src = *static_cast<MathExprData*>(_data);
	if (src.numConstants != 1)
		return false;
	if (src.numCodeBytes != 2 ||
		src.code[0] != MathExprData::PushConst ||
		src.code[1] != MathExprData::Return)
		return false;
	return true;
}

bool MathExpr::IsConstant(float cmp) const
{
	float tmp;
	return GetConstant(tmp) && tmp == cmp;
}

bool MathExpr::GetConstant(float& val) const
{
	if (!_data)
	{
		val = 0;
		return true;
	}
	if (IsConstant())
	{
		val = static_cast<MathExprData*>(_data)->constants[0];
		return true;
	}
	return false;
}


#if UI_BUILD_TESTS
#include "Test.h"
struct TestMathExpr
{
	TestMathExpr()
	{
		TestCompile();
		TestCompileErrors();
		TestEval();
	}

	void TestCompile()
	{
		puts("TestCompile");

		// valid constants
		ASSERT_EQUAL(true, MathExpr().Compile("1", nullptr));
		ASSERT_EQUAL(true, MathExpr().Compile("-1", nullptr));
		ASSERT_EQUAL(true, MathExpr().Compile("1.23", nullptr));
		ASSERT_EQUAL(true, MathExpr().Compile("1.23e+4", nullptr));
		ASSERT_EQUAL(true, MathExpr().Compile("1.23e-4", nullptr));

		// valid builtin variables
		ASSERT_EQUAL(true, MathExpr().Compile("pi", nullptr));
		ASSERT_EQUAL(true, MathExpr().Compile("e", nullptr));

		// valid variable
		struct DS_QQVar : IMathExprDataSource
		{
			ID FindVariable(const char* name) override
			{
				if (IsNameEqualTo(name, "qq"))
					return 0;
				return NOT_FOUND;
			}
			ID FindFunction(const char* name, int& outNumArgs) override
			{
				if (IsNameEqualTo(name, "ff"))
					return 0;
				return NOT_FOUND;
			}
		} qqvarsrc;
		ASSERT_EQUAL(true, MathExpr().Compile("qq", &qqvarsrc));

		// missing variables
		ASSERT_EQUAL(false, MathExpr().Compile("qq", nullptr));

		// basic operators
		ASSERT_EQUAL(true, MathExpr().Compile("1+2", nullptr));
		ASSERT_EQUAL(true, MathExpr().Compile("1-pi", nullptr));
		ASSERT_EQUAL(true, MathExpr().Compile("-pi", nullptr));
		ASSERT_EQUAL(true, MathExpr().Compile("2*pi", nullptr));
		ASSERT_EQUAL(true, MathExpr().Compile("3/pi", nullptr));

		// unusual cases
		ASSERT_EQUAL(true, MathExpr().Compile("+++0", nullptr));
		ASSERT_EQUAL(true, MathExpr().Compile("---0", nullptr));

		// basic parentheses
		ASSERT_EQUAL(true, MathExpr().Compile("(1)", nullptr));
		ASSERT_EQUAL(true, MathExpr().Compile("(1+1)", nullptr));
		ASSERT_EQUAL(true, MathExpr().Compile("(1+1+1)", nullptr));
		ASSERT_EQUAL(true, MathExpr().Compile("(1+1*1)", nullptr));

		// basic functions
		ASSERT_EQUAL(true, MathExpr().Compile("sin(1)", nullptr));
		ASSERT_EQUAL(true, MathExpr().Compile("max(1,2)", nullptr));
		ASSERT_EQUAL(true, MathExpr().Compile("lerp(1,2,0.5)", nullptr));

		// function with variable args
		ASSERT_EQUAL(true, MathExpr().Compile("qq+lerp(qq,qq,qq)", &qqvarsrc));

		// unfinished expr
		ASSERT_EQUAL(false, MathExpr().Compile("32+lerp(0,1,ff(", &qqvarsrc));

		// bug 1
		ASSERT_EQUAL(true, MathExpr().Compile("64+lerp(-8,8,qq)", &qqvarsrc));
	}

	struct ErrorCollector : IMathExprErrorOutput
	{
		static constexpr int MAX_BUF = 2048;
		int count = 0;
		char errors[MAX_BUF] = {};

		void OnError(int pos, const char* error) override
		{
			if (count)
				count += snprintf(&errors[count], MAX_BUF - count, "###");
			count += snprintf(&errors[count], MAX_BUF - count, "[%d] %s", pos, error);
		}
	};

	void ErrorCheck(int line, const char* code, const char* expErr)
	{
		ErrorCollector ec;
		MathExpr e;
		if (e.Compile(code, nullptr, &ec))
		{
			fprintf(stderr, "%d: ERROR - expected expression to fail but it didn't: \"%s\"\n", line, code);
			return;
		}
		if (strcmp(ec.errors, expErr))
		{
			fprintf(stderr, "%d: ERROR - unexpected expression \"%s\" error: \"%s\"\n(expected \"%s\")\n", line, code, ec.errors, expErr);
			return;
		}
	}

#define ASSERT_ERR(code, err) ErrorCheck(__LINE__, code, err)

	void TestCompileErrors()
	{
		ASSERT_ERR("+", "[2] unexpected end of expression");
		ASSERT_ERR("(1", "[1] no matching right parenthesis found");
		ASSERT_ERR("12)", "[3] no matching left parenthesis found");
		ASSERT_ERR("12+qq", "[4] variable not found: qq");
		ASSERT_ERR("()", "[1] empty subexpression");
		ASSERT_ERR("(1,)", "[4] unexpected end of expression");
		ASSERT_ERR("(1,,)", "[4] unexpected end of expression");
		ASSERT_ERR("1,+2", "[2] unexpected top-level comma");
		ASSERT_ERR("1+sin()", "[3] unexpected argument count (expected 1, got 0)");
		ASSERT_ERR("12+sin(1,2)", "[4] unexpected argument count (expected 1, got 2)");
		ASSERT_ERR("123+qq()", "[5] function not found: qq");
	}

	float Eval(const char* expr)
	{
		MathExpr e;
		if (!e.Compile(expr, nullptr))
			return -9876; // unlikely output
		return e.Evaluate(nullptr);
	}
	struct Data : IMathExprDataSource
	{
		enum
		{
			VOne,
			VTwo,
		};
		ID FindVariable(const char* name) override
		{
			if (IsNameEqualTo(name, "one")) return VOne;
			if (IsNameEqualTo(name, "two")) return VTwo;
			return NOT_FOUND;
		}
		ID FindFunction(const char* name, int& outNumArgs) override
		{
			if (IsNameEqualTo(name, "zeroargs")) { outNumArgs = 0; return 0; }
			if (IsNameEqualTo(name, "onearg")) { outNumArgs = 1; return 1; }
			if (IsNameEqualTo(name, "twoargs")) { outNumArgs = 2; return 2; }
			return NOT_FOUND;
		}
		float GetVariable(ID id) override
		{
			switch (id)
			{
			case VOne: return 1;
			case VTwo: return 2;
			default: return 0;
			}
		}
		float CallFunction(ID id, const float* args, int numArgs) override
		{
			switch (id)
			{
			case 0: return 123;
			case 1: return args[0] * 5;
			case 2: return args[0] + args[1];
			default: return 0;
			}
		}
	};
	float EvalWithData(const char* expr)
	{
		Data d;
		MathExpr e;
		if (!e.Compile(expr, &d, nullptr))
			return -9876; // unlikely output
		return e.Evaluate(&d);
	}
	void TestEval()
	{
		// number parsing
		ASSERT_NEAR(1, Eval("1"));
		ASSERT_NEAR(1.23e+1f, Eval("1.23e+1"));

		// unary
		ASSERT_NEAR(1, Eval("+1"));
		ASSERT_NEAR(-1, Eval("-1"));

		// unusual unary
		ASSERT_NEAR(-1, Eval("0-1"));
		ASSERT_NEAR(-1, Eval("---1"));
		ASSERT_NEAR(-1, Eval("0---1")); // sign, unary, binary

		// constants
		ASSERT_NEAR(PI, Eval("pi"));
		ASSERT_NEAR(E, Eval("e"));

		// case insensitivity
		ASSERT_NEAR(PI, Eval("pI"));
		ASSERT_NEAR(PI, Eval("Pi"));
		ASSERT_NEAR(PI, Eval("PI"));

		// basic ops
		ASSERT_NEAR(1 + 1, Eval("1+1"));
		ASSERT_NEAR(3 - 2, Eval("3-2"));
		ASSERT_NEAR(2 * 2, Eval("2*2"));
		ASSERT_NEAR(4.0f / 2.0f, Eval("4/2"));
		ASSERT_NEAR(fmodf(5, 3), Eval("5%3"));
		ASSERT_NEAR(powf(2, 3), Eval("2^3"));

		// basic precedence
		ASSERT_NEAR(2 + 2 * 2, Eval("2+2*2"));
		ASSERT_NEAR(2 * 2 + 2, Eval("2*2+2"));
		ASSERT_NEAR(2 * (2 + 2), Eval("2*(2+2)"));

		// advanced precedence
		ASSERT_NEAR(1 + 2 - 3, Eval("1+2-3"));
		ASSERT_NEAR(1 + 2 - 3 * 4, Eval("1+2-3*4"));
		ASSERT_NEAR(1.0f * 2.0f / 3.0f, Eval("1*2/3"));
		ASSERT_NEAR(1 + 2 - 3.0f * 4.0f / 5.0f, Eval("1+2-3*4/5"));

		// functions
		ASSERT_NEAR(sinf(1), Eval("sin(1)"));
		ASSERT_NEAR(max(1.0f, 2.0f), Eval("max(1,2)"));
		ASSERT_NEAR(lerp(1, 2, 0.5f), Eval("lerp(1,2,0.5)"));

		// custom variables
		ASSERT_NEAR(1, EvalWithData("one"));
		ASSERT_NEAR(2, EvalWithData("two"));
		ASSERT_NEAR(2, EvalWithData("one+one"));
		ASSERT_NEAR(4, EvalWithData("two*two"));
		ASSERT_NEAR(3, EvalWithData("one+two"));

		// custom functions
		ASSERT_NEAR(123, EvalWithData("zeroargs()"));
		ASSERT_NEAR(55, EvalWithData("onearg(1+10)"));
		ASSERT_NEAR(444, EvalWithData("twoargs(123,321)"));

		// degree versions
		ASSERT_NEAR(64 + sinf(90 * DEG2RAD) * 16, Eval("64+sind(90)*16"));
		ASSERT_NEAR(64 + sinf(90 * DEG2RAD) * 16, EvalWithData("64+sind(90*one)*16"));
		ASSERT_NEAR(64 + asinf(1) * RAD2DEG * 16, Eval("64+asind(1)*16"));
		ASSERT_NEAR(64 + asinf(1) * RAD2DEG * 16, EvalWithData("64+asind(one)*16"));
	}
}
g_tests;
#endif

} // ui
