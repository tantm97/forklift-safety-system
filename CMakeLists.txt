cmake_minimum_required(VERSION 3.18)
project(forklift_safety_system VERSION 0.1.0 LANGUAGES CXX)

# ─── Standards & defaults ─────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

# ─── Options ──────────────────────────────────────────────────────────────────
option(FSS_WITH_ONNXRUNTIME  "Build with ONNX Runtime inference backend" ON)
option(FSS_WITH_TENSORRT     "Build with TensorRT inference backend"     OFF)
option(FSS_BUILD_TESTS       "Build unit tests"                          ON)
option(FSS_WARN_AS_ERROR     "Treat warnings as errors"                  OFF)
option(FSS_WITH_BOOST_BEAST  "Build real WebSocket server (Boost.Beast)" ON)

# ─── Compiler flags ───────────────────────────────────────────────────────────
if(MSVC)
    add_compile_options(/W4 /permissive-)
    if(FSS_WARN_AS_ERROR)
        add_compile_options(/WX)
    endif()
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wnon-virtual-dtor)
    if(FSS_WARN_AS_ERROR)
        add_compile_options(-Werror)
    endif()
endif()

# ─── Dependencies ─────────────────────────────────────────────────────────────
find_package(Threads REQUIRED)
find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs videoio dnn)

# yaml-cpp (required for config loader)
find_package(yaml-cpp QUIET)

# Boost.Beast WebSocket server (optional; falls back to log-only stub)
if(FSS_WITH_BOOST_BEAST)
    find_package(Boost 1.74 CONFIG QUIET COMPONENTS system)
    if(Boost_FOUND)
        message(STATUS "Boost ${Boost_VERSION} found — enabling real WebSocket server")
    else()
        message(WARNING "Boost not found — falling back to log-only WebSocket stub")
        set(FSS_WITH_BOOST_BEAST OFF)
    endif()
endif()

# ONNX Runtime (optional; controlled by FSS_WITH_ONNXRUNTIME)
if(FSS_WITH_ONNXRUNTIME)
    find_path(ONNXRUNTIME_INCLUDE_DIR onnxruntime_cxx_api.h
        PATH_SUFFIXES onnxruntime onnxruntime/core/session)
    find_library(ONNXRUNTIME_LIB onnxruntime)
    if(NOT ONNXRUNTIME_INCLUDE_DIR OR NOT ONNXRUNTIME_LIB)
        message(WARNING "ONNX Runtime not found — building without ORT backend")
        set(FSS_WITH_ONNXRUNTIME OFF)
    endif()
endif()

# TensorRT (optional; user supplies TENSORRT_ROOT)
if(FSS_WITH_TENSORRT)
    find_path(TENSORRT_INCLUDE_DIR NvInfer.h HINTS ${TENSORRT_ROOT}/include)
    find_library(TENSORRT_LIB nvinfer HINTS ${TENSORRT_ROOT}/lib)
    if(NOT TENSORRT_INCLUDE_DIR OR NOT TENSORRT_LIB)
        message(WARNING "TensorRT not found — building without TRT backend")
        set(FSS_WITH_TENSORRT OFF)
    endif()
endif()

# ─── Core library ─────────────────────────────────────────────────────────────
add_library(forklift_core STATIC
    src/domain/BoundingBox.cpp
    src/domain/SafetyZone.cpp
    src/domain/Alert.cpp
    src/application/SafetyZoneService.cpp
    src/application/RiskDetectionService.cpp
    src/application/FrameProcessingPipeline.cpp
    src/infrastructure/video/RtspCameraSource.cpp
    src/infrastructure/video/VideoFileSource.cpp
    src/infrastructure/ai/YoloV8Postprocessor.cpp
    src/infrastructure/transport/WebSocketAlertPublisher.cpp
    src/infrastructure/config/YamlConfigLoader.cpp
    src/infrastructure/logging/Logger.cpp
    src/shared/ThreadPool.cpp
)

target_include_directories(forklift_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(forklift_core
    PUBLIC
        Threads::Threads
        ${OpenCV_LIBS}
)

if(yaml-cpp_FOUND)
    target_link_libraries(forklift_core PUBLIC yaml-cpp)
    target_compile_definitions(forklift_core PUBLIC FSS_HAS_YAML_CPP=1)
endif()

if(FSS_WITH_BOOST_BEAST)
    target_include_directories(forklift_core PUBLIC ${Boost_INCLUDE_DIRS})
    target_link_libraries(forklift_core PUBLIC Boost::headers)
    target_compile_definitions(forklift_core PUBLIC FSS_WITH_BOOST_BEAST=1)
endif()

if(FSS_WITH_ONNXRUNTIME)
    target_sources(forklift_core PRIVATE src/infrastructure/ai/OnnxInferenceEngine.cpp)
    target_include_directories(forklift_core PUBLIC ${ONNXRUNTIME_INCLUDE_DIR})
    target_link_libraries(forklift_core PUBLIC ${ONNXRUNTIME_LIB})
    target_compile_definitions(forklift_core PUBLIC FSS_WITH_ONNXRUNTIME=1)
endif()

if(FSS_WITH_TENSORRT)
    target_sources(forklift_core PRIVATE src/infrastructure/ai/TensorRTInferenceEngine.cpp)
    target_include_directories(forklift_core PUBLIC ${TENSORRT_INCLUDE_DIR})
    target_link_libraries(forklift_core PUBLIC ${TENSORRT_LIB})
    target_compile_definitions(forklift_core PUBLIC FSS_WITH_TENSORRT=1)
endif()

# ─── Executable ───────────────────────────────────────────────────────────────
add_executable(forklift_safety src/interface/main.cpp)
target_link_libraries(forklift_safety PRIVATE forklift_core)

# ─── Tests ────────────────────────────────────────────────────────────────────
if(FSS_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ─── Install ──────────────────────────────────────────────────────────────────
install(TARGETS forklift_safety RUNTIME DESTINATION bin)
install(DIRECTORY conf/ DESTINATION etc/forklift)
