# Im still learn how to use and make makefile :)


# Buat sebuah variable dengan nama objek yang berisi source code C
# Buat sebuah variable dengan nama header yang berisi header file
# Buat sebuah variable dengan nama compiler yang berisi compiler yang akan digunakan
# buat sebuah variable untuk flag C yang akan digunakan pada compiler
# buat sebuah variable untuk flag Library yang akan digunakan pada compiler
# buat sebuah variable untuk output file
OBJECT = chip8.c
HEADER = chip8.h
COMPILER = gcc
C_FLAG = -w -pedantic
L_FLAG = -lSDL2
OUTPUT = chip8

# Satukan semua konfigurasi
all: $(OBJECT)
	$(COMPILER) $(OBJECT) $(HEADER) $(C_FLAG) $(L_FLAG) -o $(OUTPUT)

