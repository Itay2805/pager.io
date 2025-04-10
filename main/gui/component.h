#pragma once

#include <stdint.h>

typedef struct component {
    /**
     * Render a single row of the component
     */
    void (*render)(struct component* component, uint32_t* framebuffer_row, uint16_t y, uint16_t x0, uint16_t x1);

    /**
     * The location of the component
     */
    int16_t left;
    int16_t top;
    int16_t right;
    int16_t bottom;

    /**
     * Component specific data
     */
    union {
        struct {
            uint16_t color;
        } rect;
    } data;
} component_t;

/**
 * The render callback used to draw text
 */
void component_render_text(component_t* component, uint32_t* framebuffer_row, uint16_t y, uint16_t x0, uint16_t x1);

/**
 * The render callback used to draw a rect
 */
void component_render_rect(component_t* component, uint32_t* framebuffer_row, uint16_t y, uint16_t x0, uint16_t x1);
