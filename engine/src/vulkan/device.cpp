#include "device.hpp"

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
		VkInstance instance, VkDebugUtilsMessengerEXT messenger, VkAllocationCallbacks const *pAllocator) {
	return pfnVkDestroyDebugUtilsMessengerEXT(instance, messenger, pAllocator);
}

VKAPI_ATTR VkBool32 VKAPI_CALL debugMessageFunc(
		VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageTypes,
		VkDebugUtilsMessengerCallbackDataEXT const *pCallbackData,
		void * /*pUserData*/) {
	std::ostringstream message;

	message << vk::to_string(static_cast<vk::DebugUtilsMessageSeverityFlagBitsEXT>(messageSeverity)) << ": "
					<< vk::to_string(static_cast<vk::DebugUtilsMessageTypeFlagsEXT>(messageTypes)) << ":\n";
	message << std::string("\t") << "messageIDName   = <" << pCallbackData->pMessageIdName << ">\n";
	message << std::string("\t") << "messageIdNumber = " << pCallbackData->messageIdNumber << "\n";
	message << std::string("\t") << "message         = <" << pCallbackData->pMessage << ">\n";
	if (0 < pCallbackData->queueLabelCount) {
		message << std::string("\t") << "Queue Labels:\n";
		for (uint32_t i = 0; i < pCallbackData->queueLabelCount; i++) {
			message << std::string("\t\t") << "labelName = <" << pCallbackData->pQueueLabels[i].pLabelName << ">\n";
		}
	}
	if (0 < pCallbackData->cmdBufLabelCount) {
		message << std::string("\t") << "CommandBuffer Labels:\n";
		for (uint32_t i = 0; i < pCallbackData->cmdBufLabelCount; i++) {
			message << std::string("\t\t") << "labelName = <" << pCallbackData->pCmdBufLabels[i].pLabelName << ">\n";
		}
	}
	if (0 < pCallbackData->objectCount) {
		message << std::string("\t") << "Objects:\n";
		for (uint32_t i = 0; i < pCallbackData->objectCount; i++) {
			message << std::string("\t\t") << "Object " << i << "\n";
			message << std::string("\t\t\t")
							<< "objectType   = " << vk::to_string(static_cast<vk::ObjectType>(pCallbackData->pObjects[i].objectType))
							<< "\n";
			message << std::string("\t\t\t") << "objectHandle = " << pCallbackData->pObjects[i].objectHandle << "\n";
			if (pCallbackData->pObjects[i].pObjectName) {
				message << std::string("\t\t\t") << "objectName   = <" << pCallbackData->pObjects[i].pObjectName << ">\n";
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
	Device::Device(std::shared_ptr<Window> window) {
		constexpr auto validation_layers = std::array<const char *, 1>{"VK_LAYER_KHRONOS_validation"};

		m_window = window;

		vk::ApplicationInfo applicationInfo{
				.pApplicationName = "GEG",
				.applicationVersion = VK_MAKE_VERSION(0, 0, 1),
				.pEngineName = "GEG",
				.engineVersion = VK_MAKE_VERSION(0, 0, 1),
				.apiVersion = VK_API_VERSION_1_2,
		};

		// required extensions
		uint32_t glfw_exts_count = 0;
		const char **glfw_exts;
		glfw_exts = glfwGetRequiredInstanceExtensions(&glfw_exts_count);

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

		m_instance = vk::createInstance({
				.pApplicationInfo = &applicationInfo,
				.enabledLayerCount = static_cast<uint32_t>(validation_layers_found ? validation_layers.size() : 0),
				.ppEnabledLayerNames = validation_layers_found ? validation_layers.data() : nullptr,
				.enabledExtensionCount = static_cast<uint32_t>(req_exts.size()),
				.ppEnabledExtensionNames = req_exts.data(),
		});

		if (found_all_opt_exts) {
			// check if debug utils is supported
			pfnVkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
					m_instance.getProcAddr("vkCreateDebugUtilsMessengerEXT"));

			GEG_CORE_ASSERT(
					pfnVkCreateDebugUtilsMessengerEXT,
					"GetInstanceProcAddr: Unable to find pfnVkCreateDebugUtilsMessengerEXT function.");

			pfnVkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
					m_instance.getProcAddr("vkDestroyDebugUtilsMessengerEXT"));

			GEG_CORE_ASSERT(
					pfnVkDestroyDebugUtilsMessengerEXT,
					"GetInstanceProcAddr: Unable to find pfnVkDestroyDebugUtilsMessengerEXT function.")

			vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(
					vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
			vk::DebugUtilsMessageTypeFlagsEXT messageTypeFlags(
					vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance |
					vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);

			m_debug_messenger = m_instance.createDebugUtilsMessengerEXT({
					.messageSeverity = severityFlags,
					.messageType = messageTypeFlags,
					.pfnUserCallback = &debugMessageFunc,
			});

			m_debug_messenger_created = true;
			GEG_CORE_INFO("Debug Messenger created");
		}

		// selecting a phiysical device
		auto gpus = m_instance.enumeratePhysicalDevices();
		GEG_CORE_ASSERT(!gpus.empty(), "No GPU with vulkan support found");

		auto found_device = std::find_if(gpus.begin(), gpus.end(), [](const vk::PhysicalDevice &gpu) {
			return gpu.getFeatures().geometryShader && gpu.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu;
		});

		if (found_device == gpus.end()) {
			found_device = std::find_if(
					gpus.begin(), gpus.end(), [](const vk::PhysicalDevice &gpu) { return gpu.getFeatures().geometryShader; });
		}

		GEG_CORE_ASSERT(found_device != gpus.end(), "No suitable GPU found");
		m_physical_device = *found_device;
		GEG_CORE_INFO("Using GPU: {}", m_physical_device.getProperties().deviceName);

		// selecting a queue family
		auto queue_families = m_physical_device.getQueueFamilyProperties();
		GEG_CORE_ASSERT(!queue_families.empty(), "No queue families found");

		// assuming that the qraphics queue family supports presentation
		uint32_t i = 0;
		for (auto &queue_family : queue_families) {
			if (queue_family.queueFlags & vk::QueueFlagBits::eGraphics) {
				m_graphics_queue_family_index = i;
				break;
			}
			i++;
		}
		GEG_CORE_ASSERT(m_graphics_queue_family_index.has_value(), "No graphics queue family found");

		// creating a logical device
		float queue_priority = 1.0f;
		vk::DeviceQueueCreateInfo device_queue_create_info{
				.queueFamilyIndex = m_graphics_queue_family_index.value(),
				.queueCount = 1,
				.pQueuePriorities = &queue_priority,
		};

		m_device = m_physical_device.createDevice({
				.queueCreateInfoCount = 1,
				.pQueueCreateInfos = &device_queue_create_info,
		});

		m_graphics_queue = m_device.getQueue(m_graphics_queue_family_index.value(), 0);
	};

	Device::~Device() {
		GEG_CORE_INFO("destroying vulkan device");
		m_device.destroy();
		if (m_debug_messenger_created) m_instance.destroyDebugUtilsMessengerEXT(m_debug_messenger);
		m_instance.destroy();
	};

	void Device::init(){};
	void Device::destroy(){};

}		 // namespace geg::vulkan
