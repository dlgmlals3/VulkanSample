#ifndef VULKAN2_SWAPCHAIN_H
#define VULKAN2_SWAPCHAIN_H

#include <stdint.h>
#include <vulkan/vulkan.h>
#include <vector>
#include "VkUtil.h"
#include "AndroidOut.h"

class SwapChain {
public:
    static VkSurfaceFormatKHR ChooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats);
    static VkPresentModeKHR ChooseBestPresentationMode(const std::vector<VkPresentModeKHR> presentationModes);
    static VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& surfaceCapabilities);
    static VkImageView CreateImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
    static SwapChainDetails GetSwapChainDetails(VkPhysicalDevice device, VkSurfaceKHR surface);
private:

};


#endif //VULKAN2_SWAPCHAIN_H
