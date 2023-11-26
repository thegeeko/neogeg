#pragma once

namespace geg {
  class Timer {
  public:
    static void update();
    static uint64_t now();
    static uint64_t now_ms();
    static double geg_now();
    static double geg_now_ms();
    static double delta();
    static uint64_t frame_count();
    static uint32_t fps();
  };
}    // namespace geg