# minix3x
Experimental continuation and customization of MINIX.

## Building the system

Build the whole system (toolchain + release) using the `build.sh` script, e.g. for i386:

```bash
./build.sh -j $(nproc) -m i386 -O ../obj.i386 -D ../obj.i386/destdir.i386 -U -u release
```

## Creating a disk image

After a successful `make release`, generate a bootable disk image using the
`releasetools/x86_hdimage.sh` script (run it from the repository root):

```bash
ARCH=i386 OBJ=../obj.i386 releasetools/x86_hdimage.sh
```

The result is a `minix_x86.img` file in the repository root.

## Running the image in QEMU

```bash
qemu-system-i386 -m 256 -drive file=minix_x86.img,format=raw,if=ide -display curses
```

Notes:
- `-display curses` renders the emulated screen as text in the current
  terminal and is useful in environments without X11/GTK (e.g. an SSH session
  or a VM without a GUI). If a graphical environment is available, you can
  drop this option or use `-display gtk`/`-display sdl` instead.
- If KVM is available (native Linux, or a VM with nested virtualization
  enabled), add `--enable-kvm` to speed up emulation.
- Only one QEMU instance can have the image file open at a time — a
  `Failed to get "write" lock` error means another QEMU process is already
  using it.
