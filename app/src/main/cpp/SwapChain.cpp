#include "header/SwapChain.h"

using namespace std;

SwapChainDetails SwapChain::GetSwapChainDetails(VkPhysicalDevice device, VkSurfaceKHR surface)
{
    SwapChainDetails swapChainDetails;
    // -- FORMATS --
    uint32_t formatCount = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    if (formatCount != 0)
    {
        swapChainDetails.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, swapChainDetails.formats.data());
    }
    // vkGetPhysicalDeviceSurfaceFormatsKHR 함수 자체에서 에러 발생
    // 불편하면 함수 대신 그냥 넣어줘라
    /*
        swapChainDetails.formats.push_back(VkSurfaceFormatKHR
                                               { VK_FORMAT_R8G8B8A8_UNORM, VK_COLORSPACE_SRGB_NONLINEAR_KHR});
    */

    // -- CAPABILITIES --
    // Get the surface capabilities for the given surface on the given physical device
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &swapChainDetails.surfaceCapabilities);


    // -- PRESENTATION MODES --
    uint32_t presentationCount = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationCount, nullptr);

    // If presentation modes returned, get list of presentation modes
    if (presentationCount != 0)
    {
        swapChainDetails.presentationModes.resize(presentationCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentationCount, swapChainDetails.presentationModes.data());
    }

    return swapChainDetails;
}

VkSurfaceFormatKHR SwapChain::ChooseBestSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& formats)
{
    // If only 1 format available and is undefined, then this means ALL formats are available (no restrictions)
    if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
    {
        return { VK_FORMAT_R8G8B8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
    }

    // If restricted, search for optimal format
    for (const auto& format : formats)
    {
        if ((format.format == VK_FORMAT_R8G8B8A8_UNORM || format.format == VK_FORMAT_B8G8R8A8_UNORM)
            && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return format;
        }
    }

    // If can't find optimal format, then just return first format
    return formats[0];
}

VkPresentModeKHR SwapChain::ChooseBestPresentationMode(const std::vector<VkPresentModeKHR> presentationModes)
{
    // Look for Mailbox presentation mode
    for (const auto& presentationMode : presentationModes)
    {
        if (presentationMode == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            return presentationMode;
        }
    }

    // If can't find, use FIFO as Vulkan spec says it must be present
    return VK_PRESENT_MODE_FIFO_KHR;
}

VkImageView SwapChain::CreateImageView(VkDevice device, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
    VkImageViewCreateInfo viewCreateInfo = {};
    viewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewCreateInfo.image = image;											// Image to create view for
    viewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;						// Type of image (1D, 2D, 3D, Cube, etc)
    viewCreateInfo.format = format;											// Format of image data
    viewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;			// Allows remapping of rgba components to other rgba values
    viewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    // Subresources allow the view to view only a part of an image
    viewCreateInfo.subresourceRange.aspectMask = aspectFlags;				// Which aspect of image to view (e.g. COLOR_BIT for viewing colour)
    viewCreateInfo.subresourceRange.baseMipLevel = 0;						// Start mipmap level to view from
    viewCreateInfo.subresourceRange.levelCount = 1;							// Number of mipmap levels to view
    viewCreateInfo.subresourceRange.baseArrayLayer = 0;						// Start array level to view from
    viewCreateInfo.subresourceRange.layerCount = 1;							// Number of array levels to view

    // Create image view and return it
    VkImageView imageView;
    VkResult result = vkCreateImageView(device, &viewCreateInfo, nullptr, &imageView);
    if (result != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create an Image View!");
    }

    return imageView;
}
