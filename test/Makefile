include ../Makefile.inc

CPPFLAGS := -I../ $(UEFI_INC) -DCPPUTEST --include ./pre_include.hpp
CXXFLAGS := -g -O2 -Wall -std=c++1z -masm=intel

OBJS = common.o ../asmfunc.o ../memory.o \
       ../usb/malloc.o ../usb/busdriver.o \
       ../usb/xhci/xhci.o ../usb/xhci/ring.o ../usb/xhci/port.o \
       ../usb/xhci/trb.o ../usb/xhci/devmgr.o ../usb/xhci/interrupt.o \
       test_queue.o test_mutex.o test_bitutil.o test_xhci.o test_memory.o \
       usb/test_malloc.o usb/test_register.o usb/test_busdriver.o usb/test_device.o \
       usb/xhci/test_registers.o usb/xhci/test_ring.o \
       usb/xhci/test_interrupt.o usb/xhci/test_devmgr.o usb/xhci/test_device.o

DEPENDS = $(join $(dir $(OBJS)),$(addprefix .,$(notdir $(OBJS:.o=.d))))

.PHONY: all
all: test.run

test.run: $(OBJS) $(DEPENDS)
	$(CXX) -o test.run $(OBJS) -lCppUTest -lCppUTestExt -lpthread

.PHONY: run
run: test.run
	./test.run

.PHONY: clean
clean:
	$(RM) $(OBJS)

.%.d: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MM $< > $@
	$(eval OBJ = $(<:.cpp=.o))
	sed --in-place 's|$(notdir $(OBJ))|$(OBJ)|' $@

.PHONY: depends
depends:
	$(MAKE) $(DEPENDS)

-include $(DEPENDS)
