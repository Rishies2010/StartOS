clang -m64 -ffreestanding -fno-stack-protector \
      -nostdlib -fno-pie -fPIC \
      -c test_app.c -o test_app.o

ld -m elf_x86_64 -e main -T userelf.ld test_app.o -o test_app.elf \
   --warn-unresolved-symbols \
   --noinhibit-exec

if [ ! -f test_app.elf ]; then
    exit 1
fi
