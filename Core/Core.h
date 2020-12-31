
#pragma once


static inline bool IsSpace(char c) { return c == ' ' || c == '\t' || c == '\n' || c == '\r'; }
static inline bool IsAlpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
static inline bool IsDigit(char c) { return c >= '0' && c <= '9'; }
static inline bool IsHexDigit(char c) { return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }
static inline bool IsAlphaNum(char c) { return IsAlpha(c) || IsDigit(c); }



inline float lerp(float a, float b, float s) { return a + (b - a) * s; }
inline float invlerp(float a, float b, float x) { return (x - a) / (b - a); }
template <class T> __forceinline T min(T a, T b) { return a < b ? a : b; }
template <class T> __forceinline T max(T a, T b) { return a > b ? a : b; }
template <class T> __forceinline T clamp(T x, T vmin, T vmax) { return x < vmin ? vmin : x > vmax ? vmax : x; }

