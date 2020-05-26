#####################################################################
# main AVR executable
#####################################################################
add_avr_executable(
   avr-gameport
   
   main
   hwinit
   joystick
   usb_descriptor
)

# #####################################################################
# # libraries to link to
# #####################################################################
target_link_libraries(
    avr-gameport.elf
    libVUsb
)
