/*
Name:		UDPMessengerLib.cpp
Created:	28.12.2016 20:49:35
Author:	mklem
Editor:	http://www.visualmicro.com
*/


//extern "C" 
//{
//	#include <stdlib.h>
//	#include <stdarg.h>
//}

#include <stdio.h>
#include "UDPMessengerLib.h"


#define Version 1_0


UDPMessenger::UDPMessenger()
{


}

//SerialDebug
#ifdef DebugModeEnable

void UDPMessenger::SetDebugMode(Stream &comDebug)
{
	_comDebug = &comDebug;
	_DebugMode = true;
}
#endif

void UDPMessenger::attach(messengerCallbackFunction newFunction)
{
	_default_callback = newFunction;
}

void UDPMessenger::attach(byte msgId, messengerCallbackFunction newFunction)
{
	if (msgId >= 0 && msgId < MAXCALLBACKS)
		_callbackList[msgId] = newFunction;
}



void UDPMessenger::InitMessenger(String SSID, String key, int localPort, bool Mode)
{


	//Kons

	_SSID = SSID;
	_key = key;
	_localPort = localPort;
	_Mode = Mode;

	_WL_Status = WL_IDLE_STATUS;

	_command_separator = ';';
	_field_separator = ',';


	//Kons


	_default_callback = NULL;

	_print_newlines = false;
	_pauseProcessing = false;

	_bufferLength = MESSENGERBUFFERSIZE;
	_bufferLastIndex = MESSENGERBUFFERSIZE - 1;
	reset();


	for (int i = 0; i < MAXCALLBACKS; i++)
		_callbackList[i] = NULL;


	//Testen ob Wifi verfügbart
	if (WiFi.status() == WL_NO_SHIELD)
	{

#ifdef DebugModeEnable
		//SerialDebug
		if (_DebugMode)
		{
			_comDebug->println("WiFi shield not present");
		}

#endif
		while (true)
		{
			StatusLED(3, 200, 500);
		};
	}


	if (_Mode == false)
	{
		// attempt to connect to Wifi network:
		while (_WL_Status != WL_CONNECTED)
		{

#ifdef DebugModeEnable
			if (_DebugMode) {
				_comDebug->print("Attempting to connect to SSID: ");
				_comDebug->println(_SSID);
				\
			}
#endif

			// Connect to WPA/WPA2 network. Change this line if using open or WEP network:
			_WL_Status = WiFi.begin(_SSID, _key);

			StatusLED(2, 200, 200);

			// wait 10 seconds for connection:
			delay(10000);
		}

#ifdef DebugModeEnable
		//SerialDebug
		if (_DebugMode) {
			_comDebug->println("Connected to wifi");
			printWifiStatus();
			_comDebug->println("Opening Port: " + String(_localPort, DEC));
			// if you get a connection, report back via serial:	
		}
#endif
		_UDP.begin(_localPort);
	}
	else if (_Mode == true)
	{

		char __key[sizeof(_key)];
		_key.toCharArray(__key, sizeof(__key));

		char __SSID[sizeof(_SSID)];
		_SSID.toCharArray(__SSID, sizeof(__SSID));


		if (_key != "" && (sizeof(_key) == 10 || sizeof(_key) == 16))
		{
			_WL_Status = WiFi.beginAP(__SSID, 1, __key);

#ifdef DebugModeEnable	
			if (_DebugMode)
			{
				_comDebug->println("WEP Key: " + _key);
			}
#endif
		}
		else
		{
			_WL_Status = WiFi.beginAP(__SSID);



			if (_WL_Status != WL_AP_LISTENING)
			{

#ifdef DebugModeEnable
				if (_DebugMode)
				{
					_comDebug->println("Creating access point failed");
				}
#endif
				// don't continue
				while (true)
				{
					StatusLED(3, 200, 500);
				};
			}
		}
#ifdef DebugModeEnable
		if (_DebugMode)
		{

			printWifiStatus();
			_comDebug->println("Opening Port: " + String(_localPort, DEC));
		}
#endif

		_UDP.begin(_localPort);


	}
}

void UDPMessenger::Process()
{

	int _LastPacketSize = _UDP.parsePacket();

#ifdef DebugModeEnable
	if (_DebugMode)
	{
		_comDebug->println("lastPacketSize" + String(_LastPacketSize, DEC));
	}

#endif
	if (_LastPacketSize)
	{

		_remoteIP = _UDP.remoteIP();
		_remotePort = _UDP.remotePort();

#ifdef DebugModeEnable
		if (_DebugMode)
		{
			_comDebug->println("remote Port " + String(_remotePort, DEC));
			_comDebug->println("remote IP " + String(_remoteIP, DEC));
		}

#endif

		size_t bytesAvailable = min(_LastPacketSize, MAXSTREAMBUFFERSIZE);
		_UDP.read(_streamBuffer, bytesAvailable);

		_messageState = kProccesingMessage;

#ifdef DebugModeEnable
		if (_DebugMode)
		{
			_comDebug->println("Processing Message");
		}
#endif
		for (size_t byteNo = 0; byteNo < bytesAvailable; byteNo++)
		{
			_messageState = ProcessByte(_streamBuffer[byteNo]);

			if (_messageState == kEndOfMessage)
			{

#ifdef DebugModeEnable
				if (_DebugMode)
				{
					_comDebug->println("EndOfMessage");
				}
#endif

				handleMessage();
			}
		}
	}

}

uint8_t UDPMessenger::ProcessByte(char dataChar)
{
	_messageState = kProccesingMessage;


	if (dataChar == _command_separator)

	{
		_commandBuffer[_bufferIndex] = 0;

		if (_bufferIndex > 0)
		{
			_messageState = kEndOfMessage;
			_current = _commandBuffer;
		}

		reset();
	}
	else
	{
		_commandBuffer[_bufferIndex] = dataChar;
		_bufferIndex++;

		if (_bufferIndex >= _bufferLastIndex)
		{
			reset();
		}
	}
	return _messageState;
}

/**
* Waits for reply from sender or timeout before continuing
*/
void UDPMessenger::handleMessage()
{
	_lastCommandId = readInt16Arg();
	// if command attached, we will call it
	if (_lastCommandId >= 0 && _lastCommandId < MAXCALLBACKS && _ArgOk && _callbackList[_lastCommandId] != NULL)
	{

#ifdef DebugModeEnable
		if (_DebugMode)
		{
			_comDebug->println("Callback: " + String(_lastCommandId, DEC));
		}

#endif
		(*_callbackList[_lastCommandId])();
	}
	else // If command not attached, call default callback (if attached)
		if (_default_callback != NULL)
		{

#ifdef DebugModeEnable
			if (_DebugMode)
			{
				_comDebug->println("Default Callback");
			}
#endif

			(*_default_callback)();
		}
}

/**
* Gets next argument. Returns true if an argument is available
*/
bool UDPMessenger::next()
{
	char * _temppointer = NULL;
	// Currently, cmd messenger only supports 1 char for the field seperator
	switch (_messageState)
	{

	case kProccesingMessage:
		return false;
	case kEndOfMessage:
		_temppointer = _commandBuffer;
		_messageState = kProcessingArguments;
	default:
		if (_dumped)
			_current = split_r(_temppointer, ',', &_last);
		if (_current != NULL)
		{
			_dumped = true;
			return true;
		}
	}
	return false;
}

/**
* Find next argument in command
*/
int UDPMessenger::findNext(char *str, char delim)
{
	int pos = 0;
	while (true)
	{

		if (*str == ',')
		{
			return pos;
		}
		else {
			str++;
			pos++;
		}
	}
	return pos;
}

/**
* Read the next argument as int
*/
int16_t UDPMessenger::readInt16Arg()
{
	if (next()) {
		_dumped = true;
		_ArgOk = true;
		return atoi(_current);
	}
	_ArgOk = false;
	return 0;
}

/**
* Send start of command. This makes it easy to send multiple arguments per command
*/
void UDPMessenger::sendCmdStart(byte cmdId)
{
	if (!_startCommand) {
		_startCommand = true;
		_pauseProcessing = true;
		_UDP.beginPacket(_remoteIP, _remotePort);
		_UDP.write(cmdId);

	}
}

/**
* Send a command without arguments, without acknowledge
*/
bool UDPMessenger::sendCmd(byte cmdId)
{
	if (!_startCommand) {
		sendCmdStart(cmdId);
		return sendCmdEnd();
	}
	return false;
}

/**
* Send end of command
*/
bool UDPMessenger::sendCmdEnd()
{
	bool repl;
	if (_startCommand) {

		_UDP.write(';');


		//_comDebug->println("EndPacket: " + _UDP.endPacket());
		_UDP.endPacket();

	}
	_pauseProcessing = false;
	_startCommand = false;
	return repl;
}



/**
* Read the next argument as int
*/
int32_t UDPMessenger::readInt32Arg()
{
	if (next()) {
		_dumped = true;
		_ArgOk = true;
		return atol(_current);
	}
	_ArgOk = false;
	return 0L;
}

/**
* Read the next argument as bool
*/
bool UDPMessenger::readBoolArg()
{
	return (readInt16Arg() != 0) ? true : false;
}

/**
* Read the next argument as char
*/
char UDPMessenger::readCharArg()
{
	if (next()) {
		_dumped = true;
		_ArgOk = true;
		return _current[0];
	}
	_ArgOk = false;
	return 0;
}

/**
* Read the next argument as float
*/
float UDPMessenger::readFloatArg()
{
	if (next()) {
		_dumped = true;
		_ArgOk = true;
		//return atof(current);
		return strtod(_current, NULL);
	}
	_ArgOk = false;
	return 0;
}

/**
* Read the next argument as double
*/
double UDPMessenger::readDoubleArg()
{
	if (next()) {
		_dumped = true;
		_ArgOk = true;
		return strtod(_current, NULL);
	}
	_ArgOk = false;
	return 0;
}


/*
Read next argument as string.
Note that the String is valid until the current command is replaced
*/

char* UDPMessenger::readStringArg()
{
	if (next()) {
		_dumped = true;
		_ArgOk = true;
		return _current;
	}
	_ArgOk = false;
	return '\0';
}



/*
* Split string in different tokens, based on delimiter
* Note that this is basically strtok_r, but with support for an escape character
*/
char* UDPMessenger::split_r(char *str, const char delim, char **nextp)
{
	char *ret;
	// if input null, this is not the first call, use the nextp pointer instead
	if (str == NULL) {
		str = *nextp;
	}
	// Strip leading delimiters
	while (findNext(str, delim) == 0 && *str) {
		str++;
	}
	// If this is a \0 char, return null
	if (*str == '\0') {
		return NULL;
	}
	// Set start of return pointer to this position
	ret = str;
	// Find next delimiter
	str += findNext(str, delim);
	// and exchange this for a a \0 char. This will terminate the char
	if (*str) {
		*str++ = '\0';
	}
	// Set the next pointer to this char
	*nextp = str;
	// return current pointer
	return ret;
}

/*
Resets the command buffer and message state
*/
void UDPMessenger::reset()
{
	_bufferIndex = 0;
	_current = NULL;
	_last = NULL;
	_dumped = true;
}

#ifdef DebugModeEnable
void UDPMessenger::printWifiStatus() {
	// print the SSID of the network you're attached to:
	_comDebug->print("SSID: ");
	_comDebug->println(WiFi.SSID());

	// print your WiFi shield's IP address:
	IPAddress ip = WiFi.localIP();
	_comDebug->print("IP Address: ");
	_comDebug->println(ip);


}
#endif

//Blocking
void UDPMessenger::StatusLED(int Count, int delay_1, int delay_2)
{
	pinMode(LED_BUILTIN, OUTPUT);

	for (int i = 0; i < Count; i++)
	{
		digitalWrite(LED_BUILTIN, HIGH);
		delay(delay_1);
		digitalWrite(LED_BUILTIN, LOW);
		delay(delay_2);
	}

	delay(delay_1 + delay_2);
}
