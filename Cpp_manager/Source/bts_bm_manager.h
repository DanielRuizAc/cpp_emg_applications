#pragma once
#include "stdafx.h"
#include <process.h>

// Include BioDAQ template libraries (registration has been done!)

#import "mscorlib.tlb"
#import "bts.biodaq.drivers.tlb"
#import "bts.biodaq.core.tlb"

using namespace std;
using namespace mscorlib;
using namespace bts_biodaq_core;
using namespace bts_biodaq_drivers;


struct sample {
	__int64 index;
	float value;
	ExitStatus status;
};



class bts_bm_manager
{
private:
	IBioDAQPtr ptrBioDAQ;
	IBMViewListPtr bmViewList;
	IChannelViewListPtr chViewList;
	IPortListPtr ptrPortList;
	IPortCOMPtr ptrCOMPort;
	long protocolItems;
	IQueueSinkPtr g_ptrQueueSink;             // Queue sink interface.
	CRITICAL_SECTION g_csObject;
public:
	bts_bm_manager();
	~bts_bm_manager();
	bool ConnectionCOMPort(int comNum, BaudRate baud);
	bool ConfigureAquisition(CodingType type, bool compression, EMGChannelRangeCodes gain, int nSamples);
	void ObtainQueueSink();

	bool ArmStart();

	void Clean();
	bool Stop();

	void Read(long lastIndex ,int nSamples);

	
};

