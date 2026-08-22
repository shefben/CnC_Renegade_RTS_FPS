
/***********************************************************************************************
 ***              C O N F I D E N T I A L  ---  W E S T W O O D  S T U D I O S               ***
 ***********************************************************************************************
 *                                                                                             *
 *                 Project Name : Bandwidth Tester                                             *
 *                                                                                             *
 *                     $Archive:: /Commando/Code/BandTest/BandTest.h                          $*
 *                                                                                             *
 *                      $Author:: Steve_t                                                     $*
 *                                                                                             *
 *                     $Modtime:: 1/28/02 1:50p                                               $*
 *                                                                                             *
 *                    $Revision:: 5                                                           $*
 *                                                                                             *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 *                                                                                             *
 *---------------------------------------------------------------------------------------------*
 *                                                                                             *
 * Functions:                                                                                  *
 * - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - */

/*
** The following ifdef block is the standard way of creating macros which make exporting
** from a DLL simpler. All files within this DLL are compiled with the BANDTEST_EXPORTS
** symbol defined on the command line. this symbol should not be defined on any project
** that uses this DLL. This way any other project whose source files include this file see
** BANDTEST_API functions as being imported from a DLL, wheras this DLL sees symbols
** defined with this macro as being exported.
*/
#ifdef TTINIT_EXPORTS
#define BANDTEST_API __declspec(dllexport)
#else
#define BANDTEST_API __declspec(dllimport)
#endif

/*
** Extended failure codes.
*/
enum {
	BANDTEST_OK,
	BANDTEST_NO_WINSOCK2,
	BANDTEST_NO_RAW_SOCKET_PERMISSION,
	BANDTEST_NO_RAW_SOCKET_CREATE,
	BANDTEST_NO_UDP_SOCKET_BIND,
	BANDTEST_NO_TTL_SET,
	BANDTEST_NO_PING_RESPONSE,
	BANDTEST_NO_FINAL_PING_TIME,
	BANDTEST_NO_EXTERNAL_ROUTER,
	BANDTEST_NO_IP_DETECT,
	BANDTEST_UNKNOWN_ERROR,
	BANDTEST_WRONG_API_VERSION,
	BANDTEST_BAD_PARAM,
};


typedef struct tBandtestSettingsStruct {

	/*
	** Use ICMP packets instead of UDP packets when testing bandwidth.
	*/
	unsigned int AlwaysICMP				: 1;

	/*
	** Use various values of TTL when testing bandwidth.
	*/
	unsigned int TTLScatter				: 1;

	/*
	** Max number of packets to send for slow pings.
	*/
	unsigned int FastPingPackets		: 7;

	/*
	** Max number of packets to send for fast pings.
	*/
	unsigned int SlowPingPackets  	: 7;

	/*
	** Pings over this time in ms use smaller number of packets for bandwidth discovery.
	*/
	unsigned int FastPingThreshold	: 5;

	/*
	** Tell bandtest.dll to do a ping profile.
	*/
	unsigned int PingProfile			: 1;

} BandtestSettingsStruct;

#define BANDTEST_API_VERSION 0x101

BANDTEST_API unsigned long Detect_Bandwidth(unsigned long server_ip, unsigned long my_ip, int retries, int &failure_code, unsigned long &downstream, unsigned long api_version, BandtestSettingsStruct *settings = NULL, char *regpath = NULL);