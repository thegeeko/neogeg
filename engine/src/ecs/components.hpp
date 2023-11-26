#pragma once

#include "assets/asset-manager.hpp"
#include "glm/fwd.hpp"
#include "pch.hpp"
#include "glm/glm.hpp"
#include "glm/ext/matrix_transform.hpp"

namespace geg::components {
  struct Name {
    Name() = default;
    Name(std::string name): name(std::move(name)){};
    std::string name = "un-named";

    operator const std::string&() const { return name; };
    operator std::string() const { return name; };
  };

  struct Transform {
    Transform() = default;

    glm::vec3 translation{0};
    glm::vec3 scale{1};
    glm::vec3 rotation{0};

    glm::mat4 model_matrix() const {
      glm::mat4 model_mat = glm::translate(glm::mat4{1}, translation);
      model_mat = glm::rotate(model_mat, glm::radians(rotation.x), {1.f, 0.f, 0.f});    // x
      model_mat = glm::rotate(model_mat, glm::radians(rotation.y), {0.f, 1.f, 0.f});    // y
      model_mat = glm::rotate(model_mat, glm::radians(rotation.z), {0.f, 0.f, 1.f});    // z
      model_mat = glm::scale(model_mat, scale);

      return model_mat;
    }

    glm::mat3 normal_matrix() const {
      glm::mat4 model_mat =
          glm::rotate(glm::mat4{1.0f}, glm::radians(rotation.x), {1.f, 0.f, 0.f});    // x
      model_mat = glm::rotate(model_mat, glm::radians(rotation.y), {0.f, 1.f, 0.f});    // y
      model_mat = glm::rotate(model_mat, glm::radians(rotation.z), {0.f, 0.f, 1.f});    // z
      model_mat = glm::scale(model_mat, {1.0f / scale.x, 1.0f / scale.y, 1.0f / scale.z});

      return model_mat;
    }
  };

  struct Mesh {
    Mesh() = default;
    Mesh(MeshId id): id(id) {}

    operator bool() const { return id > -1; }

    MeshId id = -1;
  };

  struct PBR {
    PBR() = default;
    PBR(TextureId albedo, TextureId metallic, TextureId roughness):
        albedo(albedo), roughness(roughness), metallic(metallic) {}

    operator bool() const { return albedo > -1 && roughness > -1 && metallic > -1; }

    TextureId albedo = -1;
    TextureId roughness = -1;
    TextureId metallic = -1;

    bool albedo_only = false;
    bool roughness_only = false;
    bool metallic_only = false;

    float AO = 0.0f;
  };

  struct Light {
    glm::vec4 light_color{1.0f};
  };
}    // namespace geg::components
