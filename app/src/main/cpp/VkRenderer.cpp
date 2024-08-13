
#include <vector>
#include <array>
#include <cassert>
#include <set>
#include <math.h>
#include "header/VkRenderer.h"
#include "header/SwapChain.h"
#include "header/Mesh.h"
#include "header/VkTexture.h"
#include <stdlib.h>

using namespace std;

VkRenderer::~VkRenderer() {
    aout << " ~VK Destroy Instance" << endl;
    CleanUp();
}

VkRenderer::VkRenderer(ANativeWindow *window, AAssetManager *assetManager)
: mAssetManager {assetManager} {
    mWindow = window;
    try {
        CreateInstance();
        CreateSurface();
        GetPhysicalDevice();
        CreateLogicalDevice();

        CreateSwapChain();
        CreateRenderPass();
        CreateDescriptorSetLayout();
        CreatePushConstantRange();
        CreateGraphicsPipeline();
        CreateDepthBufferImage();

        CreateFramebuffers();
        CreateCommandPool();
        CreateCommandBuffers();
        CreateTextureSampler();
        //AllocateDynamicBufferTransferSpace();
        CreateUniformBuffers();
        CreateDescriptorPool();
        CreateDescriptorSets();
        CreateSynchronisation();

        mUboViewProjection.projection = glm::perspective(glm::radians(45.0f), (float)mSwapChainExtent.width / (float)mSwapChainExtent.height, 0.1f, 100.0f);
        mUboViewProjection.view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        mUboViewProjection.projection[1][1] *= -1;

        // Create a mesh
        // Vertex Data
        std::vector<Vertex> meshVertices = {
                { { -0.4, 0.4, 0.0 },{ 1.0f, 0.0f, 0.0f },{ 1.0f, 1.0f } },	// 0
                { { -0.4, -0.4, 0.0 },{ 1.0f, 0.0f, 0.0f },{ 1.0f, 0.0f } },	    // 1
                { { 0.4, -0.4, 0.0 },{ 1.0f, 0.0f, 0.0f },{ 0.0f, 0.0f } },    // 2
                { { 0.4, 0.4, 0.0 },{ 1.0f, 0.0f, 0.0f },{ 0.0f, 1.0f } },   // 3
        };
        std::vector<Vertex> meshVertices2 = {
                { { -0.25, 0.6, 0.0 },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 1.0f } },	// 0
                { { -0.25, -0.6, 0.0 },{ 0.0f, 0.0f, 1.0f },{ 1.0f, 0.0f } },	    // 1
                { { 0.25, -0.6, 0.0 },{ 0.0f, 0.0f, 1.0f },{ 0.0f, 0.0f } },    // 2
                { { 0.25, 0.6, 0.0 },{ 0.0f, 0.0f, 1.0f },{ 0.0f, 1.0f } },   // 3
        };

        // Index Data
        std::vector<uint32_t> meshIndices = {
                0, 1, 2,
                2, 3, 0
        };
        Mesh firstMesh = Mesh(mPhysicalDevice, mDevice,
                              mGraphicQueue, mGraphicsCommandPool,
                              &meshVertices, &meshIndices,
                              CreateTexture("giraffe.jpg"));
        Mesh secondMesh = Mesh(mPhysicalDevice, mDevice,
                               mGraphicQueue, mGraphicsCommandPool,
                               &meshVertices2, &meshIndices,
                               CreateTexture("panda.jpg"));

        mMeshList.push_back(firstMesh);
        mMeshList.push_back(secondMesh);


        aout << "End initialize()" << endl;
    }
    catch (const std::runtime_error &e) {
        aout << "runtime Error" << endl;
    }
}
void VkRenderer::CleanUp() {
    // Wait until no actions being run on device before destroying
    vkDeviceWaitIdle(mDevice);
    vkDestroyDescriptorPool(mDevice, mSamplerDescriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(mDevice, mSamplerSetLayout, nullptr);

    vkDestroySampler(mDevice, mTextureSampler, nullptr);

    for (size_t i = 0; i < mTextureImages.size(); i++)
    {
        vkDestroyImageView(mDevice, mTextureImageViews[i], nullptr);
        vkDestroyImage(mDevice, mTextureImages[i], nullptr);
        vkFreeMemory(mDevice, mTextureImageMemory[i], nullptr);
    }

    // free(mModelTransferSpace); // dlgmlals3 error
    vkDestroyImageView(mDevice, mDepthBufferImageView, nullptr);
    vkDestroyImage(mDevice, mDepthBufferImage, nullptr);
    vkFreeMemory(mDevice, mDepthBufferImageMemory, nullptr);

    vkDestroyDescriptorPool(mDevice, mDescriptorPool, nullptr);
    vkDestroyDescriptorSetLayout(mDevice, mDescriptorSetLayout, nullptr);
    for (size_t i = 0; i < mSwapChainImages.size(); i++)
    {
        vkDestroyBuffer(mDevice, vpUniformBuffer[i], nullptr);
        vkFreeMemory(mDevice, vpUniformBufferMemory[i], nullptr);
        //vkDestroyBuffer(mDevice, modelDUniformBuffer[i], nullptr);
        //vkFreeMemory(mDevice, modelDUniformBufferMemory[i], nullptr);
    }


    for (size_t i = 0; i < mMeshList.size(); i++)
    {
        mMeshList[i].destroyBuffers();
    }


    for (size_t i = 0; i < MAX_FRAME_DRAWS; i++)
    {
        vkDestroySemaphore(mDevice, mRenderFinished[i], nullptr);
        vkDestroySemaphore(mDevice, mImageAvailable[i], nullptr);
        vkDestroyFence(mDevice, mDrawFences[i], nullptr);
    }

    vkDestroyCommandPool(mDevice, mGraphicsCommandPool, nullptr);

    for (auto framebuffer : mSwapChainFramebuffers) {
        vkDestroyFramebuffer(mDevice, framebuffer, nullptr);
    }

    vkDestroyPipeline(mDevice, mGraphicsPipeline, nullptr);
    vkDestroyPipelineLayout(mDevice, mPipelineLayout, nullptr);

    for (auto image : mSwapChainImages)
    {
        vkDestroyImageView(mDevice, image.imageView, nullptr);
    }
    vkDestroySwapchainKHR(mDevice, mSwapChain, nullptr);
    vkDestroySurfaceKHR(mInstance, mSurface, nullptr);
    vkDestroyDevice(mDevice, nullptr);
    vkDestroyInstance(mInstance, nullptr);
}
void VkRenderer::UpdateModel(int modelId, glm::mat4 newModel)
{
    if (modelId >= mMeshList.size()) return;

    mMeshList[modelId].setModel(newModel);
}


void VkRenderer::Draw() {
    // -- GET NEXT IMAGE --
    // Wait for given fence to signal (open) from last draw before continuing
    vkWaitForFences(mDevice, 1, &mDrawFences[mCurrentFrame], VK_TRUE, std::numeric_limits<uint64_t>::max());
    // Manually reset (close) fences
    vkResetFences(mDevice, 1, &mDrawFences[mCurrentFrame]);

    // Get index of next image to be drawn to, and signal semaphore when ready to be drawn to
    uint32_t imageIndex;
    vkAcquireNextImageKHR(mDevice, mSwapChain, std::numeric_limits<uint64_t>::max(), mImageAvailable[mCurrentFrame], VK_NULL_HANDLE, &imageIndex);

    RecordCommands(imageIndex);
    UpdateUniformBuffers(imageIndex);

    // -- SUBMIT COMMAND BUFFER TO RENDER --
    // Queue submission information
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    
    submitInfo.waitSemaphoreCount = 1;										// Number of semaphores to wait on
    submitInfo.pWaitSemaphores = &mImageAvailable[mCurrentFrame];				// List of semaphores to wait on
    VkPipelineStageFlags waitStages[] = {
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };
    submitInfo.pWaitDstStageMask = waitStages;						// Stages to check semaphores at
    submitInfo.commandBufferCount = 1;								// Number of command buffers to submit
    submitInfo.pCommandBuffers = &mCommandBuffers[imageIndex];		// Command buffer to submit
    submitInfo.signalSemaphoreCount = 1;							// Number of semaphores to signal
    submitInfo.pSignalSemaphores = &mRenderFinished[mCurrentFrame];	// Semaphores to signal when command buffer finishes

    // Submit command buffer to queue
    VkResult result = vkQueueSubmit(mGraphicQueue, 1, &submitInfo, mDrawFences[mCurrentFrame]);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to submit Command Buffer to Queue!");
    }

    // -- PRESENT RENDERED IMAGE TO SCREEN --
    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;										// Number of semaphores to wait on
    presentInfo.pWaitSemaphores = &mRenderFinished[mCurrentFrame];			// Semaphores to wait on
    presentInfo.swapchainCount = 1;											// Number of swapchains to present to
    presentInfo.pSwapchains = &mSwapChain;									// Swapchains to present images to
    presentInfo.pImageIndices = &imageIndex;								// Index of images in swapchains to present

    // Present image
    result = vkQueuePresentKHR(mPresentationQueue, &presentInfo);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to present Image!");
    }
    // Get next frame (use % MAX_FRAME_DRAWS to keep value below MAX_FRAME_DRAWS)
    mCurrentFrame = (mCurrentFrame + 1) % MAX_FRAME_DRAWS;

    return;
}

void VkRenderer::CreateDescriptorSetLayout()
{
    aout << " CreateDescriptorSetLayout" << endl;
    // UboViewProjection Binding Info
    VkDescriptorSetLayoutBinding vpLayoutBinding = {};
    vpLayoutBinding.binding = 0;											// Binding point in shader (designated by binding number in shader)
    vpLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;	// Type of descriptor (uniform, dynamic uniform, image sampler, etc)
    vpLayoutBinding.descriptorCount = 1;									// Number of descriptors for binding
    vpLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;				// Shader stage to bind to
    vpLayoutBinding.pImmutableSamplers = nullptr;							// For Texture: Can make sampler data unchangeable (immutable) by specifying in layout

    // Model Binding Info
    /*
    VkDescriptorSetLayoutBinding modelLayoutBinding = {};
    modelLayoutBinding.binding = 1;
    modelLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    modelLayoutBinding.descriptorCount = 1;
    modelLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    modelLayoutBinding.pImmutableSamplers = nullptr;
    */

    std::vector<VkDescriptorSetLayoutBinding> layoutBindings = { vpLayoutBinding };

    // Create Descriptor Set Layout with given bindings
    VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
    layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutCreateInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());	// Number of binding infos
    layoutCreateInfo.pBindings = layoutBindings.data();								// Array of binding infos

    // Create Descriptor Set Layout
    VkResult result = vkCreateDescriptorSetLayout(mDevice, &layoutCreateInfo, nullptr, &mDescriptorSetLayout);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a Descriptor Set Layout!");
    }

    // CREATE TEXTURE SAMPLER DESCRIPTOR SET LAYOUT
    // Texture binding info
    VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
    samplerLayoutBinding.binding = 0;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerLayoutBinding.pImmutableSamplers = nullptr;

    // Create a Descriptor Set Layout with given bindings for texture
    VkDescriptorSetLayoutCreateInfo textureLayoutCreateInfo = {};
    textureLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    textureLayoutCreateInfo.bindingCount = 1;
    textureLayoutCreateInfo.pBindings = &samplerLayoutBinding;

    // Create Descriptor Set Layout
    result = vkCreateDescriptorSetLayout(mDevice, &textureLayoutCreateInfo, nullptr, &mSamplerSetLayout);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a Descriptor Set Layout!");
    }
}

void VkRenderer::CreatePushConstantRange()
{
    // Define push constant values (no 'create' needed!)
    mPushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;	// Shader stage push constant will go to
    mPushConstantRange.offset = 0;								// Offset into given data to pass to push constant
    mPushConstantRange.size = sizeof(Model);						// Size of data being passed
}


void VkRenderer::CreateSynchronisation()
{
    aout << "CreateSynchronisation" << endl;
    mImageAvailable.resize(MAX_FRAME_DRAWS);
    mRenderFinished.resize(MAX_FRAME_DRAWS);
    mDrawFences.resize(MAX_FRAME_DRAWS);

    // Semaphore creation information
    VkSemaphoreCreateInfo semaphoreCreateInfo = {};
    semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    // Fence creation information
    VkFenceCreateInfo fenceCreateInfo = {};
    fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for (size_t i = 0; i < MAX_FRAME_DRAWS; i++)
    {
        if (vkCreateSemaphore(mDevice, &semaphoreCreateInfo, nullptr, &mImageAvailable[i]) != VK_SUCCESS ||
            vkCreateSemaphore(mDevice, &semaphoreCreateInfo, nullptr, &mRenderFinished[i]) != VK_SUCCESS ||
            vkCreateFence(mDevice, &fenceCreateInfo, nullptr, &mDrawFences[i]) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create a Semaphore and/or Fence!");
        }
    }
}


void VkRenderer::CreateFramebuffers()
{
    aout << "CreateFrameBuffers" << endl;
    // Resize framebuffer count to equal swap chain image count
    mSwapChainFramebuffers.resize(mSwapChainImages.size());

    // Create a framebuffer for each swap chain image
    for (size_t i = 0; i < mSwapChainImages.size(); i++)
    {
        std::array<VkImageView, 2> attachments = {
                mSwapChainImages[i].imageView,
                mDepthBufferImageView
        };

        VkFramebufferCreateInfo framebufferCreateInfo = {};
        framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferCreateInfo.renderPass = mRenderPass;										// Render Pass layout the Framebuffer will be used with
        framebufferCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferCreateInfo.pAttachments = attachments.data();							// List of attachments (1:1 with Render Pass)
        framebufferCreateInfo.width = mSwapChainExtent.width;								// Framebuffer width
        framebufferCreateInfo.height = mSwapChainExtent.height;								// Framebuffer height
        framebufferCreateInfo.layers = 1;													// Framebuffer layers

        VkResult result = vkCreateFramebuffer(mDevice, &framebufferCreateInfo, nullptr, &mSwapChainFramebuffers[i]);
        if (result != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create a Framebuffer!");
        }
    }
}

void VkRenderer::CreateCommandPool()
{
    aout << "CreateCommandPool" << endl;
    // Get indices of queue families from device
    QueueFamilyIndices queueFamilyIndices = GetQueueFamilies(mPhysicalDevice);

    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;	// Queue Family type that buffers from this command pool will use

    // Create a Graphics Queue Family Command Pool
    VkResult result = vkCreateCommandPool(mDevice, &poolInfo, nullptr, &mGraphicsCommandPool);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a Command Pool!");
    }
}

void VkRenderer::CreateCommandBuffers()
{
    aout << "CreateCommandBuffers" << endl;
    // Resize command buffer count to have one for each framebuffer
    mCommandBuffers.resize(mSwapChainFramebuffers.size());

    VkCommandBufferAllocateInfo cbAllocInfo = {};
    cbAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbAllocInfo.commandPool = mGraphicsCommandPool;
    cbAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    // VK_COMMAND_BUFFER_LEVEL_PRIMARY	: Buffer you submit directly to queue. Cant be called by other buffers.
    // VK_COMMAND_BUFFER_LEVEL_SECONARY	: Buffer can't be called directly.
    // Can be called from other buffers via "vkCmdExecuteCommands" when recording commands in primary buffer

    cbAllocInfo.commandBufferCount = static_cast<uint32_t>(mCommandBuffers.size());

    // Allocate command buffers and place handles in array of buffers
    VkResult result = vkAllocateCommandBuffers(mDevice, &cbAllocInfo, mCommandBuffers.data());
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate Command Buffers!");
    }
}

void VkRenderer::CreateTextureSampler()
{
    // Sampler Creation Info
    VkSamplerCreateInfo samplerCreateInfo = {};
    samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerCreateInfo.magFilter = VK_FILTER_LINEAR;						// How to render when image is magnified on screen
    samplerCreateInfo.minFilter = VK_FILTER_LINEAR;						// How to render when image is minified on screen
    samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;	// How to handle texture wrap in U (x) direction
    samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;	// How to handle texture wrap in V (y) direction
    samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;	// How to handle texture wrap in W (z) direction
    samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;	// Border beyond texture (only workds for border clamp)
    samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;				// Whether coords should be normalized (between 0 and 1)
    samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;		// Mipmap interpolation mode
    samplerCreateInfo.mipLodBias = 0.0f;								// Level of Details bias for mip level
    samplerCreateInfo.minLod = 0.0f;									// Minimum Level of Detail to pick mip level
    samplerCreateInfo.maxLod = 0.0f;									// Maximum Level of Detail to pick mip level
    samplerCreateInfo.anisotropyEnable = VK_TRUE;						// Enable Anisotropy
    samplerCreateInfo.maxAnisotropy = 16;								// Anisotropy sample level

    VkResult result = vkCreateSampler(mDevice, &samplerCreateInfo, nullptr, &mTextureSampler);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Filed to create a Texture Sampler!");
    }
}

void VkRenderer::CreateUniformBuffers()
{
    // ViewProjection buffer size
    VkDeviceSize vpBufferSize = sizeof(UboViewProjection);

    // Model buffer size
    // VkDeviceSize modelBufferSize = mModelUniformAlignment * MAX_OBJECTS;

    // One uniform buffer for each image (and by extension, command buffer)
    vpUniformBuffer.resize(mSwapChainImages.size());
    vpUniformBufferMemory.resize(mSwapChainImages.size());
    // modelDUniformBuffer.resize(mSwapChainImages.size());
    // modelDUniformBufferMemory.resize(mSwapChainImages.size());

    // Create Uniform buffers
    for (size_t i = 0; i < mSwapChainImages.size(); i++)
    {
        createBuffer(mPhysicalDevice, mDevice, vpBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                     VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &vpUniformBuffer[i], &vpUniformBufferMemory[i]);

        // createBuffer(mPhysicalDevice, mDevice, modelBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
           //          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &modelDUniformBuffer[i], &modelDUniformBufferMemory[i]);
    }
}

void VkRenderer::CreateDescriptorPool()
{
    aout << "CreateDescriptorPool" << endl;
    // Type of descriptors + how many DESCRIPTORS, not Descriptor Sets (combined makes the pool size)
    // ViewProjection Pool
    VkDescriptorPoolSize vpPoolSize = {};
    vpPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    vpPoolSize.descriptorCount = static_cast<uint32_t>(vpUniformBuffer.size());

    // Model Pool (DYNAMIC)
    /*
    VkDescriptorPoolSize modelPoolSize = {};
    modelPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    modelPoolSize.descriptorCount = static_cast<uint32_t>(modelDUniformBuffer.size());
    */

    // List of pool sizes
    std::vector<VkDescriptorPoolSize> descriptorPoolSizes = { vpPoolSize };

    // Data to create Descriptor Pool
    VkDescriptorPoolCreateInfo poolCreateInfo = {};
    poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolCreateInfo.maxSets = static_cast<uint32_t>(mSwapChainImages.size());					// Maximum number of Descriptor Sets that can be created from pool
    poolCreateInfo.poolSizeCount = static_cast<uint32_t>(descriptorPoolSizes.size());		// Amount of Pool Sizes being passed
    poolCreateInfo.pPoolSizes = descriptorPoolSizes.data();									// Pool Sizes to create pool with

    // Create Descriptor Pool
    VkResult result = vkCreateDescriptorPool(mDevice, &poolCreateInfo, nullptr, &mDescriptorPool);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a Descriptor Pool!");
    }
    // CREATE SAMPLER DESCRIPTOR POOL
    // Texture sampler pool
    VkDescriptorPoolSize samplerPoolSize = {};
    samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerPoolSize.descriptorCount = MAX_OBJECTS;

    VkDescriptorPoolCreateInfo samplerPoolCreateInfo = {};
    samplerPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    samplerPoolCreateInfo.maxSets = MAX_OBJECTS;
    samplerPoolCreateInfo.poolSizeCount = 1;
    samplerPoolCreateInfo.pPoolSizes = &samplerPoolSize;

    result = vkCreateDescriptorPool(mDevice, &samplerPoolCreateInfo, nullptr, &mSamplerDescriptorPool);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a Descriptor Pool!");
    }
}

void VkRenderer::CreateDescriptorSets()
{
    // Resize Descriptor Set list so one for every buffer
    mDescriptorSets.resize(mSwapChainImages.size());

    std::vector<VkDescriptorSetLayout> setLayouts(mSwapChainImages.size(), mDescriptorSetLayout);

    // Descriptor Set Allocation Info
    VkDescriptorSetAllocateInfo setAllocInfo = {};
    setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setAllocInfo.descriptorPool = mDescriptorPool;									// Pool to allocate Descriptor Set from
    setAllocInfo.descriptorSetCount = static_cast<uint32_t>(mSwapChainImages.size());// Number of sets to allocate
    setAllocInfo.pSetLayouts = setLayouts.data();									// Layouts to use to allocate sets (1:1 relationship)

    // Allocate descriptor sets (multiple)
    VkResult result = vkAllocateDescriptorSets(mDevice, &setAllocInfo, mDescriptorSets.data());
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate Descriptor Sets!");
    }

    // Update all of descriptor set buffer bindings
    for (size_t i = 0; i < mSwapChainImages.size(); i++)
    {
        // VIEW PROJECTION DESCRIPTOR
        // Buffer info and data offset info
        VkDescriptorBufferInfo vpBufferInfo = {};
        vpBufferInfo.buffer = vpUniformBuffer[i];		// Buffer to get data from
        vpBufferInfo.offset = 0;						// Position of start of data
        vpBufferInfo.range = sizeof(UboViewProjection);				// Size of data

        // Data about connection between binding and buffer
        VkWriteDescriptorSet vpSetWrite = {};
        vpSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        vpSetWrite.dstSet = mDescriptorSets[i];								// Descriptor Set to update
        vpSetWrite.dstBinding = 0;											// Binding to update (matches with binding on layout/shader)
        vpSetWrite.dstArrayElement = 0;									// Index in array to update
        vpSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;		// Type of descriptor
        vpSetWrite.descriptorCount = 1;									// Amount to update
        vpSetWrite.pBufferInfo = &vpBufferInfo;							// Information about buffer data to bind

        // MODEL DESCRIPTOR
        // Model Buffer Binding Info
        /*
        VkDescriptorBufferInfo modelBufferInfo = {};
        modelBufferInfo.buffer = modelDUniformBuffer[i];
        modelBufferInfo.offset = 0;
        modelBufferInfo.range = mModelUniformAlignment;

        VkWriteDescriptorSet modelSetWrite = {};
        modelSetWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        modelSetWrite.dstSet = mDescriptorSets[i];
        modelSetWrite.dstBinding = 1;
        modelSetWrite.dstArrayElement = 0;
        modelSetWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
        modelSetWrite.descriptorCount = 1;
        modelSetWrite.pBufferInfo = &modelBufferInfo;
        */

        // List of Descriptor Set Writes
        std::vector<VkWriteDescriptorSet> setWrites = { vpSetWrite };

        // Update the descriptor sets with new buffer/binding info
        vkUpdateDescriptorSets(mDevice, static_cast<uint32_t>(setWrites.size()), setWrites.data(),
                               0, nullptr);
    }
}

void VkRenderer::UpdateUniformBuffers(uint32_t imageIndex)
{
    // Copy VP data
    void * data;
    vkMapMemory(mDevice, vpUniformBufferMemory[imageIndex], 0, sizeof(UboViewProjection), 0, &data);
    memcpy(data, &mUboViewProjection, sizeof(UboViewProjection));
    vkUnmapMemory(mDevice, vpUniformBufferMemory[imageIndex]);

    // Copy Model data
    /*
    for (size_t i = 0; i < meshList.size(); i++)
    {
        UboModel * thisModel = (UboModel *)((uint64_t)mModelTransferSpace + (i * mModelUniformAlignment));
        *thisModel = meshList[i].getModel();
    }


    // Map the list of model data
    vkMapMemory(mDevice,modelDUniformBufferMemory[imageIndex], 0, mModelUniformAlignment * meshList.size(), 0, &data);
    memcpy(data, mModelTransferSpace, mModelUniformAlignment * meshList.size());
    vkUnmapMemory(mDevice, modelDUniformBufferMemory[imageIndex]);
    */
}

void VkRenderer::RecordCommands(uint32_t currentImage)
{
    // aout << "RecordCommands" << endl;
    // Information about how to begin each command buffer
    VkCommandBufferBeginInfo bufferBeginInfo = {};
    bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    // Buffer can be resubmitted when it has already been submitted and is awaiting execution
    // bufferBeginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    // Information about how to begin a render pass (only needed for graphical applications)
    VkRenderPassBeginInfo renderPassBeginInfo = {};
    renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassBeginInfo.renderPass = mRenderPass;							// Render Pass to begin
    renderPassBeginInfo.renderArea.offset = { 0, 0 };				    // Start point of render pass in pixels
    renderPassBeginInfo.renderArea.extent = mSwapChainExtent;				// Size of region to run render pass on (starting at offset)

    std::array<VkClearValue, 2> clearValues = {};
    clearValues[0].color = { 0.6f, 0.65f, 0.4f, 1.0f };
    clearValues[1].depthStencil.depth = 1.0f;
    renderPassBeginInfo.pClearValues = clearValues.data();					// List of clear values
    renderPassBeginInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassBeginInfo.framebuffer = mSwapChainFramebuffers[currentImage];

    // Start recording commands to command buffer!
    VkResult result = vkBeginCommandBuffer(mCommandBuffers[currentImage], &bufferBeginInfo);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to start recording a Command Buffer!");
    }

    // Begin Render Pass
    vkCmdBeginRenderPass(mCommandBuffers[currentImage], &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

    // Bind Pipeline to be used in render pass
    vkCmdBindPipeline(mCommandBuffers[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS, mGraphicsPipeline);

    for (size_t j = 0; j < mMeshList.size(); j++)
    {
        VkBuffer vertexBuffers[] = { mMeshList[j].getVertexBuffer() };					// Buffers to bind
        VkDeviceSize offsets[] = { 0 };												// Offsets into buffers being bound
        vkCmdBindVertexBuffers(mCommandBuffers[currentImage], 0, 1, vertexBuffers, offsets);	// Command to bind vertex buffer before drawing with them

        // Bind mesh index buffer, with 0 offset and using the uint32 type
        vkCmdBindIndexBuffer(mCommandBuffers[currentImage], mMeshList[j].getIndexBuffer(), 0, VK_INDEX_TYPE_UINT32);

        // Dynamic Offset Amount
        // uint32_t dynamicOffset = static_cast<uint32_t>(modelUniformAlignment) * j;

        // "Push" constants to given shader stage directly (no buffer)
        Model model = mMeshList[j].getModel();
        vkCmdPushConstants(
                mCommandBuffers[currentImage],
                mPipelineLayout,
                VK_SHADER_STAGE_VERTEX_BIT,		// Stage to push constants to
                0,								// Offset of push constants to update
                sizeof(Model),					// Size of data being pushed
                &model);		// Actual data being pushed (can be array)

        std::array<VkDescriptorSet, 2> descriptorSetGroup = {
                mDescriptorSets[currentImage],
                mSamplerDescriptorSets[mMeshList[j].getTexId()] };

        // Bind Descriptor Sets
        vkCmdBindDescriptorSets(mCommandBuffers[currentImage], VK_PIPELINE_BIND_POINT_GRAPHICS, mPipelineLayout,
                                0, static_cast<uint32_t>(descriptorSetGroup.size()), descriptorSetGroup.data(), 0, nullptr);

        // Execute pipeline
        vkCmdDrawIndexed(mCommandBuffers[currentImage], mMeshList[j].getIndexCount(), 1, 0, 0, 0);
    }
    // End Render Pass
    vkCmdEndRenderPass(mCommandBuffers[currentImage]);

    // Stop recording to command buffer
    result = vkEndCommandBuffer(mCommandBuffers[currentImage]);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to stop recording a Command Buffer!");
    }

    // Stop recording to command buffer
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to stop recording a Command Buffer!");
    }
    return;
}

void VkRenderer::CreateRenderPass() {
    aout << "Create RenderPass" << endl;
    // Colour attachment of render pass
    VkAttachmentDescription colourAttachment = {};
    colourAttachment.format = mSwapChainImageFormat;						// Format to use for attachment
    colourAttachment.samples = VK_SAMPLE_COUNT_1_BIT;					// Number of samples to write for multisampling
    colourAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;				// Describes what to do with attachment before rendering
    colourAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;			// Describes what to do with attachment after rendering
    colourAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;	// Describes what to do with stencil before rendering
    colourAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;	// Describes what to do with stencil after rendering

    // Framebuffer data will be stored as an image, but images can be given different data layouts
    // to give optimal use for certain operations
    colourAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;			// Image data layout before render pass starts
    colourAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;		// Image data layout after render pass (to change to)

    // Depth attachment of render pass
    VkAttachmentDescription depthAttachment = {};
    depthAttachment.format = ChooseSupportedFormat(
            { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT },
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


    // Attachment reference uses an attachment index that refers to index in the attachment list passed to renderPassCreateInfo
    VkAttachmentReference colourAttachmentReference = {};
    colourAttachmentReference.attachment = 0;
    colourAttachmentReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Depth Attachment Reference
    VkAttachmentReference depthAttachmentReference = {};
    depthAttachmentReference.attachment = 1;
    depthAttachmentReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    // Information about a particular subpass the Render Pass is using
    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;		// Pipeline type subpass is to be bound to
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colourAttachmentReference;
    subpass.pDepthStencilAttachment = &depthAttachmentReference;

    // Need to determine when layout transitions occur using subpass dependencies
    std::array<VkSubpassDependency, 2> subpassDependencies;

    // Conversion from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
    // Transition must happen after...
    subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;						// Subpass index (VK_SUBPASS_EXTERNAL = Special value meaning outside of renderpass)
    subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;		// Pipeline stage
    subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;				// Stage access mask (memory access)
    // But must happen before...
    subpassDependencies[0].dstSubpass = 0;
    subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    subpassDependencies[0].dependencyFlags = 0;


    // Conversion from VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
    // Transition must happen after...
    subpassDependencies[1].srcSubpass = 0;
    subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;;
    // But must happen before...
    subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
    subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    subpassDependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    subpassDependencies[1].dependencyFlags = 0;

    std::array<VkAttachmentDescription, 2> renderPassAttachments = { colourAttachment, depthAttachment };

    // Create info for Render Pass
    VkRenderPassCreateInfo renderPassCreateInfo = {};
    renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassCreateInfo.attachmentCount = static_cast<uint32_t>(renderPassAttachments.size());
    renderPassCreateInfo.pAttachments = renderPassAttachments.data();
    renderPassCreateInfo.subpassCount = 1;
    renderPassCreateInfo.pSubpasses = &subpass;
    renderPassCreateInfo.dependencyCount = static_cast<uint32_t>(subpassDependencies.size());
    renderPassCreateInfo.pDependencies = subpassDependencies.data();

    VkResult result = vkCreateRenderPass(mDevice, &renderPassCreateInfo, nullptr, &mRenderPass);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a Render Pass!");
    }
}

void VkRenderer::CreateGraphicsPipeline()
{
    aout << "CreateGraphicsPipeline" << endl;
    // Read in SPIR-V code of shaders
    auto vertexShaderCode = ReadFile("Shaders/shader.vert");
    auto fragmentShaderCode =  ReadFile("Shaders/shader.frag");
    //for (auto ch : vertexShaderCode) aout << ch;
    //for (auto ch : fragmentShaderCode) aout << ch;
    if (vertexShaderCode.size() <= 0 || fragmentShaderCode.size() <= 0) {
        aout << "Error shader code" << endl;
        return;
    }
    // Create Shader Modules
    string_view vertexCode(reinterpret_cast<char*>(vertexShaderCode.data()), vertexShaderCode.size());
    string_view fragCode(reinterpret_cast<char*>(fragmentShaderCode.data()), fragmentShaderCode.size());
    VkShaderModule vertexShaderModule = CreateShaderModule(vertexCode, VK_SHADER_TYPE_VERTEX);
    VkShaderModule fragmentShaderModule = CreateShaderModule(fragCode, VK_SHADER_TYPE_FRAGMENT);

    // Vertex Stage creation information
    VkPipelineShaderStageCreateInfo vertexShaderCreateInfo = {};
    vertexShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertexShaderCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;				// Shader Stage name
    vertexShaderCreateInfo.module = vertexShaderModule;						// Shader module to be used by stage
    vertexShaderCreateInfo.pName = "main";									// Entry point in to shader

    // Fragment Stage creation information
    VkPipelineShaderStageCreateInfo fragmentShaderCreateInfo = {};
    fragmentShaderCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragmentShaderCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;				// Shader Stage name
    fragmentShaderCreateInfo.module = fragmentShaderModule;						// Shader module to be used by stage
    fragmentShaderCreateInfo.pName = "main";									// Entry point in to shader

    // Put shader stage creation info in to array
    // Graphics Pipeline creation info requires array of shader stage creates
    VkPipelineShaderStageCreateInfo shaderStages[] = { vertexShaderCreateInfo, fragmentShaderCreateInfo };

    // How the data for a single vertex (including info such as position, colour, texture coords, normals, etc) is as a whole
    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = 0;									// Can bind multiple streams of data, this defines which one
    bindingDescription.stride = sizeof(Vertex);						// Size of a single vertex object
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;		// How to move between data after each vertex.
    // VK_VERTEX_INPUT_RATE_INDEX		: Move on to the next vertex
    // VK_VERTEX_INPUT_RATE_INSTANCE	: Move to a vertex for the next instance

    // How the data for an attribute is defined within a vertex
    std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions;

    // Position Attribute
    attributeDescriptions[0].binding = 0;							// Which binding the data is at (should be same as above)
    attributeDescriptions[0].location = 0;							// Location in shader where data will be read from
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;	// Format the data will take (also helps define size of data)
    attributeDescriptions[0].offset = offsetof(Vertex, pos);		// Where this attribute is defined in the data for a single vertex

    // Colour Attribute
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, col);

    // Texture Attribute
    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(Vertex, tex);

    VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
    vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
    vertexInputCreateInfo.pVertexBindingDescriptions = &bindingDescription;											// List of Vertex Binding Descriptions (data spacing/stride information)
    vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputCreateInfo.pVertexAttributeDescriptions = attributeDescriptions.data();								// List of Vertex Attribute Descriptions (data format and where to bind to/from)

    // -- INPUT ASSEMBLY --
    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;// Primitive type to assemble vertices as
    inputAssembly.primitiveRestartEnable = VK_FALSE;	// Allow overriding of "strip" topology to start new primitives

    // -- VIEWPORT & SCISSOR --
    // Create a viewport info struct
    VkViewport viewport = {};
    viewport.x = 0.0f;									// x start coordinate
    viewport.y = 0.0f;									// y start coordinate
    viewport.width = (float)mSwapChainExtent.width;		// width of viewport
    viewport.height = (float)mSwapChainExtent.height;	// height of viewport
    viewport.minDepth = 0.0f;							// min framebuffer depth
    viewport.maxDepth = 1.0f;							// max framebuffer depth

    // Create a scissor info struct
    VkRect2D scissor = {};
    scissor.offset = { 0,0 };							// Offset to use region from
    scissor.extent = mSwapChainExtent;					// Extent to describe region to use, starting at offset

    VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
    viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportStateCreateInfo.viewportCount = 1;
    viewportStateCreateInfo.pViewports = &viewport;
    viewportStateCreateInfo.scissorCount = 1;
    viewportStateCreateInfo.pScissors = &scissor;


    // -- DYNAMIC STATES --
    // Dynamic states to enable
    //std::vector<VkDynamicState> dynamicStateEnables;
    //dynamicStateEnables.push_back(VK_DYNAMIC_STATE_VIEWPORT);	// Dynamic Viewport : Can resize in command buffer with vkCmdSetViewport(commandbuffer, 0, 1, &viewport);
    //dynamicStateEnables.push_back(VK_DYNAMIC_STATE_SCISSOR);	// Dynamic Scissor	: Can resize in command buffer with vkCmdSetScissor(commandbuffer, 0, 1, &scissor);

    //// Dynamic State creation info
    //VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
    //dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    //dynamicStateCreateInfo.dynamicStateCount = static_cast<uint32_t>(dynamicStateEnables.size());
    //dynamicStateCreateInfo.pDynamicStates = dynamicStateEnables.data();


    // -- RASTERIZER --
    VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {};
    rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizerCreateInfo.depthClampEnable = VK_FALSE;			// Change if fragments beyond near/far planes are clipped (default) or clamped to plane
    rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;	// Whether to discard data and skip rasterizer. Never creates fragments, only suitable for pipeline without framebuffer output
    rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;	// How to handle filling points between vertices
    rasterizerCreateInfo.lineWidth = 1.0f;						// How thick lines should be when drawn
    rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;		// Which face of a tri to cull
    rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;	// Winding to determine which side is front
    rasterizerCreateInfo.depthBiasEnable = VK_FALSE;			// Whether to add depth bias to fragments (good for stopping "shadow acne" in shadow mapping)


    // -- MULTISAMPLING --
    VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo = {};
    multisamplingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisamplingCreateInfo.sampleShadingEnable = VK_FALSE;					// Enable multisample shading or not
    multisamplingCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;	// Number of samples to use per fragment


    // -- BLENDING --
    // Blending decides how to blend a new colour being written to a fragment, with the old value

    // Blend Attachment State (how blending is handled)
    VkPipelineColorBlendAttachmentState colourState = {};
    colourState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT	// Colours to apply blending to
                                 | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colourState.blendEnable = VK_TRUE;													// Enable blending

    // Blending uses equation: (srcColorBlendFactor * new colour) colorBlendOp (dstColorBlendFactor * old colour)
    colourState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colourState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colourState.colorBlendOp = VK_BLEND_OP_ADD;

    // Summarised: (VK_BLEND_FACTOR_SRC_ALPHA * new colour) + (VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA * old colour)
    //			   (new colour alpha * new colour) + ((1 - new colour alpha) * old colour)

    colourState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colourState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colourState.alphaBlendOp = VK_BLEND_OP_ADD;
    // Summarised: (1 * new alpha) + (0 * old alpha) = new alpha

    VkPipelineColorBlendStateCreateInfo colourBlendingCreateInfo = {};
    colourBlendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colourBlendingCreateInfo.logicOpEnable = VK_FALSE;				// Alternative to calculations is to use logical operations
    colourBlendingCreateInfo.attachmentCount = 1;
    colourBlendingCreateInfo.pAttachments = &colourState;


    // -- PIPELINE LAYOUT (TODO: Apply Future Descriptor Set Layouts) --
    std::array<VkDescriptorSetLayout, 2> descriptorSetLayouts = { mDescriptorSetLayout, mSamplerSetLayout };

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
    pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCreateInfo.setLayoutCount = static_cast<uint32_t>(descriptorSetLayouts.size());
    pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts.data();
    pipelineLayoutCreateInfo.pushConstantRangeCount = 1;
    pipelineLayoutCreateInfo.pPushConstantRanges = &mPushConstantRange;


    // Create Pipeline Layout
    VkResult result = vkCreatePipelineLayout(mDevice, &pipelineLayoutCreateInfo, nullptr, &mPipelineLayout);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create Pipeline Layout!");
    }

    // -- DEPTH STENCIL TESTING --
    VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = {};
    depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencilCreateInfo.depthTestEnable = VK_TRUE;				// Enable checking depth to determine fragment write
    depthStencilCreateInfo.depthWriteEnable = VK_TRUE;				// Enable writing to depth buffer (to replace old values)
    depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;		// Comparison operation that allows an overwrite (is in front)
    depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;		// Depth Bounds Test: Does the depth value exist between two bounds
    depthStencilCreateInfo.stencilTestEnable = VK_FALSE;			// Enable Stencil Test


    // -- GRAPHICS PIPELINE CREATION --
    VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
    pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineCreateInfo.stageCount = 2;									// Number of shader stages
    pipelineCreateInfo.pStages = shaderStages;							// List of shader stages
    pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;		// All the fixed function pipeline states
    pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
    pipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
    pipelineCreateInfo.pDynamicState = nullptr;
    pipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
    pipelineCreateInfo.pMultisampleState = &multisamplingCreateInfo;
    pipelineCreateInfo.pColorBlendState = &colourBlendingCreateInfo;
    pipelineCreateInfo.pDepthStencilState = &depthStencilCreateInfo;
    pipelineCreateInfo.layout = mPipelineLayout;							// Pipeline Layout pipeline should use
    pipelineCreateInfo.renderPass = mRenderPass;					    // Render pass description the pipeline is compatible with
    pipelineCreateInfo.subpass = 0;										// Subpass of render pass to use with pipeline

    // Pipeline Derivatives : Can create multiple pipelines that derive from one another for optimisation
    pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;	// Existing pipeline to derive from...
    pipelineCreateInfo.basePipelineIndex = -1;				// or index of pipeline being created to derive from (in case creating multiple at once)

    // Create Graphics Pipeline
    result = vkCreateGraphicsPipelines(mDevice, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &mGraphicsPipeline);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a Graphics Pipeline!");
    }

    // Destroy Shader Modules, no longer needed after Pipeline created
    vkDestroyShaderModule(mDevice, fragmentShaderModule, nullptr);
    vkDestroyShaderModule(mDevice, vertexShaderModule, nullptr);
}

void VkRenderer::CreateDepthBufferImage()
{
    aout << "CreateDepthBufferImage" << endl;

    // Get supported format for depth buffer
    VkFormat depthFormat = ChooseSupportedFormat(
            { VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT },
            VK_IMAGE_TILING_OPTIMAL,
            VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

    // Create Depth Buffer Image
    mDepthBufferImage = CreateImage(mSwapChainExtent.width, mSwapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL,
                                   VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &mDepthBufferImageMemory);

    // Create Depth Buffer Image View
    mDepthBufferImageView = SwapChain::CreateImageView(mDevice, mDepthBufferImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
}


std::vector<char> VkRenderer::ReadFile(const char* shaderFile) {
    aout << " AAssetManager_open " << shaderFile << endl;
    vector<char> buf;
    int fileSize;
    AAsset* asset = AAssetManager_open(mAssetManager, shaderFile, AASSET_MODE_UNKNOWN);
    if ( asset != NULL ) {
        fileSize = AAsset_getLength(asset);
        if ( fileSize != 0 ) {
            buf.resize(fileSize);
            AAsset_read(asset, buf.data(), fileSize);
        }
        AAsset_close(asset);
    }
    return buf;
}

VkShaderModule VkRenderer::CreateShaderModule(string_view code, VkShaderType shaderType)
{
    std::vector<uint32_t> binary;
    vkCompileShader(code, shaderType, &binary);

    // Shader Module creation information
    VkShaderModuleCreateInfo shaderModuleCreateInfo = {};
    shaderModuleCreateInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shaderModuleCreateInfo.codeSize = binary.size() * sizeof(uint32_t);										// Size of code
    shaderModuleCreateInfo.pCode = binary.data();		// Pointer to code (of uint32_t pointer type)

    VkShaderModule shaderModule;
    VkResult result = vkCreateShaderModule(mDevice, &shaderModuleCreateInfo, nullptr, &shaderModule);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a shader module!");
    }
    return shaderModule;
}

void VkRenderer::CreateSwapChain()
{
    aout << "CreateSwapChain()" << endl;

    SwapChainDetails swapChainDetails = SwapChain::GetSwapChainDetails(mPhysicalDevice, mSurface);
    VkSurfaceFormatKHR surfaceFormat = SwapChain::ChooseBestSurfaceFormat(swapChainDetails.formats);
    VkPresentModeKHR presentMode = SwapChain::ChooseBestPresentationMode(swapChainDetails.presentationModes);
    VkExtent2D extent = swapChainDetails.surfaceCapabilities.currentExtent;

    // How many images are in the swap chain? Get 1 more than the minimum to allow triple buffering
    uint32_t imageCount = swapChainDetails.surfaceCapabilities.minImageCount + 1;

    // If imageCount higher than max, then clamp down to max,
    // If 0, then limitless
    if (swapChainDetails.surfaceCapabilities.maxImageCount > 0
        && swapChainDetails.surfaceCapabilities.maxImageCount < imageCount)
    {
        imageCount = swapChainDetails.surfaceCapabilities.maxImageCount;
    }

    // Creation information for swap chain
    VkSwapchainCreateInfoKHR swapChainCreateInfo = {};
    swapChainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapChainCreateInfo.surface = mSurface;
    swapChainCreateInfo.imageFormat = surfaceFormat.format;
    swapChainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
    swapChainCreateInfo.presentMode = presentMode;
    swapChainCreateInfo.imageExtent = extent;
    swapChainCreateInfo.minImageCount = imageCount;
    swapChainCreateInfo.imageArrayLayers = 1;
    swapChainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    swapChainCreateInfo.preTransform = swapChainDetails.surfaceCapabilities.currentTransform;	// Transform to perform on swap chain images
    swapChainCreateInfo.clipped = VK_TRUE;
    swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    // VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR 지원안함..
    if (swapChainDetails.surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) {
        swapChainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    }
    if (!(swapChainDetails.surfaceCapabilities.supportedUsageFlags & swapChainCreateInfo.imageUsage)) {
        aout << "Error not supported image usage" << endl;
    }

    if (mQueueFamilyIndices.graphicsFamily != mQueueFamilyIndices.presentationFamily)
    {
        // Queues to share between
        uint32_t queueFamilyIndices[] = {
                (uint32_t)mQueueFamilyIndices.graphicsFamily,
                (uint32_t)mQueueFamilyIndices.presentationFamily
        };
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;		// Image share handling
        swapChainCreateInfo.queueFamilyIndexCount = 2;							// Number of queues to share images between
        swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;			// Array of queues to share between
    }
    else
    {
        swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapChainCreateInfo.queueFamilyIndexCount = 0;
        swapChainCreateInfo.pQueueFamilyIndices = nullptr;
    }

    // IF old swap chain been destroyed and this one replaces it, then link old one to quickly hand over responsibilities
    swapChainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    // Create Swapchain
    VkResult result = vkCreateSwapchainKHR(mDevice, &swapChainCreateInfo, nullptr, &mSwapChain);
    if (result != VK_SUCCESS)
    {
        aout << "Failed to create a Swapchain!" << endl;
    }

    // Store for later reference
    mSwapChainImageFormat = surfaceFormat.format;
    mSwapChainExtent = extent;

    // Get swap chain images (first count, then values)
    uint32_t swapChainImageCount;
    vkGetSwapchainImagesKHR(mDevice, mSwapChain, &swapChainImageCount, nullptr);
    std::vector<VkImage> images(swapChainImageCount);
    vkGetSwapchainImagesKHR(mDevice, mSwapChain, &swapChainImageCount, images.data());

    for (VkImage image : images)
    {
        SwapchainImage swapChainImage = {};
        swapChainImage.image = image;
        swapChainImage.imageView = SwapChain::CreateImageView(mDevice, image, mSwapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
        mSwapChainImages.push_back(swapChainImage); // Add to swapchain image list// Add to swapchain image list
    }

    aout << "SwapChain Creation Information " << endl;
    aout << "SwapchainImage count : " << swapChainImageCount << endl;
    aout << "format : " << swapChainCreateInfo.imageFormat << " color Space : " << swapChainCreateInfo.imageColorSpace << endl;
    aout << "present mode : " << swapChainCreateInfo.presentMode << endl;
    aout << "extent Width : " << swapChainCreateInfo.imageExtent.width << " Height : " << swapChainCreateInfo.imageExtent.height << endl;
    aout << " ======================================== " << endl;
}

void VkRenderer::CreateSurface() {
    aout << "CreateSurface" << endl;
    VkAndroidSurfaceCreateInfoKHR surfaceCreateInfo {
            .sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
            .window = mWindow
    };
    vkCreateAndroidSurfaceKHR(mInstance, &surfaceCreateInfo, nullptr, &mSurface);
}

void VkRenderer::CreateInstance() {
    aout << "CreateInstance()" << endl;
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Vulkan App";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "NO ENGINE";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = 0;
    createInfo.ppEnabledExtensionNames = nullptr;
    createInfo.enabledLayerCount = 0;
    createInfo.ppEnabledLayerNames = nullptr;

    vector<string> extensions {
            "VK_KHR_surface",
            "VK_KHR_android_surface"
    };
    vector<string> layer {
            "VK_LAYER_KHRONOS_validation"
    };
    if (CheckInstanceExtension(extensions)) {
        createInfo.enabledExtensionCount = mExtensionNames.size();
        createInfo.ppEnabledExtensionNames = mExtensionNames.data();
    }
    else {
        aout << "extension is nothing ..." << endl;
    }
    if (CheckInstanceLayer(layer)) {
        createInfo.enabledLayerCount = mValidationLayers.size();
        createInfo.ppEnabledLayerNames = mValidationLayers.data();
    } else {
        aout << "layer is nothing ..." << endl;
    }

    auto ret = vkCreateInstance(&createInfo, nullptr, &mInstance);
    if (ret != VK_SUCCESS) {
        aout << " instance is not created " << ret << endl;
    }
    else {
        aout << "vulkan instance is created..." << endl;
    }
}

bool VkRenderer::CheckInstanceExtension(vector<string>& request) {
    uint32_t extensionCount;
    VK_CHECK_ERROR(vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr));
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

    mExtensionNames.clear();
    for (auto extension: extensions) {
        auto it = find(request.begin(), request.end(), extension.extensionName);
        if (it != request.end()) {
            mExtensionNames.push_back(it->c_str());
            aout << "Instance Extension : " << extension.extensionName << endl;
        }
    }
    if (mExtensionNames.empty()) return false;
    return true;
}

bool VkRenderer::CheckInstanceLayer(std::vector<string>& request) {
    uint32_t validationLayerCount;
    VK_CHECK_ERROR(vkEnumerateInstanceLayerProperties(&validationLayerCount, nullptr));
    std::vector<VkLayerProperties> layers(validationLayerCount);
    VK_CHECK_ERROR(vkEnumerateInstanceLayerProperties(&validationLayerCount, layers.data()));

    mValidationLayers.clear();
    for (auto &layer: layers) {
        auto it = find(request.begin(), request.end(), layer.layerName);
        if (it != request.end()) {
            aout << "Instance Layer : " << layer.layerName << std::endl;
            mValidationLayers.push_back(it->c_str());
        }
    }
    if (mValidationLayers.empty()) return false;
    return true;
}

bool VkRenderer::CheckDeviceExtension(VkPhysicalDevice device, std::vector<std::string> &request) {
    uint32_t deviceExtensionCount;
    vkEnumerateDeviceExtensionProperties(mPhysicalDevice, nullptr, &deviceExtensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(deviceExtensionCount);
    vkEnumerateDeviceExtensionProperties(mPhysicalDevice, nullptr, &deviceExtensionCount,
    extensions.data());

    mDeviceExtensionNames.clear();
    for (const auto &properties: extensions) {
        auto it = find(request.begin(), request.end(), properties.extensionName);
        if (it != request.end()) {
            aout << "Device Extension : " << properties.extensionName << std::endl;
            mDeviceExtensionNames.push_back(properties.extensionName);
        }
    }
    if (mDeviceExtensionNames.empty()) return false;
    return true;
}

void VkRenderer::GetPhysicalDevice() {
    aout << "PhysicalDevice()" << endl;

    uint32_t deviceCount;
    vkEnumeratePhysicalDevices(mInstance, &deviceCount, nullptr);
    if (deviceCount == 0) return;

    std::vector<VkPhysicalDevice> deviceList(deviceCount);
    vkEnumeratePhysicalDevices(mInstance, &deviceCount, deviceList.data());

    // rendering 가능한 physical device selection
    for (const auto &device: deviceList) {
        QueueFamilyIndices indices = GetQueueFamilies(device);

        if (indices.isValid()) {
            mPhysicalDevice = device;
            mQueueFamilyIndices.graphicsFamily = indices.graphicsFamily;
            mQueueFamilyIndices.presentationFamily = indices.presentationFamily;
            break;
        }
    }

    // Get properties of our new device
    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(mPhysicalDevice, &deviceProperties);

    //mMinUniformBufferOffset = deviceProperties.limits.minUniformBufferOffsetAlignment;

    if (mPhysicalDevice != nullptr) {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(mPhysicalDevice, &deviceProperties);
        aout << "device ID : " << deviceProperties.deviceID << endl;
        aout << "device Name : " << deviceProperties.deviceName << endl;
        aout << "device Type : " << deviceProperties.deviceType << endl;
        aout << "device Version : " << deviceProperties.driverVersion << endl;

        VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
        vkGetPhysicalDeviceMemoryProperties(mPhysicalDevice, &deviceMemoryProperties);
        aout << "device MemoryTypes : " << deviceMemoryProperties.memoryTypes << endl;
        aout << "device memoryHeaps : " << deviceMemoryProperties.memoryHeaps << endl;
    } else {
        aout << "device is nothing..." << endl;
    }
}

VkFormat VkRenderer::ChooseSupportedFormat(const std::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags)
{
    // Loop through options and find compatible one
    for (VkFormat format : formats)
    {
        // Get properties for give format on this device
        VkFormatProperties properties;
        vkGetPhysicalDeviceFormatProperties(mPhysicalDevice, format, &properties);

        // Depending on tiling choice, need to check for different bit flag
        if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & featureFlags) == featureFlags)
        {
            return format;
        }
        else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & featureFlags) == featureFlags)
        {
            return format;
        }
    }

    throw std::runtime_error("Failed to find a matching format!");
}

VkImage VkRenderer::CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags useFlags, VkMemoryPropertyFlags propFlags, VkDeviceMemory * imageMemory)
{
    // CREATE IMAGE
    // Image Creation Info
    VkImageCreateInfo imageCreateInfo = {};
    imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;						// Type of image (1D, 2D, or 3D)
    imageCreateInfo.extent.width = width;								// Width of image extent
    imageCreateInfo.extent.height = height;								// Height of image extent
    imageCreateInfo.extent.depth = 1;									// Depth of image (just 1, no 3D aspect)
    imageCreateInfo.mipLevels = 1;										// Number of mipmap levels
    imageCreateInfo.arrayLayers = 1;									// Number of levels in image array
    imageCreateInfo.format = format;									// Format type of image
    imageCreateInfo.tiling = tiling;									// How image data should be "tiled" (arranged for optimal reading)
    imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;			// Layout of image data on creation
    imageCreateInfo.usage = useFlags;									// Bit flags defining what image will be used for
    imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;					// Number of samples for multi-sampling
    imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;			// Whether image can be shared between queues

    // Create image
    VkImage image;
    VkResult result = vkCreateImage(mDevice, &imageCreateInfo, nullptr, &image);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create an Image!");
    }

    // CREATE MEMORY FOR IMAGE

    // Get memory requirements for a type of image
    VkMemoryRequirements memoryRequirements;
    vkGetImageMemoryRequirements(mDevice, image, &memoryRequirements);

    // Allocate memory using image requirements and user defined properties
    VkMemoryAllocateInfo memoryAllocInfo = {};
    memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocInfo.allocationSize = memoryRequirements.size;
    memoryAllocInfo.memoryTypeIndex = findMemoryTypeIndex(mPhysicalDevice, memoryRequirements.memoryTypeBits, propFlags);

    result = vkAllocateMemory(mDevice, &memoryAllocInfo, nullptr, imageMemory);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate memory for image!");
    }

    // Connect memory to image
    vkBindImageMemory(mDevice, image, *imageMemory, 0);

    return image;
}

void VkRenderer::AllocateDynamicBufferTransferSpace()
{
    /*
    // Calculate alignment of model data
    mModelUniformAlignment = (sizeof(UboModel) + mMinUniformBufferOffset - 1)
                            & ~(mMinUniformBufferOffset - 1);

    // Create space in memory to hold dynamic buffer that is aligned to our required alignment and holds MAX_OBJECTS

    mModelTransferSpace = (UboModel *)malloc(mModelUniformAlignment * MAX_OBJECTS);
    // mModelTransferSpace = (UboModel *)aligned_alloc(mModelUniformAlignment * MAX_OBJECTS, mModelUniformAlignment); // dlgmlals3
    */
}

int VkRenderer::CreateTexture(std::string fileName)
{
    // Create Texture Image and get its location in array
    int textureImageLoc = CreateTextureImage(fileName);

    // Create Image View and add to list
    VkImageView imageView = SwapChain::CreateImageView(mDevice, mTextureImages[textureImageLoc], VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT);
    mTextureImageViews.push_back(imageView);

    // Create Texture Descriptor
    int descriptorLoc = CreateTextureDescriptor(imageView);

    // Return location of set with texture
    return descriptorLoc;
}

int VkRenderer::CreateTextureDescriptor(VkImageView textureImage)
{
    VkDescriptorSet descriptorSet;

    // Descriptor Set Allocation Info
    VkDescriptorSetAllocateInfo setAllocInfo = {};
    setAllocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    setAllocInfo.descriptorPool = mSamplerDescriptorPool;
    setAllocInfo.descriptorSetCount = 1;
    setAllocInfo.pSetLayouts = &mSamplerSetLayout;

    // Allocate Descriptor Sets
    VkResult result = vkAllocateDescriptorSets(mDevice, &setAllocInfo, &descriptorSet);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate Texture Descriptor Sets!");
    }

    // Texture Image Info
    VkDescriptorImageInfo imageInfo = {};
    imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;	// Image layout when in use
    imageInfo.imageView = textureImage;									// Image to bind to set
    imageInfo.sampler = mTextureSampler;									// Sampler to use for set

    // Descriptor Write Info
    VkWriteDescriptorSet descriptorWrite = {};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &imageInfo;

    // Update new descriptor set
    vkUpdateDescriptorSets(mDevice, 1, &descriptorWrite, 0, nullptr);

    // Add descriptor set to list
    mSamplerDescriptorSets.push_back(descriptorSet);

    // Return descriptor set location
    return mSamplerDescriptorSets.size() - 1;
}


int VkRenderer::CreateTextureImage(std::string fileName)
{
    aout << "CreateTextureImage " << endl;
    VkTexture texture;
    VkTextureCreateInfo textureCreateInfo{
            .sType = VK_STRUCTURE_TYPE_TEXTURE_CREATE_INFO,
            .pAssetManager = mAssetManager,
            .pFileName = fileName.c_str()
    };
    VK_CHECK_ERROR(vkCreateTexture(mDevice, &textureCreateInfo, nullptr, &texture));

    // VkTexture 속성 얻기
    VkTextureProperties textureProperties;
    vkGetTextureProperties(texture, &textureProperties);

    auto width = textureProperties.extent.width;
    auto height = textureProperties.extent.height;
    VkDeviceSize imageSize = width * height * 4;

    // Create staging buffer to hold loaded data, ready to copy to device
    VkBuffer imageStagingBuffer;
    VkDeviceMemory imageStagingBufferMemory;
    createBuffer(mPhysicalDevice, mDevice, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 &imageStagingBuffer, &imageStagingBufferMemory);

    // Copy image data to staging buffer
    void *data;
    vkMapMemory(mDevice, imageStagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, textureProperties.pData, static_cast<size_t>(imageSize));
    vkUnmapMemory(mDevice, imageStagingBufferMemory);

    // Free original image data
    // stbi_image_free(imageData);

    // Create image to hold final texture
    VkImage texImage;
    VkDeviceMemory texImageMemory;

    texImage = CreateImage(width, height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
                           VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &texImageMemory);


    // COPY DATA TO IMAGE
    // Transition image to be DST for copy operation
    transitionImageLayout(mDevice, mGraphicQueue, mGraphicsCommandPool,
                          texImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // Copy image data
    copyImageBuffer(mDevice, mGraphicQueue, mGraphicsCommandPool, imageStagingBuffer, texImage, width, height);

    // Transition image to be shader readable for shader usage
    transitionImageLayout(mDevice, mGraphicQueue, mGraphicsCommandPool,
                          texImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    // Add texture data to vector for reference
    mTextureImages.push_back(texImage);
    mTextureImageMemory.push_back(texImageMemory);

    // Destroy staging buffers
    vkDestroyBuffer(mDevice, imageStagingBuffer, nullptr);
    vkFreeMemory(mDevice, imageStagingBufferMemory, nullptr);

    // Return index of new texture image
    return mTextureImages.size() - 1;
}

void VkRenderer::CreateLogicalDevice() {
    aout << "CreateLogicalDevice()" << endl;

    VkPhysicalDeviceFeatures deviceFeatures = {};
    deviceFeatures.samplerAnisotropy = VK_TRUE;		// Enable Anisotropy

    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = mQueueFamilyIndices.graphicsFamily;
    queueCreateInfo.queueCount = 1;
    float priority = 1.0f;
    queueCreateInfo.pQueuePriorities = &priority;

    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = 1;
    deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;
    deviceCreateInfo.enabledExtensionCount = 0;
    deviceCreateInfo.ppEnabledExtensionNames = nullptr;

    vector<string> extensionNames { "VK_KHR_swapchain" };
    if (CheckDeviceExtension(mPhysicalDevice, extensionNames)) {
        deviceCreateInfo.enabledExtensionCount = static_cast<uint32_t>(mDeviceExtensionNames.size());
        deviceCreateInfo.ppEnabledExtensionNames = mDeviceExtensionNames.data();
    }

    VkResult result = vkCreateDevice(mPhysicalDevice, &deviceCreateInfo, nullptr, &mDevice);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to create a Logical Device!");
    }
    vkGetDeviceQueue(mDevice, mQueueFamilyIndices.graphicsFamily, 0, &mGraphicQueue);
    vkGetDeviceQueue(mDevice, mQueueFamilyIndices.graphicsFamily, 0, &mPresentationQueue);
}


QueueFamilyIndices VkRenderer::GetQueueFamilies(const VkPhysicalDevice& device) {
    QueueFamilyIndices indices;
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilyList(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilyList.data());

    int i = 0;
    for (const auto &queueFamily: queueFamilyList) {
        if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            indices.graphicsFamily = i;
        }

        VkBool32 presentationSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, mSurface, &presentationSupport);
        if (!presentationSupport) {
            aout << "surface is not supported to presentation" << endl;
        }

        // Check if queue is presentation type (can be both graphics and presentation)
        if (queueFamily.queueCount > 0 && presentationSupport)
        {
            indices.presentationFamily = i;
        }
        if (indices.isValid()) break;
        i++;
    }
    return indices;
}



