cmake_minimum_required(VERSION 3.26)
project(QuickPocket VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_definitions(NOMINMAX WIN32_LEAN_AND_MEAN)

find_package(CommonLibSSE CONFIG QUIET)
if(NOT CommonLibSSE_FOUND)
    set(COMMONLIB_LOCAL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/extern/CommonLibSSE-NG")
    if(EXISTS "${COMMONLIB_LOCAL_PATH}/CMakeLists.txt")
        add_subdirectory("${COMMONLIB_LOCAL_PATH}" "${CMAKE_BINARY_DIR}/_deps/CommonLibSSE-NG")
    else()
        message(FATAL_ERROR "CommonLibSSE not found. Install CommonLibSSE or clone with --recursive to init submodules.")
    endif()
endif()

add_library(
    QuickPocket
    SHARED
    src/Config.cpp
    src/ChanceService.cpp
    src/QuickLootBridge.cpp
    src/HoverController.cpp
    src/Papyrus.cpp
    src/PickpocketRules.cpp
    src/PlantModeController.cpp
    src/EventHandlers.cpp
    src/main.cpp
)

target_include_directories(
    QuickPocket
    PRIVATE
    include
)

target_link_libraries(
    QuickPocket
    PRIVATE
    CommonLibSSE::CommonLibSSE
)

target_compile_definitions(QuickPocket PRIVATE _UNICODE UNICODE)
