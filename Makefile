
INC = -Iinclude
LIB = -lpthread

SRC = src
OBJ = obj
OBJ64 = obj64
INCLUDE = include

CC = gcc
DEBUG = -g
CFLAGS = -Wall -c $(DEBUG)
LFLAGS = -Wall $(DEBUG)

# 64-bit mode flags
CFLAGS64 = $(CFLAGS) -DMM64=1

vpath %.c $(SRC)
vpath %.h $(INCLUDE)

MAKE = $(CC) $(INC) 

# Object files needed by modules
MEM_OBJ = $(addprefix $(OBJ)/, paging.o mem.o cpu.o loader.o)
SYSCALL_OBJ = $(addprefix $(OBJ)/, syscall.o  sys_mem.o sys_listsyscall.o)
OS_OBJ = $(addprefix $(OBJ)/, cpu.o mem.o loader.o queue.o os.o sched.o timer.o mm-vm.o mm64.o mm.o mm-memphy.o libstd.o libmem.o)
OS_OBJ += $(SYSCALL_OBJ)
SCHED_OBJ = $(addprefix $(OBJ)/, cpu.o loader.o)
HEADER = $(wildcard $(INCLUDE)/*.h)

# 64-bit object files
SYSCALL_OBJ64 = $(addprefix $(OBJ64)/, syscall.o sys_mem.o sys_listsyscall.o)
OS_OBJ64 = $(addprefix $(OBJ64)/, cpu.o mem.o loader.o queue.o os.o sched.o timer.o mm-vm.o mm64.o mm.o mm-memphy.o libstd.o libmem.o)
OS_OBJ64 += $(SYSCALL_OBJ64)

.PHONY: all os os32 os64 clean clean32 clean64 help

all: os
#mem sched os

# Default 32-bit build (same as os)
os32: os

# Just compile memory management modules
mem: $(MEM_OBJ)
	$(MAKE) $(LFLAGS) $(MEM_OBJ) -o mem $(LIB)

# Just compile scheduler
sched: $(SCHED_OBJ)
	$(MAKE) $(LFLAGS) $(MEM_OBJ) -o sched $(LIB)

# Compile syscall
syscalltbl.lst: $(SRC)/syscall.tbl
	@echo $(OS_OBJ)
	chmod +x $(SRC)/syscalltbl.sh
	$(SRC)/syscalltbl.sh $< $(SRC)/$@ 
#	mv $(OBJ)/syscalltbl.lst $(INCLUDE)/

# Compile the whole OS simulation (32-bit mode - default)
os: $(OBJ) syscalltbl.lst $(OS_OBJ)
	$(MAKE) $(LFLAGS) $(OS_OBJ) -o os $(LIB)
	@echo "Built 32-bit OS (os)"

# Compile 64-bit OS simulation
os64: $(OBJ64) syscalltbl.lst $(OS_OBJ64)
	$(MAKE) $(LFLAGS) $(OS_OBJ64) -o os64 $(LIB)
	@echo "Built 64-bit OS (os64)"

# 32-bit object compilation rule
$(OBJ)/%.o: %.c ${HEADER} $(OBJ)
	$(MAKE) $(CFLAGS) $< -o $@

# 64-bit object compilation rule
$(OBJ64)/%.o: %.c ${HEADER} $(OBJ64)
	$(MAKE) $(CFLAGS64) $< -o $@

# Prepare objectives container for 32-bit
$(OBJ):
	mkdir -p $(OBJ)

# Prepare objectives container for 64-bit
$(OBJ64):
	mkdir -p $(OBJ64)

# Clean 32-bit build
clean32:
	rm -f $(SRC)/*.lst
	rm -f $(OBJ)/*.o os sched mem pdg
	rm -rf $(OBJ)

# Clean 64-bit build
clean64:
	rm -f $(OBJ64)/*.o os64
	rm -rf $(OBJ64)

# Clean all builds
clean: clean32 clean64

# Help target
help:
	@echo "Available targets:"
	@echo "  os      - Build 32-bit OS (default)"
	@echo "  os32    - Build 32-bit OS (alias for os)"
	@echo "  os64    - Build 64-bit OS with 5-level page tables"
	@echo "  clean   - Clean all builds"
	@echo "  clean32 - Clean 32-bit build only"
	@echo "  clean64 - Clean 64-bit build only"
	@echo "  help    - Show this help message"
	@echo ""
	@echo "Memory modes:"
	@echo "  32-bit: 22-bit address bus, 256B pages, single-level page table"
	@echo "  64-bit: 57-bit address bus, 4KB pages, 5-level page table (PGD/P4D/PUD/PMD/PT)"
