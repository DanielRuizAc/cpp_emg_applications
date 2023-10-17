// Multifile_xsens_prove.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <chrono>
#include <stdio.h>
#include <tchar.h>
#include "xsens_basic_lib.h"
#include "xmecontrolbase.h"
// #include "boost/json.hpp"
#include "boost/json/src.hpp"
#include "boost/chrono.hpp"
#include "boost/asio.hpp"
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

        obj_segm[segNames[index]] = { {"position", pos}, {"velocity", vel}, {"acceleration", acel}, {"orientation", orientation}, {"ang_vel", ang_vel}, {"ang_accel", ang_acel} };
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
        XsTime::msleep(5000);
        XsTimeStamp start_trial = XsTimeStamp::now();
        XsString st = start_trial.toString();
        st.replaceAll("/", "");
        st.replaceAll(":", "");
        std::cout << st + XsString(".mvn") << std::endl;
        std::cout << st << std::endl;
        xsensManager.CreateMVNFile(st + XsString(".mvn"));
        st = st + ".txt";
        std::ofstream outfile;
        outfile.open(st.c_str());

        std::cout << "Starting a 40 seconds trial" << std::endl;
        xsensManager.ToogleTimePoseMode();
        xsensManager.StartRecording();
        boost::asio::steady_timer t(io);
        t.expires_after(boost::asio::chrono::milliseconds(100));
        t.wait();
        for (int counter=0; counter<400; counter++){
            std::vector<XmePose> records = xsensManager.ReadUnreadPoses();
            std::cout << "Number of records: " << records.size() << std::endl;

            boost::json::object json_batch;
            json_batch["TimeStamp"] = XsTimeStamp::now().toString().c_str();
            boost::json::array data_array;

            for (unsigned short i = 0; i < records.size(); i++) {
                boost::json::object json_frame = Obtainframe(records[i], xsensManager.segmentNames_str, xsensManager.joints, xsensManager.getXmeControl());
                data_array.push_back(json_frame);
            }

            json_batch["data"] = data_array;
            outfile << json_batch << std::endl;

            t.wait();
            t.expires_at(t.expires_at() + boost::asio::chrono::milliseconds(100));
        }

        xsensManager.ToogleTimePoseMode();
        xsensManager.StopRecording();
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

    std::printf("Stopping All \n");
    (void)_getch();
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
