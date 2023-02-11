#pragma once

#include "core/window.hpp"
namespace geg {
	class Renderer {
    public:
    virtual ~Renderer() = default;
		virtual void render() = 0;
	};
}		 // namespace geg
