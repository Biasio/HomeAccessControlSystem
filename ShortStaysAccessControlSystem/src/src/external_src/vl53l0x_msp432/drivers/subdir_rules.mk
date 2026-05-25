################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
src/external_src/vl53l0x_msp432/drivers/%.obj: ../src/external_src/vl53l0x_msp432/drivers/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"/Applications/ti/ccs1280/ccs/tools/compiler/ti-cgt-arm_20.2.7.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 -me -Ooff --fp_mode=strict --include_path="/private/tmp/EmbeddedHomeAccessControlSystem/simplelink_msp432p4_sdk_3_40_01_02/source/ti/drivers" --include_path="/private/tmp/EmbeddedHomeAccessControlSystem/ShortStaysAccessControlSystem/src/external_src/vl53l0x_msp432/drivers" --include_path="/private/tmp/EmbeddedHomeAccessControlSystem/simplelink_msp432p4_sdk_3_40_01_02/source/ti/devices/msp432p4xx/driverlib" --include_path="/private/tmp/EmbeddedHomeAccessControlSystem/simplelink_msp432p4_sdk_3_40_01_02/source/ti/devices/msp432p4xx" --include_path="/Applications/ti/ccs1280/ccs/ccs_base/arm/include" --include_path="/private/tmp/EmbeddedHomeAccessControlSystem/simplelink_msp432p4_sdk_3_40_01_02/source/ti/posix/ccs" --include_path="/Applications/ti/ccs1280/ccs/ccs_base/arm/include/CMSIS" --include_path="/Applications/ti/ccs1280/ccs/tools/compiler/ti-cgt-arm_20.2.7.LTS/include" --include_path="/private/tmp/EmbeddedHomeAccessControlSystem/ShortStaysAccessControlSystem" --include_path="/private/tmp/EmbeddedHomeAccessControlSystem/simplelink_msp432p4_sdk_3_40_01_02/source" --include_path="/private/tmp/EmbeddedHomeAccessControlSystem/simplelink_msp432p4_sdk_3_40_01_02/examples/nortos/MSP_EXP432P401R/demos/boostxl_edumkii_lightsensor_msp432p401r/LcdDriver" --advice:power="all" --define=MSP_EXP432P401R --define=MSP432P401R --define=DeviceFamily_MSP432P4x1x --define=__MSP432P401R__ --define=ccs -g --c99 --gcc --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="src/external_src/vl53l0x_msp432/drivers/$(basename $(<F)).d_raw" --obj_directory="src/external_src/vl53l0x_msp432/drivers" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


