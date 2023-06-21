#include "layer-stack.hpp"

namespace geg {
LayerStack::~LayerStack() {
  for (Layer *layer : layers) {
    layer->on_detach();
    delete layer;
  }

  GEG_CORE_WARN("Layer stack cleared");
}

void LayerStack::pushLayer(Layer *layer) {
  layers.emplace(layers.begin() + layerInsertIndex, layer);
  layerInsertIndex++;
}

void LayerStack::pushOverlay(Layer *overlay) {
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
