#include "bts_freeemg_manage.h"
#include <chrono>
#include <stdio.h>
#include <tchar.h>
#include "boost/json.hpp"
#include "boost/json/src.hpp"
#include "boost/chrono.hpp"
#include <boost/asio.hpp>
#include <iostream>
#include <fstream>

using namespace std::chrono;

int _tmain(int argc, _TCHAR* argv[])
{
	// return 0;
	bts_manage_tools::bts_bm_manager btsManager;

	printf("Create a USB COM port where BioDAQ device is connected...\n");
	if (!btsManager.ConnectionCOMPort(4)) {
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

	Sleep(100);


	auto begin = high_resolution_clock::now();
	int last = 0;

	auto start_ts = system_clock::now();
	auto start_ts_t = system_clock::to_time_t(start_ts);
	tm local_tm = *localtime(&start_ts_t);

	std::cout << local_tm.tm_year + 1900 << '-';
	std::cout << local_tm.tm_mon + 1 << '-';
	std::cout << local_tm.tm_mday << ' ' << std::endl;

	std::string s_tr_time = std::to_string(local_tm.tm_year + 1900) + '-' + std::to_string(local_tm.tm_mon + 1);
	s_tr_time =+ "-" + local_tm.tm_mday;


	bts_manage_tools::batch queue_batch;
	vector<bts_manage_tools::sample> allSamples(0);
	boost::asio::io_context io;
	// const boost::chrono::steady_clock::time_point initial = boost::chrono::high_resolution_clock::now();

	std::ofstream outfile;
	// outfile.open("Batch.txt");
	outfile.open(s_tr_time+".txt");

	boost::asio::steady_timer t(io);
	t.expires_after(boost::asio::chrono::milliseconds(100));
	for (int i = 0; i < 600*4; i++) {
		boost::json::object json_batch;
		auto json_ts = boost::chrono::high_resolution_clock::now();
		json_batch["TimeStamp"] = boost::chrono::to_string(json_ts);
		queue_batch = btsManager.CompleteDequeue(high_resolution_clock::now());
		if (queue_batch.channelsData.size() > 0) {
			std::printf("Data obtained \n");
		}
		// Sleep(50);
		// auto now =  high_resolution_clock::now();
		// auto duration = duration_cast<milliseconds>(now - begin);
		// while (duration.count() <= last + 100) {
		// 	now = high_resolution_clock::now();
		// 	duration = duration_cast<milliseconds>(now - begin);
		// }


		// last += 100;
		boost::json::array json_channels;
		std::map<int,std::vector<bts_manage_tools::sample>>::iterator iter_channels;
		for (iter_channels = queue_batch.channelsData.begin(); iter_channels != queue_batch.channelsData.end(); iter_channels++) {
			boost::json::object channel_batch;
			int ch = iter_channels->first;
			channel_batch["Channel"] = ch;
			unsigned int sz = iter_channels->second.size();
			boost::json::array data_array;
			for (int j = 0; j < sz; j++) {
				boost::json::object data_obj;
				data_obj["index"] = iter_channels->second[j].index;
				data_obj["value"] = iter_channels->second[j].value;
				// data_obj["TS"] = boost::chrono::to_string(iter_channels->second[j].timestamp);
				data_array.push_back(data_obj);
			}
			channel_batch["data"] = data_array;
			json_channels.push_back(channel_batch);
		}


		json_batch["data"] = json_channels;
		// allSamples.insert(allSamples.end(), queue_batch.channelsData[6].begin(), queue_batch.channelsData[6].end());

		outfile << json_batch << std::endl;
		printf("Duration %d \n", boost::chrono::duration_cast<boost::chrono::milliseconds>(boost::chrono::high_resolution_clock::now() - json_ts).count());
//		if (i == 99) std::cout << boost::json::serialize(json_batch) << std::endl;

		t.wait();
		t.expires_at(t.expires_at() + boost::asio::chrono::milliseconds(100));
		// std::cout << boost::json::serialize(example) << std::endl;
	}

	if (!btsManager.Stop()) {
		printf("Problem stopping");
	}

	btsManager.Clean();
	outfile.close();
}
