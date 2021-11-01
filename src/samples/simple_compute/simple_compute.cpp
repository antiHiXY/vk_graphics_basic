#include "simple_compute.h"

#include <vk_pipeline.h>
#include <vk_buffers.h>
#include <vk_utils.h>

#include <array>

SimpleCompute::SimpleCompute(uint32_t a_length) : m_length(a_length)
{
#ifdef NDEBUG
  m_enableValidation = false;
#else
  m_enableValidation = true;
#endif
}

void SimpleCompute::SetupValidationLayers()
{
  m_validationLayers.push_back("VK_LAYER_KHRONOS_validation");
  m_validationLayers.push_back("VK_LAYER_LUNARG_monitor");
}

void SimpleCompute::InitVulkan(const char** a_instanceExtensions, uint32_t a_instanceExtensionsCount, uint32_t a_deviceId)
{
  m_instanceExtensions.clear();
  for (uint32_t i = 0; i < a_instanceExtensionsCount; ++i) {
    m_instanceExtensions.push_back(a_instanceExtensions[i]);
  }
  SetupValidationLayers();
  VK_CHECK_RESULT(volkInitialize());
  CreateInstance();
  volkLoadInstance(m_instance);

  CreateDevice(a_deviceId);
  volkLoadDevice(m_device);

  m_commandPool = vk_utils::createCommandPool(m_device, m_queueFamilyIDXs.compute, VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

  m_cmdBufferCompute = vk_utils::createCommandBuffers(m_device, m_commandPool, 1)[0];

  m_pCopyHelper = std::make_unique<vk_utils::SimpleCopyHelper>(m_physicalDevice, m_device, m_transferQueue, m_queueFamilyIDXs.compute, 8*1024*1024);
}


void SimpleCompute::CreateInstance()
{
  VkApplicationInfo appInfo = {};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pNext = nullptr;
  appInfo.pApplicationName = "VkRender";
  appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
  appInfo.pEngineName = "SimpleCompute";
  appInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
  appInfo.apiVersion = VK_MAKE_VERSION(1, 1, 0);

  m_instance = vk_utils::createInstance(m_enableValidation, m_validationLayers, m_instanceExtensions, &appInfo);
  if (m_enableValidation)
    vk_utils::initDebugReportCallback(m_instance, &debugReportCallbackFn, &m_debugReportCallback);
}

void SimpleCompute::CreateDevice(uint32_t a_deviceId)
{
  m_physicalDevice = vk_utils::findPhysicalDevice(m_instance, true, a_deviceId, m_deviceExtensions);

  m_device = vk_utils::createLogicalDevice(m_physicalDevice, m_validationLayers, m_deviceExtensions,
                                           m_enabledDeviceFeatures, m_queueFamilyIDXs,
                                           VK_QUEUE_COMPUTE_BIT | VK_QUEUE_TRANSFER_BIT);

  vkGetDeviceQueue(m_device, m_queueFamilyIDXs.compute, 0, &m_computeQueue);
  vkGetDeviceQueue(m_device, m_queueFamilyIDXs.transfer, 0, &m_transferQueue);
}


void SimpleCompute::SetupSimplePipeline()
{
  std::vector<std::pair<VkDescriptorType, uint32_t> > dtypes = {
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,             8}
  };

  m_descriptorSets.resize(3);
  m_descriptorSetLayouts.resize(2);

  // Создание и аллокация буферов
  m_A = vk_utils::createBuffer(m_device, sizeof(float) * m_length, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
  m_sum = vk_utils::createBuffer(m_device, sizeof(float) * m_groupsAmount, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
  m_sum_final = vk_utils::createBuffer(m_device, sizeof(float) * m_length, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
  vk_utils::allocateAndBindWithPadding(m_device, m_physicalDevice, {m_A, m_sum, m_sum_final}, 0);

  m_pBindings = std::make_unique<vk_utils::DescriptorMaker>(m_device, dtypes, 3);

  // Создание descriptor set для передачи буферов в шейдер
  m_pBindings->BindBegin(VK_SHADER_STAGE_COMPUTE_BIT);
  m_pBindings->BindBuffer(0, m_A);
  m_pBindings->BindBuffer(1, m_sum);
  m_pBindings->BindBuffer(2, m_sum_final);
  m_pBindings->BindEnd(m_descriptorSets.data(), m_descriptorSetLayouts.data());

  m_pBindings->BindBegin(VK_SHADER_STAGE_COMPUTE_BIT);
  m_pBindings->BindBuffer(0, m_sum);
  m_pBindings->BindBuffer(1, m_sum);
  m_pBindings->BindBuffer(2, m_sum);
  m_pBindings->BindEnd(m_descriptorSets.data() + 1, m_descriptorSetLayouts.data());

  m_pBindings->BindBegin(VK_SHADER_STAGE_COMPUTE_BIT);
  m_pBindings->BindBuffer(0, m_sum);
  m_pBindings->BindBuffer(1, m_sum_final);
  m_pBindings->BindEnd(m_descriptorSets.data() + 2, m_descriptorSetLayouts.data() + 1);

  // Заполнение буферов
  std::vector<float> values(m_length);
  for (uint32_t i = 0; i < values.size(); ++i) {
    values[i] = (float)i;
  }
  m_pCopyHelper->UpdateBuffer(m_A, 0, values.data(), sizeof(float) * values.size());
}

void SimpleCompute::BuildCommandBufferSimple(VkCommandBuffer a_cmdBuff, VkPipeline)
{
  vkResetCommandBuffer(a_cmdBuff, 0);

  m_pipelines.resize(2);
  //m_layouts.resize(2);

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
  // Заполняем буфер команд
  VK_CHECK_RESULT(vkBeginCommandBuffer(a_cmdBuff, &beginInfo));

  vkCmdBindPipeline(a_cmdBuff, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelines[0]);
  vkCmdBindDescriptorSets(a_cmdBuff, VK_PIPELINE_BIND_POINT_COMPUTE, m_layouts[0], 0, 1, m_descriptorSets.data(), 0, NULL);
  vkCmdPushConstants(a_cmdBuff, m_layouts[0], VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(m_length), &m_length);
  vkCmdDispatch(a_cmdBuff, m_groupsAmount, 1, 1);


  std::array barriers
  {
      VkBufferMemoryBarrier
      {
          .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
          .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
          .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
          .buffer = m_sum,
          .offset = 0,
          .size = sizeof(float) * m_groupsAmount
      },
      VkBufferMemoryBarrier
      {
          .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER,
          .srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT,
          .dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
          .buffer = m_sum_final,
          .offset = 0,
          .size = sizeof(float) * m_length,
      }
  };

  vkCmdPipelineBarrier(m_cmdBufferCompute, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       {}, 0, nullptr, barriers.size(), barriers.data(), 0, nullptr);

  vkCmdBindDescriptorSets(a_cmdBuff, VK_PIPELINE_BIND_POINT_COMPUTE, m_layouts[0], 0, 1, m_descriptorSets.data() + 1, 0, NULL);
  vkCmdPushConstants(a_cmdBuff, m_layouts[0], VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(m_groupsAmount), &m_groupsAmount);
  vkCmdDispatch(a_cmdBuff, 1, 1, 1);

  vkCmdPipelineBarrier(m_cmdBufferCompute, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
                       {}, 0, nullptr, 1, barriers.data(), 0, nullptr);

  vkCmdBindPipeline(a_cmdBuff, VK_PIPELINE_BIND_POINT_COMPUTE, m_pipelines[1]);
  vkCmdBindDescriptorSets(a_cmdBuff, VK_PIPELINE_BIND_POINT_COMPUTE, m_layouts[1], 0, 1, m_descriptorSets.data() + 2, 0, NULL);
  vkCmdPushConstants(a_cmdBuff, m_layouts[1], VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(m_length), &m_length);
  vkCmdDispatch(a_cmdBuff, m_groupsAmount, 1, 1);

  VK_CHECK_RESULT(vkEndCommandBuffer(a_cmdBuff));
}


void SimpleCompute::CleanupPipeline()
{
  if (m_cmdBufferCompute)
  {
    vkFreeCommandBuffers(m_device, m_commandPool, 1, &m_cmdBufferCompute);
  }

  vkDestroyBuffer(m_device, m_A, nullptr);
  vkDestroyBuffer(m_device, m_sum, nullptr);
  vkDestroyBuffer(m_device, m_sum_final, nullptr);

  for (int i = 0; i < m_layouts.size(); ++i)
  {
      vkDestroyPipelineLayout(m_device, m_layouts[i], nullptr);
  }
  for (int i = 0; i < m_pipelines.size(); ++i)
  {
      vkDestroyPipeline(m_device, m_pipelines[i], nullptr);
  }
}


void SimpleCompute::Cleanup()
{
  CleanupPipeline();

  if (m_commandPool != VK_NULL_HANDLE)
  {
    vkDestroyCommandPool(m_device, m_commandPool, nullptr);
  }
}


void SimpleCompute::CreateComputePipeline()
{
  m_layouts.resize(2);
  // Загружаем шейдер
  std::vector<uint32_t> code = vk_utils::readSPVFile("../resources/shaders/simple.comp.spv");
  std::vector<uint32_t> code_s = vk_utils::readSPVFile("../resources/shaders/dimple.comp.spv");
  VkShaderModuleCreateInfo createInfo
  {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = code.size() * sizeof(uint32_t),
      .pCode = code.data()
  };

  VkShaderModuleCreateInfo createInfo_s
  {
      .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
      .codeSize = code_s.size() * sizeof(uint32_t),
      .pCode = code_s.data()
  };


  VkShaderModule shaderModule;
  VkShaderModule shaderModule_s;
  // Создаём шейдер в вулкане
  VK_CHECK_RESULT(vkCreateShaderModule(m_device, &createInfo, NULL, &shaderModule));
  VK_CHECK_RESULT(vkCreateShaderModule(m_device, &createInfo_s, NULL, &shaderModule_s));

  VkPipelineShaderStageCreateInfo shaderStageCreateInfo
  {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_COMPUTE_BIT,
      .module = shaderModule,
      .pName = "main"
  };


  VkPipelineShaderStageCreateInfo shaderStageCreateInfo_s
  {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
      .stage = VK_SHADER_STAGE_COMPUTE_BIT,
      .module = shaderModule_s,
      .pName = "main"
  };

  VkPushConstantRange pcRange
  {
      .stageFlags = VK_SHADER_STAGE_COMPUTE_BIT,
      .offset = 0,
      .size = sizeof(m_length) * 2
  };

  // Создаём layout для pipeline
  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo
  {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 1,
      .pSetLayouts = m_descriptorSetLayouts.data(),
      .pushConstantRangeCount = 1,
      .pPushConstantRanges = &pcRange
  };

  VK_CHECK_RESULT(vkCreatePipelineLayout(m_device, &pipelineLayoutCreateInfo, NULL, &m_layouts[0]));

  VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo_s
  {
      .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
      .setLayoutCount = 1,
      .pSetLayouts = m_descriptorSetLayouts.data() + 1,
      .pushConstantRangeCount = 1,
      .pPushConstantRanges = &pcRange
  };

  VK_CHECK_RESULT(vkCreatePipelineLayout(m_device, &pipelineLayoutCreateInfo_s, NULL, &m_layouts[1]));

  std::array pipelineCreateInfos
  {
    VkComputePipelineCreateInfo
    {
      .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
      .stage = shaderStageCreateInfo,
      .layout = m_layouts[0]
    },
    VkComputePipelineCreateInfo
    {
      .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
      .stage = shaderStageCreateInfo_s,
      .layout = m_layouts[1]
    }
  };
  m_pipelines.resize(pipelineCreateInfos.size());
  // Создаём pipeline - объект, который выставляет шейдер и его параметры
  VK_CHECK_RESULT(vkCreateComputePipelines(m_device, VK_NULL_HANDLE, pipelineCreateInfos.size(), pipelineCreateInfos.data(), NULL, m_pipelines.data()));

  vkDestroyShaderModule(m_device, shaderModule, nullptr);
}


void SimpleCompute::Execute()
{
  SetupSimplePipeline();
  CreateComputePipeline();

  BuildCommandBufferSimple(m_cmdBufferCompute, nullptr);

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &m_cmdBufferCompute;

  VkFenceCreateInfo fenceCreateInfo = {};
  fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceCreateInfo.flags = 0;
  VK_CHECK_RESULT(vkCreateFence(m_device, &fenceCreateInfo, NULL, &m_fence));

  // Отправляем буфер команд на выполнение
  VK_CHECK_RESULT(vkQueueSubmit(m_computeQueue, 1, &submitInfo, m_fence));

  //Ждём конца выполнения команд
  VK_CHECK_RESULT(vkWaitForFences(m_device, 1, &m_fence, VK_TRUE, 100000000000));

  std::vector<float> values(m_length);
  m_pCopyHelper->ReadBuffer(m_sum_final, 0, values.data(), sizeof(float) * values.size());
  for (auto v: values) {
    std::cout << v << ' ';
  }
}
