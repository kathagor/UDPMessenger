/*
Name:		UDPMessengerLib.h
Created:	28.12.2016 20:49:35
Author:	mklem
Editor:	http://www.visualmicro.com
*/





#ifndef _UDPMessengerLib_h
#define _UDPMessengerLib_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "arduino.h"
#include <WiFi101.h>
#include <WiFiUdp.h>
#else
#include "WProgram.h"
#endif


extern "C"
{
	// callback functions always follow the signature: void cmd(void);
	typedef void(*messengerCallbackFunction) (void);
}


#define MAXCALLBACKS 50					// The maximum number of commands   (default: 50)
#define MAXSTREAMBUFFERSIZE 512			//Max Size of Streambuffer
#define MESSENGERBUFFERSIZE 64			// The length of the commandbuffer  (default: 64)

//#define DebugModeEnable

#define UDP_MESS_AP true				//Setting up WiFi as AccesPoint
#define UDP_MESS_INF false				//Connect to existing WLAN

enum
{
	kProccesingMessage,					// Message is being received, not reached command separator
	kEndOfMessage,						// Message is fully received, reached command separator
	kProcessingArguments,				// Message is received, arguments are being read parsed
};

class UDPMessenger
{

public:

	UDPMessenger();

	void attach(messengerCallbackFunction newFunction);					//attaches a default callback function
	void attach(byte msgId, messengerCallbackFunction newFunction);		//attaches a callback function for a specific message

#ifdef DebugModeEnable
	void SetDebugMode(Stream& comDebug); //Activates Debuging over Serial
#endif // DebugModeEnable




	void InitMessenger(String SSID, String key, int localPort, bool Mode);		//initilize the UDPMessenger 
	void Process();				//Call in "Loop", process the incoming data

	void sendCmdStart(byte cmdId);
	bool sendCmd(byte cmdId);
	void sendCmdfArg(char *fmt, ...);
	bool sendCmdEnd();


	/**
	* Send a command with a single argument of any type
	* Note that the argument is sent as string
	*/
	template < class T >
	bool sendCmd(byte cmdId, T arg)
	{
		if (!_startCommand) {
			sendCmdStart(cmdId);
			sendCmdArg(arg);
			return sendCmdEnd();
		}
		return false;
	}

	/**
	* Send a single argument as string
	*  Note that this will only succeed if a sendCmdStart has been issued first
	*/
	template < class T > void sendCmdArg(T arg)
	{
		if (_startCommand) {
			_UDP.write(',');
			_UDP.write(arg);
		}
	}

	/**
	* Send a single argument as string with custom accuracy
	*  Note that this will only succeed if a sendCmdStart has been issued first
	*/
	template < class T > void sendCmdArg(T arg, unsigned int n)
	{
		if (_startCommand) {
			_UDP.write(',');
			_UDP.print(arg, n);
		}
	}



	char *readStringArg();

	bool readBoolArg();
	int16_t readInt16Arg();
	int32_t readInt32Arg();
	char readCharArg();
	float readFloatArg();
	double readDoubleArg();

	bool next();
	int findNext(char *str, char delim);



private:

	WiFiUDP _UDP;



	String _SSID;
	String _key;

	unsigned int _localPort;



	IPAddress _remoteIP;			//IP Adress of remote host
	unsigned int _remotePort;		//Port of the remote Host


	int _WL_Status;
	int _LastPacketSize;

	uint8_t _bufferIndex;           // Index where to write data in buffer
	uint8_t _bufferLength;          // Is set to MESSENGERBUFFERSIZE
	uint8_t _bufferLastIndex;		// The last index of the buffer
	uint8_t _messageState;			// Current state of message processing
	uint8_t _lastCommandId;			// ID of last received command 


	bool _DebugMode;				//enables Debing over Serial
	bool _Mode;						//Wifi working in AP-Mode or connect to router

	bool _dumped;                   // Indicates if last argument has been externally read 
	bool _ArgOk;					// Indicated if last fetched argument could be read

	bool _print_newlines;			// Indicates if \r\n should be added after send command
	bool _startCommand;				// Indicates if sending of a command is underway
	bool _pauseProcessing;			// pauses processing of new commands, during sending


	char _commandBuffer[MESSENGERBUFFERSIZE];	// Buffer that holds the data for processing
	char _streamBuffer[MAXSTREAMBUFFERSIZE];	// Buffer that holds the UDP packet data
	char _command_separator;					// Character indicating end of command (default: ';')
	char _field_separator;						// Character indicating end of argument (default: ',')

	char *_current;								// Pointer to current buffer position
	char *_last;									// Pointer to previous buffer position

	char *split_r(char *str, const char delim, char **nextp);

	Stream *_comDebug;

	messengerCallbackFunction _default_callback;				// default callback function  
	messengerCallbackFunction _callbackList[MAXCALLBACKS];	// list of attached callback functions 


	void reset();


	void printWifiStatus();
	void StatusLED(int Count, int delay_1, int delay_2);

	void handleMessage();

	uint8_t ProcessByte(char dataChar); //Process one byte of Data

};


#endif


