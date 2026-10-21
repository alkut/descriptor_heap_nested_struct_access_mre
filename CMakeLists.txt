cmake_minimum_required(VERSION 3.31.6)
project(descriptor_heap_nested_struct_access_mre LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

include(FetchContent)

set(ZSTD_BUILD_PROGRAMS OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_SHARED OFF CACHE BOOL "" FORCE)
set(ZSTD_BUILD_STATIC ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    zstd
    GIT_REPOSITORY https://github.com/facebook/zstd.git
    GIT_TAG        v1.5.6
    SOURCE_SUBDIR  build/cmake
)
FetchContent_MakeAvailable(zstd)

add_executable(descriptor_heap_layout
	descriptor_heap_layout.cpp
)

add_executable(shader_mapping
	shader_mapping.cpp
)

find_package(Vulkan REQUIRED COMPONENTS glslc)

find_program(SPIRV_DIS_EXECUTABLE 
    NAMES spirv-dis spirv-dis.exe
    PATHS "$ENV{VULKAN_SDK}/Bin"
    NO_DEFAULT_PATH
)

message(STATUS "Using spirv-disasm: ${SPIRV_DIS_EXECUTABLE}")
message(STATUS "Using glslc compiler: ${Vulkan_GLSLC_EXECUTABLE}")

target_link_libraries(descriptor_heap_layout PRIVATE Vulkan::Headers libzstd_static)
target_link_libraries(shader_mapping PRIVATE Vulkan::Headers libzstd_static)

set(NVDISASM_EXECUTABLE "" CACHE FILEPATH "Path to nvdisasm executable")
set(CUDA_ARCH "SM86" CACHE STRING "Target SM architecture for nvdisasm (e.g. SM86, SM89, SM120)")

if(NOT NVDISASM_EXECUTABLE)
    find_package(CUDAToolkit QUIET)
    if(CUDAToolkit_FOUND)
        find_program(NVDISASM_EXECUTABLE
            NAMES nvdisasm nvdisasm.exe
            HINTS ${CUDAToolkit_BIN_DIR}
        )
    else()
        find_program(NVDISASM_EXECUTABLE
            NAMES nvdisasm nvdisasm.exe
            HINTS 
                "$ENV{CUDA_PATH}/bin"
                "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v13.2/bin"
                "C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA"
            PATH_SUFFIXES bin
        )
    endif()
endif()

if(NVDISASM_EXECUTABLE AND EXISTS "${NVDISASM_EXECUTABLE}")
    message(STATUS "Found nvdisasm: ${NVDISASM_EXECUTABLE}")

    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/disasm_runner.cmake" [=[
        execute_process(
            COMMAND "${NVDISASM}" -b "${ARCH}" "${INPUT}"
            OUTPUT_FILE "${OUTPUT}"
            RESULT_VARIABLE RES
        )
        if(NOT RES EQUAL 0)
            message(FATAL_ERROR "nvdisasm failed with exit code ${RES}")
        endif()
    ]=])

    add_custom_target(disasm_cubin
        COMMAND "${CMAKE_COMMAND}"
            -D "NVDISASM=${NVDISASM_EXECUTABLE}"
            -D "ARCH=${CUDA_ARCH}"
            -D "INPUT=${CMAKE_SOURCE_DIR}/shaders/shader_mapping/working.cubin"
            -D "OUTPUT=${CMAKE_SOURCE_DIR}/shaders/shader_mapping/working.sass"
            -P "${CMAKE_CURRENT_BINARY_DIR}/disasm_runner.cmake"
        COMMAND "${CMAKE_COMMAND}"
            -D "NVDISASM=${NVDISASM_EXECUTABLE}"
            -D "ARCH=${CUDA_ARCH}"
            -D "INPUT=${CMAKE_SOURCE_DIR}/shaders/shader_mapping/failing.cubin"
            -D "OUTPUT=${CMAKE_SOURCE_DIR}/shaders/shader_mapping/failing.sass"
            -P "${CMAKE_CURRENT_BINARY_DIR}/disasm_runner.cmake"
        COMMAND "${CMAKE_COMMAND}"
            -D "NVDISASM=${NVDISASM_EXECUTABLE}"
            -D "ARCH=${CUDA_ARCH}"
            -D "INPUT=${CMAKE_SOURCE_DIR}/shaders/descriptor_heap_layout/working.cubin"
            -D "OUTPUT=${CMAKE_SOURCE_DIR}/shaders/descriptor_heap_layout/working.sass"
            -P "${CMAKE_CURRENT_BINARY_DIR}/disasm_runner.cmake"
        COMMAND "${CMAKE_COMMAND}"
            -D "NVDISASM=${NVDISASM_EXECUTABLE}"
            -D "ARCH=${CUDA_ARCH}"
            -D "INPUT=${CMAKE_SOURCE_DIR}/shaders/descriptor_heap_layout/failing.cubin"
            -D "OUTPUT=${CMAKE_SOURCE_DIR}/shaders/descriptor_heap_layout/failing.sass"
            -P "${CMAKE_CURRENT_BINARY_DIR}/disasm_runner.cmake"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        COMMENT "Disassembling shaders/*.cubin -> shaders/*.sass with nvdisasm (-b ${CUDA_ARCH})"
        VERBATIM
    )
else()
    message(STATUS "nvdisasm not found. Target disasm_cubin will not be created (set NVDISASM_EXECUTABLE to enable).")
endif()

add_custom_command(
        OUTPUT 
            "${CMAKE_SOURCE_DIR}/shaders/shader_mapping/working.comp.spv" 
            "${CMAKE_SOURCE_DIR}/shaders/shader_mapping/failing.comp.spv"
            "${CMAKE_SOURCE_DIR}/shaders/shader_mapping/working.comp.spv.disasm" 
            "${CMAKE_SOURCE_DIR}/shaders/shader_mapping/failing.comp.spv.disasm"
            "${CMAKE_SOURCE_DIR}/shaders/descriptor_heap_layout/working.comp.spv" 
            "${CMAKE_SOURCE_DIR}/shaders/descriptor_heap_layout/failing.comp.spv"
            "${CMAKE_SOURCE_DIR}/shaders/descriptor_heap_layout/working.comp.spv.disasm" 
            "${CMAKE_SOURCE_DIR}/shaders/descriptor_heap_layout/failing.comp.spv.disasm"

        COMMAND 
            "${Vulkan_GLSLC_EXECUTABLE}"
                --target-env=vulkan1.1 
                "${CMAKE_SOURCE_DIR}/shaders/shader_mapping/working.comp" 
                -o "${CMAKE_SOURCE_DIR}/shaders/shader_mapping/working.comp.spv"

        COMMAND 
            "${Vulkan_GLSLC_EXECUTABLE}"
                --target-env=vulkan1.1 
                "${CMAKE_SOURCE_DIR}/shaders/shader_mapping/failing.comp" 
                -o "${CMAKE_SOURCE_DIR}/shaders/shader_mapping/failing.comp.spv"

        COMMAND 
            "${Vulkan_GLSLC_EXECUTABLE}"
                --target-env=vulkan1.1 
                "${CMAKE_SOURCE_DIR}/shaders/descriptor_heap_layout/working.comp" 
                -o "${CMAKE_SOURCE_DIR}/shaders/descriptor_heap_layout/working.comp.spv"

        COMMAND 
            "${Vulkan_GLSLC_EXECUTABLE}"
                --target-env=vulkan1.1 
                "${CMAKE_SOURCE_DIR}/shaders/descriptor_heap_layout/failing.comp" 
                -o "${CMAKE_SOURCE_DIR}/shaders/descriptor_heap_layout/failing.comp.spv"

        COMMAND
            "${SPIRV_DIS_EXECUTABLE}"
                "${CMAKE_SOURCE_DIR}/shaders/shader_mapping/working.comp.spv"
                -o "${CMAKE_SOURCE_DIR}/shaders/shader_mapping/working.comp.spv.disasm"

        COMMAND
            "${SPIRV_DIS_EXECUTABLE}"
                "${CMAKE_SOURCE_DIR}/shaders/shader_mapping/failing.comp.spv"
                -o "${CMAKE_SOURCE_DIR}/shaders/shader_mapping/failing.comp.spv.disasm"

        COMMAND
            "${SPIRV_DIS_EXECUTABLE}"
                "${CMAKE_SOURCE_DIR}/shaders/descriptor_heap_layout/working.comp.spv"
                -o "${CMAKE_SOURCE_DIR}/shaders/descriptor_heap_layout/working.comp.spv.disasm"

        COMMAND
            "${SPIRV_DIS_EXECUTABLE}"
                "${CMAKE_SOURCE_DIR}/shaders/descriptor_heap_layout/failing.comp.spv"
                -o "${CMAKE_SOURCE_DIR}/shaders/descriptor_heap_layout/failing.comp.spv.disasm"

        DEPENDS 
            "${CMAKE_SOURCE_DIR}/shaders/shader_mapping/working.comp"
            "${CMAKE_SOURCE_DIR}/shaders/shader_mapping/failing.comp"
            "${CMAKE_SOURCE_DIR}/shaders/descriptor_heap_layout/working.comp"
            "${CMAKE_SOURCE_DIR}/shaders/descriptor_heap_layout/failing.comp"

        COMMENT "Compiling ${CMAKE_SOURCE_DIR}/shaders/*.comp to SPIR-V"

        VERBATIM
)

add_custom_target(CompileTestComputeShaders ALL DEPENDS 
    "${CMAKE_SOURCE_DIR}/shaders/shader_mapping/working.comp.spv"
    "${CMAKE_SOURCE_DIR}/shaders/shader_mapping/failing.comp.spv"
    "${CMAKE_SOURCE_DIR}/shaders/shader_mapping/working.comp.spv.disasm"
    "${CMAKE_SOURCE_DIR}/shaders/shader_mapping/failing.comp.spv.disasm"
    "${CMAKE_SOURCE_DIR}/shaders/descriptor_heap_layout/working.comp.spv"
    "${CMAKE_SOURCE_DIR}/shaders/descriptor_heap_layout/failing.comp.spv"
    "${CMAKE_SOURCE_DIR}/shaders/descriptor_heap_layout/working.comp.spv.disasm"
    "${CMAKE_SOURCE_DIR}/shaders/descriptor_heap_layout/failing.comp.spv.disasm"
)

add_dependencies(descriptor_heap_layout CompileTestComputeShaders)
add_dependencies(shader_mapping CompileTestComputeShaders)

target_compile_definitions(descriptor_heap_layout PRIVATE 
    PROJECT_ROOT="${CMAKE_SOURCE_DIR}"
    WORKING_SHADER_SPIRV_FILEPATH="${CMAKE_SOURCE_DIR}/shaders/descriptor_heap_layout/working.comp.spv"
    FAILING_SHADER_SPIRV_FILEPATH="${CMAKE_SOURCE_DIR}/shaders/descriptor_heap_layout/failing.comp.spv"
)
target_compile_definitions(shader_mapping PRIVATE 
    PROJECT_ROOT="${CMAKE_SOURCE_DIR}"
    WORKING_SHADER_SPIRV_FILEPATH="${CMAKE_SOURCE_DIR}/shaders/shader_mapping/working.comp.spv"
    FAILING_SHADER_SPIRV_FILEPATH="${CMAKE_SOURCE_DIR}/shaders/shader_mapping/failing.comp.spv"
)

target_compile_options(descriptor_heap_layout PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/Zc:preprocessor>
)
target_compile_options(shader_mapping PRIVATE
        $<$<CXX_COMPILER_ID:MSVC>:/Zc:preprocessor>
)

file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/clean_binaries.cmake" [=[
    file(GLOB_RECURSE FILES_TO_REMOVE
        "${DIR}/*.spv"
        "${DIR}/*.spv.disasm"
        "${DIR}/*.cubin"
        "${DIR}/*.sass"
    )
    if(FILES_TO_REMOVE)
        file(REMOVE ${FILES_TO_REMOVE})
        foreach(F ${FILES_TO_REMOVE})
            message(STATUS "Removed: ${F}")
        endforeach()
    else()
        message(STATUS "No binaries to clean.")
    endif()
]=])

add_custom_target(clean_binaries
    COMMAND "${CMAKE_COMMAND}"
        -D "DIR=${CMAKE_SOURCE_DIR}/shaders"
        -P "${CMAKE_CURRENT_BINARY_DIR}/clean_binaries.cmake"
    COMMENT "Removing all generated spv, spv.disasm, cubin, sass files"
    VERBATIM
)
