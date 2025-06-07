#!/bin/bash
# Script outline to install and build kernel.
# Author: Siddhant Jajoo.

set -e
set -u

OUTDIR=/tmp/aeld
KERNEL_REPO=git://git.kernel.org/pub/scm/linux/kernel/git/stable/linux-stable.git
KERNEL_VERSION=v5.15.163
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

    echo "building kernel"

    # TODO: Add your kernel build steps here
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} mrproper
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} defconfig
    make -j4 ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} all
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} modules
    make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} dtbs
fi

echo "Adding the Image in outdir"
cp ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}/Image

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
echo "creating base directories"
mkdir -p ${OUTDIR}/rootfs
cd rootfs
mkdir -p bin dev etc home lib lib64 proc sbin sys tmp usr var
mkdir -p usr/bin usr/lib usr/sbin
mkdir -p var/log

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
git clone https://github.com/mirror/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
    # TODO:  Configure busybox
else
    cd busybox
fi

# TODO: Make and install busybox
echo "making busybox"
make distclean
make defconfig
make ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE}
echo "installing busybox"
make CONFIG_PREFIX=${OUTDIR}/rootfs ARCH=${ARCH} CROSS_COMPILE=${CROSS_COMPILE} install

echo "Library dependencies"
${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "program interpreter"
${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "Shared library"

SYSROOT=$(${CROSS_COMPILE}gcc -print-sysroot)

# ======================================
# Attempt to automate this step and fail
# ======================================

# INTERP=$(${CROSS_COMPILE}readelf -a ${OUTDIR}/rootfs/bin/busybox | grep "program interpreter" | 
#        awk -F: '{print $2}' | tr -d ' []')

# # TODO: Add library dependencies to rootfs
# INTERP_PATH=$(find "${SYSROOT}" -name $(basename ${INTERP}) | head -n1)
# if [ -z "${INTERP_PATH}" ]; then
#     echo "Error: Could not find ${INTERP} in sysroot"
#     exit 1
# fi
# echo "Copying interpreter from ${INTERP_PATH} to rootfs/$(dirname $INTERP)"
# mkdir -p "$OUTDIR/rootfs/$($dirname ${INTERP})"
# cp -a "${INTERP_PATH}" "${OUTDIR}/rootfs/${INTERP}"

# # SHARED_LIBS=$(find "$SYSROOT" -name $(basename $SHARED_LIB) | head -n1)
# SHARED_LIBS=$(${CROSS_COMPILE}readelf -d ${OUTDIR}/rootfs/bin/busybox | grep NEEDED | awk -F'[][]' '{print $2}')
# for {LIB} in ${SHARED_LIBS}; do
#     {LIB_PATH}=$(find "${SYSROOT}" -name "${LIB}" | head -n1) 
#     if [ -z "${LIB_PATH}" ]; then
#         echo "Error: Could not find ${LIB} in sysroot"
#         exit 1
#     fi
#     echo "Copying shared lib ${LIB} from ${LIB_PATH} to ${OUTDIR}/rootfs/lib/)"
#     cp -a "${LIB_PATH}" "${OUTDIR}/rootfs/lib/"
# done

# ======================================
# :(
# ======================================

cp -a ${SYSROOT}/lib/ld-linux-aarch64.so.1 ${OUTDIR}/rootfs/lib/
cp -a ${SYSROOT}/lib64/libm.so.6 ${OUTDIR}/rootfs/lib64/
cp -a ${SYSROOT}/lib64/libresolv.so.2 ${OUTDIR}/rootfs/lib64/
cp -a ${SYSROOT}/lib64/libc.so.6 ${OUTDIR}/rootfs/lib64/


echo "All dependencies copied to ${OUTDIR}/rootfs"

# TODO: Make device nodes
echo "Making device nodes"
cd ${OUTDIR}/rootfs
sudo mknod -m 666 dev/null c 1 3
sudo mknod -m 666 dev/console c 5 1

# TODO: Clean and build the writer utility
echo "Cleaning and building the writer application"
cd ${FINDER_APP_DIR}
make CROSS_COMPILE=${CROSS_COMPILE} clean
make CROSS_COMPILE=${CROSS_COMPILE} writer

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
echo "Copying finder related scripts and executables from ${FINDER_APP_DIR} \
      to /home directory of rootfs"
mkdir -p "${OUTDIR}/rootfs/home/conf"
cp writer "${OUTDIR}/rootfs/home/"
cp finder.sh "${OUTDIR}/rootfs/home/"
cp finder-test.sh "${OUTDIR}/rootfs/home"
cp conf/username.txt "${OUTDIR}/rootfs/home/conf/"
cp conf/assignment.txt "${OUTDIR}/rootfs/home/conf/"
cp autorun-qemu.sh "${OUTDIR}/rootfs/home/"

# TODO: Chown the root directory
sudo chown -R root:root ${OUTDIR}/rootfs

# TODO: Create initramfs.cpio.gz
echo "Creating initramfs.cpio image"
cd "${OUTDIR}/rootfs/"
find . | cpio -H newc -ov --owner root:root > ${OUTDIR}/initramfs.cpio
cd "${OUTDIR}"
gzip -f initramfs.cpio

echo "Done!"
