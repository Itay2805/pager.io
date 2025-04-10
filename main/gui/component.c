#include "component.h"

void render_rect(component_t* component, uint32_t* framebuffer_row, uint16_t y, uint16_t x0, uint16_t x1) {
    uint16_t color = component->data.rect.color;
    uint32_t color_pair = (color << 16) | color;
    if (x0 % 2 == 1) {
        ((uint16_t*)framebuffer_row)[x0] = color;
    }
    for (int x = x0/2; x < x1/2; x++) {
        framebuffer_row[x] = color_pair;
    }
    if (x1 % 2 == 1) {
        ((uint16_t*)framebuffer_row)[x1-1] = color;
    }
}

uint32_t font_buffer[16/4] = {
    0x10000000, 0xc6c66c38, 0xc6c6c6fe, 0x000000c6
};

void render_text(component_t* component, uint32_t* framebuffer_row, uint16_t y, uint16_t x0, uint16_t x1) {
    int16_t font_y = (y - component->top) / 2;
    if (font_y < 0 || font_y >= 16) {
        return;
    }
    uint8_t font_row = *((uint8_t*)font_buffer + font_y);
    for (int x = x0; x < x1; x++) {
        uint8_t font_col = ((x - x0) / 2) % 8;
        uint8_t font_bit = (font_row >> font_col) & 1;
        if (font_bit) {
            ((uint16_t*)framebuffer_row)[x] = component->data.rect.color;
        }
    }
}
