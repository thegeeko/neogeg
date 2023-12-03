#include "scene.hpp"
#include "entity.hpp"
#include "components.hpp"

namespace geg {

  Entity Scene::create_entity(const std::string& name) {
    Entity entity(this);
    entity.add_component<components::Name>(name);
    entity.add_component<components::Transform>();

    return entity;
  }

  void Scene::for_each(std::function<void(Entity&)> cb) {
    registry.each([this, cb](entt::entity e) {
      Entity entity(this, e);
      cb(entity);
    });
  };

  Entity Scene::create_entity() {
    Entity entity(this);
    entity.add_component<components::Name>("untitled :(");
    entity.add_component<components::Transform>();

    return entity;
  }

  void Scene::delete_entity(Entity entity) {
    registry.destroy(entity);
  }


}    // namespace geg
