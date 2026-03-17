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
  
#pragma once

/* ========================== includes ========================== */
// Graphics
#include "power/power.h"
#include "control/control.h"

// Graphics
#include "graphics/gbuffers.h"
#include "graphics/colors.h"
#include "graphics/primitives.h"
#include "graphics/blitter.h"
#include "fonts/fonts.h"

/* ======================== definitions ========================= */

#define PICOENGINE_VERSION 250321

// error messages
#define PE_SUCCESS           0
#define PE_UNKNOWN_ERROR    -1

/* ====================== init ====================== */
int pe_init(pe_config_t pe_cfg);
