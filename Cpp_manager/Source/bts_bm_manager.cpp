#include "bts_bm_manager.h"

bts_bm_manager::bts_bm_manager() {
	// Make sure to add USES_CONVERSION for using ATL string conversion macros
	USES_CONVERSION;

	// Make sure to add the CoInitializeEx() call when the application loads
	CoInitializeEx(NULL, COINIT_MULTITHREADED);
	this->g_csObject = CRITICAL_SECTION();

	InitializeCriticalSection(&(this->g_csObject));

	this->ptrCOMPort = nullptr;
	this->ptrBioDAQ =nullptr;
	this->g_ptrQueueSink =nullptr;
	this->bmViewList = nullptr;
	this->chViewList = nullptr;
	this->ptrPortList = nullptr;
	this->protocolItems = 0L;
}


bts_bm_manager::~bts_bm_manager() {
	CoUninitialize();

	DeleteCriticalSection(&(this->g_csObject));
}


bool bts_bm_manager::ConnectionCOMPort(int comNum, BaudRate baud) {
	this->ptrCOMPort = IPortCOMPtr(__uuidof(PortCOM));
	this->ptrCOMPort->Number = comNum;
	this->ptrCOMPort->PutBaudRate(baud);

	// Create the ports List
	this->ptrPortList = IPortListPtr(__uuidof(PortList));

	// Get the IPort interface of the ptrCOMPort
	IPort* pPort;
	this->ptrCOMPort.QueryInterface(__uuidof(IPort), &pPort);

	// Insert the new COM port at first position (position zero)
	this->ptrPortList->Insert(0, pPort);
	
	// Instantiate the BioDAQ 
	this->ptrBioDAQ = IBioDAQPtr(__uuidof(BioDAQ));

	BioDAQExitStatus errCode = this->ptrBioDAQ->Init(this->ptrPortList);

	if (BioDAQExitStatus_Success != errCode)
	{
		printf("Failed: unable to initialize BioDAQ device on USB COM port %d.\n", this->ptrCOMPort->Number);
		printf("Press a key to exit...\n");
		_gettch();
		return false;
	}

	// Get the list of BMs 
	this->bmViewList = this->ptrBioDAQ->BmsView;
	this->chViewList = this->ptrBioDAQ->ChannelsView;
	this->protocolItems = this->chViewList->Count;

	return true;
}


bool bts_bm_manager::ConfigureAquisition(CodingType type, bool compression, EMGChannelRangeCodes gain, int nSamples) {
	
	if (type == CodingType_BitField)
	{
		printf("The coding type selected is not valid");
		return false;
	}
	if (compression)
	{
		if (nSamples <= 19 || nSamples >= 101 || nSamples % 2 == 0) {
			printf("The number of samples does not match with the coding and compression type"); 
			return false;
		}
	}
	else {
		if (nSamples <= 10 || nSamples >= 62 || nSamples % 2 != 0)
		{
			printf("The number of samples does not match with the coding and compression type");
			return false;
		}
	}
	// Set properties for all channels
	this->chViewList->SetEMGChannelsRangeCode(gain);   // EMG channel range
	this->chViewList->SetEMGChannelsSamplingRate(SamplingRate_Rate1KHz);         // Sampling rate: 1 kHz
	this->chViewList->SetEMGChannelsCodingType(type);
	this->chViewList->SetEMGChannelsCompression(compression);                           // Coding type: ADPCM
	for (long i = 0; i < this->bmViewList->GetCount(); i++) {
		this->bmViewList->GetItem(i)->PacketSamples = nSamples;
		SAFEARRAY* serial = this->bmViewList->GetItem(i)->Serial;
		long datapacketsize = this->bmViewList->GetItem(i)->DataPacketSize;
		// printf("BM serial: %d, PacketSamples: %i, DataPacketSize: %l");
	}
	return true;
}


void bts_bm_manager::ObtainQueueSink() {
	for (long i=0; i < this->ptrBioDAQ->Sinks->GetCount(); i++) {
		IBaseSinkPtr g_ptrBaseSinkAux;             // Queue sink interface.

		// Get the specialized IQueueSink interface from generic IDataSink interface
		IDataSink* pDataSinkTmp = ptrBioDAQ->Sinks->GetItem(i);
		pDataSinkTmp->QueryInterface(__uuidof(IBaseSink), (void**)&g_ptrBaseSinkAux);
		if (g_ptrBaseSinkAux->Type == SinkType_Queue) {
			this->g_ptrQueueSink = g_ptrBaseSinkAux;
			break;
		}
	}
}

bool bts_bm_manager::ArmStart() {
	this->ObtainQueueSink();
	printf("Arming BioDAQ device...\n");
	BioDAQExitStatus errCode = ptrBioDAQ->Arm();
	if (BioDAQExitStatus_Success != errCode)
	{
		printf("Failed: unable to arm BioDAQ device\n");
		printf("Press a key to exit...\n");
		_gettch();

		// roll-back
		this->ptrBioDAQ->Reset();
		this->ptrBioDAQ->Release();
		this->bmViewList->Release();
		this->chViewList->Release();
		this->ptrCOMPort->Release();

		return false;
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
		this->Clean();

		return false;
	}

	return true;
}


void bts_bm_manager::Clean() {
	// Release BMs view list
	this->bmViewList->Release();
	// Release channels view list
	this->chViewList->Release();

	// Reset BioDAQ
	this->ptrBioDAQ->Reset();

	// Release BioDAQ
	this->ptrBioDAQ->Release();

	// Clear ports list
	this->ptrPortList->Clear();
	// Release COM port
	this->ptrCOMPort->Release();
}


bool bts_bm_manager::Stop() {
	BioDAQExitStatus errCode = ptrBioDAQ->Stop();
	if (BioDAQExitStatus_Success != errCode)
	{
		printf("Failed: unable to stop BioDAQ device\n");
		return false;
	}
	return true;
}


void bts_bm_manager::Read(long lastIndex, int nSamples) {
	printf("----------------------------------------------");
	int queueSize = 0;
	bts_biodaq_core::IChannelPtr channel;
	for (int q = 0; q < this->protocolItems; q++)
	{
		// retrieve the size of current queue
		EnterCriticalSection(&(this->g_csObject));
		queueSize = g_ptrQueueSink->QueueSize(q);
		LeaveCriticalSection(&(this->g_csObject));

		if (0 == queueSize) continue;

		channel = this->chViewList->GetItem(q);
		if (VARIANT_FALSE == channel->Active) continue;

		int channelIndex = channel->ProtocolChannelIndex;
		if (channelIndex < 0 || channelIndex > protocolItems) continue;

		for (int sample = 0; sample < nSamples; sample++)
		{
			__int64 Si = lastIndex + sample;
			float value = 0.0f;
			// get sample value
			EnterCriticalSection(&(this->g_csObject));
			SinkExitStatus exitStatus = SinkExitStatus_Success;

			exitStatus = this->g_ptrQueueSink->Read(
				channelIndex,    // channel index
				Si,              // sample index 
				&value           // recovery sample value
			);

			LeaveCriticalSection(&(this->g_csObject));

			if (SinkExitStatus_Success == exitStatus && channelIndex == 0 && sample == 0 && value != 0)
			{
				// value is recovery from queue
				// print value in the console
				printf("\tChannel [%d], sample index: %d, value: %f V\n", channelIndex, (int)Si, value);
				printf("Queue size: %d \n", queueSize);
			}
			else if (SinkExitStatus_Success != exitStatus) {
				printf("error_%d_____________********************************************************************************\n", (int)Si);
			}
		}

	}

}

vector<sample> bts_bm_manager::Read_vector(long lastIndex, int nSamples, int chan) {
	int queueSize = 0;
	bts_biodaq_core::IChannelPtr channel;
	vector<sample> ysamp(0);
	for (int q = 0; q < this->protocolItems; q++)
	{
		// retrieve the size of current queue
		EnterCriticalSection(&(this->g_csObject));
		queueSize = g_ptrQueueSink->QueueSize(q);
		LeaveCriticalSection(&(this->g_csObject));

		if (0 == queueSize) continue;

		channel = this->chViewList->GetItem(q);
		if (VARIANT_FALSE == channel->Active) continue;

		int channelIndex = channel->ProtocolChannelIndex;
		if (channelIndex < 0 || channelIndex > protocolItems) continue;

		for (int samp = 0; samp < nSamples; samp++)
		{
			__int64 Si = lastIndex + samp;
			float value = 0.0f;
			// get sample value
			EnterCriticalSection(&(this->g_csObject));
			SinkExitStatus exitStatus = SinkExitStatus_Success;

			exitStatus = this->g_ptrQueueSink->Read(
				channelIndex,    // channel index
				Si,              // sample index 
				&value           // recovery sample value
			);

			LeaveCriticalSection(&(this->g_csObject));

			if (SinkExitStatus_Success == exitStatus && channelIndex == chan)
			{
				// value is recovery from queue
				// print value in the console
				ysamp.push_back(sample{ Si, value, exitStatus });
				if (samp == 0 && value != 0)
				{
					printf("\tChannel [%d], sample index: %d, value: %f V\n", channelIndex, (int)Si, value);
					printf("Queue size: %d \n", queueSize);
				}
			}
			else if (SinkExitStatus_Success != exitStatus) {
				printf("error_%d_____________********************************************************************************\n", (int)Si);
			}
		}

	}
	return ysamp;
}