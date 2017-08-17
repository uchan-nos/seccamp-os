#!/bin/sh

#==========
# variables
#==========
export EDK2_ROOT=${EDK2_ROOT:-${HOME}/workspace/github.com/uchan-nos/edk2}
export EDK2_RELEASE=${EDK2_RELEASE:-RELEASE}
export EDK2_TOOLCHAIN=${EDK2_TOOLCHAIN:-GCC5}

export OVMF_CODE=${OVMF_CODE:-${EDK2_ROOT}/Build/OvmfX64/${EDK2_RELEASE}_${EDK2_TOOLCHAIN}/FV/OVMF_CODE.fd}
export OVMF_VARS=${OVMF_VARS:-./OVMF_VARS.fd}
export BOOTLOADER=${BOOTLOADER:-${EDK2_ROOT}/Build/MyBootLoaderX64/${EDK2_RELEASE}_${EDK2_TOOLCHAIN}/X64/Loader.efi}

export NEWLIB_INCDIR=${NEWLIB_INCDIR:-${HOME}/x86_64-pc-linux-gnu/include}
export NEWLIB_LIBDIR=${NEWLIB_LIBDIR:-${HOME}/x86_64-pc-linux-gnu/lib}

export DISKIMAGE=${DISKIMAGE:-./diskimage}

#======================
# generate Makefile.inc
#======================
python3 -c "from string import Template; import os; print(Template(open('Makefile.tmpl').read()).substitute(os.environ), end='')" > Makefile.inc

#=========================
# construct disk structure
#=========================
mkdir -p ./${DISKIMAGE}/EFI/BOOT
ln -s ${BOOTLOADER} ${DISKIMAGE}/EFI/BOOT/BOOTX64.EFI
ln -s ../kernel.elf ${DISKIMAGE}/kernel.elf
