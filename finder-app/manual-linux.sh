#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.1.10
BUSYBOX_VERSION=1_33_1
FINDER_APP_DIR=$(realpath $(dirname $0))
ARCH=arm64
CROSS_COMPILE=aarch64-none-linux-gnu-

if [ $# -lt 1 ]
then
	echo "Using default directory ${OUTDIR} for output"
else
	OUTDIR=$1
	echo "Using passed directory ${OUTDIR} for output"
fi

mkdir -p ${OUTDIR}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/linux-stable" ]; then
    #Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]; then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    echo "Deep clean kernel build tree"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper
    echo "Config arm dev board as simulation in QEMU"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
    echo "Build a kernel image for booting with QEMU"
    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all
    echo "Build kernel modules"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules
    echo "Build the devicetree"
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs

fi

echo "Adding the Image in outdir"

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

echo "Create necessary base directories"
mkdir -p ${OUTDIR}/rootfs
cd ${OUTDIR}/rootfs
mkdir -p bin sbin dev etc home lib lib64 proc sys tmp usr var
mkdir -p usr/bin usr/lib usr/sbin
mkdir -p var/log

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    echo "Configure busybox"
    make distclean
    make defconfig
else
    cd busybox
fi

echo " Make and install busybox"
make -j4 CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install
cd "${OUTDIR}"/rootfs

echo "Library dependencies"
${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"

echo "Add library dependencies to rootfs"
SYSROOT=$(${CROSS_COMPILE}gcc -print-sysroot)
cp "${SYSROOT}"/lib/ld-linux-aarch64.* "${OUTDIR}/rootfs/lib"
cp "${SYSROOT}"/lib64/libm.so.* "${OUTDIR}/rootfs/lib64"
cp "${SYSROOT}"/lib64/libresolv.so.* "${OUTDIR}/rootfs/lib64"
cp "${SYSROOT}"/lib64/libc.so.* "${OUTDIR}/rootfs/lib64"

echo "Make device nodes"
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 620 dev/console c 5 1

echo "Clean and build the writer utility"
cd "${FINDER_APP_DIR}"/
make clean
make arch=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}

echo "Copy the finder related scripts and executables to the /home directory on the target rootfs"
cp writer "${OUTDIR}"/rootfs/home/
cp finder-test.sh "${OUTDIR}"/rootfs/home/
cp finder.sh "${OUTDIR}"/rootfs/home/
cp -r conf "${OUTDIR}"/rootfs/home/
cp autorun-qemu.sh "${OUTDIR}"/rootfs/home/

echo "Chown the root directory"
sudo chown root:root -R "${OUTDIR}"/rootfs

echo "Create initramfs.cpio.gz"
cd "${OUTDIR}"/rootfs
find . | cpio -H newc -ov --owner root:root > "${OUTDIR}"/initramfs.cpio
gzip -f "${OUTDIR}"/initramfs.cpio
