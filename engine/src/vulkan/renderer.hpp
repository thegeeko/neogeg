#include "renderer/renderer.hpp"

namespace geg {
class VulkanRenderer : public Renderer {
public:
  VulkanRenderer(std::shared_ptr<Window> window);
  void render() override;

private:
  void init();

  std::shared_ptr<Window> m_window;
};
} // namespace geg
