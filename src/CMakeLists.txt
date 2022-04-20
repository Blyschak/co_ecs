FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog
  GIT_TAG v1.9.2)
FetchContent_MakeAvailable(spdlog)

FetchContent_Declare(
  glm
  GIT_REPOSITORY https://github.com/g-truc/glm
  GIT_TAG 6ad79aae3eb5bf809c30bf1168171e9e55857e45)
FetchContent_MakeAvailable(glm)

set(GLFW_INSTALL OFF)
set(GLFW_BUILD_DOCS OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_EXAMPLES OFF)

FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw
  GIT_TAG 3.3.6)
FetchContent_MakeAvailable(glfw)

add_library(engine core/application.cpp platform/window.cpp
                   platform/glfw/glfw_window.cpp render/vulkan/vk_device.cpp)

target_link_libraries(
  engine
  PUBLIC spdlog::spdlog
  PUBLIC glm
  PUBLIC glfw)
