#!/bin/sh

for UT_KO in `find ../drivers/oem_drv -name "*.ko"`
do
    cp $UT_KO .
done

for UT_KO in `find ../drivers/bluetooth -name "*.ko"`
do
    cp $UT_KO .
done


#cp `find  ../urbetter/ -name '*.ko'` .

/usr/local/arm/arm-2009q3/bin/arm-none-linux-gnueabi-strip --strip-debug *.ko
chmod 777 *.ko


#cp *.ko ../../exynos-android_4.2.2_r1_rtm_porting/vendor/ut_oem_4412/lib/modules/ -f
