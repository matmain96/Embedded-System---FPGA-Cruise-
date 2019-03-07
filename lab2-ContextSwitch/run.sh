#!/bin/bash
# @file: run.sh
# @author: George Ungureanu, KTH/EECS/ELE
# @date: 21.08.2018
# @version: 0.1
#
# This is a bash script for automating the compilation and deployment
# of the Nios II project in the current folder. It is a more readable
# (albeit less powerful) version of the 'Makefile' one folder
# above. It is recommended for beginner students to understand what is
# happening during the compilation process.

# Paths for DE2-35 sources
CORE_FILE=/home/student/Desktop/ht_18_mainenti_lezeta.sopcinfo
SOF_FILE=/home/student/Desktop/synthesis/output_files/ht18_mainenti_lezeta.sof
JDI_FILE=../../../synthesis/output_files/ht18_mainenti_lezeta.jdi
# Paths for DE2-115 sources
# CORE_FILE=../../hardware/DE2-115-pre-built/DE2_115_Nios2System.sopcinfo
# SOF_FILE=../../hardware/DE2-115-pre-built/IL2206_DE2_115_Nios2.sof
# JDI_FILE=../../hardware/DE2-115-pre-built/IL2206_DE2_115_Nios2.jdi

APP_NAME=ContestSwitch
CPU_NAME=nios2_ht18_mainenti_lezeta
BSP_PATH=../../bsp/il2206-pre-built-ucosii
SRC_PATH=./src

# Project internal folders
GEN=gen
BIN=bin
mkdir -p $GEN
mkdir -p $BIN

echo -e "\n******************************************"
echo -e   "Building the BSP and compiling the program"
echo -e   "******************************************\n"

nios2-bsp ucosii $BSP_PATH $CORE_FILE \
      --cpu-name $CPU_NAME \
      --default_sections_mapping sram \
      --set hal.sys_clk_timer timer_0 \
	  --set hal.timestamp_timer timer_1 \
      --set hal.make.bsp_cflags_debug -g \
      --set hal.make.bsp_cflags_optimization -Os \
      --set hal.enable_sopc_sysid_check 1 \
      --set ucosii.os_tmr_en 1

cd $GEN
nios2-app-generate-makefile \
    --bsp-dir ../$BSP_PATH \
    --elf-name ../$BIN/$APP_NAME.elf \
    --src-dir ../$SRC_PATH \
    --set APP_CFLAGS_OPTIMIZATION -O0

make 3>&1 1>>log.txt 2>&1
cd ..

echo -e "\n**************************"
echo -e  "Download hardware to board"
echo -e  "**************************\n"

nios2-configure-sof $SOF_FILE

echo -e "\n**************************"
echo -e   "Download software to board"
echo -e   "**************************\n"
cd $BIN
xterm -e "nios2-terminal -i 0" &
nios2-download -g $APP_NAME.elf --cpu_name $CPU_NAME --jdi $JDI_FILE

echo ""
echo "Code compilation errors are logged in 'log.txt'"
