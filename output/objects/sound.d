output\objects\sound.o: source\PE\sound.c

:cmdList=ccrh850 -c  -MD -Isource\PE -Idevice -G -object_dir=output\objects -list -list_dir=output/list -passsource -bsp generic -sda=all -fsoft -cpu=rh850 -ansi -filetype.c source\PE\sound.c -o output\objects\sound.o ; 
:cmdHash=0x05e70b87

:installDir=c:\ghs\comp_202354
:installDirHash=0x9d2c864f
