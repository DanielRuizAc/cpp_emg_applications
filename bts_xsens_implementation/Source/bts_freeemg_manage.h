#pragma once
#include "pch.h"

#import "mscorlib.tlb"
#import "bts.biodaq.drivers.tlb"
#import "bts.biodaq.core.tlb"

using namespace std;
using namespace mscorlib;
using namespace bts_biodaq_core;
using namespace bts_biodaq_drivers;

namespace bts_manage_tools {

	const int NITEMS = 8;

	struct sample {
		__int64 index;
		float value;
		SinkExitStatus status;
	};

	struct batch {
		map<int, vector<sample>> channelsData;
	};

	class bts_bm_manager
	{
	private:
		IBioDAQPtr ptrBioDAQ;
		IBMViewListPtr bmViewList;
		IChannelViewListPtr chViewList;
		IPortListPtr ptrPortList;
		IPortCOMPtr ptrCOMPort;
		ISensorViewDictionaryPtr ptrSensorViewDictionary;
		long protocolItems;
		IQueueSinkPtr g_ptrQueueSink;             // Queue sink interface.
		CRITICAL_SECTION g_csObject;
		vector<int> active_channels;
		vector<int> readable_channels;

		static bool _buildMACAddress(char* pInput, char* szAddress);
		static void charToHex(unsigned char c, char hex[3]);
		static char hexDigit(unsigned n);


	public:
		bts_bm_manager();
		~bts_bm_manager();
		bool ConnectionCOMPort(int comNum, BaudRate baud);
		bool ConnectionCOMPort(int comNum);
		bool ConfigureAquisition(CodingType type, bool compression, EMGChannelRangeCodes gain, int nSamples);
		bool ConfigureAquisition(bool compression, int nSamples);
		void ObtainQueueSink();

		void DefineActiveChannels();
		void PrintSensorData();

		bool ArmStart();

		void Clean();
		bool Stop();

		int MaxQueueSize();
		void Read_basic(long lastIndex, int nSamples);
		batch CompleteDequeue();
	};
}