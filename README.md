pongoOS for QEMU
================

![screenshot](./.github/screenshot.png)


Fork of PongoOS which can be run in QEMU.  Working so far:
- Framebuffer (ramfb)
- UART/serial (pl011)
- Interrupt Controller (GICv2)
- Device tree parsing
- QEMU fw_cfg parsing

Not working:
- Everything else

Not sure if this is useful to anybody.

To run:
```bash
make
qemu-system-aarch64 \
    -cpu cortex-a72 -M virt,highmem=off -accel tcg \
    -device loader,file=build/Pongo.bin,addr=0x1000,force-raw=on \
    -device loader,addr=0x1000,cpu-num=0 -m 4096 \
    -device ramfb \
    -serial stdio
```
You need to use the [utmapp fork](https://github.com/utmapp/qemu) of qemu which has working TCG emulation.

Using qemu's `-s -S` arguments, you can also attach gdb and step through the execution.