include config.mk

# You should not have to change anything below here.

CC	:= avr32-gcc
CXX	:= avr32-g++
AS	:= avr32-as
LINKER	:= $(if $(CXXSRCS), $(CXX), $(CC))
OBJ	:= $(CXXSRCS:.cpp=.o) $(CSRCS:.c=.o) $(ASSRCS:.S=.o)

COMPILER_FLAGS	:= \
	-c -march=$(ARCH) -mpart=$(PART) $(WARNINGS) $(DEFS) 		\
		$(PLATFORM_INC_PATH:%=-I%) $(INC_PATH:%=-I%) $(CPP_EXTRA_FLAGS)

all:	$(TARGET)

clean:
	rm -rf $(TARGET) $(OBJ)
	rm -rf *.bak *.*~ *~ *.o *.lst *.map *.hex *.bin *.srec

$(TARGET):	$(OBJ)
	$(LINKER) -march=$(ARCH) -mpart=$(PART) -T$(LINKER_SCRIPT) $(LD_EXTRA_FLAGS) $^ -o $@

%.o:	%.c
	$(CC) $(COMPILER_FLAGS) -o $@ $<

%.o:	%.cpp
	$(CXX) $(COMPILER_FLAGS) -o $@ $<

%.o:	%.S
	$(CC) $(COMPILER_FLAGS) -o $@ $<

flash:	$(TARGET)
	batchisp -device at32$(PART) -hardware usb -operation erase f memory flash blankcheck loadbuffer $(TARGET) program verify start noreset 0

