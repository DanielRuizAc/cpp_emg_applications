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
	IPortListPtr ptrPortList(__uuidof(PortList));

	// Get the IPort interface of the ptrCOMPort
	IPort* pPort;
	this->ptrCOMPort.QueryInterface(__uuidof(IPort), &pPort);

	// Insert the new COM port at first position (position zero)
	ptrPortList->Insert(0, pPort);
	
	// Instantiate the BioDAQ 
	this->ptrBioDAQ = IBioDAQPtr(__uuidof(BioDAQ));

	BioDAQExitStatus errCode = this->ptrBioDAQ->Init(ptrPortList);

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
	for (long i; i < this->ptrBioDAQ->Sinks->GetCount(); i++) {
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

bool ArmStart();