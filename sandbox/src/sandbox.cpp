#include "core/app.hpp"
#include "core/layer.hpp"
#include "ecs/entity.hpp"
#include "ecs/components.hpp"

class TestLayer: public geg::Layer {
		public:
	TestLayer(): geg::Layer("test") {}

	void on_attach(geg::AssetManager& asset_manager) override {
		namespace cmps = geg::components;

		geg::MeshId mesh = asset_manager.enqueue_mesh("assets/cerberus/mesh.fbx");
		geg::TextureId albedo = asset_manager.enqueue_texture("assets/cerberus/albedo.tga", true, 4);
		geg::TextureId metallic =
				asset_manager.enqueue_texture("assets/cerberus/metallic.tga", false, 1);
		geg::TextureId roughness =
				asset_manager.enqueue_texture("assets/cerberus/roughness.tga", false, 1);

		geg::Entity cerberus = scene.create_entity("cerberus");
		cerberus.add_component<cmps::Mesh>(mesh);
		cerberus.add_component<cmps::Transform>();
		cerberus.add_component<cmps::PBR>(albedo, metallic, roughness);
	};

	void on_detach() override {}
	void update(float ts) override {};
	void ui(float ts) override {};
	void on_event(geg::Event& event) override {};
};

auto main() -> int {
	geg::App::init_logger();
	auto app = geg::App(geg::AppInfo{});
	TestLayer* layer = new TestLayer;
	app.attach_layer(layer);
	app.run();
}
