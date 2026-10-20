output\objects\pe\sound.o: sound.c

:cmdList=ccrh850 -c  -MD -I. -I..\..\device -G -object_dir=output\objects\pe -list -list_dir=output/list/pe -passsource -bsp generic -sda=all -fsoft -cpu=rh850 -ansi -filetype.c sound.c -o output\objects\pe\sound.o ; 
:cmdHash=0xdd2b2b3d

:installDir=c:\ghs\comp_202354
:installDirHash=0x9d2c864f
