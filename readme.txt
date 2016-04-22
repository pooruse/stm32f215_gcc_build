************************************************************
about IGS STM32 bootloader:
************************************************************

                  _______________________ 0x8000000
                 |                       |    <- bootloader  (load bin file by uart1 or 2)
                 |_______________________|0x8004000
                 |_______________________|0x8005000
                 |                       |
                 |                       |
                 |                       |
                 |                       |
                 |                       |     <- app        (load bin file by uart1 or 2)
                 |                       |
                 |                       |
                 |                       |
                 |                       |
                 |                       |
                 |                       |
                 |_______________________|0x8080000

How to switch the code between bootloader support or not.				 
1. If you want test your MCU code without bootload, you need...
  a. comment IAP_Init() function in the main.
  b. set readonly memory address as 0x8000000 and size is 0x80000

2. If you need your code support the IGS stm32 bootloader, check the file "Bootloader 修改流程及解保護.pdf", please.


************************************************************
about version management:
************************************************************
please open the file IGS_STM32_IAP_APP.h at boot_driver\inc
find 
#define FirmwareVer	"MH_PCB01_V109\0\0\0\0\0\0\0" 
and change it, the firmware version is a 20 bytes data. 
if your string size is below 20 bytes, you should stuff it to 20 bytes with \0 (0x00)

