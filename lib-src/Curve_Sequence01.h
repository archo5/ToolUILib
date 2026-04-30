
#pragma once

#include "Core/Math.h"


namespace ui {


struct Curve_Sequence01
{
	enum class Mode
	{
		Hold,
		SinglePowerCurve,
		DoublePowerCurve,
		SawWave,
		PulseWave,
		Steps,
	};
	struct Point
	{
		float deltaX;
		float posX; // computed
		float posY;
		Mode mode = Mode::SinglePowerCurve;
		float tweak = 0;

		void OnSerialize(IObjectIterator& oi, const FieldInfo& FI);
	};

	Array<Point> points;

	void SerializeData(IObjectIterator& oi);

	static void GenSawWaveMiddlePoints(float tweak, const std::function<void(Vec2f)>& outpts);
	float EvaluateSegment(const Point& p0, const Point& p1, float q);
	float Evaluate(float t);

	void RemovePoint(size_t i);
	void AddPoint(Vec2f pos);
};


} // ui
