
#include <IPAddressResolver.h>
#include <GlobalNodeListAccess.h>
#include <GlobalStatisticsAccess.h>
#include <UnderlayConfiguratorAccess.h>
#include <RpcMacros.h>
#include "CommonMessages_m.h"

#include <GlobalDhtTestMap1.h>
#include <iostream>
#include <string>
#include <vector>
#include <cmath>


using namespace std;

#include "DHTTestApp1.h"

Define_Module(DHTTestApp1);

using namespace std;

DHTTestApp1::~DHTTestApp1()
{
	 cancelAndDelete(dhttestput_timer);
     cancelAndDelete(dhttestget_timer);
	 cancelAndDelete(dhttestmod_timer);

}

DHTTestApp1::DHTTestApp1()
{
	 dhttestput_timer = NULL;
	 dhttestget_timer = NULL;
	 dhttestmod_timer = NULL;

}


void DHTTestApp1::initializeApp(int stage)
{
    if (stage != MIN_STAGE_APP)
        return;

    // fetch parameters
    debugOutput = par("debugOutput");
    activeNetwInitPhase = par("activeNetwInitPhase");

    mean = par("testInterval");
    p2pnsTraffic = par("p2pnsTraffic");
    deviation = mean / 10;

    if (p2pnsTraffic) {
        ttl = 3600*24*365;
    } else {
        ttl = par("testTtl");
    }

    globalNodeList = GlobalNodeListAccess().get();
    underlayConfigurator = UnderlayConfiguratorAccess().get();
    globalStatistics = GlobalStatisticsAccess().get();

    globalDhtTestMap1 = dynamic_cast<GlobalDhtTestMap1*>(simulation.getModuleByPath(
            "globalObserver.globalFunctions[0].function"));

    if (globalDhtTestMap1 == NULL) {
        throw cRuntimeError("DHTTestApp1::initializeApp(): "
                                "GlobalDhtTestMap1 module not found!");
    }

    // statistics
    numSent = 0;
    numGetSent = 0;
    numGetError = 0;
    numGetSuccess = 0;
    numPutSent = 0;
    numPutError = 0;
    numPutSuccess = 0;

    //initRpcs();
    WATCH(numSent);
    WATCH(numGetSent);
    WATCH(numGetError);
    WATCH(numGetSuccess);
    WATCH(numPutSent);
    WATCH(numPutError);
    WATCH(numPutSuccess);

    nodeIsLeavingSoon = false;

    // initiate test message transmission
    dhttestput_timer = new cMessage("dhttest_put_timer");
    dhttestget_timer = new cMessage("dhttest_get_timer");
    dhttestmod_timer = new cMessage("dhttest_mod_timer");

    if (mean > 0) {
        scheduleAt(simTime() + truncnormal(mean, deviation),
                   dhttestput_timer);
        scheduleAt(simTime() + truncnormal(mean + mean / 3,
                                                      deviation),
                                                      dhttestget_timer);
        scheduleAt(simTime() + truncnormal(mean + 2 * mean / 3,
                                                      deviation),
                                                      dhttestmod_timer);
    }
}

void DHTTestApp1::handleRpcResponse(BaseResponseMessage* msg,
                                   const RpcState& state, simtime_t rtt)
{
    RPC_SWITCH_START(msg)
    RPC_ON_RESPONSE( DHTputCAPI ) {
        handlePutResponse(_DHTputCAPIResponse,
                          check_and_cast<DHTStatsContext*>(state.getContext()));
        EV << "[DHTTestApp1::handleRpcResponse()]\n"
           << "    DHT Put RPC Response received: id=" << state.getId()
           << " msg=" << *_DHTputCAPIResponse << " rtt=" << rtt
           << endl;
        break;
    }
    RPC_ON_RESPONSE(DHTgetCAPI)
    {
        handleGetResponse(_DHTgetCAPIResponse,
                          check_and_cast<DHTStatsContext*>(state.getContext()));
        EV << "[DHTTestApp1::handleRpcResponse()]\n"
           << "    DHT Get RPC Response received: id=" << state.getId()
           << " msg=" << *_DHTgetCAPIResponse << " rtt=" << rtt
           << endl;
        break;
    }
    RPC_SWITCH_END()
}

void DHTTestApp1::handlePutResponse(DHTputCAPIResponse* msg,
                                   DHTStatsContext* context)
{
    DHTEntry entry = {context->value, simTime() + ttl};

    globalDhtTestMap1->insertEntry(context->key, entry);

    if (context->measurementPhase == false) {
        // don't count response, if the request was not sent
        // in the measurement phase
        delete context;
        return;
    }

    if (msg->getIsSuccess()) {
        RECORD_STATS(numPutSuccess++);
        RECORD_STATS(globalStatistics->addStdDev("DHTTestApp1: PUT Latency (s)",
                               SIMTIME_DBL(simTime() - context->requestTime)));
    } else {
        RECORD_STATS(numPutError++);
    }

    delete context;
}

void DHTTestApp1::handleGetResponse(DHTgetCAPIResponse* msg,
                                   DHTStatsContext* context)
{
    if (context->measurementPhase == false) {
        // don't count response, if the request was not sent
        // in the measurement phase
        delete context;
        return;
    }

    RECORD_STATS(globalStatistics->addStdDev("DHTTestApp1: GET Latency (s)",
                               SIMTIME_DBL(simTime() - context->requestTime)));

    if (!(msg->getIsSuccess())) {
        //cout << "DHTTestApp: success == false" << endl;
        RECORD_STATS(numGetError++);
        delete context;
        return;
    }

    const DHTEntry* entry = globalDhtTestMap1->findEntry(context->key);

    if (entry == NULL) {
        //unexpected key
        RECORD_STATS(numGetError++);
        //cout << "DHTTestApp1: unexpected key" << endl;
        delete context;
        return;
    }

    if (simTime() > entry->endtime) {
        //this key doesn't exist anymore in the DHT, delete it in our hashtable

        globalDhtTestMap1->eraseEntry(context->key);
        delete context;

        if (msg->getResultArraySize() > 0) {
            RECORD_STATS(numGetError++);
            //cout << "DHTTestApp1: deleted key still available" << endl;
            return;
        } else {
            RECORD_STATS(numGetSuccess++);
            //cout << "DHTTestApp1: success (1)" << endl;
            return;
        }
    } else {
        delete context;
        if ((msg->getResultArraySize() > 0) && (msg->getResult(0).getValue() != entry->value)) {
            RECORD_STATS(numGetError++);
            //cout << "DHTTestApp1: wrong value" << endl;
            //cout << "value: " << msg->getResult(0).getValue() << endl;
            return;
        } else {
            RECORD_STATS(numGetSuccess++);
            //cout << "DHTTestApp1: success (2)" << endl;
            return;
        }
    }

}

void DHTTestApp1::handleTraceMessage(cMessage* msg)
{
    char* cmd = new char[strlen(msg->getName()) + 1];
    strcpy(cmd, msg->getName());

    if (strlen(msg->getName()) < 5) {
        delete[] cmd;
        delete msg;
        return;
    }

    if (strncmp(cmd, "PUT ", 4) == 0) {
        // Generate key
        char* buf = cmd + 4;

        while (!isspace(buf[0])) {
            if (buf[0] == '\0')
                throw cRuntimeError("Error parsing PUT command");
            buf++;
        }

        buf[0] = '\0';
        BinaryValue b(cmd + 4);
        OverlayKey destKey(OverlayKey::sha1(b));

        // get value
        buf++;

        // build putMsg
        DHTputCAPICall* dhtPutMsg = new DHTputCAPICall();
        dhtPutMsg->setKey(destKey);
        dhtPutMsg->setValue(buf);
        dhtPutMsg->setTtl(ttl);
        dhtPutMsg->setIsModifiable(true);
        RECORD_STATS(numSent++; numPutSent++);
        sendInternalRpcCall(TIER1_COMP, dhtPutMsg,
                new DHTStatsContext(globalStatistics->isMeasuring(),
                                    simTime(), destKey));
    } else if (strncmp(cmd, "GET ", 4) == 0) {
        // Get key
        BinaryValue b(cmd + 4);
        OverlayKey key(OverlayKey::sha1(b));

        DHTgetCAPICall* dhtGetMsg = new DHTgetCAPICall();
        dhtGetMsg->setKey(key);
        RECORD_STATS(numSent++; numGetSent++);
        sendInternalRpcCall(TIER1_COMP, dhtGetMsg,
                new DHTStatsContext(globalStatistics->isMeasuring(),
                                    simTime(), key));
    } else {
        throw cRuntimeError("Unknown trace command; "
                                "only GET and PUT are allowed");
    }

    delete[] cmd;
    delete msg;
}

void DHTTestApp1::handleTimerEvent(cMessage* msg)
{
    if (msg->isName("dhttest_put_timer")) {
        // schedule next timer event
        scheduleAt(simTime() + truncnormal(mean, deviation), msg);

        // do nothing if the network is still in the initialization phase
        if (((!activeNetwInitPhase) && (underlayConfigurator->isInInitPhase()))
                || underlayConfigurator->isSimulationEndingSoon()
                || nodeIsLeavingSoon)
            return;

        if (p2pnsTraffic) {
            if (globalDhtTestMap1->p2pnsNameCount < 4*globalNodeList->getNumNodes()) {
                for (int i = 0; i < 4; i++) {
                    // create a put test message with random destination key
                    OverlayKey destKey = OverlayKey::random();
                    DHTputCAPICall* dhtPutMsg = new DHTputCAPICall();
                    dhtPutMsg->setKey(destKey);
                    dhtPutMsg->setValue(generateRandomValue());
                    dhtPutMsg->setTtl(ttl);
                    dhtPutMsg->setIsModifiable(true);

                    RECORD_STATS(numSent++; numPutSent++);
                    sendInternalRpcCall(TIER1_COMP, dhtPutMsg,
                            new DHTStatsContext(globalStatistics->isMeasuring(),
                                                simTime(), destKey, dhtPutMsg->getValue()));
                    globalDhtTestMap1->p2pnsNameCount++;
                }
            }
            cancelEvent(msg);
            return;
        }

        // create a put test message with random destination key
        OverlayKey destKey = OverlayKey::random();
        DHTputCAPICall* dhtPutMsg = new DHTputCAPICall();
        dhtPutMsg->setKey(destKey);
        dhtPutMsg->setValue(generateRandomValue());
        dhtPutMsg->setTtl(ttl);
        dhtPutMsg->setIsModifiable(true);

        RECORD_STATS(numSent++; numPutSent++);
        sendInternalRpcCall(TIER1_COMP, dhtPutMsg,
                new DHTStatsContext(globalStatistics->isMeasuring(),
                                    simTime(), destKey, dhtPutMsg->getValue()));
    } else if (msg->isName("dhttest_get_timer")) {
        scheduleAt(simTime() + truncnormal(mean, deviation), msg);

        // do nothing if the network is still in the initialization phase
        if (((!activeNetwInitPhase) && (underlayConfigurator->isInInitPhase()))
                || underlayConfigurator->isSimulationEndingSoon()
                || nodeIsLeavingSoon) {
            return;
        }

        if (p2pnsTraffic && (uniform(0, 1) > ((double)mean/1800.0))) {
            return;
        }

        const OverlayKey& key = globalDhtTestMap1->getRandomKey();

        if (key.isUnspecified()) {
            EV << "[DHTTestApp1::handleTimerEvent() @ " << thisNode.getIp()
               << " (" << thisNode.getKey().toString(16) << ")]\n"
               << "    Error: No key available in global DHT test map!"
               << endl;
            return;
        }

        DHTgetCAPICall* dhtGetMsg = new DHTgetCAPICall();
        dhtGetMsg->setKey(key);
        RECORD_STATS(numSent++; numGetSent++);

        sendInternalRpcCall(TIER1_COMP, dhtGetMsg,
                new DHTStatsContext(globalStatistics->isMeasuring(),
                                    simTime(), key));
    } else if (msg->isName("dhttest_mod_timer")) {
        scheduleAt(simTime() + truncnormal(mean, deviation), msg);

        // do nothing if the network is still in the initialization phase
        if (((!activeNetwInitPhase) && (underlayConfigurator->isInInitPhase()))
                || underlayConfigurator->isSimulationEndingSoon()
                || nodeIsLeavingSoon) {
            return;
        }

        if (p2pnsTraffic) {
            if (globalDhtTestMap1->p2pnsNameCount >= 4*globalNodeList->getNumNodes()) {
                const OverlayKey& key = globalDhtTestMap1->getRandomKey();

                if (key.isUnspecified())
                    return;

                DHTputCAPICall* dhtPutMsg = new DHTputCAPICall();
                dhtPutMsg->setKey(key);
                dhtPutMsg->setValue(generateRandomValue());
                dhtPutMsg->setTtl(ttl);
                dhtPutMsg->setIsModifiable(true);

                RECORD_STATS(numSent++; numPutSent++);
                sendInternalRpcCall(TIER1_COMP, dhtPutMsg,
                        new DHTStatsContext(globalStatistics->isMeasuring(),
                                            simTime(), key, dhtPutMsg->getValue()));
            }
            cancelEvent(msg);
            return;
        }

        const OverlayKey& key = globalDhtTestMap1->getRandomKey();

        if (key.isUnspecified())
            return;

        DHTputCAPICall* dhtPutMsg = new DHTputCAPICall();
        dhtPutMsg->setKey(key);
        dhtPutMsg->setValue(generateRandomValue());
        dhtPutMsg->setTtl(ttl);
        dhtPutMsg->setIsModifiable(true);

        RECORD_STATS(numSent++; numPutSent++);
        sendInternalRpcCall(TIER1_COMP, dhtPutMsg,
                new DHTStatsContext(globalStatistics->isMeasuring(),
                                    simTime(), key, dhtPutMsg->getValue()));
    }
}


BinaryValue DHTTestApp1::generateRandomValue()
{
    char value[DHTTESTAPP_VALUE_LEN + 1];

    for (int i = 0; i < DHTTESTAPP_VALUE_LEN; i++) {
        value[i] = intuniform(0, 25) + 'a';
    }

    value[DHTTESTAPP_VALUE_LEN] = '\0';
    return BinaryValue(value);
}

void DHTTestApp1::handleNodeLeaveNotification()
{
    nodeIsLeavingSoon = true;
}

void DHTTestApp1::finishApp()
{
    simtime_t time = globalStatistics->calcMeasuredLifetime(creationTime);

    if (time >= GlobalStatistics::MIN_MEASURED) {
        // record scalar data
        globalStatistics->addStdDev("DHTTestApp1: Sent Total Messages/s",
                                    numSent / time);
        globalStatistics->addStdDev("DHTTestApp1: Sent GET Messages/s",
                                    numGetSent / time);
        globalStatistics->addStdDev("DHTTestApp1: Failed GET Requests/s",
                                    numGetError / time);
        globalStatistics->addStdDev("DHTTestApp1: Successful GET Requests/s",
                                    numGetSuccess / time);

        globalStatistics->addStdDev("DHTTestApp1: Sent PUT Messages/s",
                                    numPutSent / time);
        globalStatistics->addStdDev("DHTTestApp1: Failed PUT Requests/s",
                                    numPutError / time);
        globalStatistics->addStdDev("DHTTestApp1: Successful PUT Requests/s",
                                    numPutSuccess / time);

        if ((numGetSuccess + numGetError) > 0) {
            globalStatistics->addStdDev("DHTTestApp1: GET Success Ratio",
                                        (double) numGetSuccess
                                        / (double) (numGetSuccess + numGetError));
        }
    }
}
HashFunction1::HashFunction1(int PreValue, int TypeValue, int IdValue,int NbHashP, int NbHashT, int NbHashI, int HashType):DHTTestApp1()
{
	if(PreValue <= 0)
	{
		throw cRuntimeError ("PreValue must be > 0");
	}
	if(TypeValue <= 0)
	{
		throw cRuntimeError ("TypeValue must be > 0");
	}
	if(IdValue <= 0)
	{
		throw cRuntimeError ("IdValue must be >0");
	}
	if(NbHashP <= 0)
	{
		throw cRuntimeError ("NbHashP must be > 0");
	}
	if(NbHashT <= 0)
	{
		throw cRuntimeError ("NbHashT must be > 0");
	}
	if(NbHashI <= 0)
	{
		throw cRuntimeError ("NbHashI must be > 0");
	}
	this->PreValue= PreValue;
	this->TypeValue= TypeValue;
	this->IdValue= IdValue;
	this->NbHashP= NbHashP;
	this->NbHashT= NbHashT;
	this->NbHashI= NbHashI;
	this->HashType= HashType;
	if(this->HashType==0)
	{
		throw cRuntimeError ("HashType must be known");
	}
	}
void HashFunction1::Clear()
{
}

int HashFunction1::Hash(Key* key)
{
	DHTTestApp1::generateRandomValue();
   BP = this->value;
	if(BP==0)
	{
		throw cRuntimeError("buffer reference is null");
	}
	BT =  this->value;
	if(BT==0)
	{
		throw cRuntimeError("buffer reference is null");
	}
	BI = this->value;
	if(BI==0)
	{
		throw cRuntimeError("buffer reference is null");
	}
	if(LengthP==0)
	{
		throw cRuntimeError("key length must be> 0");
	}
	if(LengthT==0)
	{
		throw cRuntimeError("key length must be> 0");
	}
	if(LengthI==0)
	{
		throw cRuntimeError("key length must be > 0");
	}

for(int i = 0, j = 0; i < 	NbHashP; i++)
{
	j = MurmurHash(key, len , seed );
	ResultP= abs(j% 65535);
}
for(int i = 0, InitvalT=0; i < NbHashT; i++)
{
	InitvalT = MurmurHash(key, len, seed);
	ResultT= abs(InitvalT% 65535);
}
i_ibm= IBM_CRC16(buf, len);
temp= i_ibm;
i_ibm= i_ibm/3;
i_ccitt= CCITT_CRC16(data_p, length);
i_ccitt= i_ccitt/3;
Hash1= i_ccitt;
Hash2= MurmurHash(key, len, seed);

for(int i = 0; i < NbHashI; i++)
{
	ResultI= abs(Hash1 + i * Hash2)% 65535;
}
return ResultP;
return ResultT;
return ResultI;
}
unsigned int MurmurHash ( const void *key, int len, unsigned int seed )
{
	const unsigned int m = 0x5bd1e995;
	const int r = 24;
	unsigned int h = seed ^ len;
	const unsigned char * data = (const unsigned char *)key;
	while(len >= 4)
	{
		unsigned int k = *(unsigned int *)data;
		k *= m;
		k ^= k >> r;
		k *= m;
		h *= m;
		h ^= k;
		data += 4;
		len -= 4;
	}
	switch(len)
	{
	case 3: h ^= data[2] << 16;
	case 2: h ^= data[1] << 8;
	case 1: h ^= data[0];
	h *= m;
	};
	h ^= h >> 13;
	h *= m;
	h ^= h >> 15;
	return h;
}
unsigned short IBM_CRC16(char* buf, int len)
{
	unsigned short crc= 0;
	int j;
	unsigned char i,b;
	for ( j = 0; j < len; j++)
	{
		b = buf[j];
		for ( i = 0; i < 8; i++)
		{
			crc = ((b ^ (unsigned char)crc) & 1) ? (( crc >> 1) ^ 0xA001) : (crc >> 1);
			b >>= 1;
		}
	}
	return crc;
}
unsigned short CCITT_CRC16(char *data_p, unsigned length)
{
	unsigned char i;
	unsigned short data;
	unsigned short crc = 0xffff;
	unsigned int PLOY= 0x8408;
	if (length == 0)
		return ( ~crc);
	do
	{
		for ( i = 0, data=(unsigned short)0xff & *data_p++; i < 8; i++, data >>= 1)
		{
if ((crc & 0x0001) ^ (data & 0x0001))
	                          crc = (crc >> 1) ^ PLOY;
else crc >>= 1;
		}
	} while (--length);

	crc = ~crc;
	data = crc;
	crc = (crc << 8) | (data >> 8 & 0xff);

return (crc);
}
EDTFFilter::EDTFFilter() { }
EDTFFilter::~EDTFFilter() { }
ETDFFilter::EDTFFilter(int PreValueSize, int TypeValueSize, int IdValueSize, int NbHashP, int NbHashT, int NbHashI, int HashTypeP,int HashTypeT, int HashTypeI, int nr): HashFunction1(PreValue, TypeValue, IdValue, NbHashP, NbHashT, NbHashI, HashType)
{
 this->nr = nr;
 this->currentNbRrcord = 0;



 Array[0][0]= (this->PreValueSize && this->TypeValueSize && this->IdValueSize);
 Array[0][1]= (this->NbHashP && this->NbHashT && this->NbHashI);
 Array[0][2]= (this->HashTypeP && this->HashTypeT && this->HashTypeI);






}




void EDTFFilter:: Add(Key* key){
	if(key == 0)
	{
		throw cRuntimeError("key can not be null");
	}
	EDTFFilter* bf = (this-> GetActiveStandardBF());


	if(bf == 0)
	{
		AddRow();
        bf = sizeof Array / sizeof Array[0];
		currentNbRrcord = 0;
	}
    Add(key);
	currentNbRrcord++;
}
void EDTFFilter:: And(Filter* filter){
	if(filter== 0 || !(reinterpret_cast<EDTFFilter*>(filter) != 0)|| filter->PreValueSize != this->PreValueSize
			|| filter->TypeValueSize != this->TypeValueSize || filter->IdValueSize != this->IdValueSize
			|| filter->NbHashP != this->NbHashP || filter->NbHashT != this->NbHashT
			|| filter->NbHashI != this->NbHashI)

		{
			throw  cRuntimeError("filters cannot be and-ed");
	}
	EDTFFilter1* dbf = reinterpret_cast<EDTFFilter1*>(filter);
		dbf = sizeof Array / sizeof Array[0];
		if(dbf != (sizeof Array / sizeof Array[0]) ||  nr != this->nr)
		{
	      throw cRuntimeError("filters cannot be And-ed");
		}
		for(i= 0; i< sizeof Array / sizeof Array[0]; i++)
		{
			for(j= 0; j < p; j++)
			{
				k = Array[i][j];
				And (filter );

			}
		}

}
bool EDTFFilter:: MemberTest1(Key* key){
	if(key == 0){
		return true;
	}
    return false;
}


void EDTFFilter:: Not()

{
	for ( i = 0; i <  sizeof Array / sizeof Array[0]; i++)
		{
			for( j = 0; j < p; j++)
			{
	            k =  Array[i][j];
	            Not();

			}
		}



}
void EDTFFilter:: Or(Filter* filter){
	if(filter== 0 || !(reinterpret_cast<EDTFFilter*>(filter) != 0)|| filter->PreValueSize != this->PreValueSize
			|| filter->TypeValueSize != this->TypeValueSize || filter->IdValueSize != this->IdValueSize
			|| filter->NbHashP != this->NbHashP || filter->NbHashT != this->NbHashT
			|| filter->NbHashI != this->NbHashI)

	{
		throw  cRuntimeError("filters cannot be or-ed");
}
	EDTFFilter1* dbf = reinterpret_cast<EDTFFilter1*>(filter);
	dbf = sizeof Array / sizeof Array[0];
	if(dbf != (sizeof Array / sizeof Array[0]) ||  nr != this->nr)
	{
      throw cRuntimeError("filters cannot be or-ed");
	}
	for(i= 0; i< sizeof Array / sizeof Array[0]; i++)
	{
		for(j= 0; j < p; j++)
		{
			k = Array[i][j];
			Or(filter);

		}
	}
}
void EDTFFilter:: Xor(Filter* filter){
	if(filter== 0 || !(reinterpret_cast<EDTFFilter*>(filter) != 0)|| filter->PreValueSize != this->PreValueSize
			|| filter->TypeValueSize != this->TypeValueSize || filter->IdValueSize != this->IdValueSize
			|| filter->NbHashP != this->NbHashP || filter->NbHashT != this->NbHashT
			|| filter->NbHashI != this->NbHashI)

		{
			throw  cRuntimeError("filters cannot be xor-ed");
	}
	EDTFFilter1* dbf = reinterpret_cast<EDTFFilter1*>(filter);
		dbf = sizeof Array / sizeof Array[0];
		if(dbf != (sizeof Array / sizeof Array[0]) ||  nr != this->nr)
		{
	      throw cRuntimeError("filters cannot be Xor-ed");
		}
		for(i= 0; i< sizeof Array / sizeof Array[0]; i++)
		{
			for(j= 0; j < p; j++)
			{
				k = Array[i][j];
				Xor(filter);

			}
		}
}
string EDTFFilter:: ToString(){

	res = stringBulider;
	for(i = 0; i < sizeof Array / sizeof Array[0]; i++){
				for(j = 0; j <= p ; j++){
					k = Array[i][j];

				}

}
	return res = this->ToString();
	}
void EDTFFilter:: Write(DataOutput* out) {
out-> nr = this-> nr;
	out-> currentNbRrcord = this->currentNbRrcord;
	out =  sizeof Array / sizeof Array[0];
	for(i = 0; i < sizeof Array / sizeof Array[0]; i++){
			for(j = 0; j <= p ; j++){
				k = Array[i][j];
				Write(out);
			}
	}






}

void EDTFFilter:: ReadFields(DataInput* in){
	nr = in->ReadInt;
	currentNbRrcord = in->ReadInt;
	len = in->ReadInt;
	for(i = 0; i < sizeof Array / sizeof Array[0]; i++){
				for(j = 0; j <= p ; j++){
					k = Array[i][j];
					ReadFields(in);


				}
	}
}
void EDTFFilter:: AddRow() {
	k = sizeof Array / sizeof Array[0];
	k = k+1;
	tmp = k;
	for(i = 0; i < sizeof Array / sizeof Array[0]; i++){
		for(j = 0; j <= p ; j++){
			tmp= Array[i][j];
		}
	}
	k = sizeof Array / sizeof Array[0];
	k = k -1;
	tmp = k;
    tmp= ((this->PreValueSize && this->TypeValueSize && this->IdValueSize) ,
    		(this->NbHashP && this->NbHashT && this->NbHashI) ,
    		(this->HashTypeP && this->HashTypeT && this->HashTypeI));
    Array[i][j] =  tmp;

}
EDTFFilter::EDTFFilter* EDTFFilter:: GetActiveStandardBF()
{
	if (currentNbRrcord >= nr )
	{
		return 0;
	}


    return sizeof Array / sizeof Array[0];
}
















