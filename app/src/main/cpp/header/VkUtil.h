// MIT License
//
// Copyright (c) 2024 Daemyung Jang
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef PRACTICE_VULKAN_VKUTIL_H
#define PRACTICE_VULKAN_VKUTIL_H


#include <string>
#include <string_view>
#include <vector>
#include <random>
#include <vulkan/vulkan.h>
#include <fstream>
#include <glm/glm.hpp>
#include "AndroidOut.h"
#include <shaderc/shaderc.hpp>

#ifndef NDEBUG

const int MAX_OBJECTS = 2;

// Scene Settings
// Scene Settings

// Scene Settings
struct UboViewProjection {
    glm::mat4 projection;
    glm::mat4 view;
};

struct Vertex // Vertex data representation
{
    glm::vec3 pos; // Vertex Position (x, y, z)
    glm::vec3 col; // Vertex Colour (r, g, b)
    glm::vec2 tex; // Texture Coords (u, v)
};

const int MAX_FRAME_DRAWS = 2;

static uint32_t findMemoryTypeIndex(VkPhysicalDevice physicalDevice, uint32_t allowedTypes, VkMemoryPropertyFlags properties)
{
    // Get properties of physical device memory
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++)
    {
        if ((allowedTypes & (1 << i))														// Index of memory type must match corresponding bit in allowedTypes
            && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)	// Desired property bit flags are part of memory type's property flags
        {
            // This memory type is valid, so return its index
            return i;
        }
    }
}

static void createBuffer(VkPhysicalDevice physicalDevice, VkDevice device, VkDeviceSize bufferSize, VkBufferUsageFlags bufferUsage,
                         VkMemoryPropertyFlags bufferProperties, VkBuffer * buffer, VkDeviceMemory * bufferMemory)
{
    // CREATE VERTEX BUFFER
    // Information to create a buffer (doesn't include assigning memory)
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;								// Size of buffer (size of 1 vertex * number of vertices)
    bufferInfo.usage = bufferUsage;								// Multiple types of buffer possible
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;			// Similar to Swap Chain images, can share vertex buffers

    VkResult result = vkCreateBuffer(device, &bufferInfo, nullptr, buffer);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create a Vertex Buffer!");
    }

    // GET BUFFER MEMORY REQUIREMENTS
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, *buffer, &memRequirements);

    // ALLOCATE MEMORY TO BUFFER
    VkMemoryAllocateInfo memoryAllocInfo = {};
    memoryAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    memoryAllocInfo.allocationSize = memRequirements.size;
    memoryAllocInfo.memoryTypeIndex = findMemoryTypeIndex(physicalDevice, memRequirements.memoryTypeBits,		// Index of memory type on Physical Device that has required bit flags
                                                          bufferProperties);																						// VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT	: CPU can interact with memory
    // VK_MEMORY_PROPERTY_HOST_COHERENT_BIT	: Allows placement of data straight into buffer after mapping (otherwise would have to specify manually)
    // Allocate memory to VkDeviceMemory
    result = vkAllocateMemory(device, &memoryAllocInfo, nullptr, bufferMemory);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to allocate Vertex Buffer Memory!");
    }

    // Allocate memory to given vertex buffer
    vkBindBufferMemory(device, *buffer, *bufferMemory, 0);
}

static VkCommandBuffer beginCommandBuffer(VkDevice device, VkCommandPool commandPool)
{
    // Command buffer to hold transfer commands
    VkCommandBuffer commandBuffer;

    // Command Buffer details
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = commandPool;
    allocInfo.commandBufferCount = 1;

    // Allocate command buffer from pool
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    // Information to begin the command buffer record
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;	// We're only using the command buffer once, so set up for one time submit

    // Begin recording transfer commands
    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

static void endAndSubmitCommandBuffer(VkDevice device, VkCommandPool commandPool, VkQueue queue, VkCommandBuffer commandBuffer)
{
    // End commands
    vkEndCommandBuffer(commandBuffer);

    // Queue submission information
    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    // Submit transfer command to transfer queue and wait until it finishes
    vkQueueSubmit(queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(queue);

    // Free temporary command buffer back to pool
    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

static void copyBuffer(VkDevice device, VkQueue transferQueue, VkCommandPool transferCommandPool,
                       VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize bufferSize)
{
    // Create buffer
    VkCommandBuffer transferCommandBuffer = beginCommandBuffer(device, transferCommandPool);

    // Region of data to copy from and to
    VkBufferCopy bufferCopyRegion = {};
    bufferCopyRegion.srcOffset = 0;
    bufferCopyRegion.dstOffset = 0;
    bufferCopyRegion.size = bufferSize;

    // Command to copy src buffer to dst buffer
    vkCmdCopyBuffer(transferCommandBuffer, srcBuffer, dstBuffer, 1, &bufferCopyRegion);

    endAndSubmitCommandBuffer(device, transferCommandPool, transferQueue, transferCommandBuffer);
}

static void copyImageBuffer(VkDevice device, VkQueue transferQueue, VkCommandPool transferCommandPool,
                            VkBuffer srcBuffer, VkImage image, uint32_t width, uint32_t height)
{
    // Create buffer
    VkCommandBuffer transferCommandBuffer = beginCommandBuffer(device, transferCommandPool);

    VkBufferImageCopy imageRegion = {};
    imageRegion.bufferOffset = 0;											// Offset into data
    imageRegion.bufferRowLength = 0;										// Row length of data to calculate data spacing
    imageRegion.bufferImageHeight = 0;										// Image height to calculate data spacing
    imageRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;	// Which aspect of image to copy
    imageRegion.imageSubresource.mipLevel = 0;								// Mipmap level to copy
    imageRegion.imageSubresource.baseArrayLayer = 0;						// Starting array layer (if array)
    imageRegion.imageSubresource.layerCount = 1;							// Number of layers to copy starting at baseArrayLayer
    imageRegion.imageOffset = { 0, 0, 0 };									// Offset into image (as opposed to raw data in bufferOffset)
    imageRegion.imageExtent = { width, height, 1 };							// Size of region to copy as (x, y, z) values

    // Copy buffer to given image
    vkCmdCopyBufferToImage(transferCommandBuffer, srcBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &imageRegion);

    endAndSubmitCommandBuffer(device, transferCommandPool, transferQueue, transferCommandBuffer);
}

static void transitionImageLayout(VkDevice device, VkQueue queue, VkCommandPool commandPool, VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout)
{
    // Create buffer
    VkCommandBuffer commandBuffer = beginCommandBuffer(device, commandPool);

    VkImageMemoryBarrier imageMemoryBarrier = {};
    imageMemoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    imageMemoryBarrier.oldLayout = oldLayout;									// Layout to transition from
    imageMemoryBarrier.newLayout = newLayout;									// Layout to transition to
    imageMemoryBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;			// Queue family to transition from
    imageMemoryBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;			// Queue family to transition to
    imageMemoryBarrier.image = image;											// Image being accessed and modified as part of barrier
    imageMemoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;	// Aspect of image being altered
    imageMemoryBarrier.subresourceRange.baseMipLevel = 0;						// First mip level to start alterations on
    imageMemoryBarrier.subresourceRange.levelCount = 1;							// Number of mip levels to alter starting from baseMipLevel
    imageMemoryBarrier.subresourceRange.baseArrayLayer = 0;						// First layer to start alterations on
    imageMemoryBarrier.subresourceRange.layerCount = 1;							// Number of layers to alter starting from baseArrayLayer

    VkPipelineStageFlags srcStage;
    VkPipelineStageFlags dstStage;

    // If transitioning from new image to image ready to receive data...
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
        imageMemoryBarrier.srcAccessMask = 0;								// Memory access stage transition must after...
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;		// Memory access stage transition must before...

        srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
        // If transitioning from transfer destination to shader readable...
    else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
        imageMemoryBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        imageMemoryBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    }

    vkCmdPipelineBarrier(
            commandBuffer,
            srcStage, dstStage,		// Pipeline stages (match to src and dst AccessMasks)
            0,						// Dependency flags
            0, nullptr,				// Memory Barrier count + data
            0, nullptr,				// Buffer Memory Barrier count + data
            1, &imageMemoryBarrier	// Image Memory Barrier count + data
    );

    endAndSubmitCommandBuffer(device, commandPool, queue, commandBuffer);
}

struct SwapChainDetails {
    VkSurfaceCapabilitiesKHR surfaceCapabilities;		// Surface properties, e.g. image size/extent
    std::vector<VkSurfaceFormatKHR> formats;			// Surface image formats, e.g. RGBA and size of each colour
    std::vector<VkPresentModeKHR> presentationModes;	// How images should be presented to screen
};

struct SwapchainImage {
    VkImage image;
    VkImageView imageView;
};

// Indices (locations) of Queue Families (if they exist at all)
struct QueueFamilyIndices {
    int graphicsFamily = -1;
    int presentationFamily = -1;
    bool isValid()
    {
        return graphicsFamily >= 0 && presentationFamily >= 0;
    }
};

typedef enum VkShaderType {
    VK_SHADER_TYPE_VERTEX = shaderc_vertex_shader,
    VK_SHADER_TYPE_FRAGMENT = shaderc_fragment_shader
} VkShaderType;

inline VkResult
vkGetMemoryTypeIndex(const VkPhysicalDeviceMemoryProperties &physicalDeviceMemoryProperties,
                     const VkMemoryRequirements &memoryRequirements,
                     VkMemoryPropertyFlags memoryPropertyFlags,
                     uint32_t *memoryTypeIndex) {
    for (auto i = 0; i != physicalDeviceMemoryProperties.memoryTypeCount; ++i) {
        if (memoryRequirements.memoryTypeBits & (1 << i)) {
            if ((physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags &
                 memoryPropertyFlags) == memoryPropertyFlags) {
                *memoryTypeIndex = i;
                return VK_SUCCESS;
            }
        }
    }

    *memoryTypeIndex = UINT32_MAX;
    return VK_ERROR_UNKNOWN;
}

static std::vector<char> ReadFile_(const std::string &filename)
{
    // Open stream from given file
    // std::ios::binary tells stream to read file as binary
    // std::ios::ate tells stream to start reading from end of file
    std::ifstream file(filename, std::ios::binary | std::ios::ate);

    // Check if file stream successfully opened
    if (!file.is_open())
    {
        throw std::runtime_error("Failed to open a file!");
    }

    // Get current read position and use to resize file buffer
    size_t fileSize = (size_t)file.tellg();
    std::vector<char> fileBuffer(fileSize);

    // Move read position (seek to) the start of the file
    file.seekg(0);

    // Read the file data into the buffer (stream "fileSize" in total)
    file.read(fileBuffer.data(), fileSize);

    // Close stream
    file.close();

    return fileBuffer;
}

inline VkResult
vkCompileShader(std::string_view shaderCode, VkShaderType shaderType,
                std::vector<uint32_t> *shaderBinary) {
    std::random_device device;
    std::mt19937 generator(device());
    std::uniform_int_distribution<> distribution('a', 'z');
    std::string tag('a', 4);

    for (auto i = 0; i < 16; ++i) {
        tag[i] = static_cast<char>(distribution(generator));
    }
    aout << "tag : " << tag << std::endl;

    shaderc::Compiler compiler;
    auto result = compiler.CompileGlslToSpv(shaderCode.data(),
                                            shaderCode.size(),
                                            static_cast<shaderc_shader_kind>(shaderType),
                                            tag.c_str());

    if (result.GetCompilationStatus() != shaderc_compilation_status_success) {
        aout << result.GetErrorMessage() << std::endl;
        return VK_ERROR_UNKNOWN;
    }

    *shaderBinary = std::vector<uint32_t>(result.cbegin(), result.cend());
    return VK_SUCCESS;
}

inline std::string vkToString(VkResult vkResult) {
    switch (vkResult) {
        case VK_SUCCESS:
            return "VK_SUCCESS";
        case VK_NOT_READY:
            return "VK_NOT_READY";
        case VK_TIMEOUT:
            return "VK_TIMEOUT";
        case VK_EVENT_SET:
            return "VK_EVENT_SET";
        case VK_EVENT_RESET:
            return "VK_EVENT_RESET";
        case VK_INCOMPLETE:
            return "VK_INCOMPLETE";
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case VK_ERROR_INITIALIZATION_FAILED:
            return "VK_ERROR_INITIALIZATION_FAILED";
        case VK_ERROR_DEVICE_LOST:
            return "VK_ERROR_DEVICE_LOST";
        case VK_ERROR_MEMORY_MAP_FAILED:
            return "VK_ERROR_MEMORY_MAP_FAILED";
        case VK_ERROR_LAYER_NOT_PRESENT:
            return "VK_ERROR_LAYER_NOT_PRESENT";
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            return "VK_ERROR_EXTENSION_NOT_PRESENT";
        case VK_ERROR_FEATURE_NOT_PRESENT:
            return "VK_ERROR_FEATURE_NOT_PRESENT";
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            return "VK_ERROR_INCOMPATIBLE_DRIVER";
        case VK_ERROR_TOO_MANY_OBJECTS:
            return "VK_ERROR_TOO_MANY_OBJECTS";
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
            return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        case VK_ERROR_FRAGMENTED_POOL:
            return "VK_ERROR_FRAGMENTED_POOL";
        case VK_ERROR_UNKNOWN:
            return "VK_ERROR_UNKNOWN";
        case VK_ERROR_OUT_OF_POOL_MEMORY:
            return "VK_ERROR_OUT_OF_POOL_MEMORY";
        case VK_ERROR_INVALID_EXTERNAL_HANDLE:
            return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
        case VK_ERROR_FRAGMENTATION:
            return "VK_ERROR_FRAGMENTATION";
        case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
            return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
        case VK_PIPELINE_COMPILE_REQUIRED:
            return "VK_PIPELINE_COMPILE_REQUIRED";
        case VK_ERROR_SURFACE_LOST_KHR:
            return "VK_ERROR_SURFACE_LOST_KHR";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
            return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        case VK_SUBOPTIMAL_KHR:
            return "VK_SUBOPTIMAL_KHR";
        case VK_ERROR_OUT_OF_DATE_KHR:
            return "VK_ERROR_OUT_OF_DATE_KHR";
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
            return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
        case VK_ERROR_VALIDATION_FAILED_EXT:
            return "VK_ERROR_VALIDATION_FAILED_EXT";
        case VK_ERROR_INVALID_SHADER_NV:
            return "VK_ERROR_INVALID_SHADER_NV";
        case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
            return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
        case VK_ERROR_NOT_PERMITTED_KHR:
            return "VK_ERROR_NOT_PERMITTED_KHR";
        case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
            return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
        case VK_THREAD_IDLE_KHR:
            return "VK_THREAD_IDLE_KHR";
        case VK_THREAD_DONE_KHR:
            return "VK_THREAD_DONE_KHR";
        case VK_OPERATION_DEFERRED_KHR:
            return "VK_OPERATION_DEFERRED_KHR";
        case VK_OPERATION_NOT_DEFERRED_KHR:
            return "VK_OPERATION_NOT_DEFERRED_KHR";
        default:
            return "Unhandled VkResult";
    }
}

#define VK_CHECK_ERROR(vkFunction)                                                     \
    do {                                                                               \
        if (auto vkResult = vkFunction; vkResult != VK_SUCCESS) {                      \
            aout << #vkFunction << " returns " << vkToString(vkResult) << "." << endl; \
            abort();                                                                   \
        }                                                                              \
    } while (0)
#else
#define VK_CHECK_ERROR(vkFunction)                                                     \
    do {                                                                               \
        vkFunction                                                                     \
    } while (0)
#endif

#endif //PRACTICE_VULKAN_VKUTIL_H