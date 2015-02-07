#!/bin/sh

PAK_NAME=package4412_bsp2

function make_recovery()
{
	make clean
	cp recovery_config .config -f
	make -j12
	mv arch/arm/boot/zImage ../img/zImage.debug -f
	cp .config recovery_config -f
}

function make_kernel()
{
	make clean
	cp bdwg_config .config -f
	make -j12
	mv arch/arm/boot/zImage ../img/zImage -f
	cp .config  bdwg_config -f
}

case $1 in
 
    all )
#        make_recovery
#        make_kernel
        #make distclean
        #make smdk4212_config
        #make smdk4412_config
        ;;

    k* )
       make clean
       make -j12
       mv arch/arm/boot/zImage ../img/zImage -f
       cp .config  bdwg_config -f
       ;;

    c* )
        mkdir -p ../img/ 
        cp arch/arm/boot/zImage ../img/ -f
        ;;

    d* )
#	mkdir -p ../img/
#	make clean
#	cp recovery_config .config -f
#	make -j12
#	mv arch/arm/boot/zImage ../img/zImage.debug -f
#        cp arch/arm/boot/zImage ../img/ -f
        ;;

    ba* )
	cp .config bdwg_config
        ;;

    bdwg )
        echo "copy zImage to release tool $PAK_NAME  +!!"
		scp ../img/zImage albertli@192.168.1.161:/home2/albertli/d7xx_release_exynos_linuxtool/${PAK_NAME}/

        echo "copy zImage to release tool $PAK_NAME  -!!"
        ;;


    * )
        make -j12
        ;;

esac

echo "**********  Compile End $1 Command !!!!    ***********************" 
