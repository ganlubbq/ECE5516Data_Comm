//PIC32 Server - Microchip BSD stack socket API
//MPLAB X C32 Compiler     PIC32MX795F512L
//      Microchip DM320004 Ethernet Starter Board

// ECE4532    Dennis Silage, PhD
//main.c
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
#define LEN 35
#define NUM_SIX_BYTE_SEQ 5
void DelayMsec(unsigned int);
char setParityBit(char);
char* calcParityMatrix(char*);
char bin2ascii(char*);
int calcParCol(char*);
int calcParRow(char);
char flipBit(char, int);
char* decodeBuf(char*);

int main(){
  char msg[LEN] ="abcdef ghijk lmnop qrstu vwxyz";
  int rlen, tlen, j;
  SOCKET srvr, StreamSock = INVALID_SOCKET;
  IP_ADDRcurr_ip;
  static BYTErbfr[LEN];// receive data buffer
  static BYTE tbfr[LEN];// transmit data buffer
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

  while(1){
    IP_ADDR ip;

    TCPIPProcess();
    DHCPTask();

    ip.Val = TCPIPGetIPAddr();
    if(curr_ip.Val != ip.Val)// DHCP server change IP address?
      curr_ip.Val = ip.Val;

    // TCP Server Code
    if(StreamSock == INVALID_SOCKET){
      StreamSock = accept(srvr, (struct sockaddr*)&addr, &addrlen );
      if(StreamSock != INVALID_SOCKET){
	tlen = 1296;// send buffer size
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
    else{
      // receive TCP data
      rlen = recvfrom(StreamSock, rbfr, sizeof(rbfr), 0, NULL, NULL);
      if(rlen > 0){
	//start decoding
	if (strlen(rbfr)==LEN){
	  if(rbfr[0]==2)
	    goto transfer;
	  int col = -1;
	  int row = -1;
	  int count = 0;
	  char tmp[LEN];
                    
	  memcpy(tmp,rbfr,LEN);
	  col=calcParCol(tmp);
                    
	  if(col==-1){ //if col == -1 after calcParCol, no errors were found
	    memcpy(tbfr,tmp,LEN);
	    send(StreamSock,tbfr,LEN,0);
	  }
                    
	  else{
	    row = calcParRow(tmp[col]);
	    tmp[col] = flipBit(tmp[row], col);
	    memcpy(tbfr,tmp,LEN);
	    memcpy(tbfr,decodeBuf(tbfr),LEN);
	    send(StreamSock, tbfr, LEN, 0);
	  }//end else
                    
	}//end decode
                
	if (rbfr[0]==2){// 02 start of message
	transfer: 
	  if(rbfr[1]==71){//G global reset
	    mPORTDSetBits(BIT_0);   // LED1=1
	    DelayMsec(50);
	    mPORTDClearBits(BIT_0); // LED1=0
	  }
                

	  if(rbfr[1]==84){ //Transfer
	    int len = strlen(msg);
	    int j = 0;
	    char byteWpar[6];
	    char par2[8];
	    char dim2par;
	    int i = 0;
	    int count;
	    count = 1;
	    for(j=0; j<len; j++){
	      //set parity bit in first dimension
	      byteWpar[i] = setParityBit(msg[j]);
	      i++;
	      if (i == 6){
		memcpy(par2,calcParityMatrix(byteWpar),8);
		dim2par = bin2ascii(par2);
		memcpy(tbfr+(j+count)-i,byteWpar, 6);
		tbfr[j+count]= dim2par;
		i = 0;
		count++;
	      }
	    }
	    send(StreamSock,tbfr,LEN, 0);

	  }        
	}
      }
      else if(rlen < 0){
	closesocket( StreamSock );
	StreamSock = SOCKET_ERROR;
      }   
    }// end else
  }   // end while(1)
}
// DelayMsec( )   software millisecond delay
void DelayMsec(unsigned int msec)
{
  unsigned int tWait, tStart;

  tWait=(SYS_FREQ/2000)*msec;
  tStart=ReadCoreTimer();
  while((ReadCoreTimer()-tStart)<tWait);
}

char setParityBit(char byte)
{
  char total = 0;
  char parity = 0;
  int i;
  for(i = 1; i<= 0x80; i<<=1){
    if(i & byte){
      total+=1;
    }    
  }
    
  if(total % 2) parity = 1;
    
  byte = byte<<1;
  byte = byte|parity;
    
  return byte;
}


char* calcParityMatrix(char* buf){
  int sum = 0;
  int i = 0;
  int j;
  int col = 0x80;
  char par2[8];
  for (j=0; j<8;j++){
    if(j != 0)
      col >>=1;
    for(i=0; i<6; i++ ){
      if(buf[i]&col)
	sum++;
    }
    if(sum%2)
      par2[j]=1;
    else
      par2[j]=0;
    sum=0;
  }
  return par2;
}


char bin2ascii(char byte[8]){
  char out = 0;
  int j = 7;
  int i;
  for (i=0; i < 8; i++){
    out = out|(byte[i]<<j);
    j = j-1;
  }
  return out;
}
//first check during decoding
int calcParCol(char* buf){
  int sum = 0;
  int col = -1;
  int curr = 0x80;
  int i = 0;
  int j = 0;
  int k = 0;
  char tmp[7];
    
  for(k=0; k<NUM_SIX_BYTE_SEQ ;k++){
    memcpy(tmp,buf+(7*k),7);

    //calculate parity in each column
    for (j=0; j<8;j++){
      if(j != 0)
	curr >>=1;
      for(i=0; i<7; i++ ){ //check seven bytes at a time
	if(tmp[i]&curr)
	  sum++;
      }
      if(sum%2){
	col=j; //corresponds to the column with the error
	return col;
      }
      sum=0;
    }
  }
  return col;
}

//if a column is found with odd parity, check the rows for odd parity
int calcParRow(char byte){
  int row = -1;
  int i;
  int total = 0;
    
  for(i = 1; i<= 0x80; i<<=1){
    if(i & byte){
      total+=1;
    }    
  }
  if (total%2) 
    row = 1;
  return row;
}

//this function flips the bit in error
char flipBit(char byteWerr, int colWerror){
  int i = 0x80;
    
  if (colWerror == 0){
    byteWerr=byteWerr^i;
    return byteWerr;
  }
    
  i>>=colWerror;
  byteWerr=byteWerr^i;
  return byteWerr;
}

char* decodeBuf(char buf[LEN]){
  int i;
  for (i=0; i<LEN; i++){
    buf[i]>>=1;
  }
  return buf;
}
