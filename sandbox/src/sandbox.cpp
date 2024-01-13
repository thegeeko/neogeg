#include "assets/asset-manager.hpp"
#include "core/app.hpp"
#include "core/layer.hpp"
#include "debug/inspector.hpp"
#include "ecs/entity.hpp"
#include "ecs/components.hpp"
#include "vulkan/geg-vulkan.hpp"

namespace cmps = geg::components;
/// the scene is provided by the geg::Layer class
class TestLayer: public geg::Layer {
public:
  TestLayer(): geg::Layer("test"), scene_hierarchy(scene) {}

  void on_attach() override {
    auto& asset_manager = geg::AssetManager::get();
    // asset_manager.load_scene(&scene,
    // "/home/thegeeko/3d-models/gltf/2.0/DamagedHelmet/glTF/DamagedHelmet.gltf");
    asset_manager.load_scene(
        &scene, "/home/thegeeko/3d-models/gltf/2.0/SciFiHelmet/glTF/SciFiHelmet.gltf");

    light = scene.create_entity("light");
    light.add_component<cmps::Light>(cmps::Light{
        .light_color = {1.0f, 1.0f, 1.0f, 300.0f},
    });

    geg::TextureId env_texture =
        asset_manager.enqueue_texture("assets/envmap.hdr", vk::Format::eR32G32B32A32Sfloat, 6);
    geg::Entity env_map = scene.create_entity("env map");
    env_map.add_component<cmps::EnvMap>(cmps::EnvMap{
        .env_map = env_texture,
    });
  };

  void on_detach() override {}
  void update(float ts, geg::CameraPositionerInterface* cam) override {
    scene_hierarchy.draw_panel();

    // light follow cam
    light.get_component<cmps::Transform>().translation = cam->position();
  };
  void ui(float ts) override{};
  void on_event(geg::Event& event) override{};

private:
  geg::panels::Scene scene_hierarchy;
  geg::Entity light;
};

auto main() -> int {
  geg::App::init_logger();
  auto app = geg::App(geg::AppInfo{});
  TestLayer* layer = new TestLayer;
  app.attach_layer(layer);
  app.run();
}
