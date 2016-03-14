# TCL File Generated by Component Editor 10.1sp1
# Fri Mar 06 19:15:35 EST 2015
# DO NOT MODIFY


# +-----------------------------------
# | 
# | ycbcr_to_rgb2 "ycbcr_to_rgb2" v1.0
# | null 2015.03.06.19:15:35
# | 
# | 
# | C:/Users/c73chan/ECE423/Lab1/ycbcr_to_rgb.vhd
# | 
# |    ./ycbcr_to_rgb.vhd syn
# | 
# +-----------------------------------

# +-----------------------------------
# | request TCL package from ACDS 10.1
# | 
package require -exact sopc 10.1
# | 
# +-----------------------------------

# +-----------------------------------
# | module ycbcr_to_rgb2
# | 
set_module_property NAME ycbcr_to_rgb2
set_module_property VERSION 1.0
set_module_property INTERNAL false
set_module_property OPAQUE_ADDRESS_MAP true
set_module_property DISPLAY_NAME ycbcr_to_rgb2
set_module_property TOP_LEVEL_HDL_FILE ycbcr_to_rgb.vhd
set_module_property TOP_LEVEL_HDL_MODULE ycbcr_to_rgb
set_module_property INSTANTIATE_IN_SYSTEM_MODULE true
set_module_property EDITABLE true
set_module_property ANALYZE_HDL TRUE
# | 
# +-----------------------------------

# +-----------------------------------
# | files
# | 
add_file ycbcr_to_rgb.vhd SYNTHESIS
# | 
# +-----------------------------------

# +-----------------------------------
# | parameters
# | 
# | 
# +-----------------------------------

# +-----------------------------------
# | display items
# | 
# | 
# +-----------------------------------

# +-----------------------------------
# | connection point nios_custom_instruction_slave_0
# | 
add_interface nios_custom_instruction_slave_0 nios_custom_instruction end
set_interface_property nios_custom_instruction_slave_0 clockCycle 0
set_interface_property nios_custom_instruction_slave_0 operands 2

set_interface_property nios_custom_instruction_slave_0 ENABLED true

add_interface_port nios_custom_instruction_slave_0 dataa dataa Input 32
add_interface_port nios_custom_instruction_slave_0 datab datab Input 32
add_interface_port nios_custom_instruction_slave_0 result result Output 32
# | 
# +-----------------------------------
