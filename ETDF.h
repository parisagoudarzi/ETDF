ifndef __DHTTESTAPP_H_
#define __DHTTESTAPP_H_

#include <omnetpp.h>

#include <GlobalNodeList.h>
#include <GlobalStatistics.h>
#include <UnderlayConfigurator.h>
#include <TransportAddress.h>
#include <OverlayKey.h>
#include <InitStages.h>
#include <BinaryValue.h>
#include <BaseApp.h>
#include <set>
#include <sstream>
#include <cmath>
#include <string>
#include <vector>



class GlobalDhtTestMap1;

/**
 * A simple test application for the DHT layer
 *
 * A simple test application that does random put and get calls
 * on the DHT layer
 *
 * @author Ingmar Baumgart
 */
class DHTTestApp1 : public BaseApp
{
private:
    /**
     * A container used by the DHTTestApp to
     * store context information for statistics
     *
     * @author Ingmar Baumgart
     */
    class DHTStatsContext : public cPolymorphic
    {
    public:
        bool measurementPhase;
        simtime_t requestTime;
        OverlayKey key;
        BinaryValue value;

        DHTStatsContext(bool measurementPhase,
                        simtime_t requestTime,
                        const OverlayKey& key,
                        const BinaryValue& value = BinaryValue::UNSPECIFIED_VALUE) :
            measurementPhase(measurementPhase), requestTime(requestTime),
            key(key), value(value) {};
    };


    void initializeApp(int stage);

    /**
     * Get a random key of the hashmap
     */
    OverlayKey getRandomKey();

    /**
     * generate a random human readable binary value
     */


    void finishApp();

    /**
     * processes get responses
     *
     * method to handle get responses
     * should be overwritten in derived application if needed
     * @param msg get response message
     * @param context context object used for collecting statistics
     */
    virtual void handleGetResponse(DHTgetCAPIResponse* msg,
                                   DHTStatsContext* context);

    /**
     * processes put responses
     *
     * method to handle put responses
     * should be overwritten in derived application if needed
     * @param msg put response message
     * @param context context object used for collecting statistics
     */
    virtual void handlePutResponse(DHTputCAPIResponse* msg,
                                   DHTStatsContext* context);

    /**
     * processes self-messages
     *
     * method to handle self-messages
     * should be overwritten in derived application if needed
     * @param msg self-message
     */
    virtual void handleTimerEvent(cMessage* msg);

    /**
     * handleTraceMessage gets called of handleMessage(cMessage* msg)
     * if a message arrives at trace_in. The command included in this
     * message should be parsed and handled.
     *
     * @param msg the command message to handle
     */
    virtual void handleTraceMessage(cMessage* msg);

    virtual void handleNodeLeaveNotification();

    // see RpcListener.h
    void handleRpcResponse(BaseResponseMessage* msg, const RpcState& state,
                           simtime_t rtt);

    UnderlayConfigurator* underlayConfigurator; /**< pointer to UnderlayConfigurator in this node */

    GlobalNodeList* globalNodeList; /**< pointer to GlobalNodeList in this node*/

    GlobalStatistics* globalStatistics; /**< pointer to GlobalStatistics module in this node*/
    GlobalDhtTestMap1* globalDhtTestMap1; /**< pointer to the GlobalDhtTestMap module */

    // parameters
    bool debugOutput; /**< debug output yes/no?*/
    double mean; //!< mean time interval between sending test messages
    double deviation; //!< deviation of time interval
    int ttl; /**< ttl for stored DHT records */
    bool p2pnsTraffic; //!< model p2pns application traffic */
    bool activeNetwInitPhase; //!< is app active in network init phase?

    // statistics
    int numSent; /**< number of sent packets*/
    int numGetSent; /**< number of get sent*/
    int numGetError; /**< number of false get responses*/
    int numGetSuccess; /**< number of false get responses*/
    int numPutSent; /**< number of put sent*/
    int numPutError; /**< number of error in put responses*/
    int numPutSuccess; /**< number of success in put responses*/

    cMessage *dhttestput_timer, *dhttestget_timer, *dhttestmod_timer;
    bool nodeIsLeavingSoon; //!< true if the node is going to be killed shortly

    static const int DHTTESTAPP_VALUE_LEN = 20;



public:
    DHTTestApp1();

    /**
     * virtual destructor
     */
    virtual ~DHTTestApp1();
    BinaryValue generateRandomValue();

};
class HashFunction1 : public DHTTestApp1
{
   protected:

	int PreValue;
	int TypeValue;
	int IdValue;
	int NbHashP;
	int NbHashT;
	int NbHashI;
	int HashType;
	int ResultP;
	int ResultT;
	int ResultI;
    char  *BP;
	char *BT;
	char *BI;
	char LengthP;
	char *LengthT;
	char *LengthI;
	int PreI;
	int len;
	unsigned int seed;

    unsigned short i_ccitt;
    char *data_p;
    unsigned short i_ibm;
    char *buf;
    unsigned short temp;

    unsigned length;
    int Hash1;
    int Hash2;
    int h;
    int crc;
    char* getBytes;
    char* value;



	HashFunction1(HashFunction1 *HashFunction1);

   public:
	HashFunction1(int PreValue=0, int TypeValue=0, int IdValue=0, int NbHashP=0, int NbHashT=0, int NbHashI=0, int HashType=0);
	virtual void Clear();
	 struct Key
	{
		int* key;
		int* getBytes();

	};


     int Hash(Key* key);
     virtual unsigned int MurmurHash ( const void *key, int len, unsigned int seed ){return h;}
    virtual unsigned short CCITT_CRC16(char *data_p, unsigned length){return crc;}
    virtual unsigned short IBM_CRC16(char *buf, int len){return crc;}
};
class ETDF Filter : public HashFunction1
{
private:
	int nr ;
	int currentNbRrcord ;
	int* PreValueSize;
	int* TypeValueSize;
	int* IdValueSize;
	int* NbHashP;
	int* NbHashT;
	int* NbHashI;
	int* HashTypeP;
	int* HashTypeT;
	int* HashTypeI;
     int tmp;
     int tmp1;
     int tmp2;
     static int const H=2;
     int k;
	 static int const p=2;
     int Array[][p];
     int** matrix;
     int sizex;
     static int const sizey=2;

     string res;
     		string stringBulider;








	int i;
	int j;


public:
	struct EDTF1
	{
		EDTF1* dbf;
		~EDTF1();
		EDTF* nr;
		EDTF* Array[][p];
		int* dbf1;
	};

	struct EDTF
	{

		EDTFFilter* bf;
		EDTFFilter* length;
		EDTFFilter* nullptr;
		EDTFFilter* Array[][p];
		EDTFFilter* matrix[][sizey];




	};
	struct Key
	{
		int* key;
	};
	struct Filter
	{
		int* filter;
		int* PreValueSize;
		int* TypeValueSize;
		int* IdValueSize;
		int* NbHashP;
		int* NbHashT;
		int* NbHashI;



	};
	struct DataOutput
	{
		int* out;
        int nr;
        int currentNbRrcord ;

		int* WriteInt;
	};
	struct DataInput
	{
		int* in;
		int nr;
		int ReadInt;
		int len;
	};
	struct String
	{
		int* string;
		int* res;
		int* stringBulider;
	};
	EDTFFilter();
	virtual ~EDTFFilter();
	EDTFFilter(int PreValueSize, int TypeValueSize, int IdValueSize, int NbHashP, int NbHashT, int NbHashI,int HashTypeP,int HashTypeT, int HashTypeI, int nr);

    virtual void Add(Key* key);
    virtual void And(Filter* filter);
    virtual bool MemberTest1(Key* key);
    virtual void Not();
    virtual void Or(Filter* filter);
    virtual void Xor(Filter* filter);

    virtual string ToString();
    virtual void Write(DataOutput* out);
    virtual void ReadFields(DataInput* in);
private:
    virtual void AddRow();
    virtual BloomFilter* GetActiveStandardBF();






};



#endif
