// bts_xsens_multitrial.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "bts_freeemg_manage.h"
#include "xsens_basic_lib.h"
#include <fstream>
#include <stdio.h>
#include <tchar.h>

#include "boost/json/src.hpp"
#include "boost/chrono.hpp"
#include "boost/asio.hpp"
#include "boost/thread.hpp"
#include "boost/thread/future.hpp"

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


boost::json::object Obtainframe(XmePose const& p, std::string segNames[], XmeJointArray& j, XmeControl& xme_c) {

	if (p.empty())
	{
		std::printf("No data available\n");
		return boost::json::object();
	}
	int index = 0;
	boost::json::object obj_segm;

	for (XmeKinematicStateArray::const_iterator ptr_segments = p.m_segmentStates.begin();
		ptr_segments < p.m_segmentStates.end();
		ptr_segments++) {

		if (index >= 15) break;
		boost::json::object pos;
		boost::json::object orientation;
		pos["x"] = ptr_segments->m_position[0];
		pos["y"] = ptr_segments->m_position[1];
		pos["z"] = ptr_segments->m_position[2];

		orientation["q1"] = ptr_segments->m_orientation[0];
		orientation["q2"] = ptr_segments->m_orientation[1];
		orientation["q3"] = ptr_segments->m_orientation[2];
		orientation["q4"] = ptr_segments->m_orientation[3];

		obj_segm[segNames[index]] = { {"position", pos}, {"orientation", orientation} };
		index++;
	};

	boost::json::object obj_joint;
	XsMatrix jointVals = xme_c.jointAngles(p);

	for (int i = 0; i < j.size(); i++) {
		const XmeJoint& jnt = j[i];
		boost::json::object joint_data;

		if (jnt.parentConnection().segmentIndex() >= 16 || jnt.childConnection().segmentIndex() >= 15) continue;

		std::string parent_name = segNames[jnt.parentConnection().segmentIndex()];
		std::string child_name = segNames[jnt.childConnection().segmentIndex()];

		joint_data["parent"] = parent_name;
		joint_data["child"] = child_name;
		joint_data["x"] = jointVals[i][0];
		joint_data["y"] = jointVals[i][1];
		joint_data["z"] = jointVals[i][2];

		obj_joint[parent_name + " - " + child_name] = joint_data;
	}
	boost::json::object complete_obj;

	complete_obj["frame_number"] = p.m_frameNumber;
	complete_obj["segments"] = obj_segm;
	complete_obj["joints"] = obj_joint;

	return complete_obj;
}


void thread_dequeue(boost::json::object* json_msg, bts_manage_tools::bts_bm_manager& Mngr, boost::chrono::steady_clock::time_point initial) {
	auto batch_ts = boost::chrono::high_resolution_clock::now();
	bts_manage_tools::batch queue_batch = Mngr.CompleteDequeue();
	boost::json::object bts_batch = CreateJsonBatch(queue_batch, batch_ts, initial);
	*json_msg = bts_batch;
	printf("Duration %d \n", boost::chrono::duration_cast<boost::chrono::milliseconds>(boost::chrono::high_resolution_clock::now() - batch_ts).count());
}

int main()
{
	xsens_manage_tools::xsensManager xsensManager("Npose", "UpperBodyNoHands");
	xsensManager.ReviewStatus();
	xsensManager.ConfigurePatient();

	bool scan = xsensManager.scanMode();
	if (scan) {
		std::cout << "In scan mode\n";
	}
	else {
		std::cout << "out of scan mode!\n";
	}
	while (xsensManager.IsScanning()) {
		Sleep(100);
	}

	waitForKeyPress();
	xsensManager.ReviewStatus();
	
	waitForKeyPress();
	xsensManager.ConfigurePatient();

	bool userQuit = false;
	resetKbhit();

	while (!userQuit)
	{
		std::cout << "Are you ok with the calibration performed?" << std::endl;
		switch (_getch()) {
		case 'y':
		case 'Y':
			userQuit = true;
			break;
		default:
			std::cout << "Making Again" << std::endl;
			xsensManager.Calibrate();
			break;
		}
	}


	bts_manage_tools::bts_bm_manager btsManager;
	resetKbhit();
	printf("Create a USB COM port where BioDAQ device is connected...\n");
	if (!btsManager.ConnectionCOMPort(4)) {
		printf("Problem connecting device");
		waitForKeyPress();
		return 0;
	}

	printf("Configuring the parameters of the data aquisition\n");
	if (!btsManager.ConfigureAquisition(true, 25)) {
		printf("Problem configuring the aquisition");
		btsManager.Clean();
		waitForKeyPress();
		return 0;
	}

	Sleep(1000);

	btsManager.PrintSensorData();
	waitForKeyPress();

	xsensManager.StopRecording();

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

		std::string s_tr_time = std::to_string(local_tm.tm_year + 1900) + '-' + std::to_string(local_tm.tm_mon + 1);
		s_tr_time = s_tr_time + "-" + std::to_string(local_tm.tm_mday);
		s_tr_time = s_tr_time + "-" + std::to_string(local_tm.tm_hour) + std::to_string(local_tm.tm_min) + std::to_string(local_tm.tm_sec);

		std::ofstream outfile;
		// outfile.open("Batch.txt");
		outfile.open(s_tr_time + ".txt");
		xsensManager.CreateMVNFile(XsString(s_tr_time) + XsString(".mvn"));

		std::cout << "Starting a 2 minutes trial" << std::endl;

		printf("Arming and starting BioDAQ device...\n");
		if (!btsManager.ArmStart()) {
			printf("Starting aquisition");
			btsManager.Clean();
			waitForKeyPress();
			return 0;
		}

		(void)xsensManager.ReadUnreadPoses();

		xsensManager.StartRecording();
		xsensManager.ToogleTimePoseMode();

		boost::asio::steady_timer t(io);
		t.expires_after(boost::asio::chrono::milliseconds(200));
		t.wait();

		for (int counter = 0; counter < 2*300; counter++) {
			auto batch_ts = boost::chrono::high_resolution_clock::now();
			boost::json::object bts_batch;
			boost::thread th(&thread_dequeue, &bts_batch, btsManager, initial);
			std::vector<XmePose> records = xsensManager.ReadUnreadPoses();
			boost::json::object xsens_batch;
			boost::json::array data_array;

			for (unsigned short i = 0; i < records.size(); i++) {
				boost::json::object json_frame = Obtainframe(records[i], xsensManager.segmentNames_str, xsensManager.joints, xsensManager.getXmeControl());
				data_array.push_back(json_frame);
			}
			xsens_batch["data"] = data_array;
			if (xsensManager.getXmeControl().status().isProcessing()) printf("Is processing \n");


			th.join();
			boost::json::object json_batch = { {"xsens", xsens_batch}, { "bts", bts_batch}, {"TimeStamp", XsTimeStamp::now().toString().c_str()} };

			outfile << boost::json::serialize(json_batch) << std::endl;
			t.wait();
			t.expires_at(t.expires_at() + boost::asio::chrono::milliseconds(200));
		}
		if (!btsManager.Stop()) {
			printf("Problem stopping");
		}

		xsensManager.StopRecording();
		xsensManager.ToogleTimePoseMode();
		xsensManager.CloseMVNFile();
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
	xsensManager.~xsensManager();
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
