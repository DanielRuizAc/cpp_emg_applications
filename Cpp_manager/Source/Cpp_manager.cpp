// Include standard libraries
#include "stdafx.h"
#include <process.h>
#include <chrono>

#include "bts_bm_manager.h"
// Include BioDAQ template libraries (registration has been done!)
#include "MatlabEngine.hpp"
#include "MatlabDataArray.hpp"

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

	vector<__int64> xvals(0);
	vector<float> yvals(0);

	std::unique_ptr<matlab::engine::MATLABEngine> matlabPtr = matlab::engine::startMATLAB();
	matlab::data::ArrayFactory factory;

	yvals.push_back(0.0f);
	Sleep(5000);
	// Create arguments for the call to Matlab's plot function
	std::vector<matlab::data::Array> args({
		factory.createArray({ yvals.size(), 1 }, yvals.begin(), yvals.end()),
		factory.createCharArray(std::string("r-"))
		});
	// Invoke the plot command
	matlab::data::Array line = matlabPtr->feval(u"plot", args);

	yvals.pop_back();

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
		vector<sample> sampl = btsManager.Read_vector(last, 100, 4);
		vector<sample>::iterator ptr;
		cout << "The vector elements are : ";
		for (ptr = sampl.begin(); ptr < sampl.end(); ptr++)
			yvals.push_back(ptr->value), xvals.push_back(ptr->index);


		std::vector<matlab::data::Array> args_realt({
			line,
			factory.createCharArray(std::string("YData")),
			factory.createArray({ yvals.size(), 1 }, yvals.begin(), yvals.end()),
			factory.createCharArray(std::string("XData")),
			factory.createArray({ xvals.size(), 1 }, xvals.begin(), xvals.end())
			});
		matlabPtr->feval(u"set", args_realt);
		matlabPtr->eval(u"drawnow");

		// auto start = high_resolution_clock::now();
		//btsManager.Read(index,100);
		// auto stop = high_resolution_clock::now();
		// auto duration = duration_cast<microseconds>(stop - start);
		// printf("time consumed: %d\n", (int)duration.count());
		index += 100;
		Sleep(60);
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