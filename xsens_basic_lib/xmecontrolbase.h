
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

#ifndef XMECONTROLBASE_H
#define XMECONTROLBASE_H

#include <xme/xmecontrol.h>
#include <xme/xmecallback.h>
#include <xme/xmepose.h>
#include <mutex>
#include <vector>

extern volatile std::atomic_bool readSignal;

extern volatile std::atomic_int xsens_wait_msgs;

struct PoseRecord{
	XmePose pose;
	XsMatrix joints;
	XsTimeStamp ts;
};

class XmeControlBaseSC : public XmeCallback
{
public:
	XmeControlBaseSC();
	virtual ~XmeControlBaseSC();

	void onBufferOverflow(XmeControl*) override;
	void onCalibrationComplete(XmeControl*) override;
	void onCalibrationProcessed(XmeControl*) override;
	void onCalibrationAborted(XmeControl*) override;
	void onContactsUpdated(XmeControl*) override;
	void onCopyFileError(XmeControl*) override;
	void onCopyFileReady(XmeControl*) override;
	void onHardwareDisconnected(XmeControl*) override;
	void onHardwareError(XmeControl*) override;
	void onHardwareReady(XmeControl*) override;
	void onLowBatteryLevel(XmeControl*) override;
	void onPoseReady(XmeControl*) override;
	void onProcessingComplete(XmeControl*) override;
	void onProcessingProgress(XmeControl*, XmeProcessingStage stage, int a, int b) override;
	void onRecordingStateChanged(XmeControl*, XmeRecordingState newState) override;

	inline XmeControl& xme() const
	{
		return *m_xmeControl;
	}

	inline XmePose lastPose() const
	{
		std::lock_guard<std::mutex> lock(m_poseMutex);
		return m_lastPose;
	}

	inline std::vector<PoseRecord> lastListPose() const
	{
		readSignal = true;
		std::lock_guard<std::mutex> lock(m_poseMutex);
		xsens_wait_msgs = 0;
		return this->unread_poses;
	}

	inline void Erase_unreaded() {
		std::lock_guard<std::mutex> lock(m_poseMutex);
		this->unread_poses.clear();
	}

	inline void setAwindaChannel(int channel)
	{
		m_awindaChannel = channel;
	}

private:
	XmeControl* m_xmeControl;
	int m_awindaChannel;

	mutable std::mutex m_poseMutex;
	XmePose m_lastPose;
	std::vector<PoseRecord> unread_poses;
};

#endif
