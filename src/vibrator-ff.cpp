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

bool inputDeviceSupportsFF(std::string devPath) {
	int ret;
	unsigned char features[1 + FF_MAX/8/sizeof(unsigned char)];
	int tempFd = open(devPath.c_str(), O_RDWR);
	int request = EVIOCGBIT(EV_FF, sizeof(features)*sizeof(unsigned char));
	bool supported = false;

	ret = ioctl(tempFd, request, &features);

	if (testBit(FF_RUMBLE, features)) {
		std::cout << "FF: '" << devPath << "' supports rumble!" << std::endl;
		supported =  true;
	}

	close(tempFd);
	return supported;
}

bool VibratorFF::usable() {
	return VibratorFF::getFirstFFDevice().size() > 0;
}

void VibratorFF::configure(State state, int durationMs) {
	int ret;
	struct input_event play;
	struct input_event stop;

	if (fd < 0) {
		std::cout << "FF: can't play effects" << std::endl;
		return;
	}

	if (state == State::Off) {
		std::cout << "FF: Stopping effects" << std::endl;
		goto stop;
	}

	std::cout << "Rumbling for " << durationMs << "ms" << std::endl;
	effect.u.rumble.strong_magnitude = 0x6000; // This should be adjustable
	ret = ioctl(fd, EVIOCSFF, &effect);
	if (ret < 0) {
		std::cout << "FF: Failed to upload rumble effect" << std::endl;
		close(fd);
		fd = -1;
		return;
	}

	// Create an input event to play the effect uploaded in the constructor
	play.type = EV_FF;
	play.code = effect.id;
	// This is the number of times to play the effect, the rumble effect however
	// continue indefinitely.
	play.value = 1;
	ret = write(fd, (const void*) &play, sizeof(play));
	if (ret < 0) {
		std::cout << "Failed to play rumble" << std::endl;
		return;
	}

	// Vibrate for the correct duration.
	usleep(durationMs * 1000);

stop:
	// Now create a stop event to stop the effect playing
	stop.type = EV_FF;
	stop.code = effect.id;
	// This will override the play event and call the effect handler
	// in the driver with all values zero'd out.
	stop.value = 0;
	ret = write(fd, (const void*) &stop, sizeof(stop));
	if (ret < 0)
		std::cout << "FF: Failed to STOP rumble (oh no)" << std::endl;
}

// This finds the first device that supports force feedback
// and assumes that it supports rumble, which it may not.
// We should also query the device feature flags and be SURE
std::string VibratorFF::getFirstFFDevice() {
	Udev::Udev udev;
	Udev::UdevEnumerate enumerate = udev.enumerate_new();
	std::string path = "";

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
			}
		}
	}
	return path;
}

VibratorFF::VibratorFF(): Vibrator() {
	devname = VibratorFF::getFirstFFDevice();
	int ret;

	effect.type = FF_RUMBLE;
	effect.id = -1;
	effect.u.rumble.strong_magnitude = 0; // This should be adjustable
	effect.u.rumble.weak_magnitude = 0;

	std::cout << "FD: Opening device '" << devname << "'" << std::endl;
	fd = open(devname.c_str(), O_RDWR);
	if (fd < 0) {
		std::cerr << "FF: Can't open force feedback device: " << devname << std::endl;
		return;
	}

	configure(State::Off, 0);
}

VibratorFF::~VibratorFF() {
	std::cout << "FF: Destructor called" << std::endl;
	int ret;
	if (fd > 0) {
		std::cout << "FF: Removing effect " << effect.id << " and closing fd" << std::endl;
		ret = ioctl(fd, EVIOCRMFF, effect.id);
		if (ret < 0)
			std::cout << "FF: Failed to remove effect" << std::endl;
		close(fd);	
	}
}
}
