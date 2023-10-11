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

boost::json::object Obtainframe(XmePose const& p, XsStringArray& segNames, XmeJointArray& j, XsMatrix& jointVals) {
    boost::json::object obj_segm;
    if (p.empty())
    {
        std::printf("No data available\n");
        return obj_segm;
    }
    XmeKinematicStateArray::const_iterator ptr_segments;
    int index = 0;
    for (ptr_segments = p.m_segmentStates.begin(); ptr_segments < p.m_segmentStates.end(); ptr_segments++) {
        boost::json::object pos;
        boost::json::object orientation;
        pos["x"] = ptr_segments->m_position[0];
        pos["y"] = ptr_segments->m_position[1];
        pos["z"] = ptr_segments->m_position[2];

        orientation["q1"] = ptr_segments->m_orientation[0];
        orientation["q2"] = ptr_segments->m_orientation[1];
        orientation["q3"] = ptr_segments->m_orientation[2];
        orientation["q4"] = ptr_segments->m_orientation[3];

        obj_segm[segNames[index].c_str()] = { {"position", pos}, {"orientation", orientation} };
        index++;
    };

    boost::json::object obj_joint;
    // XmeJointArray::const_iterator ptr_joints;

    for (int i = 0; i < j.size(); i++) {
        const XmeJoint& jnt = j[i];
        boost::json::object joint_data;
        std::string parent_name = segNames[jnt.parentConnection().segmentIndex()].c_str();
        std::string child_name = segNames[jnt.childConnection().segmentIndex()].c_str();
        joint_data["parent"] = parent_name;
        joint_data["child"] = child_name;
        joint_data["x"] = jointVals[i][0];
        joint_data["y"] = jointVals[i][1];
        joint_data["z"] = jointVals[i][2];

        obj_joint[parent_name + " - " + child_name] = joint_data;
    }

    boost::json::object complete_obj;

    complete_obj["segments"] = obj_segm;
    complete_obj["joints"] = obj_joint;

    return complete_obj;
}



int _tmain(int argc, _TCHAR* argv[])
{   
    std::cout << "Hello World!\n";
    XsTimeStamp start_program = XsTimeStamp::now();
    //XmeLicense lic;
    xsens_manage_tools::xsensManager Manager("Npose", "UpperBodyNoHands");
    // xsens_manage_tools::xsensManager Manager("Npose", "UpperBody");

    // return 0;
    Manager.ReviewStatus();

    Manager.ConfigurePatient();

    bool scan = Manager.scanMode();
    if (scan) {
        std::cout << "In scan mode\n";
    }
    else {
        std::cout << "out of scan mode!\n";
    }
    while (Manager.IsScanning()) {
        Sleep(100);
    }

    waitForKeyPress();
    XsString st = start_program.toString();
    st.replaceAll("/", "");
    st.replaceAll(":", "");
    std::cout << st + XsString(".mvn") << std::endl;
    std::cout << st << std::endl;
    Manager.CreateMVNFile(st + XsString(".mvn"));
    st = st + ".txt";
    // Manager.OpenMVNFile(MVNFILE);


    std::cout << "Scan Finished!\n";

    bool userQuit = false;
    resetKbhit();
    while (!userQuit)
    {
        Manager.Calibrate();
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

    // Manager.OpenMVNFile("testfile.mvn");


    std::ofstream outfile; 
    outfile.open(st.c_str());
    int64_t difference = 1000;
    Manager.ToogleTimePoseMode();
    XsTimeStamp start = XsTimeStamp::now();


    for (unsigned short i = 0; i < 200; i++) {
        // while (start.elapsedToNow() < difference);
        while (xsens_wait_msgs < 10);
        difference += 1000;
        // XsTime::msleep(1000);
        std::vector<PoseRecord> records = Manager.ReadUnreadPoses();
        std::cout << "Number of records: " << records.size() << std::endl;

        // if (records.size()>0) xsens_manage_tools::printArmsPose(records[0].pose);
        // if (records.size() > 0) std::cout<<boost::json::serialize(Obtainframe(records[0].pose, Manager.segmentNames))<<std::endl;
        
        if (records.size() > 0) {
            boost::json::object json_batch;
            json_batch["TimeStamp"] = XsTimeStamp::now().toString().c_str();
            boost::json::array data_array;
            for (unsigned short i = 0; i < records.size(); i++) {
                boost::json::object json_frame = Obtainframe(records[i].pose, Manager.segmentNames, Manager.joints, records[i].joints);
                json_frame["TimeStamp"] = records[0].ts.toString().c_str();
                data_array.push_back(json_frame);
            }
            json_batch["data"] = data_array;
            outfile << json_batch << std::endl;
            // boost::json::object json_frame = Obtainframe(records[0].pose, Manager.segmentNames, Manager.joints ,records[0].joints);
            // json_frame["TimeStamp"] = records[0].ts.toString().c_str();
            // outfile << json_frame << std::endl;
        }

        if (i == 100) Manager.StartRecording();

        std::cout << "ok" << std::endl;
    }

    Manager.ToogleTimePoseMode();

    Manager.StopRecording();
    Manager.CloseMVNFile();
    outfile.close();

    std::printf("Stopping All \n");

    _getch();
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
