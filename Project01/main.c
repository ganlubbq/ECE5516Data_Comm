//PIC32 Server - Microchip BSD stack socket API
//MPLAB X C32 Compiler     PIC32MX795F512L
//      Microchip DM320004 Ethernet Starter Board

// ECE4532    Dennis Silage, PhD
//main.c
// edited by: Robert Irwin
//version 1-27-14

//      Starter Board Resources:
//LED1 (RED)RD0
//LED2 (YELLOW)RD1
//LED3 (GREEN)RD2
//SW1RD6
//SW2RD7
//SW3RD13

//Control Messages
//02 start of message
//03 end of message


#include <string.h>

#include <plib.h>// PIC32 Peripheral library functions and macros
#include "tcpip_bsd_config.h"// in \source
#include <TCPIP-BSD\tcpip_bsd.h>

#include "hardware_profile.h"
#include "system_services.h"
#include "display_services.h"

#include "mstimer.h"

#define PC_SERVER_IP_ADDR "192.168.2.105"  // check ipconfig for IP address
#define SYS_FREQ (80000000)

void DelayMsec(unsigned int);

int main()
{

  int rlen, tlen, i;
  SOCKET srvr, StreamSock = INVALID_SOCKET;
  IP_ADDRcurr_ip;
  static BYTErbfr[10];// receive data buffer
  static BYTE tbfr[1500];// transmit data buffer
  struct sockaddr_in addr;
  int addrlen = sizeof(struct sockaddr_in);
  unsigned int sys_clk, pb_clk;

  // LED setup
  mPORTDSetPinsDigitalOut(BIT_0 | BIT_1 | BIT_2 );// RD0, RD1 and RD2 as outputs
  mPORTDClearBits(BIT_0 | BIT_1 | BIT_2);
  // switch setup
  mPORTDSetPinsDigitalIn(BIT_6 | BIT_7 | BIT_13);     // RD6, RD7, RD13 as inputs

  // system clock
  sys_clk=GetSystemClock();
  pb_clk=SYSTEMConfigWaitStatesAndPB(sys_clk);
  // interrupts enabled
  INTEnableSystemMultiVectoredInt();
  // system clock enabled
  SystemTickInit(sys_clk, TICKS_PER_SECOND);

  // initialize TCP/IP
  TCPIPSetDefaultAddr(DEFAULT_IP_ADDR, DEFAULT_IP_MASK, DEFAULT_IP_GATEWAY,
		      DEFAULT_MAC_ADDR);
  if (!TCPIPInit(sys_clk))
    return -1;
  DHCPInit();

  // create TCP server socket
  if((srvr = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP )) == SOCKET_ERROR )
    return -1;
  // bind to a local port
  addr.sin_port = 6653;
  addr.sin_addr.S_un.S_addr = IP_ADDR_ANY;
  if( bind(srvr, (struct sockaddr*)&addr, addrlen ) == SOCKET_ERROR )
    return -1;
  listen(srvr, 5 );

  while(1)
    {
      IP_ADDR ip;

      TCPIPProcess();
      DHCPTask();

      ip.Val = TCPIPGetIPAddr();
      if(curr_ip.Val != ip.Val)// DHCP server change IP address?
	curr_ip.Val = ip.Val;

      // TCP Server Code
      if(StreamSock == INVALID_SOCKET)
	{
	  StreamSock = accept(srvr, (struct sockaddr*)&addr, &addrlen );
	  if(StreamSock != INVALID_SOCKET)
	    {
	      tlen = 1296;// send buffer size
	      setsockopt(StreamSock, SOL_SOCKET, SO_SNDBUF, (char*)&tlen, sizeof(int));
	      mPORTDSetBits(BIT_0);   // LED1=1
	      DelayMsec(50);
	      mPORTDClearBits(BIT_0); // LED1=0
	      mPORTDSetBits(BIT_1);   // LED2=1
	      DelayMsec(50);
	      mPORTDClearBits(BIT_1); // LED2=0
	      mPORTDSetBits(BIT_2);   // LED3=1
	      DelayMsec(50);
	      mPORTDClearBits(BIT_2); // LED3=0
	    }
	}
      else
	{
	  // receive TCP data
	  rlen = recvfrom(StreamSock, rbfr, sizeof(rbfr), 0, NULL, NULL);
	  if(rlen > 0)
	    {
	      if (rbfr[0]==2)// 02 start of message
		//                                mPORTDSetBits(BIT_0);// LED1=1
		{
		  if(rbfr[1]==71)//G global reset
		    {
		      mPORTDSetBits(BIT_0);   // LED1=1
		      DelayMsec(50);
		      mPORTDClearBits(BIT_0); // LED1=0
		    }
		}

	      if(rbfr[1]==84)//T transfer
		{
		  mPORTDSetBits(BIT_2);   // LED3=1
		  i=0;
		lpdat:  i=i+1;
		  tbfr[i]=i;   //LSByte;
		  i=i+1;
		  tbfr[i]=0;//MSByte;
		  if (i<1296)
		    goto lpdat;

		  send(StreamSock, tbfr, tlen, 0 );
		  DelayMsec(50);
		  mPORTDClearBits(BIT_2);// LED3=0
		}
	      mPORTDClearBits(BIT_0); // LED1=0
	    }

	  else if(rlen < 0)
	    {
	      closesocket( StreamSock );
	      StreamSock = SOCKET_ERROR;
	    }
	}

    }// end while(1)
}   // end

// DelayMsec( )   software millisecond delay
void DelayMsec(unsigned int msec)
{
  unsigned int tWait, tStart;

  tWait=(SYS_FREQ/2000)*msec;
  tStart=ReadCoreTimer();
  while((ReadCoreTimer()-tStart)<tWait);
}
