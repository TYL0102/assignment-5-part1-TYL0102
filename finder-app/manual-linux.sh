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
if [ ! -d "${OUTDIR}/linux-stable" ]
then
    # Clone only if the repository does not exist.
	echo "CLONING GIT LINUX STABLE VERSION ${KERNEL_VERSION} IN ${OUTDIR}"
	git clone ${KERNEL_REPO} --depth 1 --single-branch --branch ${KERNEL_VERSION}
fi
if [ ! -e ${OUTDIR}/linux-stable/arch/${ARCH}/boot/Image ]
then
    cd linux-stable
    echo "Checking out version ${KERNEL_VERSION}"
    git checkout ${KERNEL_VERSION}

    # TODO: Add your kernel build steps here
    make -j$(nproc) ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE defconfig
    echo "CONFIG_BLK_DEV_INITRD=y" >> .config
    echo "CONFIG_EXT4_FS=y" >> .config
    echo "CONFIG_TMPFS=y" >> .config
    make -j$(nproc) ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE olddefconfig
    make -j$(nproc) ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE Image
    cd ..

fi

# TODO+: Adding the Image in outdir
echo "Adding the Image in outdir"
cp linux-stable/arch/${ARCH}/boot/Image ${OUTDIR}

echo "Creating the staging directory for the root filesystem"
cd "$OUTDIR"
if [ -d "${OUTDIR}/rootfs" ]
then
	echo "Deleting rootfs directory at ${OUTDIR}/rootfs and starting over"
    sudo rm  -rf ${OUTDIR}/rootfs
fi

# TODO: Create necessary base directories
mkdir -p rootfs/{bin,sbin,etc,proc,sys,usr/bin,usr/sbin,dev,lib,lib64,home,home/conf}

cd "$OUTDIR"
if [ ! -d "${OUTDIR}/busybox" ]
then
    git clone git://busybox.net/busybox.git
    cd busybox
    git checkout ${BUSYBOX_VERSION}
else
    cd busybox
fi

# TODO: Make and install busybox
make -j$(nproc) ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE distclean
make -j$(nproc) ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE defconfig
make -j$(nproc) ARCH=$ARCH CROSS_COMPILE=$CROSS_COMPILE CONFIG_PREFIX="${OUTDIR}/rootfs" install

# echo "Library dependencies"
# ${CROSS_COMPILE}readelf -a bin/busybox | grep "program interpreter"
# ${CROSS_COMPILE}readelf -a bin/busybox | grep "Shared library"

# TODO: Add library dependencies to rootfs
SYSROOT=$(${CROSS_COMPILE}gcc --print-sysroot)
cp -L ${SYSROOT}/lib/ld-linux-aarch64.so.1 ${OUTDIR}/rootfs/lib/
cp -L ${SYSROOT}/lib/ld-linux-aarch64.so.1 ${OUTDIR}/rootfs/lib64/
cp -L ${SYSROOT}/lib64/libc.so.6 ${OUTDIR}/rootfs/lib64/
cp -L ${SYSROOT}/lib64/libm.so.6 ${OUTDIR}/rootfs/lib64/
cp -L ${SYSROOT}/lib64/libresolv.so.2 ${OUTDIR}/rootfs/lib64/

# TODO: Make device nodes
sudo mknod -m 666 ${OUTDIR}/rootfs/dev/null c 1 3
sudo mknod -m 600 ${OUTDIR}/rootfs/dev/console c 5 1

# TODO+: Create init script
cat > ${OUTDIR}/rootfs/init << 'EOF'
#!/bin/sh
mount -t proc none /proc
mount -t sysfs none /sys
mount -t devtmpfs none /dev
exec /home/autorun-qemu.sh
EOF
chmod +x ${OUTDIR}/rootfs/init

# TODO: Clean and build the writer utility
cd $FINDER_APP_DIR
make clean
make CROSS_COMPILE=$CROSS_COMPILE

# TODO: Copy the finder related scripts and executables to the /home directory
# on the target rootfs
cp finder.sh finder-test.sh writer autorun-qemu.sh "${OUTDIR}/rootfs/home/"
cp conf/username.txt conf/assignment.txt "${OUTDIR}/rootfs/home/conf/"

# TODO: Chown the root directory
cd $OUTDIR
sudo chown -R root:root rootfs

# TODO: Create initramfs.cpio.gz
cd rootfs
# find . | cpio -H newc -o | gzip > "${OUTDIR}/initramfs.cpio.gz"
find . -print0 | cpio --null -H newc -o | gzip > "${OUTDIR}/initramfs.cpio.gz"

echo "Build complete. Kernel: ${OUTDIR}/Image, Initramfs: ${OUTDIR}/initramfs.cpio.gz"