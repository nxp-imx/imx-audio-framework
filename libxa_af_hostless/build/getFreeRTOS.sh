#!/bin/sh 

git clone git://github.com/foss-xtensa/amazon-freertos.git FreeRTOS
cd FreeRTOS
git checkout 017c01b82bbd6e507dbfd2fac6ae453a0f0f4007
cd portable/XCC/Xtensa
xt-make clean
xt-make
