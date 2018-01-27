include ../Makefile.inc

CPPFLAGS := -I../ $(UEFI_INC)
CXXFLAGS := -g -Wall -std=c++1z -masm=intel

OBJS = ../asmfunc.o ../memory.o test_queue.o test_mutex.o test_bitutil.o test_xhci.o test_memory.o \
       usb/test_malloc.o ../usb/malloc.o usb/test_register.o usb/xhci/test_registers.o usb/xhci/test_ring.o

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
