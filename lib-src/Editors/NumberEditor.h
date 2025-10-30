
#pragma once

#include "../Model/Controls.h"
#include "DragConfig.h"


namespace ui {

struct Textbox;

struct NumberEditorBase : FrameElement
{
	~NumberEditorBase();

	void OnReset() override;
	void OnPaint(const UIPaintContext& ctx) override;
	void OnEvent(Event& e) override;

	Rangef CalcEstimatedWidth(const Size2f& containerSize, EstSizeType type) override;
	Rangef CalcEstimatedHeight(const Size2f& containerSize, EstSizeType type) override;

	virtual void OnDragEdit(float diff, float snap) = 0;
	virtual std::string ValueToString(bool forEditing) = 0;
	virtual void SetValueFromString(StringView str) = 0;

	DragConfig dragConfig;

	Textbox* _activeTextbox = nullptr;
	bool dragged = false;
};

struct NumberFormatSettings
{
	i8 digits = -1;
	bool beautify : 1;
	bool shortFloat : 1;
	bool hex : 1;

	NumberFormatSettings() : beautify(true), shortFloat(true), hex(false) {}
};

template <class TVal> const char* GetScanFormat(NumberFormatSettings nfs);
template <> inline const char* GetScanFormat<signed int>(NumberFormatSettings nfs) { return nfs.hex ? "%x" : "%d"; }
template <> inline const char* GetScanFormat<unsigned int>(NumberFormatSettings nfs) { return nfs.hex ? "%x" : "%u"; }
template <> inline const char* GetScanFormat<signed long>(NumberFormatSettings nfs) { return nfs.hex ? "%lx" : "%ld"; }
template <> inline const char* GetScanFormat<unsigned long>(NumberFormatSettings nfs) { return nfs.hex ? "%lx" : "%lu"; }
template <> inline const char* GetScanFormat<signed long long>(NumberFormatSettings nfs) { return nfs.hex ? "%llx" : "%lld"; }
template <> inline const char* GetScanFormat<unsigned long long>(NumberFormatSettings nfs) { return nfs.hex ? "%llx" : "%llu"; }
template <> inline const char* GetScanFormat<float>(NumberFormatSettings nfs) { return nfs.hex ? "%a" : "%g"; }
template <> inline const char* GetScanFormat<double>(NumberFormatSettings nfs) { return nfs.hex ? "%A" : "%G"; }

template <class TVal> void GetPrintFormat(NumberFormatSettings nfs, char bfr[32]);
template <> inline void GetPrintFormat<signed int>(NumberFormatSettings nfs, char bfr[32])
{
	*bfr++ = '%';
	if (nfs.digits >= 0)
		bfr += snprintf(bfr, 4, ".%d", int(nfs.digits));
	*bfr++ = nfs.hex ? 'x' : 'd';
	*bfr = 0;
}
template <> inline void GetPrintFormat<unsigned int>(NumberFormatSettings nfs, char bfr[32])
{
	*bfr++ = '%';
	if (nfs.digits >= 0)
		bfr += snprintf(bfr, 4, ".%d", int(nfs.digits));
	*bfr++ = nfs.hex ? 'x' : 'u';
	*bfr = 0;
}
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

		range = Range<TNum>::All();
		format = {};
	}
	void OnDragEdit(float diff, float snap) override
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

		SetValue(nv);
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
		GetPrintFormat<TNum>(format, fmt);
		char buf[1024];
		snprintf(buf, 1024, fmt, value);
		return buf;
	}
	void SetValueFromString(StringView str) override
	{
		sscanf(str.Data(), GetScanFormat<TNum>(format), &const_cast<TNum&>(value));
	}

	const TNum value = 0;
	void SetValue(TNum v) { const_cast<TNum&>(value) = v; }

	Range<TNum> range = Range<TNum>::All();
	NumberFormatSettings format = {};

	double _accumulator = 0;
};

} // ui
