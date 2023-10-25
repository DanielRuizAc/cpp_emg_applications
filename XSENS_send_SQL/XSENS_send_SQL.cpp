#include "pch.h"
#include "xsens_basic_lib.h"
#include "xmecontrolbase.h"

using namespace System;
using namespace System::Collections::Generic;




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

List<String^ >^ Obtainframe(XmePose const& p, std::string segNames[], XmeJointArray& j, XmeControl& xme_c, System::String^ ts) {
    Console::WriteLine(".DADFASDFJHIUDSFHUASBDISDAFIASDFIUDAFB");
    List<String ^ >^ values_insert = gcnew List<String^ >();
    if (p.empty())
    {
        std::printf("No data available\n");
        return values_insert;
    }
    int index = 0;
    Int32 frame = p.m_frameNumber;
    for (XmeKinematicStateArray::const_iterator ptr_segments = p.m_segmentStates.begin();
        ptr_segments < p.m_segmentStates.end();
        ptr_segments++) {
        Console::WriteLine(".DADFASDFJHIUDSFHUASBDISDAFIASDFIUDAFB");
        if (index >= 15) break;
        Console::WriteLine("..........................................");
        values_insert->Add(System::String::Format("('{0}', {1}, {2}, '{3}', {4})", ts, Int64(frame), Int16(index), "x", Single(ptr_segments->m_position[0])));
        Console::WriteLine("+++++++++++++++++++++++++++++++++++++++++++++");
        values_insert->Add(System::String::Format("('{0}', {1}, {2}, '{3}', {4})", ts, Int64(frame), Int16(index), "y", Single(ptr_segments->m_position[1])));
        Console::WriteLine(",,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,,");
        values_insert->Add(System::String::Format("('{0}', {1}, {2}, '{3}', {4})", ts, Int64(frame), Int16(index), "z", Single(ptr_segments->m_position[2])));
        
        index++;
    }
    System::Console::WriteLine(values_insert);
    return values_insert;
}

int main(array<System::String ^> ^args)
{
    XsTimeStamp start_program = XsTimeStamp::now();
    xsens_manage_tools::xsensManager xsensManager("Npose", "UpperBodyNoHands");
    xsensManager.ReviewStatus();
    xsensManager.ConfigurePatient();

    bool scan = xsensManager.scanMode();
    if (scan) {
        System::Console::WriteLine("In scan mode");
        // std::cout << "In scan mode\n";
    }
    else {
        System::Console::WriteLine("out of scan mode!");
        // std::cout << "out of scan mode!\n";
    }
    while (xsensManager.IsScanning()) {
        XsTime::msleep(100);
    }

    waitForKeyPress();

    std::cout << "Scan Finished!\n";

    bool userQuit = false;
    resetKbhit();


    while (!userQuit)
    {
        System::Console::WriteLine("Are you ok with the calibration performed?");
        // std::cout << "Are you ok with the calibration performed?" << std::endl;
        switch (_getch()) {
        case 'y':
        case 'Y':
            userQuit = true;
            break;
        default:
            System::Console::WriteLine("Making Again");
            // std::cout << "Making Again" << std::endl;
            xsensManager.Calibrate();
            break;
        }
    }

    std::vector<XmePose> records;

    xsensManager.ToogleTimePoseMode();

    for (unsigned short i = 0; i < 10; i++) {
        XsTime::msleep(100);
        records = xsensManager.ReadUnreadPoses();
        System::Console::WriteLine("Number of samples: {0}", records.size());

    }


    xsensManager.ToogleTimePoseMode();


    try
    {
        System::String^ cn = "Data Source=40.118.3.128;Initial Catalog=EMG;Persist Security Info=True;User ID=made;Password=i1LfUa95gtNHdzJA8oRs";
        System::Data::SqlClient::SqlConnection sqlc(cn);
        sqlc.Open();
        
        String^ ts_string = gcnew String(start_program.toString().c_str());
        ts_string = ts_string->Replace(' ','_');
        ts_string = ts_string->Replace(':', '_');
        List<String^>^ Pose1 = Obtainframe(records[0], xsensManager.segmentNames_str, xsensManager.joints, xsensManager.getXmeControl(), ts_string);

        System::String^ query_str = "INSERT INTO Xsens_segments_samples (experimentTimeStamp, frameNumber, segmentID, dims, measuredValue) values ";
        bool initial = true;
        int count_i = 1;
        for each (System::String ^ in_val in Pose1) {

            if (initial) {
                initial = false;
                query_str = System::String::Concat(query_str, in_val);
            }
            else {
                query_str = System::String::Concat(query_str, " , ", in_val);
                if (count_i > 999) {
                    System::Data::SqlClient::SqlCommand command(query_str, % sqlc);
                    int ex_rows = command.ExecuteNonQuery();
                    printf("affected rows %d \n", ex_rows);
                    query_str = "INSERT INTO Xsens_segments_samples (experimentTimeStamp, frameNumber, segmentID, dims, measuredValue) values ";
                    initial = true;
                    count_i = 0;
                }
            }
            count_i++;
        }

        if (!initial) {
            System::Console::WriteLine(query_str);
            System::Data::SqlClient::SqlCommand command(query_str, % sqlc);
            int ex_rows = command.ExecuteNonQuery();
            printf("affected rows %d \n", ex_rows);
        }


    }
    catch (System::Exception^ e)
    {
        std::cout << "Error";
    }

    std::printf("Stopping All \n");
    return 0;
}
