// Project 02
//PIC32 Server - Microchip BSD stack socket API
//MPLAB X C32 Compiler     PIC32MX795F512L
//      Microchip DM320004 Ethernet Starter Board

// ECE4532    Dennis Silage, PhD
//main.c
// Edited by: Robert Irwin
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
#include <math.h>

#include <plib.h>// PIC32 Peripheral library functions and macros
#include "tcpip_bsd_config.h"// in \source
#include <TCPIP-BSD\tcpip_bsd.h>

#include "hardware_profile.h"
#include "system_services.h"
#include "display_services.h"

#include "mstimer.h"

#define PC_SERVER_IP_ADDR "192.168.2.105"  // check ipconfig for IP address
#define SYS_FREQ (80000000)
#define BM 8.0
#define DEN 15.0
#define BD 12.0
void DelayMsec(unsigned int);

int main()
{
  // define the message
  char msg[] = "TCP/IP (Transmission Control Protocol/Internet Protocol) is the basic communication language or protocol of the Internet. It can also be used as a communications protocol in a private network (either an intranet or an extranet). When you are set up with direct access to the Internet, your computer is provided with a copy of the TCP/IP program just as every other computer that you may send messages to or get information from also has a copy of TCP/IP. TCP/IP is a two-layer program. The higher layer, Transmission Control Protocol, manages the assembling of a message or file into smaller packets that are transmitted over the Internet and received by a TCP layer that reassembles the packets into the original message. The lower layer, Internet Protocol, handles the address part of each packet so that it gets to the right destination. Each gateway computer on the network checks this address to see where to forward the message. Even though some packets from the same message are routed differently than others, they'll be reassembled at the destination.";
  //char msg[] = "TCP/IP Transmission Control ProtocolTCP/IP Transmission Control Protocol";
  int rlen, tlen, i, k, n;
  SOCKET srvr, StreamSock = INVALID_SOCKET;
  IP_ADDRcurr_ip;
  static BYTErbfr[10];// receive data buffer
  tlen = strlen(msg);
  char tbfr[tlen+1];// transmit data buffer
  struct sockaddr_in addr;
  int addrlen = sizeof(struct sockaddr_in);
  unsigned int sys_clk, pb_clk;
  //null terminate the string

            
  //Assemble the buffer
  strcpy(tbfr, msg);
    
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
                        
	      setsockopt(StreamSock, SOL_SOCKET, TCP_NODELAY, (char*)&tlen, sizeof(int));
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
		      send(StreamSock, tbfr, tlen, 0 );
		    }
                                

		  if(rbfr[1]==84)//T transfer
		    {
                               
		      //define tlen1 and tlen2
		      int x, tlen1, tlen2;
                                
		      x = tlen * (BM / DEN);
		      //round the length to the nearest whole integer 
		      tlen1 = floor(x + 0.5);
		      tlen2 = tlen - tlen1;
                                    
		      //initialize the two buffers
		      char tbfr1[tlen1+1]; // tlen chars plus a NULL
		      char tbfr2[tlen2+1];
                                
		      mPORTDSetBits(BIT_2);   // LED3=1
		      i=0;
		      k = 0;
		      while (i < tlen){
			if (i < tlen1){
			  tbfr1[i]=msg[i];
			  i++;
			}
			else{
			  tbfr2[k]=msg[i];
			  i++;
			  k++;
			}
		      }
		      strcat(tbfr1, "\0");
		      strcat(tbfr2, "\0");
		      send(StreamSock,tbfr1,tlen1+1,0 ); //ensure transmission of null
		      for (i=0; i < 60; i++){
			DelayMsec(60000);
		      }
		      send(StreamSock,tbfr2,tlen2+1,0 );
		      mPORTDClearBits(BIT_2);// LED3=0
                                    
		    }
                                
		  mPORTDClearBits(BIT_0); // LED1=0
                                                           
                            
		  if (rbfr[1] == 85){
		    //initialize the buffer lengths
		    int y, tlen3, tlen4, tlen5;
                                    
		    y = tlen*(BM/25);
		    tlen3 = floor(y+0.5);
		    y = tlen*(BD/100);
		    tlen4 = floor(y+0.5);
		    tlen5 = tlen-tlen3-tlen4;
                                    
		    //initialize the buffers
		    char tbfr3[tlen3+1]; //tlen3 chars plus a null
		    char tbfr4[tlen4+1];
		    char tbfr5[tlen5+1];
		    i = 0;
		    k = 0;
		    n = 0;
		    while (i < tlen){
		      if(i<tlen3){
			tbfr3[i]=msg[i];
			i++;
		      }
		      else if (i < (tlen3+tlen4)){
			tbfr4[k]=msg[i];
			k++;
			i++;
		      }
		      else{
			tbfr5[n]=msg[i];
			n++;
			i++;
		      }
		    }
		    strcat(tbfr3,"\0");
		    strcat(tbfr4,"\0");
		    strcat(tbfr5, "\0");
		    send(StreamSock,tbfr3,tlen3+1,0 );
		    DelayMsec(60);
		    send(StreamSock,tbfr4,tlen4+1,0 );
		    DelayMsec(60);
		    send(StreamSock,tbfr5,tlen5+1,0 );
		  }
                                
		  if (rbfr[1] == 86){
		    int buf_size;
		    //initialize the bytes sent variable
		    int bytes_sent = 50;
		    int max_buf_size = 50;
		    //start assembling the packets
		    long int q = 0;
                                    
		    int check;
		    while(bytes_sent == max_buf_size){
		      i = 0;
		      char *buff;
		      //check to see if we will fill the buffer
		      check = tlen-q;
		      if (check > max_buf_size){
			buff = (char *)calloc(max_buf_size+1, sizeof(char));
		      }
		      else{
			buff = (char *)calloc(check+1, sizeof(char));
		      }
		      while (i < max_buf_size-1){ //leave room for null termination
			if (q < tlen){
			  buff[i] = msg[q]; 
			  i++;
			  q++;                                             
			}
			else break;
		      }
		      //null terminate the string
		      strcat(buff, "\0");
		      buf_size = strlen(buff)+1;
		      bytes_sent = send(StreamSock,buff,buf_size,0);
		      free(buff);
		    }
                                   
		  }
                                 
		}
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
