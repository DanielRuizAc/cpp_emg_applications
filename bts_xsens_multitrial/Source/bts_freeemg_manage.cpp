#include "bts_freeemg_manage.h"

namespace bts_manage_tools {

	// Contructor of the bts_bm_manager class
	bts_bm_manager::bts_bm_manager() {
		// Make sure to add USES_CONVERSION for using ATL string conversion macros
		USES_CONVERSION;

		// Make sure to add the CoInitializeEx() call when the application loads
		CoInitializeEx(NULL, COINIT_MULTITHREADED);
		this->g_csObject = CRITICAL_SECTION();

		InitializeCriticalSection(&(this->g_csObject));

		this->ptrCOMPort = nullptr;
		this->ptrBioDAQ = nullptr;
		this->g_ptrQueueSink = nullptr;
		this->bmViewList = nullptr;
		this->chViewList = nullptr;
		this->ptrPortList = nullptr;
		this->ptrSensorViewDictionary = nullptr;
		this->protocolItems = 0L;
		this->active_channels = vector<int>();
		this->readable_channels = vector<int>();
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
		this->ptrSensorViewDictionary = this->ptrBioDAQ->SensorsView;
		this->ptrBioDAQ->Trigger(TriggerSource_Software);

		this->ptrBioDAQ->UpdateStatusInfo();

		return true;
	}


	bool bts_bm_manager::ConnectionCOMPort(int comNum) {
		return this->ConnectionCOMPort(comNum, BaudRate_baud230400);
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


	bool bts_bm_manager::ConfigureAquisition(bool compression, int nSamples) {
		return this->ConfigureAquisition(CodingType_Raw, compression, EMGChannelRangeCodes_Gain1_5mV, nSamples);
	}


	void bts_bm_manager::ObtainQueueSink() {
		for (long i = 0; i < this->ptrBioDAQ->Sinks->GetCount(); i++) {
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

	void bts_bm_manager::DefineActiveChannels() {
		this->active_channels.clear();
		bts_biodaq_core::IChannelPtr channel;
		for (int q = 0; q < this->protocolItems; q++)
		{
			channel = this->chViewList->GetItem(q);
			if (VARIANT_FALSE != channel->Active) {
				this->active_channels.push_back(channel->ProtocolChannelIndex);
				printf("Channel %d is active \n", channel->ProtocolChannelIndex);
			}
		}
	}


	void bts_bm_manager::PrintSensorData() {
		this->ptrBioDAQ->UpdateStatusInfo();
		ISensorViewPtr sensor;
		short unsigned int sens_count = this->ptrSensorViewDictionary->Count;
		ISensorViewDictionaryEnumPtr ptrSensorViewDictionaryEnum;
		ptrSensorViewDictionaryEnum = this->ptrSensorViewDictionary->GetEnumerator();
		this->readable_channels.clear();
		// ptrSensorViewDictionaryEnum->MoveNext();
		// move at first item
		bool bResult;
		for (unsigned short i = 0; i < sens_count; i++) {

			bResult = (ptrSensorViewDictionaryEnum->MoveNext() == VARIANT_TRUE) ? true : false;
			if (!bResult)
			{
				printf("Failed: invalid operation...\n");
				printf("Press a key to exit...\n");
				_gettch();
				return;
			}

			sensor = ptrSensorViewDictionaryEnum->Current;
			assert(NULL != sensor);
			// retrieve type of sensor
			SensorType sType = sensor->Device;
			if (SensorType_EMG != sType) continue;

			// Retrieve MAC address
			SAFEARRAYBOUND bound2[1] = { NITEMS, 0 };
			SAFEARRAY* address_get = SafeArrayCreate(VT_UI1, 1, bound2);
			address_get = sensor->MACBytes;
			char* pv = (char*)address_get->pvData;    // scratch
			char szMACAddressOut[_MAX_PATH];
			ZeroMemory(szMACAddressOut, _MAX_PATH * sizeof(char));
			_buildMACAddress(pv, szMACAddressOut);  // reversing MAC address in output

			OLE_COLOR color = sensor->Color;
			unsigned char labelCode = sensor->LabelCode;

			bool connected = (sensor->GetConnected() == VARIANT_TRUE) ? true : false;
			if (connected) this->readable_channels.push_back(i);
			BattLevel batteryLevel = sensor->_BattLevel;
			int batt_level_num;
			switch (batteryLevel) {
			case BattLevel_BatteryFull:
				batt_level_num = 4;
				break;

			case BattLevel_BatteryHigh:
				batt_level_num = 3;
				break;

			case BattLevel_BatteryMedium:
				batt_level_num = 2;
				break;

			case BattLevel_Batterylow:
				batt_level_num = 1;
				break;

			default:
				printf("default \n");
				batt_level_num = 0;
				break;
			}

			printf("EMG sensor: MAC %s, Label code: %d, color: blue [color code: 0x%08x], connected %s, battery level: %d\n",
				szMACAddressOut, labelCode, color, connected ? "Yes" : "No", batt_level_num);
		}
	}


	bool bts_bm_manager::ArmStart() {
		this->ObtainQueueSink();
		this->DefineActiveChannels();
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


	int bts_bm_manager::MaxQueueSize() {
		int queueSize = -1;
		vector<int>::iterator channel_ptr;
		// this->readable_channels.clear();
		for (channel_ptr = this->active_channels.begin(); channel_ptr < this->active_channels.end(); channel_ptr++) {
			int ch_index = *channel_ptr;
			if (std::find(this->readable_channels.begin(), this->readable_channels.end(), ch_index) == this->readable_channels.end()) continue;

			EnterCriticalSection(&(this->g_csObject));
			int chQueueSize = g_ptrQueueSink->QueueSize(ch_index);
			LeaveCriticalSection(&(this->g_csObject));
			
			// if (0 == chQueueSize) continue;
			// this->readable_channels.push_back(ch_index);
			if (queueSize == -1) {
				queueSize = chQueueSize;
			}
			else {
				queueSize = min(queueSize, chQueueSize);
			}
		}
		return queueSize;
	}




	void bts_bm_manager::Read_basic(long lastIndex, int nSamples) {
		int queueSize = 0;
		vector<int>::iterator channel_ptr;
		for (channel_ptr = this->active_channels.begin(); channel_ptr < this->active_channels.end(); channel_ptr++) {
			int ch_index = *channel_ptr;
			EnterCriticalSection(&(this->g_csObject));
			queueSize = g_ptrQueueSink->QueueSize(ch_index);
			LeaveCriticalSection(&(this->g_csObject));

			if (0 == queueSize) continue;

			for (int sample = 0; sample < nSamples; sample++)
			{
				__int64 Si = lastIndex + sample;
				float value = 0.0f;
				// get sample value
				EnterCriticalSection(&(this->g_csObject));
				SinkExitStatus exitStatus = SinkExitStatus_Success;

				exitStatus = this->g_ptrQueueSink->Read(
					ch_index,    // channel index
					Si,              // sample index 
					&value           // recovery sample value
				);

				LeaveCriticalSection(&(this->g_csObject));

				if (SinkExitStatus_Success == exitStatus && sample == 0 && value != 0)
				{
					// value is recovery from queue
					// print value in the console
					printf("\tChannel [%d], sample index: %d, value: %f V\n", ch_index, (int)Si, value);
					printf("Queue size: %d \n", queueSize);
				}
				else if (SinkExitStatus_Success != exitStatus) {
					printf("error_%d_____________********************************************************************************\n", (int)Si);
				}
			}

		}
	}


	batch bts_bm_manager::CompleteDequeue() {
		batch measuredBatch;
		int queueSize = this->MaxQueueSize();
		if (queueSize == -1) return measuredBatch;
		measuredBatch.channelsData = map<int, vector<sample>>();
		vector<int>::iterator channel_ptr;
		for (channel_ptr = this->readable_channels.begin(); channel_ptr < this->readable_channels.end(); channel_ptr++) {
			int ch_index = *channel_ptr;
			vector<sample> samplesVector;
			for (int sample_n = 0; sample_n < queueSize; sample_n++)
			{
				__int64 Si;
				float value = 0.0f;
				// get sample value
				EnterCriticalSection(&(this->g_csObject));
				SinkExitStatus exitStatus = SinkExitStatus_Success;

				exitStatus = this->g_ptrQueueSink->Dequeue(
					ch_index,
					&Si,
					&value
				);

				LeaveCriticalSection(&(this->g_csObject));

				sample samp{
					Si,
					value,
					exitStatus
				};
				samplesVector.push_back(samp);
				if (SinkExitStatus_Success == exitStatus && sample_n == 0 && value != 0)
				{
					// value is recovery from queue
					// print value in the console
					printf("\tChannel [%d], sample index: %d, value: %f V\n", ch_index, (int)Si, value);
					printf("Queue size: %d \n", queueSize);
				}
				else if (SinkExitStatus_Success != exitStatus) {
					printf("error_%d_____________********************************************************************************\n", (int)Si);
				}
			}
			measuredBatch.channelsData[ch_index] = samplesVector;
		}
		return measuredBatch;
	}


	char bts_bm_manager::hexDigit(unsigned n)
	{
		if (n < 10)
		{
			return n + '0';
		}
		else
		{
			return (n - 10) + 'A';
		}
	}


	void bts_bm_manager::charToHex(unsigned char c, char hex[3])
	{
		hex[0] = hexDigit(c / 0x10);
		hex[1] = hexDigit(c % 0x10);
		hex[2] = '\0';
	}


	bool bts_bm_manager::_buildMACAddress(char* pInput, char* szAddress)
	{
		const int NITEMS = 8;

		// check
		assert(NULL != pInput);
		assert(NULL != szAddress);

		unsigned char tmpValue;
		char he7[3];
		ZeroMemory(he7, 3 * sizeof(char));
		tmpValue = *pInput; pInput++;
		charToHex(tmpValue, he7);

		char he6[3];
		ZeroMemory(he6, 3 * sizeof(char));
		tmpValue = *pInput; pInput++;
		charToHex(tmpValue, he6);

		char he5[3];
		ZeroMemory(he5, 3 * sizeof(char));
		tmpValue = *pInput; pInput++;
		charToHex(tmpValue, he5);

		char he4[3];
		ZeroMemory(he4, 3 * sizeof(char));
		tmpValue = *pInput; pInput++;
		charToHex(tmpValue, he4);

		char he3[3];
		ZeroMemory(he3, 3 * sizeof(char));
		tmpValue = *pInput; pInput++;
		charToHex(tmpValue, he3);

		char he2[3];
		ZeroMemory(he2, 3 * sizeof(char));
		tmpValue = *pInput; pInput++;
		charToHex(tmpValue, he2);

		char he1[3];
		ZeroMemory(he1, 3 * sizeof(char));
		tmpValue = *pInput; pInput++;
		charToHex(tmpValue, he1);

		char he0[3];
		ZeroMemory(he0, 3 * sizeof(char));
		tmpValue = *pInput; pInput++;
		charToHex(tmpValue, he0);

		// build MAC address (reverse order)
		strcpy(szAddress, he0);
		strcat(szAddress, he1);
		strcat(szAddress, he2);
		strcat(szAddress, he3);
		strcat(szAddress, he4);
		strcat(szAddress, he5);
		strcat(szAddress, he6);
		strcat(szAddress, he7);

		return true;
	}

};