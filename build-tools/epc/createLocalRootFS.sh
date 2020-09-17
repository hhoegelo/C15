#!/bin/sh

set -e
set -x

rm -rf /bindir/squashfs-root /bindir/overlay-scratch /bindir/overlay-workdir /bindir/overlay-fs

mkdir -p /internal/AP-Linux-mnt /bindir/overlay-scratch /bindir/overlay-workdir /bindir/overlay-fs
fuseiso /bindir/AP-Linux-V.4.0.iso /internal/AP-Linux-mnt
unsquashfs -no-xattrs /internal/AP-Linux-mnt/arch/x86_64/airootfs.sfs
fuse-overlayfs -o lowerdir=/squashfs-root -o upperdir=/bindir/overlay-scratch -o workdir=/bindir/overlay-workdir /bindir/overlay-fs
mkdir /bindir/overlay-fs/Audiophile2NonLinux
chmod 777 /bindir/overlay-fs/Audiophile2NonLinux
cp -a /bindir/NonLinux.pkg.tar.gz /sources/hook /sources/install /sources/sda.sfdisk /bindir/overlay-fs/Audiophile2NonLinux
cp -a /sources/runme.sh /bindir/overlay-fs/etc/profile.d/

/bindir/overlay-fs/bin/arch-chroot /bindir/overlay-fs /bin/bash -c "\

set -x

cp /Audiophile2NonLinux/install/nlhook /lib/initcpio/install/nlhook
cp /Audiophile2NonLinux/install/oroot /lib/initcpio/install/oroot
cp /Audiophile2NonLinux/hook/nlhook /lib/initcpio/hooks/nlhook
cp /Audiophile2NonLinux/hook/oroot /lib/initcpio/hooks/oroot

sed -i 's/read.*username/username=sscl/' /etc/apl-files/runme.sh
sed -i 's/read.*password/password=sscl/' /etc/apl-files/runme.sh
sed -i 's/pacman -U/pacman --noconfirm -U/' /etc/apl-files/runme.sh
sed -i 's/Required DatabaseOptional/Never/' /etc/pacman.conf
sed -i 's/Server.*mettke/#/' /etc/pacman.d/mirrorlist
sed -i 's/GRUB_TIMEOUT=5/GRUB_TIMEOUT=1/' /etc/default/grub
sed -i 's/^HOOKS=.*$/HOOKS=\"base udev oroot block filesystems autodetect modconf keyboard net nlhook\"/' /etc/mkinitcpio.conf
sed -i 's/^BINARIES=.*$/BINARIES=\"tar rsync gzip lsblk udevadm\"/' /etc/mkinitcpio.conf
sed -i 's/^chroot_add_resolv_conf /#/' /bin/arch-chroot

mkdir -p /update-packages
tar -C /update-packages -xzf /Audiophile2NonLinux/NonLinux.pkg.tar.gz
echo 'Server = file:///update-packages/pkg/' > /etc/pacman.d/mirrorlist

cd /etc/apl-files && ./runme.sh
cd /etc/apl-files && ./autologin.sh

pacman --noconfirm -Sy
pacman --noconfirm -Su

pacman --noconfirm -Rcs xorg gnome mesa man-db man-pages b43-fwcutter geoip ipw2100-fw ipw2200-fw libjpeg-turbo tango-icon-theme xorg-xmessage xf86-input-evdev xf86-input-synaptics zd1211-firmware xfsprogs cifs-utils emacs-nox lvm2 fuse2
pacman --noconfirm -S cpupower git cmake make gcc glibmm pkgconf jdk11-openjdk libsoup freetype2 boost libpng png++

pacman --noconfirm -Su
pacman --noconfirm -Qdt

mv /update-packages/pkg/gwt-2.8.2 /
rm -rf /update-packages

truncate -s 0 /mnt/home/sscl/.zprofile
rm -rf /usr/lib/modules/5.1.7-arch1-1-ARCH
rm -rf /usr/lib/modules/extramodules-ARCH
rm -rf /usr/lib/firmware/netronome
rm -rf /usr/lib/firmware/liquidio
rm -rf /usr/lib/firmware/amdgpu
rm -rf /usr/lib/firmware/qed
cd /usr/share/locale && ls -1 | grep -v 'en_US' | xargs rm -rf {}
rm -rf /usr/share/doc
rm -rf /usr/share/info
rm -rf /usr/share/man
systemctl mask systemd-backlight@
systemctl mask systemd-random-seed
systemctl mask systemd-tmpfiles-setup
systemctl mask systemd-tmpfiles-clean
systemctl mask systemd-tmpfiles-setup-dev
rm /etc/profile.d/runme.sh
rm -rf /Audiophile2NonLinux
"

