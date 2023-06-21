#pragma once

#include "entt/entt.hpp"

namespace geg {
	class Entity;

	class Scene {
	public:
		Scene() = default;
		~Scene() = default;

		Entity create_entity();
		Entity create_entity(const std::string& name);
		entt::registry& get_reg() { return registry; }

		void for_each(std::function<void(Entity&)> cb);
	private:
		entt::registry registry;
	};
}		 // namespace geg
