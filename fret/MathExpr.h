
#pragma once
#include "pch.h"
#include "DataDesc.h"



/* SELECTOR SYNTAX

basic field offset of same struct:              field
same struct field value:                        field@
field of another struct (single instance):      #another-struct.field@
field offset of another struct (nth instance):  #another-struct[#=n+4].field
field of another struct/nth/field:              #another-struct[#=n+4].field.subfield@
field of another struct with type=3:            #another-struct[type=3].field@
combined:                                       #another-struct[type=3, #=n+4].field.subfield@

RULES:

name = [a-zA-Z_] [a-zA-Z_0-9]+
name = "\"" [^"] "\""
field = name
struct = "#" name
locator = field | struct | locator "." field
filter = field ":" expr
filter = field "=" name
filter = filter "," filter
filtered_locator = locator "[" filter "]"
any_locator = locator | filtered_locator
selector = any_locator | any_locator "@"
expr = selector | constant | ...

*/


struct ICompileErrorOutput
{
	virtual void WriteError(const char* str, ...) = 0;
};

struct IVariableSource
{
	virtual bool GetVariable(DataDesc::StructInst* inst, const std::string& field, int64_t& outVal) = 0;
	virtual std::vector<DataDesc::StructInst*> GetInitialSet() = 0;
	virtual std::vector<DataDesc::StructInst*> Subquery(const std::vector<DataDesc::StructInst*>& src, const std::string& field, const std::vector<DataDesc::Condition>& conditions, int64_t* nth) = 0;
	virtual std::vector<DataDesc::StructInst*> RootQuery(const std::string& typeName, const std::vector<DataDesc::Condition>& conditions, int64_t* nth) = 0;
};

struct MathExpr
{
	~MathExpr();
	void Compile(const char* expr);
	int64_t Evaluate(IVariableSource* vsrc);

	struct CompiledMathExpr* _impl = nullptr;
};
