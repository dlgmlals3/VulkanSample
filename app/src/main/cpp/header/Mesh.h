#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_android.h>

#include <vector>
#include "VkUtil.h"

struct Model {
    glm::mat4 model;
};

class Mesh
{
public:
    Mesh();
    Mesh(VkPhysicalDevice newPhysicalDevice, VkDevice newDevice,
         VkQueue transferQueue, VkCommandPool transferCommandPool,
         std::vector<Vertex> * vertices, std::vector<uint32_t> * indices,
         int newTexId);

    void setModel(glm::mat4 newModel);
    Model getModel();

    int getTexId();

    int getVertexCount();
    VkBuffer getVertexBuffer();

    int getIndexCount();
    VkBuffer getIndexBuffer();

    void destroyBuffers();

    ~Mesh();

private:
    Model mModel;
    int texId;

    int vertexCount;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;

    int indexCount;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;

    VkPhysicalDevice physicalDevice;
    VkDevice device;

    void createVertexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<Vertex> * vertices);
    void createIndexBuffer(VkQueue transferQueue, VkCommandPool transferCommandPool, std::vector<uint32_t> * indices);
};