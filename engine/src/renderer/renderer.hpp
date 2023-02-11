#pragma once

#include "core/window.hpp"
#include "events/events.hpp"
namespace geg {
	class Renderer {
    public:
    virtual ~Renderer() = default;
		virtual void render() = 0;
    virtual bool resize(WindowResizeEvent) = 0;
	};
}		 // namespace geg
