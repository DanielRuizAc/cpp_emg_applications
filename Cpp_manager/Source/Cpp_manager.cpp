// Include standard libraries
#include "stdafx.h"
#include <process.h>
#include <chrono>

#include "bts_bm_manager.h"
// Include BioDAQ template libraries (registration has been done!)

#import "mscorlib.tlb"
#import "bts.biodaq.drivers.tlb"
#import "bts.biodaq.core.tlb"

using namespace std;
using namespace std::chrono;
using namespace mscorlib;
using namespace bts_biodaq_core;
using namespace bts_biodaq_drivers;

unsigned int __stdcall _threadPollData(void* pvInstance);
unsigned int threadPollData();

int _tmain(int argc, _TCHAR* argv[])
{
	bts_bm_manager btsManager;
	printf("Create a USB COM port where BioDAQ device is connected...\n");
	if (!btsManager.ConnectionCOMPort(3, BaudRate_baud230400)) {
		printf("Problem connecting device");
		_gettch();
		return 0;
	}
	printf("Configuring the parameters of the data aquisition\n");
	if (!btsManager.ConfigureAquisition(CodingType_Raw, false, EMGChannelRangeCodes_Gain1_5mV, 50)) {
		printf("Problem configuring the aquisition");
		btsManager.Clean();
		_gettch();
		return 0;
	}

	printf("Arming and starting BioDAQ device...\n");
	if (!btsManager.ArmStart()) {
		printf("Starting aquisition");
		btsManager.Clean();
		_gettch();
		return 0;
	}

	Sleep(420);

	long index = 0;
	auto begin = high_resolution_clock::now();
	int last = 0;
	for (int i = 0; i < 400; i++) {
		// auto start = high_resolution_clock::now();
		btsManager.Read(index,100);
		// auto stop = high_resolution_clock::now();
		// auto duration = duration_cast<microseconds>(stop - start);
		// printf("time consumed: %d\n", (int)duration.count());
		index += 100;
		Sleep(80);
		auto now = high_resolution_clock::now();
		auto duration = duration_cast<milliseconds>(now - begin);
		while (duration.count() <= last + 100) {
			now = high_resolution_clock::now();
			duration = duration_cast<milliseconds>(now - begin);
		}
		last += 100;
	}

	if (!btsManager.Stop()) {
		printf("Problem stopping");
	}

	btsManager.Clean();

	return 0;
}