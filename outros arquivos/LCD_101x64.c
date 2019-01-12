//---------------------------------------------------------------------------------
// Displaytest LCD Siemens A70 (3.3V 101x64 sw-Pixel / Controller: ?? PCF8812 ??)
//
// getestet mit AT90S2313 14.318180 MHz
//
// to do: auf Speed optimieren, Font, Line, etc.
//---------------------------------------------------------------------------------

#include "LCD_101x64.h"
//---------------------------------------------------------------------------------

#define Data_HI PORTD |=  (1<<PORTD4);        // Data=1;
#define Data_LO PORTD &= ~(1<<PORTD4);        // Data=0;

#define Clock_HI PORTB |=  (1<<PORTB2);       // Clock=1;
#define Clock_LO PORTB &= ~(1<<PORTB2);       // Clock=0;  

#define Control_HI PORTB |=  (1<<PORTB1);     // Control=1; CD comand / data
#define Control_LO PORTB &= ~(1<<PORTB1);     // Control=0; 

#define Reset_HI PORTB |=  (1<<PORTB0);       // Reset=1;
#define Reset_LO PORTB &= ~(1<<PORTB0);       // Reset=0;  

#define ChipSelect_HI PORTD |=  (1<<PORTD6);  // ChipSelect=1; CE
#define ChipSelect_LO PORTD &= ~(1<<PORTD6);  // ChipSelect=0;

//---------------------------------------------------------------------------------

// ------------------------------------------------------------------
//
// ------------------------------------------------------------------
void Reset_LCD(void)
{
  _delay_us(10);
  Data_LO;
  Clock_LO;
  Control_LO;
  _delay_us(10);
  Reset_HI;
  _delay_us(10);
  ChipSelect_LO;
  _delay_us(10);
  Reset_LO;
  _delay_us(10);  
  Reset_HI;
  ChipSelect_HI;
}


// ------------------------------------------------------------------
// command to display
// ------------------------------------------------------------------
void SendCommand(BYTE Command)
{
BYTE i,s;

  s=0x80;

  Data_LO;
  Clock_LO;
  Control_LO;
  ChipSelect_LO;
  _delay_us(10);
  for (i=0; i<8; i++) 
  {
    if (Command & s) { Data_HI; } 
    else             { Data_LO; }
    s = s>>1;
    _delay_us(10);
    Clock_HI;
    _delay_us(10);
    Clock_LO;
    _delay_us(10);
  }
  ChipSelect_HI;
}


// ------------------------------------------------------------------
//
// ------------------------------------------------------------------
void Init_LCD(void)
{

  Reset_LCD();

  SendCommand(0b00100001);    // Function set: chip is active, horizontal addressing, use extended instruction set
  SendCommand(0b11011010);    // 
  SendCommand(0b00010100);    // Display control: ?    
  SendCommand(0b00001011);    // Set voltage multiplier factor -> ok
  SendCommand(0b00000101);    // Temperature controlVLCD temperature coefficient 1
  SendCommand((128+80));      // contrast
  SendCommand(0b00100000);    // Function set
  SendCommand(0b00010001);    // VLCD programming range: high
  SendCommand(0b00001100);    // Display control: normal 
//    SendCommand(0b00001101);    // Display control: inverse -> ok
}   


// ------------------------------------------------------------------
// Line: 0..7, Row 0..100;
// ------------------------------------------------------------------
void GotoLineRow(BYTE Line, BYTE Row)
{
  SendCommand( 0b00101000);
  SendCommand((0b01000000 + Line));
  SendCommand((0b10000000 + Row ));
}


// ------------------------------------------------------------------
// data to display
// ------------------------------------------------------------------
void SendData(BYTE Value)
{          
  BYTE i,s;

  s=0x80;
  Data_LO;
  Clock_LO;
  Control_HI;
  ChipSelect_LO;

  _delay_us(10);
  for (i=0; i<8; i++) 
  {
    if (Value & s) { Data_HI; } 
    else           { Data_LO; }
    s = s>>1;
    _delay_us(10);
    Clock_HI;
    _delay_us(10);
    Clock_LO;
    _delay_us(10);
  }
  ChipSelect_HI;
}


// ------------------------------------------------------------------
