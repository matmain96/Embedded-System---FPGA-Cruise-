APP_NAME := cruise_skeleton

# paths
# DE2-35 sources
CORE_FILE := /home/student/Desktop/ht_18_mainenti_lezeta.sopcinfo
SOF_FILE := /home/student/Desktop/synthesis/output_files/ht18_mainenti_lezeta.sof
JDI_FILE := ../../../synthesis/output_files/ht18_mainenti_lezeta.jdi
# DE2-115 sources
# CORE_FILE := ../../hardware/DE2-115-pre-built/DE2_115_Nios2System.sopcinfo
# SOF_FILE  := ../../hardware/DE2-115-pre-built/IL2206_DE2_115_Nios2.sof
# JDI_FILE  := ../../hardware/DE2-115-pre-built/IL2206_DE2_115_Nios2.jdi

BSP_PATH  := ../../bsp/il2206-pre-built-ucosii
SRC_PATH := src
GEN_PATH := gen
BIN_PATH := bin

# component names
CPU_NAME := nios2_ht18_mainenti_lezeta
BSP_TYPE := ucosii

# commands
NIOS2_BSP_COMMANDS := \
      --default_sections_mapping sram \
      --set hal.sys_clk_timer timer_0 \
      --set hal.make.bsp_cflags_debug -g \
      --set hal.make.bsp_cflags_optimization -Os \
      --set hal.enable_sopc_sysid_check 1 \
      --set ucosii.os_tmr_en 1

MAKEFILE_COMMANDS := --set APP_CFLAGS_OPTIMIZATION -O0

include ../Makefile.in
