#include "layer-stack.hpp"

namespace geg {
LayerStack::~LayerStack() {
  for (Layer *layer : layers) {
    layer->on_detach();
    delete layer;
  }

  GEG_CORE_INFO("Layer stack cleared");
}

void LayerStack::pushLayer(Layer *layer) {
  layer->on_attach();
  layers.emplace(layers.begin() + layerInsertIndex, layer);
  layerInsertIndex++;
}

void LayerStack::pushOverlay(Layer *overlay) {
  overlay->on_attach();
  layers.emplace_back(overlay);
}

void LayerStack::popLayer(Layer *layer) {
  auto it = std::find(layers.begin(), layers.begin() + layerInsertIndex, layer);
  if (it != layers.begin() + layerInsertIndex) {
    layer->on_detach();
    layers.erase(it);
    layerInsertIndex--;
  }
}

void LayerStack::popOverlay(Layer *overlay) {
  auto it = std::find(layers.begin() + layerInsertIndex, layers.end(), overlay);
  if (it != layers.end()) {
    overlay->on_detach();
    layers.erase(it);
  }
}
} // namespace geg