#include "app_serial_thread.h"

#include "serial_parser.h"
#include "windows_serial.h"

#include <chrono>
#include <iostream>
#include <thread>

void serial_thread(const AppCommandLine &commandLine, AppData *data)
{
	std::cout << "Trying to initialize serial on " << commandLine.port
	          << " at " << commandLine.baud << "bps with the settings:\n"
	          << "\tbyte size: " << commandLine.serialSettings.byte_size << "\n"
	          << "\tparity:    " << commandLine.serialSettings.parity << "\n"
	          << "\tstop bits: " << commandLine.serialSettings.stop_bits << std::endl;

	WindowsSerial serial;
	if (!serial.begin(commandLine.port, commandLine.baud, commandLine.serialSettings)) {
		data->state = AppState::FINISHED;
		data->condition_variables.serial_done.notify_one();

		return;
	}

	data->condition_variables.serial_done.notify_one();

	SerialParser parser(&serial, &(data->sounds));

	while (data->state != AppState::FINISHED) {
		parser.update();
	}

	serial.end();
}
