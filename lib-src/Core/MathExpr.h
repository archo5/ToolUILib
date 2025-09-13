
#pragma once

#include "Platform.h"
#include "String.h"


namespace ui {

struct IMathExprErrorOutput
{
	virtual void OnError(int pos, const char* error) = 0;
};

struct IMathExprDataSource
{
	using ID = u16;
	static constexpr ID NOT_FOUND = UINT16_MAX;
	static constexpr ID RUNTIME_VAR = UINT16_MAX - 1;
	static bool IsNameEqualTo(const char* name, const char* name2);

	virtual ID FindVariable(const char* name) { return NOT_FOUND; }
	virtual ID FindFunction(const char* name, int& outNumArgs) { return NOT_FOUND; }
	virtual float GetVariable(ID id) { return 0; }
	virtual float GetRuntimeVariable(StringView name) { return 0; } // any logging of unknown runtime vars can happen from here
	virtual float CallFunction(ID id, const float* args, int numArgs) { return 0; }
};

struct MathExpr
{
	void* _data;

	MathExpr();
	MathExpr(const MathExpr&);
	MathExpr(MathExpr&&);
	~MathExpr();
	MathExpr& operator = (const MathExpr&);
	MathExpr& operator = (MathExpr&&);

	bool Compile(StringView str, IMathExprDataSource* src = nullptr, IMathExprErrorOutput* errOut = nullptr);
	float Evaluate(IMathExprDataSource* src = nullptr) const;

	bool IsConstant() const;
	bool IsConstant(float cmp) const;
	bool GetConstant(float& val) const;
};

} // ui
