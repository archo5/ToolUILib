
#include "pch.h"
#include "ExportScript.h"

struct PyWriter
{
	void BeginClass(const char* name)
	{
		WriteIndent();
		text += "class ";
		text += name;
		text += ":\n";
		indent++;
	}
	void EndClass()
	{
		indent--;
		text += "\n";
	}
	void BeginFunction(const char* nameargs)
	{
		WriteIndent();
		text += "def ";
		text += nameargs;
		text += ":\n";
		indent++;
	}
	void EndFunction()
	{
		indent--;
		text += "\n";
	}

	void Writef(const char* fmt, ...)
	{
		WriteIndent();
		char bfr[1024];
		va_list args;
		va_start(args, fmt);
		int len = vsnprintf(bfr, 1024, fmt, args);
		va_end(args);
		text.append(bfr, len);
		text += "\n";
	}
	std::string Name(ui::StringView v)
	{
		std::string ret;
		ret.reserve(v.size());
		for (char c : v)
		{
			if (ui::IsAlphaNum(c) || c == '_')
				ret += c;
			else
				ret += '_';
		}
		return ret;
	}
	std::string String(ui::StringView v)
	{
		std::string ret;
		ret += "\"";
		ret.reserve(v.size() + 2);
		for (char c : v)
		{
			if (c == '"')
			{
				ret += "\\\"";
			}
			else if (c == '\\')
			{
				ret += "\\\\";
			}
			else if (c < 0x20 || c >= 0x7f)
			{
				ret += "\\x";
				ret += "0123456789abcdef"[c >> 4];
				ret += "0123456789abcdef"[c & 15];
			}
			else
				ret += c;
		}
		ret += "\"";
		return ret;
	}
	const char* Bool(bool v)
	{
		return v ? "True" : "False";
	}

	void WriteIndent()
	{
		for (int i = 0; i < indent; i++)
			text += "\t";
	}

	std::string text;
	int indent = 0;
};

std::string ExportPythonScript(DataDesc* dd)
{
	PyWriter w;

	w.Writef("import bdat");

	std::vector<std::string> structNames;
	for (const auto& sp : dd->structs)
		structNames.push_back(sp.first);
	std::sort(structNames.begin(), structNames.end());
	for (const auto& sn : structNames)
	{
		DDStruct* S = dd->FindStructByName(sn);
		w.BeginClass((S->name + "(bdat.Struct)").c_str());

#if 0 // raw metadata
		w.Writef("params = {");
		w.indent++;
		for (const auto& P : S->params)
		{
			w.Writef("%s, %" PRId64 "),", w.String(P.name).c_str(), P.intVal);
		}
		w.indent--;
		w.Writef("}");

		w.Writef("serialized = %s", w.Bool(S->serialized));
		w.Writef("size = %" PRId64, S->size);
		w.Writef("sizeSrc = %s", w.String(S->sizeSrc).c_str());

		w.Writef("fields = {");
		w.indent++;
		for (const auto& F : S->fields)
		{
			auto name = w.String(F.name);
			w.Writef("%s: bdat.Field(", name.c_str());
			w.indent++;
			w.Writef("name=%s,", name.c_str());
			w.Writef("type=%s,", w.String(F.type).c_str());
			w.Writef("valueExpr=None,");
			w.Writef("off=%" PRId64 ",", F.off);
			w.Writef("offExpr=None,");
			w.Writef("count=%" PRId64 ",", F.count);
			w.Writef("countSrc=%s,", w.String(F.countSrc).c_str());
			w.Writef("countIsMaxSize=%s,", w.Bool(F.countIsMaxSize));
			w.Writef("individualComputedOffsets=%s,", w.Bool(F.individualComputedOffsets));
			w.Writef("readUntil0=%s,", w.Bool(F.readUntil0));
			w.indent--;
			w.Writef("),");
		}
		w.indent--;
		w.Writef("}");
#endif

		w.BeginFunction("load(self)");
		w.Writef("src = self._src");
		w.Writef("off = self._off");
		w.Writef("vs = ipvs = bdat.InParseVariableSource(self)");
		w.Writef("fdvs = bdat.FullDataVariableSource(self)");
		bool curFull = false;
		for (const auto& F : S->fields)
		{
			if (!F.condition.expr.empty())
			{
				if (curFull != false)
				{
					w.Writef("vs = ipvs");
					curFull = false;
				}
				w.Writef("if %s != 0:", F.condition.GenPyScript().c_str());
				w.indent++;
			}

			auto name = w.Name(F.name);
			if (!F.valueExpr.expr.empty())
			{
				w.Writef("self.%s = %s", name.c_str(), F.valueExpr.GenPyScript().c_str());
			}
			else
			{
				std::string count;
				if (F.IsOne())
					count = "False";
				else if (!F.countSrc.empty())
				{
					if (curFull != false)
					{
						w.Writef("vs = ipvs");
						curFull = false;
					}
					count = "self." + F.countSrc + "+" + std::to_string(F.count);
				}
				else
					count = std::to_string(F.count);

				std::string origOff = "off";
				if (!S->serialized)
				{
					origOff += "+";
					origOff += std::to_string(F.off);
				}
				std::string off = origOff;
				if (!F.offExpr.expr.empty())
				{
					if (curFull != true)
					{
						w.Writef("vs = fdvs");
						curFull = true;
					}
					w.Writef("vs.vars['origOff'] = %s", origOff.c_str());
					if (F.individualComputedOffsets)
					{
						w.Writef("cfo = []");
						w.Writef("for i in range(%s):", count.c_str());
						w.indent++;
						w.Writef("vs.vars['i'] = i");
						w.Writef("ceo = %s", F.offExpr.GenPyScript().c_str());
						if (!F.elementCondition.expr.empty())
						{
							w.Writef("vs.vars['off'] = ceo");
							w.Writef("if %s == 0:", F.elementCondition.GenPyScript().c_str());
							w.indent++;
							w.Writef("ceo = None");
							w.indent--;
							w.Writef("del vs.vars['off']");
						}
						w.Writef("cfo.append(ceo)");
						w.indent--;
					}
					else
					{
						w.Writef("vs.vars['i'] = 0");
						w.Writef("cfo = %s", F.offExpr.GenPyScript().c_str());
					}
					off = "cfo";
				}
				w.Writef("self._off_%s = %s", name.c_str(), off.c_str());

				const char* endoff = "_";
				if (!F.IsComputed() && S->serialized)
					endoff = "off";

				if (auto* FFS = dd->FindStructByName(F.type))
				{
					auto type = w.Name(F.type);
					w.Writef("self.%s, %s = bdat.read_struct(%s, src, %s, %s, %s, {})",
						name.c_str(), endoff, type.c_str(), off.c_str(), count.c_str(), w.Bool(F.countIsMaxSize));
				}
				else
				{
					auto type = w.String(F.type);
					w.Writef("self.%s, %s = bdat.read_builtin(%s, src, %s, %s, %s)",
						name.c_str(), endoff, type.c_str(), off.c_str(), count.c_str(), w.Bool(F.readUntil0));
				}
			}

			if (!F.condition.expr.empty())
			{
				w.indent--;
				w.Writef("else:");
				w.indent++;
				w.Writef("self.%s = None", name.c_str());
				w.indent--;
			}
		}
		if (S->serialized)
			w.Writef("self._offend = off");
		else
		{
			w.Writef("self._offend = TODO(size + sizeSrc)");
		}
		w.EndFunction();

		w.EndClass();
	}

	return w.text;
}
