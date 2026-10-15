/*
 * libpsbc — SPIR-V to PS4/PS5 Shader Binary Compiler Library
 *
 * Extracted from cmd/psbc/main.c to provide a reusable C API for
 * compiling SPIR-V shaders into GCN shader binaries with GnmShaderFileHeader.
 *
 * Used by:
 *   - opengnm-psbc CLI (cmd/psbc/main.c)
 *   - vulkan-ps4 ICD (vkCreateShaderModule)
 */

#ifndef PSBC_COMPILE_H
#define PSBC_COMPILE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PSBC_SHADER_METADATA_VERSION 14u

struct nir_shader;
struct nir_shader_compiler_options;

/* === Types === */

typedef enum {
    PSBC_TARGET_PS4_BASE = 0,   /* GFX7 (Sea Islands) */
    PSBC_TARGET_PS4_NEO  = 1,   /* GFX8 (Polaris) */
    PSBC_TARGET_PS5      = 2,   /* GFX10.3 (RDNA2) */
} PsbcTarget;

typedef enum {
    PSBC_STAGE_NONE       = 0,
    PSBC_STAGE_VERTEX     = 1,
    PSBC_STAGE_TESS_CTRL  = 2,
    PSBC_STAGE_TESS_EVAL  = 3,
    PSBC_STAGE_GEOMETRY   = 4,
    PSBC_STAGE_FRAGMENT   = 5,
    PSBC_STAGE_COMPUTE    = 6,
    PSBC_STAGE_TASK       = 7,
    PSBC_STAGE_EXPORT     = 8,  /* VS as ES (Export Shader) */
    PSBC_STAGE_LOCAL      = 9,  /* VS as LS (Local Shader) */
} PsbcStage;

typedef enum {
    PSBC_RESULT_OK = 0,
    PSBC_RESULT_INVALID_SPIRV,
    PSBC_RESULT_UNSUPPORTED_STAGE,
    PSBC_RESULT_COMPILE_NIR,
    PSBC_RESULT_COMPILE_ACO,
    PSBC_RESULT_OUT_OF_MEMORY,
    PSBC_RESULT_INTERNAL_ERROR,
    PSBC_RESULT_INVALID_ARGUMENT,
} PsbcResult;

#define PSBC_MAX_CONTEXT_REGISTERS 16
#define PSBC_MAX_SHADER_REGISTERS 8
#define PSBC_MAX_SEMANTICS 32
/* Private matching key shared by our AGC producer/consumer packages, after
 * the generic keys 15..46. This is not a PSSL system-semantic enum. */
#define PSBC_SEMANTIC_PRIMITIVE_ID 47u
#define PSBC_SEMANTIC_POINT_COORD 48u /* Generated input, no producer export. */
#define PSBC_MAX_VERTEX_ATTRIBUTES 32
#define PSBC_MAX_DESCRIPTOR_BINDINGS 128
/* Reserve distinct 16-sampler banks for merged Gallium vertex/geometry stages. */
#define PSBC_GALLIUM_UBO_BINDING_BASE 32
/* Opt-in stage-local descriptor arrays. Keys 4*(stage-1)+0/3 are reserved
 * for sampled/storage images; buffer lowering uses +1/+2. Atomic counters
 * lowered by Mesa share the SSBO array and must be budgeted in its length.
 * These keys do not limit resource counts: array_size describes each bank. */
#define PSBC_GALLIUM_UBO_ARRAY_BINDING(stage) (4u * ((unsigned)(stage) - 1u) + 1u)
#define PSBC_GALLIUM_SSBO_ARRAY_BINDING(stage) (4u * ((unsigned)(stage) - 1u) + 2u)
#define PSBC_GALLIUM_IMAGE_ARRAY_BINDING(stage) (4u * ((unsigned)(stage) - 1u) + 3u)

typedef enum {
    PSBC_VERTEX_FORMAT_NONE = 0,
    PSBC_VERTEX_FORMAT_R32_FLOAT,
    PSBC_VERTEX_FORMAT_R32G32_FLOAT,
    PSBC_VERTEX_FORMAT_R32G32B32_FLOAT,
    PSBC_VERTEX_FORMAT_R32G32B32A32_FLOAT,
    PSBC_VERTEX_FORMAT_B8G8R8A8_UNORM,
    PSBC_VERTEX_FORMAT_R10G10B10A2_UNORM,
    PSBC_VERTEX_FORMAT_B10G10R10A2_UNORM,
    PSBC_VERTEX_FORMAT_R10G10B10A2_SNORM,
    PSBC_VERTEX_FORMAT_B10G10R10A2_SNORM,
    PSBC_VERTEX_FORMAT_R10G10B10A2_USCALED,
    PSBC_VERTEX_FORMAT_B10G10R10A2_USCALED,
    PSBC_VERTEX_FORMAT_R10G10B10A2_SSCALED,
    PSBC_VERTEX_FORMAT_B10G10R10A2_SSCALED,
    PSBC_VERTEX_FORMAT_R32_SINT,
    PSBC_VERTEX_FORMAT_R32G32_SINT,
    PSBC_VERTEX_FORMAT_R32G32B32_SINT,
    PSBC_VERTEX_FORMAT_R32G32B32A32_SINT,
    PSBC_VERTEX_FORMAT_R32_UINT,
    PSBC_VERTEX_FORMAT_R32G32_UINT,
    PSBC_VERTEX_FORMAT_R32G32B32_UINT,
    PSBC_VERTEX_FORMAT_R32G32B32A32_UINT,
    PSBC_VERTEX_FORMAT_R8G8B8A8_UNORM,
    PSBC_VERTEX_FORMAT_R64_FLOAT,
    PSBC_VERTEX_FORMAT_R64G64_FLOAT,
    PSBC_VERTEX_FORMAT_R64G64B64_FLOAT,
    PSBC_VERTEX_FORMAT_R64G64B64A64_FLOAT,
    PSBC_VERTEX_FORMAT_R11G11B10_FLOAT,
    PSBC_VERTEX_FORMAT_R8G8B8A8_SNORM,
    PSBC_VERTEX_FORMAT_R16G16_UNORM,
    PSBC_VERTEX_FORMAT_R16G16_FLOAT,
    PSBC_VERTEX_FORMAT_R16G16B16A16_FLOAT,
    PSBC_VERTEX_FORMAT_R16G16_SNORM,
    PSBC_VERTEX_FORMAT_R8_UINT,
    PSBC_VERTEX_FORMAT_R8G8_UINT,
    PSBC_VERTEX_FORMAT_R8G8B8A8_UINT,
    PSBC_VERTEX_FORMAT_R8G8_UNORM,
    PSBC_VERTEX_FORMAT_R8G8_SNORM,
} PsbcVertexFormat;

typedef struct {
    uint8_t          location;
    uint8_t          binding;
    PsbcVertexFormat format;
    uint32_t         offset;
    uint32_t         stride; /* PS5: zero repeats a current/constant attribute. */
    uint32_t         alignment;
    uint32_t         instance_divisor;
} PsbcVertexAttribute;

typedef enum {
    PSBC_DESCRIPTOR_NONE = 0,
    PSBC_DESCRIPTOR_UNIFORM_BUFFER,
    PSBC_DESCRIPTOR_COMBINED_IMAGE_SAMPLER,
    PSBC_DESCRIPTOR_STORAGE_BUFFER,
    PSBC_DESCRIPTOR_STORAGE_IMAGE,
} PsbcDescriptorType;

typedef struct {
    uint8_t            set;
    uint8_t            binding;
    PsbcDescriptorType type;
    uint32_t           array_size;
    uint32_t           offset;
    uint32_t           stride;
} PsbcDescriptorBinding;

typedef enum {
    PSBC_HW_STAGE_UNKNOWN = 0,
    PSBC_HW_STAGE_VERTEX  = 1,
    PSBC_HW_STAGE_PIXEL   = 2,
    PSBC_HW_STAGE_NGG     = 3,
    PSBC_HW_STAGE_COMPUTE = 4,
    PSBC_HW_STAGE_HULL    = 5,
} PsbcHardwareStage;

typedef enum {
    PSBC_UNRESOLVED_NONE                   = 0,
    PSBC_UNRESOLVED_PROGRAM_CHECKSUM       = 1u << 0,
    PSBC_UNRESOLVED_NGG_ESGS_RING_ITEMSIZE = 1u << 1,
    PSBC_UNRESOLVED_AGC_LINKAGE             = 1u << 2,
} PsbcUnresolvedField;

typedef struct {
    uint16_t offset;
    uint16_t padding;
    uint32_t value;
} PsbcRegisterWrite;

typedef struct {
    uint32_t             version;
    PsbcTarget           target;
    PsbcStage            source_stage;
    PsbcHardwareStage    hardware_stage;
    uint32_t             unresolved_fields;
    uint32_t             context_register_count;
    PsbcRegisterWrite    context_registers[PSBC_MAX_CONTEXT_REGISTERS];
    uint32_t             shader_register_count;
    PsbcRegisterWrite    shader_registers[PSBC_MAX_SHADER_REGISTERS];
    bool                 linkage_valid;
    PsbcRegisterWrite    linkage_ge_cntl;
    PsbcRegisterWrite    linkage_stages_en;
    PsbcRegisterWrite    linkage_user_vgpr_en;
    uint32_t             input_semantic_count;
    uint32_t             input_semantics[PSBC_MAX_SEMANTICS];
    uint32_t             output_semantic_count;
    uint32_t             output_semantics[PSBC_MAX_SEMANTICS];
    uint32_t             clip_distance_mask;
    uint32_t             cull_distance_mask;
    uint32_t             address32_hi;
    uint32_t             user_sgpr_count;
    bool                 vertex_buffer_table_valid;
    uint32_t             vertex_buffer_table_user_data_dword;
    bool                 descriptor_set0_valid;
    uint32_t             descriptor_set0_user_data_dword;
    uint32_t             descriptor_binding_count;
    PsbcDescriptorBinding descriptor_bindings[PSBC_MAX_DESCRIPTOR_BINDINGS];
    bool                 base_vertex_valid;
    uint32_t             base_vertex_user_data_dword;
    bool                 is_indexed_draw_valid;
    uint32_t             is_indexed_draw_user_data_dword;
    bool                 start_instance_valid;
    uint32_t             start_instance_user_data_dword;
    bool                 instance_id_bias_valid;
    uint32_t             instance_id_bias_user_data_dword;
    bool                 primitive_query_valid;
    uint32_t             primitive_query_buffer_user_data_dword;
    uint32_t             primitive_query_state_user_data_dword;
    uint32_t             primitive_query_enable_mask;
    uint32_t             primitive_query_counter_offset;
    bool                 streamout_valid;
    uint32_t             streamout_buffer_table_user_data_dword;
    uint32_t             streamout_enabled_stream_buffers_mask;
    uint32_t             streamout_strides_dwords[4];
    uint32_t             streamout_config_sgpr;
    uint32_t             streamout_write_index_sgpr;
    uint32_t             streamout_offset_sgprs[4];
    bool                 scratch_valid;
    bool                 scratch_buffer_backed; /* MUBUF spills; false means flat scratch. */
    uint32_t             scratch_bytes_per_wave;
    uint32_t             scratch_size_per_thread;
    uint32_t             scratch_buffer_table_user_data_dword;
    bool                 ngg_lds_layout_valid;
    uint32_t             ngg_lds_layout_user_data_dword;
    uint32_t             ngg_lds_layout; /* GS output base in bytes, after ES inputs. */
    uint32_t             compute_workgroup_size[3];
    uint32_t             compute_wave_size;
    uint32_t             compute_lds_bytes; /* Final compiler allocation, not source shared size. */
    bool                 compute_grid_size_valid;
    uint32_t             compute_grid_size_user_data_dword; /* Three direct uint32 values. */
    uint32_t             compute_private_stride; /* Ordinary storage; reserved user words 0/1. */
} PsbcShaderMetadata;

typedef struct {
    void*               data;         /* Legacy PSSL/GNM wrapper */
    size_t              size;
    void*               machine_code; /* Raw ACO machine code; no wrapper/trailer */
    size_t              machine_code_size;
    PsbcShaderMetadata  metadata;
} PsbcShaderOutput;

typedef struct {
    PsbcTarget  target;
    PsbcStage   stage;
    const char* entrypoint;  /* Default: "main" */
    bool        optimise;    /* Default: true */
    bool        ngg;         /* Experimental PS5 VS->FS NGG lowering */
    bool        omit_implicit_primitive_id; /* Caller proves FS does not read it; false if unknown. */
    bool        primitive_id_per_primitive; /* FS consumes an implicit PS5 NGG VS export, not a GS varying. */
    bool        ps5_global_streamout; /* No-GDS NGG GS/linked TES counters */
    bool        ps5_global_primitive_query; /* GPU primitive counts without XFB. */
    bool        split_vertex_instances; /* Standalone VS: runtime supplies logical instance ID bias. */
    bool        force_accelerated_dot; /* Diagnostic only: emit native dot ISA */
    uint32_t    primitive_type; /* GFX10 DI primitive type: 0, 1..6, 10..13 */
    bool        provoking_vtx_last; /* Select the final flat-shaded vertex */
    bool        flat_input_vertex_valid;
    uint8_t     flat_input_vertex;
    uint32_t    address32_hi; /* Upper half for ACO 32-bit GPU pointers */
    uint32_t    vertex_attribute_count;
    PsbcVertexAttribute vertex_attributes[PSBC_MAX_VERTEX_ATTRIBUTES];
    uint32_t    descriptor_binding_count;
    PsbcDescriptorBinding descriptor_bindings[PSBC_MAX_DESCRIPTOR_BINDINGS];
    uint32_t    rasterization_samples; /* 0=single/default, otherwise 1/2/4/8 */
    uint32_t    ps_iter_samples;       /* Fragment invocations per pixel (min sample shading), 0/1=off */
    uint32_t    spi_shader_col_format; /* Per-MRT export nibbles; 0=legacy defaults */
    uint32_t    color_is_int8;         /* Per-MRT narrow integer clamp masks */
    uint32_t    color_is_int10;
    /* Internal NIR only: scalar UBO/SSBO slots index the stage-local banks
     * above. Off preserves the existing flat UBO descriptor layout. */
    bool        gallium_buffer_arrays;
    bool        compute_private_buffer; /* Opt-in scalar32 explicit private lowering; not spills. */
    bool        compute_buffer_spills; /* Opt-in GFX10 MUBUF register spills. */
} PsbcCompileOptions;

/* Compiler-model inputs, not a native tessellation allocation contract. */
typedef struct {
    uint32_t input_patch_vertices;
    uint32_t offchip_workgroup_capacity_dwords;
    uint32_t address32_hi;
    /* Vertex inputs plus a shared set-0 layout. With gallium_buffer_arrays,
     * UBO/SSBO banks use stage-specific bindings for VS/TCS/TES/optional GS. */
    PsbcCompileOptions vertex;
} PsbcTessellationCompileOptions;

/* Fixed pipeline state proven by the linked compiler. Ring totals remain a
 * device allocation decision; the sizes here are per hardware workgroup. */
typedef struct {
    uint32_t valid;
    uint32_t input_patch_vertices;
    uint32_t output_patch_vertices;
    uint32_t num_patches;
    uint32_t lds_bytes;
    uint32_t hs_rsrc2;
    uint32_t ls_hs_config;
    uint32_t tf_param;
    uint32_t hs_ring_offsets_sgpr;
    uint32_t tes_ring_offsets_sgpr;
    uint32_t hs_ring_offsets_register;
    uint32_t tes_ring_offsets_register;
    uint32_t final_offchip_layout_valid;
    uint32_t final_offchip_layout_user_data_dword;
    uint32_t final_offchip_layout;
    uint32_t offchip_ring_bytes_per_workgroup;
    uint32_t tess_factor_ring_bytes_per_workgroup;
} PsbcTessellationRuntimeInfo;

typedef struct {
    PsbcShaderOutput hs;  /* Raw merged VS+TCS code; no native wrapper/linkage. */
    PsbcShaderOutput tes; /* Raw TES/NGG code; no native wrapper/linkage. */
    PsbcTessellationRuntimeInfo runtime;
} PsbcTessellationOutput;

/* === API === */

/*
 * Initialize the psbc library. Must be called once before any compilation.
 * Refcounted — call psbc_shutdown() once per psbc_init() when done.
 * Performs Mesa one-time init (glsl_type_singleton, etc.).
 */
void psbc_init(void);

/*
 * Shutdown the psbc library. Decrements refcount, cleans up when it reaches 0.
 */
void psbc_shutdown(void);

/* Immutable PS5 ACO NIR options for Gallium/front-end shader creation. */
const struct nir_shader_compiler_options*
psbc_get_nir_options(PsbcStage stage);

/*
 * Compile a SPIR-V shader into a PS4/PS5 shader binary.
 *
 *   spirv       — pointer to SPIR-V bytecode
 *   spirv_size  — size of SPIR-V bytecode in bytes
 *   opts        — compilation options (target, stage, entrypoint, etc.)
 *   out         — receives the legacy wrapper, raw code, and typed metadata;
 *                 release all owned memory with psbc_free_output()
 *
 * The standalone builder emits GNM wrappers for all represented stages.
 * On GFX9+, a standalone geometry result is a compiler diagnostic only;
 * a loadable geometry pipeline also requires a merged pre-raster stage and,
 * for legacy geometry, a separately compiled copy shader.
 *
 * Returns PSBC_RESULT_OK on success, error code otherwise.
 * On error, all output fields are zero.
 */
PsbcResult psbc_compile_shader(
    const uint32_t*       spirv,
    size_t                spirv_size,
    const PsbcCompileOptions* opts,
    PsbcShaderOutput*     out
);

/*
 * Compile an existing NIR shader.  The input remains owned by the caller;
 * libpsbc clones it before optimization and target lowering.  This is the
 * Gallium-facing path and requires opts->stage to match nir->info.stage.
 */
PsbcResult psbc_compile_nir(
    const struct nir_shader* nir,
    const PsbcCompileOptions* opts,
    PsbcShaderOutput* out
);

/* Compile a complete PS5 NGG vertex+geometry pair into one hardware shader. */
PsbcResult psbc_compile_geometry_pipeline(
    const uint32_t* vertex_spirv,
    size_t vertex_spirv_size,
    const uint32_t* geometry_spirv,
    size_t geometry_spirv_size,
    const PsbcCompileOptions* opts,
    PsbcShaderOutput* out
);

/* NIR equivalent used by Gallium when a geometry shader is bound. */
PsbcResult psbc_compile_nir_geometry_pipeline(
    const struct nir_shader* vertex_nir,
    const struct nir_shader* geometry_nir,
    const PsbcCompileOptions* opts,
    PsbcShaderOutput* out
);

/* Compiler-only GFX10.3 VS+TCS/TES, initially 3-in/3-out triangles, equal
 * spacing, ccw, non-point mode, no external resources/vertex attributes/XFB.
 * Valid NIR is required. Modes may be specified in either TCS or TES, following
 * Mesa's merge rules. All three inputs are borrowed, including options pointers.
 * out must be initialized empty: success transfers BOTH code allocations;
 * failure leaves an empty out zero. A nonempty out is rejected unchanged.
 * The output includes fixed draw state and per-workgroup ring requirements;
 * the caller still owns device-wide ring allocation. No capacity defaults. */
PsbcResult psbc_compile_nir_tessellation_pipeline(
    const struct nir_shader* vs,
    const struct nir_shader* tcs,
    const struct nir_shader* tes,
    const struct nir_shader* gs,
    const PsbcTessellationCompileOptions* options,
    PsbcTessellationOutput* out
);

/* Null-safe; frees both outputs and zeroes the pair, so repeated calls are safe. */
void psbc_free_tessellation_output(PsbcTessellationOutput* out);

/*
 * Free a PsbcShaderOutput previously filled by psbc_compile_shader().
 */
void psbc_free_output(PsbcShaderOutput* out);

/*
 * Convert a PsbcResult to a human-readable string.
 */
const char* psbc_result_string(PsbcResult result);

/*
 * Convert a stage name string ("vertex", "fragment", etc.) to PsbcStage.
 * Returns PSBC_STAGE_NONE if the name is not recognized.
 */
PsbcStage psbc_stage_from_name(const char* name);

#ifdef __cplusplus
}
#endif

#endif /* PSBC_COMPILE_H */
