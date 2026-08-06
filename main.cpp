#include "vulkan/vulkan_core.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <ranges>
#include <vector>
#include <span>

#ifndef _WIN32
	#error Only Windows Supported
#endif// #ifndef _WIN32

#ifndef TEST_FAILING_SHADER_SPIRV_FILEPATH
	#error Add compiler definition
#endif// #ifndef TEST_FAILING_SHADER_SPIRV_FILEPATH

#ifndef TEST_WORKING_SHADER_SPIRV_FILEPATH
	#error Add compiler definition
#endif// #ifndef TEST_WORKING_SHADER_SPIRV_FILEPATH

#define VALIDATION_LAYERS_ENABLED 1
#define EXIT_CODE_ON_ANY_VALIDATION_LAYER_ERROR 42
#define TEST_BUFFER_SIZE_ELEMENTS (1 << 16)

#define ASSERT(CONDITION, EXIT_CODE, FORMATTED_MESSAGE, ...)                                \
    do {                                                                                    \
		if (!(CONDITION)) {                                                                 \
			std::printf("Assertion failed!\nFile/Line: %s:%i\n", __FILE__, __LINE__);       \
			std::printf(FORMATTED_MESSAGE __VA_OPT__(,) __VA_ARGS__);                       \
			std::fflush(stdout);                                                            \
			std::exit(EXIT_CODE);                                                           \
		}                                                                                   \
    }                                                                                       \
    while (false)

extern "C" {
struct HINSTANCE__;
typedef struct HINSTANCE__* HINSTANCE;
typedef HINSTANCE HMODULE;
typedef int (__stdcall *FARPROC)();

__declspec(dllimport) HMODULE __stdcall LoadLibraryA(const char* lpLibFileName);
__declspec(dllimport) FARPROC __stdcall GetProcAddress(HMODULE hModule, const char* lpProcName);
__declspec(dllimport) int __stdcall FreeLibrary(HMODULE hModule);
}

// No suballocation intentionally - for simple code and debugging
struct BufferAllocation {
    VkBuffer m_buffer = VK_NULL_HANDLE;
    VkDeviceMemory m_deviceMemory = VK_NULL_HANDLE;
    VkDeviceAddress m_deviceAddress = 0;
    void* m_mapped = nullptr;
};

struct ShaderInput {
    int value1;
    int value2;
    int value3;
    int value4;
    int value5;
    int value6;
};

struct ShaderOutput {
	int value2;
    int value3;
    int value6;
};

struct ShaderPushData {
    std::uint32_t inputBufferIdx;
    std::uint32_t outputBufferIdx;
};

VkDeviceSize AlignUp(VkDeviceSize offset, VkDeviceSize alignment);

VkBool32 ValidationLayerDebugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT /*messageSeverity*/,
    VkDebugUtilsMessageTypeFlagsEXT /*messageType*/,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* /*pUserData */
);

VkDebugUtilsMessengerCreateInfoEXT GetDebugUtilsMessengerCreateInfo();

VkDebugUtilsMessengerEXT CreateDebugUtilsMessengerEXT(
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr, 
    VkInstance instance, 
    const VkAllocationCallbacks* pAllocator
);

void DestroyDebugUtilsMessengerEXT(
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr, 
    VkInstance instance, 
    VkDebugUtilsMessengerEXT debugMessenger, 
    const VkAllocationCallbacks* pAllocator
);

VkInstance CreateInstance(
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr, 
    const VkAllocationCallbacks* pAllocator, 
    bool enableValidationLayers
);

void DestroyInstance(
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr, 
    VkInstance instance, 
    const VkAllocationCallbacks* pAllocator
);

VkPhysicalDevice GetPhysicalDevice(
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr, 
    VkInstance instance
);

VkPhysicalDeviceDescriptorHeapPropertiesEXT GetDescriptorHeapProperties(
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr, 
    VkInstance instance, 
    VkPhysicalDevice physicalDevice
);

std::uint32_t GetQueueFamilyIndex(
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr, 
    VkInstance instance, 
    VkPhysicalDevice physicalDevice
);

VkDevice CreateDevice(
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr, 
    VkInstance instance, 
    VkPhysicalDevice physicalDevice, 
    std::uint32_t queueFamilyIndex, 
    const VkAllocationCallbacks* pAllocator
);

void DestroyDevice(
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr, 
    VkInstance instance, 
    VkDevice device, 
    const VkAllocationCallbacks* pAllocator
);

VkQueue GetQueue(
    PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr, 
    VkDevice device, 
    std::uint32_t queueFamilyIndex
);

VkCommandPool CreateCommandPool(
    PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr, 
    VkDevice device, 
    std::uint32_t queueFamilyIndex, 
    const VkAllocationCallbacks* pAllocator
);

void DestroyCommandPool(
    PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr, 
    VkDevice device, 
    VkCommandPool commandPool, 
    const VkAllocationCallbacks* pAllocator
);

VkCommandBuffer CreateCommandBuffer(
    PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr, 
    VkDevice device, 
    VkCommandPool commandPool
);

void DestroyCommandBuffer(
    PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr, 
    VkDevice device, 
    VkCommandPool commandPool, 
    VkCommandBuffer commandBuffer
);

VkFence CreateFence(
    PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr, 
    VkDevice device, 
    const VkAllocationCallbacks* pAllocator
);

void DestroyFence(
    PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr, 
    VkDevice device, 
    VkFence fence, 
    const VkAllocationCallbacks* pAllocator
);

std::uint32_t GetMemoryTypeIndex(
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr, 
    VkInstance instance, 
    VkPhysicalDevice physicalDevice, 
    std::uint32_t memoryTypeBitsRequirement
);

BufferAllocation CreateBuffer(
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr, 
    VkInstance instance, 
    PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr, 
    VkDevice device, 
    VkPhysicalDevice physicalDevice, 
    const VkBufferCreateInfo& bufferCreateInfo, 
    const VkAllocationCallbacks* pAllocator
);

void DestroyBuffer(
    PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr, 
    VkDevice device, 
    const BufferAllocation& bufferAllocation, 
    const VkAllocationCallbacks* pAllocator
);

void FlushBuffer(
    PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr, 
    VkDevice device, 
    const BufferAllocation& bufferAllocation
);

void InvalidateCacheBuffer(
    PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr, 
    VkDevice device, 
    const BufferAllocation& bufferAllocation
);

BufferAllocation CreateBufferWorkingInput(
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr, 
    VkInstance instance, 
    PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr, 
    VkDevice device, 
    VkPhysicalDevice physicalDevice, 
    const VkAllocationCallbacks* pAllocator
);

BufferAllocation CreateBufferFailingInput(
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr, 
    VkInstance instance, 
    PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr, 
    VkDevice device, 
    VkPhysicalDevice physicalDevice, 
    const VkAllocationCallbacks* pAllocator
);

BufferAllocation CreateBufferWorkingOutput(
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr, 
    VkInstance instance, 
    PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr, 
    VkDevice device, 
    VkPhysicalDevice physicalDevice, 
    const VkAllocationCallbacks* pAllocator
);

BufferAllocation CreateBufferFailingOutput(
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr, 
    VkInstance instance, 
    PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr, 
    VkDevice device, 
    VkPhysicalDevice physicalDevice, 
    const VkAllocationCallbacks* pAllocator
);

BufferAllocation CreateBufferResourceHeap(
    PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr, 
    VkInstance instance, 
    PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr, 
    VkDevice device, 
    VkPhysicalDevice physicalDevice, 
    VkDeviceSize resourceHeapBufferSize, 
    const VkAllocationCallbacks* pAllocator
);

VkShaderModule CreateShaderModule(
    PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr, 
    VkDevice device, 
    const char* filepath, 
    const VkAllocationCallbacks* pAllocator
);

/*
----------------------------------------------------------------------------------------- GLSL code -----------------------------------------------------------------------------------------

#version 450 core

#extension GL_EXT_descriptor_heap : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout (local_size_x = 16, local_size_y = 1, local_size_z = 1) in;

struct Input {
    int value1;
    int value2;
    int value3;
    int value4;
    int value5;
    int value6;
};

struct Output {
    int value2;
    int value3;
    int value6;
};

layout (descriptor_heap) buffer InputBlock {
    Input inputValues[];
} InputBuffers[];

layout (descriptor_heap) buffer OutputBlock {
    Output outputValues[];
} OutputBuffers[];

layout (push_constant) uniform PushConstantBlock {
    uint inputBufferIdx;
    uint outputBufferIdx;
} PushConstant;

void main() {
    uint i = gl_GlobalInvocationID.x;
    OutputBuffers[PushConstant.outputBufferIdx].outputValues[i].value2 = InputBuffers[PushConstant.inputBufferIdx].inputValues[i].value2;
    OutputBuffers[PushConstant.outputBufferIdx].outputValues[i].value3 = InputBuffers[PushConstant.inputBufferIdx].inputValues[i].value3;
    OutputBuffers[PushConstant.outputBufferIdx].outputValues[i].value6 = InputBuffers[PushConstant.inputBufferIdx].inputValues[i].value6;
}

----------------------------------------------------------------------------------------- SPIR-V code -----------------------------------------------------------------------------------------

; SPIR-V
; Version: 1.3
; Generator: Google Shaderc over Glslang; 11
; Bound: 82
; Schema: 0
               OpCapability Shader
               OpCapability UntypedPointersKHR
               OpCapability DescriptorHeapEXT
               OpExtension "SPV_EXT_descriptor_heap"
               OpExtension "SPV_KHR_untyped_pointers"
          %1 = OpExtInstImport "GLSL.std.450"
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main" %gl_GlobalInvocationID
               OpExecutionMode %main LocalSize 16 1 1
               OpSource GLSL 450
               OpSourceExtension "GL_EXT_descriptor_heap"
               OpSourceExtension "GL_EXT_nonuniform_qualifier"
               OpSourceExtension "GL_GOOGLE_cpp_style_line_directive"
               OpSourceExtension "GL_GOOGLE_include_directive"
               OpName %main "main"
               OpName %i "i"
               OpName %gl_GlobalInvocationID "gl_GlobalInvocationID"
               OpName %resource_heap "resource_heap"
               OpName %PushConstantBlock "PushConstantBlock"
               OpMemberName %PushConstantBlock 0 "inputBufferIdx"
               OpMemberName %PushConstantBlock 1 "outputBufferIdx"
               OpName %PushConstant "PushConstant"
               OpName %Output "Output"
               OpMemberName %Output 0 "value2"
               OpMemberName %Output 1 "value3"
               OpMemberName %Output 2 "value6"
               OpName %OutputBlock "OutputBlock"
               OpMemberName %OutputBlock 0 "outputValues"
               OpName %Input "Input"
               OpMemberName %Input 0 "value1"
               OpMemberName %Input 1 "value2"
               OpMemberName %Input 2 "value3"
               OpMemberName %Input 3 "value4"
               OpMemberName %Input 4 "value5"
               OpMemberName %Input 5 "value6"
               OpName %InputBlock "InputBlock"
               OpMemberName %InputBlock 0 "inputValues"
               OpDecorate %gl_GlobalInvocationID BuiltIn GlobalInvocationId
               OpDecorate %resource_heap BuiltIn ResourceHeapEXT
               OpDecorateId %_runtimearr_18 ArrayStrideIdEXT %19
               OpDecorate %PushConstantBlock Block
               OpMemberDecorate %PushConstantBlock 0 Offset 0
               OpMemberDecorate %PushConstantBlock 1 Offset 4
               OpMemberDecorate %Output 0 Offset 0
               OpMemberDecorate %Output 1 Offset 4
               OpMemberDecorate %Output 2 Offset 8
               OpDecorate %_runtimearr_Output ArrayStride 12
               OpDecorate %OutputBlock Block
               OpMemberDecorate %OutputBlock 0 Offset 0
               OpDecorateId %_runtimearr_18_0 ArrayStrideIdEXT %19
               OpMemberDecorate %Input 0 Offset 0
               OpMemberDecorate %Input 1 Offset 4
               OpMemberDecorate %Input 2 Offset 8
               OpMemberDecorate %Input 3 Offset 12
               OpMemberDecorate %Input 4 Offset 16
               OpMemberDecorate %Input 5 Offset 20
               OpDecorate %_runtimearr_Input ArrayStride 24
               OpDecorate %InputBlock Block
               OpMemberDecorate %InputBlock 0 Offset 0
               OpDecorate %gl_WorkGroupSize BuiltIn WorkgroupSize
       %void = OpTypeVoid
          %3 = OpTypeFunction %void
       %uint = OpTypeInt 32 0
%_ptr_Function_uint = OpTypePointer Function %uint
     %v3uint = OpTypeVector %uint 3
%_ptr_Input_v3uint = OpTypePointer Input %v3uint
%gl_GlobalInvocationID = OpVariable %_ptr_Input_v3uint Input
     %uint_0 = OpConstant %uint 0
%_ptr_Input_uint = OpTypePointer Input %uint
%_ptr_UniformConstant = OpTypeUntypedPointerKHR UniformConstant
%resource_heap = OpUntypedVariableKHR %_ptr_UniformConstant UniformConstant
         %18 = OpTypeBufferEXT StorageBuffer
         %19 = OpConstantSizeOfEXT %uint %18
%_runtimearr_18 = OpTypeRuntimeArray %18
%PushConstantBlock = OpTypeStruct %uint %uint
%_ptr_PushConstant_PushConstantBlock = OpTypePointer PushConstant %PushConstantBlock
%PushConstant = OpVariable %_ptr_PushConstant_PushConstantBlock PushConstant
        %int = OpTypeInt 32 1
      %int_1 = OpConstant %int 1
%_ptr_PushConstant_uint = OpTypePointer PushConstant %uint
     %Output = OpTypeStruct %int %int %int
%_runtimearr_Output = OpTypeRuntimeArray %Output
%OutputBlock = OpTypeStruct %_runtimearr_Output
      %int_0 = OpConstant %int 0
%_runtimearr_18_0 = OpTypeRuntimeArray %18
      %Input = OpTypeStruct %int %int %int %int %int %int
%_runtimearr_Input = OpTypeRuntimeArray %Input
 %InputBlock = OpTypeStruct %_runtimearr_Input
%_ptr_StorageBuffer_InputBlock = OpTypePointer StorageBuffer %InputBlock
%_ptr_StorageBuffer_int = OpTypePointer StorageBuffer %int
%_ptr_StorageBuffer_OutputBlock = OpTypePointer StorageBuffer %OutputBlock
      %int_2 = OpConstant %int 2
      %int_5 = OpConstant %int 5
    %uint_16 = OpConstant %uint 16
     %uint_1 = OpConstant %uint 1
%gl_WorkGroupSize = OpConstantComposite %v3uint %uint_16 %uint_1 %uint_1
       %main = OpFunction %void None %3
          %5 = OpLabel
          %i = OpVariable %_ptr_Function_uint Function
         %14 = OpAccessChain %_ptr_Input_uint %gl_GlobalInvocationID %uint_0
         %15 = OpLoad %uint %14
               OpStore %i %15
         %27 = OpAccessChain %_ptr_PushConstant_uint %PushConstant %int_1
         %28 = OpLoad %uint %27
         %33 = OpLoad %uint %i
         %35 = OpAccessChain %_ptr_PushConstant_uint %PushConstant %int_0
         %36 = OpLoad %uint %35
         %40 = OpLoad %uint %i
         %41 = OpUntypedAccessChainKHR %_ptr_UniformConstant %_runtimearr_18_0 %resource_heap %36
         %43 = OpBufferPointerEXT %_ptr_StorageBuffer_InputBlock %41
         %45 = OpAccessChain %_ptr_StorageBuffer_int %43 %int_0 %40 %int_1
         %46 = OpLoad %int %45
         %47 = OpUntypedAccessChainKHR %_ptr_UniformConstant %_runtimearr_18 %resource_heap %28
         %49 = OpBufferPointerEXT %_ptr_StorageBuffer_OutputBlock %47
         %50 = OpAccessChain %_ptr_StorageBuffer_int %49 %int_0 %33 %int_0
               OpStore %50 %46
         %51 = OpAccessChain %_ptr_PushConstant_uint %PushConstant %int_1
         %52 = OpLoad %uint %51
         %53 = OpLoad %uint %i
         %54 = OpAccessChain %_ptr_PushConstant_uint %PushConstant %int_0
         %55 = OpLoad %uint %54
         %56 = OpLoad %uint %i
         %58 = OpUntypedAccessChainKHR %_ptr_UniformConstant %_runtimearr_18_0 %resource_heap %55
         %59 = OpBufferPointerEXT %_ptr_StorageBuffer_InputBlock %58
         %60 = OpAccessChain %_ptr_StorageBuffer_int %59 %int_0 %56 %int_2
         %61 = OpLoad %int %60
         %62 = OpUntypedAccessChainKHR %_ptr_UniformConstant %_runtimearr_18 %resource_heap %52
         %63 = OpBufferPointerEXT %_ptr_StorageBuffer_OutputBlock %62
         %64 = OpAccessChain %_ptr_StorageBuffer_int %63 %int_0 %53 %int_1
               OpStore %64 %61
         %65 = OpAccessChain %_ptr_PushConstant_uint %PushConstant %int_1
         %66 = OpLoad %uint %65
         %67 = OpLoad %uint %i
         %68 = OpAccessChain %_ptr_PushConstant_uint %PushConstant %int_0
         %69 = OpLoad %uint %68
         %70 = OpLoad %uint %i
         %72 = OpUntypedAccessChainKHR %_ptr_UniformConstant %_runtimearr_18_0 %resource_heap %69
         %73 = OpBufferPointerEXT %_ptr_StorageBuffer_InputBlock %72
         %74 = OpAccessChain %_ptr_StorageBuffer_int %73 %int_0 %70 %int_5
         %75 = OpLoad %int %74
         %76 = OpUntypedAccessChainKHR %_ptr_UniformConstant %_runtimearr_18 %resource_heap %66
         %77 = OpBufferPointerEXT %_ptr_StorageBuffer_OutputBlock %76
         %78 = OpAccessChain %_ptr_StorageBuffer_int %77 %int_0 %67 %int_2
               OpStore %78 %75
               OpReturn
               OpFunctionEnd

 */
VkShaderModule CreateShaderModuleWorking(
    PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr, 
    VkDevice device, 
    const VkAllocationCallbacks* pAllocator
);

/*
----------------------------------------------------------------------------------------- GLSL code -----------------------------------------------------------------------------------------

#version 450 core

#extension GL_EXT_descriptor_heap : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout (local_size_x = 16, local_size_y = 1, local_size_z = 1) in;

struct Inner2 {
    int value4;
    int value5;
    int value6;
};

struct Inner {
    int value2;
    int value3;
    Inner2 inner2;
};

struct Input {
    int value1;
    Inner inner;
};

struct Output {
    int value2;
    int value3;
    int value6;
};

layout (descriptor_heap) buffer InputBlock {
    Input inputValues[];
} InputBuffers[];

layout (descriptor_heap) buffer OutputBlock {
    Output outputValues[];
} OutputBuffers[];

layout (push_constant) uniform PushConstantBlock {
    uint inputBufferIdx;
    uint outputBufferIdx;
} PushConstant;

void main() {
    uint i = gl_GlobalInvocationID.x;
    OutputBuffers[PushConstant.outputBufferIdx].outputValues[i].value2 = InputBuffers[PushConstant.inputBufferIdx].inputValues[i].inner.value2;
    OutputBuffers[PushConstant.outputBufferIdx].outputValues[i].value3 = InputBuffers[PushConstant.inputBufferIdx].inputValues[i].inner.value3;
    OutputBuffers[PushConstant.outputBufferIdx].outputValues[i].value6 = InputBuffers[PushConstant.inputBufferIdx].inputValues[i].inner.inner2.value6;
}

----------------------------------------------------------------------------------------- SPIR-V code -----------------------------------------------------------------------------------------

; SPIR-V
; Version: 1.3
; Generator: Google Shaderc over Glslang; 11
; Bound: 83
; Schema: 0
               OpCapability Shader
               OpCapability UntypedPointersKHR
               OpCapability DescriptorHeapEXT
               OpExtension "SPV_EXT_descriptor_heap"
               OpExtension "SPV_KHR_untyped_pointers"
          %1 = OpExtInstImport "GLSL.std.450"
               OpMemoryModel Logical GLSL450
               OpEntryPoint GLCompute %main "main" %gl_GlobalInvocationID
               OpExecutionMode %main LocalSize 16 1 1
               OpSource GLSL 450
               OpSourceExtension "GL_EXT_descriptor_heap"
               OpSourceExtension "GL_EXT_nonuniform_qualifier"
               OpSourceExtension "GL_GOOGLE_cpp_style_line_directive"
               OpSourceExtension "GL_GOOGLE_include_directive"
               OpName %main "main"
               OpName %i "i"
               OpName %gl_GlobalInvocationID "gl_GlobalInvocationID"
               OpName %resource_heap "resource_heap"
               OpName %PushConstantBlock "PushConstantBlock"
               OpMemberName %PushConstantBlock 0 "inputBufferIdx"
               OpMemberName %PushConstantBlock 1 "outputBufferIdx"
               OpName %PushConstant "PushConstant"
               OpName %Output "Output"
               OpMemberName %Output 0 "value2"
               OpMemberName %Output 1 "value3"
               OpMemberName %Output 2 "value6"
               OpName %OutputBlock "OutputBlock"
               OpMemberName %OutputBlock 0 "outputValues"
               OpName %Inner2 "Inner2"
               OpMemberName %Inner2 0 "value4"
               OpMemberName %Inner2 1 "value5"
               OpMemberName %Inner2 2 "value6"
               OpName %Inner "Inner"
               OpMemberName %Inner 0 "value2"
               OpMemberName %Inner 1 "value3"
               OpMemberName %Inner 2 "inner2"
               OpName %Input "Input"
               OpMemberName %Input 0 "value1"
               OpMemberName %Input 1 "inner"
               OpName %InputBlock "InputBlock"
               OpMemberName %InputBlock 0 "inputValues"
               OpDecorate %gl_GlobalInvocationID BuiltIn GlobalInvocationId
               OpDecorate %resource_heap BuiltIn ResourceHeapEXT
               OpDecorateId %_runtimearr_18 ArrayStrideIdEXT %19
               OpDecorate %PushConstantBlock Block
               OpMemberDecorate %PushConstantBlock 0 Offset 0
               OpMemberDecorate %PushConstantBlock 1 Offset 4
               OpMemberDecorate %Output 0 Offset 0
               OpMemberDecorate %Output 1 Offset 4
               OpMemberDecorate %Output 2 Offset 8
               OpDecorate %_runtimearr_Output ArrayStride 12
               OpDecorate %OutputBlock Block
               OpMemberDecorate %OutputBlock 0 Offset 0
               OpDecorateId %_runtimearr_18_0 ArrayStrideIdEXT %19
               OpMemberDecorate %Inner2 0 Offset 0
               OpMemberDecorate %Inner2 1 Offset 4
               OpMemberDecorate %Inner2 2 Offset 8
               OpMemberDecorate %Inner 0 Offset 0
               OpMemberDecorate %Inner 1 Offset 4
               OpMemberDecorate %Inner 2 Offset 8
               OpMemberDecorate %Input 0 Offset 0
               OpMemberDecorate %Input 1 Offset 4
               OpDecorate %_runtimearr_Input ArrayStride 24
               OpDecorate %InputBlock Block
               OpMemberDecorate %InputBlock 0 Offset 0
               OpDecorate %gl_WorkGroupSize BuiltIn WorkgroupSize
       %void = OpTypeVoid
          %3 = OpTypeFunction %void
       %uint = OpTypeInt 32 0
%_ptr_Function_uint = OpTypePointer Function %uint
     %v3uint = OpTypeVector %uint 3
%_ptr_Input_v3uint = OpTypePointer Input %v3uint
%gl_GlobalInvocationID = OpVariable %_ptr_Input_v3uint Input
     %uint_0 = OpConstant %uint 0
%_ptr_Input_uint = OpTypePointer Input %uint
%_ptr_UniformConstant = OpTypeUntypedPointerKHR UniformConstant
%resource_heap = OpUntypedVariableKHR %_ptr_UniformConstant UniformConstant
         %18 = OpTypeBufferEXT StorageBuffer
         %19 = OpConstantSizeOfEXT %uint %18
%_runtimearr_18 = OpTypeRuntimeArray %18
%PushConstantBlock = OpTypeStruct %uint %uint
%_ptr_PushConstant_PushConstantBlock = OpTypePointer PushConstant %PushConstantBlock
%PushConstant = OpVariable %_ptr_PushConstant_PushConstantBlock PushConstant
        %int = OpTypeInt 32 1
      %int_1 = OpConstant %int 1
%_ptr_PushConstant_uint = OpTypePointer PushConstant %uint
     %Output = OpTypeStruct %int %int %int
%_runtimearr_Output = OpTypeRuntimeArray %Output
%OutputBlock = OpTypeStruct %_runtimearr_Output
      %int_0 = OpConstant %int 0
%_runtimearr_18_0 = OpTypeRuntimeArray %18
     %Inner2 = OpTypeStruct %int %int %int
      %Inner = OpTypeStruct %int %int %Inner2
      %Input = OpTypeStruct %int %Inner
%_runtimearr_Input = OpTypeRuntimeArray %Input
 %InputBlock = OpTypeStruct %_runtimearr_Input
%_ptr_StorageBuffer_InputBlock = OpTypePointer StorageBuffer %InputBlock
%_ptr_StorageBuffer_int = OpTypePointer StorageBuffer %int
%_ptr_StorageBuffer_OutputBlock = OpTypePointer StorageBuffer %OutputBlock
      %int_2 = OpConstant %int 2
    %uint_16 = OpConstant %uint 16
     %uint_1 = OpConstant %uint 1
%gl_WorkGroupSize = OpConstantComposite %v3uint %uint_16 %uint_1 %uint_1
       %main = OpFunction %void None %3
          %5 = OpLabel
          %i = OpVariable %_ptr_Function_uint Function
         %14 = OpAccessChain %_ptr_Input_uint %gl_GlobalInvocationID %uint_0
         %15 = OpLoad %uint %14
               OpStore %i %15
         %27 = OpAccessChain %_ptr_PushConstant_uint %PushConstant %int_1
         %28 = OpLoad %uint %27
         %33 = OpLoad %uint %i
         %35 = OpAccessChain %_ptr_PushConstant_uint %PushConstant %int_0
         %36 = OpLoad %uint %35
         %42 = OpLoad %uint %i
         %43 = OpUntypedAccessChainKHR %_ptr_UniformConstant %_runtimearr_18_0 %resource_heap %36
         %45 = OpBufferPointerEXT %_ptr_StorageBuffer_InputBlock %43
         %47 = OpAccessChain %_ptr_StorageBuffer_int %45 %int_0 %42 %int_1 %int_0
         %48 = OpLoad %int %47
         %49 = OpUntypedAccessChainKHR %_ptr_UniformConstant %_runtimearr_18 %resource_heap %28
         %51 = OpBufferPointerEXT %_ptr_StorageBuffer_OutputBlock %49
         %52 = OpAccessChain %_ptr_StorageBuffer_int %51 %int_0 %33 %int_0
               OpStore %52 %48
         %53 = OpAccessChain %_ptr_PushConstant_uint %PushConstant %int_1
         %54 = OpLoad %uint %53
         %55 = OpLoad %uint %i
         %56 = OpAccessChain %_ptr_PushConstant_uint %PushConstant %int_0
         %57 = OpLoad %uint %56
         %58 = OpLoad %uint %i
         %59 = OpUntypedAccessChainKHR %_ptr_UniformConstant %_runtimearr_18_0 %resource_heap %57
         %60 = OpBufferPointerEXT %_ptr_StorageBuffer_InputBlock %59
         %61 = OpAccessChain %_ptr_StorageBuffer_int %60 %int_0 %58 %int_1 %int_1
         %62 = OpLoad %int %61
         %63 = OpUntypedAccessChainKHR %_ptr_UniformConstant %_runtimearr_18 %resource_heap %54
         %64 = OpBufferPointerEXT %_ptr_StorageBuffer_OutputBlock %63
         %65 = OpAccessChain %_ptr_StorageBuffer_int %64 %int_0 %55 %int_1
               OpStore %65 %62
         %66 = OpAccessChain %_ptr_PushConstant_uint %PushConstant %int_1
         %67 = OpLoad %uint %66
         %68 = OpLoad %uint %i
         %70 = OpAccessChain %_ptr_PushConstant_uint %PushConstant %int_0
         %71 = OpLoad %uint %70
         %72 = OpLoad %uint %i
         %73 = OpUntypedAccessChainKHR %_ptr_UniformConstant %_runtimearr_18_0 %resource_heap %71
         %74 = OpBufferPointerEXT %_ptr_StorageBuffer_InputBlock %73
         %75 = OpAccessChain %_ptr_StorageBuffer_int %74 %int_0 %72 %int_1 %int_2 %int_2
         %76 = OpLoad %int %75
         %77 = OpUntypedAccessChainKHR %_ptr_UniformConstant %_runtimearr_18 %resource_heap %67
         %78 = OpBufferPointerEXT %_ptr_StorageBuffer_OutputBlock %77
         %79 = OpAccessChain %_ptr_StorageBuffer_int %78 %int_0 %68 %int_2
               OpStore %79 %76
               OpReturn
               OpFunctionEnd

 */
VkShaderModule CreateShaderModuleFailing(
    PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr, 
    VkDevice device, 
    const VkAllocationCallbacks* pAllocator
);

void DestroyShaderModule(
    PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr, 
    VkDevice device, 
    VkShaderModule shaderModule, 
    const VkAllocationCallbacks* pAllocator
);

VkPipeline CreateComputePipeline(
    PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr, 
    VkDevice device, 
    VkShaderModule shaderModule, 
    const VkAllocationCallbacks* pAllocator
);

void DestroyPipeline(
    PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr, 
    VkDevice device, 
    VkPipeline pipeline, 
    const VkAllocationCallbacks* pAllocator
);

void VerifyOutput(
    std::span<const ShaderOutput, TEST_BUFFER_SIZE_ELEMENTS> output, 
    const char* name
);

int main() {
    const VkAllocationCallbacks* pAllocator = nullptr;
    constexpr const char* kVulkanDllName = "vulkan-1.dll";
    const HMODULE vulkanModule = LoadLibraryA(kVulkanDllName);
    ASSERT(vulkanModule != nullptr, 54, "Failed to load Vulkan DLL: %s", kVulkanDllName);
    const auto vkGetInstanceProcAddr = reinterpret_cast<PFN_vkGetInstanceProcAddr>(GetProcAddress(vulkanModule, "vkGetInstanceProcAddr"));
    ASSERT(vkGetInstanceProcAddr != nullptr, 55, "Failed to get proc address of vkGetInstanceProcAddr from vulkan module");
    const auto instance = CreateInstance(vkGetInstanceProcAddr, pAllocator, /* enableValidationLayers= */ VALIDATION_LAYERS_ENABLED);
    const auto debugMessenger = VALIDATION_LAYERS_ENABLED ? CreateDebugUtilsMessengerEXT(vkGetInstanceProcAddr, instance, pAllocator) : VK_NULL_HANDLE;
    const auto physicalDevice = GetPhysicalDevice(vkGetInstanceProcAddr, instance);
    const auto descriptorHeapProperties = GetDescriptorHeapProperties(vkGetInstanceProcAddr, instance, physicalDevice);
    const auto resourceHeapBufferDescriptorsOffset = AlignUp(descriptorHeapProperties.minResourceHeapReservedRange, descriptorHeapProperties.bufferDescriptorSize);
    const auto resourceHeapBufferFirstBufferDescriptorIndex = resourceHeapBufferDescriptorsOffset / descriptorHeapProperties.bufferDescriptorSize;
    const auto resourceHeapBufferSize = resourceHeapBufferDescriptorsOffset + 4 * descriptorHeapProperties.bufferDescriptorSize;
    const auto queueFamilyIndex = GetQueueFamilyIndex(vkGetInstanceProcAddr, instance, physicalDevice);
    const auto device = CreateDevice(vkGetInstanceProcAddr, instance, physicalDevice, queueFamilyIndex, pAllocator);
    const auto vkGetDeviceProcAddr = reinterpret_cast<PFN_vkGetDeviceProcAddr>(vkGetInstanceProcAddr(instance, "vkGetDeviceProcAddr"));
    ASSERT(vkGetDeviceProcAddr != nullptr, 56, "Failed to get instance function pointer vkGetDeviceProcAddr");
    const auto queue = GetQueue(vkGetDeviceProcAddr, device, queueFamilyIndex);
    const auto commandPool = CreateCommandPool(vkGetDeviceProcAddr, device, queueFamilyIndex, pAllocator);
    const auto commandBuffer = CreateCommandBuffer(vkGetDeviceProcAddr, device, commandPool);
    const auto fence = CreateFence(vkGetDeviceProcAddr, device, pAllocator);
    const auto bufferWorkingInput = CreateBufferWorkingInput(vkGetInstanceProcAddr, instance, vkGetDeviceProcAddr, device, physicalDevice, pAllocator);
    const std::span<ShaderInput, TEST_BUFFER_SIZE_ELEMENTS> bufferWorkingInputMapped(static_cast<ShaderInput*>(bufferWorkingInput.m_mapped), TEST_BUFFER_SIZE_ELEMENTS);
	const auto bufferWorkingOutput = CreateBufferWorkingOutput(vkGetInstanceProcAddr, instance, vkGetDeviceProcAddr, device, physicalDevice, pAllocator);
    const std::span<ShaderOutput, TEST_BUFFER_SIZE_ELEMENTS> bufferWorkingOutputMapped(static_cast<ShaderOutput*>(bufferWorkingOutput.m_mapped), TEST_BUFFER_SIZE_ELEMENTS);
	const auto bufferFailingInput = CreateBufferFailingInput(vkGetInstanceProcAddr, instance, vkGetDeviceProcAddr, device, physicalDevice, pAllocator);
    const std::span<ShaderInput, TEST_BUFFER_SIZE_ELEMENTS> bufferFailingInputMapped(static_cast<ShaderInput*>(bufferFailingInput.m_mapped), TEST_BUFFER_SIZE_ELEMENTS);
	const auto bufferFailingOutput = CreateBufferFailingOutput(vkGetInstanceProcAddr, instance, vkGetDeviceProcAddr, device, physicalDevice, pAllocator);
    const std::span<ShaderOutput, TEST_BUFFER_SIZE_ELEMENTS> bufferFailingOutputMapped(static_cast<ShaderOutput*>(bufferFailingOutput.m_mapped), TEST_BUFFER_SIZE_ELEMENTS);
	const auto bufferResourceHeap = CreateBufferResourceHeap(vkGetInstanceProcAddr, instance, vkGetDeviceProcAddr, device, physicalDevice, resourceHeapBufferSize, pAllocator);
	const auto workingShaderModule = CreateShaderModuleWorking(vkGetDeviceProcAddr, device, pAllocator);
    const auto failingShaderModule = CreateShaderModuleFailing(vkGetDeviceProcAddr, device, pAllocator);
    const auto workingPipeline = CreateComputePipeline(vkGetDeviceProcAddr, device, workingShaderModule, pAllocator);
    const auto failingPipeline = CreateComputePipeline(vkGetDeviceProcAddr, device, failingShaderModule, pAllocator);

    // Fill buffers
    {
        constexpr ShaderInput inputValues{
			.value1 = 1,
            .value2 = 2,
            .value3 = 3,
            .value4 = 4,
            .value5 = 5,
            .value6 = 6,
		};
        constexpr ShaderOutput outputValues{
			.value2 = -2,
            .value3 = -3,
            .value6 = -6,
        };
        std::ranges::fill(bufferWorkingInputMapped, inputValues);
        std::ranges::fill(bufferFailingInputMapped, inputValues);
        std::ranges::fill(bufferWorkingOutputMapped, outputValues);
        std::ranges::fill(bufferFailingOutputMapped, outputValues);
        std::byte* resourceHeapFirstDescriptorMapped = static_cast<std::byte*>(bufferResourceHeap.m_mapped) + resourceHeapBufferDescriptorsOffset;
        const VkDeviceAddressRangeEXT deviceAddressRangeWorkingInput{
			.address = bufferWorkingInput.m_deviceAddress,
            .size = TEST_BUFFER_SIZE_ELEMENTS * sizeof(ShaderInput),
        };
        const VkResourceDescriptorInfoEXT resourceDescriptorInfoWorkingInput{
			.sType = VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT,
            .pNext = nullptr,
            .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .data = VkResourceDescriptorDataEXT{
				.pAddressRange = &deviceAddressRangeWorkingInput,
            },
        };
        const VkHostAddressRangeEXT resourceHeapMappedWorkingInput{
            .address = resourceHeapFirstDescriptorMapped + 0 * descriptorHeapProperties.bufferDescriptorSize,
            .size = descriptorHeapProperties.bufferDescriptorSize,
        };
        const VkDeviceAddressRangeEXT deviceAddressRangeWorkingOutput{
			.address = bufferWorkingOutput.m_deviceAddress,
            .size = TEST_BUFFER_SIZE_ELEMENTS * sizeof(ShaderOutput),
        };
        const VkResourceDescriptorInfoEXT resourceDescriptorInfoWorkingOutput{
			.sType = VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT,
            .pNext = nullptr,
            .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .data = VkResourceDescriptorDataEXT{
				.pAddressRange = &deviceAddressRangeWorkingOutput,
            },
        };
        const VkHostAddressRangeEXT resourceHeapMappedWorkingOutput{
            .address = resourceHeapFirstDescriptorMapped + 1 * descriptorHeapProperties.bufferDescriptorSize,
            .size = descriptorHeapProperties.bufferDescriptorSize,
        };

        const VkDeviceAddressRangeEXT deviceAddressRangeFailingInput{
			.address = bufferFailingInput.m_deviceAddress,
            .size = TEST_BUFFER_SIZE_ELEMENTS * sizeof(ShaderInput),
        };
        const VkResourceDescriptorInfoEXT resourceDescriptorInfoFailingInput{
			.sType = VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT,
            .pNext = nullptr,
            .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .data = VkResourceDescriptorDataEXT{
				.pAddressRange = &deviceAddressRangeFailingInput,
            },
        };
        const VkHostAddressRangeEXT resourceHeapMappedFailingInput{
            .address = resourceHeapFirstDescriptorMapped + 2 * descriptorHeapProperties.bufferDescriptorSize,
            .size = descriptorHeapProperties.bufferDescriptorSize,
        };
        const VkDeviceAddressRangeEXT deviceAddressRangeFailingOutput{
			.address = bufferFailingOutput.m_deviceAddress,
            .size = TEST_BUFFER_SIZE_ELEMENTS * sizeof(ShaderOutput),
        };
        const VkResourceDescriptorInfoEXT resourceDescriptorInfoFailingOutput{
			.sType = VK_STRUCTURE_TYPE_RESOURCE_DESCRIPTOR_INFO_EXT,
            .pNext = nullptr,
            .type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            .data = VkResourceDescriptorDataEXT{
				.pAddressRange = &deviceAddressRangeFailingOutput,
            },
        };
        const VkHostAddressRangeEXT resourceHeapMappedFailingOutput{
            .address = resourceHeapFirstDescriptorMapped + 3 * descriptorHeapProperties.bufferDescriptorSize,
            .size = descriptorHeapProperties.bufferDescriptorSize,
        };
        const VkResourceDescriptorInfoEXT resourceDescriptorInfos[4] = {
	        resourceDescriptorInfoWorkingInput,
            resourceDescriptorInfoWorkingOutput,
            resourceDescriptorInfoFailingInput,
            resourceDescriptorInfoFailingOutput,
        };
        const VkHostAddressRangeEXT resourceHeapMappedRanges[4] = {
	        resourceHeapMappedWorkingInput,
            resourceHeapMappedWorkingOutput,
            resourceHeapMappedFailingInput,
            resourceHeapMappedFailingOutput,
        };
        const auto vkWriteResourceDescriptorsEXT = reinterpret_cast<PFN_vkWriteResourceDescriptorsEXT>(vkGetDeviceProcAddr(device, "vkWriteResourceDescriptorsEXT"));
        ASSERT(vkWriteResourceDescriptorsEXT != nullptr, 57, "Failed to get device function pointer vkWriteResourceDescriptorsEXT");
        const VkResult writeDescriptorsToResourceHeapResult = vkWriteResourceDescriptorsEXT(device, 4, resourceDescriptorInfos, resourceHeapMappedRanges);
        ASSERT(writeDescriptorsToResourceHeapResult == VK_SUCCESS, 58, "Failed to write descriptors to resource heap, result code: %i", writeDescriptorsToResourceHeapResult);
    	FlushBuffer(vkGetDeviceProcAddr, device, bufferWorkingInput);
        FlushBuffer(vkGetDeviceProcAddr, device, bufferWorkingOutput);
        FlushBuffer(vkGetDeviceProcAddr, device, bufferFailingInput);
        FlushBuffer(vkGetDeviceProcAddr, device, bufferFailingOutput);
        FlushBuffer(vkGetDeviceProcAddr, device, bufferResourceHeap);
    }


    // Fill command buffers
    {
        const auto vkBeginCommandBuffer = reinterpret_cast<PFN_vkBeginCommandBuffer>(vkGetDeviceProcAddr(device, "vkBeginCommandBuffer"));
        const auto vkEndCommandBuffer = reinterpret_cast<PFN_vkEndCommandBuffer>(vkGetDeviceProcAddr(device, "vkEndCommandBuffer"));
        const auto vkCmdBindResourceHeapEXT = reinterpret_cast<PFN_vkCmdBindResourceHeapEXT>(vkGetDeviceProcAddr(device, "vkCmdBindResourceHeapEXT"));
        const auto vkCmdPushDataEXT = reinterpret_cast<PFN_vkCmdPushDataEXT>(vkGetDeviceProcAddr(device, "vkCmdPushDataEXT"));
        const auto vkCmdBindPipeline = reinterpret_cast<PFN_vkCmdBindPipeline>(vkGetDeviceProcAddr(device, "vkCmdBindPipeline"));
        const auto vkCmdDispatch = reinterpret_cast<PFN_vkCmdDispatch>(vkGetDeviceProcAddr(device, "vkCmdDispatch"));
        ASSERT(vkBeginCommandBuffer != nullptr, 59, "Failed to get device function pointer vkBeginCommandBuffer");
        ASSERT(vkEndCommandBuffer != nullptr, 60, "Failed to get device function pointer vkEndCommandBuffer");
        ASSERT(vkCmdBindResourceHeapEXT != nullptr, 61, "Failed to get device function pointer vkCmdBindResourceHeapEXT");
        ASSERT(vkCmdPushDataEXT != nullptr, 62, "Failed to get device function pointer vkCmdPushDataEXT");
        ASSERT(vkCmdBindPipeline != nullptr, 63, "Failed to get device function pointer vkCmdBindPipeline");
        ASSERT(vkCmdDispatch != nullptr, 64, "Failed to get device function pointer vkCmdDispatch");
        const VkCommandBufferBeginInfo commandBufferBeginInfo{
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            .pInheritanceInfo = nullptr,
        };
        const VkResult beginCommandBufferResult = vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo);
        ASSERT(beginCommandBufferResult == VK_SUCCESS, 65, "Failed to begin command buffer, result code: %i", beginCommandBufferResult);
        const VkBindHeapInfoEXT resourceHeapBindInfo{
			.sType = VK_STRUCTURE_TYPE_BIND_HEAP_INFO_EXT,
            .pNext = nullptr,
            .heapRange = VkDeviceAddressRangeEXT{
				.address = bufferResourceHeap.m_deviceAddress,
                .size = resourceHeapBufferSize,
            },
            .reservedRangeOffset = 0,
            .reservedRangeSize = resourceHeapBufferDescriptorsOffset,
        };
        vkCmdBindResourceHeapEXT(commandBuffer, &resourceHeapBindInfo);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, workingPipeline);
        const ShaderPushData pushDataWorking{
			.inputBufferIdx = static_cast<std::uint32_t>(resourceHeapBufferFirstBufferDescriptorIndex) + 0u,
            .outputBufferIdx = static_cast<std::uint32_t>(resourceHeapBufferFirstBufferDescriptorIndex) + 1u,
        };
        const VkPushDataInfoEXT pushDataInfoWorking{
			.sType = VK_STRUCTURE_TYPE_PUSH_DATA_INFO_EXT,
            .pNext = nullptr,
            .offset = 0,
            .data = VkHostAddressRangeConstEXT{
				.address = &pushDataWorking,
                .size = sizeof(pushDataWorking),
            },
        };
        vkCmdPushDataEXT(commandBuffer, &pushDataInfoWorking);
        vkCmdDispatch(commandBuffer, TEST_BUFFER_SIZE_ELEMENTS / 16, 1, 1);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, failingPipeline);
        const ShaderPushData pushDataFailing{
			.inputBufferIdx = static_cast<std::uint32_t>(resourceHeapBufferFirstBufferDescriptorIndex) + 2u,
            .outputBufferIdx = static_cast<std::uint32_t>(resourceHeapBufferFirstBufferDescriptorIndex) + 3u,
        };
        const VkPushDataInfoEXT pushDataInfoFailing{
			.sType = VK_STRUCTURE_TYPE_PUSH_DATA_INFO_EXT,
            .pNext = nullptr,
            .offset = 0,
            .data = VkHostAddressRangeConstEXT{
				.address = &pushDataFailing,
                .size = sizeof(pushDataFailing),
            },
        };
        vkCmdPushDataEXT(commandBuffer, &pushDataInfoFailing);
        vkCmdDispatch(commandBuffer, TEST_BUFFER_SIZE_ELEMENTS / 16, 1, 1);
        const VkResult endCommandBufferResult = vkEndCommandBuffer(commandBuffer);
        ASSERT(endCommandBufferResult == VK_SUCCESS, 65, "Failed to end command buffer, result code: %i", endCommandBufferResult);
    }

    // Submit and wait
    {
        const auto vkQueueSubmit = reinterpret_cast<PFN_vkQueueSubmit>(vkGetDeviceProcAddr(device, "vkQueueSubmit"));
        const auto vkWaitForFences = reinterpret_cast<PFN_vkWaitForFences>(vkGetDeviceProcAddr(device, "vkWaitForFences"));
        ASSERT(vkQueueSubmit != nullptr, 66, "Failed to get device function pointer vkQueueSubmit");
        ASSERT(vkWaitForFences != nullptr, 67, "Failed to get device function pointer vkWaitForFences");
        const VkSubmitInfo submitInfo{
			.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = nullptr,
            .waitSemaphoreCount = 0,
            .pWaitSemaphores = nullptr,
            .pWaitDstStageMask = nullptr,
            .commandBufferCount = 1,
            .pCommandBuffers = &commandBuffer,
            .signalSemaphoreCount = 0,
            .pSignalSemaphores = nullptr,
        };
        const VkResult queueSubmitResult = vkQueueSubmit(queue, 1, &submitInfo, fence);
        ASSERT(queueSubmitResult == VK_SUCCESS, 68, "Failed to submit command buffer to queue, result code: %i", queueSubmitResult);
        const VkResult waitFenceResult = vkWaitForFences(device, 1, &fence, VK_TRUE, std::numeric_limits<std::uint64_t>::max());
        ASSERT(waitFenceResult == VK_SUCCESS, 69, "Failed to wait fence, result code: %i", waitFenceResult);
        InvalidateCacheBuffer(vkGetDeviceProcAddr, device, bufferWorkingInput);
        InvalidateCacheBuffer(vkGetDeviceProcAddr, device, bufferWorkingOutput);
        InvalidateCacheBuffer(vkGetDeviceProcAddr, device, bufferFailingInput);
        InvalidateCacheBuffer(vkGetDeviceProcAddr, device, bufferFailingOutput);
        InvalidateCacheBuffer(vkGetDeviceProcAddr, device, bufferResourceHeap);
    }
    VerifyOutput(bufferWorkingOutputMapped, "no inner struct");
    VerifyOutput(bufferFailingOutputMapped, "with inner struct");

    
    DestroyBuffer(vkGetDeviceProcAddr, device, bufferWorkingInput, pAllocator);
    DestroyBuffer(vkGetDeviceProcAddr, device, bufferWorkingOutput, pAllocator);
    DestroyBuffer(vkGetDeviceProcAddr, device, bufferFailingInput, pAllocator);
    DestroyBuffer(vkGetDeviceProcAddr, device, bufferFailingOutput, pAllocator);
    DestroyBuffer(vkGetDeviceProcAddr, device, bufferResourceHeap, pAllocator);
    DestroyFence(vkGetDeviceProcAddr, device, fence, pAllocator);
    DestroyCommandBuffer(vkGetDeviceProcAddr, device, commandPool, commandBuffer);
    DestroyCommandPool(vkGetDeviceProcAddr, device, commandPool, pAllocator);
    DestroyPipeline(vkGetDeviceProcAddr, device, workingPipeline, pAllocator);
    DestroyPipeline(vkGetDeviceProcAddr, device, failingPipeline, pAllocator);
    DestroyShaderModule(vkGetDeviceProcAddr, device, failingShaderModule, pAllocator);
    DestroyShaderModule(vkGetDeviceProcAddr, device, workingShaderModule, pAllocator);
    DestroyDevice(vkGetInstanceProcAddr, instance, device, pAllocator);
    if constexpr (VALIDATION_LAYERS_ENABLED) {
        DestroyDebugUtilsMessengerEXT(vkGetInstanceProcAddr, instance, debugMessenger, pAllocator);
    }
    DestroyInstance(vkGetInstanceProcAddr, instance, pAllocator);
    FreeLibrary(vulkanModule);
}

VkDeviceSize AlignUp(const VkDeviceSize offset, const VkDeviceSize alignment) {
    ASSERT(alignment > 0 && (alignment & (alignment - 1)) == 0, 1, "Alignment %llu is not a power of two", alignment);
    return (offset + alignment - 1) & (~(alignment - 1));
}

VkBool32 ValidationLayerDebugCallback(
        const VkDebugUtilsMessageSeverityFlagBitsEXT /*messageSeverity*/,
        const VkDebugUtilsMessageTypeFlagsEXT /*messageType*/,
        const VkDebugUtilsMessengerCallbackDataEXT *const pCallbackData,
        void *const /*pUserData */
) {
    ASSERT(false, EXIT_CODE_ON_ANY_VALIDATION_LAYER_ERROR, "Validation Layer: %s\n", pCallbackData->pMessage);
    return VK_TRUE;
}

VkDebugUtilsMessengerCreateInfoEXT GetDebugUtilsMessengerCreateInfo() {
    constexpr VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo {
            .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
            .pNext = nullptr,
            .flags = 0,
            .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
            .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
            .pfnUserCallback = ValidationLayerDebugCallback,
            .pUserData = nullptr,
    };
    return debugCreateInfo;
}

VkDebugUtilsMessengerEXT CreateDebugUtilsMessengerEXT(
    const PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr, 
    const VkInstance instance, 
    const VkAllocationCallbacks *const pAllocator
) {
    const auto vkCreateDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
    ASSERT(vkCreateDebugUtilsMessengerEXT != nullptr, 2, "Failed to get instance function pointer vkCreateDebugUtilsMessengerEXT");
	const VkDebugUtilsMessengerCreateInfoEXT createInfo = GetDebugUtilsMessengerCreateInfo();
    VkDebugUtilsMessengerEXT debugMessenger;
    const VkResult createDebugUtilsMessengerResult = vkCreateDebugUtilsMessengerEXT(instance, &createInfo, pAllocator, &debugMessenger);
    ASSERT(createDebugUtilsMessengerResult == VK_SUCCESS, 3, "Failed to create debug utils messenger, result code: %i", createDebugUtilsMessengerResult);
    return debugMessenger;
}

void DestroyDebugUtilsMessengerEXT(
    const PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr, 
    const VkInstance instance, 
    const VkDebugUtilsMessengerEXT debugMessenger, 
    const VkAllocationCallbacks *const pAllocator
) {
    const auto vkDestroyDebugUtilsMessengerEXT = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));
    ASSERT(vkDestroyDebugUtilsMessengerEXT != nullptr, 4, "Failed to get instance function pointer vkDestroyDebugUtilsMessengerEXT");
	vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, pAllocator);
}

VkInstance CreateInstance(
    const PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr, 
    const VkAllocationCallbacks *const pAllocator, 
    const bool enableValidationLayers
) {
    const auto vkCreateInstance = reinterpret_cast<PFN_vkCreateInstance>(vkGetInstanceProcAddr(VK_NULL_HANDLE, "vkCreateInstance"));
    ASSERT(vkCreateInstance != nullptr, 5, "Failed to get instance function pointer vkCreateInstance");
    constexpr VkApplicationInfo applicationInfo{
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pNext = nullptr,
            .pApplicationName = nullptr,
            .applicationVersion = 0,
            .pEngineName = nullptr,
            .engineVersion = 0,
            .apiVersion = VK_MAKE_API_VERSION(0, 1, 1, 0),
    };
    const auto debugUtilsMessengerCreateInfo = GetDebugUtilsMessengerCreateInfo();
    constexpr const char* validationLayers[] = {
        "VK_LAYER_KHRONOS_validation",
    };
    constexpr const char* instanceExtensions[] = {
        VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME,
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
    };
    const VkInstanceCreateInfo instanceCreateInfo{
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pNext = (enableValidationLayers ? &debugUtilsMessengerCreateInfo : nullptr),
            .flags = 0,
            .pApplicationInfo = &applicationInfo,
            .enabledLayerCount = enableValidationLayers,
            .ppEnabledLayerNames = validationLayers,
            .enabledExtensionCount = 1u + static_cast<std::uint32_t>(enableValidationLayers),
            .ppEnabledExtensionNames = instanceExtensions,
    };
    VkInstance instance;
    const VkResult createInstanceResult = vkCreateInstance(&instanceCreateInfo, pAllocator, &instance);
    ASSERT(createInstanceResult == VK_SUCCESS, 6, "Failed to create instance, result code: %i", createInstanceResult);
    return instance;
}

void DestroyInstance(
    const PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr, 
    const VkInstance instance, 
    const VkAllocationCallbacks *const pAllocator
) {
    const auto vkDestroyInstance = reinterpret_cast<PFN_vkDestroyInstance>(vkGetInstanceProcAddr(instance, "vkDestroyInstance"));
    ASSERT(vkDestroyInstance != nullptr, 7, "Failed to get instance function pointer vkDestroyInstance");
    vkDestroyInstance(instance, pAllocator);
}

VkPhysicalDevice GetPhysicalDevice(
    const PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr, 
    const VkInstance instance
) {
    const auto vkEnumeratePhysicalDevices = reinterpret_cast<PFN_vkEnumeratePhysicalDevices>(vkGetInstanceProcAddr(instance, "vkEnumeratePhysicalDevices"));
    const auto vkGetPhysicalDeviceProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties>(vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties"));
    ASSERT(vkEnumeratePhysicalDevices != nullptr, 8, "Failed to get instance function pointer vkEnumeratePhysicalDevices");
    ASSERT(vkGetPhysicalDeviceProperties != nullptr, 9, "Failed to get instance function pointer vkGetPhysicalDeviceProperties");
	std::uint32_t physicalDeviceCount;
    const VkResult getPhysicalDeviceCountResult = vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
    ASSERT(getPhysicalDeviceCountResult == VK_SUCCESS, 10, "Failed to get physical device count, result code: %i", getPhysicalDeviceCountResult);
    std::vector<VkPhysicalDevice> physicalDevices(physicalDeviceCount);
    const VkResult getPhysicalDevicesResult = vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, physicalDevices.data());
    ASSERT(getPhysicalDevicesResult == VK_SUCCESS, 11, "Failed to get physical devices, result code: %i", getPhysicalDevicesResult);
    const auto firstDiscretePhysicalDeviceIt = std::ranges::find_if(physicalDevices, [vkGetPhysicalDeviceProperties](const VkPhysicalDevice physicalDevice) -> bool {
        VkPhysicalDeviceProperties physicalDeviceProperties;
        vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);
        return physicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
    });
    ASSERT(firstDiscretePhysicalDeviceIt != std::ranges::end(physicalDevices), 12, "Failed to find discrete physical device");
    return *firstDiscretePhysicalDeviceIt;
}

VkPhysicalDeviceDescriptorHeapPropertiesEXT GetDescriptorHeapProperties(
    const PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr, 
    const VkInstance instance, 
    const VkPhysicalDevice physicalDevice
) {
    const auto vkGetPhysicalDeviceProperties2 = reinterpret_cast<PFN_vkGetPhysicalDeviceProperties2>(vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceProperties2"));
    ASSERT(vkGetPhysicalDeviceProperties2 != nullptr, 13, "Failed to get instance function pointer vkGetPhysicalDeviceProperties2");
	VkPhysicalDeviceDescriptorHeapPropertiesEXT descriptorHeapProperties;
    descriptorHeapProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_HEAP_PROPERTIES_EXT;
    descriptorHeapProperties.pNext = nullptr;
	VkPhysicalDeviceProperties2 physicalDeviceProperties2{
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
        .pNext = &descriptorHeapProperties,
        .properties = {},
    };
    vkGetPhysicalDeviceProperties2(physicalDevice, &physicalDeviceProperties2);
    return descriptorHeapProperties;
}

std::uint32_t GetQueueFamilyIndex(
    const PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr, 
    const VkInstance instance, 
    const VkPhysicalDevice physicalDevice
) {
    const auto vkGetPhysicalDeviceQueueFamilyProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceQueueFamilyProperties>(vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceQueueFamilyProperties"));
    ASSERT(vkGetPhysicalDeviceQueueFamilyProperties != nullptr, 14, "Failed to get instance function pointer vkGetPhysicalDeviceQueueFamilyProperties");
	std::uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());
    for (std::uint32_t i = 0; i < queueFamilyCount; ++i) {
	    if ((queueFamilyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) != 0) {
            return i;
	    }
    }
    ASSERT(false, 15, "Failed to find queue family with compute support!");
}

VkDevice CreateDevice(
    const PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr, 
    const VkInstance instance, 
    const VkPhysicalDevice physicalDevice, 
    const std::uint32_t queueFamilyIndex, 
    const VkAllocationCallbacks *const pAllocator
) {
    const auto vkCreateDevice = reinterpret_cast<PFN_vkCreateDevice>(vkGetInstanceProcAddr(instance, "vkCreateDevice"));
    ASSERT(vkCreateDevice != nullptr, 14, "Failed to get instance function pointer vkCreateDevice");
    constexpr float kQueuePriority = 1.0f;
    const VkDeviceQueueCreateInfo deviceQueueCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .queueFamilyIndex = queueFamilyIndex,
            .queueCount = 1,
            .pQueuePriorities = &kQueuePriority,
    };
    const VkPhysicalDeviceFeatures enabledVulkan10Features{};
    constexpr const char* deviceExtensions[] = {
        VK_KHR_CREATE_RENDERPASS_2_EXTENSION_NAME,
        VK_KHR_DEPTH_STENCIL_RESOLVE_EXTENSION_NAME,
        VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME,
        VK_KHR_MAINTENANCE_5_EXTENSION_NAME,
        VK_KHR_SHADER_UNTYPED_POINTERS_EXTENSION_NAME,
        VK_KHR_BUFFER_DEVICE_ADDRESS_EXTENSION_NAME,
        VK_EXT_DESCRIPTOR_HEAP_EXTENSION_NAME,
    };
    VkPhysicalDeviceBufferDeviceAddressFeatures bufferDeviceAddressFeatures{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES,
        .pNext = nullptr,
        .bufferDeviceAddress = VK_TRUE,
        .bufferDeviceAddressCaptureReplay = VK_FALSE,
		.bufferDeviceAddressMultiDevice = VK_FALSE,
    };
    VkPhysicalDeviceShaderUntypedPointersFeaturesKHR untypedPointerFeatures{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_UNTYPED_POINTERS_FEATURES_KHR,
            .pNext = &bufferDeviceAddressFeatures,
            .shaderUntypedPointers = VK_TRUE,
    };
    VkPhysicalDeviceDescriptorHeapFeaturesEXT descriptorHeapFeatures{
            .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_HEAP_FEATURES_EXT,
            .pNext = &untypedPointerFeatures,
            .descriptorHeap = true,
            .descriptorHeapCaptureReplay = false,
    };
    const VkDeviceCreateInfo deviceCreateInfo{
            .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
            .pNext = &descriptorHeapFeatures,
            .flags = 0,
            .queueCreateInfoCount = 1,
            .pQueueCreateInfos = &deviceQueueCreateInfo,
            .enabledLayerCount = 0,
            .ppEnabledLayerNames = nullptr,
            .enabledExtensionCount = std::size(deviceExtensions),
            .ppEnabledExtensionNames = deviceExtensions,
            .pEnabledFeatures = &enabledVulkan10Features,
    };
    VkDevice device;
    const VkResult createDeviceResult = vkCreateDevice(physicalDevice, &deviceCreateInfo, pAllocator, &device);
    ASSERT(createDeviceResult == VK_SUCCESS, 15, "Failed to create device, result code: %i", createDeviceResult);
    return device;
}

void DestroyDevice(
    const PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr, 
    const VkInstance instance, 
    const VkDevice device, 
    const VkAllocationCallbacks *const pAllocator
) {
    const auto vkDestroyDevice = reinterpret_cast<PFN_vkDestroyDevice>(vkGetInstanceProcAddr(instance, "vkDestroyDevice"));
    ASSERT(vkDestroyDevice != nullptr, 16, "Failed to get instance function pointer vkDestroyDevice");
    vkDestroyDevice(device, pAllocator);
}

VkQueue GetQueue(
    const PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr, 
    const VkDevice device, 
    const std::uint32_t queueFamilyIndex
) {
    const auto vkGetDeviceQueue = reinterpret_cast<PFN_vkGetDeviceQueue>(vkGetDeviceProcAddr(device, "vkGetDeviceQueue"));
    ASSERT(vkGetDeviceQueue != nullptr, 17, "Failed to get device function pointer vkGetDeviceQueue");
    VkQueue queue = VK_NULL_HANDLE;
    vkGetDeviceQueue(device, queueFamilyIndex, 0, &queue);
    return queue;
}

VkCommandPool CreateCommandPool(
    const PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr, 
    const VkDevice device, 
    const std::uint32_t queueFamilyIndex, 
    const VkAllocationCallbacks *const pAllocator
) {
    const auto vkCreateCommandPool = reinterpret_cast<PFN_vkCreateCommandPool>(vkGetDeviceProcAddr(device, "vkCreateCommandPool"));
    ASSERT(vkCreateCommandPool != nullptr, 18, "Failed to get device function pointer vkCreateCommandPool");
    const VkCommandPoolCreateInfo commandPoolCreateInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
        .queueFamilyIndex = queueFamilyIndex,
    };
    VkCommandPool commandPool = VK_NULL_HANDLE;
    const VkResult createCommandPoolResult = vkCreateCommandPool(device, &commandPoolCreateInfo, pAllocator, &commandPool);
    ASSERT(createCommandPoolResult == VK_SUCCESS, 19, "Failed to create command pool, result code: %i", createCommandPoolResult);
    return commandPool;
}

void DestroyCommandPool(
    const PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr, 
    const VkDevice device, 
    const VkCommandPool commandPool, 
    const VkAllocationCallbacks *const pAllocator
) {
    const auto vkDestroyCommandPool = reinterpret_cast<PFN_vkDestroyCommandPool>(vkGetDeviceProcAddr(device, "vkDestroyCommandPool"));
    ASSERT(vkDestroyCommandPool != nullptr, 20, "Failed to get device function pointer vkDestroyCommandPool");
    vkDestroyCommandPool(device, commandPool, pAllocator);
}

VkCommandBuffer CreateCommandBuffer(
    const PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr, 
    const VkDevice device, 
    const VkCommandPool commandPool
) {
    const auto vkAllocateCommandBuffers = reinterpret_cast<PFN_vkAllocateCommandBuffers>(vkGetDeviceProcAddr(device, "vkAllocateCommandBuffers"));
    ASSERT(vkAllocateCommandBuffers != nullptr, 21, "Failed to get device function pointer vkAllocateCommandBuffers");
    const VkCommandBufferAllocateInfo commandBufferAllocateInfo{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = commandPool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1,
	};
    VkCommandBuffer commandBuffer = VK_NULL_HANDLE;
    const VkResult allocateCommandBufferResult = vkAllocateCommandBuffers(device, &commandBufferAllocateInfo, &commandBuffer);
    ASSERT(allocateCommandBufferResult == VK_SUCCESS, 22, "Failed to allocate command buffer, result code: %i", allocateCommandBufferResult);
    return commandBuffer;
}

void DestroyCommandBuffer(
    const PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr, 
    const VkDevice device, 
    const VkCommandPool commandPool, 
    const VkCommandBuffer commandBuffer
) {
    const auto vkFreeCommandBuffers = reinterpret_cast<PFN_vkFreeCommandBuffers>(vkGetDeviceProcAddr(device, "vkFreeCommandBuffers"));
    ASSERT(vkFreeCommandBuffers != nullptr, 23, "Failed to get device function pointer vkFreeCommandBuffers");
    vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

VkFence CreateFence(
    const PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr, 
    const VkDevice device, 
    const VkAllocationCallbacks *const pAllocator
) {
    const auto vkCreateFence = reinterpret_cast<PFN_vkCreateFence>(vkGetDeviceProcAddr(device, "vkCreateFence"));
    ASSERT(vkCreateFence != nullptr, 24, "Failed to get device function pointer vkCreateFence");
    const VkFenceCreateInfo fenceCreateInfo{
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
    };
    VkFence fence = VK_NULL_HANDLE;
    const VkResult createFenceResult = vkCreateFence(device, &fenceCreateInfo, pAllocator, &fence);
    ASSERT(createFenceResult == VK_SUCCESS, 25, "Failed to create fence, result code: %i", createFenceResult);
    return fence;
}

void DestroyFence(
    const PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr, 
    const VkDevice device, 
    const VkFence fence, 
    const VkAllocationCallbacks *const pAllocator
) {
    const auto vkDestroyFence = reinterpret_cast<PFN_vkDestroyFence>(vkGetDeviceProcAddr(device, "vkDestroyFence"));
    ASSERT(vkDestroyFence != nullptr, 26, "Failed to get device function pointer vkDestroyFence");
    vkDestroyFence(device, fence, pAllocator);
}

std::uint32_t GetMemoryTypeIndex(
    const PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr, 
    const VkInstance instance, 
    const VkPhysicalDevice physicalDevice, 
    const std::uint32_t memoryTypeBitsRequirement
) {
    const auto vkGetPhysicalDeviceMemoryProperties = reinterpret_cast<PFN_vkGetPhysicalDeviceMemoryProperties>(vkGetInstanceProcAddr(instance, "vkGetPhysicalDeviceMemoryProperties"));
    ASSERT(vkGetPhysicalDeviceMemoryProperties != nullptr, 27, "Failed to get instance function pointer vkGetPhysicalDeviceMemoryProperties");
	VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);
    for (std::uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
        if ((memoryTypeBitsRequirement & (1u << i)) == 0u) {
	        continue;
        }
	    if (memoryProperties.memoryTypes[i].propertyFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
            return i;
	    }
    }
    ASSERT(false, 28, "Failed to find host-visible memory type for memoryTypeBitsRequirement %u", memoryTypeBitsRequirement);
}

BufferAllocation CreateBuffer(
    const PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr, 
    const VkInstance instance, 
    const PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr, 
    const VkDevice device, 
    const VkPhysicalDevice physicalDevice, 
    const VkBufferCreateInfo& bufferCreateInfo, 
    const VkAllocationCallbacks *const pAllocator
) {
    const auto vkCreateBuffer = reinterpret_cast<PFN_vkCreateBuffer>(vkGetDeviceProcAddr(device, "vkCreateBuffer"));
    const auto vkGetBufferMemoryRequirements = reinterpret_cast<PFN_vkGetBufferMemoryRequirements>(vkGetDeviceProcAddr(device, "vkGetBufferMemoryRequirements"));
    const auto vkAllocateMemory = reinterpret_cast<PFN_vkAllocateMemory>(vkGetDeviceProcAddr(device, "vkAllocateMemory"));
    const auto vkBindBufferMemory = reinterpret_cast<PFN_vkBindBufferMemory>(vkGetDeviceProcAddr(device, "vkBindBufferMemory"));
    const auto vkGetBufferDeviceAddressKHR = reinterpret_cast<PFN_vkGetBufferDeviceAddressKHR>(vkGetDeviceProcAddr(device, "vkGetBufferDeviceAddressKHR"));
    const auto vkMapMemory = reinterpret_cast<PFN_vkMapMemory>(vkGetDeviceProcAddr(device, "vkMapMemory"));
    ASSERT(vkCreateBuffer != nullptr, 29, "Failed to get device function pointer vkCreateBuffer");
    ASSERT(vkGetBufferMemoryRequirements != nullptr, 30, "Failed to get device function pointer vkGetBufferMemoryRequirements");
    ASSERT(vkAllocateMemory != nullptr, 31, "Failed to get device function pointer vkAllocateMemory");
    ASSERT(vkBindBufferMemory != nullptr, 32, "Failed to get device function pointer vkBindBufferMemory");
    ASSERT(vkGetBufferDeviceAddressKHR != nullptr, 33, "Failed to get device function pointer vkGetBufferDeviceAddressKHR");
    ASSERT(vkMapMemory != nullptr, 34, "Failed to get device function pointer vkMapMemory");
	VkBuffer buffer = VK_NULL_HANDLE;
    const VkResult createBufferResult = vkCreateBuffer(device, &bufferCreateInfo, pAllocator, &buffer);
    ASSERT(createBufferResult == VK_SUCCESS, 35, "Failed to create buffer, result code: %i", createBufferResult);
    VkMemoryRequirements memoryRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memoryRequirements);
    const std::uint32_t memoryTypeIndex = GetMemoryTypeIndex(vkGetInstanceProcAddr, instance, physicalDevice, memoryRequirements.memoryTypeBits);
    constexpr VkMemoryAllocateFlagsInfo addressableDeviceMemoryAllocationFlags {
	        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_FLAGS_INFO,
	        .pNext = nullptr,
	        .flags = VK_MEMORY_ALLOCATE_DEVICE_ADDRESS_BIT,
	        .deviceMask = 1u,
	};
    const VkMemoryAllocateInfo memoryAllocateInfo{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = &addressableDeviceMemoryAllocationFlags,
        .allocationSize = memoryRequirements.size,
        .memoryTypeIndex = memoryTypeIndex,
    };
    VkDeviceMemory deviceMemory = VK_NULL_HANDLE;
    const VkResult allocateDeviceMemoryResult = vkAllocateMemory(device, &memoryAllocateInfo, pAllocator, &deviceMemory);
    ASSERT(allocateDeviceMemoryResult == VK_SUCCESS, 36, "Failed to allocate device memory, result code: %i", allocateDeviceMemoryResult);
    const VkResult bindBufferToDeviceMemoryResult = vkBindBufferMemory(device, buffer, deviceMemory, 0);
    ASSERT(bindBufferToDeviceMemoryResult == VK_SUCCESS, 37, "Failed to bind buffer to device memory, result code: %i", bindBufferToDeviceMemoryResult);
    const VkBufferDeviceAddressInfo bufferDeviceAddressInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO,
        .pNext = nullptr,
        .buffer = buffer,
    };
    const VkDeviceAddress deviceAddress = vkGetBufferDeviceAddressKHR(device, &bufferDeviceAddressInfo);
    void* mapped = nullptr;
    const VkResult mapMemoryResult = vkMapMemory(device, deviceMemory, 0, memoryRequirements.size, 0, &mapped);
    ASSERT(mapMemoryResult == VK_SUCCESS, 38, "Failed to map host-visible device memory, result code: %i", mapMemoryResult);
    return BufferAllocation{
		.m_buffer = buffer,
        .m_deviceMemory = deviceMemory,
        .m_deviceAddress = deviceAddress,
        .m_mapped = mapped,
    };
}

void DestroyBuffer(
    const PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr, 
    const VkDevice device, 
    const BufferAllocation& bufferAllocation, 
    const VkAllocationCallbacks *const pAllocator
) {
    const auto vkDestroyBuffer = reinterpret_cast<PFN_vkDestroyBuffer>(vkGetDeviceProcAddr(device, "vkDestroyBuffer"));
    const auto vkFreeMemory = reinterpret_cast<PFN_vkFreeMemory>(vkGetDeviceProcAddr(device, "vkFreeMemory"));
    ASSERT(vkDestroyBuffer != nullptr, 39, "Failed to get device function pointer vkDestroyBuffer");
    ASSERT(vkFreeMemory != nullptr, 40, "Failed to get device function pointer vkFreeMemory");
    vkDestroyBuffer(device, bufferAllocation.m_buffer, pAllocator);
    vkFreeMemory(device, bufferAllocation.m_deviceMemory, pAllocator);
}

void FlushBuffer(
    const PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr, 
    const VkDevice device, 
    const BufferAllocation& bufferAllocation
) {
    const auto vkFlushMappedMemoryRanges = reinterpret_cast<PFN_vkFlushMappedMemoryRanges>(vkGetDeviceProcAddr(device, "vkFlushMappedMemoryRanges"));
    ASSERT(vkFlushMappedMemoryRanges != nullptr, 41, "Failed to get device function pointer vkFlushMappedMemoryRanges");
    const VkMappedMemoryRange mappedMemoryRange{
		.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
        .pNext = nullptr,
        .memory = bufferAllocation.m_deviceMemory,
        .offset = 0,
        .size = VK_WHOLE_SIZE,
    };
    const VkResult flushMappedMemoryRangeResult = vkFlushMappedMemoryRanges(device, 1, &mappedMemoryRange);
    ASSERT(flushMappedMemoryRangeResult == VK_SUCCESS, 42, "Failed to flush mapped memory, result code: %i", flushMappedMemoryRangeResult);
}

void InvalidateCacheBuffer(
    const PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr, 
    const VkDevice device, 
    const BufferAllocation& bufferAllocation
) {
    const auto vkInvalidateMappedMemoryRanges = reinterpret_cast<PFN_vkInvalidateMappedMemoryRanges>(vkGetDeviceProcAddr(device, "vkInvalidateMappedMemoryRanges"));
    ASSERT(vkInvalidateMappedMemoryRanges != nullptr, 43, "Failed to get device function pointer vkInvalidateMappedMemoryRanges");
	const VkMappedMemoryRange mappedMemoryRange{
		.sType = VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE,
        .pNext = nullptr,
        .memory = bufferAllocation.m_deviceMemory,
        .offset = 0,
        .size = VK_WHOLE_SIZE,
    };
    const VkResult invalidateMappedMemoryRangesResult = vkInvalidateMappedMemoryRanges(device, 1, &mappedMemoryRange);
    ASSERT(invalidateMappedMemoryRangesResult == VK_SUCCESS, 44, "Failed to invalidate mapped memory, result code: %i", invalidateMappedMemoryRangesResult);
}

BufferAllocation CreateBufferWorkingInput(
    const PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr, 
    const VkInstance instance, 
    const PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr, 
    const VkDevice device, 
    const VkPhysicalDevice physicalDevice, 
    const VkAllocationCallbacks *const pAllocator
) {
    const VkBufferCreateInfo bufferCreateInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = TEST_BUFFER_SIZE_ELEMENTS * sizeof(ShaderInput),
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    };
    return CreateBuffer(vkGetInstanceProcAddr, instance, vkGetDeviceProcAddr, device, physicalDevice, bufferCreateInfo, pAllocator);
}

BufferAllocation CreateBufferFailingInput(
    const PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr, 
    const VkInstance instance, 
    const PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr, 
    const VkDevice device, 
    const VkPhysicalDevice physicalDevice, 
    const VkAllocationCallbacks *const pAllocator
) {
    const VkBufferCreateInfo bufferCreateInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = TEST_BUFFER_SIZE_ELEMENTS * sizeof(ShaderInput),
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    };
    return CreateBuffer(vkGetInstanceProcAddr, instance, vkGetDeviceProcAddr, device, physicalDevice, bufferCreateInfo, pAllocator);
}

BufferAllocation CreateBufferWorkingOutput(
    const PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr, 
    const VkInstance instance, 
    const PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr, 
    const VkDevice device, 
    const VkPhysicalDevice physicalDevice, 
    const VkAllocationCallbacks *const pAllocator
) {
    const VkBufferCreateInfo bufferCreateInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = TEST_BUFFER_SIZE_ELEMENTS * sizeof(ShaderOutput),
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    };
    return CreateBuffer(vkGetInstanceProcAddr, instance, vkGetDeviceProcAddr, device, physicalDevice, bufferCreateInfo, pAllocator);
}

BufferAllocation CreateBufferFailingOutput(
    const PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr, 
    const VkInstance instance, 
    const PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr, 
    const VkDevice device, 
    const VkPhysicalDevice physicalDevice, 
    const VkAllocationCallbacks *const pAllocator
) {
    const VkBufferCreateInfo bufferCreateInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = TEST_BUFFER_SIZE_ELEMENTS * sizeof(ShaderOutput),
        .usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    };
    return CreateBuffer(vkGetInstanceProcAddr, instance, vkGetDeviceProcAddr, device, physicalDevice, bufferCreateInfo, pAllocator);
}

BufferAllocation CreateBufferResourceHeap(
    const PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr, 
    const VkInstance instance, 
    const PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr, 
    const VkDevice device, 
    const VkPhysicalDevice physicalDevice, 
    const VkDeviceSize resourceHeapBufferSize, 
    const VkAllocationCallbacks *const pAllocator
) {
    const VkBufferCreateInfo bufferCreateInfo{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .size = resourceHeapBufferSize,
        .usage = VK_BUFFER_USAGE_DESCRIPTOR_HEAP_BIT_EXT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
    };
    return CreateBuffer(vkGetInstanceProcAddr, instance, vkGetDeviceProcAddr, device, physicalDevice, bufferCreateInfo, pAllocator);
}

VkShaderModule CreateShaderModule(
    const PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr, 
    const VkDevice device, 
    const char *const filepath, 
    const VkAllocationCallbacks *const pAllocator
) {
    const auto vkCreateShaderModule = reinterpret_cast<PFN_vkCreateShaderModule>(vkGetDeviceProcAddr(device, "vkCreateShaderModule"));
    ASSERT(vkCreateShaderModule != nullptr, 45, "Failed to get device function pointer vkCreateShaderModule");
    const std::filesystem::path path(filepath);
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    ASSERT(file.is_open(), 46, "Failed to open file: %s", filepath);
    std::vector<std::uint32_t> spirvBuffer;
    const auto size = file.tellg();
    ASSERT(size % 4 == 0, 47, "Corrupted SPIR-V file: %s, size: %lli is not divisible by 4", filepath, static_cast<std::int64_t>(size));
    file.seekg(0, std::ios::beg);
    spirvBuffer.resize(size / sizeof(spirvBuffer[0]));
    file.read(reinterpret_cast<char*>(spirvBuffer.data()), size);
    ASSERT(file.gcount() == size, 48, "Failed to read file: %s, read %lli instead of %lli", filepath, static_cast<std::int64_t>(file.gcount()), static_cast<std::int64_t>(size));
    const VkShaderModuleCreateInfo shaderModuleCreateInfo{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0,
        .codeSize = static_cast<std::size_t>(size),
        .pCode = spirvBuffer.data(),
    };
    VkShaderModule shaderModule = VK_NULL_HANDLE;
    const VkResult createShaderModuleResult = vkCreateShaderModule(device, &shaderModuleCreateInfo, pAllocator, &shaderModule);
    ASSERT(createShaderModuleResult == VK_SUCCESS, 49, "Failed to create shader module, result code: %i", createShaderModuleResult);
    return shaderModule;
}

VkShaderModule CreateShaderModuleWorking(
    const PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr, 
    const VkDevice device, 
    const VkAllocationCallbacks *const pAllocator
) {
    return CreateShaderModule(vkGetDeviceProcAddr, device, TEST_WORKING_SHADER_SPIRV_FILEPATH, pAllocator);
}

VkShaderModule CreateShaderModuleFailing(
    const PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr, 
    const VkDevice device, 
    const VkAllocationCallbacks *const pAllocator
) {
    return CreateShaderModule(vkGetDeviceProcAddr, device, TEST_FAILING_SHADER_SPIRV_FILEPATH, pAllocator);
}

void DestroyShaderModule(
    const PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr, 
    const VkDevice device, 
    const VkShaderModule shaderModule, 
    const VkAllocationCallbacks *const pAllocator
) {
	const auto vkDestroyShaderModule = reinterpret_cast<PFN_vkDestroyShaderModule>(vkGetDeviceProcAddr(device, "vkDestroyShaderModule"));
    ASSERT(vkDestroyShaderModule != nullptr, 50, "Failed to get device function pointer vkDestroyShaderModule");
    vkDestroyShaderModule(device, shaderModule, pAllocator);
}

VkPipeline CreateComputePipeline(
    const PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr, 
    const VkDevice device, 
    const VkShaderModule shaderModule, 
    const VkAllocationCallbacks *const pAllocator
) {
	const auto vkCreateComputePipelines = reinterpret_cast<PFN_vkCreateComputePipelines>(vkGetDeviceProcAddr(device, "vkCreateComputePipelines"));
    ASSERT(vkCreateComputePipelines != nullptr, 51, "Failed to get device function pointer vkCreateComputePipelines");
    const VkPipelineCreateFlags2CreateInfo pipelineCreateFlags2{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_CREATE_FLAGS_2_CREATE_INFO,
        .pNext = nullptr,
        .flags = VK_PIPELINE_CREATE_2_DESCRIPTOR_HEAP_BIT_EXT,
    };
    const VkComputePipelineCreateInfo computePipelineCreateInfo{
		.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .pNext = &pipelineCreateFlags2,
        .flags = 0,
        .stage = VkPipelineShaderStageCreateInfo{
			.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0,
            .stage = VK_SHADER_STAGE_COMPUTE_BIT,
            .module = shaderModule,
            .pName = "main",
            .pSpecializationInfo = nullptr,
        },
        .layout = VK_NULL_HANDLE,
        .basePipelineHandle = VK_NULL_HANDLE,
        .basePipelineIndex = 0,
    };
    VkPipeline computePipeline = VK_NULL_HANDLE;
    const VkResult createComputePipelineResult = vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &computePipelineCreateInfo, pAllocator, &computePipeline);
    ASSERT(createComputePipelineResult == VK_SUCCESS, 52, "Failed to create compute pipeline, result code: %i", createComputePipelineResult);
	return computePipeline;
}

void DestroyPipeline(
    const PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr, 
    const VkDevice device, 
    const VkPipeline pipeline, 
    const VkAllocationCallbacks *const pAllocator
) {
	const auto vkDestroyPipeline = reinterpret_cast<PFN_vkDestroyPipeline>(vkGetDeviceProcAddr(device, "vkDestroyPipeline"));
    ASSERT(vkDestroyPipeline != nullptr, 53, "Failed to get device function pointer vkDestroyPipeline");
    vkDestroyPipeline(device, pipeline, pAllocator);
}

void VerifyOutput(
    const std::span<const ShaderOutput, TEST_BUFFER_SIZE_ELEMENTS> output, 
    const char *const name
) {
    bool allOk = true;
    bool allShifted = true;
    for (const auto [value2, value3, value6]: output) {
        const bool isOk = value2 == 2 && value3 == 3 && value6 == 6;
        const bool isShifted = value2 == 1 && value3 == 2 && value6 == 3;
        allOk &= isOk;
        allShifted &= isShifted;
    }
    if (allOk) {
        printf("all ok for \"%s\"\n", name);
        return;
    }
    if (allShifted) {
	    printf("failed: mismatch pattern like offset of inner struct is ignored for \"%s\"\n", name);
        return;
    }
    printf("failed: unknown mismatch pattern for \"%s\"\n", name);
}
