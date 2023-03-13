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

		ImGui::Begin("color");
		ImGui::ColorPicker4("color", &push.color[0]);
		ImGui::End();

		push.mvp = projection * camera.view_matrix();

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
				m_pipeline_layout, vk::ShaderStageFlagBits::eAllGraphics, 0, sizeof(typeof(push)), &push);
		cmd.bindDescriptorSets(
				vk::PipelineBindPoint::eGraphics, m_pipeline_layout, 0, 1, &m->descriptor_set, 0, nullptr);
		cmd.draw(m->index_count(), 1, 0, 0);

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
				.size = sizeof(glm::vec4) + sizeof(glm::mat4),
		};

		// @TODO: automate this
		auto layout = m_device->build_descriptor()
											.bind_buffer_layout(
													0, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eVertex)
											.bind_buffer_layout(
													1, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eVertex)
											.build_layout()
											.value();

		m_pipeline_layout = m_device->vkdevice.createPipelineLayout(vk::PipelineLayoutCreateInfo{
				.setLayoutCount = 1,
				.pSetLayouts = &layout,
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
