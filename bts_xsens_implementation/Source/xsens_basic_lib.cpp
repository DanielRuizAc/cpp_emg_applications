#include "xsens_basic_lib.h"

volatile std::atomic_bool eventSignal = false;
volatile std::atomic_bool readSignal = false;

namespace xsens_manage_tools {

	void printArmsPose(XmePose const& p) {
		if (p.empty())
		{
			std::printf("No data available\n");
			return;
		}

		XsVector3 const& posPelvis = p.m_segmentStates[XME_PELVIS_SEGMENT_INDEX].m_position;
		XsVector3 const& posLArm = p.m_segmentStates[XME_LEFT_UPPER_ARM_SEGMENT_INDEX].m_position;
		XsVector3 const& posRArm = p.m_segmentStates[XME_RIGHT_UPPER_ARM_SEGMENT_INDEX].m_position;

		int64_t frameTime = p.m_relativeTime;

		std::printf("\nPose %d at timestamp %4d.%03d\n", (int)p.m_frameNumber, (int)(frameTime / 1000), (int)(frameTime % 1000));

		std::printf("Pelvis     [%+.4f %+.4f %+.4f]\n"
			"Left Arm [%+.4f %+.4f %+.4f]\n"
			"Right Arm [%+.4f %+.4f %+.4f]\n"
			, posPelvis[0], posPelvis[1], posPelvis[2]
			, posLArm[0], posLArm[1], posLArm[2]
			, posRArm[0], posRArm[1], posRArm[2]);

		std::printf("sdfafsafasdf");
	}


	xsensManager::xsensManager(XsString calibT, XsString Conf) {
		this->lic = std::shared_ptr<XmeLicense>(new XmeLicense());
		char* pts[16];

		printf("Constructor \n");
		std::cout << "XME Version: " << xmeGetDllVersion().toString() << " License: " << this->lic->getCurrentLicense() << std::endl;
		this->calibType = calibT;
		this->configuration = Conf;
		this->channel = 15;

		this->instance = nullptr;

		try
		{
			this->instance = std::shared_ptr<XmeControlBaseSC>(new XmeControlBaseSC());
			this->instance->setAwindaChannel(this->channel);	// We always set this channel, but this is only required if you use Awinda

			XsStringArray segList = this->instance->xme().segmentNames();
			this->segmentNames = segList;
			this->joints = this->instance->xme().joints();

			XsStringArray::iterator string_ptr;

			for (string_ptr = segList.begin(); string_ptr < segList.end(); string_ptr++) {
				std::cout << *string_ptr << std::endl;
				std::cout << "Id: " << this->instance->xme().segmentId(*string_ptr) << std::endl;
				std::cout << "Index: " << this->instance->xme().segmentIndex(*string_ptr) << std::endl;
				if (this->instance->xme().segmentIndex(*string_ptr)<=15) {
					this->segmentNames_ch[this->instance->xme().segmentIndex(*string_ptr)] = string_ptr->c_str();
				}
			}

		}
		catch (XsException const& e)
		{
			auto txt = e.text();
			if (txt.empty())
				std::cerr << "Error: " << XsResultValue_toString(e.code()) << std::endl;
			else
				std::cerr << "Error: " << txt << std::endl;
			if (e.code() == XRV_INVALIDOPERATION || e.code() == XRV_INVALIDINSTANCE)
				std::cerr << "Make sure you have the correct license.\n* The RecordSDK license does not allow opening of files.\n* The Record license does not allow most SDK operations.\n* Unlicensed indicates that the License Manager has not been run to install the basic (Record) license." << std::endl;

			std::cout << "Press a key to end the program." << std::endl;
		}
	}

	xsensManager::~xsensManager() {
		this->instance = nullptr;
		xmeTerminate();
	}


	bool xsensManager::scanMode() {
		try
		{
			this->instance->xme().setConfiguration(this->configuration);
			if (this->instance->xme().scanMode())
				this->instance->xme().setScanMode(false);
			else
				this->instance->xme().setScanMode(true);
			printf("setScanMode set to %s\n", this->instance->xme().scanMode() ? "true" : "false");
			return this->instance->xme().scanMode() ? "true" : "false";
		}
		catch (XsException const& e)
		{
			auto txt = e.text();
			if (txt.empty())
				std::cerr << "Error: " << XsResultValue_toString(e.code()) << std::endl;
			else
				std::cerr << "Error: " << txt << std::endl;
			if (e.code() == XRV_OUTPUTCANNOTBEOPENED)
				std::cerr << "Make sure you have permission to write to this folder" << std::endl;
			if (e.code() == XRV_INVALIDOPERATION || e.code() == XRV_INVALIDINSTANCE)
				std::cerr << "Make sure you have the correct license.\n* The RecordSDK license does not allow opening of files.\n* The Record license does not allow most SDK operations." << std::endl;
			return false;
		}

	}


	XmeControl &xsensManager::getXmeControl() {
		return this->instance->xme();
	}


	void xsensManager::ConfigurePatient() {
		this->instance->xme().setBodyDimension("bodyHeight", 1.725);
		this->instance->xme().setBodyDimension("shoulderHeight", 1.473);
		this->instance->xme().setBodyDimension("shoulderWidth", 0.313);
		this->instance->xme().setBodyDimension("elbowSpan", 0.851);
		this->instance->xme().setBodyDimension("wristSpan", 1.335);
		this->instance->xme().setBodyDimension("armSpan", 1.610);
		this->instance->xme().setBodyDimension("wristSpan", 1.335);
		this->instance->xme().setBodyDimension("hipHeight", 1.335);

		XsStringArray bodyLabelList = this->instance->xme().bodyDimensionLabelList();
		XsStringArray::iterator string_ptr;

		std::cout << std::endl;

		for (string_ptr = bodyLabelList.begin(); string_ptr < bodyLabelList.end(); string_ptr++) {
			std::cout << *string_ptr << std::endl;
			std::cout << "Description: " << this->instance->xme().bodyDimensionDescription(*string_ptr) << std::endl;
			std::cout << "Value: " << this->instance->xme().bodyDimensionValueEstimate(*string_ptr) << std::endl;
		}

	}


	void xsensManager::ReviewStatus() {
		XmeStatus const& status = this->instance->xme().status();
		XsString const& calibType = this->calibType;
		XsString const& config = this->configuration;

		std::printf(
			"\n= Menu ================================= Status ===============================\n"
			"| Configuration:          %-12s |\n"
			"| Ready:                  %c            |\n"
			"| Recording:              %c            |\n"
			"| Calibration Type:       %-12s |\n"
			"| Calibrating:            %c            |\n"
			"| PostProcessing:         %c            |\n"
			"| Hardware Status:        %3s          |\n"
			"| Frames Recorded:    %5d            |\n"
			"| Frames Processed:   %5d            |\n"
			"===============================================================================\n"
			"|                             Connected Devices                               |\n"
			"===============================================================================\n"
			, config.c_str()
			, status.isConnected() ? 'Y' : 'N'
			, status.isRecording() ? 'Y' : 'N'
			, calibType.c_str()
			, status.isCalibrating() ? 'Y' : 'N'
			, status.isProcessing() ? 'Y' : 'N'
			, (status.suitStatus().m_hardwareStatus == XHS_HardwareOk) ? "OK " : (status.isScanning() ? "ERR" : "DIS")
			, status.frameCount()
			, status.framesProcessed());

		const XmeDeviceStatus masterStatus = status.suitStatus().m_masterDevice;

		std::printf("| Master Devices Connected: %1d                                                 |\n", (int)(masterStatus.empty() ? 0 : 1));
		std::printf("===============================================================================\n");

		if (!masterStatus.empty())
		{
			std::string deviceRadioQuality("");

			const int masterRadioQuality = masterStatus.m_radioQuality;
			if (masterRadioQuality >= 0 && masterRadioQuality <= 25)
				deviceRadioQuality = "BAD";
			if (masterRadioQuality > 25 && masterRadioQuality <= 75)
				deviceRadioQuality = "MODERATE";
			if (masterRadioQuality > 75)
				deviceRadioQuality = "GOOD";
			if (masterRadioQuality == -1)
				deviceRadioQuality = "UNKNOWN";

			std::printf(
				"| Dev. Id:      %08llX | Conn. Quality :     %9s | Pow. Level:     %3u|\n"
				, masterStatus.m_deviceId.toInt()
				, deviceRadioQuality.c_str()
				, masterStatus.m_batteryLevel);

			std::printf("===============================================================================\n");
		}
	}


	bool xsensManager::IsScanning() {
		return this->instance->xme().status().isScanning() || this->instance->xme().status().isCalibrating() || this->instance->xme().status().isProcessing();
	}


	void xsensManager::Calibrate() {
		XsTime::msleep(5000);
		printf("\nStarting 1-click calibration with %s\n", this->calibType.c_str());
		printf("STEP 1: Initialize calibration\n");
		this->instance->xme().initializeCalibration(this->calibType);
		auto times = this->instance->xme().calibrationRecordingTimesRemaining();

		printf("STEP 2: Start calibration\n");
		this->instance->xme().startCalibration();

		printf("STEP 3: Wait for the recording to complete\n");
		while (!(times = this->instance->xme().calibrationRecordingTimesRemaining()).empty())
		{
			double tRemaining = times.empty() ? 0.0 : ((double)times[0] / 1000.0);
			printf("%.1f seconds remaining\n", tRemaining);
			XsTime::msleep(200);
		}

		while (!eventSignal)
			XsTime::msleep(10);
		eventSignal = false;

		printf("STEP 4: Wait for processing to complete\n");
		while (!eventSignal)
			XsTime::msleep(10);
		eventSignal = false;

		printf("STEP 5: Calibration complete\n");
		XsTime::msleep(10000);
		xsensManager::displayCalibrationResults(this->instance->xme().calibrationResult(this->calibType));
	}


	bool xsensManager::ToogleTimePoseMode() {
		if (this->instance->xme().realTimePoseMode())
			this->instance->xme().setRealTimePoseMode(false);
		else
			this->instance->xme().setRealTimePoseMode(true);
		printf("RealTimePoseMode set to %s\n", this->instance->xme().realTimePoseMode() ? "true" : "false");
		return this->instance->xme().realTimePoseMode();
	}


	std::vector<XmePose> xsensManager::ReadUnreadPoses() {
		if (!readSignal) {
			printf("reading \n");
			return this->instance->lastListPose();
			printf("done \n");
		}
		else {
			return std::vector<XmePose>();
		}
	}


	void xsensManager::displayCalibrationResults(const XmeCalibrationResult& calibrationResult)
	{
		std::cout << "Calibration result: ";
		switch (calibrationResult.m_quality)
		{
		case XCalQ_Unknown:
			std::cout << "unknown";
			break;
		case XCalQ_Good:
			std::cout << "good";
			break;
		case XCalQ_Acceptable:
			std::cout << "acceptable";
			break;
		case XCalQ_Poor:
			std::cout << "poor";
			break;
		case XCalQ_Failed:
			std::cout << "failed";
			break;
		default:
			std::cout << "<error>";
			break;
		}
		std::cout << std::endl;

		XsStringArray warnings = calibrationResult.m_warnings;
		if (!warnings.empty())
		{
			std::cout << "Calibration produced warnings:" << std::endl;
			for (auto const& it : warnings)
				std::cout << it << std::endl;
		}
		std::cout << std::endl;
	}

	void xsensManager::CreateMVNFile(XsString const fileName) {
		try
		{
			printf("Creating new MVN file %s\n", fileName.c_str());
			this->instance->xme().createMvnFile(fileName);
		}
		catch (XsException const& e)
		{
			auto txt = e.text();
			if (txt.empty())
				std::cerr << "Error: " << XsResultValue_toString(e.code()) << std::endl;
			else
				std::cerr << "Error: " << txt << std::endl;
			if (e.code() == XRV_INVALIDOPERATION || e.code() == XRV_INVALIDINSTANCE)
				std::cerr << "Make sure you have the correct license.\n* The RecordSDK license does not allow opening of files.\n* The Record license does not allow most SDK operations.\n* Unlicensed indicates that the License Manager has not been run to install the basic (Record) license." << std::endl;

			std::cout << "Press a key to end the program." << std::endl;
		}
	}


	void xsensManager::OpenMVNFile(XsString const fileName) {
		try
		{
			printf("Opening MVN file %s\n", fileName.c_str());
			this->instance->xme().openMvnFile(fileName);
		}
		catch (XsException const& e)
		{
			auto txt = e.text();
			if (txt.empty())
				std::cerr << "Error: " << XsResultValue_toString(e.code()) << std::endl;
			else
				std::cerr << "Error: " << txt << std::endl;
			if (e.code() == XRV_INVALIDOPERATION || e.code() == XRV_INVALIDINSTANCE)
				std::cerr << "Make sure you have the correct license.\n* The RecordSDK license does not allow opening of files.\n* The Record license does not allow most SDK operations.\n* Unlicensed indicates that the License Manager has not been run to install the basic (Record) license." << std::endl;

			std::cout << "Press a key to end the program." << std::endl;
		}
	}


	void xsensManager::CloseMVNFile() {
		try
		{
			printf("Saving MVN file.\n");
			this->instance->xme().saveFile();
			printf("Closing MVN file.\n");
			this->instance->xme().closeFile();
		}
		catch (XsException const& e)
		{
			auto txt = e.text();
			if (txt.empty())
				std::cerr << "Error: " << XsResultValue_toString(e.code()) << std::endl;
			else
				std::cerr << "Error: " << txt << std::endl;
			if (e.code() == XRV_INVALIDOPERATION || e.code() == XRV_INVALIDINSTANCE)
				std::cerr << "Make sure you have the correct license.\n* The RecordSDK license does not allow opening of files.\n* The Record license does not allow most SDK operations.\n* Unlicensed indicates that the License Manager has not been run to install the basic (Record) license." << std::endl;

			std::cout << "Press a key to end the program." << std::endl;
		}
	}


	void xsensManager::StartRecording() {
		try
		{
			printf("Starting recording\n");
			this->instance->xme().startRecording();
		}
		catch (XsException const& e)
		{
			auto txt = e.text();
			if (txt.empty())
				std::cerr << "Error: " << XsResultValue_toString(e.code()) << std::endl;
			else
				std::cerr << "Error: " << txt << std::endl;
			if (e.code() == XRV_INVALIDOPERATION || e.code() == XRV_INVALIDINSTANCE)
				std::cerr << "Make sure you have the correct license.\n* The RecordSDK license does not allow opening of files.\n* The Record license does not allow most SDK operations.\n* Unlicensed indicates that the License Manager has not been run to install the basic (Record) license." << std::endl;

			std::cout << "Press a key to end the program." << std::endl;
		}
	}


	void xsensManager::StopRecording() {
		try
		{
			printf("Stopping recording\n");
			this->instance->xme().stopRecording();
		}
		catch (XsException const& e)
		{
			auto txt = e.text();
			if (txt.empty())
				std::cerr << "Error: " << XsResultValue_toString(e.code()) << std::endl;
			else
				std::cerr << "Error: " << txt << std::endl;
			if (e.code() == XRV_INVALIDOPERATION || e.code() == XRV_INVALIDINSTANCE)
				std::cerr << "Make sure you have the correct license.\n* The RecordSDK license does not allow opening of files.\n* The Record license does not allow most SDK operations.\n* Unlicensed indicates that the License Manager has not been run to install the basic (Record) license." << std::endl;

			std::cout << "Press a key to end the program." << std::endl;
		}
	}


}