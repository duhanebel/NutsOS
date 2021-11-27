SHELL = /bin/bash

ASM_SRC = $(shell find src -type f -name '*.asm' ! -name boot.asm)
ASM_OBJ = $(patsubst src/%.asm, build/%.asm.o, $(ASM_SRC))
C_SRC = $(shell find src -type f -name '*.c')
C_OBJ = $(patsubst src/%.c, build/%.o, $(C_SRC))
OBJ_FILES = $(ASM_OBJ) $(C_OBJ)

$(info $$C_SRC is [${C_SRC}])
$(info $$C_OBJ is [${C_OBJ}])
$(info $$OBJ_FILES is [${OBJ_FILES}])

INCLUDES = -I./src
FLAGS = -g -ffreestanding -falign-jumps -falign-functions -falign-labels -falign-loops -fstrength-reduce -fomit-frame-pointer -finline-functions -Wno-unused-function -fno-builtin -Werror -Wno-unused-label -Wno-cpp -Wno-unused-parameter -nostdlib -nostartfiles -nodefaultlibs -Wall -O0 -Iinc

# CC=./dev-img/run.sh i686-elf-gcc
# LD=./dev-img/run.sh i686-elf-ld
CC=i686-elf-gcc
LD=i686-elf-ld

all: ./build/empty.bin

./build/empty.bin: ${OBJ_FILES}
	mkdir -p ./bin
#	$(LD) -g -relocatable $(OBJ_FILES) -o ./build/kernelfull.o
	$(CC) $(FLAGS) -T ./src/linker.ld -ffreestanding -O0 -nostdlib ./build/main.o -o ./bin/empty.bin


$(ASM_OBJ): build/%.asm.o: src/%.asm
	mkdir -p $(@D)
	nasm -f elf -g $< -o $@

$(C_OBJ): build/%.o: src/%.c
	mkdir -p $(@D)
	$(CC)  $(INCLUDES) $(FLAGS) -std=gnu99 -c $< -o $@

clean:
	rm -rf ./bin/*.bin
	rm -rf ./build/*.o
	rm -rf ${OBJ_FILES}
