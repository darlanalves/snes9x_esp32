/*
 * Pico Engine - a library for the Pico Held 2 handheld
 *
 * Copyright (C) 2023 - 2025 Daniel Kammer (daniel.kammer@web.de)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef BLITTER_H
#define BLITTER_H

/* ========================== includes ========================== */
#include "../typedefs.h"

/* ======================== definitions ========================= */
#define BLIT_NO_ALPHA -1   // No color is transparent

typedef enum {
  BLIT_FLIP_NONE = 0,  /**< @brief do not flip the image */
  BLIT_FLIP_HORI = 1,  /**< @brief flip horizontally */
  BLIT_FLIP_VERT = 2,  /**< @brief flip vertically */
  BLIT_FLIP_ALL  = 4,   /**< @brief flip both horizontally and vertically */
} blit_flip_options_t ; 

/* ====================== function declarations ====================== */

/**
 * @brief  Blits a source buffer to a destination buffer.
 *
 * @note   alpha must equal either `BLIT_NO_ALPHA` (no alpha channel) or the
 *         color that shall be transparent
 *
 * @param[in] kx: x coordinate of the upper left corner
 * @param[in] ky: y coordinate of the upper left corner
 * @param[in] alpha: color which is NOT being drawn (`BLIT_NO_ALPHA` for no transparency)
 * @param[in] src: ptr to the source buffer that is supposed to be blitted
 * @param[in] dst: ptr to destination buffer where the source buffer shall be blitted to
 */
void blit_buf(coord_t kx,
              coord_t ky,
              int32_t alpha,
              gbuffer_t src,
              gbuffer_t dst);

/**
 * @brief  Blits a source buffer to a destination buffer and blends them together (50% each).
 *
 * @note   alpha must equal either `BLIT_NO_ALPHA` (no alpha channel) or the
 *         color that shall be transparent
 *
 * @param[in] kx: x coordinate of the upper left corner
 * @param[in] ky: y coordinate of the upper left corner
 * @param[in] alpha: color which is NOT being drawn (`BLIT_NO_ALPHA` for no transparency)
 * @param[in] src: ptr to the source buffer that is supposed to be blitted
 * @param[in] dst: ptr to destination buffer where the source buffer shall be blitted to
 */
 #if LCD_COLORDEPTH==16
void blit_buf_blend(coord_t kx,
                    coord_t ky,
                    int32_t alpha,
                    int opacity,
                    gbuffer_t src,
                    gbuffer_t dst);
#endif                      

#endif // BLITTER_H
