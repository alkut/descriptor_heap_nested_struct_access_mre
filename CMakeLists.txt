cmake_minimum_required(VERSION 3.31.6)
project(descriptor_heap_nested_struct_access_mre LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

add_executable(descriptor_heap_nested_struct_access_mre 
	main.cpp
)

find_package(Vulkan REQUIRED COMPONENTS glslc)

find_program(SPIRV_DIS_EXECUTABLE 
    NAMES spirv-dis spirv-dis.exe
    PATHS "$ENV{VULKAN_SDK}/Bin"
    NO_DEFAULT_PATH
)

message(STATUS "Using spirv-disasm: ${SPIRV_DIS_EXECUTABLE}")
message(STATUS "Using glslc compiler: ${Vulkan_GLSLC_EXECUTABLE}")

target_link_libraries(descriptor_heap_nested_struct_access_mre PRIVATE Vulkan::Headers)

add_custom_command(
        OUTPUT 
            "${CMAKE_SOURCE_DIR}/shaders/compiled/test_working.comp.spv" 
            "${CMAKE_SOURCE_DIR}/shaders/compiled/test_failing.comp.spv"
            "${CMAKE_SOURCE_DIR}/shaders/compiled/test_working.comp.spv.disasm" 
            "${CMAKE_SOURCE_DIR}/shaders/compiled/test_failing.comp.spv.disasm"

        COMMAND 
            "${Vulkan_GLSLC_EXECUTABLE}"
                --target-env=vulkan1.1 
                "${CMAKE_SOURCE_DIR}/shaders/test_working.comp" 
                -o "${CMAKE_SOURCE_DIR}/shaders/compiled/test_working.comp.spv"

        COMMAND 
            "${Vulkan_GLSLC_EXECUTABLE}"
                --target-env=vulkan1.1 
                "${CMAKE_SOURCE_DIR}/shaders/test_failing.comp" 
                -o "${CMAKE_SOURCE_DIR}/shaders/compiled/test_failing.comp.spv"

        COMMAND
            "${SPIRV_DIS_EXECUTABLE}"
                "${CMAKE_SOURCE_DIR}/shaders/compiled/test_working.comp.spv"
                -o "${CMAKE_SOURCE_DIR}/shaders/compiled/test_working.comp.spv.disasm"

        COMMAND
            "${SPIRV_DIS_EXECUTABLE}"
                "${CMAKE_SOURCE_DIR}/shaders/compiled/test_failing.comp.spv"
                -o "${CMAKE_SOURCE_DIR}/shaders/compiled/test_failing.comp.spv.disasm"

        DEPENDS 
            "${CMAKE_SOURCE_DIR}/shaders/test_working.comp"
            "${CMAKE_SOURCE_DIR}/shaders/test_failing.comp"

        COMMENT "Compiling ${CMAKE_SOURCE_DIR}/shaders/*.comp to SPIR-V"

        VERBATIM
)

add_custom_target(CompileTestComputeShaders ALL DEPENDS 
    "${CMAKE_SOURCE_DIR}/shaders/compiled/test_working.comp.spv"
    "${CMAKE_SOURCE_DIR}/shaders/compiled/test_failing.comp.spv"
    "${CMAKE_SOURCE_DIR}/shaders/compiled/test_working.comp.spv.disasm"
    "${CMAKE_SOURCE_DIR}/shaders/compiled/test_failing.comp.spv.disasm"
)

add_dependencies(descriptor_heap_nested_struct_access_mre CompileTestComputeShaders)

target_compile_definitions(descriptor_heap_nested_struct_access_mre PRIVATE 
    TEST_WORKING_SHADER_SPIRV_FILEPATH="${CMAKE_SOURCE_DIR}/shaders/compiled/test_working.comp.spv"
    TEST_FAILING_SHADER_SPIRV_FILEPATH="${CMAKE_SOURCE_DIR}/shaders/compiled/test_failing.comp.spv"
)

target_compile_options(descriptor_heap_nested_struct_access_mre PUBLIC
        $<$<CXX_COMPILER_ID:MSVC>:/Zc:preprocessor>
)
