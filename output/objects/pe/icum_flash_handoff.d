output\objects\pe\icum_flash_handoff.o: source\PE\icum_flash_handoff.c \
 device\device.h device\dr7f701649.dvf.h device\icu_feret.h device\cpu.h \
 C:\ghs\comp_202354\ansi\..\include\v800\v800_ghs.h \
 source\PE\icum_flash_handoff.h source\icum_flash_handoff_protocol.h

:cmdList=ccrh850 -c  -MD -Isource\PE -Isource -Idevice -G -object_dir=output\objects\pe -list -list_dir=output/list/pe -passsource -bsp generic -sda=all -fsoft -cpu=rh850 -ansi -filetype.c source\PE\icum_flash_handoff.c -o output\objects\pe\icum_flash_handoff.o ; 
:cmdHash=0xfcc0a12d

:installDir=c:\ghs\comp_202354
:installDirHash=0x9d2c864f
