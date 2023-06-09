#include "mesh-renderer.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "imgui.h"

namespace geg::vulkan {

	MeshRenderer::MeshRenderer(
			std::shared_ptr<Device> device,
			std::shared_ptr<Swapchain> swapchain,
			DepthResources depth_resources):
			Renderer(std::move(device), std::move(swapchain), depth_resources) {
		init_pipeline();
	}

	MeshRenderer::~MeshRenderer() {
		delete m;
		m_device->vkdevice.destroyPipeline(m_pipeline);
		m_device->vkdevice.destroyPipelineLayout(m_pipeline_layout);
	}

	void MeshRenderer::fill_commands(
			const vk::CommandBuffer& cmd, const Camera& camera, uint32_t frame_index) {
		begin(cmd, frame_index);

		global_data.proj = projection;
		global_data.view = camera.view_matrix();
		global_data.proj_view = projection * camera.view_matrix();
		global_data.cam_pos = camera.position();

		// update the ubos
		m_global_ubo.write_at_frame(&global_data, sizeof(global_data), 0);
		m_object_ubo.write_at_frame(&objec_data, sizeof(objec_data), 0);

		ImGui::Begin("color");
		ImGui::ColorPicker3("albedo", &objec_data.color[0]);
		ImGui::DragFloat("metallic", &objec_data.metallic, 0.02f, 0, 1.0f);
		ImGui::DragFloat("roughness", &objec_data.roughness, 0.02f, 0, 1.0f);
		ImGui::DragFloat("ao", &objec_data.ao, 0.02f, 0, 1.0f);
		ImGui::End();

		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline);
		cmd.setViewport(
				0,
				vk::Viewport{
						.x = 0,
						.y = 0,
						.width = (float)m_swapchain->extent().width,
						.height = (float)m_swapchain->extent().height,
						.minDepth = 0,
						.maxDepth = 1,
				});
		cmd.setScissor(
				0,
				vk::Rect2D{
						.offset = {0},
						.extent = m_swapchain->extent(),
				});
		cmd.pushConstants(
				m_pipeline_layout, vk::ShaderStageFlagBits::eAllGraphics, 0, sizeof(push_data), &push_data);

		std::array<vk::DescriptorSet, 3> sets = {
				m_global_ubo.descriptor_set,
				m_object_ubo.descriptor_set,
				m->descriptor_set,
		};

		// ubos need the inflight frame index not the general frame index
		std::array<uint32_t, 2> offsets = {
				m_global_ubo.frame_offset(0),
				m_object_ubo.frame_offset(0),
		};

		cmd.bindDescriptorSets(
				vk::PipelineBindPoint::eGraphics,
				m_pipeline_layout,
				0,
				sets.size(),
				sets.data(),
				offsets.size(),
				offsets.data());

		cmd.draw(m->indices_count(), 1, 0, 0);

		cmd.endRenderPass();
	}

	void MeshRenderer::init_pipeline() {
		auto vert_shader_stage = m_shader.vert_stage_info;
		auto frag_shader_stage = m_shader.frag_stage_info;

		auto vertex_input_info = vk::PipelineVertexInputStateCreateInfo{};

		auto input_assembly = vk::PipelineInputAssemblyStateCreateInfo{
				.topology = vk::PrimitiveTopology::eTriangleList,
				.primitiveRestartEnable = VK_FALSE,
		};

		vk::PipelineViewportStateCreateInfo viewport_state{
				.viewportCount = 1,
				.scissorCount = 1,
		};

		std::array<vk::DynamicState, 2> dynamic_states = {
				vk::DynamicState::eViewport,
				vk::DynamicState::eScissor,
		};

		vk::PipelineDynamicStateCreateInfo dynamic_state{
				.dynamicStateCount = static_cast<uint32_t>(dynamic_states.size()),
				.pDynamicStates = dynamic_states.data(),
		};

		auto rasterizer = vk::PipelineRasterizationStateCreateInfo{
				.depthClampEnable = VK_FALSE,
				.rasterizerDiscardEnable = VK_FALSE,
				.polygonMode = vk::PolygonMode::eFill,
				.cullMode = vk::CullModeFlagBits::eNone,
				.frontFace = vk::FrontFace::eClockwise,
				.depthBiasEnable = VK_FALSE,
				.lineWidth = 1.0f,
		};

		auto multisampling = vk::PipelineMultisampleStateCreateInfo{
				.rasterizationSamples = vk::SampleCountFlagBits::e1,
				.sampleShadingEnable = VK_FALSE,
				.minSampleShading = 1.0f,
		};

		auto depth_stencil = vk::PipelineDepthStencilStateCreateInfo{
				.depthTestEnable = VK_TRUE,
				.depthWriteEnable = VK_TRUE,
				.depthCompareOp = vk::CompareOp::eLess,
				.depthBoundsTestEnable = VK_FALSE,
				.stencilTestEnable = VK_FALSE,
		};

		auto color_blend_attachment = vk::PipelineColorBlendAttachmentState{
				.blendEnable = VK_FALSE,
				.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
													vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA,
		};

		auto color_blending = vk::PipelineColorBlendStateCreateInfo{
				.logicOpEnable = VK_FALSE,
				.attachmentCount = 1,
				.pAttachments = &color_blend_attachment,
		};

		vk::PushConstantRange push_range{
				.stageFlags = vk::ShaderStageFlagBits::eAllGraphics,
				.offset = 0,
				.size = sizeof(push_data),
		};

		// @TODO: automate this
		auto gubo_layout = m_global_ubo.descriptor_set_layout;
		auto oubo_layout = m_object_ubo.descriptor_set_layout;
		auto geometry_layout =
				m_device->build_descriptor()
						.bind_buffer_layout(
								0, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eVertex)
						.bind_buffer_layout(
								1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eVertex)
						.build_layout()
						.value();

		const std::array<vk::DescriptorSetLayout, 3> layouts = {
				gubo_layout,
				oubo_layout,
				geometry_layout,
		};

		m_pipeline_layout = m_device->vkdevice.createPipelineLayout(vk::PipelineLayoutCreateInfo{
				.setLayoutCount = layouts.size(),
				.pSetLayouts = layouts.data(),
				.pushConstantRangeCount = 1,
				.pPushConstantRanges = &push_range,
		});

		auto result = m_device->vkdevice.createGraphicsPipeline(
				{},
				{
						.stageCount = 2,
						.pStages = std::array{vert_shader_stage, frag_shader_stage}.data(),
						.pVertexInputState = &vertex_input_info,
						.pInputAssemblyState = &input_assembly,
						.pViewportState = &viewport_state,
						.pRasterizationState = &rasterizer,
						.pMultisampleState = &multisampling,
						.pDepthStencilState = &depth_stencil,
						.pColorBlendState = &color_blending,
						.pDynamicState = &dynamic_state,
						.layout = m_pipeline_layout,
						.renderPass = m_renderpass,
						.subpass = 0,
						.basePipelineHandle = vk::Pipeline{},
						.basePipelineIndex = -1,
				});

		GEG_CORE_ASSERT(result.result == vk::Result::eSuccess, "Failed to create graphics pipeline!");

		m_pipeline = result.value;
	}

}		 // namespace geg::vulkan
