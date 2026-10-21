output\objects\icum\icum_ostm.o: source\ICUM\icum_ostm.c device\device.h \
 device\dr7f701649.dvf.h device\icu_feret.h device\cpu.h \
 C:\ghs\comp_202354\ansi\..\include\v800\v800_ghs.h source\ICUM\icum_comm.h \
 source\ICUM\icum_ostm.h

:cmdList=ccrh850 -c  -MD -Isource\ICUM -Isource -Idevice -G -object_dir=output\objects\icum -list -list_dir=output/list/icum -passsource -bsp generic -sda=all -fsoft -cpu=rh850 -ansi -filetype.c source\ICUM\icum_ostm.c -o output\objects\icum\icum_ostm.o ; 
:cmdHash=0x3091c310

:installDir=c:\ghs\comp_202354
:installDirHash=0x9d2c864f
