/**************************************************************************************************************/
/* Ephys2Plx.c                                                                                                */
/* Version:                                                                                                   */
/*         1.0       Jan 22, 2014 - Guangxing Li                                                              */
/*         1.1       July 22, 2014 - GL  (bug fix)                                                            */
/**************************************************************************************************************/


# include "mex.h"
# include <stdio.h>
//# include <stdbool.h>
# include <stdint.h>
# include <string.h>
# include <math.h>
# include "matrix.h"


#pragma pack(push,1)                    // deal with bad designed struct "NEUEVFLT" and "DIGLABEL"

#define Pause_Signal 'p'
#define Resume_Signal 'c'

#define LimitWordsInWaveform 120


// Ephys file header (is followed by the channel descriptors)
/*
struct Ephys_Settings
{

} Ephys_Settings, *iEphys_Settings;     // Settings.xml*/

struct Ephys_ContinuousHead
{
    char            Format[25];         // format "Open Ephys Data Format"
    float           Version;            // File format version "0.2000"
    unsigned int    Header_bytes;       // Header length. "1024"
    char            Desciption[250];    // descripthon.
    char            Date_created[20];   // File create date "20-Jan-2014 160913"
    char            Channel[10];        // Channel Name "CH1"
    char            ChannelType[15];    // Channel Type "Continuous"
    unsigned int    SampleRate;         // Sampling Rate "3000"
    unsigned int    BlockLength;        // BlockLength "1024"
    unsigned int    BufferSize;         // BufferSize "1024"
    float           BitVolts;           // BitVolts "0.1950"
} Ephys_ContinuousHead, **iEphys_ContinuousHead;                 // ContinuousHead

struct Ephys_SpikeHead
{
    char            Format[25];         // format "Open Ephys Data Format"
    float           Version;            // File format version "0.2000"
    unsigned int    Header_bytes;       // Header length. "1024"
    char            Desciption[366];    // descripthon.
    char            Date_created[20];   // File create date "20-Jan-2014 160913"
    char            Electrode[25];      // Electrode Name "Single electrode 1"
    unsigned int    Num_channels;       // num_channels "1"
    unsigned int    SampleRate;         // Sampling Rate "30000"
    
} Ephys_SpikeHead, **iEphys_SpikeHead;    // Spike Header

struct Ephys_EventHead
{
    char            Format[25];         // format "Open Ephys Data Format"
    float           Version;            // File format version "0.2000"
    unsigned int    Header_bytes;       // Header length. "1024"
    char            Desciption[250];    // descripthon.
    char            Date_created[20];   // File create date "20-Jan-2014 160913"
    char            Channel[10];        // Channel Name "CH1"
    char            ChannelType[15];    // Channel Type "Event"
    unsigned int    SampleRate;         // Sampling Rate "3000"
    unsigned int    BlockLength;        // BlockLength "1024"
    unsigned int    BufferSize;         // BufferSize "1024"
    float           BitVolts;           // BitVolts "1"
} Ephys_EventHead, *iEphys_EventHead;    // Event Head

struct Ephys_ContinuousDataPacket
{
	long long      Timestamp;           // int64 Timestamp
	unsigned short NumSamples;          // uint16 Number indicating the samples per record (always 1024, at least for now)
	unsigned short RecordNum;           // uint16 Recording number (version 0.2 and higher)
} Ephys_ContinuousDataPacket, **iEphys_ContinuousDataPacket;     // Continuous Data Packet

struct Ephys_ContinuousDataPacket_Data
{
	short          *Data;               // int16 samples
	char           *Marker;             // 10-byte record marker (0 1 2 3 4 5 6 7 8 255)
}Ephys_ContinuousDataPacket_Data, **iEphys_ContinuousDataPacket_Data;


struct Ephys_EventDataPacket
{
    long long      Timestamp;           // int64 Timestamp
    short          SamplePosition;      // int16 sample position within a buffer
    unsigned char  EventType;           // uint8 event type
    unsigned char  processorID;         // uint8 processor ID
    unsigned char  EventID;             // uint8 event ID
    unsigned char  EventChannel;        // uint8 event channel
    unsigned short RecordNum;           // uint16 recording number (version 0.2 and higher)
} Ephys_EventDataPacket, *iEphys_EventDataPacket;

struct Ephys_SpikeDataPacket
{
    unsigned char  EventType;           // uint8 event type
    long long      Timestamp;           // int64 Timestamp
	long long	   Timestamp_soft;		// int64 software Timestamp
	unsigned short source;				// uint16 Internal electrode index
    unsigned short NumChannels;         // uint16 number of channels (N)
    unsigned short NumSamplesPerSpike;  // uint16 number of samples per spike (M)
	unsigned short sortedId;			// unit16 sorted unit ID
	unsigned short ElectrodeID;         // uint16 electrodeID
	unsigned short channel;				// uint16 channel index
	unsigned char  color[3];
	float		   pcProj[2];
	unsigned short samplingFrequencyHz; // uint16 sampling frequency
} Ephys_SpikeDataPacket, **iEphys_SpikeDataPacket;

struct Ephys_SpikeDataPacket_Data
{
    short		   *Data;               // pointer of uint16 array; N*M uint16 number of samples (individual channels are contiguous); NOTE: in Ephys this is uint16, but we will convert to short before put it in
    float		   *Gains;              // pointer of float array; N float gains 
    unsigned short *Thresholds;         // pointer of uint16 array; N uint16 thresholds used for spike extraction 
} Ephys_SpikeDataPacket_Data, **iEphys_SpikeDataPacket_Data;

// file header (is followed by the channel descriptors)
struct  PL_FileHeader 
{
    unsigned int MagicNumber;   // = 0x58454c50;
	
    int     Version;            // Version of the data format; determines which data items are valid
    char    Comment[128];       // User-supplied comment 
    int     ADFrequency;        // Timestamp frequency in hertz
    int     NumDSPChannels;     // Number of DSP channel headers in the file
    int     NumEventChannels;   // Number of Event channel headers in the file
    int     NumSlowChannels;    // Number of A/D channel headers in the file
    int     NumPointsWave;      // Number of data points in waveform
    int     NumPointsPreThr;    // Number of data points before crossing the threshold
	
    int     Year;               // Time/date when the data was acquired
    int     Month; 
    int     Day; 
    int     Hour; 
    int     Minute; 
    int     Second; 
	
    int     FastRead;           // reserved
    int     WaveformFreq;       // waveform sampling rate; ADFrequency above is timestamp freq 
    double  LastTimestamp;      // duration of the experimental session, in ticks
    
    // The following 6 items are only valid if Version >= 103
    char    Trodalness;                 // 1 for single, 2 for stereotrode, 4 for tetrode
    char    DataTrodalness;             // trodalness of the data representation
    char    BitsPerSpikeSample;         // ADC resolution for spike waveforms in bits (usually 12)
    char    BitsPerSlowSample;          // ADC resolution for slow-channel data in bits (usually 12)
    unsigned short SpikeMaxMagnitudeMV; // the zero-to-peak voltage in mV for spike waveform adc values (usually 3000)
    unsigned short SlowMaxMagnitudeMV;  // the zero-to-peak voltage in mV for slow-channel waveform adc values (usually 5000)
    
    // Only valid if Version >= 105
    unsigned short SpikePreAmpGain;       // usually either 1000 or 500
	
    char    Padding[46];   
} plxFileHeader, *iplxFileHeader;       // so that the file header is 256 bytes

struct PL_ChanHeader 
{
    char    Name[32];       // Name given to the DSP channel
    char    SIGName[32];    // Name given to the corresponding SIG channel
    int     Channel;        // DSP channel number, 1-based
    int     WFRate;         // When MAP is doing waveform rate limiting, this is limit w/f per sec divided by 10
    int     SIG;            // SIG channel associated with this DSP channel 1 - based
    int     Ref;            // SIG channel used as a Reference signal, 1- based
    int     Gain;           // actual gain divided by SpikePreAmpGain. For pre version 105, actual gain divided by 1000. 
    int     Filter;         // 0 or 1
    int     Threshold;      // Threshold for spike detection in a/d values
    int     Method;         // Method used for sorting units, 1 - boxes, 2 - templates
    int     NUnits;         // number of sorted units
    short   Template[5][64];// Templates used for template sorting, in a/d values
    int     Fit[5];         // Template fit 
    int     SortWidth;      // how many points to use in template sorting (template only)
    short   Boxes[5][2][4]; // the boxes used in boxes sorting
    int     SortBeg;        // beginning of the sorting window to use in template sorting (width defined by SortWidth)
    char    Comment[128];
    int     Padding[11];
} plxChanHeader, *iplxChanHeader;      //The spike channel header is 1020 bytes

struct PL_EventHeader 
{
    char    Name[32];       // name given to this event
    int     Channel;        // event number, 1-based
    char    Comment[128];
    int     Padding[33];
}plxEventHeader, *iplxEventHeader;       //The event channel header is 296 bytes

struct PL_SlowChannelHeader 
{
    char    Name[32];       // name given to this channel
    int     Channel;        // channel number, 0-based
    int     ADFreq;         // digitization frequency
    int     Gain;           // gain at the adc card
    int     Enabled;        // whether this channel is enabled for taking data, 0 or 1
    int     PreAmpGain;     // gain at the preamp

    // As of Version 104, this indicates the spike channel (PL_ChanHeader.Channel) of
    // a spike channel corresponding to this continuous data channel. 
    // <=0 means no associated spike channel.
    int     SpikeChannel;

    char    Comment[128];
    int     Padding[28];
}plxSlowHeader, *iplxSlowHeader;          //The continuous channel header is 296 bytes

struct PL_DataBlockHeader
{
    short   Type;                       // Data type; 1=spike, 4=Event, 5=continuous
    unsigned short   UpperByteOf5ByteTimestamp; // Upper 8 bits of the 40 bit timestamp
    unsigned int     TimeStamp;                 // Lower 32 bits of the 40 bit timestamp
    short   Channel;                    // Channel number
    short   Unit;                       // Sorted unit number; 0=unsorted
    short   NumberOfWaveforms;          // Number of waveforms in the data to folow, usually 0 or 1
    short   NumberOfWordsInWaveform;    // Number of samples per waveform in the data to follow
}plxDataHeader, *iplxDataHeader; // 16 bytes


short usingDSPList[144];      // List of on using channels in Cerebus data file (Plexon Offline-Sorter can not handle 144 channels, probably limi = 130)
short NumUsingDSP;            // Number of on using channels in Cerebus data file (Plexon Offline-Sorter can not handle 144 channels, probably limi = 130)

// Counters for the number of timestamps and waveforms in each channel and unit.
// channel numbers are 1-based - array entry at [0] is unused
int     tscounts[130][5]; // number of timestamps[channel][unit]
int     wfcounts[130][5]; // number of waveforms[channel][unit]

int ephys_tscounts[144][16];    // number of timestamps[channel][unit] in Cerebus data file
int ephys_wfcounts[144][16];    // number of waveforms[channel][unit] in Cerebus data file

// Starting at index 300, this array also records the number of samples for the 
// continuous channels.  Note that since EVCounts has only 512 entries, continuous 
// channels above channel 211 do not have sample counts.
int     evcounts[512];    // number of timestamps[event_number]
int cere_evcounts[512];   // number of timestamps[event_number] in Cerebus data file

int     continus[130];
int     eventChannels[130];
int     numEventChannels;

char *fnames[]={"MagicNumber","Version","Comment","ADFrequency","NumDSPChannels","NumEventChannels","NumSlowChannels",              /*Field Names in plx File Header*/
                  "NumPointsWave","NumPointsPreThr","Year","Month","Day","Hour","Minute","Second","FastRead","WaveformFreq",
                  "LastTimestamp","Trodalness","DataTrodalness","BitsPerSpikeSample","BitsPerSlowSample","SpikeMaxMagnitudeMV",
                  "SlowMaxMagnitudeMV","SpikePreAmpGain","Padding","tscounts","wfcounts","evcounts","continus"};
char *chnames[]={"Name","SIGName","Channel","WFRate","SIG","Ref","Gain","Filter","Threshold","Method","NUnits","Template","Fit",    /*Field Name in Spike Channel Hrader*/
                  "SortWidth","Boxes","SortBeg","Comment","Padding"};
char *ehnames[]={"Name","Channel","Comment","Padding"};                                                                             /*Field Name in Event Channel Header*/         
char *shnames[]={"Name","Channel","ADFreq","Gain","Enabled","PreAmpGain","SpikeChannel","Comment","Padding"};                       /*Field Name in Continuous Channel Header*/

int  chanHeaderOrder [63], eventHeaderOrder[512], slowHeaderOrder[130];

int  countNEUEVWAV, countNEUEVLBL, countNEUEVFLT, countDIGLABEL, countOTHER;

unsigned long long NumEventPacket, NumSpikePacket, NumContinuPacket;

unsigned long long StartTimeStamp;

int **iNSx_21_ChannelID;

char SerialDigiInputBuffer[8];
int SDIBufferSize;
double SerialDigiInput;

long Nev_FileLength, Percent_Finished;

FILE *iSpikeFiles[128], *iContinuousFiles[256], *iEventFile[2], *EventTemp;
int NumSpikeFiles, NumContinuousFiles, *iContinuousChannels;

int NumSamplesPerSpikePlex;


/**
	
 * Ansi C "itoa" based on Kernighan & Ritchie's "Ansi C"
	
 * with slight modification to optimize for specific architecture:
	
 */

/*
void strreverse(char* begin, char* end) {
	
	char aux;
	
	while(end>begin)
		aux=*end, *end--=*begin, *begin++=aux;
	
}
	
void itoa(int value, char* str, int base) {
	
	static char num[] = "0123456789abcdefghijklmnopqrstuvwxyz";
	char* wstr=str;
	int sign;
	
	div_t res;
	
	// Validate base
	if ((base<2) || (base>35)){ *wstr='\0'; return; }
	
	// Take care of sign
	if ((sign=value) < 0) value = -value;
	
	// Conversion. Number is reversed.
	do {
		res = div(value,base);
		*wstr++ = num[res.rem];
	}while(value=res.quot);
	if(sign<0) *wstr++='-';
	*wstr='\0';
    
	// Reverse string
	strreverse(str,wstr-1);
}
*/

/**********************************************************************
    Read Big Endian
/*********************************************************************/

uint16_t be16_to_cpu(const uint8_t *buf)
{
	return ((uint16_t)buf[1]) | (((uint16_t)buf[0]) << 8);
}

/************************************************************************
 *
 *
 ************************************************************************/

void ephysReadDataPacket(){
    int i, j;
    FILE *file;
    short RecordNum_Temp;
    bool endEventFile;
    bool foundSameChannel;
    
    file = iEventFile[0];
	//Debug Info
	//mexPrintf("File pointer for EventFile: %d\n", file->_file);
    rewind(file);
    fseek(file, Ephys_EventHead.Header_bytes, 0);
    endEventFile = false;
    numEventChannels = 0;

    while(!endEventFile)
    {
        if(!feof(file))
        {
            fread(&Ephys_EventDataPacket,sizeof(struct Ephys_EventDataPacket),1,file);  //read one packet from first file
        }
		else
		{
			endEventFile = true;
		}
        
        if ((Ephys_EventDataPacket.Timestamp == 0) && (Ephys_EventDataPacket.processorID == 0) && (Ephys_EventDataPacket.EventChannel==0))
        {
            endEventFile = true;
        }
        else
        {
			if ((Ephys_EventDataPacket.processorID == 100) && (Ephys_EventDataPacket.EventType == 3) && (Ephys_EventDataPacket.EventChannel != 0)) //only count TTL signal from FPGA, and not count CH=0
            {
                foundSameChannel = false;
                for(i = 0; i < numEventChannels; i++)
                {
                    if (eventChannels[i]==Ephys_EventDataPacket.EventChannel)
                    {
                        foundSameChannel=true;
                    }
                }

                if(!foundSameChannel)
                {
                    numEventChannels++;
                    eventChannels[numEventChannels-1]=Ephys_EventDataPacket.EventChannel;
                }
            }
        }
    }
    //Debug Info
    //mexPrintf("Timestamp = %d\n",Ephys_EventDataPacket.Timestamp);
    //mexPrintf("SamplePosition = %d\n",Ephys_EventDataPacket.SamplePosition);
    //mexPrintf("EventType = %d\n",Ephys_EventDataPacket.EventType);
    //mexPrintf("processorID = %d\n",Ephys_EventDataPacket.processorID);
    //mexPrintf("EventID = %d\n",Ephys_EventDataPacket.EventID);
    //mexPrintf("EventChannel = %d\n",Ephys_EventDataPacket.EventChannel);
    //mexPrintf("RecordNum = %d\n",Ephys_EventDataPacket.RecordNum);
    
	NumSamplesPerSpikePlex = 0;

    for (i=0;i<NumSpikeFiles;i++)
    {
        file = iSpikeFiles[i];
		//debug info
		mexPrintf("File pointer for SpikeFile: %d\n", file->_file);
        rewind(file);
        mexPrintf("Ephys_SpikeHead.Header_bytes: %d\n",iEphys_SpikeHead[i]->Header_bytes);
        fseek(file, iEphys_SpikeHead[i]->Header_bytes, 0);
        iEphys_SpikeDataPacket[i] = (struct Ephys_SpikeDataPacket *) mxCalloc(1,sizeof(struct Ephys_SpikeDataPacket));
        if (iEphys_SpikeDataPacket[i]==NULL) {
            mexErrMsgTxt("Out of memory CODE:2-1 !");
            exit(1);
        }
        if(!feof(file))
        {
            fread(iEphys_SpikeDataPacket[i],sizeof(struct Ephys_SpikeDataPacket),1,file);//read one packet from each file
        }
        
        //Debug Info
       // mexPrintf("EventType = %d\n",iEphys_SpikeDataPacket[i]->EventType);
       // mexPrintf("Timestamp = %d\n",iEphys_SpikeDataPacket[i]->Timestamp);
       // mexPrintf("ElectrodeID = %d\n",iEphys_SpikeDataPacket[i]->ElectrodeID);
       // mexPrintf("NumChannels = %d\n",iEphys_SpikeDataPacket[i]->NumChannels);
       // mexPrintf("NumSamplesPerSpike = %d\n",iEphys_SpikeDataPacket[i]->NumSamplesPerSpike);
        
        iEphys_SpikeDataPacket_Data[i] = (struct Ephys_SpikeDataPacket_Data *) mxCalloc(1,sizeof(struct Ephys_SpikeDataPacket_Data));
        if (iEphys_SpikeDataPacket[i]==NULL) {
            mexErrMsgTxt("Out of memory CODE:2-2 !");
            exit(1);
        }
        
		if ((iEphys_SpikeDataPacket[i]->NumChannels > 0) && (iEphys_SpikeDataPacket[i]->NumSamplesPerSpike > 0))
		{
			if ((iEphys_SpikeDataPacket[i]->EventType != 4) || (iEphys_SpikeDataPacket[i]->NumChannels != 1))
			{
				mexErrMsgTxt("Spike data file error OR number of channels in file > 1 !");
				exit(1);
			}

			if (NumSamplesPerSpikePlex == 0)
			{
				NumSamplesPerSpikePlex = iEphys_SpikeDataPacket[i]->NumSamplesPerSpike;
			}
			else
			{
				if (NumSamplesPerSpikePlex != iEphys_SpikeDataPacket[i]->NumSamplesPerSpike)
				{
					mexErrMsgTxt("The length of spike waveform different in different file !");
					exit(1);
				}
			}
			
			iEphys_SpikeDataPacket_Data[i]->Data = (unsigned short *)mxCalloc(iEphys_SpikeDataPacket[i]->NumChannels * iEphys_SpikeDataPacket[i]->NumSamplesPerSpike, sizeof(unsigned short));
			iEphys_SpikeDataPacket_Data[i]->Gains = (float *)mxCalloc(iEphys_SpikeDataPacket[i]->NumChannels, sizeof(float));
			iEphys_SpikeDataPacket_Data[i]->Thresholds = (unsigned short *)mxCalloc(iEphys_SpikeDataPacket[i]->NumChannels, sizeof(unsigned short));
			if ((iEphys_SpikeDataPacket_Data[i]->Data == NULL) || (iEphys_SpikeDataPacket_Data[i]->Gains == NULL) || (iEphys_SpikeDataPacket_Data[i]->Thresholds == NULL)) {
				mexErrMsgTxt("Out of memory CODE:2-4 !");
				exit(1);
			}

			if (!feof(file))
			{
				fread(iEphys_SpikeDataPacket_Data[i]->Data, sizeof(unsigned short), iEphys_SpikeDataPacket[i]->NumChannels * iEphys_SpikeDataPacket[i]->NumSamplesPerSpike, file);
				fread(iEphys_SpikeDataPacket_Data[i]->Gains, sizeof(float), iEphys_SpikeDataPacket[i]->NumChannels, file);
				fread(iEphys_SpikeDataPacket_Data[i]->Thresholds, sizeof(unsigned short), iEphys_SpikeDataPacket[i]->NumChannels, file);

				//debug data
				//mexPrintf("Gains = %d\n", (iEphys_SpikeDataPacket_Data[i]->Gains)[0]);
				//mexPrintf("Thresholds = %d\n", (iEphys_SpikeDataPacket_Data[i]->Thresholds)[0]);
				//mexPrintf("RecordNum = %d\n", iEphys_SpikeDataPacket_Data[i]->RecordNum);
			}
		}
		else
		{
			mexPrintf("No Spikes saved in SpikeFile: %d\n", i+1);
			iEphys_SpikeDataPacket[i]->ElectrodeID = chanHeaderOrder[i];   //at least put electrodeID
			iEphys_SpikeDataPacket_Data[i]->Data = (unsigned short *)mxCalloc(1, sizeof(unsigned short));
			iEphys_SpikeDataPacket_Data[i]->Gains = (unsigned short *)mxCalloc(1, sizeof(unsigned short));
			iEphys_SpikeDataPacket_Data[i]->Thresholds = (unsigned short *)mxCalloc(1, sizeof(unsigned short));
			if ((iEphys_SpikeDataPacket_Data[i]->Data == NULL) || (iEphys_SpikeDataPacket_Data[i]->Gains == NULL) || (iEphys_SpikeDataPacket_Data[i]->Thresholds == NULL)) {
				mexErrMsgTxt("Out of memory CODE:2-5 !");
				exit(1);
			}
			*iEphys_SpikeDataPacket_Data[i]->Data = 0;
			*iEphys_SpikeDataPacket_Data[i]->Gains = 0;
			*iEphys_SpikeDataPacket_Data[i]->Thresholds = 0;

			//debug data
			//mexPrintf("Gains = %d\n", (iEphys_SpikeDataPacket_Data[i]->Gains)[0]);
			//mexPrintf("Thresholds = %d\n", (iEphys_SpikeDataPacket_Data[i]->Thresholds)[0]);
			//mexPrintf("Data = %d\n", (iEphys_SpikeDataPacket_Data[i]->Data)[0]);
		}
    }

    for (i=0;i<NumContinuousFiles;i++)
    {
        file = iContinuousFiles[i];
		//Debug infor
		//mexPrintf("EventTemp File pointer for ContinuousFile: %d\n",file->_file);
        rewind(file);
        //mexPrintf("Ephys_ContinuousHead.Header_bytes: %d\n",iEphys_ContinuousHead[i]->Header_bytes);
        fseek(file, iEphys_ContinuousHead[i]->Header_bytes, 0);
        iEphys_ContinuousDataPacket[i] = (struct Ephys_ContinuousDataPacket *) mxCalloc(1,sizeof(struct Ephys_ContinuousDataPacket));
		iEphys_ContinuousDataPacket_Data[i] = (struct Ephys_ContinuousDataPacket_Data *) mxCalloc(1, sizeof(struct Ephys_ContinuousDataPacket_Data));

		if ((iEphys_ContinuousDataPacket[i] == NULL) || iEphys_ContinuousDataPacket_Data[i]==NULL)
		{
			mexErrMsgTxt("ephysReadDataPacket() Error, Code:0001 !");
			exit(1);
		}

        if(!feof(file))
        {
            //fread(iEphys_ContinuousDataPacket[i],sizeof(struct Ephys_ContinuousDataPacket),1,file);//read one packet from each file
			long long      Timestamp;           // int64 Timestamp
			unsigned short NumSamples;          // uint16 Number indicating the samples per record (always 1024, at least for now)
			unsigned short RecordNum;           // uint16 Recording number (version 0.2 and higher)

			fread(&Timestamp, sizeof(long long), 1, file);
			fread(&NumSamples, sizeof(unsigned short), 1, file);
			fread(&RecordNum, sizeof(unsigned short), 1, file);
			iEphys_ContinuousDataPacket[i]->Timestamp = Timestamp;
			iEphys_ContinuousDataPacket[i]->NumSamples = NumSamples;
			iEphys_ContinuousDataPacket[i]->RecordNum = RecordNum;


			iEphys_ContinuousDataPacket_Data[i]->Data = (short *)mxCalloc(iEphys_ContinuousDataPacket[i]->NumSamples, sizeof(short));
			iEphys_ContinuousDataPacket_Data[i]->Marker = (char *)mxCalloc(10, sizeof(char));
			if ((iEphys_ContinuousDataPacket_Data[i]->Data == NULL) || iEphys_ContinuousDataPacket_Data[i]->Marker == NULL)
			{
				mexErrMsgTxt("ephysReadDataPacket() Error, Code:0002!");
				exit(1);
			}

			fread(iEphys_ContinuousDataPacket_Data[i]->Data, sizeof(short), iEphys_ContinuousDataPacket[i]->NumSamples, file);
			fread(iEphys_ContinuousDataPacket_Data[i]->Marker, sizeof(char), 10, file);

			//Debug Info
			//mexPrintf("Timestamp = %d\n", iEphys_ContinuousDataPacket[i]->Timestamp);
			//mexPrintf("NumSamples = %d\n", iEphys_ContinuousDataPacket[i]->NumSamples);
			//mexPrintf("RecordNum = %d\n", iEphys_ContinuousDataPacket[i]->RecordNum);
        }
    }
}


/************************************************************************
 *
 *
 ************************************************************************/
/*
void cereReadNsxDataPackets (FILE *fp) {
    int i;
    short *DataPoints;                      // This corresponds to a single data collection point. There will be exactly "Channel Count" number of data points. They will be sorted in the same order as they are presented in "Channel ID". Data will be stored as digital values.
    
    iNSx_DataPacket = &Nsx_DataPacket;
    
    while(!feof(fp)) {
    
    fread(iNSx_DataPacket,sizeof(struct CE_NSX_DataPacket),1,fp);
    
     mexPrintf("Header:          %d\n",Nsx_DataPacket.Header);
     mexPrintf("Timestamp:       %d\n",Nsx_DataPacket.Timestamp);
     mexPrintf("NumOfDataPoints: %d\n",Nsx_DataPacket.NumOfDataPoints);
     
     DataPoints = mxCalloc(Nsx_DataPacket.NumOfDataPoints*Nsx_BasicHeader.ChannelCount,sizeof(short));      //Data points array = Number of Data Points * Number of Channels
     
     if (DataPoints == NULL) {
        mexErrMsgTxt("Out of memory CODE:5-1 !");
        exit(1);
    }
     
     mexPrintf("Size of DataPoints: %d\n",Nsx_DataPacket.NumOfDataPoints*Nsx_BasicHeader.ChannelCount);
     
     fread(DataPoints,Nsx_DataPacket.NumOfDataPoints*Nsx_BasicHeader.ChannelCount*sizeof(short),1,fp);

    }
}*/


/************************************************************************
*
*
************************************************************************/
void WriteEventDataPacket(int EventChannel, long long timeStamp, FILE *plx_fp)
{
	struct PL_DataBlockHeader plxDataHeader_temp;

	NumEventPacket++;

	plxDataHeader_temp.Type = 4;    //Event
	plxDataHeader_temp.UpperByteOf5ByteTimestamp = 0;
	plxDataHeader_temp.TimeStamp = timeStamp;

	plxDataHeader_temp.Channel = EventChannel;  //Strart
	plxDataHeader_temp.Unit = 0;
	plxDataHeader_temp.NumberOfWaveforms = 0;
	plxDataHeader_temp.NumberOfWordsInWaveform = 0;
	fwrite(&plxDataHeader_temp, sizeof(struct PL_DataBlockHeader), 1, plx_fp);
}
/************************************************************************
 *
 *
 ************************************************************************/
void WriteSpikeDataPacket(int iSPKFile, FILE *plx_fp){
    char *iVoidData;
    struct PL_DataBlockHeader plxDataHeader_temp;
    char *iWaveform;             // The sampled waveform of the spike. - NOTE: Raw A/D values in Plexon data file are "short integers" which is 2 Bytes per datapoint.
    unsigned char Write_Plx_DigiInput;
    char binbuf[8]; //Binary string
    
    NumSpikePacket = NumSpikePacket + 1;
    plxDataHeader_temp.Type = 1;    //Spike
    plxDataHeader_temp.UpperByteOf5ByteTimestamp = 0;
	plxDataHeader_temp.TimeStamp = (int)iEphys_SpikeDataPacket[iSPKFile]->Timestamp - StartTimeStamp;    //add code to handle longlong
	plxDataHeader_temp.Channel = iEphys_SpikeDataPacket[iSPKFile]->ElectrodeID + 1;  //plexon is 1-based
    plxDataHeader_temp.Unit = 0;    //no such info in ephys
    plxDataHeader_temp.NumberOfWaveforms = 1;
    plxDataHeader_temp.NumberOfWordsInWaveform = iEphys_SpikeDataPacket[iSPKFile]->NumSamplesPerSpike;

    fwrite(&plxDataHeader_temp,sizeof(struct PL_DataBlockHeader),1,plx_fp);
    fwrite(iEphys_SpikeDataPacket_Data[iSPKFile]->Data,sizeof(short),iEphys_SpikeDataPacket[iSPKFile]->NumSamplesPerSpike,plx_fp);
    
    //mexPrintf("Spike Channel = %d; Unit = %d \n",plxDataHeader_temp.Channel, plxDataHeader_temp.Unit);

    //mexPrintf("Unit Classification Number   = %d\n",NEV_SPIKEData_temp.UnitClassifiNum);

    //mexPrintf("Writting Spike Ch-%d\n",plxDataHeader_temp.Channel);
    
}


/************************************************************************
 *
 *
 ************************************************************************/
void WriteContinueDataPack (int iConFile, FILE *plx_fp) {
    int j,k;
	short *iPlxWaveform;                      // This corresponds to a single data collection point. There will be exactly "Channel Count" number of data points. They will be sorted in the same order as they are presented in "Channel ID". Data will be stored as digital values.
    //short *DataPoints;
    int len_EphysWaveform, len_PlxWaveform;
    struct PL_DataBlockHeader plxDataHeader_temp;
    int iPart;
    FILE *file;
	short tempValue, tempValue_Raw;
    
    len_EphysWaveform = iEphys_ContinuousDataPacket[iConFile]->NumSamples;
    
	iPlxWaveform = mxCalloc(LimitWordsInWaveform, sizeof(short));
    //DataPoints = mxCalloc(len_EphysWaveform,sizeof(short));   
    
    if (iPlxWaveform == NULL){
        mexErrMsgTxt("Out of memory CODE:WCDP-1 !");
        exit(1);
    }

    iPart = 0;
    
    //file = iContinuousFiles[iConFile];
    
    //fread(DataPoints,len_EphysWaveform*sizeof(short),1,file);

    for (j=0;j<((int)(len_EphysWaveform/LimitWordsInWaveform)+1);j++)
    {
        if (j < (int)(len_EphysWaveform/LimitWordsInWaveform))
        {
            for (k=0; k<LimitWordsInWaveform; k++) {
				if (iContinuousChannels[iConFile] == 36)
				{
					tempValue_Raw = iEphys_ContinuousDataPacket_Data[iConFile]->Data[j*LimitWordsInWaveform + k];
				}

                iPlxWaveform[k]=iEphys_ContinuousDataPacket_Data[iConFile]->Data[j*LimitWordsInWaveform+k];
				tempValue = iPlxWaveform[k];

                //iPlxWaveform[k]=0;
            }
            len_PlxWaveform = LimitWordsInWaveform;
        }
        else
        {
            for (k=0; k<(len_EphysWaveform-len_PlxWaveform*j); k++) {
                iPlxWaveform[k]=iEphys_ContinuousDataPacket_Data[iConFile]->Data[j*LimitWordsInWaveform+k];
                //iPlxWaveform[k]=0;
            }
            len_PlxWaveform = len_EphysWaveform-len_PlxWaveform*j;
        }
        plxDataHeader_temp.Type = 5;                         //Continuous
        plxDataHeader_temp.UpperByteOf5ByteTimestamp = 0;
		plxDataHeader_temp.TimeStamp = iEphys_ContinuousDataPacket[iConFile]->Timestamp + j*LimitWordsInWaveform - StartTimeStamp;
        //mexPrintf("Timestamp In Plx:       %d\n",iStartTS_NSx[i]);
        plxDataHeader_temp.Channel = iContinuousChannels[iConFile] - 1;       //***0-based A/D channel number (0 to 255)
        plxDataHeader_temp.Unit = 0;                                     //Always 0
        plxDataHeader_temp.NumberOfWaveforms = 1;                        //Always 1
        plxDataHeader_temp.NumberOfWordsInWaveform = len_PlxWaveform;

		if (iContinuousChannels[iConFile] == 36)
		{
			short value1 = iPlxWaveform[0];
			short value2 = iPlxWaveform[1];
			short value3 = iPlxWaveform[2];
			short value4 = iPlxWaveform[3];
			int testvalue = 0;
		}

        fwrite(&plxDataHeader_temp,sizeof(struct PL_DataBlockHeader),1,plx_fp);
		fwrite(iPlxWaveform, len_PlxWaveform*sizeof(short), 1, plx_fp);
        NumContinuPacket++;
    }

    //mexPrintf("Writting Continuous Ch-%d\n",plxDataHeader_temp.Channel);

    
    mxFree(iPlxWaveform);
    //mxFree(DataPoints);
}


/************************************************************************
 *
 *
 ************************************************************************/
void ReadandWriteDataPacket (FILE *plx_fp) {
    int i,j;
	int indexP;
    char binbuf[8]; //Binary string
    unsigned int fDataPack_Availiable, fData_Ready;   
    int *iContinuPart;
	unsigned short *tempSamples;
    long long *iStartTS_Con, *iLastRawTS_Con;
    char iMinStartTS_Con;
    long long vMinStartTS_Con;
    char *State_SpikeFiles, *State_ConFiles;
    char State_EventFile;
	short *voidData;
    FILE *file;
    unsigned short RecordNum_Temp;
	unsigned short conFinished;
	bool startEventWrited = false;
	bool stopEventWrited = false;
	int Con_FileLength, Con_FileIndex, Percent_Finished;

    int Debug_Count=0;
    
    enum SPIKEFILE_STATE
    {
        SPST_NONE = 0x00,   //Clean
        SPST_FEND = 0x01,   //File End
        SPST_DATA = 0x02,   //DataReady
    } SPST;
    
    enum CONFILE_STATE
    {
        COST_NONE = 0x00,   //Clean
        COST_FEND = 0x01,   //File End
        COST_DATA = 0x02,   //DataReady
    } COST;
    
    enum EVENTFILE_STATE
    {
        EVET_NONE = 0x00,   //Clean
        EVET_FEND = 0x01,   //File End
        EVET_DATA = 0x02,   //DataReady
    } EVET;
    
    NumEventPacket = 0;
    NumSpikePacket = 0;
    NumContinuPacket = 0;
    
    Percent_Finished = 0;
    
    iContinuPart = mxCalloc(NumContinuousFiles,sizeof(int));
    iStartTS_Con = mxCalloc(NumContinuousFiles,sizeof(long long));
    iLastRawTS_Con = mxCalloc(NumContinuousFiles,sizeof(long long));
    State_SpikeFiles = mxCalloc(NumSpikeFiles,sizeof(char));
    State_ConFiles = mxCalloc(NumContinuousFiles,sizeof(char));
         
    if ((iContinuPart==NULL) || (iStartTS_Con==NULL) || (iLastRawTS_Con==NULL) || (State_SpikeFiles==NULL) || (State_ConFiles==NULL)){
        mexErrMsgTxt("Out of memory CODE:6-1 !");
        exit(1);
    }
    
    State_EventFile = EVET_NONE;
    
    for (i =0; i<NumContinuousFiles; i++) {
        iStartTS_Con[i] = 0;
        iLastRawTS_Con[i] = 0;
        iContinuPart[i] = 1;                    //At least have 1 part
        State_ConFiles[i] = COST_NONE;
    }
    
	file = iEventFile[0];
	rewind(file);
	fseek(file, Ephys_EventHead.Header_bytes, 0);

    for (i =0; i<NumSpikeFiles; i++) {
        State_SpikeFiles[i] = SPST_NONE;
		file = iSpikeFiles[i];
		rewind(file);
		fseek(file, iEphys_SpikeHead[i]->Header_bytes, 0);
    }
	
	Con_FileLength = 0;
	Con_FileIndex = 0;

	for (i = 0; i < NumContinuousFiles; i++) {
		file = iContinuousFiles[i];
		fseek(file, 0, SEEK_END);
		if (Con_FileLength < ftell(file))
		{
			Con_FileLength = ftell(file);
			Con_FileIndex = i;
		}
		rewind(file);
		fseek(file, iEphys_ContinuousHead[i]->Header_bytes, 0);
	}
    
    
    
    fDataPack_Availiable = NumSpikeFiles + NumContinuousFiles + 1;   //one Event file
    

    //fDataPack_Availiable = 2;
    
    //mexPrintf("DataPack_Available: %d\n",fDataPack_Availiable);
    
    mexPrintf("Reading and Writing data:");

	Percent_Finished = 0;

    while(fDataPack_Availiable > 0) { 
		for (i=0;i<NumSpikeFiles;i++)
        {
            if(((State_EventFile & EVET_DATA) == 0) && ((State_EventFile & EVET_FEND)==0))   //No data in memory
            {
                file=iEventFile[0];
                if(!feof(file)){
                     fread(&Ephys_EventDataPacket,sizeof(struct Ephys_EventDataPacket),1,file);  //read one packet
                     if ((Ephys_EventDataPacket.Timestamp == 0) && (Ephys_EventDataPacket.processorID == 0) && (Ephys_EventDataPacket.EventChannel==0))
                    {
                        State_EventFile|=EVET_FEND;
                        fDataPack_Availiable = fDataPack_Availiable - 1;
                    }
                     else
                     {
                         State_EventFile|=EVET_DATA; //Data Ready
                     }
                }
				else
				{
					State_EventFile |= EVET_FEND;
					fDataPack_Availiable = fDataPack_Availiable - 1;
				}
            }
            
            if (((State_SpikeFiles[i] & SPST_DATA) == 0) && ((State_SpikeFiles[i] & SPST_FEND)==0))   //No data in memory
            {
                file = iSpikeFiles[i];
                //mexPrintf("SpikeChannel-%d\n",i);
                if(!feof(file)){
                    fread(iEphys_SpikeDataPacket[i],sizeof(struct Ephys_SpikeDataPacket),1,file);

					mxFree(iEphys_SpikeDataPacket_Data[i]->Data);
					mxFree(iEphys_SpikeDataPacket_Data[i]->Gains);
					mxFree(iEphys_SpikeDataPacket_Data[i]->Thresholds);

					int tempNumChannels = iEphys_SpikeDataPacket[i]->NumChannels;
					int tempNumSamplesPerSpike = iEphys_SpikeDataPacket[i]->NumSamplesPerSpike;
					
					//Debug Info
       // mexPrintf("EventType = %d\n",iEphys_SpikeDataPacket[i]->EventType);
       // mexPrintf("Timestamp = %d\n",iEphys_SpikeDataPacket[i]->Timestamp);
       // mexPrintf("ElectrodeID = %d\n",iEphys_SpikeDataPacket[i]->ElectrodeID);
       // mexPrintf("NumChannels = %d\n",iEphys_SpikeDataPacket[i]->NumChannels);
       // mexPrintf("NumSamplesPerSpike = %d\n",iEphys_SpikeDataPacket[i]->NumSamplesPerSpike);

					if ((iEphys_SpikeDataPacket[i]->NumChannels > 0) && (iEphys_SpikeDataPacket[i]->NumSamplesPerSpike > 0))
					{

						if ((iEphys_SpikeDataPacket[i]->EventType != 4) || (iEphys_SpikeDataPacket[i]->NumChannels != 1))
						{
							mexErrMsgTxt("Spike data file error OR number of channels in file > 1 !");
							exit(1);
						}

						iEphys_SpikeDataPacket_Data[i]->Data = (short *)mxCalloc(iEphys_SpikeDataPacket[i]->NumChannels * iEphys_SpikeDataPacket[i]->NumSamplesPerSpike, sizeof(short));
						tempSamples = (unsigned short *)mxCalloc(iEphys_SpikeDataPacket[i]->NumChannels * iEphys_SpikeDataPacket[i]->NumSamplesPerSpike, sizeof(unsigned short));
						iEphys_SpikeDataPacket_Data[i]->Gains = (float *)mxCalloc(iEphys_SpikeDataPacket[i]->NumChannels, sizeof(float));
						iEphys_SpikeDataPacket_Data[i]->Thresholds = (unsigned short *)mxCalloc(iEphys_SpikeDataPacket[i]->NumChannels, sizeof(unsigned short));
						if ((iEphys_SpikeDataPacket_Data[i]->Data == NULL) || (iEphys_SpikeDataPacket_Data[i]->Gains == NULL) || (iEphys_SpikeDataPacket_Data[i]->Thresholds == NULL)) {
							mexErrMsgTxt("Out of memory CODE:2-3 !");
							exit(1);
						}
						fread(tempSamples, sizeof(unsigned short), iEphys_SpikeDataPacket[i]->NumChannels * iEphys_SpikeDataPacket[i]->NumSamplesPerSpike, file);

						for (indexP = 0; indexP < iEphys_SpikeDataPacket[i]->NumChannels * iEphys_SpikeDataPacket[i]->NumSamplesPerSpike; indexP++)
						{
							iEphys_SpikeDataPacket_Data[i]->Data[indexP] = tempSamples[indexP] - 32768;
						}

						fread(iEphys_SpikeDataPacket_Data[i]->Gains, sizeof(float), iEphys_SpikeDataPacket[i]->NumChannels, file);
						fread(iEphys_SpikeDataPacket_Data[i]->Thresholds, sizeof(unsigned short), iEphys_SpikeDataPacket[i]->NumChannels, file);
						fread(voidData, sizeof(short), 1, file);  //Unknown data in Ephys spike file.

						//fread(&RecordNum_Temp, sizeof(unsigned short), 1, file);
						//iEphys_SpikeDataPacket_Data[i]->RecordNum = RecordNum_Temp;
						State_SpikeFiles[i] |= SPST_DATA;  //data ready
					}
					else
					{
						State_SpikeFiles[i] |= SPST_FEND;   //File end
						iEphys_SpikeDataPacket[i]->Timestamp = 0;
						fDataPack_Availiable = fDataPack_Availiable - 1;
					}

                   
                }else
                {
                    State_SpikeFiles[i] |= SPST_FEND;   //File end
                    iEphys_SpikeDataPacket[i]->Timestamp = 0;
                    fDataPack_Availiable = fDataPack_Availiable - 1;
                }
            }
            
        }
        
		conFinished = 1;    //initial value to check whether continu channels have finished
        for (i=0; i<NumContinuousFiles; i++) {
            if ((State_ConFiles[i] & COST_FEND)==0){
                //mexPrintf("ContinuseChannel-%d\n",i);
                if ((State_ConFiles[i] & COST_DATA) == 0) {          //No con data available in memory
                    file = iContinuousFiles[i];
                    if (!feof(file)) {

						if (i == Con_FileIndex)
						{
							if (ftell(file) >(Percent_Finished*Con_FileLength / 10)) {
								mexPrintf("%d%%...", Percent_Finished * 10);
								//mexPrintf("Position: %d of %d\n",ftell(fp),(Percent_Finished*Nev_FileLength/10));
								mexEvalString("drawnow");
								Percent_Finished++;
							}
						}

						fread(iEphys_ContinuousDataPacket[i], sizeof(Ephys_ContinuousDataPacket), 1, file); //read one packet from each file

						iEphys_ContinuousDataPacket_Data[i]->Data = (short *)mxCalloc(iEphys_ContinuousDataPacket[i]->NumSamples, sizeof(short));
						iEphys_ContinuousDataPacket_Data[i]->Marker = (char *)mxCalloc(10, sizeof(char));
						if ((iEphys_ContinuousDataPacket_Data[i]->Data == NULL) || iEphys_ContinuousDataPacket_Data[i]->Marker == NULL)
						{
							mexErrMsgTxt("ReadandWriteDataPacket() Error, Code:0001!");
							exit(1);
						}

						for (j = 0; j < iEphys_ContinuousDataPacket[i]->NumSamples; j++)
						{
							unsigned char Temp_Data[2];
							fread(&Temp_Data, sizeof(unsigned char), 2, file);

							/*
							if (iContinuousChannels[i] == 36)
							{
								int testValueb = 0;
							}*/

							iEphys_ContinuousDataPacket_Data[i]->Data[j] = be16_to_cpu(Temp_Data);
						}

						fread(iEphys_ContinuousDataPacket_Data[i]->Marker, sizeof(char), 10, file);

                        State_ConFiles[i] |= COST_DATA;                //Con data ready
                        iContinuPart[i] = 1;
                        iStartTS_Con[i] = iEphys_ContinuousDataPacket[i]->Timestamp;
                        iLastRawTS_Con[i] = iEphys_ContinuousDataPacket[i]->Timestamp;
                    }
                    else {
                        State_ConFiles[i] |= COST_FEND;      //Remove flag for Con
                        iStartTS_Con[i] = -1;
                        fDataPack_Availiable = fDataPack_Availiable - 1;
                    }
                }
				conFinished = 0;     //still have data in chotinuous files
            }
        }
        
		if (conFinished==0)
		{
			for (i=0; i<NumContinuousFiles; i++) {          //find initial values from the file which still has data
				if (State_ConFiles[i] & COST_DATA) {
					iMinStartTS_Con = i;
					vMinStartTS_Con = iStartTS_Con[i];
					break;
				}
			}
            
			for (i=0; i<NumContinuousFiles; i++) {          // find smallest value from the files which still have data
				if (State_ConFiles[i] & COST_DATA) {
					//mexPrintf("ConChannel-%d \t TimeStamp: %d\n",i,iStartTS_Con[i]);
					if (iStartTS_Con[i] < vMinStartTS_Con) {
						iMinStartTS_Con = i;
						vMinStartTS_Con = iStartTS_Con[i];
					}
				}
			}
		}
		else    //no continuous channes, so using spike channels now
		{
			for (i = 0; i<NumSpikeFiles; i++) {          //find initial values from the file which still has data
				if (State_SpikeFiles[i] & SPST_DATA) {
					iMinStartTS_Con = i;
					vMinStartTS_Con = iEphys_SpikeDataPacket[i]->Timestamp;
					break;
				}
			}

			for (i = 0; i<NumSpikeFiles; i++) {          // find smallest value from the files which still have data
				if (State_SpikeFiles[i] & SPST_DATA) {
					//mexPrintf("ConChannel-%d \t TimeStamp: %d\n",i,iStartTS_Con[i]);
					if (iEphys_SpikeDataPacket[i]->Timestamp < vMinStartTS_Con) {
						iMinStartTS_Con = i;
						vMinStartTS_Con = iEphys_SpikeDataPacket[i]->Timestamp;
					}
				}
			}
		}
       
		if (startEventWrited == false)
		{
			if (Ephys_EventDataPacket.Timestamp < vMinStartTS_Con)
			{
				WriteEventDataPacket(258, 0, plx_fp);
				StartTimeStamp = Ephys_EventDataPacket.Timestamp;
			}
			else
			{
				WriteEventDataPacket(258, 0, plx_fp);
				StartTimeStamp = vMinStartTS_Con;
			}
			startEventWrited = true;
		}

        if (State_EventFile & EVET_DATA)
        {
			long long tempValue = Ephys_EventDataPacket.Timestamp;
			if ((Ephys_EventDataPacket.Timestamp < vMinStartTS_Con) || (fDataPack_Availiable == 1))
            {
                if ((Ephys_EventDataPacket.processorID==100)&&(Ephys_EventDataPacket.EventType==3))    //TTL signal from FPGA
                {
                    if ((Ephys_EventDataPacket.EventID==1)&&(Ephys_EventDataPacket.EventChannel!=0))
                    {
						WriteEventDataPacket(Ephys_EventDataPacket.EventChannel, vMinStartTS_Con - StartTimeStamp, plx_fp);
                        State_EventFile &= ~EVET_DATA;  //Remove flag
                        //event channels
                    }
                    else
                    {
                        State_EventFile &= ~EVET_DATA;   //throw away the data
                    }
                }
                else
                {
                    State_EventFile &= ~EVET_DATA;   //throw away the data
                }
            }
        }

        for (i=0;i<NumSpikeFiles;i++)
        {
            if(State_SpikeFiles[i] & SPST_DATA){
				//mexPrintf("Spike TimeStamp: %d \t vMinStartTS_Con:%d\n", iEphys_SpikeDataPacket[i]->Timestamp, vMinStartTS_Con);

                if (iEphys_SpikeDataPacket[i]->Timestamp <= vMinStartTS_Con)
                {
                    //mexPrintf("Write Spike Channel-%d\n",i);
                    WriteSpikeDataPacket(i,plx_fp);
                    State_SpikeFiles[i] &= ~SPST_DATA;       //Remove flag
                }
            }
        }
        
        //mexPrintf("Write ConChannel-%d\n",iMinStartTS_Con);
        WriteContinueDataPack (iMinStartTS_Con, plx_fp);
        State_ConFiles[iMinStartTS_Con] &= ~COST_DATA;   
     }


	 if (!stopEventWrited)
	 {
		 if (vMinStartTS_Con<Ephys_EventDataPacket.Timestamp)
		 {
			 vMinStartTS_Con = Ephys_EventDataPacket.Timestamp;
		 }

		 WriteEventDataPacket(259, vMinStartTS_Con, plx_fp);
		 stopEventWrited = true;
	 }

    mexPrintf("\nWrited Event Data Packets  = %d\n",NumEventPacket);
    mexPrintf("Writed Spike Data Packets  = %d\n",NumSpikePacket);
    mexPrintf("Writed Continuous Data Packets  = %d\n",NumContinuPacket);
}


/************************************************************************
 *
 *
 ************************************************************************/
void makePlxFileHeader (void) {
    int i,j,k;
    int BytesPerWave;
    
    BytesPerWave = 0;
    
    mexPrintf("Making .plx file header...\n");
    
    iplxFileHeader = &plxFileHeader;
    
    plxFileHeader.MagicNumber = 0x58454c50;   // = 0x58454c50
    plxFileHeader.Version = 106;     // Version of the data format; determines which data items are valid. Now simulate Ver.106
    plxFileHeader.Comment[0] = '\0';    //There is no comments in settings.xml, so put empyt for now
    /*
    for (i=0;i<128;i++) {
        plxFileHeader.Comment[i] = cereBasicHeader.Comment[i];  //Only first 128 chars can be put in Plexon header
    }*/
    plxFileHeader.ADFrequency = Ephys_EventHead.SampleRate;  //Put SampleRate in EventHead as global SampleRate
    
    plxFileHeader.NumDSPChannels   = NumSpikeFiles;            // Number of DSP channel headers in the file
	plxFileHeader.NumEventChannels = 2 + numEventChannels;     // Number of Event channel headers in the file (Plexon have 2 more channels neamed "Start" and "Stop")
    plxFileHeader.NumSlowChannels  = NumContinuousFiles;       // Number of A/D channel headers in the file

	plxFileHeader.NumPointsWave = NumSamplesPerSpikePlex;      // Number of data points in waveform (using value in first channel as global)
    plxFileHeader.NumPointsPreThr  = 5;                                     // Number of data points before crossing the threshold, Set to 10, This value is not avaliable in ephys files 
	
    plxFileHeader.Year  = 2014;               // Time/date when the data was acquired
    plxFileHeader.Month = 7; 
    plxFileHeader.Day   = 17; 
    plxFileHeader.Hour  = 11; 
    plxFileHeader.Minute= 11; 
    plxFileHeader.Second= 11; 
	
    plxFileHeader.FastRead = 0;           // reserved
    plxFileHeader.WaveformFreq = iEphys_SpikeHead[0]->SampleRate;     // waveform sampling rate; Using value in first channel as global, ADFrequency above is timestamp freq 
   
    
    plxFileHeader.LastTimestamp = 0;      // duration of the experimental session, in ticks  ################# need more code ###################
    
    // The following 6 items are only valid if Version >= 103
    plxFileHeader.Trodalness     = 1;                 // 1 for single, 2 for stereotrode, 4 for tetrode; using value in first channel as global
    plxFileHeader.DataTrodalness = 1;                 // trodalness of the data representation
   
    plxFileHeader.BitsPerSpikeSample = 16;         // ADC resolution for spike waveforms in bits (usually 16)
    plxFileHeader.BitsPerSlowSample  = 16;         // ADC resolution for slow-channel data in bits (usually 16)
   
    plxFileHeader.SpikeMaxMagnitudeMV = 6390;  // the zero-to-peak voltage in mV for spike waveform adc values (usually 3000)
    plxFileHeader.SlowMaxMagnitudeMV  = 6390;  // the zero-to-peak voltage in mV for slow-channel waveform adc values (Set to 6390mV)   -- Slow channel in Ephys is 0.195mv/bit
    
    // Only valid if Version >= 105
    plxFileHeader.SpikePreAmpGain = 1;         // usually either 1000 or 500
	
    for (i=0; i<46; i++) {
        plxFileHeader.Padding[i]=0;            // Write as zero.
    }

	plxFileHeader.Padding[3] = 1;            // state bit: ephys system data

    for (i=0; i<130; i++) {
        for (j=0; j<5; j++) {
            tscounts[i][j]=0;
            wfcounts[i][j]=0;
        }
    }
    
    for (i=0; i<512; i++) {
        evcounts[i] = 0;
    }
}


/************************************************************************
 *
 *
 ************************************************************************/
void makePlxChanHeader (void) {
    int i,j,k,l;

    if (NumUsingDSP > 0) {
    
        iplxChanHeader = mxCalloc(NumUsingDSP,sizeof(struct PL_ChanHeader));

        if (iplxChanHeader == NULL) {
            mexErrMsgTxt("Out of memory CODE:7-1 !");
            exit(1);
        }

        for (i=0; i<NumUsingDSP; i++) {
            
            //mexPrintf("NumUsingDSP=%d\n",NumUsingDSP);
            
			strncpy(iplxChanHeader[i].Name, iEphys_SpikeHead[i]->Electrode, 31);
			strcpy(iplxChanHeader[i].SIGName, "N/A");     //Use "ElectrodeID" in Ephys_SpikeDataPacket to find Channel Name in Ephys_ContinuousHead?

			iplxChanHeader[i].Channel = iEphys_SpikeDataPacket[i]->ElectrodeID + 1;  //in Plexon is 1-based
			iplxChanHeader[i].WFRate = 3000;
			iplxChanHeader[i].SIG = iEphys_SpikeDataPacket[i]->ElectrodeID + 1;     //in Plexon is 1-based
			iplxChanHeader[i].Ref = iEphys_SpikeDataPacket[i]->ElectrodeID + 1;     //in Plexon is 1-based
			iplxChanHeader[i].Gain = (int)((*iEphys_SpikeDataPacket_Data[i]->Gains)/1000);
			iplxChanHeader[i].Filter = 1;
			iplxChanHeader[i].Threshold = (iEphys_SpikeDataPacket_Data[i]->Thresholds)[0];  //using the value in first datapack
			iplxChanHeader[i].Method = 1;
			iplxChanHeader[i].NUnits = 0;   //No this infor?

			for (j = 0; j<5; j++) {
				for (k = 0; k<64; k++) {
					iplxChanHeader[i].Template[j][k] = 0;
				}
			}

			for (j = 0; j<5; j++) {
				iplxChanHeader[i].Fit[j] = 0;
			}

			iplxChanHeader[i].SortWidth = 32;

			for (j = 0; j<5; j++) {
				for (k = 0; k<2; k++) {
					for (l = 0; l<4; l++) {
						iplxChanHeader[i].Boxes[j][k][l] = 0;
					}
				}
			}

			iplxChanHeader[i].SortBeg = 0;

			for (j = 0; j<11; j++) {
				iplxChanHeader[i].Padding[j] = 0;
			}
        }
        
		//debug info
        //plxChanHeader = iplxChanHeader[i];
        //mexPrintf("Name = %s\n",plxChanHeader.Name);
        //mexPrintf("SIGName = %s\n",plxChanHeader.SIGName);
        //mexPrintf("Channel = %d\n",plxChanHeader.Channel);
        //mexPrintf("Gain = %d\n",plxChanHeader.Gain);
    }
} 


/************************************************************************
 *
 *
 ************************************************************************/
void makePlxEventHeader (void) {
    int i,j;
    char ChannelName[11];
    char ChannelNamePre[8]="EventCh";
    char ChannelNum[4];
    
    iplxEventHeader = mxCalloc(2+numEventChannels,sizeof(struct PL_EventHeader));    //Plexon have 2 more channels neamed "Start" and "Stop"
    
    if (iplxEventHeader == NULL) {
        mexErrMsgTxt("Out of memory CODE:8-1 !");
        exit(1);
    }
       
    strcpy(iplxEventHeader[0].Name, "Start");
    iplxEventHeader[0].Channel = 258;
    strcpy(iplxEventHeader[0].Comment,"");
    for (i=0; i<33; i++) {
        iplxEventHeader[0].Padding[i] = 0;
    }
    
    strcpy(iplxEventHeader[1].Name, "Stop");
    iplxEventHeader[1].Channel = 259;
    strcpy(iplxEventHeader[1].Comment,"");
    for (i=0; i<33; i++) {
        iplxEventHeader[1].Padding[i] = 0;
    }
    
    for(i=0; i<numEventChannels; i++)
    {
        sprintf(ChannelNum,"%03d",eventChannels[i]);
        sprintf(ChannelName,"%s%s",ChannelNamePre,ChannelNum);
        strcpy(iplxEventHeader[2+i].Name,ChannelName);
        iplxEventHeader[2+i].Channel=eventChannels[i];
        strcpy(iplxEventHeader[2+i].Comment,"");
        for (j=0; j<33; j++) {
            iplxEventHeader[2+i].Padding[j] = 0;
        }
    }
    
    /*
    strcpy(iplxEventHeader[2].Name,"evt01");
    iplxEventHeader[2].Channel = 1;
    strcpy(iplxEventHeader[2].Comment,"");
    for (i=0; i<33; i++) {
        iplxEventHeader[2].Padding[i] = 0;
    }
    
    strcpy(iplxEventHeader[3].Name,"Strobed");
    iplxEventHeader[3].Channel = 257;
    strcpy(iplxEventHeader[3].Comment,"");
    for (i=0; i<33; i++) {
        iplxEventHeader[3].Padding[i] = 0;
    }*/
}


/************************************************************************
 *
 *
 ************************************************************************/
void makePlxContinuHeader (void) {
    int i;
    
    iplxSlowHeader = mxCalloc(NumContinuousFiles,sizeof(struct PL_SlowChannelHeader));
    
    if (iplxSlowHeader == NULL) {
        mexErrMsgTxt("Out of memory CODE:9-1 !");
        exit(1);
    }

    for (i=0; i<NumContinuousFiles; i++) {

        strncpy(iplxSlowHeader[i].Name,iEphys_ContinuousHead[i]->Channel,31);
        //strcpy(iplxSlowHeader[cContinuChanNow].Name, "12345678");
        iplxSlowHeader[i].Channel = iContinuousChannels[i] - 1;             //channel number, !!!0-based!!!

        iplxSlowHeader[i].ADFreq = iEphys_ContinuousHead[i]->SampleRate;

        iplxSlowHeader[i].Gain =1;         //in Ephys, 0.195mv/bit
        iplxSlowHeader[i].Enabled = 1;            //Always 1
        iplxSlowHeader[i].PreAmpGain = 1;         //in Ephys, 0.195mv/bit

		iplxSlowHeader[i].SpikeChannel = iContinuousChannels[i];       

        strncpy(iplxSlowHeader[i].Comment, iEphys_ContinuousHead[i]->Desciption,127);
    }
    
    //mexPrintf("Channel:%d\t ADFreq:%d\n",iplxSlowHeader[5].Channel,iplxSlowHeader[1].ADFreq);
}


/************************************************************************
 *
 *
 ************************************************************************/

void writePLXFileHeader (FILE *fp) {
    
    mexPrintf("Writing .plx file header...\n");
    
    fwrite(iplxFileHeader,sizeof(struct PL_FileHeader),1,fp);
    //mexPrintf("Length of FileHeader = %d\n",sizeof(struct PL_FileHeader));
    fwrite(&tscounts,sizeof(tscounts),1,fp);
    //mexPrintf("Length of tscounts = %d\n",sizeof(tscounts));
    fwrite(&wfcounts,sizeof(wfcounts),1,fp);
    //mexPrintf("Length of wfcounts = %d\n",sizeof(wfcounts));
    fwrite(&evcounts,sizeof(evcounts),1,fp);
    //mexPrintf("Length of evcounts = %d\n",sizeof(evcounts));
    
}


/************************************************************************
 *
 *
 ************************************************************************/

void writePLXChanHeader (FILE *fp) {
        
    mexPrintf("Writing .plx Channel header...\n");
    
    fwrite(iplxChanHeader,sizeof(struct PL_ChanHeader),NumUsingDSP,fp);
    
    //mexPrintf("Number of ChanHeader = %d\n",NumUsingDSP);
    
}


/************************************************************************
 *
 *
 ************************************************************************/
void writePLXEventHeader (FILE *fp) {
    
    mexPrintf("Writing .plx Event header...\n");
	fwrite(iplxEventHeader, sizeof(struct PL_EventHeader), 2 + numEventChannels, fp);
}


/************************************************************************
 *
 *
 ************************************************************************/
void writePLXContinuHeader (FILE *fp) {
    mexPrintf("Writing .plx Continuous header...\n");
    fwrite(iplxSlowHeader,sizeof(struct PL_SlowChannelHeader),NumContinuousFiles,fp);
}


/************************************************************************
 *
 *
 ************************************************************************/
void getString_mxArray(mxArray *ele_mxArray, char *string, int stringLength)
{
    int mxArrayLength;
    
    if (mxGetClassID(ele_mxArray) == mxCHAR_CLASS)
    {
        mxArrayLength = mxGetM(ele_mxArray)*mxGetN(ele_mxArray)+1;
        if (mxArrayLength <= stringLength)
        {
            mxGetString(ele_mxArray,string,mxArrayLength);
        }
        else
        {
            mexErrMsgTxt("Input string is shorter than need!");
        }
    }else
    {
        mexErrMsgTxt("Not a String!");
    }
    
}

/************************************************************************
 *
 *
 ************************************************************************/
void getContinousHead(struct Ephys_ContinuousHead *Ephys_ContinuousHead, mxArray *cellElement)
{
    int nfields;
    mwSize          NStructElems;
    mxArray         *tmp;
    char            *string;
    
    nfields = mxGetNumberOfFields(cellElement);
    NStructElems = mxGetNumberOfElements(cellElement);

    //mexPrintf("nfields = %d \t NStructElems = %d\n",nfields,NStructElems);

    //Ephys Format
    tmp=mxGetFieldByNumber(cellElement, 0, 0);
    if (mxGetClassID(tmp) == mxCHAR_CLASS)
    {
        string=mxCalloc(25,sizeof(char));
        getString_mxArray(tmp,string,25);
        strcpy(Ephys_ContinuousHead->Format, string);
        mxFree(string);
    }else
    {
        mexErrMsgTxt("ContinuousHead:Format Datatype Error!");
    }
    
    //Ephys Version
    tmp=mxGetFieldByNumber(cellElement, 0, 1);
    if (mxGetClassID(tmp) == mxDOUBLE_CLASS)
    {
        Ephys_ContinuousHead->Version=(float)*mxGetPr(tmp);
    }else
    {
        mexErrMsgTxt("ContinuousHead:Version Datatype Error!");
    }
    
    //Ephys HeaderBytes
    tmp=mxGetFieldByNumber(cellElement, 0, 2);
    if (mxGetClassID(tmp) == mxDOUBLE_CLASS)
    {
        Ephys_ContinuousHead->Header_bytes=(int)*mxGetPr(tmp);
    }else
    {
        mexErrMsgTxt("ContinuousHead:Header_bytes Datatype Error!");
    }
    
    //Ephys Desciption
    tmp=mxGetFieldByNumber(cellElement, 0, 3);
    if (mxGetClassID(tmp) == mxCHAR_CLASS)
    {
        string=mxCalloc(250,sizeof(char));
        getString_mxArray(tmp,string,250);
        strcpy(Ephys_ContinuousHead->Desciption, string);
        mxFree(string);
    }else
    {
        mexErrMsgTxt("ContinuousHead:Desciption Datatype Error!");
    }
    
    //Ephys Date_created
    tmp=mxGetFieldByNumber(cellElement, 0, 4);
    if (mxGetClassID(tmp) == mxCHAR_CLASS)
    {
        string=mxCalloc(20,sizeof(char));
        getString_mxArray(tmp,string,20);
        strcpy(Ephys_ContinuousHead->Date_created, string);
        mxFree(string);
    }else
    {
        mexErrMsgTxt("ContinuousHead:Date_created Datatype Error!");
    }
    
    //Ephys ChannelName
    tmp=mxGetFieldByNumber(cellElement, 0, 5);
    if (mxGetClassID(tmp) == mxCHAR_CLASS)
    {
        string=mxCalloc(10,sizeof(char));
        getString_mxArray(tmp,string,10);
        strcpy(Ephys_ContinuousHead->Channel, string);
        mxFree(string);
    }else
    {
        mexErrMsgTxt("ContinuousHead:ChannelName Datatype Error!");
    }
    
    //Ephys ChannelType
    tmp=mxGetFieldByNumber(cellElement, 0, 6);
    if (mxGetClassID(tmp) == mxCHAR_CLASS)
    {
        string=mxCalloc(15,sizeof(char));
        getString_mxArray(tmp,string,15);
        strcpy(Ephys_ContinuousHead->ChannelType, string);
        mxFree(string);
    }else
    {
        mexErrMsgTxt("ContinuousHead:ChannelType Datatype Error!");
    }
    
    //Ephys SampleRate
    tmp=mxGetFieldByNumber(cellElement, 0, 7);
    if (mxGetClassID(tmp) == mxDOUBLE_CLASS)
    {
        Ephys_ContinuousHead->SampleRate=(int)*mxGetPr(tmp);
    }else
    {
        mexErrMsgTxt("ContinuousHead:SampleRate Datatype Error!");
    }
    
    //Ephys BlockLength
    tmp=mxGetFieldByNumber(cellElement, 0, 8);
    if (mxGetClassID(tmp) == mxDOUBLE_CLASS)
    {
        Ephys_ContinuousHead->BlockLength=(int)*mxGetPr(tmp);
    }else
    {
        mexErrMsgTxt("ContinuousHead:BlockLength Datatype Error!");
    }
    
    //Ephys BufferSize
    tmp=mxGetFieldByNumber(cellElement, 0, 9);
    if (mxGetClassID(tmp) == mxDOUBLE_CLASS)
    {
        Ephys_ContinuousHead->BufferSize=(int)*mxGetPr(tmp);
    }else
    {
        mexErrMsgTxt("ContinuousHead:BufferSize Datatype Error!");
    }
    
    //Ephys BitVolts
    tmp=mxGetFieldByNumber(cellElement, 0, 10);
    if (mxGetClassID(tmp) == mxDOUBLE_CLASS)
    {
        Ephys_ContinuousHead->BitVolts=(float)*mxGetPr(tmp);
    }else
    {
        mexErrMsgTxt("ContinuousHead:BitVolts Datatype Error!");
    }
    
    /*
    mexPrintf("Format = %s\n",Ephys_ContinuousHead->Format);
    mexPrintf("Version = %f\n",Ephys_ContinuousHead->Version);
    mexPrintf("Version = %d\n",Ephys_ContinuousHead->Header_bytes);
    mexPrintf("Desciption = %s\n",Ephys_ContinuousHead->Desciption);
    mexPrintf("Date_created = %s\n",Ephys_ContinuousHead->Date_created);
    mexPrintf("Channel = %s\n",Ephys_ContinuousHead->Channel);
    mexPrintf("ChannelType = %s\n",Ephys_ContinuousHead->ChannelType);
    mexPrintf("SampleRate = %d\n",Ephys_ContinuousHead->SampleRate);
    mexPrintf("BlockLength = %d\n",Ephys_ContinuousHead->BlockLength);
    mexPrintf("BufferSize = %d\n",Ephys_ContinuousHead->BufferSize);
    mexPrintf("BufferSize = %f\n",Ephys_ContinuousHead->BitVolts);*/
}

/************************************************************************
 *
 *
 ************************************************************************/
void getSpikeHead(struct Ephys_SpikeHead *Ephys_SpikeHead, mxArray *cellElement)
{
    int nfields;
    mwSize          NStructElems;
    mxArray         *tmp;
    char            *string;
    
    nfields = mxGetNumberOfFields(cellElement);
    NStructElems = mxGetNumberOfElements(cellElement);

    //mexPrintf("nfields = %d \t NStructElems = %d\n",nfields,NStructElems);

    //Ephys Format
    tmp=mxGetFieldByNumber(cellElement, 0, 0);
    if (mxGetClassID(tmp) == mxCHAR_CLASS)
    {
        string=mxCalloc(25,sizeof(char));
        getString_mxArray(tmp,string,25);
        strcpy(Ephys_SpikeHead->Format, string);
        mxFree(string);
    }else
    {
        mexErrMsgTxt("Ephys_SpikeHead:Format Datatype Error!");
    }
    
    //Ephys Version
    tmp=mxGetFieldByNumber(cellElement, 0, 1);
    if (mxGetClassID(tmp) == mxDOUBLE_CLASS)
    {
        Ephys_SpikeHead->Version=(float)*mxGetPr(tmp);
    }else
    {
        mexErrMsgTxt("Ephys_SpikeHead:Version Datatype Error!");
    }
    
    //Ephys HeaderBytes
    tmp=mxGetFieldByNumber(cellElement, 0, 2);
    if (mxGetClassID(tmp) == mxDOUBLE_CLASS)
    {
        Ephys_SpikeHead->Header_bytes=(int)*mxGetPr(tmp);
    }else
    {
        mexErrMsgTxt("Ephys_SpikeHead:Header_bytes Datatype Error!");
    }
    
    //Ephys Desciption
    tmp=mxGetFieldByNumber(cellElement, 0, 3);
    if (mxGetClassID(tmp) == mxCHAR_CLASS)
    {
        string=mxCalloc(366,sizeof(char));
        getString_mxArray(tmp,string,366);
        strcpy(Ephys_SpikeHead->Desciption, string);
        mxFree(string);
    }else
    {
        mexErrMsgTxt("Ephys_SpikeHead:Desciption Datatype Error!");
    }
    
    //Ephys Date_created
    tmp=mxGetFieldByNumber(cellElement, 0, 4);
    if (mxGetClassID(tmp) == mxCHAR_CLASS)
    {
        string=mxCalloc(20,sizeof(char));
        getString_mxArray(tmp,string,20);
        strcpy(Ephys_SpikeHead->Date_created, string);
        mxFree(string);
    }else
    {
        mexErrMsgTxt("Ephys_SpikeHead:Desciption Datatype Error!");
    }
    
    //Ephys ElectrodeName
    tmp=mxGetFieldByNumber(cellElement, 0, 5);
    if (mxGetClassID(tmp) == mxCHAR_CLASS)
    {
        string=mxCalloc(25,sizeof(char));
        getString_mxArray(tmp,string,25);
        strcpy(Ephys_SpikeHead->Electrode, string);
        mxFree(string);
    }else
    {
        mexErrMsgTxt("Ephys_SpikeHead:electrode Datatype Error!");
    }
    
    //Ephys num_channels
    tmp=mxGetFieldByNumber(cellElement, 0, 6);
    if (mxGetClassID(tmp) == mxDOUBLE_CLASS)
    {
        Ephys_SpikeHead->Num_channels=(int)*mxGetPr(tmp);
    }else
    {
        mexErrMsgTxt("ContinuousHead:Header_bytes num_channels Error!");
    }
    
    //Ephys SampleRate
    tmp=mxGetFieldByNumber(cellElement, 0, 7);
    if (mxGetClassID(tmp) == mxDOUBLE_CLASS)
    {
        Ephys_SpikeHead->SampleRate=(int)*mxGetPr(tmp);
    }else
    {
        mexErrMsgTxt("Ephys_SpikeHead:Header_bytes Datatype Error!");
    }
    
    /*
    mexPrintf("Format = %s\n",Ephys_SpikeHead->Format);
    mexPrintf("Version = %f\n",Ephys_SpikeHead->Version);
    mexPrintf("Header_bytes = %d\n",Ephys_SpikeHead->Header_bytes);
    mexPrintf("Desciption = %s\n",Ephys_SpikeHead->Desciption);
    mexPrintf("Date_created = %s\n",Ephys_SpikeHead->Date_created);
    mexPrintf("Electrode = %s\n",Ephys_SpikeHead->Electrode);
    mexPrintf("Num_channels = %d\n",Ephys_SpikeHead->Num_channels);
    mexPrintf("SampleRate = %d\n",Ephys_SpikeHead->SampleRate); */
}

/************************************************************************
 *
 *
 ************************************************************************/
void getEventHead(struct Ephys_EventHead *Ephys_EventHead, mxArray *cellElement)
{
    int nfields;
    mwSize          NStructElems;
    mxArray         *tmp;
    char            *string;
    
    nfields = mxGetNumberOfFields(cellElement);
    NStructElems = mxGetNumberOfElements(cellElement);

    //mexPrintf("nfields = %d \t NStructElems = %d\n",nfields,NStructElems);

    //Ephys Format
    tmp=mxGetFieldByNumber(cellElement, 0, 0);
    if (mxGetClassID(tmp) == mxCHAR_CLASS)
    {
        string=mxCalloc(25,sizeof(char));
        getString_mxArray(tmp,string,25);
        strcpy(Ephys_EventHead->Format, string);
        mxFree(string);
    }else
    {
        mexErrMsgTxt("Ephys_EventHead:Format Datatype Error!");
    }
    
    //Ephys Version
    tmp=mxGetFieldByNumber(cellElement, 0, 1);
    if (mxGetClassID(tmp) == mxDOUBLE_CLASS)
    {
        Ephys_EventHead->Version=(float)*mxGetPr(tmp);
    }else
    {
        mexErrMsgTxt("Ephys_EventHead:Version Datatype Error!");
    }
    
    //Ephys HeaderBytes
    tmp=mxGetFieldByNumber(cellElement, 0, 2);
    if (mxGetClassID(tmp) == mxDOUBLE_CLASS)
    {
        Ephys_EventHead->Header_bytes=(int)*mxGetPr(tmp);
    }else
    {
        mexErrMsgTxt("Ephys_EventHead:Header_bytes Datatype Error!");
    }
    
    //Ephys Desciption
    tmp=mxGetFieldByNumber(cellElement, 0, 3);
    if (mxGetClassID(tmp) == mxCHAR_CLASS)
    {
        string=mxCalloc(250,sizeof(char));
        getString_mxArray(tmp,string,250);
        strcpy(Ephys_EventHead->Desciption, string);
        mxFree(string);
    }else
    {
        mexErrMsgTxt("Ephys_EventHead:Desciption Datatype Error!");
    }
    
    //Ephys Date_created
    tmp=mxGetFieldByNumber(cellElement, 0, 4);
    if (mxGetClassID(tmp) == mxCHAR_CLASS)
    {
        string=mxCalloc(20,sizeof(char));
        getString_mxArray(tmp,string,20);
        strcpy(Ephys_EventHead->Date_created, string);
        mxFree(string);
    }else
    {
        mexErrMsgTxt("Ephys_EventHead:Date_created Datatype Error!");
    }
    
    //Ephys ChannelName
    tmp=mxGetFieldByNumber(cellElement, 0, 5);
    if (mxGetClassID(tmp) == mxCHAR_CLASS)
    {
        string=mxCalloc(10,sizeof(char));
        getString_mxArray(tmp,string,10);
        strcpy(Ephys_EventHead->Channel, string);
        mxFree(string);
    }else
    {
        mexErrMsgTxt("Ephys_EventHead:Channel Datatype Error!");
    }
    
    //Ephys ChannelType
    tmp=mxGetFieldByNumber(cellElement, 0, 6);
    if (mxGetClassID(tmp) == mxCHAR_CLASS)
    {
        string=mxCalloc(15,sizeof(char));
        getString_mxArray(tmp,string,15);
        strcpy(Ephys_EventHead->ChannelType, string);
        mxFree(string);
    }else
    {
        mexErrMsgTxt("Ephys_EventHead:ChannelType Datatype Error!");
    }
    
    //Ephys SampleRate
    tmp=mxGetFieldByNumber(cellElement, 0, 7);
    if (mxGetClassID(tmp) == mxDOUBLE_CLASS)
    {
        Ephys_EventHead->SampleRate=(int)*mxGetPr(tmp);
    }else
    {
        mexErrMsgTxt("Ephys_EventHead:SampleRate Datatype Error!");
    }
    
    //Ephys BlockLength
    tmp=mxGetFieldByNumber(cellElement, 0, 8);
    if (mxGetClassID(tmp) == mxDOUBLE_CLASS)
    {
        Ephys_EventHead->BlockLength=(int)*mxGetPr(tmp);
    }else
    {
        mexErrMsgTxt("Ephys_EventHead:BlockLength Datatype Error!");
    }
    
    //Ephys BufferSize
    tmp=mxGetFieldByNumber(cellElement, 0, 9);
    if (mxGetClassID(tmp) == mxDOUBLE_CLASS)
    {
        Ephys_EventHead->BufferSize=(int)*mxGetPr(tmp);
    }else
    {
        mexErrMsgTxt("Ephys_EventHead:BufferSize Datatype Error!");
    }
    
    //Ephys BitVolts
    tmp=mxGetFieldByNumber(cellElement, 0, 10);
    if (mxGetClassID(tmp) == mxDOUBLE_CLASS)
    {
        Ephys_EventHead->BitVolts=(float)*mxGetPr(tmp);
    }else
    {
        mexErrMsgTxt("Ephys_EventHead:BitVolts Datatype Error!");
    }
    
    /*
    mexPrintf("Format = %s\n",Ephys_EventHead->Format);
    mexPrintf("Version = %f\n",Ephys_EventHead->Version);
    mexPrintf("Header_bytes = %d\n",Ephys_EventHead->Header_bytes);
    mexPrintf("Desciption = %s\n",Ephys_EventHead->Desciption);
    mexPrintf("Date_created = %s\n",Ephys_EventHead->Date_created);
    mexPrintf("Channel = %s\n",Ephys_EventHead->Channel);
    mexPrintf("ChannelType = %s\n",Ephys_EventHead->ChannelType);
    mexPrintf("SampleRate = %d\n",Ephys_EventHead->SampleRate);
    mexPrintf("BlockLength = %d\n",Ephys_EventHead->BlockLength);
    mexPrintf("BufferSize = %d\n",Ephys_EventHead->BufferSize);
    mexPrintf("BitVolts = %f\n",Ephys_EventHead->BitVolts);*/
}

/************************************************************************
 *
 *
 ************************************************************************/
FILE* OpenFile(mxArray *cellElement)
{
    char *fileNamebuf;                                                  /*Pointer to FileName*/
    int fileNameLength;
    FILE *iFile;
    
    if (mxGetClassID(cellElement) == mxCHAR_CLASS)
    {
        fileNameLength = mxGetM(cellElement)*mxGetN(cellElement)+1;
        fileNamebuf=mxCalloc(fileNameLength,sizeof(char));
        getString_mxArray(cellElement,fileNamebuf,fileNameLength);
        if((iFile=fopen(fileNamebuf,"rb"))==NULL)
        {
            mexPrintf("File Name: %s\n",fileNamebuf);
            mexErrMsgTxt("Cannot be opened!");
            exit(1);
        }
        //mexPrintf("File Name: %s \t Pointer: %d \n",fileNamebuf,iFile->_file);
        EventTemp = iFile;
        mxFree(fileNamebuf);
        return iFile;
    }else
    {
        mexErrMsgTxt("FileName Datatype Error!");
    }
}

/************************************************************************
 *
 *
 ************************************************************************/
int GetFilePointers(mxArray *cellElement,FILE *iFiles[], int NumFiles)
{
    mxArray *cellFileName;
    int i;
    int numFileNames;
	mwSize numDims;
	mwIndex  *subs;
	mwSize x;
	mxArray *TempArray;
	mwIndex index;
	mxClassID classID;
     /*
    for(i=0; i<NumFiles; i++)
    {
        iFiles[i] = (FILE *) mxCalloc(1, sizeof(FILE));
        if (iFiles[i]==NULL) {
            mexErrMsgTxt("Out of memory CODE:1-3-1 !");
            exit(1);
       }
    }*/
    
    //mexPrintf("ClassID: %d \t NumFiles: %d",mxGetClassID(cellElement),NumFiles);
    
    if ((mxGetClassID(cellElement) == mxCHAR_CLASS) && (NumFiles == 1))
    {
        iFiles[0]=OpenFile(cellElement);
    }else if ((mxGetClassID(cellElement) == mxCELL_CLASS) && (NumFiles > 1))
    {
	   numDims = mxGetNumberOfDimensions(cellElement);
       numFileNames = *mxGetDimensions(cellElement); 
       if (numFileNames == NumFiles)
       {
           for(i=0; i<NumFiles; i++)
           {
               cellFileName = mxGetCell(cellElement,i);
               iFiles[i]=OpenFile(cellFileName);
			   if (numDims==2)
			   {
				   /* Allocate memory for the subs array on the fly */
				   subs = mxCalloc(numDims, sizeof(mwIndex));
				   subs[0] = i;
				   subs[1] = 1;
				   index = mxCalcSingleSubscript(cellElement, numDims, subs);
				   TempArray = mxGetCell(cellElement, index);
				   classID = mxGetClassID(TempArray);
				   if (classID == mxDOUBLE_CLASS)
				   {
					   chanHeaderOrder[i] = *mxGetPr(TempArray) - 1;
				   }
				   else
				   {
					   mexErrMsgTxt("Data type Error!");
				   }
			   }
           }
       }else
       {
           mexErrMsgTxt("Number of Files and FileNames are mismatch!");
       }
    }else
    {
        mexErrMsgTxt("FileName Datatype Error!");
    }
}

void closeEphysFiles()
{
    int i;
    FILE *file;
    
    file=iEventFile[0];
    fclose(file);
    
    for (i=0;i<NumSpikeFiles;i++)
    {
        file=iSpikeFiles[i];
        fclose(file);
    }
    
    for (i=0;i<NumContinuousFiles;i++)
    {
        file=iContinuousFiles[i];
        fclose(file);
    }
    
    
}

/************************************************************************
 *
 *
 ************************************************************************/
void mexFunction ( 
				  int nlhs,
				  mxArray *plhs[],
				  int nrhs,
				  const mxArray *prhs[])
{
    const char      **fnames;        /*pointers to field names*/
    mwSize          dims;
    const mxArray   *array_ptr, *tmp, *fout, *cell, *cellTemp;
    mxArray         *valueOut[30], *chvalueOut[17], *ehvalueOut[3], *shvalueOut[8] , *cellElement;                           /*temp array*/

    char            *pdata=NULL;
    mwIndex         jstruct, jcell;
    
    mwSize          ndim;
    
    double *p;
    
    char *file_name_plx;             /*pointer to File Name*/
    int m_file_name_plx, n_file_name_plx;           /*Number of Rows and Lines*/
    int file_name_len_plx;   /*The length of Filename*/
    int status;
    FILE *fp_ephys, *fp_plx;
    FILE *file;
    int chanNumNow;
   
    /*structure*/
    int fieldnum;                                                       /*Field Number in Structure*/

    char *buf[50];                                                      /*Pointer to FieldName, Max length is 50*/
    
    double *itscounts, *iwfcounts, *ievcounts, *icontinus, *itemplate, *ifit, *iboxes;
    
    int i;
    
    const mwSize boxesdims[]={2,4,5};
    
    SDIBufferSize = 0;
    
    /*check for proper number of input and output arguments*/
    if (nrhs!=3)
        mexErrMsgTxt("Three input required.");
    else if (nlhs > 0)
        mexErrMsgTxt("Too many output arguments.");
    
    /*Check for proper input type */
    if ((!mxIsChar(prhs[2])) || (mxGetM(prhs[2]) != 1 )) {
        mexErrMsgTxt(".plx FileName must be a string.");
    }
    
    m_file_name_plx=mxGetM(prhs[2]);
    n_file_name_plx=mxGetN(prhs[2]);
    
    if (m_file_name_plx !=1)
        mexErrMsgTxt(".plx FileName be a row vector.");
    
   #ifdef __x86_64__

        mexPrintf("\n64-bit Matlab\n");

   #elif __i386__

        mexPrintf("\n32-bit Matlab\n");

   #endif
   
   if (mxGetClassID(prhs[0]) == mxCELL_CLASS)
   {
       cell = prhs[0];
   }else{
       mexErrMsgTxt("First input argument must be CELL!");
   }
   
   if (*mxGetDimensions(cell) != 3)
   {
        mexErrMsgTxt("FileHeaders must include Spike, Continuous and Event!");
   }
   
   //Get Event Header
   cellElement = mxGetCell(cell,0);
   getEventHead(&Ephys_EventHead, cellElement);
   
   //Get Spike Headers
   cellTemp = mxGetCell(cell,1);
   if (mxGetClassID(cellTemp) != mxCELL_CLASS)
   {
       mexErrMsgTxt("Spike Headers must be in CELL!");
   }else{
       NumSpikeFiles = *mxGetDimensions(cellTemp);
       NumUsingDSP = NumSpikeFiles;
       //mexPrintf("NumSpikeFiles SpikeFiles: %d\n",NumSpikeFiles);
       iEphys_SpikeHead = (struct Ephys_SpikeHead **) mxCalloc(NumSpikeFiles,sizeof(struct Ephys_SpikeHead *));
        if (iEphys_SpikeHead==NULL) {
            mexErrMsgTxt("Out of memory CODE:1-1 !");
            exit(1);
        }
       for (i=0; i<NumSpikeFiles; i++)
       {
           iEphys_SpikeHead[i] = (struct Ephys_SpikeHead *) mxCalloc(1,sizeof(struct Ephys_SpikeHead));
           if (iEphys_SpikeHead[i]==NULL) {
                mexErrMsgTxt("Out of memory CODE:1-1-1 !");
                exit(1);
           }
           cellElement = mxGetCell(cellTemp,i);
           getSpikeHead(iEphys_SpikeHead[i],cellElement);
       }
   }
   
   //Get Continuous Headers
   cellTemp = mxGetCell(cell,2);
   if (mxGetClassID(cellTemp) != mxCELL_CLASS)
   {
       mexErrMsgTxt("Continuous Headers must be in CELL!");
   }else{
       NumContinuousFiles = *mxGetDimensions(cellTemp);
       iEphys_ContinuousHead = (struct Ephys_ContinuousHead **) mxCalloc(NumContinuousFiles,sizeof(struct Ephys_ContinuousHead *));
        if (iEphys_ContinuousHead==NULL) {
            mexErrMsgTxt("Out of memory CODE:1-2 !");
            exit(1);
        }
       for (i=0; i<NumContinuousFiles; i++)
       {
           iEphys_ContinuousHead[i] = (struct Ephys_ContinuousHead *) mxCalloc(1,sizeof(struct Ephys_ContinuousHead));
           if (iEphys_ContinuousHead[i]==NULL) {
                mexErrMsgTxt("Out of memory CODE:1-2-1 !");
                exit(1);
           }
           cellElement = mxGetCell(cellTemp,i);
           getContinousHead(iEphys_ContinuousHead[i],cellElement);
       }
   }
   
   //Open SpikeFiles and ContinuousFiles
   if (mxGetClassID(prhs[1]) == mxCELL_CLASS)
   {
       cell = prhs[1];
   }else{
       mexErrMsgTxt("Second input argument must be CELL!");
   }
   
   if (*mxGetDimensions(cell) != 3)
   {
        mexErrMsgTxt("FileNames must include Spike, Continuous and Event!");
   }
   
   //Open Event File
   cellElement = mxGetCell(cell,0);
   //iEventFile = (FILE *) mxCalloc(1,sizeof(FILE *));
   if ((iEventFile==NULL)) {
        mexErrMsgTxt("Out of memory CODE:1-3 !");
        exit(1);
    }
   GetFilePointers(cellElement, iEventFile, 1);


   //Open Spike Files
   cellElement = mxGetCell(cell,1);
   //iSpikeFiles = (FILE **) mxCalloc(NumSpikeFiles,sizeof(FILE *));
   if ((iSpikeFiles==NULL)) {
        mexErrMsgTxt("Out of memory CODE:1-4 !");
        exit(1);
    }
   GetFilePointers(cellElement, iSpikeFiles, NumSpikeFiles);
   
   //Open Continuous Files
   cellElement = mxGetCell(cell,2);
   //iContinuousFiles = (FILE **) mxCalloc(NumContinuousFiles,sizeof(FILE *));
   if ((iContinuousFiles==NULL)) {
        mexErrMsgTxt("Out of memory CODE:1-4 !");
        exit(1);
    }
   GetFilePointers(cellElement, iContinuousFiles, NumContinuousFiles);
   iContinuousChannels = mxCalloc(NumContinuousFiles,sizeof(int));
   for (i=0;i<NumContinuousFiles;i++)
   {
        cellTemp = mxGetCell(cellElement,NumContinuousFiles+i);
        if (mxGetClassID(cellTemp) == mxDOUBLE_CLASS)
        {
            iContinuousChannels[i]=(int)*mxGetPr(cellTemp);
            mexPrintf("Channel Number: %d\n",iContinuousChannels[i]);
        }else
        {
            mexErrMsgTxt("Ephys_EventHead:Version Datatype Error!");
        }
   }
   
   //Open PLX File to save data  
    file_name_len_plx=m_file_name_plx * n_file_name_plx + 1;
    file_name_plx=mxCalloc(file_name_len_plx,sizeof(char));
    
    if (file_name_plx == NULL) {
        mexErrMsgTxt("Out of memory CODE:1-5 !");
        exit(1);
    }
    
    status=mxGetString(prhs[2],file_name_plx,file_name_len_plx);
    if(status !=0)
    {
        mexWarnMsgTxt("Decode Plx file name error: Not enough space, String is truncated.");
    }
    mexPrintf("Plx File Name is: %s\n",file_name_plx);
    if((fp_plx=fopen(file_name_plx,"wb"))==NULL)
	{
        mexErrMsgTxt("Cannot create Plx file!");
		exit(1);
	} 

    iEphys_SpikeDataPacket = (struct Ephys_SpikeDataPacket **) mxCalloc(NumSpikeFiles,sizeof(struct Ephys_SpikeDataPacket *));
    iEphys_SpikeDataPacket_Data = (struct Ephys_SpikeDataPacket_Data **) mxCalloc(NumSpikeFiles,sizeof(struct Ephys_SpikeDataPacket_Data *));
	iEphys_ContinuousDataPacket = (struct Ephys_ContinuousDataPacket **) mxCalloc(NumContinuousFiles,sizeof(struct Ephys_ContinuousDataPacket *));
	iEphys_ContinuousDataPacket_Data = (struct Ephys_ContinuousDataPacket_Data **) mxCalloc(NumContinuousFiles, sizeof(struct Ephys_ContinuousDataPacket_Data*));
	if ((iEphys_ContinuousDataPacket == NULL) || (iEphys_SpikeDataPacket == NULL) || (iEphys_SpikeDataPacket_Data == NULL) || (iEphys_ContinuousDataPacket_Data==NULL))
	{
        mexErrMsgTxt("Out of memory CODE:1-6!");
		exit(1);
	}
    
    ephysReadDataPacket();

    makePlxFileHeader();
    makePlxChanHeader();
    makePlxEventHeader();
    makePlxContinuHeader();
    
  
    writePLXFileHeader(fp_plx); 
    writePLXChanHeader(fp_plx);
    writePLXEventHeader(fp_plx);
    writePLXContinuHeader(fp_plx);
    
    ReadandWriteDataPacket (fp_plx);

    closeEphysFiles();
    fclose(fp_plx);
}
