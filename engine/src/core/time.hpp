#pragma once

namespace geg {
	class Timer {
	public:
		static void update();
		static long now();
		static long now_ms();
		static double geg_now();
		static double geg_now_ms();
		static float dellta();
	};
}		 // namespace geg