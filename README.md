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
You need to use the [utmapp fork](https://github.com/utmapp/qemu) of qemu which has working TCG emulation and apply the following patch to change the memory layout to what pongo expects:
```patch
diff --git a/hw/arm/virt.c b/hw/arm/virt.c
index 27dbeb549e..8a208badcb 100644
--- a/hw/arm/virt.c
+++ b/hw/arm/virt.c
@@ -160,7 +160,7 @@ static const MemMapEntry base_memmap[] = {
     [VIRT_PCIE_PIO] =           { 0x3eff0000, 0x00010000 },
     [VIRT_PCIE_ECAM] =          { 0x3f000000, 0x01000000 },
     /* Actual RAM size depends on initial RAM and device memory settings */
-    [VIRT_MEM] =                { GiB, LEGACY_RAMLIMIT_BYTES },
+    [VIRT_MEM] =                { 0x800000000, LEGACY_RAMLIMIT_BYTES },
 };

 /*
```

Using qemu's `-s -S` arguments, you can also attach gdb and step through the execution.