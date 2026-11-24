# Simulador do Visor de Vida MTG no PC (Linux).
# Uso rápido (veja o README.md):
#   git clone --depth 1 https://github.com/lovyan03/LovyanGFX.git
#   cmake -B build -DSKETCH=../etapas/etapa03_ola/etapa03_ola.ino
#   cmake --build build -j
#   ./build/simulador
cmake_minimum_required(VERSION 3.10)
project(simulador_visor CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Onde está a biblioteca LovyanGFX (por padrão, clonada dentro desta pasta)
set(LOVYANGFX_DIR "${CMAKE_CURRENT_SOURCE_DIR}/LovyanGFX" CACHE PATH "Pasta da LovyanGFX")
# Qual sketch rodar
set(SKETCH "../etapas/etapa03_ola/etapa03_ola.ino" CACHE FILEPATH "Arquivo .ino a simular")

get_filename_component(SKETCH_ABS "${SKETCH}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
if(NOT EXISTS "${SKETCH_ABS}")
  message(FATAL_ERROR "Sketch não encontrado: ${SKETCH_ABS}")
endif()
if(NOT EXISTS "${LOVYANGFX_DIR}/src/LovyanGFX.hpp")
  message(FATAL_ERROR "LovyanGFX não encontrada em ${LOVYANGFX_DIR}. Rode: git clone --depth 1 https://github.com/lovyan03/LovyanGFX.git")
endif()
message(STATUS "Simulando: ${SKETCH_ABS}")

file(GLOB LGFX_SOURCES CONFIGURE_DEPENDS
  ${LOVYANGFX_DIR}/src/lgfx/Fonts/efont/*.c
  ${LOVYANGFX_DIR}/src/lgfx/Fonts/IPA/*.c
  ${LOVYANGFX_DIR}/src/lgfx/Fonts/lvgl/*.c
  ${LOVYANGFX_DIR}/src/lgfx/utility/*.c
  ${LOVYANGFX_DIR}/src/lgfx/v1/*.cpp
  ${LOVYANGFX_DIR}/src/lgfx/v1/panel/*.cpp
  ${LOVYANGFX_DIR}/src/lgfx/v1/touch/*.cpp
  ${LOVYANGFX_DIR}/src/lgfx/v1/lv_font/*.c
  ${LOVYANGFX_DIR}/src/lgfx/v1/platforms/sdl/*.cpp
)

find_package(SDL2 REQUIRED)

# outros .cpp da pasta do sketch (ex.: game.cpp, ui.cpp) também entram, como na IDE do Arduino
get_filename_component(SKETCH_DIR "${SKETCH_ABS}" DIRECTORY)
file(GLOB SKETCH_CPPS CONFIGURE_DEPENDS "${SKETCH_DIR}/*.cpp")

add_executable(simulador main.cpp ${SKETCH_CPPS} ${LGFX_SOURCES})
target_include_directories(simulador PRIVATE ${LOVYANGFX_DIR}/src ${SDL2_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR} ${SKETCH_DIR})
target_compile_definitions(simulador PRIVATE LGFX_SDL LGFX_USE_V1 "SKETCH_FILE=\"${SKETCH_ABS}\"")
target_link_libraries(simulador PRIVATE ${SDL2_LIBRARIES} pthread)
# o .ino é incluído no main.cpp: recompila quando ele mudar
set_source_files_properties(main.cpp PROPERTIES OBJECT_DEPENDS "${SKETCH_ABS}")
