cmake_minimum_required(VERSION 3.20)
project(MyEveNGINEGame LANGUAGES CXX)
find_package(EveNGINE 0.6 CONFIG REQUIRED)
add_executable(my-game main.cpp)
target_link_libraries(my-game PRIVATE EveNGINE::Core EveNGINE::Platform)
target_compile_features(my-game PRIVATE cxx_std_17)
add_custom_command(TARGET my-game POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_CURRENT_SOURCE_DIR}/content" "$<TARGET_FILE_DIR:my-game>/starter/content")
