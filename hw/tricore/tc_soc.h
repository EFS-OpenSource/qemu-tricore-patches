/*
 * Infineon tc39x SoC System emulation.
 *
 * Copyright (c) 2020 Andreas Konopik
 * Copyright (c) 2020 David Brenken
 * Copyright (c) 2024 Georg Hofstetter <georg.hofstetter@efs-techhub.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef TC39X_SOC_H
#define TC39X_SOC_H

#include "hw/sysbus.h"
#include "target/tricore/cpu.h"
#include "qom/object.h"

#include "hw/tricore/tricore.h"


typedef struct MemmapEntry {
    hwaddr base;
    hwaddr size;
} MemmapEntry;

#endif
