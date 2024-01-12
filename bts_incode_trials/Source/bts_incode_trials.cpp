// bts_incode_trials.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "bts_freeemg_manage.h"
#include <fstream>
#include <stdio.h>
#include <tchar.h>

#include "boost/json/src.hpp"
#include "boost/chrono.hpp"
#include "boost/asio.hpp"
#include "boost/thread.hpp"
#include "boost/thread/future.hpp"
#include "boost/date_time/posix_time/posix_time.hpp"

void resetKbhit()
{
	while (_kbhit())
		(void)_getch();
}

void waitForKeyPress()
{
	resetKbhit();
	(void)_getch();
}

std::string string_format_two_digits(const std::string& format, int number)
{
	int size_s = std::snprintf(nullptr, 0, format.c_str(), number) + 1; // Extra space for '\0'
	if (size_s <= 0) { throw std::runtime_error("Error during formatting."); }
	auto size = static_cast<size_t>(size_s);
	std::unique_ptr<char[]> buf(new char[size]);
	std::snprintf(buf.get(), size, format.c_str(), number);
	return std::string(buf.get(), buf.get() + size - 1); // We don't want the '\0' inside
}


boost::json::object CreateJsonBatch(bts_manage_tools::batch information,
	boost::chrono::steady_clock::time_point batch_time,
	boost::chrono::steady_clock::time_point initial_time) {

	boost::json::object json_batch;
	// json_batch["TimeStamp"] = boost::chrono::to_string(batch_time);
	boost::json::array json_channels;
	std::map<int, std::vector<bts_manage_tools::sample>>::iterator iter_channels;
	for (iter_channels = information.channelsData.begin(); iter_channels != information.channelsData.end(); iter_channels++) {
		boost::json::object channel_batch;
		int ch = iter_channels->first;
		channel_batch["Channel"] = ch;
		unsigned int sz = iter_channels->second.size();
		boost::json::array data_array;
		for (int j = 0; j < sz; j++) {
			boost::json::object data_obj;
			data_obj["index"] = iter_channels->second[j].index;
			data_obj["value"] = iter_channels->second[j].value;
			// data_obj["TS"] = boost::chrono::to_string(initial_time + boost::chrono::milliseconds(iter_channels->second[j].index));
			data_array.push_back(data_obj);
		}
		channel_batch["data"] = data_array;
		json_channels.push_back(channel_batch);
	}
	json_batch["data"] = json_channels;
	return json_batch;
}


void thread_dequeue(boost::json::object* json_msg, bts_manage_tools::bts_bm_manager& Mngr, boost::chrono::steady_clock::time_point initial, long last_index, int n_samples) {
	auto batch_ts = boost::chrono::high_resolution_clock::now();
	bts_manage_tools::batch queue_batch = Mngr.Read_basic(last_index, n_samples); //Mngr.CompleteDequeue();
	boost::json::object bts_batch = CreateJsonBatch(queue_batch, batch_ts, initial);
	*json_msg = bts_batch;
	printf("Duration %d \n", boost::chrono::duration_cast<boost::chrono::milliseconds>(boost::chrono::high_resolution_clock::now() - batch_ts).count());
}

int main()
{
	bts_manage_tools::bts_bm_manager btsManager;
	resetKbhit();
	printf("Create a USB COM port where BioDAQ device is connected...\n");
	if (!btsManager.ConnectionCOMPort(4)) {
		printf("Problem connecting device");
		waitForKeyPress();
		return 0;
	}

	/*
	printf("Configuring the parameters of the data aquisition\n");
	if (!btsManager.ConfigureAquisition(true, 25)) {
		printf("Problem configuring the aquisition");
		btsManager.Clean();
		waitForKeyPress();
		return 0;
	}

	*/

	Sleep(1000);

	btsManager.PrintSensorData();
	waitForKeyPress();

	//    std::cout << "Hello World!\n";

	resetKbhit();
	bool maketrial;
	std::cout << "Would you like to start a new trial?" << std::endl;
	switch (_getch()) {
	case 'y':
	case 'Y':
		maketrial = true;
		break;
	default:
		maketrial = false;
		break;
	}

	boost::asio::io_context io;
	while (maketrial) {
		boost::chrono::steady_clock::time_point initial = boost::chrono::high_resolution_clock::now();
		boost::chrono::system_clock::time_point start_ts = boost::chrono::system_clock::now();
		time_t start_ts_t = boost::chrono::system_clock::to_time_t(start_ts);

		tm local_tm = *localtime(&start_ts_t);

		std::string s_tr_time = std::to_string(local_tm.tm_year + 1900) + '-' + string_format_two_digits("%02d", local_tm.tm_mon + 1); //std::to_string(local_tm.tm_mon + 1);
		s_tr_time = s_tr_time + "-" + string_format_two_digits("%02d" ,local_tm.tm_mday);
		s_tr_time = s_tr_time + "-" + string_format_two_digits("%02d", local_tm.tm_hour) + string_format_two_digits("%02d", local_tm.tm_min) + string_format_two_digits("%02d", local_tm.tm_sec);

		std::ofstream outfile;
		// outfile.open("Batch.txt");
		outfile.open(s_tr_time + ".txt");

		std::cout << "Starting a 4 minutes trial" << std::endl;

		printf("Arming and starting BioDAQ device...\n");
		if (!btsManager.ArmStart()) {
			printf("Starting aquisition");
			btsManager.Clean();
			waitForKeyPress();
			return 0;
		}

		boost::asio::steady_timer t(io);
		t.expires_after(boost::asio::chrono::milliseconds(1000));
		t.wait();

		int delay = 200; //miliseconds
		double update_time = (double) delay / 1000.0;
		double desired_time = 30*60;

		long max_count = (long)(desired_time / update_time);

		for (long counter = 0; counter < max_count; counter++) {
			boost::json::object bts_batch;
			boost::thread th(&thread_dequeue, &bts_batch, btsManager, initial, counter * delay, delay);
			boost::json::object xsens_batch;
			boost::json::array data_array;

			th.join();

			// boost::chrono::system_clock::time_point batch_ts = boost::chrono::system_clock::now();
			// time_t batch_ts_t = boost::chrono::system_clock::to_time_t(batch_ts);
			if (GetKeyState('A') & 0x8000/*Check if high-order bit is set (1 << 15)*/)
			{
				counter = max_count;
			}

			// tm local_tm = *localtime(&batch_ts_t);


			boost::posix_time::ptime now = boost::posix_time::microsec_clock::local_time();
			boost::posix_time::time_duration td = now.time_of_day();
			boost::posix_time::millisec_posix_time_system_config::date_type posix_date = now.date();

			long hours = td.hours();
			long minutes = td.minutes();
			long seconds = td.seconds();
			long milliseconds = td.total_milliseconds() -
				((hours * 3600 + minutes * 60 + seconds) * 1000);


			std::string s_tr_time = std::to_string(posix_date.year()) + '-' + string_format_two_digits("%02d", posix_date.month()); //std::to_string(local_tm.tm_mon + 1);
			s_tr_time = s_tr_time + "-" + string_format_two_digits("%02d", posix_date.day());
			s_tr_time = s_tr_time + "-" + string_format_two_digits("%02d", hours) + string_format_two_digits("%02d", minutes) + string_format_two_digits("%02d", seconds) + string_format_two_digits("%03d", milliseconds);

			boost::json::object json_batch = {{ "bts", bts_batch}, {"TimeStamp", s_tr_time} };

			outfile << boost::json::serialize(json_batch) << std::endl;
			t.wait();
			t.expires_at(t.expires_at() + boost::asio::chrono::milliseconds(delay));
		}
		if (!btsManager.Stop()) {
			printf("Problem stopping");
		}
		outfile.close();

		std::cout << "Would you like to start a new trial?" << std::endl;
		switch (_getch()) {
		case 'y':
		case 'Y':
			maketrial = true;
			break;
		default:
			maketrial = false;
			break;
		}

	}

	std::cout << "Finishing" << std::endl;

	btsManager.Clean();
	waitForKeyPress();
	return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
