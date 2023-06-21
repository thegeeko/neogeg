#include "scene.hpp"
#include "entity.hpp"
#include "components.hpp"

namespace geg {

	Entity Scene::create_entity(const std::string& name) {
		Entity entity(this);
		entity.add_component<components::Name>(name);

		return entity;
	}

	void Scene::for_each(std::function<void(Entity&)> cb) {
		registry.each([this, cb](entt::entity e){
			Entity entity(this, e);
			cb(entity);
		});
	};

	Entity Scene::create_entity() {
		return Entity(this);
	}

}		 // namespace geg
