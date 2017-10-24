include Makefile.inc

OBJS = main.o hankaku.o asmfunc.o inthandler.o libc/func.o \
       graphics.o debug_console.o memory.o memory_op.o desctable.o pci.o \
       command.o xhci.o

.PHONY: all
all:
	$(MAKE) kernel.elf

hankaku.o: hankaku.bin
	$(OBJCOPY) -I binary -O elf64-x86-64 -B i386 $< $@

#kernel.elf: $(OBJS) depends Makefile
#	$(LD) -Tkernel.ld -z max-page-size=0x1000 \
#	    -Map kernel.map -o kernel.elf $(OBJS) -lc

CFLAGS := $(CFLAGS) -fPIE
CXXFLAGS := $(CXXFLAGS) -fPIE
kernel.elf: $(OBJS) depends Makefile
	$(LD) -Tkernel.ld -z max-page-size=0x1000 \
	    -Map kernel.map -o kernel.elf $(OBJS) -lc -pie

.PHONY: run
run: all
	$(QEMU) \
	    -vga std -monitor stdio -gdb tcp::1234 -m 128M \
	    -drive if=pflash,format=raw,readonly,file=$(OVMF_CODE) \
	    -drive if=pflash,format=raw,file=$(OVMF_VARS) \
	    -drive if=ide,file=fat:rw:$(DISKIMAGE),index=0,media=disk \
	    -device nec-usb-xhci,id=xhci

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

.PHONY: test
test:
	$(MAKE) -C test run

# generate dependency files
.%.d: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -MM $< > $@

.%.d: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MM $< > $@

.%.d: %.s
	touch $@

.%.d: %.bin
	touch $@

DEPENDS = $(join $(dir $(OBJS)),$(addprefix .,$(notdir $(OBJS:.o=.d))))
.PHONY: depends
depends: $(DEPENDS)
-include $(DEPENDS)
