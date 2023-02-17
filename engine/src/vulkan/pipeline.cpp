#include "pipeline.hpp"

namespace geg::vulkan {
	GraphicsPipeline::GraphicsPipeline(
			std::shared_ptr<Device> device,
			std::shared_ptr<Renderpass> renderpass,
			const Shader& shader) {
		m_device = device;
		m_renderpass = renderpass;

		auto vert_shader_stage = shader.vert_stage_info;
		auto frag_shader_stage = shader.frag_stage_info;

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

		pipeline_layout = m_device->logical_device.createPipelineLayout(vk::PipelineLayoutCreateInfo{});
		auto result = m_device->logical_device.createGraphicsPipeline(
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
						.layout = pipeline_layout,
						.renderPass = m_renderpass->render_pass,
						.subpass = 0,
						.basePipelineHandle = vk::Pipeline{},
						.basePipelineIndex = -1,
				});

		GEG_CORE_ASSERT(result.result == vk::Result::eSuccess, "Failed to create graphics pipeline!");

		pipeline = result.value;
	};

	GraphicsPipeline::~GraphicsPipeline() {
		m_device->logical_device.destroyPipeline(pipeline);
		m_device->logical_device.destroyPipelineLayout(pipeline_layout);
	}
}		 // namespace geg::vulkan
