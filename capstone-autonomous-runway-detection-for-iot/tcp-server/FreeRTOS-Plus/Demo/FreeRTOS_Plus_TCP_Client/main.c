/*
    FreeRTOS V9.0.0 - Copyright (C) 2016 Real Time Engineers Ltd.
    All rights reserved

    VISIT http://www.FreeRTOS.org TO ENSURE YOU ARE USING THE LATEST VERSION.

    This file is part of the FreeRTOS distribution.

    FreeRTOS is free software; you can redistribute it and/or modify it under
    the terms of the GNU General Public License (version 2) as published by the
    Free Software Foundation >>!AND MODIFIED BY!<< the FreeRTOS exception.

    ***************************************************************************
    >>!   NOTE: The modification to the GPL is included to allow you to     !<<
    >>!   distribute a combined work that includes FreeRTOS without being   !<<
    >>!   obliged to provide the source code for proprietary components     !<<
    >>!   outside of the FreeRTOS kernel.                                   !<<
    ***************************************************************************

    FreeRTOS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE.  Full license text is available on the following
    link: http://www.freertos.org/a00114.html

    ***************************************************************************
     *                                                                       *
     *    FreeRTOS provides completely free yet professionally developed,    *
     *    robust, strictly quality controlled, supported, and cross          *
     *    platform software that is more than just the market leader, it     *
     *    is the industry's de facto standard.                               *
     *                                                                       *
     *    Help yourself get started quickly while simultaneously helping     *
     *    to support the FreeRTOS project by purchasing a FreeRTOS           *
     *    tutorial book, reference manual, or both:                          *
     *    http://www.FreeRTOS.org/Documentation                              *
     *                                                                       *
    ***************************************************************************

    http://www.FreeRTOS.org/FAQHelp.html - Having a problem?  Start by reading
    the FAQ page "My application does not run, what could be wrong?".  Have you
    defined configASSERT()?

    http://www.FreeRTOS.org/support - In return for receiving this top quality
    embedded software for free we request you assist our global community by
    participating in the support forum.

    http://www.FreeRTOS.org/training - Investing in training allows your team to
    be as productive as possible as early as possible.  Now you can receive
    FreeRTOS training directly from Richard Barry, CEO of Real Time Engineers
    Ltd, and the world's leading authority on the world's leading RTOS.

    http://www.FreeRTOS.org/plus - A selection of FreeRTOS ecosystem products,
    including FreeRTOS+Trace - an indispensable productivity tool, a DOS
    compatible FAT file system, and our tiny thread aware UDP/IP stack.

    http://www.FreeRTOS.org/labs - Where new FreeRTOS products go to incubate.
    Come and try FreeRTOS+TCP, our new open source TCP/IP stack for FreeRTOS.

    http://www.OpenRTOS.com - Real Time Engineers ltd. license FreeRTOS to High
    Integrity Systems ltd. to sell under the OpenRTOS brand.  Low cost OpenRTOS
    licenses offer ticketed support, indemnification and commercial middleware.

    http://www.SafeRTOS.com - High Integrity Systems also provide a safety
    engineered and independently SIL3 certified version for use in safety and
    mission critical applications that require provable dependability.

    1 tab == 4 spaces!
*/

/*
 * Instructions for using this project are provided on:
 * http://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_TCP/examples_FreeRTOS_simulator.html
 *
 * NOTE: Some versions of Visual Studio will generate erroneous compiler
 * warnings about variables being used before they are set.
 */

/* Standard includes. */
#include <stdio.h>
#include <time.h>

/* FreeRTOS includes. */
#include <FreeRTOS.h>
#include "task.h"
#include "timers.h"
#include "queue.h"
#include "semphr.h"

/* FreeRTOS+TCP includes. */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#include "FreeRTOS_TCP_server.h"
#include "FreeRTOS_DHCP.h"

/* FreeRTOS+FAT includes. */
#include "ff_headers.h"
#include "ff_stdio.h"
#include "ff_ramdisk.h"

/* Demo application includes. */
#include "SimpleUDPClientAndServer.h"
#include "TwoUDPEchoClients.h"
#include "TCPEchoClient_SingleTasks.h"
#include "TCPEchoClient_SeparateTasks.h"
#include "UDPCommandConsole.h"
#include "TCPCommandConsole.h"
#include "UDPSelectServer.h"
#include "SimpleTCPEchoServer.h"
#include "TFTPServer.h"
#include "demo_logging.h"

#include "canny.h"
#include "rsa.h"

/* UDP command server task parameters. */
#define mainUDP_CLI_TASK_PRIORITY						( tskIDLE_PRIORITY )
#define mainUDP_CLI_PORT_NUMBER							( 5001UL )

/* TCP command server task parameters.  The standard telnet port is used even
though this is not implementing a real telnet server. */
#define mainTCP_CLI_TASK_PRIORITY						( tskIDLE_PRIORITY )
#define mainTCP_CLI_PORT_NUMBER							( 23UL )

/* Simple UDP client and server task parameters. */
#define mainSIMPLE_UDP_CLIENT_SERVER_TASK_PRIORITY		( tskIDLE_PRIORITY )
#define mainSIMPLE_UDP_CLIENT_SERVER_PORT				( 5005UL )

/* Select UDP server task parameters. */
#define mainUDP_SELECT_SERVER_TASK_PRIORITY				( tskIDLE_PRIORITY )
#define mainUDP_SELECT_SERVER_PORT						( 30001UL )

/* Echo client task parameters - used for both TCP and UDP echo clients. */
#define mainECHO_CLIENT_TASK_STACK_SIZE 				( configMINIMAL_STACK_SIZE * 2 )
#define mainECHO_CLIENT_TASK_PRIORITY					( tskIDLE_PRIORITY + 1 )

/* FTP and HTTP servers execute in the TCP server work task. */
#define mainTCP_SERVER_TASK_PRIORITY					( tskIDLE_PRIORITY + 2 )
#define	mainTCP_SERVER_STACK_SIZE						1400 /* Not used in the Win32 simulator. */

/* TFTP server parameters. */
#define mainTFTP_SERVER_PRIORITY						( tskIDLE_PRIORITY + 1 )
#define mainTFTP_SERVER_STACK_SIZE						1400 /* Not used in the Win32 simulator. */

/* Dimensions the buffer used to send UDP print and debug messages. */
#define cmdPRINTF_BUFFER_SIZE		512

/* The number and size of sectors that will make up the RAM disk.  The RAM disk
is huge to allow some verbose FTP tests. */
#define mainRAM_DISK_SECTOR_SIZE	512UL /* Currently fixed! */
#define mainRAM_DISK_SECTORS		( ( 5UL * 1024UL * 1024UL ) / mainRAM_DISK_SECTOR_SIZE ) /* 5M bytes. */
#define mainIO_MANAGER_CACHE_SIZE	( 15UL * mainRAM_DISK_SECTOR_SIZE )

/* Where the RAM disk is mounted. */
#define mainRAM_DISK_NAME			"/ram"

/* Define a name that will be used for LLMNR and NBNS searches. */
#define mainHOST_NAME				"RTOSDemo"
#define mainDEVICE_NICK_NAME		"windows_demo"

/* Set to 0 to run the STDIO examples once only, or 1 to create multiple tasks
that run the tests continuously. */
#define mainRUN_STDIO_TESTS_IN_MULTIPLE_TASK 0

/* Set the following constants to 1 or 0 to define which tasks to include and
exclude:

mainCREATE_FTP_SERVER:  When set to 1 the TCP server task will include an FTP
server.
See http://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_TCP/FTP_Server.html

mainCREATE_HTTP_SERVER:  When set to 1 the TCP server task will include a basic
HTTP server.
See http://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_TCP/HTTP_web_Server.html

mainCREATE_UDP_CLI_TASKS:  When set to 1 a command console that uses a UDP port
for input and output is created using FreeRTOS+CLI.  The port number used is set
by the mainUDP_CLI_PORT_NUMBER constant above.  A dumb UDP terminal such as YAT
can be used to connect.
See http://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_TCP/UDP_CLI.html

mainCREATE_TCP_CLI_TASKS:  When set to 1 a command console that uses a TCP port
for input and output is created using FreeRTOS+CLI.  The port number used is set
by the mainTCP_CLI_PORT_NUMBER constant above.  A dumb UDP terminal such as YAT
can be used to connect.

mainCREATE_SIMPLE_UDP_CLIENT_SERVER_TASKS:  When set to 1 two UDP client tasks
and two UDP server tasks are created.  The clients talk to the servers.  One set
of tasks use the standard sockets interface, and the other the zero copy sockets
interface.  These tasks are self checking and will trigger a configASSERT() if
they detect a difference in the data that is received from that which was sent.
As these tasks use UDP, and can therefore loose packets, they will cause
configASSERT() to be called when they are run in a less than perfect networking
environment.
See http://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_TCP/UDP_client_server.html

mainCREATE_SELECT_UDP_SERVER_TASKS: Uses two tasks to demonstrate the use of the
FreeRTOS_select() function.

mainCREATE_UDP_ECHO_TASKS:  When set to 1 a two tasks are created that send
UDP echo requests to the standard echo port (port 7).  One task uses the
standard socket interface, the other the zero copy socket interface.  The IP
address of the echo server must be configured using the configECHO_SERVER_ADDR0
to configECHO_SERVER_ADDR3 constants in FreeRTOSConfig.h.  These tasks are self
checking and will trigger a configASSERT() if the received echo reply does not
match the transmitted echo request.  As these tasks use UDP, and can therefore
loose packets, they will cause configASSERT() to be called when they are run in
a less than perfect networking environment, or when connected to an echo server
that (legitimately as UDP is used) opts not to reply to every echo request.
See http://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_TCP/UDP_Echo_Clients.html

mainCREATE_TCP_ECHO_TASKS_SINGLE:  When set to 1 a set of tasks are created that
send TCP echo requests to the standard echo port (port 7), then wait for and
verify the echo reply, from within the same task (Tx and Rx are performed in the
same RTOS task).  The IP address of the echo server must be configured using the
configECHO_SERVER_ADDR0 to configECHO_SERVER_ADDR3 constants in
FreeRTOSConfig.h.
See http://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_TCP/TCP_Echo_Clients.html

mainCREATE_TCP_ECHO_TASKS_SEPARATE:  As per the description for the
mainCREATE_TCP_ECHO_TASKS_SINGLE constant above, except this time separate tasks
are used to send data to and receive data from the echo server (one task is used
for Tx and another task for Rx, using the same socket).
See http://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_TCP/TCP_Echo_Clients_Separate.html

mainCREATE_SIMPLE_TCP_ECHO_SERVER:  When set to 1 FreeRTOS tasks are used with
FreeRTOS+TCP to create a TCP echo server.  Echo clients are also created, but
the echo clients use Windows threads (as opposed to FreeRTOS tasks) and use the
Windows TCP library (Winsocks).  This creates a communication between the
FreeRTOS+TCP TCP/IP stack and the Windows TCP/IP stack.
See http://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_TCP/TCP_Echo_Server.html
*/
#define mainCREATE_UDP_CLI_TASKS					0
#define mainCREATE_TCP_CLI_TASKS					0
#define mainCREATE_SIMPLE_UDP_CLIENT_SERVER_TASKS	0
#define mainCREATE_SELECT_UDP_SERVER_TASKS			0 /* _RB_ Requires retest. */
#define mainCREATE_UDP_ECHO_TASKS					0
#define mainCREATE_TCP_ECHO_TASKS_SINGLE			0
#define mainCREATE_TCP_ECHO_TASKS_SEPARATE			0
#define mainCREATE_SIMPLE_TCP_ECHO_SERVER			0
#define mainCREATE_FTP_SERVER						1
#define mainCREATE_HTTP_SERVER 						0
#define mainCREATE_TFTP_SERVER						0

/* Set the following constant to pdTRUE to log using the method indicated by the
name of the constant, or pdFALSE to not log using the method indicated by the
name of the constant.  Options include to standard out (mainLOG_TO_STDOUT), to a
disk file (mainLOG_TO_DISK_FILE), and to a UDP port (mainLOG_TO_UDP).  If
mainLOG_TO_UDP is set to pdTRUE then UDP messages are sent to the IP address
configured as the echo server address (see the configECHO_SERVER_ADDR0
definitions in FreeRTOSConfig.h) and the port number set by configPRINT_PORT in
FreeRTOSConfig.h. */
#define mainLOG_TO_STDOUT 		pdTRUE
#define mainLOG_TO_DISK_FILE 	pdFALSE
#define mainLOG_TO_UDP 			pdFALSE

/*-----------------------------------------------------------*/
typedef struct
{
    uint8_t *pData;
    uint32_t len;
} message_t;

/*
 * Register commands that can be used with FreeRTOS+CLI through the UDP socket.
 * The commands are defined in CLI-commands.c and File-related-CLI-commands.c
 * respectively.
 */
extern void vRegisterCLICommands( void );
extern void vRegisterFileSystemCLICommands( void );

/*
 * A software timer is created that periodically checks that some of the TCP/IP
 * demo tasks are still functioning as expected.  This is the timer's callback
 * function.
 */
static void prvCheckTimerCallback( TimerHandle_t xTimer );

/*
 * Just seeds the simple pseudo random number generator.
 */
static void prvSRand( UBaseType_t ulSeed );

/*
 * Miscellaneous initialisation including preparing the logging and seeding the
 * random number generator.
 */
static void prvMiscInitialisation( void );

/*
 * Creates a RAM disk, then creates files on the RAM disk.  The files can then
 * be viewed via the FTP server and the command line interface.
 */
static void prvCreateDiskAndExampleFiles( void );

/*
 * Functions used to create and then test files on a disk.
 */
extern void vCreateAndVerifyExampleFiles( const char *pcMountPath );
extern void vStdioWithCWDTest( const char *pcMountPath );
extern void vMultiTaskStdioWithCWDTest( const char *const pcMountPath, uint16_t usStackSizeWords );

/*
 * The task that runs the FTP and HTTP servers.
 */
#if( ( mainCREATE_FTP_SERVER == 1 ) || ( mainCREATE_HTTP_SERVER == 1 ) )
	static void prvServerWorkTask( void *pvParameters );
#endif

static void prvCannyEdgeTask( void *pvParameters );
static void prvEncryptionTask( void *pvParameters );
static void prvClientTask( void *pvParameters );

/* The default IP and MAC address used by the demo.  The address configuration
defined here will be used if ipconfigUSE_DHCP is 0, or if ipconfigUSE_DHCP is
1 but a DHCP server could not be contacted.  See the online documentation for
more information. */
static const uint8_t ucIPAddress[ 4 ] = { configIP_ADDR0, configIP_ADDR1, configIP_ADDR2, configIP_ADDR3 };
static const uint8_t ucNetMask[ 4 ] = { configNET_MASK0, configNET_MASK1, configNET_MASK2, configNET_MASK3 };
static const uint8_t ucGatewayAddress[ 4 ] = { configGATEWAY_ADDR0, configGATEWAY_ADDR1, configGATEWAY_ADDR2, configGATEWAY_ADDR3 };
static const uint8_t ucDNSServerAddress[ 4 ] = { configDNS_SERVER_ADDR0, configDNS_SERVER_ADDR1, configDNS_SERVER_ADDR2, configDNS_SERVER_ADDR3 };

/* Default MAC address configuration.  The demo creates a virtual network
connection that uses this MAC address by accessing the raw Ethernet data
to and from a real network connection on the host PC.  See the
configNETWORK_INTERFACE_TO_USE definition for information on how to configure
the real network connection to use. */
const uint8_t ucMACAddress[ 6 ] = { configMAC_ADDR0, configMAC_ADDR1, configMAC_ADDR2, configMAC_ADDR3, configMAC_ADDR4, configMAC_ADDR5 };

/* The UDP address to which print messages are sent. */
static struct freertos_sockaddr xPrintUDPAddress;

/* Use by the pseudo random number generator. */
static UBaseType_t ulNextRand;

/* Handle of the task that runs the FTP client */
static TaskHandle_t xTCPClientTaskHandle = NULL;
static TaskHandle_t xCannyTaskHandle = NULL;
static TaskHandle_t xEncryptionTaskHandle = NULL;
static QueueHandle_t xEncryptQueue = NULL;
static QueueHandle_t xTCPClientQueue = NULL;

/*-----------------------------------------------------------*/

/*
 * NOTE: Some versions of Visual Studio will generate erroneous compiler
 * warnings about variables being used before they are set.
 */
int main( void )
{
	const uint32_t ulLongTime_ms = 250UL, ulCheckTimerPeriod_ms = 15000UL;
	TimerHandle_t xCheckTimer;

	/*
	 * Instructions for using this project are provided on:
	 * http://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_TCP/examples_FreeRTOS_simulator.html
	 *
	 * NOTE: Some versions of Visual Studio will generate erroneous compiler
	 * warnings about variables being used before they are set.
	 */

	/* Miscellaneous initialisation including preparing the logging and seeding
	the random number generator. */
	prvMiscInitialisation();

	/* Initialise the network interface.

	***NOTE*** Tasks that use the network are created in the network event hook
	when the network is connected and ready for use (see the definition of
	vApplicationIPNetworkEventHook() below).  The address values passed in here
	are used if ipconfigUSE_DHCP is set to 0, or if ipconfigUSE_DHCP is set to 1
	but a DHCP server cannot be	contacted. */
	FreeRTOS_debug_printf( ( "FreeRTOS_IPInit\n" ) );
	FreeRTOS_IPInit( ucIPAddress, ucNetMask, ucGatewayAddress, ucDNSServerAddress, ucMACAddress );

	xTaskCreate(prvClientTask, "ClientTask", mainTCP_SERVER_STACK_SIZE, NULL, mainTCP_SERVER_TASK_PRIORITY + 1, &xTCPClientTaskHandle);
    xTaskCreate(prvCannyEdgeTask, "CannyEdgeTask", mainTCP_SERVER_STACK_SIZE, NULL, mainTCP_SERVER_TASK_PRIORITY, &xCannyTaskHandle );
    xTaskCreate(prvEncryptionTask, "EncryptionTask", mainTCP_SERVER_STACK_SIZE, NULL, mainTCP_SERVER_TASK_PRIORITY, &xEncryptionTaskHandle );

    xEncryptQueue = xQueueCreate( 1, sizeof(message_t));
    xTCPClientQueue = xQueueCreate( 1, sizeof(message_t));

	/* Start the RTOS scheduler. */
	FreeRTOS_debug_printf( ("vTaskStartScheduler\n") );
	vTaskStartScheduler();

	/* If all is well, the scheduler will now be running, and the following
	line will never be reached.  If the following line does execute, then
	there was insufficient FreeRTOS heap memory available for the idle and/or
	timer tasks	to be created.  See the memory management section on the
	FreeRTOS web site for more details (this is standard text that is not not
	really applicable to the Win32 simulator port). */
	for( ;; )
	{
		Sleep( ulLongTime_ms );
	}
}
/*-----------------------------------------------------------*/

static void prvCreateDiskAndExampleFiles( void )
{
static uint8_t ucRAMDisk[ mainRAM_DISK_SECTORS * mainRAM_DISK_SECTOR_SIZE ];
FF_Disk_t *pxDisk;

	/* Create the RAM disk. */
	pxDisk = FF_RAMDiskInit( mainRAM_DISK_NAME, ucRAMDisk, mainRAM_DISK_SECTORS, mainIO_MANAGER_CACHE_SIZE );
	configASSERT( pxDisk );

	/* Print out information on the disk. */
	FF_RAMDiskShowPartition( pxDisk );

	/* Create a few example files on the disk.  These are not deleted again. */
	vCreateAndVerifyExampleFiles( mainRAM_DISK_NAME );

	/* A few sanity checks only - can only be called after
	vCreateAndVerifyExampleFiles(). */
	#if( mainRUN_STDIO_TESTS_IN_MULTIPLE_TASK == 1 )
	{
		/* Note the stack size is not actually used in the Windows port. */
		vMultiTaskStdioWithCWDTest( mainRAM_DISK_NAME, configMINIMAL_STACK_SIZE * 2U );
	}
	#else
	{
		vStdioWithCWDTest( mainRAM_DISK_NAME );
	}
	#endif
}
/*-----------------------------------------------------------*/

#if( ( mainCREATE_FTP_SERVER == 1 ) || ( mainCREATE_HTTP_SERVER == 1 ) )

	static void prvServerWorkTask( void *pvParameters )
	{
	TCPServer_t *pxTCPServer = NULL;
	const TickType_t xInitialBlockTime = pdMS_TO_TICKS( 200UL );

	/* A structure that defines the servers to be created.  Which servers are
	included in the structure depends on the mainCREATE_HTTP_SERVER and
	mainCREATE_FTP_SERVER settings at the top of this file. */
	static const struct xSERVER_CONFIG xServerConfiguration[] =
	{
		#if( mainCREATE_HTTP_SERVER == 1 )
				/* Server type,		port number,	backlog, 	root dir. */
				{ eSERVER_HTTP, 	80, 			12, 		configHTTP_ROOT },
		#endif

		#if( mainCREATE_FTP_SERVER == 1 )
				/* Server type,		port number,	backlog, 	root dir. */
				{ eSERVER_FTP,  	21, 			12, 		"" }
		#endif
	};

		/* Remove compiler warning about unused parameter. */
		( void ) pvParameters;

		/* Create the RAM disk used by the FTP and HTTP servers. */
	    /* prvCreateDiskAndExampleFiles(); */

		/* The priority of this task can be raised now the disk has been
		initialised. */
		vTaskPrioritySet( NULL, mainTCP_SERVER_TASK_PRIORITY );

		/* If the CLI is included in the build then register commands that allow
		the file system to be accessed. */
		#if( ( mainCREATE_UDP_CLI_TASKS == 1 ) || ( mainCREATE_TCP_CLI_TASKS == 1 ) )
		{
			vRegisterFileSystemCLICommands();
		}
		#endif /* mainCREATE_UDP_CLI_TASKS */


		/* Wait until the network is up before creating the servers.  The
		notification is given from the network event hook. */
		ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

		/* Create the servers defined by the xServerConfiguration array above. */
		for( ;; )
		{
			FreeRTOS_FTPClientWork();
		}
	}

#endif /* ( ( mainCREATE_FTP_SERVER == 1 ) || ( mainCREATE_HTTP_SERVER == 1 ) ) */

static void prvCannyEdgeTask( void *pvParameters )
{
	(void) pvParameters;

	message_t plainBuf;

    while(1)
    {
        static bitmap_info_header_t ih;
        const pixel_t* in_bitmap_data = load_bmp("lenna.bmp", &ih);

        if (in_bitmap_data == NULL)
        {
            FreeRTOS_printf(("BMP image is not loaded.\n"));
            break;
        }

        FreeRTOS_printf(("BMP: width=%d height=%d bitspp=%d\n", ih.width, ih.height, ih.bitspp));

		TickType_t startTick = xTaskGetTickCount();

        const pixel_t* out_bitmap_data = canny_edge_detection(in_bitmap_data, &ih, 40, 80, 1.0);

        if (out_bitmap_data == NULL)
        {
            FreeRTOS_printf(("failed canny_edge_detection.\n"));
            free((pixel_t*)in_bitmap_data);
            break;
        }

        free((pixel_t*)in_bitmap_data);

        if (save_bmp("lenna_ce.bmp", &ih, out_bitmap_data)) {
            FreeRTOS_printf(("BMP image is not saved.\n"));
        }

        if(!pack_pixels(ih.width, ih.height, out_bitmap_data, &plainBuf.pData, &plainBuf.len))
        {
            FreeRTOS_printf(("failed to pack pixels.\n"));
            free((pixel_t*)out_bitmap_data);
            break;
        }

        free((pixel_t*)out_bitmap_data);

		TickType_t endTick = xTaskGetTickCount();
        FreeRTOS_printf(("duration in ticks: %d\n", endTick - startTick));

        if (xQueueSend(xEncryptQueue, &plainBuf, pdMS_TO_TICKS(1000)) != pdPASS)
        {
            free((uint8_t *)plainBuf.pData);
        }
        else
        {
           // the encryption task releases the buffer
        }
    }

    vTaskDelete(NULL);
}

// p = 61, q = 53
// n = p * q
// de = 1 mod (p - 1)(q - 1)
// de = 413 * 17 = 1 mod 780
#define N 3233
#define E 17
#define D 413

static void prvEncryptionTask( void *pvParameters )
{
    (void) pvParameters;

	message_t plainBuf;
	message_t encBuf;

    while(1)
    {
        if(xQueueReceive(xEncryptQueue, &plainBuf, pdMS_TO_TICKS(1000)) == pdPASS)
        {
            if(plainBuf.pData != NULL)
            {
				TickType_t startTick = xTaskGetTickCount();

				FreeRTOS_printf(("encryption\n"));

				int32_t nx = *((int32_t*)plainBuf.pData);
				int32_t ny = ((int32_t*)plainBuf.pData)[1];
				pixel_t* pixels = &(pixel_t*)((int32_t*)plainBuf.pData)[2];

				pixel_t* enc = encrypt_image(pixels, nx, ny, N, E);
				if (enc == NULL) {
					FreeRTOS_printf(("failed to allocate an encryption buffer.\n"));
					free((uint8_t*)plainBuf.pData);
				}
				else
				{
					free((uint8_t*)plainBuf.pData);

					if (!pack_pixels(nx, ny, enc, &encBuf.pData, &encBuf.len))
					{
						FreeRTOS_printf(("failed to pack pixels.\n"));
						free((pixel_t*)enc);
					}
					else
					{
						free((pixel_t*)enc);

						TickType_t endTick = xTaskGetTickCount();
						FreeRTOS_printf(("duration in ticks: %d\n", endTick - startTick));

						if (xQueueSend(xTCPClientQueue, &encBuf, pdMS_TO_TICKS(1000)) != pdPASS)
						{
							free((uint8_t*)encBuf.pData);
						}
					}
				}
            }
        }
    }

    vTaskDelete(NULL);
}

void socket_transmission(char* pcBufferToTransmit, const size_t xTotalLengthToSend, int32_t num);

static void prvClientTask( void *pvParameters )
{
	(void)pvParameters;

    message_t uploadBuf;
	int frame = 1;

    ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // wait for IP link is UP

    while(1)
    {
		if (xQueueReceive(xTCPClientQueue, &uploadBuf, pdMS_TO_TICKS(1000)) == pdPASS)
		{
			if (uploadBuf.pData != NULL)
			{
				TickType_t startTick = xTaskGetTickCount();

				FreeRTOS_printf(("socket transmission\n"));

				socket_transmission(uploadBuf.pData, uploadBuf.len, frame++);

				free((uint8_t*)uploadBuf.pData);

				TickType_t endTick = xTaskGetTickCount();
				FreeRTOS_printf(("duration in ticks: %d\n", endTick - startTick));
			}
		}
    }

    vTaskDelete(NULL);
}

// PC IP4 address
#define TCP_SERVER_ADDR1 192
#define TCP_SERVER_ADDR2 168
#define TCP_SERVER_ADDR3 100
#define TCP_SERVER_ADDR4 11
// VirtualBox IP4 address
//#define TCP_SERVER_ADDR1 10
//#define TCP_SERVER_ADDR2 0
//#define TCP_SERVER_ADDR3 2
//#define TCP_SERVER_ADDR4 15

#define TCP_SERVER_PORT 4040

void socket_transmission(char* pcBufferToTransmit, const size_t xTotalLengthToSend, int32_t num)
{
	FreeRTOS_printf(("(%d) starting transmission\n", num));

	static const TickType_t xReceiveTimeOut = pdMS_TO_TICKS(400);
	static const TickType_t xSendTimeOut = pdMS_TO_TICKS(200);

	/* Fill in the buffer and window sizes that will be used by the socket. */
	WinProperties_t xWinProps;
	xWinProps.lTxBufSize = 6 * ipconfigTCP_MSS;
	xWinProps.lTxWinSize = 3;
	xWinProps.lRxBufSize = 6 * ipconfigTCP_MSS;
	xWinProps.lRxWinSize = 3;

	/* Create a TCP socket. */
	Socket_t xSocket = FreeRTOS_socket(FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP);
	configASSERT(xSocket != FREERTOS_INVALID_SOCKET);

	/* Set a time out so a missing reply does not cause the task to block indefinitely. */
	FreeRTOS_setsockopt(xSocket, 0, FREERTOS_SO_RCVTIMEO, &xReceiveTimeOut, sizeof(xReceiveTimeOut));
	FreeRTOS_setsockopt(xSocket, 0, FREERTOS_SO_SNDTIMEO, &xSendTimeOut, sizeof(xSendTimeOut));

	/* Set the window and buffer sizes. */
	FreeRTOS_setsockopt(xSocket, 0, FREERTOS_SO_WIN_PROPERTIES, (void*)&xWinProps, sizeof(xWinProps));

	struct freertos_sockaddr xRemoteAddress;
	xRemoteAddress.sin_port = FreeRTOS_htons(TCP_SERVER_PORT);
	xRemoteAddress.sin_addr = FreeRTOS_inet_addr_quick(TCP_SERVER_ADDR1, TCP_SERVER_ADDR2, TCP_SERVER_ADDR3, TCP_SERVER_ADDR4);

	FreeRTOS_printf(("(%d) a socket is configured\n", num));

	BaseType_t status = FreeRTOS_connect(xSocket, &xRemoteAddress, sizeof(xRemoteAddress));
	if (status == 0)
	{
		FreeRTOS_printf(("(%d) sending data\n", num));

		BaseType_t xAlreadyTransmitted = 0, xBytesSent = 0;

		while (xAlreadyTransmitted < xTotalLengthToSend)
		{
			size_t xLenToSend = xTotalLengthToSend - xAlreadyTransmitted;
			xBytesSent = FreeRTOS_send(xSocket, &(pcBufferToTransmit[xAlreadyTransmitted]), xLenToSend, 0);

			if (xBytesSent > 0) // or >=
			{
				xAlreadyTransmitted += xBytesSent;
				FreeRTOS_printf(("(%d) sending data: %d / %d \n", num, xAlreadyTransmitted, xTotalLengthToSend));
			}
			else
			{
				break;
			}
		}
	}
	else {
		FreeRTOS_printf(("(%d) socket error: %d\n", num, status));
	}

	FreeRTOS_shutdown(xSocket, FREERTOS_SHUT_RDWR);

	if (status == 0)
	{
		FreeRTOS_printf(("(%d) receiving data\n", num));

		BaseType_t xAlreadyReceived = 0;
		while (1) {
			size_t xLenToRecv = xTotalLengthToSend - xAlreadyReceived;
			BaseType_t xBytesRecv = FreeRTOS_recv(xSocket, &(pcBufferToTransmit[xAlreadyReceived]), xLenToRecv, 0);
			if (xBytesRecv > 0) // or >=
			{
				xAlreadyReceived += xBytesRecv;
				FreeRTOS_printf(("(%d) receiving data: %d \n", num, xAlreadyReceived));
			}
			else
			{
				break;
			}
		}

		if (xAlreadyReceived > 0)
		{
			if (pcBufferToTransmit[0]) {
				FreeRTOS_printf(("(%d) a trasmission is successful.\n", num));
			}
			else
			{
				FreeRTOS_printf(("(%d) a trasmission is failed.\n", num));
			}
		}
	}

	FreeRTOS_closesocket(xSocket);

	FreeRTOS_printf(("(%d) ending transmission\n", num));
}


/*-----------------------------------------------------------*/

void vApplicationIdleHook( void )
{
	const uint32_t ulMSToSleep = 1;

	/* This is just a trivial example of an idle hook.  It is called on each
	cycle of the idle task if configUSE_IDLE_HOOK is set to 1 in
	FreeRTOSConfig.h.  It must *NOT* attempt to block.  In this case the
	idle task just sleeps to lower the CPU usage. */
	Sleep( ulMSToSleep );
}
/*-----------------------------------------------------------*/

void vAssertCalled( const char *pcFile, uint32_t ulLine )
{
const uint32_t ulLongSleep = 1000UL;
volatile uint32_t ulBlockVariable = 0UL;
volatile char *pcFileName = ( volatile char *  ) pcFile;
volatile uint32_t ulLineNumber = ulLine;

	( void ) pcFileName;
	( void ) ulLineNumber;

	FreeRTOS_printf( ( "vAssertCalled( %s, %ld\n", pcFile, ulLine ) );

	/* Setting ulBlockVariable to a non-zero value in the debugger will allow
	this function to be exited. */
	taskDISABLE_INTERRUPTS();
	{
		while( ulBlockVariable == 0UL )
		{
			Sleep( ulLongSleep );
		}
	}
	taskENABLE_INTERRUPTS();
}
/*-----------------------------------------------------------*/

/* Called by FreeRTOS+TCP when the network connects or disconnects.  Disconnect
events are only received if implemented in the MAC driver. */
void vApplicationIPNetworkEventHook( eIPCallbackEvent_t eNetworkEvent )
{
	uint32_t ulIPAddress, ulNetMask, ulGatewayAddress, ulDNSServerAddress;
	char cBuffer[ 16 ];
	static BaseType_t xTasksAlreadyCreated = pdFALSE;

	/* If the network has just come up...*/
	if( eNetworkEvent == eNetworkUp )
	{
		/* Create the tasks that use the IP stack if they have not already been
		created. */
		if( xTasksAlreadyCreated == pdFALSE )
		{
            /* See the comments above the definitions of these pre-processor
            macros at the top of this file for a description of the individual
            demo tasks. */
            /* Let the client work task run now */
            xTaskNotifyGive(xTCPClientTaskHandle);

			xTasksAlreadyCreated = pdTRUE;
		}

		/* Print out the network configuration, which may have come from a DHCP
		server. */
		FreeRTOS_GetAddressConfiguration( &ulIPAddress, &ulNetMask, &ulGatewayAddress, &ulDNSServerAddress );
		FreeRTOS_inet_ntoa( ulIPAddress, cBuffer );
		FreeRTOS_printf( ( "\r\n\r\nIP Address: %s\r\n", cBuffer ) );

		FreeRTOS_inet_ntoa( ulNetMask, cBuffer );
		FreeRTOS_printf( ( "Subnet Mask: %s\r\n", cBuffer ) );

		FreeRTOS_inet_ntoa( ulGatewayAddress, cBuffer );
		FreeRTOS_printf( ( "Gateway Address: %s\r\n", cBuffer ) );

		FreeRTOS_inet_ntoa( ulDNSServerAddress, cBuffer );
		FreeRTOS_printf( ( "DNS Server Address: %s\r\n\r\n\r\n", cBuffer ) );
	}
}
/*-----------------------------------------------------------*/

/* Called automatically when a reply to an outgoing ping is received. */
void vApplicationPingReplyHook( ePingReplyStatus_t eStatus, uint16_t usIdentifier )
{
static const char *pcSuccess = "Ping reply received - ";
static const char *pcInvalidChecksum = "Ping reply received with invalid checksum - ";
static const char *pcInvalidData = "Ping reply received with invalid data - ";

	switch( eStatus )
	{
		case eSuccess	:
			FreeRTOS_printf( ( pcSuccess ) );
			break;

		case eInvalidChecksum :
			FreeRTOS_printf( ( pcInvalidChecksum ) );
			break;

		case eInvalidData :
			FreeRTOS_printf( ( pcInvalidData ) );
			break;

		default :
			/* It is not possible to get here as all enums have their own
			case. */
			break;
	}

	FreeRTOS_printf( ( "identifier %d\r\n", ( int ) usIdentifier ) );

	/* Prevent compiler warnings in case FreeRTOS_debug_printf() is not defined. */
	( void ) usIdentifier;
}
/*-----------------------------------------------------------*/

void vApplicationMallocFailedHook( void )
{
	/* Called if a call to pvPortMalloc() fails because there is insufficient
	free memory available in the FreeRTOS heap.  pvPortMalloc() is called
	internally by FreeRTOS API functions that create tasks, queues, software
	timers, and semaphores.  The size of the FreeRTOS heap is set by the
	configTOTAL_HEAP_SIZE configuration constant in FreeRTOSConfig.h. */
	vAssertCalled( __FILE__, __LINE__ );
}
/*-----------------------------------------------------------*/

static void prvCheckTimerCallback( TimerHandle_t xTimer )
{
static volatile uint32_t ulEchoClientErrors_Single = 0, ulEchoClientErrors_Separate = 0, ulEchoServerErrors = 0, ulUDPEchoClientErrors = 0, ulUDPSelectServerErrors = 0;

	( void ) xTimer;

	/* Not all the demo tasks contain a check function yet - although an
	assert() will be triggered if a task fails. */

	#if( mainCREATE_TCP_ECHO_TASKS_SINGLE == 1 )
	{
		if( xAreSingleTaskTCPEchoClientsStillRunning() != pdPASS )
		{
			ulEchoClientErrors_Single++;
		}
	}
	#endif

	#if( mainCREATE_TCP_ECHO_TASKS_SEPARATE == 1 )
	{
		if( xAreSeparateTaskTCPEchoClientsStillRunning() != pdPASS )
		{
 			ulEchoClientErrors_Separate++;
		}
	}
	#endif

	#if( mainCREATE_SIMPLE_TCP_ECHO_SERVER == 1 )
	{
		if( xAreTCPEchoServersStillRunning() != pdPASS )
		{
			ulEchoServerErrors++;
		}
	}
	#endif

	#if( mainCREATE_UDP_ECHO_TASKS == 1 )
	{
		if( xAreUDPEchoClientsStillRunning() != pdPASS )
		{
			ulUDPEchoClientErrors++;
		}
	}
	#endif

	#if( mainCREATE_SELECT_UDP_SERVER_TASKS == 1 )
	{
		if( xAreUDPSelectTasksStillRunning() != pdPASS )
		{
			ulUDPSelectServerErrors++;
		}
	}
	#endif
}
/*-----------------------------------------------------------*/

UBaseType_t uxRand( void )
{
const uint32_t ulMultiplier = 0x015a4e35UL, ulIncrement = 1UL;

	/* Utility function to generate a pseudo random number. */

	ulNextRand = ( ulMultiplier * ulNextRand ) + ulIncrement;
	return( ( int ) ( ulNextRand >> 16UL ) & 0x7fffUL );
}
/*-----------------------------------------------------------*/

static void prvSRand( UBaseType_t ulSeed )
{
	/* Utility function to seed the pseudo random number generator. */
    ulNextRand = ulSeed;
}
/*-----------------------------------------------------------*/

static void prvMiscInitialisation( void )
{
time_t xTimeNow;
uint32_t ulLoggingIPAddress;

	ulLoggingIPAddress = FreeRTOS_inet_addr_quick( configECHO_SERVER_ADDR0, configECHO_SERVER_ADDR1, configECHO_SERVER_ADDR2, configECHO_SERVER_ADDR3 );
	vLoggingInit( mainLOG_TO_STDOUT, mainLOG_TO_DISK_FILE, mainLOG_TO_UDP, ulLoggingIPAddress, configPRINT_PORT );

	/* Seed the random number generator. */
	time( &xTimeNow );
	FreeRTOS_debug_printf( ( "Seed for randomiser: %lu\n", xTimeNow ) );
	prvSRand( ( uint32_t ) xTimeNow );
	FreeRTOS_debug_printf( ( "Random numbers: %08X %08X %08X %08X\n", ipconfigRAND32(), ipconfigRAND32(), ipconfigRAND32(), ipconfigRAND32() ) );
}
/*-----------------------------------------------------------*/

struct tm *gmtime_r( const time_t *pxTime, struct tm *tmStruct )
{
	/* Dummy time functions to keep the file system happy in the absence of
	target support. */
	memcpy( tmStruct, gmtime( pxTime ), sizeof( * tmStruct ) );
	return tmStruct;
}
/*-----------------------------------------------------------*/

time_t FreeRTOS_time( time_t *pxTime )
{
time_t xReturn;

	xReturn = time( &xReturn );

	if( pxTime != NULL )
	{
		*pxTime = xReturn;
	}

	return xReturn;
}
/*-----------------------------------------------------------*/

#if( ipconfigUSE_LLMNR != 0 ) || ( ipconfigUSE_NBNS != 0 ) || ( ipconfigDHCP_REGISTER_HOSTNAME != 0 )

	const char *pcApplicationHostnameHook( void )
	{
		/* Assign the name "FreeRTOS" to this network node.  This function will
		be called during the DHCP: the machine will be registered with an IP
		address plus this name. */
		return mainHOST_NAME;
	}

#endif
/*-----------------------------------------------------------*/

#if( ipconfigUSE_LLMNR != 0 ) || ( ipconfigUSE_NBNS != 0 )

	BaseType_t xApplicationDNSQueryHook( const char *pcName )
	{
	BaseType_t xReturn;

		/* Determine if a name lookup is for this node.  Two names are given
		to this node: that returned by pcApplicationHostnameHook() and that set
		by mainDEVICE_NICK_NAME. */
		if( FF_stricmp( pcName, pcApplicationHostnameHook() ) == 0 )
		{
			xReturn = pdPASS;
		}
		else if( FF_stricmp( pcName, mainDEVICE_NICK_NAME ) == 0 )
		{
			xReturn = pdPASS;
		}
		else
		{
			xReturn = pdFAIL;
		}

		return xReturn;
	}

#endif
/*-----------------------------------------------------------*/
