#include "descriptors.hpp"
#include <vulkan/vulkan.hpp>

#include "vulkan/device.hpp"

namespace geg::vulkan {
	DescriptorAllocator::DescriptorAllocator(Device *device) {
		m_device = device;
	}

	DescriptorAllocator::~DescriptorAllocator() {
		for (auto &pool : m_free_pools)
			m_device->vkdevice.destroyDescriptorPool(pool);

		for (auto &pool : m_used_pools)
			m_device->vkdevice.destroyDescriptorPool(pool);
	}

	vk::DescriptorPool DescriptorAllocator::get_free_pool() {
		if (m_free_pools.empty()) {
			std::vector<vk::DescriptorPoolSize> sizes;
			sizes.reserve(number_of_descriptors.size());

			uint32_t count = 1000;
			for (auto [type, num] : number_of_descriptors) {
				// the second is the count of the descriptor type
				sizes.push_back({type, uint32_t(num * count)});
			}

			auto new_pool = m_device->vkdevice.createDescriptorPool({
					.maxSets = count,
					.poolSizeCount = static_cast<uint32_t>(sizes.size()),
					.pPoolSizes = sizes.data(),
			});

			m_used_pools.push_back(new_pool);
			return new_pool;

		} else {
			auto pool = m_free_pools.back();
			m_free_pools.pop_back();
			m_used_pools.push_back(pool);
			return pool;
		}
	}

	std::optional<vk::DescriptorSet> DescriptorAllocator::allocate(vk::DescriptorSetLayout layout) {
		if (!m_current_pool.has_value()) { m_current_pool = get_free_pool(); }

		try {
			return m_device->vkdevice
					.allocateDescriptorSets({
							.descriptorPool = m_current_pool.value(),
							.descriptorSetCount = 1,
							.pSetLayouts = &layout,
					})
					.front();
		} catch (vk::FragmentedPoolError) {
			m_current_pool = get_free_pool();
			return m_device->vkdevice
					.allocateDescriptorSets({
							.descriptorPool = m_current_pool.value(),
							.descriptorSetCount = 1,
							.pSetLayouts = &layout,

					})
					.front();
		} catch (vk::OutOfPoolMemoryError) {
			m_current_pool = get_free_pool();
			return m_device->vkdevice
					.allocateDescriptorSets({
							.descriptorPool = m_current_pool.value(),
							.descriptorSetCount = 1,
							.pSetLayouts = &layout,

					})
					.front();
		} catch (...) {
			GEG_CORE_ERROR("Unknown error in DescriptorAllocator::allocate");
			return {};
		}
	}

	void DescriptorAllocator::reset_pools() {
		for (auto &pool : m_used_pools) {
			m_device->vkdevice.resetDescriptorPool(pool);
			m_free_pools.push_back(pool);
		}
		m_used_pools.clear();
		m_current_pool = {};
	}

	// DescriptorAllocator

	DescriptorLayoutCache::DescriptorLayoutCache(Device *device) {
		m_device = device;
	}

	DescriptorLayoutCache::~DescriptorLayoutCache() {
		for (auto &[_, layout] : layout_cache) {
			m_device->vkdevice.destroyDescriptorSetLayout(layout);
		}
	}

	vk::DescriptorSetLayout DescriptorLayoutCache::create_layout(
			vk::DescriptorSetLayoutCreateInfo *info) {
		DescriptorLayoutInfo layout_info;
		layout_info.bindings.reserve(info->bindingCount);
		bool is_sorted = true;
		int last_binding = -1;

		// copy from the direct info struct into our own one
		for (int i = 0; i < info->bindingCount; i++) {
			layout_info.bindings.push_back(info->pBindings[i]);

			// check that the bindings are in strict increasing order
			if (info->pBindings[i].binding > last_binding) {
				last_binding = info->pBindings[i].binding;
			} else {
				is_sorted = false;
			}
		}

		// sort the bindings if they aren't in order
		if (!is_sorted) {
			std::sort(
					layout_info.bindings.begin(),
					layout_info.bindings.end(),
					[](vk::DescriptorSetLayoutBinding &a, vk::DescriptorSetLayoutBinding &b) {
						return a.binding < b.binding;
					});
		}

		// try to grab from cache
		auto it = layout_cache.find(layout_info);
		if (it != layout_cache.end()) {
			return (*it).second;
		} else {
			// create a new one (not found)
			auto layout = m_device->vkdevice.createDescriptorSetLayout(*info);

			// add to cache
			layout_cache[layout_info] = layout;
			return layout;
		}
	}

	bool DescriptorLayoutCache::DescriptorLayoutInfo::operator==(
			const DescriptorLayoutInfo &other) const {
		if (other.bindings.size() != bindings.size()) {
			return false;
		} else {
			// compare each of the bindings is the same. Bindings are sorted so they will match
			for (int i = 0; i < bindings.size(); i++) {
				if (other.bindings[i].binding != bindings[i].binding) { return false; }
				if (other.bindings[i].descriptorType != bindings[i].descriptorType) { return false; }
				if (other.bindings[i].descriptorCount != bindings[i].descriptorCount) { return false; }
				if (other.bindings[i].stageFlags != bindings[i].stageFlags) { return false; }
			}
			return true;
		}
	}

	size_t DescriptorLayoutCache::DescriptorLayoutInfo::hash() const {
		using std::hash;
		using std::size_t;

		size_t result = hash<size_t>()(bindings.size());

		for (const vk::DescriptorSetLayoutBinding &b : bindings) {
			// pack the binding data into a single int64. Not fully correct but it's ok
			size_t binding_hash = b.binding | static_cast<uint32_t>(b.descriptorType) << 8 |
														b.descriptorCount << 16 | static_cast<uint32_t>(b.stageFlags) << 24;

			// shuffle the packed binding data and xor it with the main hash
			result ^= hash<size_t>()(binding_hash);
		}

		return result;
	}

	// DescriptorLayoutCache

	DescriptorBuilder DescriptorBuilder::begin(
			DescriptorLayoutCache *layoutCache, DescriptorAllocator *allocator) {
		DescriptorBuilder db{};
		db.m_cache = layoutCache;
		db.m_alloc = allocator;
		return db;
	}

	DescriptorBuilder &DescriptorBuilder::bind_buffer(
			uint32_t binding,
			vk::DescriptorBufferInfo *buffer_info,
			vk::DescriptorType type,
			vk::ShaderStageFlags stage_flags) {
		vk::DescriptorSetLayoutBinding new_binding{};
		new_binding.descriptorCount = 1;
		new_binding.descriptorType = type;
		new_binding.stageFlags = stage_flags;
		new_binding.binding = binding;

		bindings.push_back(new_binding);

		// create the descriptor write
		vk::WriteDescriptorSet new_write{};
		new_write.descriptorCount = 1;
		new_write.descriptorType = type;
		new_write.pBufferInfo = buffer_info;
		new_write.dstBinding = binding;

		writes.push_back(new_write);
		return *this;
	}

	DescriptorBuilder &DescriptorBuilder::bind_buffer_layout(
			uint32_t binding, vk::DescriptorType type, vk::ShaderStageFlags stage_flags) {
		vk::DescriptorSetLayoutBinding new_binding{};

		new_binding.descriptorCount = 1;
		new_binding.descriptorType = type;
		new_binding.stageFlags = stage_flags;
		new_binding.binding = binding;

		bindings.push_back(new_binding);

		return *this;
	}

	DescriptorBuilder &DescriptorBuilder::bind_image(
			uint32_t binding,
			vk::DescriptorImageInfo *image_info,
			vk::DescriptorType type,
			vk::ShaderStageFlags stage_flags) {
		vk::DescriptorSetLayoutBinding new_binding{};
		new_binding.descriptorCount = 1;
		new_binding.descriptorType = type;
		new_binding.stageFlags = stage_flags;
		new_binding.binding = binding;

		bindings.push_back(new_binding);

		// create the descriptor write
		vk::WriteDescriptorSet new_write{};
		new_write.descriptorCount = 1;
		new_write.descriptorType = type;
		new_write.pImageInfo = image_info;
		new_write.dstBinding = binding;

		writes.push_back(new_write);
		return *this;
	}

	DescriptorBuilder &DescriptorBuilder::bind_image_layout(
			uint32_t binding, vk::DescriptorType type, vk::ShaderStageFlags stage_flags) {
		vk::DescriptorSetLayoutBinding new_binding{};
		new_binding.descriptorCount = 1;
		new_binding.descriptorType = type;
		new_binding.stageFlags = stage_flags;
		new_binding.binding = binding;

		bindings.push_back(new_binding);

		return *this;
	}

	std::optional<std::pair<vk::DescriptorSet, vk::DescriptorSetLayout>> DescriptorBuilder::build() {
		// build layout first
		vk::DescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.pBindings = bindings.data();
		layoutInfo.bindingCount = bindings.size();

		auto layout = m_cache->create_layout(&layoutInfo);

		// allocate descriptor
		auto set = m_alloc->allocate(layout);
		if (!set.has_value()) { return {}; };

		// write descriptor
		for (VkWriteDescriptorSet &w : writes) {
			w.dstSet = set.value();
		}
		auto device = m_alloc->m_device->vkdevice;
		device.updateDescriptorSets(writes.size(), writes.data(), 0, nullptr);

		return std::pair(set.value(), layout);
	}
	std::optional<vk::DescriptorSetLayout> DescriptorBuilder::build_layout() {
		vk::DescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.pBindings = bindings.data();
		layoutInfo.bindingCount = bindings.size();

		return m_cache->create_layout(&layoutInfo);
	}
}		 // namespace geg::vulkan
