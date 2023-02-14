#include <memory>
#include "geg-vulkan.hpp"
#include <vulkan/vulkan_enums.hpp>
#include "vulkan/device.hpp"

namespace geg::vulkan {
	class DescriptorAllocator {
	public:
		DescriptorAllocator(std::shared_ptr<Device> device);
		~DescriptorAllocator();

		void reset_pools();
		std::optional<vk::DescriptorSet> allocate(vk::DescriptorSetLayout layout);

	private:
		std::shared_ptr<Device> m_device;
		std::optional<vk::DescriptorPool> m_current_pool = nullptr;
		// this can be tweaked based on the usage of the descriptor sets
		// the first is the descriptor type, the second is the number of descriptors of that
		// DescriptorType
		std::vector<std::pair<vk::DescriptorType, float>> number_of_descriptors = {
				{vk::DescriptorType::eSampler, 0.5f},
				{vk::DescriptorType::eCombinedImageSampler, 4.f},
				{vk::DescriptorType::eSampledImage, 4.f},
				{vk::DescriptorType::eStorageImage, 1.f},
				{vk::DescriptorType::eUniformTexelBuffer, 1.f},
				{vk::DescriptorType::eStorageTexelBuffer, 1.f},
				{vk::DescriptorType::eUniformBuffer, 2.f},
				{vk::DescriptorType::eStorageBuffer, 2.f},
				{vk::DescriptorType::eUniformBufferDynamic, 1.f},
				{vk::DescriptorType::eStorageBufferDynamic, 1.f},
				{vk::DescriptorType::eInputAttachment, 0.5f}};

		std::vector<VkDescriptorPool> m_used_pools;
		std::vector<VkDescriptorPool> m_free_pools;

		vk::DescriptorPool get_free_pool();
	};

	class DescriptorLayoutCache {
	public:
		DescriptorLayoutCache(std::shared_ptr<Device> device);
		~DescriptorLayoutCache();

		vk::DescriptorSetLayout create_layout(vk::DescriptorSetLayoutCreateInfo* info);

		struct DescriptorLayoutInfo {
			std::vector<vk::DescriptorSetLayoutBinding> bindings;

			bool operator==(const DescriptorLayoutInfo& other) const;

			size_t hash() const;
		};

	private:
		struct DescriptorLayoutHash {
			std::size_t operator()(const DescriptorLayoutInfo& k) const { return k.hash(); }
		};

		std::unordered_map<DescriptorLayoutInfo, vk::DescriptorSetLayout, DescriptorLayoutHash>
				layout_cache;
		std::shared_ptr<Device> m_device;
	};

	class DescriptorBuilder {
	public:
		static DescriptorBuilder begin(
				DescriptorLayoutCache* layoutCache, DescriptorAllocator* allocator);

		DescriptorBuilder& bind_buffer(
				uint32_t binding,
				VkDescriptorBufferInfo* bufferInfo,
				VkDescriptorType type,
				VkShaderStageFlags stageFlags);

    // for building a descriptor set layout only
		DescriptorBuilder& bind_buffer_layout(
				uint32_t binding, VkDescriptorType type, VkShaderStageFlags stageFlags);

		DescriptorBuilder& bind_image(
				uint32_t binding,
				VkDescriptorImageInfo* imageInfo,
				VkDescriptorType type,
				VkShaderStageFlags stageFlags);

		DescriptorBuilder& bind_image_layout(
				uint32_t binding, VkDescriptorType type, VkShaderStageFlags stageFlags);

		bool build(VkDescriptorSet& set, VkDescriptorSetLayout& layout);
		bool build(VkDescriptorSetLayout& layout);

	private:
		DescriptorBuilder() = default;

		std::vector<VkWriteDescriptorSet> writes;
		std::vector<VkDescriptorSetLayoutBinding> bindings;

		DescriptorLayoutCache* cache;
		DescriptorAllocator* alloc;
	};

}		 // namespace geg::vulkan
