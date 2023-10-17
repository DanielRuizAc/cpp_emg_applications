// Library_proveXme.cpp : Este archivo contiene la función "main". La ejecución del programa comienza y termina ahí.
//

#include <iostream>
#include <stdio.h>
#include <tchar.h>
#include "xsens_basic_lib.h"
#include "xmecontrolbase.h"
#include "boost/json.hpp"
#include "boost/json/src.hpp"
#include "boost/chrono.hpp"
#include <fstream>

#define MVNFILE "testfile.mvn"


volatile std::atomic_int xsens_wait_msgs = 0;

void resetKbhit()
{
    while (_kbhit())
        (void)_getch();
}

void waitForKeyPress()
{
    resetKbhit();
    // wait for new key press
    (void)_getch();
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

        if (index >= 16) break;
        boost::json::object pos;
        boost::json::object orientation;
        boost::json::object vel;
        boost::json::object acel;

        boost::json::object ang_vel;
        boost::json::object ang_acel;


        pos["x"] = ptr_segments->m_position[0];
        pos["y"] = ptr_segments->m_position[1];
        pos["z"] = ptr_segments->m_position[2];

        orientation["q1"] = ptr_segments->m_orientation[0];
        orientation["q2"] = ptr_segments->m_orientation[1];
        orientation["q3"] = ptr_segments->m_orientation[2];
        orientation["q4"] = ptr_segments->m_orientation[3];

        vel["x"] = ptr_segments->m_velocity[0];
        vel["y"] = ptr_segments->m_velocity[1];
        vel["z"] = ptr_segments->m_velocity[2];

        acel["x"] = ptr_segments->m_acceleration[0];
        acel["y"] = ptr_segments->m_acceleration[1];
        acel["z"] = ptr_segments->m_acceleration[2];

        ang_vel["x"] = ptr_segments->m_angularVelocity[0];
        ang_vel["y"] = ptr_segments->m_angularVelocity[1];
        ang_vel["z"] = ptr_segments->m_angularVelocity[2];

        ang_acel["x"] = ptr_segments->m_angularAcceleration[0];
        ang_acel["y"] = ptr_segments->m_angularAcceleration[1];
        ang_acel["z"] = ptr_segments->m_angularAcceleration[2];

        obj_segm[segNames[index]] = { {"position", pos}, {"velocity", vel}, {"acceleration", acel}, {"orientation", orientation}, {"ang_vel", ang_vel}, {"ang_accel", ang_acel}};
        index++;
    };

    boost::json::object obj_joint;
    XsMatrix jointVals = xme_c.jointAngles(p);

    for (int i = 0; i < j.size(); i++) {
        const XmeJoint& jnt = j[i];
        boost::json::object joint_data;

        if (jnt.parentConnection().segmentIndex() >= 16 || jnt.childConnection().segmentIndex() >= 16) continue;

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



int _tmain(int argc, _TCHAR* argv[])
{   
    XsTimeStamp start_program = XsTimeStamp::now();
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

    XsString st = start_program.toString();
    st.replaceAll("/", "");
    st.replaceAll(":", "");
    std::cout << st + XsString(".mvn") << std::endl;
    std::cout << st << std::endl;
    xsensManager.CreateMVNFile(st + XsString(".mvn"));
    st = st + ".txt";
    // Manager.OpenMVNFile(MVNFILE);


    std::cout << "Scan Finished!\n";

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

    // Manager.OpenMVNFile("testfile.mvn");


    std::ofstream outfile; 
    outfile.open(st.c_str());
    int64_t difference = 100;
    xsensManager.StartRecording();
    xsensManager.ToogleTimePoseMode();
    XsTimeStamp start = XsTimeStamp::now();

    for (unsigned short i = 0; i < 200; i++) {
        while (start.elapsedToNow() < difference);
        // while (xsens_wait_msgs < 10);
        difference += 100;
        // XsTime::msleep(1000);
        std::vector<XmePose> records = xsensManager.ReadUnreadPoses();
        std::cout << "Number of records: " << records.size() << std::endl;
        
        if (records.size() > 0) {
            boost::json::object json_batch;
            json_batch["TimeStamp"] = XsTimeStamp::now().toString().c_str();
            boost::json::array data_array;
            for (unsigned short i = 0; i < records.size(); i++) {
                boost::json::object json_frame = Obtainframe(records[i], xsensManager.segmentNames_str, xsensManager.joints, xsensManager.getXmeControl());
                data_array.push_back(json_frame);
            }
            json_batch["data"] = data_array;
            outfile << json_batch << std::endl;
        }

        std::cout << "ok" << std::endl;
    }

    xsensManager.ToogleTimePoseMode();

    xsensManager.StopRecording();
    xsensManager.CloseMVNFile();
    outfile.close();

    std::printf("Stopping All \n");

    _getch();
    return 0;
}