#!/bin/bash

# These generated files are required by the vendored Mesa sources but omitted
# from the upstream standalone Makefile's GENERATED list.
python3 src/util/format/u_format_table.py src/util/format/u_format.yaml --enums > src/util/format/u_format_gen.h
python3 src/util/format/u_format_table.py src/util/format/u_format.yaml --header > src/util/format/u_format_pack.h
python3 src/util/format/u_format_table.py src/util/format/u_format.yaml > src/util/format/u_format_table.c
python3 src/util/format_srgb.py > src/util/format_srgb.c
python3 src/compiler/builtin_types_h.py src/compiler/builtin_types.h
python3 src/compiler/builtin_types_c.py src/compiler/builtin_types.c
python3 src/util/process_shader_stats.py src/util/shader_stats.rnc src/util/shader_stats.xml > src/util/shader_stats.h
python3 src/vulkan/util/vk_struct_type_cast_gen.py \
    --xml src/vulkan/registry/vk.xml \
    --out src/vulkan/util/vk_struct_type_cast.h \
    --beta false
python3 src/amd/packets/parse_cp_pm4_table_data_json.py \
    src/amd/packets/cp_pm4_table_data_gfx11.json \
    src/amd/packets/pm4_it_opcodes_gfx11.h \
    src/amd/packets/cp_pm4_table_data_gfx12.json \
    src/amd/packets/pm4_it_opcodes_gfx12.h \
    gfx11 packets_h > src/amd/common/amd_cp_packets_gfx11.h
python3 src/amd/packets/parse_cp_pm4_table_data_json.py \
    src/amd/packets/cp_pm4_table_data_gfx11.json \
    src/amd/packets/pm4_it_opcodes_gfx11.h \
    src/amd/packets/cp_pm4_table_data_gfx12.json \
    src/amd/packets/pm4_it_opcodes_gfx12.h \
    gfx12 packets_h > src/amd/common/amd_cp_packets_gfx12.h
python3 src/amd/common/gfx10_format_table.py \
    src/util/format/u_format.yaml \
    src/amd/registers/gfx10-rsrc.json \
    src/amd/registers/gfx11-rsrc.json > src/amd/common/gfx10_format_table.c
