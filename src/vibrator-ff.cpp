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
	return access("/sys/class/input/event0", F_OK ) != -1;
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

Udev::UdevDevice  VibratorFF::getFirstFFDevice() {
	Udev::Udev udev;
	Udev::UdevDevice device;
	Udev::UdevEnumerate enumerate = udev.enumerate_new();

	enumerate.add_match_subsystem("input");
	enumerate.add_match_sysattr("capabilities/ff", "107030000");
	enumerate.scan_devices();
	std::vector<Udev::UdevDevice> devices = enumerate.enumerate_devices();
	std::cout << "FF: Found " << devices.size() << " devices" << std::endl;
	if (devices.size() > 0) {
		std::cout << "Device " << devices.at(0).get_syspath() << " supports ff" << std::endl;
		device = udev.device_from_syspath(devices.at(0).get_syspath());
	}
	return device;
	
	// for (size_t i = 0; i < devices.size(); i++)
	// {
	// 	std::string attr = devices.at(i).get_sysattr("capabilities/ff");
	// 	std::cout << "capabilities/ff = " << attr << std::endl;
	// 	std::cout << "Testing if device '" << devices.at(i).get_devpath() << "' supports FF" << std::endl;

	// 	try { 
	// 		if(attr.compare("0") == 0) { // Need to actually run the get_sysattr function.
	// 			std::cout << "FF not supported";
	// 			continue;
	// 		}

	// 		// Construct our device from the syspath, there's probably a better
	// 		// way to avoid dealing with pointers.
	// 		std::cout << devices.at(i).get_syspath() << std::endl;
	// 		device = udev.device_from_syspath(devices.at(i).get_syspath());
	// 		std::cout << "Device " << device.get_devpath() << " supports FF!" << std::endl;
	// 		break;
	// 	} catch(const std::runtime_error ) {
	// 		// swallow
	// 		continue;
	// 	}
	// }
	// return device;
}

VibratorFF::VibratorFF(): Vibrator() {
	//m_device = getFirstFFDevice();
	// Udev::Udev udev;
	// m_device = udev.device_from_syspath("/sys/class/input/event0");

	effect.type = FF_RUMBLE;
	effect.id = -1;
	effect.u.rumble.strong_magnitude = 0;
	effect.u.rumble.weak_magnitude = 0;

	fd = open(devpath.c_str(), O_RDWR);
	if (fd < 0) {
		std::cerr << "Can't open force feedback device path: " << devpath << std::endl;
		return;
	}

	configure(State::Off, 0);
}

VibratorFF::~VibratorFF() {
	close(fd);
}
}
