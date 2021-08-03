
#pragma once

#include "Platform.h"


namespace ui {

struct IMathExprErrorOutput
{
	virtual void OnError(int pos, const char* error) = 0;
};

struct IMathExprDataSource
{
	using ID = uint16_t;
	static constexpr ID NOT_FOUND = UINT16_MAX;
	static bool IsNameEqualTo(const char* name, const char* name2);

	virtual ID FindVariable(const char* name) { return NOT_FOUND; }
	virtual ID FindFunction(const char* name, int& outNumArgs) { return NOT_FOUND; }
	virtual float GetVariable(ID id) { return 0; }
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

	bool Compile(const char* str, IMathExprDataSource* src = nullptr, IMathExprErrorOutput* errOut = nullptr);
	float Evaluate(IMathExprDataSource* src = nullptr);
};

} // ui
