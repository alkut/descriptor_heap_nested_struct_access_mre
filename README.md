## Descriptor heap nested struct access MRE

This is a minimal reproducable example, that shows unexpected behavour of VK_EXT_descriptor_heap extension when Storage Buffer contains a nested structs.

It looks like pipeline compiler ignores all offsets in access chain, except for the last (most inner) struct

For example,

```
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
```

SPIR-V seems to be correct, let's consider this line

```
InputBuffers[PushConstant.inputBufferIdx].inputValues[i].inner.inner2.value6
```

It translates to 

```
         %73 = OpUntypedAccessChainKHR %_ptr_UniformConstant %_runtimearr_18_0 %resource_heap %71
         %74 = OpBufferPointerEXT %_ptr_StorageBuffer_InputBlock %73
         %75 = OpAccessChain %_ptr_StorageBuffer_int %74 %int_0 %72 %int_1 %int_2 %int_2
         %76 = OpLoad %int %75
```

Let's take a look at

OpAccessChain %_ptr_StorageBuffer_int %74 %int_0 %72 %int_1 %int_2 %int_2

(note that %int_0 is 0 literal constant, %int_1 is 1, %int_2 is 2, %72 is i)

- %74 is InputBlock 
- then access 0-th member, get inputValues[]
- then access i-th index, get inputValues[i]
- then access 1-th member, get inputValues[i].inner
- then acess 2-th member, get inputValues[i].inner.inner2
- then acess 2-th member, get inputValues[i].inner.inner2.value6

All offset/stride decorations also seem right:

```
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
```

The observable behavour is - all offsets in access chain are ignored, except for the last one.

In the given code:

```
void main() {
    uint i = gl_GlobalInvocationID.x;
    OutputBuffers[PushConstant.outputBufferIdx].outputValues[i].value2 = InputBuffers[PushConstant.inputBufferIdx].inputValues[i].inner.value2;
    OutputBuffers[PushConstant.outputBufferIdx].outputValues[i].value3 = InputBuffers[PushConstant.inputBufferIdx].inputValues[i].inner.value3;
    OutputBuffers[PushConstant.outputBufferIdx].outputValues[i].value6 = InputBuffers[PushConstant.inputBufferIdx].inputValues[i].inner.inner2.value6;
}
```

InputBuffers[PushConstant.inputBufferIdx].inputValues[i].inner.value2 reads data with offset = offsetof(Inner, value2) = 0 (and it's Input.value1)

InputBuffers[PushConstant.inputBufferIdx].inputValues[i].inner.value3 reads data with offset = offsetof(Inner, value3) = 4 (and it's Input.inner.value2)

InputBuffers[PushConstant.inputBufferIdx].inputValues[i].inner.inner2.value6 reads data with offset = offsetof(Inner2, value6) = 8 (and it's Input.inner.value3)

## Requirements

- cmake 3.31 or newer
- c++20 compiler (only msvc tested)
- vulkan sdk (only Vulkan SDK 1.4.357 tested)

## Environment

- OS: Windows 11 IoT Enterprise 22H2
- driver version: Game Ready Driver 610.88
- hardware: GeForce RTX 3060 12 GB

## Build steps

From project root directory:

```
mkdir build
cmake -S . -B ./build
cmake --build ./build
.\build\Debug\descriptor_heap_nested_struct_access_mre.exe
```

## Sample output

```
PS C:\Users\ALEKSEY\source\repos\alkut\descriptor_heap_nested_struct_access_mre> .\build\Debug\descriptor_heap_nested_struct_access_mre.exe
all ok for "no inner struct"
failed: mismatch pattern like offset of inner struct is ignored for "with inner struct"
PS C:\Users\ALEKSEY\source\repos\alkut\descriptor_heap_nested_struct_access_mre>
```

## What this example verifies?

I created two pipelines - "failing" and "working". The only difference between them is flat/nested structs.

Working shader:

```
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
```

Failing shader:

```
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
```

Cmake already has pre-build steps for compiling both shaders into SPIR-V and disasm them - can be found in ${ProjRoot}/shaders/compiled

Example of SPIR-V for working shader:

```
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
```

Example of SPIR-V for failing shader:

```
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
```

Application do following steps
- creates 2 pipeline, 4 buffers (input and output for working and failing)
- fills buffers with initial values
- submits work for both pipelines
- waits and validate output buffer

Fill buffers with initial values:

```
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
```

Verify output buffers:

```
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
```
