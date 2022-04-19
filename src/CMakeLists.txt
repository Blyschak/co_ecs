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

add_library(engine core/engine.cpp)

target_link_libraries(
  engine
  PUBLIC spdlog::spdlog
  PUBLIC glm)
