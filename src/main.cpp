#include <iostream>

#include "app.h"

#include <MidiFile.h>

int main(int argc, char *argv[])
{
	PianoApp app;

	if (!app.initCommandLine(argc, const_cast<const char **>(argv))) {
		return 1;
	}
	else {
		using namespace smf;
		MidiFile f;
		f.read(app.arguments.midi);

		f.doTimeAnalysis();
		f.linkNotePairs();

		{
			using namespace std;
			int tracks = f.getTrackCount();
			cout << "TPQ: " << f.getTicksPerQuarterNote() << endl;
			if (tracks > 1)
				cout << "TRACKS: " << tracks << endl;
			for (int track = 0; track < tracks; track++) {
				if (tracks > 1)
					cout << "\nTrack " << track << endl;
				cout << "Tick\tSeconds\tDur\tMessage" << endl;
				for (int event = 0; event < f[track].size(); event++) {
					cout << dec << f[track][event].tick;
					cout << '\t' << dec << f[track][event].seconds;
					cout << '\t';
					if (f[track][event].isNoteOn())
						cout << f[track][event].getDurationInSeconds();
					cout << '\t' << hex;
					for (int i = 0; i < f[track][event].size(); i++)
						cout << (int)f[track][event][i] << ' ';
					cout << endl;
				}
			}
		}
	}

	if (!app.initAudio()) {
		app.cleanup();
		std::cerr << "Failed initializing OpenAL." << std::endl;
		return 2;
	}

	if (!app.initSerial()) {
		app.cleanup();
		std::cerr << "Failed initializing serial port." << std::endl;
		return 3;
	}

	if (!app.initGraphics()) {
		app.cleanup();
		std::cerr << "Failed initializing graphics." << std::endl;
		return 4;
	}

	app.mainLoop();

	app.cleanup();

	return 0;
}