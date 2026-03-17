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
 
#include <math.h>

#include "blitter.h"
#include "gbuffers.h"

#if LCD_COLORDEPTH==16
void blit_buf_blend(coord_t kx,       // coordinates of upper left corner
                    coord_t ky,
                    int32_t alpha,
                    int opacity,      // opacity
                    gbuffer_t src,    // pointer to source buffer
                    gbuffer_t dst) {  // pointer to destination buffer

  if ((opacity < 0) || (opacity > 100))
    return; // invalid

  uint16_t srcBufWidth = gbuf_get_width(src);
  uint16_t srcBufHeight = gbuf_get_height(src);
  uint16_t dstBufWidth = gbuf_get_width(dst);
  uint16_t dstBufHeight = gbuf_get_height(dst);

  // area is out of destination buffer
  if (kx >= dstBufWidth || (kx + srcBufWidth < 0) || ky >= dstBufHeight || (ky + srcBufHeight < 0))
    return;

  // check if the blitting area needs to be cropped
  uint16_t start_x = 0;
  uint16_t start_y = 0;
  uint16_t end_x = 0;
  uint16_t end_y = 0;

  uint32_t cpybuf_s = 0;

  if (kx < 0) {
    start_x = 0;
    cpybuf_s = -kx;
  } else {
    start_x = kx;
  }

  if (ky < 0) {
    start_y = 0;
    cpybuf_s += (-ky) * srcBufWidth;
  } else {
    start_y = ky;
  }

  if (kx + srcBufWidth > dstBufWidth)
    end_x = dstBufWidth;
  else
    end_x = kx + srcBufWidth;

  if (ky + srcBufHeight > dstBufHeight)
    end_y = dstBufHeight;
  else
    end_y = ky + srcBufHeight;

  // prepare start values
  uint32_t cpybuf_d = start_y * dstBufWidth + start_x;

  // copy the buffer
  if (alpha == BLIT_NO_ALPHA) {
    for (coord_t y = start_y; y < end_y; y++) {
      for (coord_t x = start_x; x < end_x; x++) {
        // apparantly the optimizer gets this faster
        // than *dst = *src (unlike the ESP-IDF compiler)
        uint16_t dst_col = dst.data[cpybuf_d];
        uint16_t src_col = src.data[cpybuf_s];

        uint8_t r1 = (dst_col >> 11) & 0x1f;
        uint8_t g1 = (dst_col >> 5) & 0x3f;
        uint8_t b1 = (dst_col & 0x1f);
        uint8_t r2 = (src_col >> 11) & 0x1f;
        uint8_t g2 = (src_col >> 5) & 0x3f;
        uint8_t b2 = (src_col & 0x1f);

        dst.data[cpybuf_d] = (r1 + r2) / 2 << 11 | (g1 + g2) / 2 << 5 | (b1 + b2) / 2;

        cpybuf_s++;
        cpybuf_d++;
      }
      cpybuf_s += srcBufWidth - (end_x - start_x);
      cpybuf_d += dstBufWidth - (end_x - start_x);
    }
  } else {
    for (coord_t y = start_y; y < end_y; y++) {
      for (coord_t x = start_x; x < end_x; x++) {
        // the branch condition slows this down signifcantly
        if (src.data[cpybuf_s] != alpha) {
          uint16_t dst_col = dst.data[cpybuf_d];
          uint16_t src_col = src.data[cpybuf_s];

          uint8_t r1 = (dst_col >> 11) & 0x1f;
          uint8_t g1 = (dst_col >> 5) & 0x3f;
          uint8_t b1 = (dst_col & 0x1f);
          uint8_t r2 = (src_col >> 11) & 0x1f;
          uint8_t g2 = (src_col >> 5) & 0x3f;
          uint8_t b2 = (src_col & 0x1f);

          int inv_o = 100 - opacity;

          dst.data[cpybuf_d] = ((r1 * inv_o + r2 * opacity) / 100) << 11 | ((g1 * inv_o + g2 * opacity) / 100) << 5 | (b1 * inv_o + b2 * opacity) / 100;
        }

        cpybuf_s++;
        cpybuf_d++;
      }
      cpybuf_s += srcBufWidth - (end_x - start_x);
      cpybuf_d += dstBufWidth - (end_x - start_x);
    }
  }

}  // blitBuf
#endif

void blit_buf(coord_t kx,       // coordinates of upper left corner
              coord_t ky,
              int32_t alpha,
              gbuffer_t src,    // pointer to source buffer
              gbuffer_t dst) {  // pointer to destination buffer

  uint16_t srcBufWidth = gbuf_get_width(src);
  uint16_t srcBufHeight = gbuf_get_height(src);
  uint16_t dstBufWidth = gbuf_get_width(dst);
  uint16_t dstBufHeight = gbuf_get_height(dst);

  // area is out of destination buffer
  if (kx >= dstBufWidth || (kx + srcBufWidth < 0) || ky >= dstBufHeight || (ky + srcBufHeight < 0))
    return;

  // check if the blitting area needs to be cropped
  uint16_t start_x = 0;
  uint16_t start_y = 0;
  uint16_t end_x = 0;
  uint16_t end_y = 0;

  uint32_t cpybuf_s = 0;

  if (kx < 0) {
    start_x = 0;
    cpybuf_s = -kx;
  } else {
    start_x = kx;
  }

  if (ky < 0) {
    start_y = 0;
    cpybuf_s += (-ky) * srcBufWidth;
  } else {
    start_y = ky;
  }

  if (kx + srcBufWidth > dstBufWidth)
    end_x = dstBufWidth;
  else
    end_x = kx + srcBufWidth;

  if (ky + srcBufHeight > dstBufHeight)
    end_y = dstBufHeight;
  else
    end_y = ky + srcBufHeight;

  // prepare start values
  uint32_t cpybuf_d = start_y * dstBufWidth + start_x;

  // copy the buffer
  if (alpha == BLIT_NO_ALPHA) {
    for (coord_t y = start_y; y < end_y; y++) {
      for (coord_t x = start_x; x < end_x; x++) {
        // apparantly the optimizer gets this faster
        // than *dst = *src (unlike the ESP-IDF compiler)
        dst.data[cpybuf_d] = src.data[cpybuf_s];

        cpybuf_s++;
        cpybuf_d++;
      }
      cpybuf_s += srcBufWidth - (end_x - start_x);
      cpybuf_d += dstBufWidth - (end_x - start_x);
    }
  } else {
    for (coord_t y = start_y; y < end_y; y++) {
      for (coord_t x = start_x; x < end_x; x++) {
        // the branch condition slows this down signifcantly
        if (src.data[cpybuf_s] != alpha)
          dst.data[cpybuf_d] = src.data[cpybuf_s];

        cpybuf_s++;
        cpybuf_d++;
      }
      cpybuf_s += srcBufWidth - (end_x - start_x);
      cpybuf_d += dstBufWidth - (end_x - start_x);
    }
  }

}  // blitBlend

