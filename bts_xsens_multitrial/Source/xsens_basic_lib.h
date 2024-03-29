#pragma once

#include "xmecontrolbase.h"
#include <xme/xmepose.h>
#include <xme/xmelicense.h>
#include <xme/xmedll.h>
#include <xstypes/xstime.h>

#include <stdio.h>
#include <conio.h>
#include <windows.h>
#include <atomic>
#include <iostream>

namespace xsens_manage_tools {

	void printArmsPose(XmePose const& p);

	class xsensManager {
	public:
		XsStringArray segmentNames;
		XmeJointArray joints;
		// char* segmentNames_ch[15];
		std::string segmentNames_str[16];

		xsensManager(XsString calibT, XsString Conf);
		~xsensManager();

		bool scanMode();
		void ReviewStatus();
		void ConfigurePatient();
		XmeControl &getXmeControl();

		bool IsScanning();

		void Calibrate();
		bool ToogleTimePoseMode();
		std::vector<XmePose> ReadUnreadPoses();
		std::vector<XmePose> ReadUnreadPoses(int lastFrame, int n_frames);

		void CreateMVNFile(XsString const fileName);
		void OpenMVNFile(XsString const fileName);
		void CloseMVNFile();

		void StartRecording();
		void StopRecording();

	private:
		std::shared_ptr<XmeLicense> lic;
		XsString calibType;
		XsString configuration;
		std::shared_ptr<XmeControlBaseSC> instance;
		int channel;
		static void displayCalibrationResults(const XmeCalibrationResult& calibrationResult);
	};


}