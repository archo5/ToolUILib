
#pragma once

#include "../Model/Controls.h"
#include "NumEditConfig.h"


namespace ui {

struct Textbox;

struct NumberEditorBase : FrameElement
{
	~NumberEditorBase();

	void OnReset() override;
	void OnPaint(const UIPaintContext& ctx) override;
	void OnEvent(Event& e) override;
	void _AttachToFrameContents(FrameContents* owner) override;

	EstSizeRange CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
	Rangef CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override;
	void OnLayout(const UIRect& rect, LayoutInfo info) override;

	virtual bool OnDragEdit(float diff, float snap) = 0;
	virtual std::string ValueToString(bool forEditing) = 0;
	virtual bool SetValueFromString(StringView str) = 0;

	static Optional<double> ParseMathExpr(StringView str);

	DragConfig dragConfig;
	std::string label;

	Textbox* _activeTextbox = nullptr;
	bool _dragged = false;
	bool _anyChg = false;
};

template <class TVal> const char* GetScanFormat(NumberFormatSettings nfs);
template <> inline const char* GetScanFormat<signed char>(NumberFormatSettings nfs) { return nfs.hex ? "%hhx%n" : "%hhd%n"; }
template <> inline const char* GetScanFormat<unsigned char>(NumberFormatSettings nfs) { return nfs.hex ? "%hhx%n" : "%hhu%n"; }
template <> inline const char* GetScanFormat<signed short>(NumberFormatSettings nfs) { return nfs.hex ? "%hx%n" : "%hd%n"; }
template <> inline const char* GetScanFormat<unsigned short>(NumberFormatSettings nfs) { return nfs.hex ? "%hx%n" : "%hu%n"; }
template <> inline const char* GetScanFormat<signed int>(NumberFormatSettings nfs) { return nfs.hex ? "%x%n" : "%d%n"; }
template <> inline const char* GetScanFormat<unsigned int>(NumberFormatSettings nfs) { return nfs.hex ? "%x%n" : "%u%n"; }
template <> inline const char* GetScanFormat<signed long>(NumberFormatSettings nfs) { return nfs.hex ? "%lx%n" : "%ld%n"; }
template <> inline const char* GetScanFormat<unsigned long>(NumberFormatSettings nfs) { return nfs.hex ? "%lx%n" : "%lu%n"; }
template <> inline const char* GetScanFormat<signed long long>(NumberFormatSettings nfs) { return nfs.hex ? "%llx%n" : "%lld%n"; }
template <> inline const char* GetScanFormat<unsigned long long>(NumberFormatSettings nfs) { return nfs.hex ? "%llx%n" : "%llu%n"; }
template <> inline const char* GetScanFormat<float>(NumberFormatSettings nfs) { return nfs.hex ? "%a%n" : "%g%n"; }
template <> inline const char* GetScanFormat<double>(NumberFormatSettings nfs) { return nfs.hex ? "%la%n" : "%lg%n"; }

template <class TVal> void GetPrintFormat(NumberFormatSettings nfs, char bfr[32]);
template <> inline void GetPrintFormat<signed int>(NumberFormatSettings nfs, char bfr[32])
{
	*bfr++ = '%';
	if (nfs.digits >= 0)
		bfr += snprintf(bfr, 4, ".%d", int(nfs.digits));
	*bfr++ = nfs.hex ? 'x' : 'd';
	*bfr = 0;
}
template <> inline void GetPrintFormat<signed short>(NumberFormatSettings nfs, char bfr[32]) { GetPrintFormat<signed int>(nfs, bfr); }
template <> inline void GetPrintFormat<signed char>(NumberFormatSettings nfs, char bfr[32]) { GetPrintFormat<signed int>(nfs, bfr); }
template <> inline void GetPrintFormat<unsigned int>(NumberFormatSettings nfs, char bfr[32])
{
	*bfr++ = '%';
	if (nfs.digits >= 0)
		bfr += snprintf(bfr, 4, ".%d", int(nfs.digits));
	*bfr++ = nfs.hex ? 'x' : 'u';
	*bfr = 0;
}
template <> inline void GetPrintFormat<unsigned short>(NumberFormatSettings nfs, char bfr[32]) { GetPrintFormat<unsigned int>(nfs, bfr); }
template <> inline void GetPrintFormat<unsigned char>(NumberFormatSettings nfs, char bfr[32]) { GetPrintFormat<unsigned int>(nfs, bfr); }
template <> inline void GetPrintFormat<signed long>(NumberFormatSettings nfs, char bfr[32])
{
	*bfr++ = '%';
	if (nfs.digits >= 0)
		bfr += snprintf(bfr, 4, ".%d", int(nfs.digits));
	*bfr++ = 'l';
	*bfr++ = nfs.hex ? 'x' : 'd';
	*bfr = 0;
}
template <> inline void GetPrintFormat<unsigned long>(NumberFormatSettings nfs, char bfr[32])
{
	*bfr++ = '%';
	if (nfs.digits >= 0)
		bfr += snprintf(bfr, 4, ".%d", int(nfs.digits));
	*bfr++ = 'l';
	*bfr++ = nfs.hex ? 'x' : 'u';
	*bfr = 0;
}
template <> inline void GetPrintFormat<signed long long>(NumberFormatSettings nfs, char bfr[32])
{
	*bfr++ = '%';
	if (nfs.digits >= 0)
		bfr += snprintf(bfr, 4, ".%d", int(nfs.digits));
	*bfr++ = 'l';
	*bfr++ = 'l';
	*bfr++ = nfs.hex ? 'x' : 'd';
	*bfr = 0;
}
template <> inline void GetPrintFormat<unsigned long long>(NumberFormatSettings nfs, char bfr[32])
{
	*bfr++ = '%';
	if (nfs.digits >= 0)
		bfr += snprintf(bfr, 4, ".%d", int(nfs.digits));
	*bfr++ = 'l';
	*bfr++ = 'l';
	*bfr++ = nfs.hex ? 'x' : 'u';
	*bfr = 0;
}
template <> inline void GetPrintFormat<float>(NumberFormatSettings nfs, char bfr[32])
{
	*bfr++ = '%';
	if (nfs.digits >= 0)
		bfr += snprintf(bfr, 4, ".%d", int(nfs.digits));
	if (nfs.shortFloat)
		*bfr++ = 'g';
	else if (nfs.hex)
		*bfr++ = 'a';
	else
		*bfr++ = 'f';
	*bfr = 0;
}
template <> inline void GetPrintFormat<double>(NumberFormatSettings nfs, char bfr[32]) { GetPrintFormat<float>(nfs, bfr); }

template <class T> float GetMinSnap(T) { return 1; }
inline float GetMinSnap(float v) { return max(nextafterf(v, INFINITY) - v, FLT_EPSILON); }
inline float GetMinSnap(double v) { return max(nextafter(v, INFINITY) - v, DBL_EPSILON); }

template <class TNum> struct NumberEditorT : NumberEditorBase
{
	void OnReset() override
	{
		NumberEditorBase::OnReset();

		range = All{};
		format = {};
	}
	bool OnDragEdit(float diff, float snap) override
	{
		if (dragConfig.relativeSpeed)
		{
			diff *= fabsf(value);
			snap = 0;
		}
		float minSnap = GetMinSnap(value);
		if (snap < minSnap)
			snap = minSnap;

		TNum nv = value;

		if (abs(_accumulator) >= snap)
		{
			// previous snap value was bigger
			_accumulator = 0;
		}
		_accumulator += diff;
		if (abs(_accumulator) >= snap)
		{
			nv += trunc(_accumulator / snap) * snap;
			_accumulator = fmod(_accumulator, snap);
		}

		if (nv > range.max || (diff > 0 && nv < value))
			nv = range.max;
		if (nv < range.min || (diff < 0 && nv > value))
			nv = range.min;

		bool chgd = nv != value;
		SetValue(nv);
		return chgd;
	}
	std::string ValueToString(bool forEditing) override
	{
		char fmt[32];
		NumberFormatSettings nfs = format;
		if (forEditing)
		{
			nfs.shortFloat = true;
			nfs.digits = -1; // TODO maximize precision for floats/doubles?
		}
		GetPrintFormat<TNum>(nfs, fmt);
		char buf[128];
		snprintf(buf, 128, fmt, value);
		if (strcmp(buf, "-0") == 0)
			return buf + 1;
		return buf;
	}
	bool SetValueFromString(StringView str) override
	{
		TNum nv = 0;
		int numproc = 0;
		if (!sscanf(str.Data(), GetScanFormat<TNum>(format), &nv, &numproc) || numproc != str.Size())
		{
			if (auto optval = ParseMathExpr(str))
			{
				double dnv = range.Cast<double>().Clamp(optval.GetValue());
				if (std::numeric_limits<TNum>::is_integer)
					dnv = round(dnv);
				nv = TNum(dnv);
			}
		}
		nv = range.Clamp(nv);
		if (nv != value)
		{
			const_cast<TNum&>(value) = nv;
			return true;
		}
		return false;
	}

	NumberEditorT& SetDragConfig(DragConfig cfg) { dragConfig = cfg; return *this; }
	NumberEditorT& SetLabel(StringView lbl) { label <<= lbl; return *this; }

	const TNum value = 0;
	NumberEditorT& SetValue(TNum v) { const_cast<TNum&>(value) = v; return *this; }

	Range<TNum> range = All{};
	NumberEditorT& SetRange(Range<TNum> r) { range = r; return *this; }

	NumberFormatSettings format = {};
	NumberEditorT& SetFormat(NumberFormatSettings f) { format = f; return *this; }

	double _accumulator = 0;
};

} // ui
