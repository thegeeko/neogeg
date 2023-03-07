#include "time.hpp"

#include <chrono>
#include <ctime>

namespace geg {
	using std::chrono::duration;
	using std::chrono::duration_cast;
	using std::chrono::high_resolution_clock;
	using std::chrono::milliseconds;
	using std::chrono::nanoseconds;
	using std::chrono::system_clock;
	using std::chrono::time_point;

	static bool timer_init = false;		 // check if this the first time
	static time_point<high_resolution_clock> start_time;		// high reslotion timer for the engine
	static double last_frame;		 // to calculate delta time

	static uint64_t time;		 // time in s since epoch
	static uint64_t time_ms;		// same as above but in ms
	static double geg_time;		 // high reslotion since the beginning of the engine
	static double geg_time_ms;		// same as above but in ms
	static double delta_time;

	static uint64_t s_frame_count;

	void Timer::update() {
		if (!timer_init) {
			start_time = high_resolution_clock::now();
			last_frame = 0;
			s_frame_count = 0;
			timer_init = true;
		}

		if (s_frame_count == UINT64_MAX) s_frame_count = 0;
		s_frame_count += 1;

		const auto ms_since_epoch =
				duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();

		time_ms = ms_since_epoch;
		time = ms_since_epoch / 1000;

		const duration<double> since_beginning_ms = high_resolution_clock::now() - start_time;
		const double since_beginning_ns =
				duration_cast<std::chrono::nanoseconds>(since_beginning_ms).count();

		geg_time = since_beginning_ns / 1000000000;
		geg_time_ms = since_beginning_ns / 1000000;
		delta_time = geg_time - last_frame;

		last_frame = geg_time;
	}

	uint64_t Timer::now() {
		return time;
	};
	uint64_t Timer::now_ms() {
		return time_ms;
	};
	double Timer::geg_now() {
		return geg_time;
	};
	double Timer::geg_now_ms() {
		return geg_time_ms;
	};
	double Timer::delta() {
		return delta_time;
	}
	uint64_t Timer::frame_count() {
		return s_frame_count;
	}
	uint32_t Timer::fps() {
		return static_cast<uint32_t>(1000 / static_cast<float>(delta_time));
	};

}		 // namespace geg