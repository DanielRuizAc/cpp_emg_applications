#include "bts_freeemg_manage.h"
#include <chrono>

using namespace std;
using namespace std::chrono;

int _tmain(int argc, _TCHAR* argv[])
{
	bts_manage_tools::bts_bm_manager btsManager;
	
	printf("Create a USB COM port where BioDAQ device is connected...\n");
	if (!btsManager.ConnectionCOMPort(3)) {
		printf("Problem connecting device");
		_gettch();
		return 0;
	}

	printf("Configuring the parameters of the data aquisition\n");
	if (!btsManager.ConfigureAquisition(true, 25)) {
		printf("Problem configuring the aquisition");
		btsManager.Clean();
		_gettch();
		return 0;
	}

	Sleep(1000);

	btsManager.PrintSensorData();

	printf("Arming and starting BioDAQ device...\n");
	if (!btsManager.ArmStart()) {
		printf("Starting aquisition");
		btsManager.Clean();
		_gettch();
		return 0;
	}

	Sleep(405);


	/*long index = 0;
	auto begin = high_resolution_clock::now();
	int last = 0;

	for (int i = 0; i < 200; i++) {
		// auto start = high_resolution_clock::now();
		btsManager.Read_basic(index, 100);
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
	*/

	auto begin = high_resolution_clock::now();
	int last = 0;

	bts_manage_tools::batch queue_batch;
	vector<bts_manage_tools::sample> allSamples(0);

	for (int i = 0; i < 600*4; i++) {
	    queue_batch = btsManager.CompleteDequeue(high_resolution_clock::now());
		if (queue_batch.channelsData.size() > 0) {
			printf("Data obtained \n");
		}
		Sleep(50);
		auto now = high_resolution_clock::now();
		auto duration = duration_cast<milliseconds>(now - begin);
		while (duration.count() <= last + 100) {
			now = high_resolution_clock::now();
			duration = duration_cast<milliseconds>(now - begin);
		}
		last += 100;
		allSamples.insert(allSamples.end(), queue_batch.channelsData[6].begin(), queue_batch.channelsData[6].end());
	}

	if (!btsManager.Stop()) {
		printf("Problem stopping");
	}

	btsManager.Clean();

	vector<bts_manage_tools::sample> samples = queue_batch.channelsData[6];
	printf("%d \n\n", (int)(allSamples.size()));
	vector<bts_manage_tools::sample>::iterator ptr2;
	for (ptr2 = samples.begin(); ptr2 < samples.end(); ptr2++)
		printf("\tChannel [%d], sample index: %d, value: %f V at time %d ms \n", 7, (int)(ptr2->index), ptr2->value, duration_cast<milliseconds>(ptr2->timestamp - begin));

	for (ptr2 = samples.begin() + 1; ptr2 < samples.end(); ptr2++) {
		if (ptr2->index - 1 != (ptr2-1)->index) {
			printf("Missing index %d \n", (int)(ptr2->index - 1));
		}
	}


}
