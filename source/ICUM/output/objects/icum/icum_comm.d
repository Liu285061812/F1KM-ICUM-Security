output\objects\icum\icum_comm.o: icum_comm.c icum_comm.h \
 ..\..\device\device.h ..\..\device\dr7f701649.dvf.h ..\..\device\icu_feret.h \
 ..\..\device\cpu.h C:\ghs\comp_202354\ansi\..\include\v800\v800_ghs.h \
 ..\shared_memory.h

:cmdList=ccrh850 -c  -MD -I. -I.. -I..\..\device -G -object_dir=output\objects\icum -list -list_dir=output/list/icum -passsource -bsp generic -sda=all -fsoft -cpu=rh850 -ansi -filetype.c icum_comm.c -o output\objects\icum\icum_comm.o ; 
:cmdHash=0xdb4cffd5

:installDir=c:\ghs\comp_202354
:installDirHash=0x9d2c864f
