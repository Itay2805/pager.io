#pragma once

#include "component.h"

/**
 * Setup the renderer, call this from the main task
 */
void init_renderer(void);

/**
 * Blit the given framebuffer into the main framebuffer, this operation
 * is sync, and will wait until the framebuffer is drawn
 */
void renderer_blit(void* framebuffer);

// TODO: synchronize the event loop with the render loop or something
