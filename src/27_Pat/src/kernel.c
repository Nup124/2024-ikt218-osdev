#include "libc/stdint.h"
#include "libc/stddef.h"
#include "libc/stdbool.h"
#include  "libc/stdarg.h"
#include <libc/stdio.h>
#include <multiboot2.h>
#include "../include/gdt.h"
#include <../include/common.h>
#include <../include/monitor.h>

struct multiboot_info {
    uint32_t size;
    uint32_t reserved;
    struct multiboot_tag *first;
};

int kernel_main();

uint16_t lengthSq(uint16_t x, uint16_t y) {
    uint16_t r = x * x + y * y;
    return r;
}

int main(uint32_t magic, struct multiboot_info* mb_info_addr) {

    init_gdt();
    monitor_clear();
    printf("Printing strings: \n");
    printf("Hello, world!\n");
    printf("This is a test.\n");
    printf("\n");

    printf("Printing integers: \n");
    printf("Integer: %d\n", 123);
    printf("Negative Integer: %d\n", -456);
    printf("Zero: %d\n", 0);
    printf("\n");

    printf("Printing hexadecimal values: \n");
    printf("Hexadecimal: %x\n", 0x1A);
    printf("Another Hexadecimal: %x\n", 0xFF);
    printf("Large Hexadecimal: %x\n", 0xABCD);
    printf("\n");

    int x = 10;
    printf("Float: %f\n", 1.23);
    printf("Character: %c\n", 'A');
    printf("Address of x: %p\n", &x);
    return kernel_main();
} 