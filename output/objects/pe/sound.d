output\objects\pe\sound.o: source\PE\sound.c

:cmdList=ccrh850 -c  -MD -Isource\PE -Idevice -G -object_dir=output\objects\pe -list -list_dir=output/list/pe -passsource -bsp generic -sda=all -fsoft -cpu=rh850 -ansi -filetype.c source\PE\sound.c -o output\objects\pe\sound.o ; 
:cmdHash=0x80c43bbe

:installDir=c:\ghs\comp_202354
:installDirHash=0x9d2c864f
