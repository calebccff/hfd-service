/*
 * Copyright 2020 UBports foundation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Caleb Connolly <caleb@connolly.tech>
 */

#pragma once

#include <linux/input.h>

#include "vibrator.h"
#include "udev/udev-cpp.h"

#define BITS_TO_LONGS(x) \
        (((x) + 8 * sizeof (unsigned long) - 1) / (8 * sizeof (unsigned long)))

// This is a hack until we have a way of querying devices
// to see if they support force feedback (maybe try ALL of them???)
#define HAPTICS_EVENT_NUM 0

namespace hfd {
class VibratorFF : public Vibrator {

public:
    VibratorFF();
    ~VibratorFF();

    static bool usable();
    static Udev::UdevDevice getFirstFFDevice();
protected:
    void configure(State state, int durationMs) override;
    
private:
    struct ff_effect effect;
    const std::string devpath = "/dev/input/event0";
    int fd;
    Udev::UdevDevice m_device;
};
}
