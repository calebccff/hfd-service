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

#include "vibrator-ff.h"

#include <fstream>
#include <string>
#include <iostream>
#include <unistd.h>
#include <vector>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>

/*
 * This vibrator supports devices using the Kernel Force Feedback API.
 * Mostly Mainline devices.
 */
namespace hfd {

bool VibratorFF::usable() {
	return VibratorFF::getFirstFFDevice().size() > 0;
}

bool inputDeviceSupportsFF(std::string devname) {
	int ret;
	unsigned char features[1 + FF_MAX/8/sizeof(unsigned char)];
	int tempFd = open(devname.c_str(), O_RDWR);
	int request = EVIOCGBIT(EV_FF, sizeof(features)*sizeof(unsigned char));
	bool supported = false;

	ret = ioctl(tempFd, request, &features);

	if (testBit(FF_RUMBLE, features)) {
		std::cout << "FF: '" << devname << "' supports rumble!" << std::endl;
		supported =  true;
	} else {
		std::cout << "FF: '" << devname << "' doesn't support rumble :(" << std::endl;
	}

	ret = close(tempFd);
	if (ret != 0) {
		std::cout << "FF: Failed to close " << tempFd << ": " << ret << std::endl;
	}
	return supported;
}

void VibratorFF::configure(State state, int durationMs) {
	// For now just assume we support rumble, this is bad
	int ret;
	//std::string path = "/dev/input/event0";// + m_device.get_property("KERNEL");
	struct input_event play;
	struct input_event stop;

	if (state == State::On) {
		effect.u.rumble.strong_magnitude = 0x6000; // This should be adjustable
		std::cout << "rumbling with magnitude: " << effect.u.rumble.strong_magnitude << " for " << durationMs << "ms" << std::endl;
		ret = ioctl(fd, EVIOCSFF, &effect);
		if (ret < 0) {
			std::cout << "Failed to upload rumble effect" << std::endl;
			return;
		}
		play.type = EV_FF;
		play.code = effect.id;
		play.value = 1;
		write(fd, (const void*) &play, sizeof(play));
		usleep(durationMs * 1000);
	}
	std::cout << "Stopping rumble" << std::endl;
	stop.type = EV_FF;
	stop.code = effect.id;
	stop.value = 0;
	write(fd, (const void*) &stop, sizeof(stop));
	// remove effect
	// ret = ioctl(fd, EVIOCRMFF, effect.id);
	// if (ret < 0)
	// 		std::cout << "Filed to STOP rumble (oh no)" << std::endl;
}

// This finds the first device that supports force feedback
// and assumes that it supports rumble, which it may not.
// We should also query the device feature flags and be SURE
std::string VibratorFF::getFirstFFDevice() {
	Udev::Udev udev;
	Udev::UdevEnumerate enumerate = udev.enumerate_new();
	std::string path = "";

	return "/dev/input/event0";

	enumerate.add_match_subsystem("input");
	enumerate.scan_devices();
	std::vector<Udev::UdevDevice> devices = enumerate.enumerate_devices();
	std::cout << "FF: Found " << devices.size() << " input devices" << std::endl;
	for(int i = 0; i < devices.size(); i++) {
		const auto properties = devices.at(i).get_properties();
		if (properties.find("DEVNAME") != properties.end()) {
			auto temp = devices.at(i).get_properties().at("DEVNAME");
			if (inputDeviceSupportsFF(temp)) {
				path = temp;
				break;
			}
		}
	}
	return path;
}

VibratorFF::VibratorFF(): Vibrator() {
	devname = VibratorFF::getFirstFFDevice();

	effect.type = FF_RUMBLE;
	effect.id = -1;
	effect.u.rumble.strong_magnitude = 0;
	effect.u.rumble.weak_magnitude = 0;

	fd = open(devname.c_str(), O_RDWR);
	if (fd < 0) {
		std::cerr << "Can't open force feedback device path: " << devname << std::endl;
		return;
	}

	configure(State::Off, 0);
}

VibratorFF::~VibratorFF() {
	if (fd > 0) {
		close(fd);
	}
}
}
