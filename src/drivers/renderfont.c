#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "include/serial.h"
#include "include/framebuffer.h"
#include "include/renderfont.h"
#include "../fs/include/vfs.h"
#include "../kernel/kernel.h"
#include "../utils/include/printf.h"
#include "../mem/include/kheap.h"
#include "../utils/include/string.h"

#define PSF_FONT_MAGIC 0x0436

#define PSF1_MODE512 0x01
#define PSF1_MODEHASTAB 0x02
#define PSF1_MODESEQ 0x03

void init_font() {
    kstatusf("Initiating graphical font drawing...");
    File *f = open("R:/sys/zap-light16.psf", 0, MODE_READONLY);
    if (f == NULL) {
        kfailf("Font file could not be opened! All output will now be through serial.\n");
        return;
    }
    size_t file_size = file_length(f);
    char *buffer = (char*) malloc(file_size);
    ku_memset((uint8_t*) buffer, 0, file_size);
    int read_status = read(f, buffer, file_size);
    if (read_status != 0) {
        kfailf("Font file could not be read (but could be opened)! All output will now be through serial.\n");
        close(f);
        return;
    }
    close(f);
    kernel.font_info = (psf1Header*) buffer;
    kernel.font_data = (uint8_t*) buffer + sizeof(psf1Header); 
    if (kernel.font_info->magic_bytes != PSF_FONT_MAGIC) {
        kfailf("Tried to parse font file, appears to be invalid. Make sure that it's a PSF1 font file format. All output will now be through serial.\n");
        return;
    }
    kernel.font_avaliable = true;
    write_string(WHT "Graphical font rendering enabled. Allocator, VFS, TempFS, devices, initial ramdisk, and heap have already been set up.\n");
    write_serial(BGRN " Ok!\n" WHT);
}

void draw_char(char ch, uint64_t x_coord, uint64_t y_coord) {
    uint64_t first_byte_idx = ch * kernel.font_info->char_size;
    for (uint8_t y = 0; y < kernel.font_info->char_size; y++) {
        for (uint8_t x = 0; x < 8; x++) {
            if ((kernel.font_data[first_byte_idx + y] >> (7 - x)) & 1)
                draw_pixel(x_coord + x, y_coord + y, kernel.fg_colour);
            else
                draw_pixel(x_coord + x, y_coord + y, kernel.bg_colour);
        }
    }
}

void scroll_line() {
    scroll_pixel(kernel.font_info->char_size + 1);
    kernel.ch_Y -= kernel.font_info->char_size + 1;
    swap_framebuffers();
}

void new_line() {
    kernel.ch_X = 5;
    kernel.ch_Y += kernel.font_info->char_size + 1;
    if (kernel.ch_Y >= kernel.framebuffers[0]->height - kernel.font_info->char_size) scroll_line();
}

void write_char(char ch) {
    if (ch == '\n') {
        new_line();
        return;
    }
    draw_char(ch, kernel.ch_X, kernel.ch_Y);
    kernel.ch_X += 8;
    if (kernel.ch_X >= kernel.framebuffers[0]->width - 5) new_line();
}

void ansi_switch_colour(uint8_t colour_code) {
    uint32_t ansi_colours[] = {
        0x808080, // grey/black
        0xFF0000, // red
        0x00FF00, // green
        0xFFBF00, // yellow
        0x0000FF, // blue
        0xBF40BF, // magenta
        0x00FFFF, // cyan
        0xFFFFFF  // white
    };
    kernel.fg_colour = ansi_colours[colour_code];
}

void write_string(const char *str) {
    while (*str) {
        if (*str == '\e') {
            ansi_switch_colour(str[5] - 48);
            str += 7;
            continue;
        }
        write_char(*str);
        str++;
    }
}

void clear_screen() {
    for (uint64_t x = 0; x < kernel.framebuffers[0]->width; x++) {
        for (uint64_t y = 0; y < kernel.framebuffers[0]->height; y++) {
            draw_pixel(x, y, kernel.bg_colour);
            kernel.ch_X = 5;
            kernel.ch_Y = 5;
        }
    }
    swap_framebuffers();
}