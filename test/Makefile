include ../Makefile.inc

CPPFLAGS = -I../
CXXFLAGS = -g -Wall -std=c++1z -masm=intel

OBJS = ../asmfunc.o test_queue.o test_mutex.o test_bitutil.o test_xhci.o

.PHONY: all
all: test.run

test.run: $(OBJS)
	$(CXX) -o test.run $(OBJS) -lCppUTest -lCppUTestExt -lpthread

.PHONY: run
run: test.run
	./test.run

.PHONY: clean
clean:
	$(RM) $(OBJS)

.%.d: %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MM $< > $@

DEPENDS = $(join $(dir $(OBJS)),$(addprefix .,$(notdir $(OBJS:.o=.d))))
.PHONY: depends
depends: $(DEPENDS)
	echo $(DEPENDS)
-include $(DEPENDS)
