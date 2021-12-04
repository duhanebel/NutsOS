SHELL = /bin/bash

PROGRAMS = $(shell find programs/ -maxdepth 1 -type d ! -name 'programs')

ASM_SRC = $(shell find src -type f -name '*.asm' ! -name boot.asm)
ASM_OBJ = $(patsubst src/%.asm, build/%.asm.o, $(ASM_SRC))
C_SRC = $(shell find src -type f -name '*.c')
C_OBJ = $(patsubst src/%.c, build/%.o, $(C_SRC))
OBJ_FILES = $(ASM_OBJ) $(C_OBJ)

#$(info $$C_SRC is [${C_SRC}])
#$(info $$C_OBJ is [${C_OBJ}])
#$(info $$OBJ_FILES is [${OBJ_FILES}])
$(info $$PROGRAMS is [${PROGRAMS}])
INCLUDES = -I./src
FLAGS = -g -ffreestanding -falign-jumps -falign-functions -falign-labels -falign-loops -fstrength-reduce -fomit-frame-pointer -finline-functions -Wno-unused-function -fno-builtin -Werror -Wno-unused-label -Wno-cpp -Wno-unused-parameter -nostdlib -nostartfiles -nodefaultlibs -Wall -O0 -Iinc

# CC=./dev-img/run.sh i686-elf-gcc
# LD=./dev-img/run.sh i686-elf-ld
CC=i686-elf-gcc
LD=i686-elf-ld

all: ./bin/boot.bin ./bin/kernel.bin $(PROGRAMS)
	rm -rf ./bin/os.bin
	dd if=./bin/boot.bin >> ./bin/os.bin
	dd if=./bin/kernel.bin >> ./bin/os.bin
	dd if=/dev/zero bs=1048576 count=16 >> ./bin/os.bin
	# Copy a test file inside the fat16 image
	sudo mount -t vfat bin/os.bin /mnt/
	sudo mkdir /mnt/bin/
	sudo cp -vr bootimg-files/* /mnt/ 
	sudo cp -v programs/*/bin/*.bin /mnt/bin
	sudo umount /mnt

run: all
	DISPLAY=host.docker.internal:0 qemu-system-i386 -hda bin/os.bin -d int -no-reboot -no-shutdown

debug: all
	DISPLAY=host.docker.internal:0 \
	gdb -ex "set confirm off" \
	    -ex "set pagination off" \
	    -ex "add-symbol-file build/kernelfull.o 0x10000add-symbol-file build/kernelfull.o 0x1000000" \
	    -ex "target remote | qemu-system-i386 -hda ./bin/os.bin -S -gdb stdio"

./bin/kernel.bin: $(OBJ_FILES)
	$(LD) -g -relocatable $(OBJ_FILES) -o ./build/kernelfull.o
	$(CC) $(FLAGS) -T ./src/linker.ld -ffreestanding -O0 -nostdlib ./build/kernelfull.o -o ./bin/kernel.bin

./bin/boot.bin: ./src/boot/boot.asm
	nasm -f bin ./src/boot/boot.asm -o ./bin/boot.bin

$(PROGRAMS):
	$(MAKE) -C $@

.PHONY: $(PROGRAMS)

$(ASM_OBJ): build/%.asm.o: src/%.asm
	mkdir -p $(@D)
	nasm -f elf -g $< -o $@

$(C_OBJ): build/%.o: src/%.c
	mkdir -p $(@D)
	$(CC)  $(INCLUDES) $(FLAGS) -std=gnu99 -c $< -o $@

clean:
	rm -rf ./bin/*.bin
	rm -rf ${OBJ_FILES}
	rm -rf ./build/*.o
