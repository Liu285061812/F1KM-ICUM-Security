output\objects\sound.o: ..\PE\sound.c

:cmdList=ccrh850 -c  -MD -I..\..\device -G -object_dir=output\objects -list -list_dir=output/list -passsource -bsp generic -sda=all -fsoft -cpu=rh850 -ansi -filetype.c ..\PE\sound.c -o output\objects\sound.o ; 
:cmdHash=0x9dd968b6

:installDir=c:\ghs\comp_202354
:installDirHash=0x9d2c864f
