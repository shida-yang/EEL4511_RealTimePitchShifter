################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
Header/%.obj: ../Header/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C2000 Compiler'
	"D:/CCS/ccs/tools/compiler/ti-cgt-c2000_18.12.2.LTS/bin/cl2000" -v28 -ml -mt --cla_support=cla1 --float_support=fpu32 --tmu_support=tmu0 --vcu_support=vcu2 --include_path="E:/past_UF_HW/2019_fall_hw/EEE4511/Lab/Lab9/Final_Lab_CCS/Lab6" --include_path="D:/ti/c2000/C2000Ware_3_02_00_00/device_support/f2837xd/common/include" --include_path="D:/ti/c2000/C2000Ware_3_02_00_00/device_support/f2837xd/headers/include" --include_path="D:/ti/c2000/C2000Ware_3_02_00_00/driverlib/f2837xd/driverlib" --include_path="D:/UF_HW/2019_fall_hw/EEE4511/Lab/Lab5/additional_reference/ADC_TIMER_Driverlib/driverlib" --include_path="D:/CCS/ccs/tools/compiler/ti-cgt-c2000_18.12.2.LTS/include" --define=CPU1 --define=_DUAL_HEADERS --define=_LAUNCHXL_F28379D -g --c99 --diag_warning=225 --diag_wrap=off --display_error_number --abi=coffabi --preproc_with_compile --preproc_dependency="Header/$(basename $(<F)).d_raw" --obj_directory="Header" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


