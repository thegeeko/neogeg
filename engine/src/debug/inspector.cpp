#include "inspector.hpp"
#include "assets/asset-manager.hpp"
#include "ecs/scene.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "ecs/components.hpp"

namespace geg {
	namespace cmps = components;

	namespace ui {
		void draw_vec3(
				const char* label, glm::vec3& vec, float step_size, float reset_value, float title_width) {
			ImGui::Columns(2);
			ImGui::PushID(label);

			ImGui::SetColumnWidth(0, title_width);
			ImGui::Text("%s", label);
			ImGui::NextColumn();

			ImGui::PushMultiItemsWidths(3, ImGui::CalcItemWidth());
			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0, 0});

			float line_height = GImGui->Font->FontSize + GImGui->Style.FramePadding.y * 2.0f;
			ImVec2 button_size{line_height + 3.0f, line_height};

			ImGui::PushStyleColor(ImGuiCol_Button, {0.8f, 0.1f, 0.15f, 1.0f});
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.9f, 0.2f, 0.2f, 1.0f});
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, {0.8f, 0.1f, 0.15f, 1.0f});
			if (ImGui::Button("x")) vec.x = reset_value;
			ImGui::PopStyleColor(3);

			ImGui::SameLine();
			ImGui::DragFloat("##X", &vec.x, step_size);
			ImGui::PopItemWidth();
			ImGui::SameLine();

			ImGui::PushStyleColor(ImGuiCol_Button, {0.2f, 0.7f, 0.2f, 1.0f});
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.3f, 0.8f, 0.3f, 1.0f});
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, {0.2f, 0.7f, 0.2f, 1.0f});
			if (ImGui::Button("y")) vec.y = reset_value;
			ImGui::PopStyleColor(3);

			ImGui::SameLine();
			ImGui::DragFloat("##Y", &vec.y, step_size);
			ImGui::PopItemWidth();
			ImGui::SameLine();

			ImGui::PushStyleColor(ImGuiCol_Button, {0.1f, 0.25f, 0.8f, 1.0f});
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, {0.2f, 0.3f, 0.9f, 1.0f});
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, {0.1f, 0.25f, 0.80f, 1.0f});
			if (ImGui::Button("z")) vec.z = reset_value;
			ImGui::PopStyleColor(3);

			ImGui::SameLine();
			ImGui::DragFloat("##Z", &vec.z, step_size);
			ImGui::PopItemWidth();

			ImGui::PopStyleVar();
			ImGui::PopID();
			ImGui::Columns(1);
		}

		bool draw_text_input(const char* label, char* buffer, size_t size, float title_width) {
			ImGui::Columns(2);

			ImGui::SetColumnWidth(0, title_width);
			ImGui::Text("%s", label);
			ImGui::NextColumn();

			bool typed = ImGui::InputText("##buff", buffer, size);

			ImGui::Columns(1);

			return typed;
		}

		void draw_text(const char* label, const char* text, ImVec4 color, float title_width) {
			ImGui::Columns(2);

			ImGui::SetColumnWidth(0, title_width);
			ImGui::Text("%s", label);
			ImGui::NextColumn();

			ImGui::TextColored(color, "%s", text);

			ImGui::Columns(1);
		}
	}		 // namespace ui

	namespace panels {
		void Scene::draw_panel() {
			ImGui::Begin("Scene");
			m_scene.for_each([this](Entity& entity) {
				const char* name = entity.get_component<cmps::Name>().name.c_str();

				const ImGuiTreeNodeFlags flags =
						((selected_entity == entity) ? ImGuiTreeNodeFlags_Selected : 0);

				const bool opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, "%s", name);
				if (ImGui::IsItemClicked()) selected_entity = entity;
				if (opened) ImGui::TreePop();
			});

			if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered()) selected_entity = {};
			ImGui::End();

			ImGui::Begin("Components");
			draw_entity(selected_entity);
			ImGui::End();
		}

		void draw_entity(Entity& entity) {
			auto& asset_manager = AssetManager::get();
			if (entity.has_component<cmps::Name>()) {
				std::string& name = entity.get_component<cmps::Name>().name;

				char buff[256];
				memset(buff, 0, 256);
				memcpy(buff, name.c_str(), 255);
				if (ui::draw_text_input("Name", buff, 255)) { name = buff; };

				ImGui::Separator();
			}

			if (entity.has_component<cmps::Transform>()) {
				auto& transform = entity.get_component<cmps::Transform>();
				ui::draw_vec3("Translation", transform.translation);
				ui::draw_vec3("Rotation", transform.rotation);
				ui::draw_vec3("Scale", transform.scale, 0.01f, 1.0f);

				ImGui::Separator();
			}

			if (entity.has_component<cmps::Mesh>()) {
				const MeshId mesh_id = entity.get_component<cmps::Mesh>().id;
				const std::string mesh_name = fmt::format("{} - id({})", asset_manager.get_mesh_name(mesh_id), mesh_id);
				ui::draw_text("Mesh", mesh_name.c_str(), {0.2f, 0.7f, 0.2f, 1.0f});
				ImGui::Separator();
			}

			if (entity.has_component<cmps::PBR>()) {
				const auto& pbr = entity.get_component<cmps::PBR>();
				const std::string albedo_name = fmt::format("{} - id({})", asset_manager.get_texture_name(pbr.albedo), pbr.albedo);
				const std::string metallic_name = fmt::format("{} - id({})", asset_manager.get_texture_name(pbr.metallic), pbr.metallic);
				const std::string roughness_name = fmt::format("{} - id({})", asset_manager.get_texture_name(pbr.roughness), pbr.roughness);

				ui::draw_text("Albedo", albedo_name.c_str(), {0.2f, 0.7f, 0.2f, 1.0f});
				ui::draw_text("Metallic", metallic_name.c_str(), {0.2f, 0.7f, 0.2f, 1.0f});
				ui::draw_text("Roughness", roughness_name.c_str(), {0.2f, 0.7f, 0.2f, 1.0f});
				ImGui::Separator();
			}
		}

	}		 // namespace panels
}		 // namespace geg
