/*
 * FreeRTOS+TCP Labs Build 160919 (C) 2016 Real Time Engineers ltd.
 * Authors include Hein Tibosch and Richard Barry
 *
 *******************************************************************************
 ***** NOTE ******* NOTE ******* NOTE ******* NOTE ******* NOTE ******* NOTE ***
 ***                                                                         ***
 ***                                                                         ***
 ***   FREERTOS+TCP IS STILL IN THE LAB (mainly because the FTP and HTTP     ***
 ***   demos have a dependency on FreeRTOS+FAT, which is only in the Labs    ***
 ***   download):                                                            ***
 ***                                                                         ***
 ***   FreeRTOS+TCP is functional and has been used in commercial products   ***
 ***   for some time.  Be aware however that we are still refining its       ***
 ***   design, the source code does not yet quite conform to the strict      ***
 ***   coding and style standards mandated by Real Time Engineers ltd., and  ***
 ***   the documentation and testing is not necessarily complete.            ***
 ***                                                                         ***
 ***   PLEASE REPORT EXPERIENCES USING THE SUPPORT RESOURCES FOUND ON THE    ***
 ***   URL: http://www.FreeRTOS.org/contact  Active early adopters may, at   ***
 ***   the sole discretion of Real Time Engineers Ltd., be offered versions  ***
 ***   under a license other than that described below.                      ***
 ***                                                                         ***
 ***                                                                         ***
 ***** NOTE ******* NOTE ******* NOTE ******* NOTE ******* NOTE ******* NOTE ***
 *******************************************************************************
 *
 * FreeRTOS+TCP can be used under two different free open source licenses.  The
 * license that applies is dependent on the processor on which FreeRTOS+TCP is
 * executed, as follows:
 *
 * If FreeRTOS+TCP is executed on one of the processors listed under the Special
 * License Arrangements heading of the FreeRTOS+TCP license information web
 * page, then it can be used under the terms of the FreeRTOS Open Source
 * License.  If FreeRTOS+TCP is used on any other processor, then it can be used
 * under the terms of the GNU General Public License V2.  Links to the relevant
 * licenses follow:
 *
 * The FreeRTOS+TCP License Information Page: http://www.FreeRTOS.org/tcp_license
 * The FreeRTOS Open Source License: http://www.FreeRTOS.org/license
 * The GNU General Public License Version 2: http://www.FreeRTOS.org/gpl-2.0.txt
 *
 * FreeRTOS+TCP is distributed in the hope that it will be useful.  You cannot
 * use FreeRTOS+TCP unless you agree that you use the software 'as is'.
 * FreeRTOS+TCP is provided WITHOUT ANY WARRANTY; without even the implied
 * warranties of NON-INFRINGEMENT, MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. Real Time Engineers Ltd. disclaims all conditions and terms, be they
 * implied, expressed, or statutory.
 *
 * 1 tab == 4 spaces!
 *
 * http://www.FreeRTOS.org
 * http://www.FreeRTOS.org/plus
 * http://www.FreeRTOS.org/labs
 *
 */


 /* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* FreeRTOS includes. */
#include "FreeRTOS.h"
#include "task.h"

/* FreeRTOS+TCP includes. */
#include "FreeRTOS_IP.h"
#include "FreeRTOS_Sockets.h"
#include "FreeRTOS_TCP_server.h"
#include "FreeRTOS_server_private.h"
#include "FreeRTOS_FTP_Client.h"


typedef enum
{
    eINIT_CLIENT,
    eCONNECT_SERVER,
    eAUTHEN_SERVER,
    eCREATDIR_SERVER,
    eUPLOAD_SERVER,
    eFAILED_SERVER,
    eDEINIT_CLIENT,
} eFPTClientState_t;

static Socket_t m_xCmdSocket, m_xDataSocket;
static eFPTClientState_t fptClientState = eINIT_CLIENT;
static char fptClientRespCmdBuffer[512];

static BaseType_t FreeRTOS_FPTClientSendCmd(Socket_t clientSocket, const char *cmd);
static BaseType_t FreeRTOS_FPTClientSendData(Socket_t clientSocket, const uint8_t *data, long int len);
static int fptClientRevResponse(char* respBuffer, int len);
static void FreeRTOS_FPTClientSocketClose(Socket_t clientSocket);
static BaseType_t FreeRTOS_FPTClientSocketOpen(Socket_t * pSocket);
static BaseType_t fptClientAuthen(char* username, char* passWord);
static BaseType_t fptCreateDirectory(char* dirName);
static BaseType_t ftpClientReqPasvMode(struct freertos_sockaddr* uploadSockAddr);
static BaseType_t fptClientUploadFile(char* fileName, uint8_t *data, long int len);



static void FreeRTOS_FPTClientSocketClose(Socket_t clientSocket)
{
    char dummyChar;

    /* Initiate graceful shutdown. */
    FreeRTOS_shutdown(clientSocket, FREERTOS_SHUT_RDWR);

    /* Wait for the socket to disconnect gracefully (indicated by FreeRTOS_recv()
    returning a FREERTOS_EINVAL error) before closing the socket. */
    while (FreeRTOS_recv(clientSocket, &dummyChar, 1, 0) >= 0)
    {
        /* Wait for shutdown to complete.  If a receive block time is used then
        this delay will not be necessary as FreeRTOS_recv() will place the RTOS task
        into the Blocked state anyway. */
        vTaskDelay(pdMS_TO_TICKS(20));

        /* Note – real applications should implement a timeout here, not just
        loop forever. */
    }
}

static BaseType_t FreeRTOS_FPTClientSocketOpen(Socket_t *pSocket)
{
    TickType_t xTimeOut = pdMS_TO_TICKS(2000);
    socklen_t xSize = sizeof(struct freertos_sockaddr);

    *pSocket = FreeRTOS_socket(FREERTOS_AF_INET, FREERTOS_SOCK_STREAM, FREERTOS_IPPROTO_TCP);

    if (*pSocket != FREERTOS_NO_SOCKET)
    {
        #if( ipconfigFTP_TX_BUFSIZE > 0 )
        {
            WinProperties_t xWinProps;
            /* Fill in the buffer and window sizes that will be used by the
            socket. */
            xWinProps.lTxBufSize = ipconfigFTP_TX_BUFSIZE;
            xWinProps.lTxWinSize = ipconfigFTP_TX_WINSIZE;
            xWinProps.lRxBufSize = ipconfigFTP_RX_BUFSIZE;
            xWinProps.lRxWinSize = ipconfigFTP_RX_WINSIZE;

            /* Set the window and buffer sizes. */
            FreeRTOS_setsockopt( *pSocket, 0, FREERTOS_SO_WIN_PROPERTIES, ( void * ) &xWinProps, sizeof( xWinProps ) );
        }
        #endif

        FreeRTOS_setsockopt((*pSocket), 0, FREERTOS_SO_RCVTIMEO, (void*)&xTimeOut, sizeof(BaseType_t));
        FreeRTOS_setsockopt((*pSocket), 0, FREERTOS_SO_SNDTIMEO, (void*)&xTimeOut, sizeof(BaseType_t));
        FreeRTOS_bind((*pSocket), NULL, sizeof(xSize));

        return pdPASS;
    }
    else
    {
        FreeRTOS_printf(("Open Socket Failed\r\n"));
        return pdFAIL;
    }
}

static BaseType_t FreeRTOS_FPTClientSendCmd(Socket_t clientSocket, const char* cmd)
{
    size_t xAlreadyTransmitted = 0, xBytesSent = 0;
    size_t xLenToSend;
    size_t transmitLen;
    BaseType_t retVal = pdFAIL;
    FreeRTOS_printf(("command to send: %s", cmd));
    transmitLen = strlen(cmd);

    /* Keep sending until the entire buffer has been sent. */
    while (xAlreadyTransmitted < transmitLen)
    {
        /* How many bytes are left to send? */
        xLenToSend = transmitLen - xAlreadyTransmitted;
        xBytesSent = FreeRTOS_send(clientSocket, &cmd[xAlreadyTransmitted], xLenToSend, 0);

        if (xBytesSent >= 0)
        {
            /* Data was sent successfully. */
            xAlreadyTransmitted += xBytesSent;
        }
        else
        {
            break;
        }
    }

    if (xAlreadyTransmitted == transmitLen)
    {
        retVal = pdPASS;
    }
    else
    {
        retVal = pdFAIL;
    }
    return retVal;
}

static BaseType_t fptClientAuthen(char* username, char* passWord)
{
    BaseType_t retVal = pdFAIL;
    char tempBuffer[50];

    sprintf(tempBuffer, "USER %s\r\n", username);
    retVal = FreeRTOS_FPTClientSendCmd(m_xCmdSocket, tempBuffer);

    if (retVal != pdPASS)
    {
        return pdFAIL;
    }

    if (fptClientRevResponse(fptClientRespCmdBuffer, 512) != 331)
    {
        return pdFAIL;
    }

    sprintf(tempBuffer, "PASS %s\r\n", passWord);
    retVal = FreeRTOS_FPTClientSendCmd(m_xCmdSocket, tempBuffer);

    if (retVal != pdPASS)
    {
        return pdFAIL;
    }

    if (fptClientRevResponse(fptClientRespCmdBuffer, 512) != 230)
    {
        return pdFAIL;
    }

    return pdPASS;
}

static BaseType_t fptCreateDirectory(char* dirName)
{
    BaseType_t retVal = pdFAIL;
    char tempBuffer[40];
    retVal = FreeRTOS_FPTClientSendCmd(m_xCmdSocket, "CWD ram\r\n");

    if (retVal != pdPASS)
    {
        return pdFAIL;
    }

    if (fptClientRevResponse(fptClientRespCmdBuffer, 512) != 250)
    {
        return pdFAIL;
    }

    sprintf(tempBuffer, "MKD %s\r\n", dirName);
    retVal = FreeRTOS_FPTClientSendCmd(m_xCmdSocket, tempBuffer);

    if (retVal != pdPASS)
    {
        return pdFAIL;
    }

    if (fptClientRevResponse(fptClientRespCmdBuffer, 512) != 257)
    {
        return pdFAIL;
    }

    sprintf(tempBuffer, "CWD %s\r\n", dirName);

    retVal = FreeRTOS_FPTClientSendCmd(m_xCmdSocket, tempBuffer);

    if (retVal != pdPASS)
    {
        return pdFAIL;
    }

    if (fptClientRevResponse(fptClientRespCmdBuffer, 512) != 250)
    {
        return pdFAIL;
    }

    return pdPASS;
}

static int fptClientRevResponse(char* respBuffer, int len)
{
    size_t xBytesReceived = 0;
    char retResponse[5] = { 0 };
    int retVal = -1;

    xBytesReceived = FreeRTOS_recv(m_xCmdSocket, respBuffer, len, 0);
    if (xBytesReceived > 0)
    {
        FreeRTOS_printf(("Received Resp: %s\r\n", respBuffer));

        for (short int i = 0; i < 4; i++)
        {
            if (respBuffer[i] != ' ')
            {
                retResponse[i] = respBuffer[i];
            }
            else
            {
                retResponse[i] = 0;
                break;
            }
        }

        retVal = atoi(retResponse);
    }
    else if (xBytesReceived < 0)
    {
        FreeRTOS_printf(("recv respond error(ret=%d)!\r\n", xBytesReceived));
        retVal = -1;
    }

    return retVal;
}

static BaseType_t ftpClientReqPasvMode(struct freertos_sockaddr* uploadSockAddr)
{
    BaseType_t retVal = pdFAIL;
    char* find;
    int ipAddr[4] = { 0 };
    int portId[2] = { 0 };


    retVal = FreeRTOS_FPTClientSendCmd(m_xCmdSocket, "PASV\r\n");

    if (retVal != pdPASS)
    {
        return pdFAIL;
    }

    if (fptClientRevResponse(fptClientRespCmdBuffer, 512) != 227)
    {
        return pdFAIL;
    }

    find = strrchr(fptClientRespCmdBuffer, '(');
    (void)sscanf(find, "(%d,%d,%d,%d,%d,%d)", &ipAddr[0], &ipAddr[1], &ipAddr[2], &ipAddr[3], &portId[0], &portId[1]);
    FreeRTOS_printf(("PASV IPAddr: %d.%d.%d.%d\r\n", ipAddr[0], ipAddr[1], ipAddr[2], ipAddr[3]));

    uploadSockAddr->sin_port = FreeRTOS_htons(portId[0] * 256 + portId[1]);
    uploadSockAddr->sin_addr = FreeRTOS_inet_addr_quick(ipAddr[0], ipAddr[1], ipAddr[2], ipAddr[3]);

    return pdPASS;
}

static BaseType_t fptClientUploadFile(char* fileName, uint8_t * data, long int len)
{
    struct freertos_sockaddr uploadSockAddr = {0};

    BaseType_t retVal = pdFAIL;
    char tempBuffer[30] = { 0 };

    retVal = FreeRTOS_FPTClientSendCmd(m_xCmdSocket, "TYPE I\r\n");

    if (retVal != pdPASS)
    {
        return pdFAIL;
    }

    if (fptClientRevResponse(fptClientRespCmdBuffer, 512) != 200)
    {
        return pdFAIL;
    }

    retVal = ftpClientReqPasvMode(&uploadSockAddr);

    if (retVal != pdPASS)
    {
        return pdFAIL;
    }

    if (pdPASS != FreeRTOS_FPTClientSocketOpen(&m_xDataSocket))
    {
        return pdFAIL;
    }

    if (FreeRTOS_connect(m_xDataSocket, &uploadSockAddr, sizeof(uploadSockAddr)) == 0)
    {
        sprintf(tempBuffer, "STOR %s\r\n", fileName);
        retVal = FreeRTOS_FPTClientSendCmd(m_xCmdSocket, tempBuffer);

        if (retVal != pdPASS)
        {
            FreeRTOS_FPTClientSocketClose(m_xDataSocket);
            return pdFAIL;
        }

        retVal = FreeRTOS_FPTClientSendData(m_xDataSocket, data, len);

        if (retVal != pdPASS)
        {
            FreeRTOS_FPTClientSocketClose(m_xDataSocket);
            FreeRTOS_printf(("Send data error!"));
            return pdFAIL;
        }

        if (fptClientRevResponse(fptClientRespCmdBuffer, 512) != 150)
        {
            FreeRTOS_FPTClientSocketClose(m_xDataSocket);
            return pdFAIL;
        }

        /* The socket has shut down and is safe to close. */
        FreeRTOS_FPTClientSocketClose(m_xDataSocket);

        if (fptClientRevResponse(fptClientRespCmdBuffer, 512) != 226)
        {
            return pdFAIL;
        }

        return pdPASS;
    }
    else
    {
        return pdFAIL;
    }
}

static BaseType_t FreeRTOS_FPTClientSendData(Socket_t clientSocket, const uint8_t* data, long int len)
{
    int xAlreadyTransmitted = 0;
    size_t xBytesSent = 0;
    int xLenToSend = 0;
    BaseType_t retVal = pdFAIL;

    /* Keep sending until the entire buffer has been sent. */
    while (xAlreadyTransmitted < len)
    {
        /* How many bytes are left to send? */
        xLenToSend = len - xAlreadyTransmitted;
        xBytesSent = FreeRTOS_send(clientSocket, &data[xAlreadyTransmitted], xLenToSend, 0);

        if (xBytesSent >= 0)
        {
            /* Data was sent successfully. */
            xAlreadyTransmitted += xBytesSent;
        }
        else
        {
            break;
        }
    }

    if (xAlreadyTransmitted == len)
    {
        retVal = pdPASS;
    }
    else
    {
        retVal = pdFAIL;
    }

    return retVal;
}

void FreeRTOS_FTPClientWork(void)
{
    struct freertos_sockaddr xCmdSockAddr;

    switch (fptClientState)
    {
    case eINIT_CLIENT:
    {
        if (pdPASS != FreeRTOS_FPTClientSocketOpen(&m_xCmdSocket))
        {
            fptClientState = eDEINIT_CLIENT;
        }
        else
        {
            fptClientState = eCONNECT_SERVER;
        }
        break;
    }

    case eCONNECT_SERVER:
    {
        xCmdSockAddr.sin_port = FreeRTOS_htons(21);
        xCmdSockAddr.sin_addr = FreeRTOS_inet_addr_quick(192, 168, 0, 12);

        if (FreeRTOS_connect(m_xCmdSocket, &xCmdSockAddr, sizeof(xCmdSockAddr)) == 0)
        {
            /*  Wait server response */
            if (fptClientRevResponse(fptClientRespCmdBuffer, 512) != 220)
            {
                fptClientState = eFAILED_SERVER;
            }
            else
            {
                fptClientState = eAUTHEN_SERVER;
            }
        }
        else
        {
            fptClientState = eFAILED_SERVER;
        }
        break;
    }

    case eAUTHEN_SERVER:
    {
        if ((fptClientAuthen("anonymous", "anonymous@example.com") == pdFAIL))
        {
            FreeRTOS_closesocket(m_xCmdSocket);
            fptClientState = eFAILED_SERVER;
        }
        else
        {
            fptClientState = eCREATDIR_SERVER;
        }
        break;
    }

    case eCREATDIR_SERVER:
    {
        if (fptCreateDirectory("longtex") == pdFAIL)
        {
            FreeRTOS_closesocket(m_xCmdSocket);
            fptClientState = eFAILED_SERVER;
        }
        else
        {
            fptClientState = eUPLOAD_SERVER;
        }
        break;
    }

    case eUPLOAD_SERVER:
    {
        break;
    }

    case eFAILED_SERVER:
    {
        /* Server is not available, close socket gracefully */
        FreeRTOS_printf(("Server is not reachable, retry in 30 seconds\r\n"));
        FreeRTOS_FPTClientSocketClose(m_xCmdSocket);
        fptClientState = eDEINIT_CLIENT;
        break;
    }

    case eDEINIT_CLIENT:
    {
        vTaskDelay(pdMS_TO_TICKS(30000));
        /* wait for 30 seconds and retry */
        fptClientState = eINIT_CLIENT;
        break;
    }

    default:
        break;
    }
}

BaseType_t FreeRTOS_FTPClientUpload(char *filename, uint8_t *pData, int len)
{
    /* gets data from queue, increases file number, request upload */
    if (fptClientUploadFile(filename, pData, len) == pdFAIL)
    {
        FreeRTOS_closesocket(m_xCmdSocket);
        fptClientState = eFAILED_SERVER;
        return pdFAIL;
    }
    return pdPASS;
}

BaseType_t isClientReadyToUpload(void)
{
    if (fptClientState == eUPLOAD_SERVER)
    {
        return pdPASS;
    }
    else
    {
        return pdFAIL;
    }
}


