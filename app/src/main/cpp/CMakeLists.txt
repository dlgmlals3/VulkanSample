# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

cmake_minimum_required(VERSION 3.22.1)

project("vulkansample")
####################################################################################################
# shaderc 정의
####################################################################################################
set(CMAKE_SHADERRC ${CMAKE_ANDROID_NDK}/sources/third_party/shaderc)

add_library(shaderc INTERFACE)

target_include_directories(shaderc INTERFACE
        ${CMAKE_SHADERRC}/include)

target_link_libraries(shaderc INTERFACE
        ${CMAKE_SHADERRC}/libs/system/${CMAKE_ANDROID_ARCH_ABI}/libshaderc.a
)

####################################################################################################
# stb 정의
####################################################################################################
include(FetchContent)

FetchContent_Declare(stb
        GIT_REPOSITORY https://github.com/nothings/stb
        GIT_TAG master
)
FetchContent_MakeAvailable(stb)
add_library(stb INTERFACE)
target_include_directories(stb INTERFACE
        ${CMAKE_BINARY_DIR}/_deps/stb-src)
target_compile_definitions(stb INTERFACE
        STB_IMAGE_IMPLEMENTATION)

add_subdirectory(glm) # if your CMakeLists is at '../cpp'

#####################################################################################
# vulkanSample
#####################################################################################
find_package(game-activity REQUIRED CONFIG)
find_package(Vulkan REQUIRED)

add_library(vulkansample SHARED
        main.cpp
        AndroidOut.cpp
        SwapChain.cpp
        VkTexture.cpp
        Mesh.cpp
        VkRenderer.cpp
        header/VkRenderer.h
        header/SwapChain.h
        header/VkUtil.h
        header/VkTexture.h
        header/Mesh.h
)

target_compile_definitions(vulkansample PRIVATE VK_USE_PLATFORM_ANDROID_KHR)

# Configure libraries CMake uses to link your target library.
target_link_libraries(vulkansample
        game-activity::game-activity
        android
        log
        Vulkan::Vulkan
        shaderc
        stb
        glm
)