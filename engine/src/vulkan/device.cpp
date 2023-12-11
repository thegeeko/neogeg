#include "device.hpp"
#include "vk_mem_alloc.h"

PFN_vkCreateDebugUtilsMessengerEXT pfnVkCreateDebugUtilsMessengerEXT;
PFN_vkDestroyDebugUtilsMessengerEXT pfnVkDestroyDebugUtilsMessengerEXT;

VKAPI_ATTR VkResult VKAPI_CALL vkCreateDebugUtilsMessengerEXT(
    VkInstance instance,
    const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
    const VkAllocationCallbacks *pAllocator,
    VkDebugUtilsMessengerEXT *pMessenger) {
  return pfnVkCreateDebugUtilsMessengerEXT(instance, pCreateInfo, pAllocator, pMessenger);
}

VKAPI_ATTR void VKAPI_CALL vkDestroyDebugUtilsMessengerEXT(
    VkInstance instance,
    VkDebugUtilsMessengerEXT messenger,
    VkAllocationCallbacks const *pAllocator) {
  return pfnVkDestroyDebugUtilsMessengerEXT(instance, messenger, pAllocator);
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugMessageFunc(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageTypes,
    VkDebugUtilsMessengerCallbackDataEXT const *pCallbackData,
    void * /*pUserData*/) {
  std::ostringstream message;

  message << vk::to_string(static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(messageSeverity))
          << ": " << vk::to_string(static_cast<vk::DebugUtilsMessageTypeFlagsEXT>(messageTypes))
          << ":\n";
  message << std::string("\t") << "messageIDName   = <" << pCallbackData->pMessageIdName << ">\n";
  message << std::string("\t") << "messageIdNumber = " << pCallbackData->messageIdNumber << "\n";
  message << std::string("\t") << "message         = <" << pCallbackData->pMessage << ">\n";
  if (0 < pCallbackData->queueLabelCount) {
    message << std::string("\t") << "Queue Labels:\n";
    for (uint32_t i = 0; i < pCallbackData->queueLabelCount; i++) {
      message << std::string("\t\t") << "labelName = <" << pCallbackData->pQueueLabels[i].pLabelName
              << ">\n";
    }
  }
  if (0 < pCallbackData->cmdBufLabelCount) {
    message << std::string("\t") << "CommandBuffer Labels:\n";
    for (uint32_t i = 0; i < pCallbackData->cmdBufLabelCount; i++) {
      message << std::string("\t\t") << "labelName = <"
              << pCallbackData->pCmdBufLabels[i].pLabelName << ">\n";
    }
  }
  if (0 < pCallbackData->objectCount) {
    message << std::string("\t") << "Objects:\n";
    for (uint32_t i = 0; i < pCallbackData->objectCount; i++) {
      message << std::string("\t\t") << "Object " << i << "\n";
      message << std::string("\t\t\t") << "objectType   = "
              << vk::to_string(static_cast<vk::ObjectType>(pCallbackData->pObjects[i].objectType))
              << "\n";
      message << std::string("\t\t\t")
              << "objectHandle = " << pCallbackData->pObjects[i].objectHandle << "\n";
      if (pCallbackData->pObjects[i].pObjectName) {
        message << std::string("\t\t\t") << "objectName   = <"
                << pCallbackData->pObjects[i].pObjectName << ">\n";
      }
    }
  }

  if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
    GEG_CORE_ERROR(message.str());
  } else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
    GEG_CORE_WARN(message.str());
  } else if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) {
    GEG_CORE_INFO(message.str());
  } else {
    GEG_CORE_TRACE(message.str());
  }

  return false;
}

namespace geg::vulkan {
  Device::Device(const std::shared_ptr<Window> &window) {
    constexpr auto validation_layers = std::array<const char *, 1>{"VK_LAYER_KHRONOS_validation"};

    this->window = window;

    vk::ApplicationInfo applicationInfo{
        .pApplicationName = "GEG Game",
        .applicationVersion = VK_MAKE_VERSION(0, 0, 1),
        .pEngineName = "GEG",
        .engineVersion = VK_MAKE_VERSION(0, 0, 1),
        .apiVersion = VK_API_VERSION_1_3,
    };

    // required extensions
    uint32_t glfw_exts_count = 0;
    const char **glfw_exts = glfwGetRequiredInstanceExtensions(&glfw_exts_count);

    auto req_exts = std::vector<const char *>(glfw_exts, glfw_exts + glfw_exts_count);
    auto opt_exts = std::vector<const char *>{VK_EXT_DEBUG_UTILS_EXTENSION_NAME};

    auto avaliable_extensions = vk::enumerateInstanceExtensionProperties();

    int found_req_exts = 0;
    int found_opt_exts = 0;
    for (auto &ext : avaliable_extensions) {
      for (auto &req_ext : req_exts)
        if (strcmp(ext.extensionName, req_ext) == 0) { found_req_exts++; }

      for (auto &opt_ext : opt_exts)
        if (strcmp(ext.extensionName, opt_ext) == 0) { found_opt_exts++; }
    }

    bool found_all_req_exts = found_req_exts == req_exts.size();
    bool found_all_opt_exts = found_opt_exts == opt_exts.size();

    GEG_CORE_ASSERT(found_all_req_exts, "Required extensions not found");
    if (!found_all_opt_exts) GEG_CORE_WARN("Optional extensions not found");

    if (found_all_opt_exts) req_exts.insert(req_exts.end(), opt_exts.begin(), opt_exts.end());

    // validation layers
    bool validation_layers_found = false;
    auto layers = vk::enumerateInstanceLayerProperties();
    for (auto &layer : layers) {
      if (strcmp(layer.layerName, validation_layers[0]) == 0) {
        validation_layers_found = true;
        break;
      }
    }

    if (!validation_layers_found) GEG_CORE_WARN("Validation layers not found");

    instance = vk::createInstance({
        .pApplicationInfo = &applicationInfo,
        .enabledLayerCount =
            static_cast<uint32_t>(validation_layers_found ? validation_layers.size() : 0),
        .ppEnabledLayerNames = validation_layers_found ? validation_layers.data() : nullptr,
        .enabledExtensionCount = static_cast<uint32_t>(req_exts.size()),
        .ppEnabledExtensionNames = req_exts.data(),
    });

    if (found_all_opt_exts) {
      // check if debug utils is supported
      pfnVkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
          instance.getProcAddr("vkCreateDebugUtilsMessengerEXT"));

      GEG_CORE_ASSERT(
          pfnVkCreateDebugUtilsMessengerEXT,
          "GetInstanceProcAddr: Unable to find pfnVkCreateDebugUtilsMessengerEXT function.");

      pfnVkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
          instance.getProcAddr("vkDestroyDebugUtilsMessengerEXT"));

      GEG_CORE_ASSERT(
          pfnVkDestroyDebugUtilsMessengerEXT,
          "GetInstanceProcAddr: Unable to find pfnVkDestroyDebugUtilsMessengerEXT function.")

      vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(
          vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning |
          vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
      vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
          vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral |
          vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
          vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);

      m_debug_messenger = instance.createDebugUtilsMessengerEXT({
          .messageSeverity = severityFlags,
          .messageType = messageTypeFlags,
          .pfnUserCallback = &debugMessageFunc,
      });

      m_debug_messenger_created = true;
      GEG_CORE_INFO("Debug Messenger created");
    }

    // create surface
    // this cuz glfw doesn't take vkhpp types
    VkSurfaceKHR tmp_surface;
    glfwCreateWindowSurface(instance, window->raw_pointer, nullptr, &tmp_surface);
    surface = tmp_surface;

    // selecting a phiysical device
    auto gpus = instance.enumeratePhysicalDevices();
    GEG_CORE_ASSERT(!gpus.empty(), "No GPU with vulkan support found");

    auto found_device = std::find_if(gpus.begin(), gpus.end(), [](const vk::PhysicalDevice &gpu) {
      return gpu.getFeatures().geometryShader &&
             gpu.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu;
    });

    if (found_device == gpus.end()) {
      found_device = std::find_if(gpus.begin(), gpus.end(), [](const vk::PhysicalDevice &gpu) {
        return gpu.getFeatures().geometryShader;
      });
    }

    GEG_CORE_ASSERT(found_device != gpus.end(), "No suitable GPU found");
    physical_device = *found_device;
    GEG_CORE_INFO("Using GPU: {}", physical_device.getProperties().deviceName);

    // selecting a queue family
    auto queue_families = physical_device.getQueueFamilyProperties();
    GEG_CORE_ASSERT(!queue_families.empty(), "No queue families found");

    // assuming that the qraphics queue family supports presentation
    // @TODO support when they are not the same queue
    uint32_t i = 0;
    for (auto &queue_family : queue_families) {
      if ((queue_family.queueFlags & vk::QueueFlagBits::eGraphics | vk::QueueFlagBits::eCompute |
           vk::QueueFlagBits::eTransfer) &&
          physical_device.getSurfaceSupportKHR(i, surface)) {
        queue_family_index = i;
        break;
      }
      i++;
    }
    GEG_CORE_ASSERT(
        queue_family_index.has_value(),
        "No graphics queue family found, fixing this is on my todo");

    // creating a logical device
    float queue_priority = 1.0f;
    vk::DeviceQueueCreateInfo device_queue_create_info{
        .queueFamilyIndex = queue_family_index.value(),
        .queueCount = 1,
        .pQueuePriorities = &queue_priority,
    };

    bool dynamic_rendering = false;
    const std::vector<vk::ExtensionProperties> properties =
        physical_device.enumerateDeviceExtensionProperties();
    for (auto &prop : properties) {
      if (std::strcmp(prop.extensionName, VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME))
        dynamic_rendering = true;
    }
    GEG_CORE_ASSERT(dynamic_rendering, "dynamic rendering extension required");

    vk::PhysicalDeviceDescriptorIndexingFeaturesEXT device_indexing_features = {
        .shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
        .descriptorBindingVariableDescriptorCount = VK_TRUE,
        .runtimeDescriptorArray = VK_TRUE,
    };

    vk::PhysicalDeviceDynamicRenderingFeatures dynamic_rendering_features = {
        .pNext = &device_indexing_features,
        .dynamicRendering = VK_TRUE,
    };

    const vk::PhysicalDeviceFeatures device_features = {
        .shaderSampledImageArrayDynamicIndexing = VK_TRUE,
    };

    const vk::PhysicalDeviceFeatures2 device_features2 = {
        .pNext = &dynamic_rendering_features,
        .features = device_features,
    };

    std::array<const char *, 1> device_extensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    vkdevice = physical_device.createDevice({
        .pNext = &device_features2,
        .queueCreateInfoCount = 1,
        .pQueueCreateInfos = &device_queue_create_info,
        .enabledExtensionCount = static_cast<uint32_t>(1),
        .ppEnabledExtensionNames = device_extensions.data(),
    });

    // TODO: improve this
    graphics_queue = vkdevice.getQueue(queue_family_index.value(), 0);

    // creating main command pool
    command_pool = vkdevice.createCommandPool(vk::CommandPoolCreateInfo{
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = queue_family_index.value(),
    });

    VmaAllocatorCreateInfo allocator_info{
        .physicalDevice = physical_device,
        .device = vkdevice,
        .instance = instance,
        .vulkanApiVersion = VK_API_VERSION_1_2,
    };
    vmaCreateAllocator(&allocator_info, &allocator);

    m_descriptor_allocator = std::make_unique<DescriptorAllocator>(this);
    m_descriptor_layout_cache = std::make_unique<DescriptorLayoutCache>(this);
  };

  Device::~Device() {
    GEG_CORE_WARN("destroying vulkan device");
    m_descriptor_layout_cache.reset();
    m_descriptor_allocator.reset();
    vmaDestroyAllocator(allocator);
    vkdevice.destroyCommandPool(command_pool);
    vkdevice.destroy();
    instance.destroySurfaceKHR(surface);
    if (m_debug_messenger_created) instance.destroyDebugUtilsMessengerEXT(m_debug_messenger);
    instance.destroy();
  }

  void Device::single_time_command(const std::function<void(vk::CommandBuffer)> &lambda) {
    auto command_buffer = vkdevice
                              .allocateCommandBuffers({
                                  .commandPool = command_pool,
                                  .level = vk::CommandBufferLevel::ePrimary,
                                  .commandBufferCount = 1,
                              })
                              .front();

    vk::CommandBufferUsageFlags flgs = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
    command_buffer.begin({
        .flags = flgs,
    });

    lambda(command_buffer);

    command_buffer.end();

    graphics_queue.submit(vk::SubmitInfo{
        .commandBufferCount = 1,
        .pCommandBuffers = &command_buffer,
    });

    graphics_queue.waitIdle();
    vkdevice.freeCommandBuffers(command_pool, {command_buffer});
  }

  void Device::copy_buffer(
      vk::Buffer src, vk::Buffer dst, vk::DeviceSize size, vk::CommandBuffer cmd) {
    vk::BufferCopy copy_region{
        .srcOffset = 0,
        .dstOffset = 0,
        .size = size,
    };

    cmd.copyBuffer(src, dst, copy_region);
  }

  void Device::upload_to_buffer(vk::Buffer buffer, void *data, vk::DeviceSize size) {
    auto buffer_info = static_cast<VkBufferCreateInfo>(vk::BufferCreateInfo{
        .size = size,
        .usage = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc,
        .sharingMode = vk::SharingMode::eExclusive,
    });

    auto flags =
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;

    VmaAllocationCreateInfo alloc_info{
        .usage = VMA_MEMORY_USAGE_CPU_COPY,
        .requiredFlags = static_cast<uint32_t>(flags),
    };

    VkBuffer staging_buffer = nullptr;
    VmaAllocation staging_allocation = nullptr;

    vmaCreateBuffer(
        allocator, &buffer_info, &alloc_info, &staging_buffer, &staging_allocation, nullptr);

    void *mapping_addr = nullptr;
    vmaMapMemory(allocator, staging_allocation, &mapping_addr);

    memcpy(mapping_addr, data, size);
    vmaUnmapMemory(allocator, staging_allocation);

    single_time_command([&](auto cmd) { copy_buffer(staging_buffer, buffer, size, cmd); });
    vmaDestroyBuffer(allocator, staging_buffer, staging_allocation);
  }

  void Device::upload_to_image(
      vk::Image image,
      vk::ImageLayout layout_after,
      vk::Format format,
      vk::Extent3D extent,
      const void *data,
      vk::DeviceSize size,
      uint32_t mip_levels) {
    auto buffer_info = static_cast<VkBufferCreateInfo>(vk::BufferCreateInfo{
        .size = size,
        .usage = vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eTransferSrc,
        .sharingMode = vk::SharingMode::eExclusive,
    });

    auto flags =
        vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;

    VmaAllocationCreateInfo alloc_info{
        .usage = VMA_MEMORY_USAGE_CPU_COPY,
        .requiredFlags = static_cast<uint32_t>(flags),
    };

    VkBuffer staging_buffer = nullptr;
    VmaAllocation staging_allocation = nullptr;

    vmaCreateBuffer(
        allocator, &buffer_info, &alloc_info, &staging_buffer, &staging_allocation, nullptr);

    void *mapping_addr = nullptr;
    vmaMapMemory(allocator, staging_allocation, &mapping_addr);

    memcpy(mapping_addr, data, size);
    vmaUnmapMemory(allocator, staging_allocation);

    single_time_command([&](auto cmd) {
      transition_image_layout(
          image, format, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal, cmd, mip_levels);
      copy_buffer_to_image(staging_buffer, image, extent, cmd);
      transition_image_layout(
          image, format, vk::ImageLayout::eTransferDstOptimal, layout_after, cmd, mip_levels);
    });

    vmaDestroyBuffer(allocator, staging_buffer, staging_allocation);
  }

  void Device::copy_buffer_to_image(
      vk::Buffer src, vk::Image dst, vk::Extent3D image_extent, vk::CommandBuffer cmd) {
    const vk::BufferImageCopy copy_region{
        .bufferOffset = 0,
        .bufferRowLength = 0,
        .bufferImageHeight = 0,
        .imageSubresource{
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .mipLevel = 0,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
        .imageExtent = image_extent,
    };

    cmd.copyBufferToImage(src, dst, vk::ImageLayout::eTransferDstOptimal, {copy_region});
  }    // namespace geg::vulkan

  void Device::transition_image_layout(
      vk::Image image,
      vk::Format,
      vk::ImageLayout old_layout,
      vk::ImageLayout new_layout,
      vk::CommandBuffer cmd,
      uint32_t mip_levels,
      uint32_t base_level) {
    if (old_layout == new_layout) return;

    vk::ImageMemoryBarrier barrier{
        .oldLayout = old_layout,
        .newLayout = new_layout,
        .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
        .image = image,
        .subresourceRange{
            .aspectMask = vk::ImageAspectFlagBits::eColor,
            .baseMipLevel = base_level,
            .levelCount = mip_levels,
            .baseArrayLayer = 0,
            .layerCount = 1,
        },
    };

    vk::PipelineStageFlags src_stage;
    vk::PipelineStageFlags dst_stage;

    if (old_layout == vk::ImageLayout::eUndefined &&
        new_layout == vk::ImageLayout::eTransferDstOptimal) {
      barrier.srcAccessMask = vk::AccessFlagBits::eNone;
      barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

      src_stage = vk::PipelineStageFlagBits::eTopOfPipe;
      dst_stage = vk::PipelineStageFlagBits::eTransfer;
    } else if (
        old_layout == vk::ImageLayout::eTransferDstOptimal &&
        new_layout == vk::ImageLayout::eTransferSrcOptimal) {
      barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
      barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

      src_stage = vk::PipelineStageFlagBits::eTransfer;
      dst_stage = vk::PipelineStageFlagBits::eTransfer;
    } else if (
        old_layout == vk::ImageLayout::eTransferDstOptimal &&
        new_layout == vk::ImageLayout::eShaderReadOnlyOptimal) {
      barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
      barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

      src_stage = vk::PipelineStageFlagBits::eTransfer;
      dst_stage = vk::PipelineStageFlagBits::eFragmentShader;
    } else if (
        old_layout == vk::ImageLayout::eTransferSrcOptimal &&
        new_layout == vk::ImageLayout::eShaderReadOnlyOptimal) {
      barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
      barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

      src_stage = vk::PipelineStageFlagBits::eTransfer;
      dst_stage = vk::PipelineStageFlagBits::eFragmentShader;
    } else if (
        old_layout == vk::ImageLayout::eColorAttachmentOptimal &&
        new_layout == vk::ImageLayout::ePresentSrcKHR) {
      barrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

      src_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
      dst_stage = vk::PipelineStageFlagBits::eTopOfPipe;
    } else if (
        old_layout == vk::ImageLayout::eUndefined &&
        new_layout == vk::ImageLayout::eColorAttachmentOptimal) {
      barrier.srcAccessMask = vk::AccessFlagBits::eNone;
      barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;

      src_stage = vk::PipelineStageFlagBits::eTopOfPipe;
      dst_stage = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    } else if (
        old_layout == vk::ImageLayout::eUndefined && new_layout == vk::ImageLayout::eGeneral) {
      barrier.srcAccessMask = vk::AccessFlagBits::eNone;
      barrier.dstAccessMask = vk::AccessFlagBits::eShaderWrite;

      src_stage = vk::PipelineStageFlagBits::eTopOfPipe;
      dst_stage = vk::PipelineStageFlagBits::eComputeShader;
    } else {
      GEG_CORE_ERROR("Unsupported image transition");
    }

    cmd.pipelineBarrier(src_stage, dst_stage, vk::DependencyFlags(0), nullptr, nullptr, {barrier});
  }

}    // namespace geg::vulkan
