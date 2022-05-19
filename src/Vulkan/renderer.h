// minimalistic code to draw a single triangle, this is not part of the API.
// TODO: Part 1b
#include "shaderc/shaderc.h" // needed for compiling shaders at runtime
#include "build/FSLogo.h"
#ifdef _WIN32 // must use MT platform DLL libraries on windows
	#pragma comment(lib, "shaderc_combined.lib") 
#endif
// Simple Vertex Shader

// Simple Pixel Shader


// Creation, Rendering & Cleanup
class Renderer
{
	// TODO: Part 2b
#define MAX_SUBMESH_PER_DRAW 1024
	struct SHADER_MODEL_DATA {

		GW::MATH::GVECTORF sunDirection, sunColor; // Lighting info
		GW::MATH::GMATRIXF viewMatrix, projectionMatrix; // Viewing
		// Per sub-mesh transform and material data
		GW::MATH::GMATRIXF matricies[MAX_SUBMESH_PER_DRAW];
		OBJ_ATTRIBUTES materials[MAX_SUBMESH_PER_DRAW];
	};

	
	
	// proxy handles
	GW::SYSTEM::GWindow win;
	GW::GRAPHICS::GVulkanSurface vlk;
	GW::CORE::GEventReceiver shutdown;
	
	// what we need at a minimum to draw a triangle
	VkDevice device = nullptr;
	VkBuffer vertexHandle = nullptr;
	VkDeviceMemory vertexData = nullptr;
	// TODO: Part 1g
	VkBuffer indexHandle = nullptr;
	VkDeviceMemory indexData = nullptr;
	// TODO: Part 2c
	std::vector<VkBuffer> storageHandle;
	std::vector<VkDeviceMemory> storageData;

	

	VkShaderModule vertexShader = nullptr;
	VkShaderModule pixelShader = nullptr;
	// pipeline settings for drawing (also required)
	VkPipeline pipeline = nullptr;
	VkPipelineLayout pipelineLayout = nullptr;
	// TODO: Part 2e
	VkDescriptorSetLayout layout = nullptr;
	// TODO: Part 2f
	VkDescriptorPool descriptorPool = nullptr;
	// TODO: Part 2g
	std::vector<VkDescriptorSet> descriptorSet;
	// TODO: Part 4f
	
	// TODO: Part 2a
	GW::MATH::GMATRIXF WorldM;
	GW::MATH::GMATRIXF ViewM;
	GW::MATH::GMATRIXF PerspectiveM;
	
	std::chrono::steady_clock::time_point lastUpdate;
	float deltaTime = 0;
	// Creating proxies
	GW::MATH::GMatrix matrixProxy;

	// TODO: Part 2b
	GW::MATH::GVECTORF LightDir = {-1.0f, -1.0f, 2.0f};
	GW::MATH::GVECTORF LightColor = { 0.9f, 0.9f, 1.0f, 1.0f};
	SHADER_MODEL_DATA ShaderData = {};
	//std::vector<SHADER_MODEL_DATA> SceneData;
	
	
	
	// TODO: Part 4g
public:
	
	struct Obj3D {
		
		OBJ_VEC3 pos; // Left-handed +Z forward coordinate w not provided, assumed to be 1.
		OBJ_VEC3 uvw; // D3D/Vulkan style top left 0,0 coordinate.
		OBJ_VEC3 nrm; // Provided direct from obj file, may or may not be normalized.

	};


	// Load a shader file as a string of characters.
	std::string ShaderAsString(const char* shaderFilePath) {
		std::string output;
		unsigned int stringLength = 0;
		GW::SYSTEM::GFile file; file.Create();
		file.GetFileSize(shaderFilePath, stringLength);
		if (stringLength && +file.OpenBinaryRead(shaderFilePath)) {
			output.resize(stringLength);
			file.Read(&output[0], stringLength);
		}
		else
			std::cout << "ERROR: Shader Source File \"" << shaderFilePath << "\" Not Found!" << std::endl;
		return output;
	}

	Renderer(GW::SYSTEM::GWindow _win, GW::GRAPHICS::GVulkanSurface _vlk)
	{
		win = _win;
		vlk = _vlk;
		unsigned int width, height;
		win.GetClientWidth(width);
		win.GetClientHeight(height);
		// TODO: Part 2a
		matrixProxy.Create();
		matrixProxy.IdentityF(WorldM);
		//matrixProxy.RotateYGlobalF(WorldM, -45, WorldM);
		matrixProxy.LookAtLHF(GW::MATH::GVECTORF{ 0.75f, -0.45f, -1.5f, 1 }, 
			GW::MATH::GVECTORF{ 0.0f, -0.15, 0, 1 },
			GW::MATH::GVECTORF{ 0.0f, 0.50f, 0.0f, 0 }, 
			ViewM);
		float aspectRatio;
		vlk.GetAspectRatio(aspectRatio);
		matrixProxy.ProjectionVulkanLHF(65 * (3.14 / 180), aspectRatio, 0.1f, 100.0f, PerspectiveM);
		
		// Working with backbuffers
		unsigned backBuffersCount = 0;
		vlk.GetSwapchainImageCount(backBuffersCount);
		storageHandle.resize(backBuffersCount);
		storageData.resize(backBuffersCount);
		

		// TODO: Part 2b
		ShaderData.projectionMatrix = PerspectiveM;
		ShaderData.viewMatrix = ViewM;
		ShaderData.sunColor = LightColor;
		ShaderData.sunDirection = LightDir;
		for (size_t i = 0; i < MAX_SUBMESH_PER_DRAW; i++)
		{
			ShaderData.matricies[i] = WorldM;
			ShaderData.materials[i] = FSLogo_materials[i].attrib;
		}
		// TODO: Part 4g
		// TODO: part 3b

		/***************** GEOMETRY INTIALIZATION ******************/
		// Grab the device & physical device so we can allocate some stuff
		VkPhysicalDevice physicalDevice = nullptr;
		vlk.GetDevice((void**)&device);
		vlk.GetPhysicalDevice((void**)&physicalDevice);
		
		// TODO: Part 1c
		// Create Vertex Buffer
		Obj3D verts[3885] = {};

		for (int i = 0; i < 3885; i++)
		{
			verts[i].pos.x = FSLogo_vertices[i].pos.x;
			verts[i].pos.y = FSLogo_vertices[i].pos.y;
			verts[i].pos.z = FSLogo_vertices[i].pos.z;

			verts[i].nrm.x = FSLogo_vertices[i].nrm.x;
			verts[i].nrm.y = FSLogo_vertices[i].nrm.y;
			verts[i].nrm.z = FSLogo_vertices[i].nrm.z;

			verts[i].uvw.x = FSLogo_vertices[i].uvw.x;
			verts[i].uvw.y = FSLogo_vertices[i].uvw.y;
			verts[i].uvw.z = FSLogo_vertices[i].uvw.z;
		}
		// Transfer triangle data to the vertex buffer. (staging would be prefered here)
		GvkHelper::create_buffer(physicalDevice, device, sizeof(verts),
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &vertexHandle, &vertexData);
		GvkHelper::write_to_buffer(device, vertexData, verts, sizeof(verts));
		// TODO: Part 1g
		GvkHelper::create_buffer(physicalDevice, device, sizeof(FSLogo_indices), 
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
			VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &indexHandle, &indexData);
		GvkHelper::write_to_buffer(device, indexData, FSLogo_indices, sizeof(FSLogo_indices));

		// TODO: Part 2d
		for (size_t i = 0; i < backBuffersCount; i++)
		{
			GvkHelper::create_buffer(physicalDevice, device, sizeof(ShaderData), 
				VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
				VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &storageHandle[i], &storageData[i]);
			GvkHelper::write_to_buffer(device, storageData[i], &ShaderData, sizeof(SHADER_MODEL_DATA));
		}
		/***************** SHADER INTIALIZATION ******************/
		// Intialize runtime shader compiler HLSL -> SPIRV
		shaderc_compiler_t compiler = shaderc_compiler_initialize();
		shaderc_compile_options_t options = shaderc_compile_options_initialize();
		shaderc_compile_options_set_source_language(options, shaderc_source_language_hlsl);
		shaderc_compile_options_set_invert_y(options, false); // TODO: Part 2i
#ifndef NDEBUG
		shaderc_compile_options_set_generate_debug_info(options);
#endif
		// Create Vertex Shader
		std::string vertexSHString = ShaderAsString("../VertexShader.hlsl");
		shaderc_compilation_result_t result = shaderc_compile_into_spv( // compile
			compiler, vertexSHString.c_str(), strlen(vertexSHString.c_str()),
			shaderc_vertex_shader, "main.vert", "main", options);
		if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
			std::cout << "Vertex Shader Errors: " << shaderc_result_get_error_message(result) << std::endl;
		GvkHelper::create_shader_module(device, shaderc_result_get_length(result), // load into Vulkan
			(char*)shaderc_result_get_bytes(result), &vertexShader);
		shaderc_result_release(result); // done
		// Create Pixel Shader
		std::string pixelSHString = ShaderAsString("../FragmentShader.hlsl");
		result = shaderc_compile_into_spv( // compile
			compiler, pixelSHString.c_str(), strlen(pixelSHString.c_str()),
			shaderc_fragment_shader, "main.frag", "main", options);
		if (shaderc_result_get_compilation_status(result) != shaderc_compilation_status_success) // errors?
			std::cout << "Pixel Shader Errors: " << shaderc_result_get_error_message(result) << std::endl;
		GvkHelper::create_shader_module(device, shaderc_result_get_length(result), // load into Vulkan
			(char*)shaderc_result_get_bytes(result), &pixelShader);
		shaderc_result_release(result); // done
		// Free runtime shader compiler resources
		shaderc_compile_options_release(options);
		shaderc_compiler_release(compiler);

		/***************** PIPELINE INTIALIZATION ******************/
		// Create Pipeline & Layout (Thanks Tiny!)
		VkRenderPass renderPass;
		vlk.GetRenderPass((void**)&renderPass);
		VkPipelineShaderStageCreateInfo stage_create_info[2] = {};
		// Create Stage Info for Vertex Shader
		stage_create_info[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage_create_info[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
		stage_create_info[0].module = vertexShader;
		stage_create_info[0].pName = "main";
		// Create Stage Info for Fragment Shader
		stage_create_info[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		stage_create_info[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		stage_create_info[1].module = pixelShader;
		stage_create_info[1].pName = "main";
		// Assembly State
		VkPipelineInputAssemblyStateCreateInfo assembly_create_info = {};
		assembly_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		assembly_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		assembly_create_info.primitiveRestartEnable = false;
		// TODO: Part 1e
		// Vertex Input State
		VkVertexInputBindingDescription vertex_binding_description = {};
		vertex_binding_description.binding = 0;
		vertex_binding_description.stride = sizeof(Obj3D);
		vertex_binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		VkVertexInputAttributeDescription vertex_attribute_description[3] = {
			{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, 0 }, //uv, normal, etc....
			{ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, 12 }, //uv, normal, etc....
			{ 2, 0, VK_FORMAT_R32G32B32_SFLOAT, 24 } //uv, normal, etc....
		};
		VkPipelineVertexInputStateCreateInfo input_vertex_info = {};
		input_vertex_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		input_vertex_info.vertexBindingDescriptionCount = 1;
		input_vertex_info.pVertexBindingDescriptions = &vertex_binding_description;
		input_vertex_info.vertexAttributeDescriptionCount = 3;
		input_vertex_info.pVertexAttributeDescriptions = vertex_attribute_description;
		// Viewport State (we still need to set this up even though we will overwrite the values)
		VkViewport viewport = {
            0, 0, static_cast<float>(width), static_cast<float>(height), 0, 1
        };
        VkRect2D scissor = { {0, 0}, {width, height} };
		VkPipelineViewportStateCreateInfo viewport_create_info = {};
		viewport_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewport_create_info.viewportCount = 1;
		viewport_create_info.pViewports = &viewport;
		viewport_create_info.scissorCount = 1;
		viewport_create_info.pScissors = &scissor;
		// Rasterizer State
		VkPipelineRasterizationStateCreateInfo rasterization_create_info = {};
		rasterization_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterization_create_info.rasterizerDiscardEnable = VK_FALSE;
		rasterization_create_info.polygonMode = VK_POLYGON_MODE_FILL;
		rasterization_create_info.lineWidth = 1.0f;
		rasterization_create_info.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterization_create_info.frontFace = VK_FRONT_FACE_CLOCKWISE;
		rasterization_create_info.depthClampEnable = VK_FALSE;
		rasterization_create_info.depthBiasEnable = VK_FALSE;
		rasterization_create_info.depthBiasClamp = 0.0f;
		rasterization_create_info.depthBiasConstantFactor = 0.0f;
		rasterization_create_info.depthBiasSlopeFactor = 0.0f;
		// Multisampling State
		VkPipelineMultisampleStateCreateInfo multisample_create_info = {};
		multisample_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisample_create_info.sampleShadingEnable = VK_FALSE;
		multisample_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisample_create_info.minSampleShading = 1.0f;
		multisample_create_info.pSampleMask = VK_NULL_HANDLE;
		multisample_create_info.alphaToCoverageEnable = VK_FALSE;
		multisample_create_info.alphaToOneEnable = VK_FALSE;
		// Depth-Stencil State
		VkPipelineDepthStencilStateCreateInfo depth_stencil_create_info = {};
		depth_stencil_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depth_stencil_create_info.depthTestEnable = VK_TRUE;
		depth_stencil_create_info.depthWriteEnable = VK_TRUE;
		depth_stencil_create_info.depthCompareOp = VK_COMPARE_OP_LESS;
		depth_stencil_create_info.depthBoundsTestEnable = VK_FALSE;
		depth_stencil_create_info.minDepthBounds = 0.0f;
		depth_stencil_create_info.maxDepthBounds = 1.0f;
		depth_stencil_create_info.stencilTestEnable = VK_FALSE;
		// Color Blending Attachment & State
		VkPipelineColorBlendAttachmentState color_blend_attachment_state = {};
		color_blend_attachment_state.colorWriteMask = 0xF;
		color_blend_attachment_state.blendEnable = VK_FALSE;
		color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_COLOR;
		color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_DST_COLOR;
		color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
		color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
		color_blend_attachment_state.alphaBlendOp = VK_BLEND_OP_ADD;
		VkPipelineColorBlendStateCreateInfo color_blend_create_info = {};
		color_blend_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		color_blend_create_info.logicOpEnable = VK_FALSE;
		color_blend_create_info.logicOp = VK_LOGIC_OP_COPY;
		color_blend_create_info.attachmentCount = 1;
		color_blend_create_info.pAttachments = &color_blend_attachment_state;
		color_blend_create_info.blendConstants[0] = 0.0f;
		color_blend_create_info.blendConstants[1] = 0.0f;
		color_blend_create_info.blendConstants[2] = 0.0f;
		color_blend_create_info.blendConstants[3] = 0.0f;
		// Dynamic State 
		VkDynamicState dynamic_state[2] = { 
			// By setting these we do not need to re-create the pipeline on Resize
			VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR
		};
		VkPipelineDynamicStateCreateInfo dynamic_create_info = {};
		dynamic_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamic_create_info.dynamicStateCount = 2;
		dynamic_create_info.pDynamicStates = dynamic_state;
		
		// TODO: Part 2e
		VkDescriptorSetLayoutBinding layoutBinding = {};
		layoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		layoutBinding.binding = 0;
		layoutBinding.descriptorCount = 1;
		layoutBinding.pImmutableSamplers = nullptr;
		layoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		//layoutBinding.stageFlags = VK_SHADER_STAGE_ALL;

		VkDescriptorSetLayoutCreateInfo createInfo = {};
		createInfo.pBindings = &layoutBinding;
		createInfo.bindingCount = 1;
		createInfo.flags = 0;
		createInfo.pNext = nullptr;
		createInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;

		VkResult r = vkCreateDescriptorSetLayout(device, &createInfo, nullptr, &layout);

		// TODO: Part 2f
		VkDescriptorPoolCreateInfo poolCreateInfo = {};
		poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolCreateInfo.flags = 0;
		poolCreateInfo.maxSets = backBuffersCount;
		poolCreateInfo.pNext = nullptr;
		poolCreateInfo.poolSizeCount = 1;

		VkDescriptorPoolSize poolSize = {};
		poolSize.descriptorCount = backBuffersCount;
		poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;

		poolCreateInfo.pPoolSizes = &poolSize;
		
		vkCreateDescriptorPool(device, &poolCreateInfo, nullptr, &descriptorPool);
			// TODO: Part 4f
		// TODO: Part 2g
		VkDescriptorSetAllocateInfo descriptorAllocateInfo = {};
		descriptorAllocateInfo.descriptorPool = descriptorPool;
		descriptorAllocateInfo.descriptorSetCount = 1;
		descriptorAllocateInfo.pNext = nullptr;
		descriptorAllocateInfo.pSetLayouts = &layout;
		descriptorAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		descriptorSet.resize(backBuffersCount);
		for (size_t i = 0; i < backBuffersCount; i++)
		{
			vkAllocateDescriptorSets(device, &descriptorAllocateInfo, &descriptorSet[i]);
		}
			// TODO: Part 4f
		// TODO: Part 2h
		VkWriteDescriptorSet writeDescriptorSet = {};
		writeDescriptorSet.descriptorCount = 1;
		writeDescriptorSet.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		writeDescriptorSet.dstArrayElement = 0;
	
		writeDescriptorSet.dstBinding = 0;
		writeDescriptorSet.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;

		for (size_t i = 0; i < backBuffersCount; i++)
		{
			writeDescriptorSet.dstSet = descriptorSet[i];
			VkDescriptorBufferInfo dbInfo = {};
			dbInfo.buffer = storageHandle[i];
			dbInfo.offset = 0;
			dbInfo.range = VK_WHOLE_SIZE;
			
			writeDescriptorSet.pBufferInfo = &dbInfo;
			vkUpdateDescriptorSets(device, 1, &writeDescriptorSet, 0, nullptr);
		}
			// TODO: Part 4f
	
		// Descriptor pipeline layout
		VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
		pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		// TODO: Part 2e
		pipeline_layout_create_info.setLayoutCount = 1;
		pipeline_layout_create_info.pSetLayouts = &layout;
		
		// TODO: Part 3c
		VkPushConstantRange pushConstant = {};
		pushConstant.size = sizeof(unsigned int);
		pushConstant.offset = 0;
		pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		pipeline_layout_create_info.pushConstantRangeCount = 1;
		pipeline_layout_create_info.pPushConstantRanges = &pushConstant;
		//pipeline_layout_create_info.flags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
		
		vkCreatePipelineLayout(device, &pipeline_layout_create_info, 
			nullptr, &pipelineLayout);
	    // Pipeline State... (FINALLY) 
		VkGraphicsPipelineCreateInfo pipeline_create_info = {};
		pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipeline_create_info.stageCount = 2;
		pipeline_create_info.pStages = stage_create_info;
		pipeline_create_info.pInputAssemblyState = &assembly_create_info;
		pipeline_create_info.pVertexInputState = &input_vertex_info;
		pipeline_create_info.pViewportState = &viewport_create_info;
		pipeline_create_info.pRasterizationState = &rasterization_create_info;
		pipeline_create_info.pMultisampleState = &multisample_create_info;
		pipeline_create_info.pDepthStencilState = &depth_stencil_create_info;
		pipeline_create_info.pColorBlendState = &color_blend_create_info;
		pipeline_create_info.pDynamicState = &dynamic_create_info;
		pipeline_create_info.layout = pipelineLayout;
		pipeline_create_info.renderPass = renderPass;
		pipeline_create_info.subpass = 0;
		pipeline_create_info.basePipelineHandle = VK_NULL_HANDLE;
		vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, 
			&pipeline_create_info, nullptr, &pipeline);

		/***************** CLEANUP / SHUTDOWN ******************/
		// GVulkanSurface will inform us when to release any allocated resources
		shutdown.Create(vlk, [&]() {
			if (+shutdown.Find(GW::GRAPHICS::GVulkanSurface::Events::RELEASE_RESOURCES, true)) {
				CleanUp(); // unlike D3D we must be careful about destroy timing
			}
		});
	}
	
	void Render()
	{
		// TODO: Part 2a
		auto now = std::chrono::steady_clock::now();
		deltaTime = std::chrono::duration_cast<std::chrono::microseconds>(now - lastUpdate).count() / 1000000.0f;
		//matrixProxy.TranslateLocalF(WorldM, , WorldM);
		//matrixProxy.IdentityF(WorldM);
		GW::MATH::GVECTORF ogPos = {};
		matrixProxy.GetTranslationF(WorldM, ogPos);
		matrixProxy.RotateYGlobalF(WorldM, deltaTime, WorldM);
		matrixProxy.TranslateGlobalF(WorldM, ogPos, WorldM);
		this->ShaderData.matricies[1] = WorldM;
		for (size_t i = 0; i < 2; i++)
		{
			GvkHelper::write_to_buffer(device, storageData[i], &this->ShaderData, sizeof(SHADER_MODEL_DATA));
		}
		
		//matrixProxy.MultiplyMatrixF(ViewM, PerspectiveM, ViewM);
		//matrixProxy.MultiplyMatrixF(WorldM, ViewM, WorldM);
		// TODO: Part 4d
		// grab the current Vulkan commandBuffer
		unsigned int currentBuffer;
		vlk.GetSwapchainCurrentImage(currentBuffer);
		VkCommandBuffer commandBuffer;
		vlk.GetCommandBuffer(currentBuffer, (void**)&commandBuffer);
		// what is the current client area dimensions?
		unsigned int width, height;
		win.GetClientWidth(width);
		win.GetClientHeight(height);
		// setup the pipeline's dynamic settings
		VkViewport viewport = {
            0, 0, static_cast<float>(width), static_cast<float>(height), 0, 1
        };
        VkRect2D scissor = { {0, 0}, {width, height} };
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		
		// now we can draw
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexHandle, offsets);
		// TODO: Part 1h
		vkCmdBindIndexBuffer(commandBuffer, indexHandle, 0, VK_INDEX_TYPE_UINT32);
		
		// TODO: Part 4d
		// TODO: Part 2i
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout,
			0, 1, &descriptorSet[currentBuffer], 0, nullptr);
		// TODO: Part 3b
		for (size_t i = 0; i < FSLogo_meshcount; i++)
		{
			vkCmdPushConstants(commandBuffer, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | 
			VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(unsigned int), &FSLogo_meshes[i].materialIndex);
			vkCmdDrawIndexed(commandBuffer, FSLogo_meshes[i].indexCount, 1, FSLogo_meshes[i].indexOffset, 0, i);
		}
		// TODO: Part 3d
		//vkCmdDraw(commandBuffer, 3885, 1, 0, 0); // TODO: Part 1d, 1h
		//vkCmdDrawIndexed(commandBuffer, 8532, 1, 0, 0, 0);
		//vkCmdDrawIndexed(commandBuffer, 5988, 1, 0, 0, 0);
		//vkCmdDrawIndexed(commandBuffer, 2544, 1, 5988, 0, 1);
		
		lastUpdate = now;
	}
	
private:
	
	void CleanUp()
	{
		// wait till everything has completed
		vkDeviceWaitIdle(device);
		// Release allocated buffers, shaders & pipeline
		// TODO: Part 1g
		vkDestroyBuffer(device, indexHandle, nullptr);
		vkFreeMemory(device, indexData, nullptr);
		// TODO: Part 2d
		for (size_t i = 0; i < storageHandle.size(); i++) { vkDestroyBuffer(device, storageHandle[i], nullptr); }
		for (size_t i = 0; i < storageData.size(); i++) { vkFreeMemory(device, storageData[i], nullptr); }

		vkDestroyBuffer(device, vertexHandle, nullptr);
		vkFreeMemory(device, vertexData, nullptr);
		vkDestroyShaderModule(device, vertexShader, nullptr);
		vkDestroyShaderModule(device, pixelShader, nullptr);
		// TODO: Part 2e
		vkDestroyDescriptorSetLayout(device, layout, nullptr);
		// TODO: part 2f
		vkDestroyDescriptorPool(device, descriptorPool, nullptr);
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
		vkDestroyPipeline(device, pipeline, nullptr);
	}
};
