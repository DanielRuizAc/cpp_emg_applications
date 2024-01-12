
//  Copyright (c) 2003-2022 Movella Technologies B.V. or subsidiaries worldwide.
//  All rights reserved.
//  
//  Redistribution and use in source and binary forms, with or without modification,
//  are permitted provided that the following conditions are met:
//  
//  1.	Redistributions of source code must retain the above copyright notice,
//  	this list of conditions and the following disclaimer.
//  
//  2.	Redistributions in binary form must reproduce the above copyright notice,
//  	this list of conditions and the following disclaimer in the documentation
//  	and/or other materials provided with the distribution.
//  
//  3.	Neither the names of the copyright holders nor the names of their contributors
//  	may be used to endorse or promote products derived from this software without
//  	specific prior written permission.
//  
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY
//  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
//  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
//  THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
//  SPECIAL, EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
//  OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
//  HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY OR
//  TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//  

/*! \file
	\brief Customized XmeControl

	Mainly used for progress reporting
*/

#include "xmecontrolbase.h"
#include <xme.h>

#include <stdio.h>		// included for printf

#include <vector>
#include <iostream>
#include <atomic>

// the signal variable is used for simple inter-process communication in this example
extern volatile std::atomic_bool eventSignal;

// #define MVNFILE "testfile.mvn"

/*! \class XmeControlBaseSC
	\brief Example implementation */

XmeControlBaseSC::XmeControlBaseSC()
	: m_xmeControl(nullptr)
	, m_awindaChannel(0)
{
	m_xmeControl = XmeControl::construct();

	if (m_xmeControl == nullptr)
	{
		XsException e(XmeControl::creationResultCode(), XsResultValue_toString(XmeControl::creationResultCode()));
		std::cout << "Unable to create XmeControl. " << e.what() << std::endl;
		throw e;
	}
	m_xmeControl->addCallbackHandler(this);
	unread_poses = std::vector<XmePose>();
}

XmeControlBaseSC::~XmeControlBaseSC()
{
	m_xmeControl->removeCallbackHandler(this);
	m_xmeControl->destruct();
}

void XmeControlBaseSC::onBufferOverflow(XmeControl*)
{
	std::printf("WARNING! Data buffer overflow. Your PC can't keep up with the incoming data. Buy a better PC.\n");
}

void XmeControlBaseSC::onCalibrationComplete(XmeControl*)
{
	std::printf("Calibration complete\n");
	eventSignal = true;
}

void XmeControlBaseSC::onCalibrationProcessed(XmeControl*)
{
	std::printf("Calibration processed\n");
	eventSignal = true;
}

void XmeControlBaseSC::onCalibrationAborted(XmeControl*)
{
	std::printf("Calibration aborted\n");
	eventSignal = true;
}

void XmeControlBaseSC::onContactsUpdated(XmeControl*)
{
	std::printf("Contacts updated\n");
}

void XmeControlBaseSC::onCopyFileError(XmeControl*)
{
	std::printf("Copy of MVN file FAILED!\n");
}

void XmeControlBaseSC::onCopyFileReady(XmeControl*)
{
	std::printf("Copy of MVN file succesful\n");
}

void XmeControlBaseSC::onHardwareDisconnected(XmeControl*)
{
	std::printf("Hardware disconnected\n");
}

void XmeControlBaseSC::onHardwareError(XmeControl*)
{
	auto s = m_xmeControl->status().suitStatus();

	if (s.m_masterDevice.m_deviceId.isAwindaX() && m_awindaChannel && s.m_wirelessChannel != m_awindaChannel)
		XmeControl::setRadioChannel(s.m_masterDevice.m_deviceId, m_awindaChannel);

	switch (s.m_hardwareStatus)
	{
	case XHS_HardwareOk:
		std::printf("XHS_HardwareOk\n");
		std::printf("master ID=%08llX\n", m_xmeControl->status().suitStatus().m_masterDevice.m_deviceId.toInt());
		break;

	case XHS_Error:
		std::printf("XHS_Error: %s\n", s.m_hardwareStatusText.c_str());
		std::printf("master ID=%08llX\n", m_xmeControl->status().suitStatus().m_masterDevice.m_deviceId.toInt());
		std::printf("failingDeviceId=%lld\n", m_xmeControl->status().suitStatus().m_lastProblematicDeviceId.toInt());
		break;

	case XHS_NothingFound:
		std::printf("XHS_NothingFound\n");
		break;

	case XHS_MissingSensor:
		std::printf("XHS_MissingSensor\n");
		std::printf("master ID=%08llX\n", m_xmeControl->status().suitStatus().m_masterDevice.m_deviceId.toInt());
		std::printf("failingDeviceId=%llu\n", m_xmeControl->status().suitStatus().m_lastProblematicDeviceId.toInt());

		std::printf("%d missing locations:", (int)s.m_missingSensors.size());
		for (auto i : s.m_missingSensors)
			std::printf(" %d(%s)", i, m_xmeControl->segmentName(i).c_str());
		std::printf("\n");

		{
			XsIntArray dups;
			XsDeviceIdArray dupIds;
			for (auto const& t : s.m_sensors)
			{
				if (t.m_validity == XDV_Duplicate)
				{
					dups.push_back(t.m_segmentId);
					dupIds.push_back(t.m_deviceId);
				}
			}
			if (dups.size())
			{
				std::printf("%d duplicate locations:", (int)dups.size());
				for (XsSize i = 0; i < dups.size(); ++i)
					std::printf(" [%s-%d(%s)]", dupIds[i].toString().c_str(), dups[i], m_xmeControl->segmentName(dups[i]).c_str());
				std::printf("\n");
			}
		}
		break;

	case XHS_ObrMode:
		std::printf("XHS_ObrMode\n");
		std::printf("master ID=%08llX\n", m_xmeControl->status().suitStatus().m_masterDevice.m_deviceId.toInt());
		break;

	case XHS_Invalid:
		std::printf("XHS_Invalid\n");
		std::printf("master ID=%08llX\n", m_xmeControl->status().suitStatus().m_masterDevice.m_deviceId.toInt());
		break;

	default:
		std::printf("Unknown Status\n");
		break;
	}
}

void XmeControlBaseSC::onHardwareReady(XmeControl*)
{
	std::printf("\nhardware ready\n");

	XmeStatus XmeStatus1 = m_xmeControl->status();
	int nsensors = (int)XmeStatus1.suitStatus().m_sensors.size();
	std::printf("Detected %u sensors\nLocation | DeviceId\n", nsensors);

	XmeDeviceStatusArray SensorConnections = XmeStatus1.suitStatus().m_sensors;
	for (unsigned int i = 0; i < SensorConnections.size(); i++)
		std::printf("%8d | 0x%llX\n", SensorConnections[i].m_segmentId, SensorConnections[i].m_deviceId.toInt());
}

void XmeControlBaseSC::onLowBatteryLevel(XmeControl*)
{
	XmeStatus xmeStatus = m_xmeControl->status();
	XmeDeviceStatusArray availableSensors = xmeStatus.suitStatus().m_sensors;

	bool skipBatteryWarning = false;
	for (XmeDeviceStatus ithSensor : availableSensors)
	{
		// If the battery level is not available then skip the warning
		skipBatteryWarning |= ((ithSensor.m_batteryLevel == 0) || (ithSensor.m_batteryLevel == -1));
	}

	if (!skipBatteryWarning)
		std::printf("Attention! Low battery level!\n");
}

void XmeControlBaseSC::onPoseReady(XmeControl* xme)
{
	// Make sure only one thread can acces m_lastPose, since the callback will be called from the xme thread
	/*std::lock_guard<std::mutex> lock(m_poseMutex);
	if (readSignal) this->unread_poses.clear();
	readSignal = false;
	XmePose lastPose = m_xmeControl->pose(XME_LAST_AVAILABLE_FRAME);
	if (!lastPose.empty())
		this->unread_poses.push_back(lastPose);

	*/
}

void XmeControlBaseSC::onProcessingComplete(XmeControl*)
{
	std::printf("Processing complete\n");
}

void XmeControlBaseSC::onProcessingProgress(XmeControl*, XmeProcessingStage stage, int a, int b)
{
	std::printf("Processing progress: frames %d-%d now in stage %d\n", a, b, (int)stage);
}

void XmeControlBaseSC::onRecordingStateChanged(XmeControl*, XmeRecordingState newState)
{
	switch (newState)
	{
	case XRS_NotRecording:
		std::printf("REC: Recording stopped\n");
		break;

	case XRS_WaitingForStart:
		std::printf("REC: Waiting for recording to start\n");
		break;

	case XRS_Recording:
		std::printf("REC: Recording started\n");
		break;

	case XRS_Flushing:
		std::printf("REC: Flushing data, receiving retransmissions\n");
		break;

	default:
		std::printf("REC: Unknown state\n");
		break;
	}
}
