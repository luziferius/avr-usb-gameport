#####################################################################
# VUSB library
#####################################################################
enable_language(ASM)

add_avr_library(
    libVUsb
    usbdrv
    usbdrvasm.S
    oddebug
)
