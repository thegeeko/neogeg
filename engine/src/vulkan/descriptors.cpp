#include "descriptors.hpp"
#include <vulkan/vulkan.hpp>

namespace geg::vulkan {
	DescriptorAllocator::DescriptorAllocator(std::shared_ptr<Device> device) {
		m_device = device;
	}

	DescriptorAllocator::~DescriptorAllocator() {
		for (auto &pool : m_free_pools)
			m_device->logical_device.destroyDescriptorPool(pool);

		for (auto &pool : m_used_pools)
			m_device->logical_device.destroyDescriptorPool(pool);
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

			auto new_pool = m_device->logical_device.createDescriptorPool({
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
			return m_device->logical_device
					.allocateDescriptorSets({
							.descriptorPool = m_current_pool.value(),
							.descriptorSetCount = 1,
							.pSetLayouts = &layout,
					})
					.front();
		} catch (vk::FragmentedPoolError) {
			m_current_pool = get_free_pool();
			return m_device->logical_device
					.allocateDescriptorSets({
							.descriptorPool = m_current_pool.value(),
							.descriptorSetCount = 1,
							.pSetLayouts = &layout,

					})
					.front();
		} catch (vk::OutOfPoolMemoryError) {
			m_current_pool = get_free_pool();
			return m_device->logical_device
					.allocateDescriptorSets({
							.descriptorPool = m_current_pool.value(),
							.descriptorSetCount = 1,
							.pSetLayouts = &layout,

					})
					.front();
		} catch (...) {
			GEG_CORE_ERROR("Unknown error in DescriptorAllocator::allocate");
			return nullptr;
		}
	}

	void DescriptorAllocator::reset_pools() {
		for (auto &pool : m_used_pools) {
			m_device->logical_device.resetDescriptorPool(pool);
			m_free_pools.push_back(pool);
		}
		m_used_pools.clear();
	}

	// DescriptorAllocator

	DescriptorLayoutCache::DescriptorLayoutCache(std::shared_ptr<Device> device) {
		m_device = device;
	}

	DescriptorLayoutCache::~DescriptorLayoutCache() {
		for (auto &[_, layout] : layout_cache) {
			m_device->logical_device.destroyDescriptorSetLayout(layout);
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
			auto layout = m_device->logical_device.createDescriptorSetLayout(*info);

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
  
  // TODO complete the descriptor builder

  
  
}		 // namespace geg::vulkan