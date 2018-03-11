include Makefile.inc

OBJS = main.o hankaku.o asmfunc.o inthandler.o libc/func.o \
       graphics.o debug_console.o memory.o memory_op.o desctable.o pci.o \
       command.o xhci.o xhci_trb.o usb/malloc.o usb/busdriver.o \
       usb/xhci/xhci.o usb/xhci/ring.o usb/xhci/trb.o usb/xhci/port.o usb/xhci/devmgr.o usb/xhci/interrupt.o

DEPENDS = $(join $(dir $(OBJS)),$(addprefix .,$(notdir $(OBJS:.o=.d))))

CFLAGS += -I$(PWD)
CFLAGS += -mno-sse4 -mno-sse4 -mno-sse4a  # Conroe CPU doesn't support SSE4 and later.

.PHONY: all
all:
	$(MAKE) LDFLAGS="$(LDFLAGS) -Ttext-segment=0x00200000" kernel.elf

.PHONY: pie
pie:
	$(MAKE) CFLAGS="$(CFLAGS) -fPIE" LDFLAGS="$(LDFLAGS) -pie -Ttext-segment=0" kernel.elf

hankaku.o: hankaku.bin
	$(OBJCOPY) -I binary -O elf64-x86-64 -B i386 $< $@

kernel.elf: $(OBJS) $(DEPENDS) Makefile
	$(LD) $(LDFLAGS) -Tkernel.ld -z max-page-size=0x1000 \
	    -Map kernel.map -o kernel.elf $(OBJS) -lc

.PHONY: raw_run
raw_run:
	$(QEMU) -trace events=trace.events \
	    -cpu Conroe \
	    -vga std -monitor stdio -gdb tcp::1234 -m 128M \
	    -drive if=pflash,format=raw,readonly,file=$(OVMF_CODE) \
	    -drive if=pflash,format=raw,file=$(OVMF_VARS) \
	    -drive if=ide,file=fat:rw:$(DISKIMAGE),index=0,media=disk \
	    -device nec-usb-xhci,id=xhci \
	    -device usb-kbd
#	    -device usb-mouse -device usb-kbd

.PHONY: run
run: all
	$(MAKE) raw_run

.PHONY: run-pie
run-pie: pie
	$(MAKE) raw_run

# -device usb-kbd $(QEMU_OPT)
# -device usb-mouse

.PHONY: clean
clean:
	$(RM) $(OBJS)
	$(RM) kernel.map

.PHONY: install_usb
install_usb:
	@mountpoint -q $(USB_MOUNT_POINT) || \
	    (echo "Please mount your usb stick at $(USB_MOUNT_POINT)"; exit 1)
	sudo cp --dereference --recursive $(DISKIMAGE)/* $(USB_MOUNT_POINT)

disk.raw: kernel.elf
	qemu-img create -f raw disk.raw 128m
	mkfs.fat disk.raw
	mkdir .mnt
	sudo mount -o loop disk.raw .mnt
	sudo cp --dereference --recursive $(DISKIMAGE)/* .mnt/
	sudo umount .mnt
	rmdir .mnt

disk.vdi: disk.raw
	rm -f disk.vdi
	VBoxManage convertfromraw disk.raw disk.vdi --format VDI --variant Fixed --uuid 3970c919-0551-4946-a7ad-40f838de3877

.PHONY: test
test:
	$(MAKE) -C test run

%.s: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -S -o $@ $<

# generate dependency files
.%.d: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -MM $< > $@
	$(eval OBJ = $(<:.c=.o))
	sed --in-place 's|$(notdir $(OBJ))|$(OBJ)|' $@

.%.d: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MM $< > $@
	$(eval OBJ = $(<:.cpp=.o))
	sed --in-place 's|$(notdir $(OBJ))|$(OBJ)|' $@

.%.d: %.s
	touch $@

.%.d: %.bin
	touch $@

.PHONY: depends
depends:
	$(MAKE) $(DEPENDS)

-include $(DEPENDS)
