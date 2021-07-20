
#pragma once


namespace ui {

struct IMathExprErrorOutput
{
	virtual void OnError(int pos, const char* error) = 0;
};

struct IMathExprDataSource
{
	virtual const char** GetVariableNames() { return nullptr; }
	virtual const char** GetFunctionNames() { return nullptr; }
	virtual float GetVariable(const char* name) { return 0; }
	virtual float CallFunction(const char* name, const float* args, int numArgs) { return 0; }
};

struct MathExpr
{
	void* _data;

	MathExpr();
	~MathExpr();

	bool Compile(const char* str, IMathExprDataSource* src = nullptr, IMathExprErrorOutput* errOut = nullptr);
	float Evaluate(IMathExprDataSource* src = nullptr);
};

} // ui
