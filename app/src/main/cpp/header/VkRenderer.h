#ifndef VULKAN2_VKRENDERER_H
#define VULKAN2_VKRENDERER_H

#include <vector>
#include <stdint.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_android.h>
#include <game-activity/GameActivity.h>
#include <string>

#include "VkRenderer.h"
#include "VkUtil.h"
#include "AndroidOut.h"
#include "Mesh.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class VkRenderer {
public:
    ~VkRenderer();
    VkRenderer(ANativeWindow *window, AAssetManager *assetManager);
    void Draw();
    void UpdateModel(int modelId, glm::mat4 newModel);

private:
    void CleanUp();
    void CreateInstance();
    void GetPhysicalDevice();
    void CreateLogicalDevice();
    void CreateSurface();
    void CreateSwapChain();
    void CreateRenderPass();
    void CreateDescriptorSetLayout();
    void CreatePushConstantRange();
    void CreateGraphicsPipeline();
    void CreateDepthBufferImage();

    void CreateFramebuffers();
    void CreateCommandPool();
    void CreateCommandBuffers();
    void RecordCommands(uint32_t currentImage);

    void CreateSynchronisation();
    void CreateTextureSampler();

    void CreateUniformBuffers();
    void CreateDescriptorPool();
    void CreateDescriptorSets();
    void UpdateUniformBuffers(uint32_t imageIndex);

    QueueFamilyIndices GetQueueFamilies(const VkPhysicalDevice& device);
    bool CheckInstanceLayer(std::vector<std::string> &);
    bool CheckInstanceExtension(std::vector<std::string> &);
    bool CheckDeviceExtension(VkPhysicalDevice device, std::vector<std::string> &);

    VkShaderModule CreateShaderModule(std::string_view code, VkShaderType shaderType);
    std::vector<char> ReadFile(const char* shaderFile);
    void AllocateDynamicBufferTransferSpace();

    VkFormat ChooseSupportedFormat(const std::vector<VkFormat> &formats, VkImageTiling tiling, VkFormatFeatureFlags featureFlags);
    VkImage CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags useFlags,
                        VkMemoryPropertyFlags propFlags, VkDeviceMemory *imageMemory);

    int CreateTextureImage(std::string fileName);
    int CreateTexture(std::string fileName);
    int CreateTextureDescriptor(VkImageView textureImage);

    // Initialize
    AAssetManager *mAssetManager;
    VkInstance mInstance;
    VkPhysicalDevice mPhysicalDevice;
    QueueFamilyIndices mQueueFamilyIndices;
    VkDevice mDevice;
    VkQueue mGraphicQueue;

    VkPhysicalDeviceMemoryProperties mPhysicalDeviceMemoryProperties;
    std::vector<const char*> mExtensionNames;
    std::vector<const char*> mValidationLayers;
    std::vector<const char*> mDeviceExtensionNames;

    // Framebuffer, CommandBuffer
    std::vector<VkFramebuffer> mSwapChainFramebuffers;
    std::vector<VkCommandBuffer> mCommandBuffers; // 1:1 framebuffer

    VkImage mDepthBufferImage;
    VkDeviceMemory mDepthBufferImageMemory;
    VkImageView mDepthBufferImageView;

    VkSampler mTextureSampler;


    // SwapChain
    ANativeWindow* mWindow;
    VkQueue mPresentationQueue;
    VkSurfaceKHR mSurface;
    VkFormat mSwapChainImageFormat;
    VkExtent2D mSwapChainExtent;
    VkSwapchainKHR mSwapChain;
    std::vector<SwapchainImage> mSwapChainImages;

    // Pipe line
    VkRenderPass mRenderPass;
    VkPipelineLayout mPipelineLayout;
    VkPipeline mGraphicsPipeline;

    // CommandPool
    VkCommandPool mGraphicsCommandPool;

    // - Descriptors
    VkDescriptorSetLayout mDescriptorSetLayout;
    VkDescriptorSetLayout mSamplerSetLayout;
    VkPushConstantRange mPushConstantRange;

    VkDescriptorPool mDescriptorPool;
    VkDescriptorPool mSamplerDescriptorPool;
    std::vector<VkDescriptorSet> mSamplerDescriptorSets;
    std::vector<VkDescriptorSet> mDescriptorSets;

    std::vector<VkBuffer> vpUniformBuffer;
    std::vector<VkDeviceMemory> vpUniformBufferMemory;

    std::vector<VkBuffer> modelDUniformBuffer;
    std::vector<VkDeviceMemory> modelDUniformBufferMemory;

    /*
    VkDeviceSize mMinUniformBufferOffset;
    size_t mModelUniformAlignment;
    Model * mModelTransferSpace;
    */

    // - Synchronisation
    int mCurrentFrame = 0;
    std::vector<VkSemaphore> mImageAvailable;
    std::vector<VkSemaphore> mRenderFinished;
    std::vector<VkFence> mDrawFences;

    // - Assets
    std::vector<VkImage> mTextureImages;
    std::vector<VkDeviceMemory> mTextureImageMemory;
    std::vector<VkImageView> mTextureImageViews;

    std::vector<Mesh> mMeshList;

    // Create a mesh
    std::vector<Vertex> mVertices = {
        {{0.4, -0.4, 0.0}, {1.0f, 0.0f, 0.0f}},
        {{0.4, 0.4, 0.0}, {0.0f, 1.0f, 0.0f}},
        {{-0.4, 0.4, 0.0}, {0.0f, 0.0f, 1.0f}},

        { { -0.4, 0.4, 0.0 }, {0.0f, 0.0f, 1.0f}},
        { { -0.4, -0.4, 0.0 }, {1.0f, 1.0f, 0.0f} },
        { { 0.4, -0.4, 0.0 }, {1.0f, 0.0f, 0.0f} }
    };


    /*
     * Triangle
    std::vector<Vertex> mVertices = {
        {{0.0, -0.4, 0.0}, {1.0f, 0.0f, 0.0f}},
        {{0.4, 0.4, 0.0}, {0.0f, 1.0f, 0.0f}},
        {{-0.4, 0.4, 0.0}, {0.0f, 0.0f, 1.0f}},
    };
    */
    UboViewProjection mUboViewProjection;

};


#endif //VULKAN2_VKRENDERER_H
