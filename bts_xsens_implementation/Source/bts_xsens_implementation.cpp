// bts_xsens_implementation.cpp : Este archivo contiene la función "main". La ejecución del programa comienza y termina ahí.
//
// #include <iostream>
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


boost::json::object Obtainframe(XmePose const& p, char* segNames[15], XmeJointArray& j, XmeControl& xme_c) {
	
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

		if (index>=16) break;
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
		
		if (jnt.parentConnection().segmentIndex() >= 16 || jnt.childConnection().segmentIndex() >= 16) continue;
		
		char* parent_name = segNames[jnt.parentConnection().segmentIndex()];
		char* child_name = segNames[jnt.childConnection().segmentIndex()];

		joint_data["parent"] = parent_name;
		joint_data["child"] = child_name;
		joint_data["x"] = jointVals[i][0];
		joint_data["y"] = jointVals[i][1];
		joint_data["z"] = jointVals[i][2];

		obj_joint[std::string(parent_name) + " - " + std::string(child_name)] = joint_data;
	}
	boost::json::object complete_obj;

	complete_obj["frame_number"] = p.m_frameNumber;
	complete_obj["segments"] = obj_segm;
	complete_obj["joints"] = obj_joint;

	return complete_obj;
}


void thread_dequeue(boost::json::object *json_msg, bts_manage_tools::bts_bm_manager& Mngr, boost::chrono::steady_clock::time_point initial) {
	auto batch_ts = boost::chrono::high_resolution_clock::now();
	bts_manage_tools::batch queue_batch = Mngr.CompleteDequeue();
	boost::json::object bts_batch = CreateJsonBatch(queue_batch, batch_ts, initial);
	*json_msg = bts_batch;
	printf("Duration %d \n",boost::chrono::duration_cast<boost::chrono::milliseconds>(boost::chrono::high_resolution_clock::now()-batch_ts).count());
}



int _tmain(int argc, _TCHAR* argv[])
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
	// xsensManager.CreateMVNFile("completeExperimentBatch.mvn");

	bool userQuit = false;
	resetKbhit();
	/*
	while (!userQuit)
	{
		xsensManager.Calibrate();
		std::cout << "Are you ok with the calibration performed?" << std::endl;
		switch (_getch()) {
		case 'y':
		case 'Y':
			userQuit = true;
			break;
		default:
			std::cout << "Making Again" << std::endl;
			break;
		}
	}
	*/

	bts_manage_tools::bts_bm_manager btsManager;
	resetKbhit();
	printf("Create a USB COM port where BioDAQ device is connected...\n");
	if (!btsManager.ConnectionCOMPort(3)) {
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


	xsensManager.ToogleTimePoseMode();
	// xsensManager.StartRecording();
	printf("Arming and starting BioDAQ device...\n");
	if (!btsManager.ArmStart()) {
		printf("Starting aquisition");
		btsManager.Clean();
		waitForKeyPress();
		return 0;
	}
	Sleep(100);

	boost::asio::io_context io;
	const boost::chrono::steady_clock::time_point initial = boost::chrono::high_resolution_clock::now();

	std::ofstream outfile;
	outfile.open("Batch.txt");
	boost::asio::steady_timer t(io);
	t.expires_after(boost::asio::chrono::milliseconds(200));

	for (int i = 0; i < 100; i++) {
		auto batch_ts = boost::chrono::high_resolution_clock::now();
		boost::json::object bts_batch;
		boost::thread th(&thread_dequeue, &bts_batch, btsManager, initial);
		std::vector<XmePose> records = xsensManager.ReadUnreadPoses();
		boost::json::object xsens_batch;
		// xsens_batch["TimeStamp"] = boost::chrono::to_string();//XsTimeStamp::now().toString().c_str();
		boost::json::array data_array;
		for (unsigned short i = 0; i < records.size(); i++) {
			boost::json::object json_frame = Obtainframe(records[i], xsensManager.segmentNames_ch, xsensManager.joints, xsensManager.getXmeControl());
			data_array.push_back(json_frame);
		}
		xsens_batch["data"] = data_array;

		th.join();
		boost::json::object json_batch = { {"xsens", xsens_batch}, { "bts", bts_batch}};

		outfile << boost::json::serialize(json_batch) << std::endl;
		t.wait();
		t.expires_at(t.expires_at() + boost::asio::chrono::milliseconds(200));
	}

	if (!btsManager.Stop()) {
		printf("Problem stopping");
	}

	xsensManager.ToogleTimePoseMode();
	// xsensManager.StopRecording();
	// xsensManager.CloseMVNFile();
	outfile.close();


	btsManager.Clean();
	outfile.close();

    return 0;
}

// Ejecutar programa: Ctrl + F5 o menú Depurar > Iniciar sin depurar
// Depurar programa: F5 o menú Depurar > Iniciar depuración

// Sugerencias para primeros pasos: 1. Use la ventana del Explorador de soluciones para agregar y administrar archivos
//   2. Use la ventana de Team Explorer para conectar con el control de código fuente
//   3. Use la ventana de salida para ver la salida de compilación y otros mensajes
//   4. Use la ventana Lista de errores para ver los errores
//   5. Vaya a Proyecto > Agregar nuevo elemento para crear nuevos archivos de código, o a Proyecto > Agregar elemento existente para agregar archivos de código existentes al proyecto
//   6. En el futuro, para volver a abrir este proyecto, vaya a Archivo > Abrir > Proyecto y seleccione el archivo .sln
