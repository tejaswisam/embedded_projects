#include "tm4c123gh6pm.h"
#include <stdint.h>
#include "PLL.h"
#include "string.h"

 char string[20]; 

// standard ASCII symbols
#define CR   0x0D  // carriage return (return to beginning of current line)
#define LF   0x0A  //line feed or newline character '\n'
#define BS   0x08  //backspace

//------------UART_Init------------
// Initialize the UART for 115,200 baud rate (assuming 80 MHz UART clock),
// 8 bit word length, no parity bits, one stop bit, FIFOs enabled
// Input: none
// Output: none
void UART_Init(void){
  SYSCTL_RCGC1_R |= 0x01; // activate UART0 Clock Gating Control
  SYSCTL_RCGC2_R |= 0x01; // activate port A Clock Gating Control
  UART0_CTL_R &= ~(0x01); // disable UART
  UART0_IBRD_R = 43;       // IBRD = int(80,000,000 / (16 * 115,200)) = int(43.40278)
  UART0_FBRD_R = 26;       // FBRD = int(0.40278 * 64 + 0.5) = 26                                     
  UART0_LCRH_R = 0x70;  // 8 bit word length, no parity bits, one stop bit, Enable FIFOs
  UART0_CTL_R |= 0x01;       // enable UART
  GPIO_PORTA_AFSEL_R |= 0x03;     // enable alt funct on PA1-0
  GPIO_PORTA_DEN_R |= 0x03;       // enable digital I/O on PA1-0
  GPIO_PORTA_PCTL_R = 0x00000011; // configure PA1-0 as UART 
  GPIO_PORTA_AMSEL_R &= ~0x03;          // disable analog functionality on PA
}

//------------UART_OutChar------------
// Output 8-bit to serial port
// Input: letter is an 8-bit ASCII character to be transferred
// Output: none
void UART_OutChar(unsigned char data){
  while((UART0_FR_R&0x20) != 0); // UART Transmit FIFO Full
  UART0_DR_R = data;
}


//------------UART_OutString------------
// Output String (NULL termination)
// Input: pointer to a NULL-terminated string to be transferred
// Output: none
void UART_OutString(char *pt){
  while(*pt){
    UART_OutChar(*pt);
    pt++;
  }
}



//------------UART_InChar------------
// Wait for new serial port input
// Input: none
// Output: ASCII code for key typed
unsigned char UART_InChar(void){
  while((UART0_FR_R&0x10) != 0); // UART Receive FIFO Empty
  return((unsigned char)(UART0_DR_R&0xFF));
}
//------------UART_InString------------
// Accepts ASCII characters from the serial port
//    and adds them to a string until <enter> is typed
//    or until max length of the string is reached.
// It echoes each character as it is inputted.
// If a backspace is inputted, the string is modified
//    and the backspace is echoed
// terminates the string with a null character
// uses busy-waiting synchronization on RDRF
// Input: pointer to empty buffer, size of buffer
// Output: Null terminated string
// -- Modified by Agustinus Darmawan + Mingjie Qiu --
void UART_InString(char *bufPt, unsigned short max) {
int length=0;
char character;
  character = UART_InChar();
  while(character != CR){
    if(character == BS){
      if(length){
        bufPt--;
        length--;
        UART_OutChar(BS);
      }
    }
    else if(length < max){
      *bufPt = character;
      bufPt++;
      length++;
      UART_OutChar(character);
    }
    character = UART_InChar();
  }
  *bufPt = 0;
}

void PortB_Init(void)
{
	unsigned long volatile delay;
	//PORT Initialization/
	SYSCTL_RCGC2_R |= 0x02; // Port B clock
	delay = SYSCTL_RCGC2_R; // wait 3-5 bus cycles
	GPIO_PORTB_DIR_R |= 0x11; // PB4 & PB0 output
	GPIO_PORTB_AFSEL_R &= ~0x11; // not alternative
	GPIO_PORTB_AMSEL_R &= ~0x11; // no analog
	GPIO_PORTB_PCTL_R &= ~0x000F000F; // bits for PB4 & PB0
	GPIO_PORTB_DEN_R |= 0x11; // enable PB4 & PB0
}


void PortF_Init(void)
{
	unsigned long delay;
	SYSCTL_RCGC2_R |= 0x20;// clock for Port F
	delay = SYSCTL_RCGC2_R;// wait 3-5 bus cycles 	
	GPIO_PORTF_LOCK_R = 0x4C4F434B;//unlock GPIOPortF
	GPIO_PORTF_CR_R = 0x1F; // allow changes to PF4-0
	// only PF0 to be unlocked, other bits can't be
	GPIO_PORTF_AMSEL_R = 0x00;// disable analog
	GPIO_PORTF_PCTL_R = 0x00;// bits for PF4-0
	GPIO_PORTF_DIR_R = 0x0E;// PF4,0 in, PF3,1 out
	GPIO_PORTF_AFSEL_R = 0x00;//disable alt func
	GPIO_PORTF_PUR_R = 0x11;// enable pullup on PF0,4 
	GPIO_PORTF_DEN_R = 0x1F;// enable digital I/O
}

void SysInit(void)
{
	NVIC_ST_CTRL_R = 0;
	NVIC_ST_CURRENT_R = 0;// any write to current clears it
	//NVIC_SYS_PRI3_R = NVIC_SYS_PRI3_R&0x00FFFFFF;// priority 0
	NVIC_ST_CTRL_R = 0x00000005;// enable with core clock and interrupts
}


void SysLoad(unsigned long period)
{
  NVIC_ST_RELOAD_R = period-1;  // number of counts to wait
  NVIC_ST_CURRENT_R = 0;       // any value written to CURRENT clears
  while((NVIC_ST_CTRL_R&0x00010000)==0)
  { // wait for count flag
  }	
}


int main(void)
{
	SysInit();
  	PLL_Init(); // set system clock to 50 MHz
	UART_Init();              // initialize UART
	PortB_Init();
	PortF_Init(); // initialize portF
           
  	while(1){
		UART_OutString("Enter speed and direction: "); //output data
    		UART_InString(string,19);   // input data
		
	 	if( strcmp(string,"50 c")== 0){ // if input data red
			while((UART0_FR_R&0x10) != 0){
				GPIO_PORTB_DATA_R |= (0x01); 
				SysLoad(400000);  // wait 5ms
				
				GPIO_PORTB_DATA_R &= ~(0x01); 
				SysLoad(400000);  // wait 5ms
				}
				UART_OutChar(LF);   // new line
				UART_OutString("Motor rotates clockwise with 50% duty cycle");} // output data message
		else if( strcmp(string,"75 c")== 0){
				while((UART0_FR_R&0x10) != 0){
				GPIO_PORTB_DATA_R |= (0x01); 
				SysLoad(600000);  // wait ms
			
				GPIO_PORTB_DATA_R &= ~(0x01); 
				SysLoad(200000);  // wait ms
				}
				UART_OutChar(LF);
				UART_OutString("Motor rotates clockwise with 75% duty cycle");}
		else if( strcmp(string,"90 c")== 0){
				while((UART0_FR_R&0x10) != 0){
				GPIO_PORTB_DATA_R |= (0x01); 
				SysLoad(720000);  // wait ms
				
				GPIO_PORTB_DATA_R &= ~(0x01); 
				SysLoad(80000);  // wait ms
				}
				UART_OutChar(LF);
				UART_OutString("Motor rotates clockwise with 90% duty cycle");}
		else if( strcmp(string,"50 cc")== 0){ // if input data red
				while((UART0_FR_R&0x10) != 0){
				GPIO_PORTB_DATA_R |= (0x10); 
				SysLoad(400000);  // wait 5ms
				
				GPIO_PORTB_DATA_R &= ~(0x10); 
				SysLoad(400000);  // wait 5ms
				}
				UART_OutChar(LF);   // new line
				UART_OutString("Motor rotates counterclockwise with 50% duty cycle");} // output data message
		else if( strcmp(string,"75 cc")== 0){
				while((UART0_FR_R&0x10) != 0){
				GPIO_PORTB_DATA_R |= (0x10); 
				SysLoad(600000);  // wait ms
			
				GPIO_PORTB_DATA_R &= ~(0x10); 
				SysLoad(200000);  // wait ms
				}
				UART_OutChar(LF);
				UART_OutString("Motor rotates counterclockwise with 75% duty cycle");}
		else if( strcmp(string,"90 cc")== 0){
				while((UART0_FR_R&0x10) != 0){
				GPIO_PORTB_DATA_R |= (0x10); 
				SysLoad(720000);  // wait ms
				
				GPIO_PORTB_DATA_R &= ~(0x10); 
				SysLoad(80000);  // wait ms
				}
				
				UART_OutChar(LF);
				UART_OutString("Motor rotates counterclockwise with 90% duty cycle");}
		else{
				GPIO_PORTB_DATA_R = (0x00);
				UART_OutChar(LF);
				UART_OutString("Motor stops");}

	UART_OutChar(LF);
  	}
  }
