// Include standard libraries
#include "stdafx.h"
#include <process.h>
#include <vector>

#include "MatlabEngine.hpp"
#include "MatlabDataArray.hpp"

// Include BioDAQ template libraries (registration has been done!)

#import "mscorlib.tlb"
#import "bts.biodaq.drivers.tlb"
#import "bts.biodaq.core.tlb"

using namespace std;
using namespace mscorlib;
using namespace bts_biodaq_core;
using namespace bts_biodaq_drivers;

int _tmain(int argc, _TCHAR* argv[])
{
	// Make sure to add USES_CONVERSION for using ATL string conversion macros
	USES_CONVERSION;

	// Make sure to add the CoInitializeEx() call when the application loads
	CoInitializeEx(NULL, COINIT_MULTITHREADED);

	CRITICAL_SECTION g_csObject;              // Critical section object.
	InitializeCriticalSection(&g_csObject);

	printf("Create a USB COM port where BioDAQ device is connected...\n");

	// Create a COM Port with a given number 
	IPortCOMPtr ptrCOMPort(__uuidof(PortCOM));
	ptrCOMPort->Number = 3;      // set the COM port number where BioDAQ device is connected (use Windows device manager tool)

	// Create the ports List
	IPortListPtr ptrPortList(__uuidof(PortList));

	// Get the IPort interface of the ptrCOMPort
	IPort* pPort;
	ptrCOMPort.QueryInterface(__uuidof(IPort), &pPort);

	// Check the number of ports in the list before insert the new one
	long count = ptrPortList->Count;

	// Insert the new COM port at first position (position zero)
	ptrPortList->Insert(0, pPort);

	// Do some check: number of ports and the port index in the list
	count = ptrPortList->Count;
	long index = ptrPortList->IndexOf(pPort);

	// Create BM View list and BM View objects
	IBMViewListPtr bmViewList;
	IBMViewPtr  bmView;

	printf("Initializing BioDAQ device...\n");

	// Instantiate the BioDAQ 
	IBioDAQPtr ptrBioDAQ(__uuidof(BioDAQ));

	// Init the system with the created list of ports
	BioDAQExitStatus errCode = ptrBioDAQ->Init(ptrPortList);
	if (BioDAQExitStatus_Success != errCode)
	{
		printf("Failed: unable to initialize BioDAQ device on USB COM port %d.\n", ptrCOMPort->Number);
		printf("Press a key to exit...\n");
		_gettch();
		return 0;
	}

	//errCode = ptrBioDAQ->Stop();
	int count_stop = 0;
	//while (BioDAQExitStatus_Success != errCode && count_stop < 5)
	//{
	//	printf("Failed: unable to stop BioDAQ device\n");
	//	count_stop++;
	//}

	// Get the list of BMs 
	bmViewList = ptrBioDAQ->BmsView;
	// Do some check: number of BMs
	long nBMCounts = bmViewList->Count;

	// Get a BM from the list (position zero)
	if (nBMCounts > 0)
	{
		bmView = bmViewList->GetItem(0);

		// Get the BM Serial Number
		SAFEARRAY* serial = bmView->Serial;
	}

	// Get the active protocol
	IProtocolPtr protocol;
	protocol = ptrBioDAQ->ActiveProtocol;
	long protocolItems = protocol->Count();

	// Get the BioDAQ state
	bts_biodaq_core::BioDAQState bioDAQState = ptrBioDAQ->State;
	if (bioDAQState==BioDAQState_Ready) printf("ptrBioDAQ state correct");
	// Set up the channels
	IChannelViewListPtr chViewList;
	bts_biodaq_core::IChannelPtr  channel;
	printf("*+++++++++++++++++++++++++++++++++++++++++++++\n");

	IProcessorPtr ptrEMAProcessor(__uuidof(RMSProcessor));
	if (ptrEMAProcessor->GetType() == ProcessorType_EMA) printf("It is correct\n\n");

	ISinksFactory* ptrSinksFactory = ptrBioDAQ->SinksFactory; 
	IDataSink* pDataSink = ptrSinksFactory->CreateSink(bts_biodaq_core::SinkType::SinkType_Math);
	pDataSink->Init();
	printf("-----------------------------------------------\n");

	IMathSinkPtr ptrMathSink;
	pDataSink->QueryInterface(__uuidof(IMathSink), (void**)&ptrMathSink);
	ptrMathSink->SetProcessor(ptrEMAProcessor, 5);
	printf("-----------------------------------------------\n");

	IQueueSinkPtr ptrMathQueueSink;
	//ptrMathSink->QueryInterface(__uuidof(IQueueSink), (void**)&ptrMathQueueSink);
	printf("-----------------------------------------------\n");

	// printf("NTimePeriods of EMA processor interface %d", ptrEMAProcessor->NTimePeriods);
	HRESULT hResult = ptrBioDAQ->Sinks->Add(pDataSink);
	printf("-----------------------------------------------\n");
	// Get the channels view list
	chViewList = ptrBioDAQ->ChannelsView;
	long nChannels = chViewList->Count;

	// Set properties for all channels
	chViewList->SetEMGChannelsRangeCode(EMGChannelRangeCodes_Gain1_5mV);   // EMG channel range
	chViewList->SetEMGChannelsSamplingRate(SamplingRate_Rate1KHz);         // Sampling rate: 1 kHz
	chViewList->SetEMGChannelsCodingType(CodingType_Raw);
	chViewList->SetEMGChannelsCompression(false);                           // Coding type: ADPCM

	IQueueSinkPtr g_ptrQueueSink;             // Queue sink interface.

	// Get the specialized IQueueSink interface from generic IDataSink interface
	IDataSink* pDataSinkTmp = ptrBioDAQ->Sinks->GetItem(0);
	//IQueueSinkPtr ptrQueueSink;
	pDataSinkTmp->QueryInterface(__uuidof(IQueueSink), (void**)&g_ptrQueueSink);
	
	IDataSink* pDataSinkTmp2 = ptrBioDAQ->Sinks->GetItem(1);
	if (pDataSinkTmp2)
	pDataSinkTmp->QueryInterface(__uuidof(IQueueSink), (void**)&ptrMathQueueSink);
	std::unique_ptr<matlab::engine::MATLABEngine> matlabPtr = matlab::engine::startMATLAB();
	matlab::data::ArrayFactory factory;
	vector<double> yvals(0);
	yvals.push_back(0.0);
	vector<double> xvals(0);

	Sleep(5000);
	// Create arguments for the call to Matlab's plot function
	std::vector<matlab::data::Array> args({
		factory.createArray({ yvals.size(), 1 }, yvals.begin(), yvals.end()),
		factory.createCharArray(std::string("r-"))
		});
	// Invoke the plot command
	matlab::data::Array line = matlabPtr->feval(u"plot", args);

	yvals.pop_back();



	printf("Arming BioDAQ device...\n");

	// Arm the BioDAQ
	errCode = ptrBioDAQ->Arm();
	if (BioDAQExitStatus_Success != errCode)
	{
		printf("Failed: unable to arm BioDAQ device\n");
		printf("Press a key to exit...\n");
		_gettch();

		// roll-back
		ptrBioDAQ->Reset();
		ptrBioDAQ->Release();
		bmViewList->Release();
		chViewList->Release();
		ptrCOMPort->Release();

		return 0;
	}

	printf("Starting BioDAQ device...\n");

	// Start the acquisition
	errCode = ptrBioDAQ->Start();
	if (BioDAQExitStatus_Success != errCode)
	{
		printf("Failed: unable to arm BioDAQ device\n");
		printf("Press a key to exit...\n");
		_gettch();

		// roll-back
		ptrBioDAQ->Reset();
		ptrBioDAQ->Release();
		bmViewList->Release();
		chViewList->Release();
		ptrCOMPort->Release();

		return 0;
	}


	// Sleep a bit
	Sleep(420);

	int P = 0;
	int NSamples = 100;

	for (int loop = 0; loop < 200; loop++) {

		int queueSize = 0;
		for (int q = 0; q < protocolItems; q++)
		{
			// retrieve the size of current queue
			EnterCriticalSection(&g_csObject);
			queueSize = g_ptrQueueSink->QueueSize(q);
			LeaveCriticalSection(&g_csObject);

			if (0 == queueSize) continue;

			// Get channel
			channel = chViewList->GetItem(q);
			if (VARIANT_FALSE == channel->Active) continue;

			int channelIndex = channel->ProtocolChannelIndex;
			if (channelIndex < 0 || channelIndex > protocolItems) continue;

			float Q = 1.0f;
			int SR = 1000;    // SamplingRate_Rate1KHz
			int DR = SR / channel->DecimationFactor;

			if (ChannelType_EMG != channel->Type) DR = 100; // FSWEGN are sampled at a fixed datarate

			// gets a defined number of samples (for EMG channels: 100; otherwise: 10; see dwTimeOut and SamplingRate_Rate1KHz)
			if (ChannelType_EMG != channel->Type) NSamples = 10;
			// loop on requested samples
			for (int sample = 0; sample < NSamples; sample++)
			{
				__int64 Si = P + sample;
				float value = 0.0f;

				__int64 Si2 = P + sample;
				float value2 = 0.0f;

				// get sample value
				EnterCriticalSection(&g_csObject);
				SinkExitStatus exitStatus = SinkExitStatus_Success;

				exitStatus = g_ptrQueueSink->Read(
					channelIndex,    // channel index
					Si,              // sample index 
					&value           // recovery sample value
				);

				LeaveCriticalSection(&g_csObject);
				
				if (SinkExitStatus_Success == exitStatus ) {

					EnterCriticalSection(&g_csObject);
					SinkExitStatus exitStatus = SinkExitStatus_Success;

					exitStatus = ptrMathQueueSink->Read(
						channelIndex,    // channel index
						Si2,              // sample index 
						&value2           // recovery sample value
					);
					LeaveCriticalSection(&g_csObject);
					yvals.push_back(value2);
					xvals.push_back(Si * 0.001);
				}
				if (SinkExitStatus_Success == exitStatus && sample == 0 && value != 0)
				{
					// value is recovery from queue
					// print value in the console
					printf("\tChannel [%d], sample index: %d, value: %f V\n", channelIndex, (int)Si, value);
					printf("\tChannel [%d], sample index: %d, value: %f V\n\n", channelIndex, (int)Si2, value2);
				}
			}
		}

		std::vector<matlab::data::Array> args_realt({
			line,
			factory.createCharArray(std::string("YData")),
			factory.createArray({ yvals.size(), 1 }, yvals.begin(), yvals.end()),
			factory.createCharArray(std::string("XData")),
			factory.createArray({ xvals.size(), 1 }, xvals.begin(), xvals.end())
			});
		matlabPtr->feval(u"set", args_realt);
		matlabPtr->eval(u"drawnow");

		P = P + NSamples;
		Sleep(100);
	}

	printf("Stopping BioDAQ device...\n");

	// Stop acquisition
	errCode = ptrBioDAQ->Stop();
	count_stop = 0;
	while (BioDAQExitStatus_Success != errCode && count<5)
	{
		printf("Failed: unable to stop BioDAQ device\n");
		count_stop++;
	}

	// Wait ... just a bit
	Sleep(250);

	Sleep(10000);
	
	bioDAQState = ptrBioDAQ->State;
	if (BioDAQState_Ready != bioDAQState)
	{
		printf("Error on state for BioDAQ device.\n");
	}

	// Reset the system

	printf("Reset objects...\n");

	// Release BMs view list
	bmViewList->Release();
	// Release channels view list
	chViewList->Release();

	// Reset BioDAQ
	ptrBioDAQ->Reset();

	// Release BioDAQ
	ptrBioDAQ->Release();

	// Clear ports list
	ptrPortList->Clear();
	// Release COM port
	ptrCOMPort->Release();

	printf("Press a key to exit...\n");
	_gettch();

	CoUninitialize();

	DeleteCriticalSection(&g_csObject);
	
	/*
	vector<double> yvals2(0);
	vector<double> xvals(0);
	for (int i = 0; i < (((int)yvals.size())/100)-1; i++) {
		for (int j = 0; j < 100; j++) {
			yvals2.push_back(yvals[i*100 + j]);
			xvals.push_back(i * 100 + j);
		}
		std::vector<matlab::data::Array> args_realt({
			line,
			factory.createCharArray(std::string("YData")),
			factory.createArray({ yvals2.size(), 1 }, yvals2.begin(), yvals2.end()),
			factory.createCharArray(std::string("XData")),
			factory.createArray({ xvals.size(), 1 }, xvals.begin(), xvals.end())
			});
		matlabPtr->feval(u"set", args_realt);
		matlabPtr->eval(u"drawnow");

		P = P + NSamples;
		Sleep(100);
	}
	*/
	return 0;

}