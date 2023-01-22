.. SPDX-License-Identifier: GPL-2.0-or-later
.. sectionauthor:: Marcel Ziswiler <marcel.ziswiler@toradex.com>

Verdin AM62 Module
==================

Quick Start
-----------

- Get the binary-only SYSFW
- Get binary-only TI Linux firmware
- Build the ARM trusted firmware binary
- Build the OPTEE binary
- Build U-Boot for the R5
- Build U-Boot for the A53
- Flash to eMMC
- Boot

For an overview of the TI AM62 SoC boot flow please head over to:
.. file:: ../ti/am62x_sk.rst

Get the SYSFW
-------------

.. code-block:: bash

    $ echo "Downloading SYSFW..."
    $ git clone git://git.ti.com/k3-image-gen/k3-image-gen.git

Get the TI Linux Firmware
-------------

.. code-block:: bash

    $ echo "Downloading TI Linux Firmware..."
    $ git clone -b ti-linux-firmware git://git.ti.com/processor-firmware/ti-linux-firmware.git

Get and Build the ARM Trusted Firmware (Trusted Firmware A)
-----------------------------------------------------------

.. code-block:: bash

    $ echo "Downloading and building TF-A..."
    $ git clone https://github.com/ARM-software/arm-trusted-firmware.git
    $ cd arm-trusted-firmware

Then build ATF (TF-A):

.. code-block:: bash

    $ export ARCH=aarch64
    $ export CROSS_COMPILE=aarch64-none-linux-gnu-
    $ make PLAT=k3 TARGET_BOARD=lite SPD=opteed

Note: Currently, not buildable with gcc-12.2 please revert to using gcc-11.3.
Note: Currently, latest master is broken please revert to using 2fcd408bb3a6756767a43c073c597cef06e7f2d5.

Get and Build OPTEE
-------------------

.. code-block:: bash

    $ echo "Downloading and building OPTEE..."
    $ git clone https://github.com/OP-TEE/optee_os.git
    $ cd optee_os

Then build OPTEE:

.. code-block:: bash

    $ export CROSS_COMPILE=arm-none-linux-gnueabihf-
    $ export CROSS_COMPILE64=aarch64-none-linux-gnu-
    $ make PLATFORM=k3 CFG_ARM64_core=y

Build U-Boot for R5
-------------------

.. code-block:: bash

    $ export ARCH=arm
    $ export CROSS_COMPILE=arm-none-linux-gnueabihf-
    $ make verdin-am62_r5_defconfig O=/tmp/r5
    $ make O=/tmp/r5
    $ cd ../k3-image-gen
    $ make SOC=am62x SBL=/tmp/r5/spl/u-boot-spl.bin SYSFW_PATH=../ti-linux-firmware/ti-sysfw/ti-fs-firmware-am62x-gp.bin
    $ cp tiboot3-am62x-gp-evm.bin ../tiboot3.bin

Build U-Boot for A53
--------------------

.. code-block:: bash

    $ export ARCH=arm64
    $ export CROSS_COMPILE=aarch64-none-linux-gnu-
    $ make verdin-am62_a53_defconfig O=/tmp/a53
    $ make ATF=$PWD/../arm-trusted-firmware/build/k3/lite/release/bl31.bin TEE=$PWD/../optee_os/out/arm-plat-k3/core/tee-pager_v2.bin DM=$PWD/../ti-linux-firmware/ti-dm/am62xx/ipc_echo_testb_mcu1_0_release_strip.xer5f O=/tmp/a53
    $ cp /tmp/a53/tispl.bin ../
    $ cp /tmp/a53/u-boot.img ../

Note: Relative paths to the artefacts are known to not work.

Flash to eMMC
-------------

.. code-block:: bash

    => mmc dev 0 1
    => fatload mmc 1 ${loadaddr} tiboot3.bin
    => mmc write ${loadaddr} 0x0 0x400
    => fatload mmc 1 ${loadaddr} tispl.bin
    => mmc write ${loadaddr} 0x400 0x1000
    => fatload mmc 1 ${loadaddr} u-boot.img
    => mmc write ${loadaddr} 0x1400 0x2000

As a convenience, instead of those commands one may also use the update U-Boot
wrapper:

.. code-block:: bash

    > run update_uboot

Boot
----

Output:

.. code-block:: bash

U-Boot SPL 2021.01-12793-gfbc143af80e-dirty (Feb 16 2023 - 09:09:06 +0100)
SYSFW ABI: 3.1 (firmware rev 0x0008 '8.6.0--v08.06.00 (Chill Capybar')
SPL initial stack usage: 13424 bytes
Trying to boot from MMC2
Warning: Detected image signing certificate on GP device. Skipping certificate t
o prevent boot failure. This will fail if the image was also encrypted
Warning: Detected image signing certificate on GP device. Skipping certificate t
o prevent boot failure. This will fail if the image was also encrypted
Starting ATF on ARM64 core...

NOTICE:  BL31: v2.7(release):v2.7.0-359-g1309c6c80
NOTICE:  BL31: Built : 09:28:14, Jan 17 2023
I/TC:
I/TC: OP-TEE version: 3.19.0-15-gd6c5d003 (gcc version 10.3.1 20210621 (GNU Tool
chain for the A-profile Architecture 10.3-2021.07 (arm-10.29))) #2 Tue Jan 17 07
:55:55 UTC 2023 aarch64
I/TC: WARNING: This OP-TEE configuration might be insecure!
I/TC: WARNING: Please check https://optee.readthedocs.io/en/latest/architecture/porting_guidelines.html
I/TC: Primary CPU initializing
I/TC: SYSFW ABI: 3.1 (firmware rev 0x0008 '8.6.0--v08.06.00 (Chill Capybar')
I/TC: HUK Initialized
I/TC: Activated SA2UL device
I/TC: Fixing SA2UL firewall owner for GP device
I/TC: Enabled firewalls for SA2UL TRNG device
I/TC: SA2UL TRNG initialized
I/TC: SA2UL Drivers initialized
I/TC: Primary CPU switching to normal world boot

U-Boot SPL 2021.01-12793-gfbc143af80e-dirty (Feb 16 2023 - 11:43:02 +0100)
SYSFW ABI: 3.1 (firmware rev 0x0008 '8.6.0--v08.06.00 (Chill Capybar')
Trying to boot from MMC2


U-Boot 2021.01-12793-gfbc143af80e-dirty (Feb 16 2023 - 11:43:02 +0100)

SoC:   AM62X SR1.0 GP
DRAM:  1 GiB
MMC:   mmc@fa10000: 0, mmc@fa00000: 1, mmc@fa20000: 2
Loading Environment from MMC... *** Warning - bad CRC, using default environment

In:    serial@2800000
Out:   serial@2800000
Err:   serial@2800000
Model: Toradex 0069 Verdin AM62 Quad 1GB WB IT V1.0A
Serial#: 00000001
get_tdx_eeprom: cannot find EEPROM by node
MISSING TORADEX CARRIER CONFIG BLOCKS
get_tdx_eeprom: cannot find EEPROM by node
am65_cpsw_nuss ethernet@8000000: K3 CPSW: nuss_ver: 0x6BA01103 cpsw_ver: 0x6BA81103 ale_ver: 0x00290105 Ports:2 mdio_freq:1000000
Setting variant to wifi
Net:
Warning: ethernet@8000000port@1 MAC addresses don't match:
Address in ROM is               34:08:e1:7e:90:ad
Address in environment is       00:14:2d:00:00:01
eth0: ethernet@8000000port@1
Hit any key to stop autoboot:  0
Verdin AM62 #
