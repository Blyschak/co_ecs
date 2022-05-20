FetchContent_Declare(
  spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog
  GIT_TAG v1.9.2)
FetchContent_MakeAvailable(spdlog)

set(LUA_BUILD_COMPILER OFF CACHE INTERNAL "")
set(LUA_BUILD_INTERPRETER OFF CACHE INTERNAL "")

FetchContent_Declare(
  lua
  GIT_REPOSITORY https://github.com/marovira/lua
  GIT_TAG a32ae8de013ebe1feb3c0d22d1028ff5d2617f84)
FetchContent_MakeAvailable(lua)

target_compile_definitions(lua_lib PUBLIC LUA_USE_POSIX)

FetchContent_Declare(
  sol2
  GIT_REPOSITORY https://github.com/ThePhD/sol2
  GIT_TAG v3.2.3)
FetchContent_MakeAvailable(sol2)

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

if(DEFINED VULKAN_SDK_PATH)
  set(Vulkan_INCLUDE_DIRS "${VULKAN_SDK_PATH}/Include")
  set(Vulkan_LIBRARIES "${VULKAN_SDK_PATH}/Lib")
  set(Vulkan_FOUND "True")
else()
  find_package(Vulkan REQUIRED)
  message(STATUS "Found Vulkan: $ENV{VULKAN_SDK}")
endif()
if(NOT Vulkan_FOUND)
  message(FATAL_ERROR "Could not find Vulkan library!")
else()
  message(STATUS "Using vulkan lib at: ${Vulkan_LIBRARIES}")
endif()

FetchContent_Declare(
  glad
  GIT_REPOSITORY https://github.com/Dav1dde/glad 
  GIT_TAG        v0.1.36
)
FetchContent_MakeAvailable(glad)

add_library(
  engine
  core/logging.cpp
  core/application.cpp
  scripting/lua_engine.cpp
  platform/window.cpp
  platform/glfw/glfw_window.cpp
  platform/glfw/glfw_monitor.cpp
  render/renderer.cpp
  render/vulkan/vk_device.cpp
  render/vulkan/vk_renderer.cpp
  render/opengl/gl_buffer.cpp
  render/opengl/gl_vao.cpp
  render/opengl/gl_renderer.cpp
)

target_link_libraries(
  engine
  PUBLIC spdlog::spdlog
  PUBLIC lua::lua
  PUBLIC sol2
  PUBLIC glm
  PUBLIC glfw
  PUBLIC glad
  PUBLIC vulkan)
