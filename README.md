# Nested Struct Member Offset Calculation in `VK_EXT_descriptor_heap`

Minimal Reproducible Example (MRE) demonstrating an unexpected result when accessing nested struct members within a Storage Buffer using `VK_EXT_descriptor_heap`.

When SPIR-V access chains traverse multiple levels of nested structs via `OpBufferPointerEXT` and `OpAccessChain`, intermediate struct member offsets appear to be omitted during driver shader compilation, causing loads to read from shifted offsets.

---

## Precompiled Artifacts (`shaders_preview/`)

Precompiled SPIR-V binaries, SPIR-V disassemblies, extracted CUDA binaries (`.cubin`), and disassembled SASS (`.sass`) for SM86 are provided in [`shaders_preview/`](shaders_preview/) for direct inspection without requiring a local build environment:

- **[`shaders_preview/descriptor_heap_layout/`](shaders_preview/descriptor_heap_layout/)** (Demonstration):
  - `working.comp` & `failing.comp`: GLSL sources using `layout(descriptor_heap)`.
  - `working.comp.spv.disasm` & `failing.comp.spv.disasm`: SPIR-V disassemblies.
  - `working.cubin` & `failing.cubin`: Pipeline binary containers dumped via `VK_KHR_pipeline_binary`.
  - `working.sass` & `failing.sass`: SASS disassembly generated with `nvdisasm -b SM86`.
- **[`shaders_preview/shader_mapping/`](shaders_preview/shader_mapping/)** (Workaround):
  - Corresponding shaders using standard descriptor set bindings remapped at pipeline creation via `VkShaderDescriptorSetAndBindingMappingInfoEXT`.

---

## Technical Analysis

### 1. Shader Structures (GLSL)

The test compares a flat struct (`working.comp`) against an identically laid-out nested struct (`failing.comp`):

```glsl
// shaders/descriptor_heap_layout/failing.comp
#version 450 core

#extension GL_EXT_descriptor_heap : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout (local_size_x = 16, local_size_y = 1, local_size_z = 1) in;

struct Inner2 {
    int value4; // offset 0
    int value5; // offset 4
    int value6; // offset 8
};

struct Inner {
    int value2;     // offset 0
    int value3;     // offset 4
    Inner2 inner2;  // offset 8
};

struct Input {
    int value1;   // offset 0
    Inner inner;  // offset 4
};

struct Output {
    int value2; // offset 0
    int value3; // offset 4
    int value6; // offset 8
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

In `working.comp`, `Input` is flat:

```glsl
struct Input {
    int value1; // offset 0
    int value2; // offset 4
    int value3; // offset 8
    int value4; // offset 12
    int value5; // offset 16
    int value6; // offset 20
};
```

Expected byte offsets of loaded fields within `Input`:
- `inner.value2`: $\text{offsetof}(Input, inner) + \text{offsetof}(Inner, value2) = 4 + 0 = 4 \text{ (0x04)}$
- `inner.value3`: $\text{offsetof}(Input, inner) + \text{offsetof}(Inner, value3) = 4 + 4 = 8 \text{ (0x08)}$
- `inner.inner2.value6`: $\text{offsetof}(Input, inner) + \text{offsetof}(Inner, inner2) + \text{offsetof}(Inner2, value6) = 4 + 8 + 8 = 20 \text{ (0x14)}$

### 2. SPIR-V Lowering

The disassembly in `shaders_preview/descriptor_heap_layout/failing.comp.spv.disasm` shows standard `OpAccessChain` instructions with member decorations:

```spirv
OpMemberDecorate %Inner2 2 Offset 8
OpMemberDecorate %Inner  2 Offset 8
OpMemberDecorate %Input  1 Offset 4
...
         %70 = OpAccessChain %_ptr_PushConstant_uint %PushConstant %int_0
         %71 = OpLoad %uint %70
         %72 = OpLoad %uint %i
         %73 = OpUntypedAccessChainKHR %_ptr_UniformConstant %_runtimearr_18_0 %resource_heap %71
         %74 = OpBufferPointerEXT %_ptr_StorageBuffer_InputBlock %73
         %75 = OpAccessChain %_ptr_StorageBuffer_int %74 %int_0 %72 %int_1 %int_2 %int_2
         %76 = OpLoad %int %75
```

The access chain for `value6` passes:
- `%int_0`: `InputBlock.inputValues`
- `%72`: array index `i`
- `%int_1`: member 1 of `Input` (`inner`)
- `%int_2`: member 2 of `Inner` (`inner2`)
- `%int_2`: member 2 of `Inner2` (`value6`)

The module compiles with `glslc` and passes validation via `spirv-val`.

### 3. SASS Disassembly (`nvdisasm -b SM86`)

In both shaders, the array element base offset within `InputBlock` is computed identically using the struct stride:
```sass
// sizeof(Input) = 6 * sizeof(int) = 24 bytes (0x18 stride)
// R0 = global invocation ID i
/*00c0*/  IMAD R1, R0, 0x18, RZ ;   // R1 = i * 24 (element base offset)
```

The discrepancy occurs in the member offsets added relative to `element_base` (`R1`):

#### Working Shader (`working.sass` — Flat Struct)

```sass
// 1. value2: expected offset = element_base + 0x4
/*00d0*/  IADD3 R2, R1, 0x4, RZ ;          // R2 = element_base + 0x4 (offsetof value2)
...
/*0110*/  LDG.E R2, [R2.U32+UR6], !P0 ;     // loads from element_base + 0x4 (value2 = 2)

// 2. value3: expected offset = element_base + 0x8
/*0140*/  IADD3 R4, R1, 0x8, RZ ;          // R4 = element_base + 0x8 (offsetof value3)
...
/*0200*/  LDG.E R4, [R4.U32+UR6], !P1 ;     // loads from element_base + 0x8 (value3 = 3)

// 3. value6: expected offset = element_base + 0x14 (20 bytes)
/*0220*/  IADD3 R1, R1, 0x14, RZ ;         // R1 = element_base + 0x14 (offsetof value6)
...
/*02b0*/  LDG.E R1, [R1.U32+UR6], !P1 ;     // loads from element_base + 0x14 (value6 = 6)
```

#### Failing Shader (`failing.sass` — Nested Struct)

```sass
// 1. inner.value2: expected offset = element_base + 0x4
// No offset is added to element_base; R1 (+ 0 bytes) is used directly:
/*00e0*/  SEL R2, R1, RZ, !P0 ;            // R2 = element_base + 0x0 (offset of inner struct omitted!)
...
/*0100*/  LDG.E R2, [R2.U32+UR6], !P0 ;     // loads from element_base + 0x0 (reads value1 = 1 instead)

// 2. inner.value3: expected offset = element_base + 0x8
// Only offsetof(Inner, value3) is added; offsetof(Input, inner) = 4 is omitted:
/*0130*/  IADD3 R4, R1, 0x4, RZ ;          // R4 = element_base + 0x4 (reads value2 = 2 instead)
...
/*01f0*/  LDG.E R4, [R4.U32+UR6], !P1 ;     // loads from element_base + 0x4

// 3. inner.inner2.value6: expected offset = element_base + 0x14 (20 bytes)
// Only offsetof(Inner2, value6) is added; outer offsets (4 + 8 = 12) are omitted:
/*0210*/  IADD3 R1, R1, 0x8, RZ ;          // R1 = element_base + 0x8 (reads value3 = 3 instead)
...
/*02a0*/  LDG.E R1, [R1.U32+UR6], !P1 ;     // loads from element_base + 0x8
```

Offset comparison summary relative to `element_base`:

The emitted offset in `failing.sass` is **always strictly the local offset within the innermost (leaf) struct**, regardless of the nesting depth:

| Field Access | Expected Total Member Offset | `working.sass` Member Offset | `failing.sass` Member Offset | Innermost Struct Local Offset | Data Actually Loaded |
|---|---|---|---|---|---|
| `inner.value2` | `+0x04` | `+0x04` (`/*00d0*/`) | `+0x00` (no addition, `/*00e0*/`) | `offsetof(Inner, value2) = 0` (`+0x00`) | `value1` (offset 0) |
| `inner.value3` | `+0x08` | `+0x08` (`/*0140*/`) | `+0x04` (`/*0130*/`) | `offsetof(Inner, value3) = 4` (`+0x04`) | `value2` (offset 4) |
| `inner.inner2.value6` | `+0x14` | `+0x14` (`/*0220*/`) | `+0x08` (`/*0210*/`) | `offsetof(Inner2, value6) = 8` (`+0x08`) | `value3` (offset 8) |

In every case, the driver compiler evaluated only the local offset within the target member's immediate parent struct, discarding all enclosing parent struct offsets in the access chain hierarchy.

### 4. Host Verification Logic

The host application fills the input buffer with sequential test values:

```cpp
// descriptor_heap_layout.cpp: lines 64-77
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

// descriptor_heap_layout.cpp: lines 976-988
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

After dispatch and execution, the output is verified:

```cpp
// descriptor_heap_layout.cpp: lines 1800-1823
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
        std::printf("all ok for \"%s\"\n", name);
        return;
    }
    if (allShifted) {
        std::printf("failed: mismatch pattern like offset of inner struct is ignored for \"%s\"\n", name);
        return;
    }
    std::printf("failed: unknown mismatch pattern for \"%s\"\n", name);
}
```

Because the shader reads from offsets `+0x00`, `+0x04`, and `+0x08`, the output buffer receives `{value2 = 1, value3 = 2, value6 = 3}` instead of `{2, 3, 6}`, triggering the `allShifted` match condition.

### 5. Vulkan API Validation Layers

Vulkan validation layers are enabled by default.

The debug callback is configured with a strict policy: any validation layer message (including warnings) immediately terminates the application via an assertion failure:

```cpp
#define VALIDATION_LAYERS_ENABLED 1
#define EXIT_CODE_ON_ANY_VALIDATION_LAYER_ERROR 42

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

VkBool32 ValidationLayerDebugCallback(
    const VkDebugUtilsMessageSeverityFlagBitsEXT /*messageSeverity*/,
    const VkDebugUtilsMessageTypeFlagsEXT /*messageType*/,
    const VkDebugUtilsMessengerCallbackDataEXT *const pCallbackData,
    void *const /*pUserData */
) {
    ASSERT(false, EXIT_CODE_ON_ANY_VALIDATION_LAYER_ERROR, "Validation Layer: %s\n", pCallbackData->pMessage);
    return VK_TRUE;
}
```

Both test executables complete execution with zero validation warnings or errors.

---

## Workaround (`shader_mapping`)

If the shader is authored with standard descriptor set bindings:

```glsl
// shaders/shader_mapping/failing.comp
layout (set = 0, binding = 0) buffer InputBlock {
    Input inputValues[];
} InputBuffers;

layout (set = 0, binding = 1) buffer OutputBlock {
    Output outputValues[];
} OutputBuffers;
```

and mapped at pipeline creation via `VkShaderDescriptorSetAndBindingMappingInfoEXT` with `VK_DESCRIPTOR_MAPPING_SOURCE_HEAP_WITH_PUSH_INDEX_EXT`:

```cpp
// shader_mapping.cpp: lines 1956-1979
const VkDescriptorMappingSourcePushIndexEXT descriptorMappingSourcePushIndexBinding0{
    .heapOffset = heapOffset,
    .pushOffset = binding0PushIndexOffset, // 0
    .heapIndexStride = heapIndexStride,
    .heapArrayStride = heapArrayStride,
    // ...
};
const VkDescriptorMappingSourcePushIndexEXT descriptorMappingSourcePushIndexBinding1{
    .heapOffset = heapOffset,
    .pushOffset = binding1PushIndexOffset, // 4
    .heapIndexStride = heapIndexStride,
    .heapArrayStride = heapArrayStride,
    // ...
};
```

the compiler lowers the SPIR-V through standard storage buffer access chains instead of `OpBufferPointerEXT`.

In `shaders_preview/shader_mapping/working.sass` and `failing.sass`, the resulting SASS is 100% identical and uses the correct offsets:

```sass
/*00d0*/  IADD3 R2, R1, 0x4, RZ ;   // value2 (offset +4)
...
/*0120*/  IADD3 R0, R1, 0x8, RZ ;   // value3 (offset +8)
...
/*01c0*/  IADD3 R1, R1, 0x14, RZ ;  // value6 (offset +20 / 0x14)
```

---

## Executables

The build produces two standalone executables:

1. **`descriptor_heap_layout`**:
   - Uses `layout(descriptor_heap)` in GLSL.
   - Output:
     ```text
     all ok for "no inner struct"
     failed: mismatch pattern like offset of inner struct is ignored for "with inner struct"
     ```

2. **`shader_mapping`**:
   - Uses `VkShaderDescriptorSetAndBindingMappingInfoEXT`.
   - Output:
     ```text
     all ok for "no inner struct"
     all ok for "with inner struct"
     ```

---

## Dependencies

- **Platform**: Windows only (the test runner relies on Win32 module APIs: `LoadLibraryA`, `GetProcAddress`)
- C++20 compiler (tested with MSVC 19.44 / Visual Studio 2022)
- CMake 3.31+
- Git (used by `FetchContent` to retrieve `zstd` v1.5.6)
- Vulkan SDK 1.3+ / 1.4+ (headers, loader, `glslc`, `spirv-dis`, `spirv-val`)
- Optional: CUDA Toolkit (`nvdisasm`) for `.cubin` SASS disassembly

### Extension Requirements

- **`VK_EXT_descriptor_heap`** (*Required*):
  Must be supported by the device and driver; the application asserts if unsupported.
- **`VK_KHR_pipeline_binary`** (*Optional*):
  Used to extract the pipeline binary, decompress the container via `zstd`, and dump `.cubin` files. If unsupported by the driver, buffer verification runs normally, but `.cubin` extraction is skipped.

---

## Build and Execution

### Configure and Build

```bash
# Configure
cmake -S . -B build

# Build (triggers glslc compilation, spirv-val validation, and spirv-dis disassembly)
cmake --build build --config Debug

# Run
./build/Debug/descriptor_heap_layout.exe
./build/Debug/shader_mapping.exe
```

### Disassembling to SASS (Optional)

If `nvdisasm` is available, pass its path and the target SM architecture during configuration:

```bash
cmake -S . -B build \
  -DNVDISASM_EXECUTABLE="C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v13.2/bin/nvdisasm.exe" \
  -DCUDA_ARCH="SM86"

cmake --build build --target disasm_cubin
```

### Cleaning Generated Binaries

To remove all generated `.spv`, `.spv.disasm`, `.cubin`, and `.sass` files from `shaders/`:

```bash
cmake --build build --target clean_binaries
```

*Note: `shaders_preview/` is not modified by this target.*

---

## CMake Configuration Variables

| Variable | Description |
|---|---|
| `Vulkan_GLSLC_EXECUTABLE` | Path to `glslc` compiler executable |
| `SPIRV_VAL_EXECUTABLE` | Path to `spirv-val` validator executable |
| `SPIRV_DIS_EXECUTABLE` | Path to `spirv-dis` disassembler executable |
| `NVDISASM_EXECUTABLE` | Path to `nvdisasm` executable |
| `CUDA_ARCH` | Target SM architecture flag for `nvdisasm -b` (default: `SM86`) |

---

## NVIDIA GPU Architecture to `CUDA_ARCH` Reference

| Architecture | Target SM (`CUDA_ARCH`) | GPU Series / Models |
|---|---|---|
| **Maxwell** | `SM50`, `SM52`, `SM53` | GeForce GTX 750 (Ti), GTX 900 series (960, 970, 980, 980 Ti, Titan X) |
| **Pascal** | `SM60`, `SM61`, `SM62` | GeForce GTX 1000 series (1050, 1060, 1070, 1080, 1080 Ti, Titan Xp) |
| **Volta** | `SM70`, `SM72` | Titan V, Quadro GV100, Tesla V100 |
| **Turing** | `SM75` | GeForce RTX 2000 series (2060–2080 Ti), GTX 1600 series (1650–1660 Ti), Titan RTX |
| **Ampere** | `SM86` | GeForce RTX 3000 series (3050–3090 Ti), RTX A2000–A6000 |
| **Ampere (DC)** | `SM80`, `SM87` | NVIDIA A100, Drive Orin |
| **Ada Lovelace** | `SM89` | GeForce RTX 4000 series (4060–4090), RTX 4000–6000 Ada Generation |
| **Hopper** | `SM90`, `SM90a` | NVIDIA H100, H200, GH200 Grace Hopper |
| **Blackwell** | `SM100`, `SM120` | GeForce RTX 5000 series (5070–5090), NVIDIA B100, B200, GB200 |

---

## Environment and Tested Driver Versions

- **OS**: Windows 11 IoT Enterprise 22H2 (Windows only)
- **GPU**: NVIDIA GeForce RTX 3060 12 GB (Ampere, `SM86`)
- **Vulkan SDK**: 1.4.363.0
- **CUDA Toolkit**: 13.2
- **Driver Versions Tested**: Every NVIDIA Game Ready Driver release starting from **610.88** up to and including **621.1.0** was tested; the behavior reproduces consistently across all versions in this range.
