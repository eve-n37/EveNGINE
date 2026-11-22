# Builds the game runtime for browsers. Configure with Emscripten:
#   emcmake cmake -S web -B build-web -DCMAKE_BUILD_TYPE=Release
#   cmake --build build-web
# The result, evengine.js and evengine.wasm, goes in Studio's web folder beside
# editor/web/index.html; Studio's web export copies all three next to the game's project.
cmake_minimum_required(VERSION 3.20)
project(EveNGINEWeb LANGUAGES C CXX)
if(NOT EMSCRIPTEN)
  message(FATAL_ERROR "Configure this folder with emcmake (Emscripten).")
endif()
set(LOTA_DESKTOP ON CACHE BOOL "" FORCE)
add_subdirectory(../engine engine)
add_executable(evengine-web ../editor/runtime/main.cpp)
target_link_libraries(evengine-web PRIVATE LotA::Studio LotA::Platform)
set_target_properties(evengine-web PROPERTIES OUTPUT_NAME evengine SUFFIX ".js")
target_link_options(evengine-web PRIVATE
  # The page (editor/web/index.html) loads the project, mounts browser storage for saves and
  # calls main() when the player clicks Play.
  -sINVOKE_RUN=0
  -sEXIT_RUNTIME=0
  -sALLOW_MEMORY_GROWTH=1
  -sFORCE_FILESYSTEM=1
  -lidbfs.js
  "-sEXPORTED_RUNTIME_METHODS=['FS','callMain','addRunDependency','removeRunDependency']"
  -sENVIRONMENT=web)
