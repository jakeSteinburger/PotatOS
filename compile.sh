if [ ! -d "limine" ]; then
    git clone https://github.com/limine-bootloader/limine.git --branch=v8.x-binary --depth=1
fi

make -C limine

make

mkdir -p sysroot

mkdir -p sysroot/boot
cp -v bin/potatos sysroot/boot/
mkdir -p sysroot/boot/limine
cp -v src/limine.conf limine/limine-bios.sys limine/limine-bios-cd.bin \
      limine/limine-uefi-cd.bin sysroot/boot/limine/

mkdir -p sysroot/EFI/BOOT
cp -v limine/BOOTX64.EFI sysroot/EFI/BOOT/
cp -v limine/BOOTIA32.EFI sysroot/EFI/BOOT/

xorriso -as mkisofs -b boot/limine/limine-bios-cd.bin \
        -no-emul-boot -boot-load-size 4 -boot-info-table \
        --efi-boot boot/limine/limine-uefi-cd.bin \
        -efi-boot-part --efi-boot-image --protective-msdos-label \
        sysroot -o potatos.iso

./limine/limine bios-install potatos.iso

qemu-system-x86_64 potatos.iso
