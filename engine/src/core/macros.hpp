#pragma once

#define GEG_BIND_CB(fn)                                     \
  [this](auto &&...args) -> decltype(auto) {                \
    return this->fn(std::forward<decltype(args)>(args)...); \
  }
