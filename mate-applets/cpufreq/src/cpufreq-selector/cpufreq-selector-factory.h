/*
 * MATE CPUFreq Applet
 * Copyright (C) 2008 Carlos Garcia Campos <carlosgc@mate.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef CPUFREQ_SELECTOR_FACTORY
#define CPUFREQ_SELECTOR_FACTORY

#include "cpufreq-selector.h"

G_BEGIN_DECLS

CPUFreqSelector *cpufreq_selector_factory_create_selector (guint cpu);

G_END_DECLS

#endif /* CPUFREQ_SELECTOR_FACTORY */
