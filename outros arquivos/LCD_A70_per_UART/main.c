//----------------------------------------------------------------------------------------
// rudimentäre Umsetzung einer Displayansteuerung über die UART
// bei Ansteuerung über die serielle PC-Schnittstelle Pegelwandler Max3232 o.ä. erforderlich
//
// Testumgebung ATmega168 + 14.318180 MHz 3.3V
// HTerm (19200 8N1)
//
// Kommandos: (Zeilenabschluß mit CR+LF (0x0D + 0x0A))
//
//    txt 1,1,"Hello World!"
//    cls
//    demo
//    time "12:59"
//    help
//    black
//
//----------------------------------------------------------------------------------------
#define XTAL F_CPU

#include <avr/sleep.h>
#include <compat/deprecated.h>
#include <avr/pgmspace.h>
#include <inttypes.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "LCD_A70_101x64.h"

//----------------------------------------------------------------------------------------
//  Pinbelegung:        mega 48, 88, 168, 328 (PDIP)
//
//                            ----v----
// (PCINT14/RESET)......PC6 -| 1     28|- PC5 (ADC5/SCL/PCINT13)
// (PCINT16/RXD)        PD0 -| 2     27|- PC4 (ADC4/SDA/PCINT12)
// (PCINT17/TXD)........PD1 -| 3     26|- PC3 (ADC3/PCINT11)
// (PCINT18/INT0)       PD2 -| 4     25|- PC2 (ADC2/PCINT10)
// (PCINT19/OC2B/INT1)..PD3 -| 5     24|- PC1 (ADC1/PCINT9)
// (PCINT20/XCK/T0)     PD4 -| 6     23|- PC0 (ADC0/PCINT8)
//                      Vcc -| 7     22|- Gnd
//                      Gnd -| 8     21|- Aref
// (PCINT6/XTAL1/TOSC1).PB6 -| 9     20|- AVcc
// (PCINT7/XTAL2/TOSC2) PB7 -|10     19|- PB5 (SCK/PCINT5)
// (PCINT21/OC0B/T1)....PD5 -|11     18|- PB4 (MISO/PCINT4)
// (PCINT22/OC0A/AIN0)  PD6 -|12     17|- PB3 (MOSI/OC2A/PCINT3)
// (PCINT23/AIN1).......PD7 -|13     16|- PB2 (SS/OC1B/PCINT2)
// (PCINT0/CLKO/ICP1)   PB0 -|14     15|- PB1 (OC1A/PCINT1)
//                            ---------
//
//----------------------------------------------------------------------------------------
#define BYTE  unsigned char
#define WORD  unsigned short
#define DWORD unsigned long
#define uchar unsigned char
#define uint unsigned int

//----------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------
#define max 40
#define BaudRate 19200UL
//----------------------------------------------------------------------------------------
#define	BAUD	BaudRate
#define bauddivider (uint)(XTAL / BAUD / 16 - 0.5)

//----------------------------------------------------------------------------------------
char puffer[max+1];

volatile char *schreibzeiger;
volatile char *lesezeiger;
volatile BYTE csum=0;
volatile BYTE cmdcnt=0;
volatile BYTE IdxLaenge;
volatile BYTE RXready;
volatile BYTE RXfull;
volatile int RXcnt;

BYTE hh,mm,ss;

//-------------------------------------------------------------------------------------------------
WORD freemilli=0;
volatile BYTE z001ms=0, z010ms=0, z100ms=0, z01sec=0, z10sec=0, buffer;
volatile BYTE Ovl10ms=0, Ovl100ms=0, Ovl1sec=0, Ovl10sec=0, Ovl1min=0;

//----------------------------------------------------------------------------------------
void InitUart(uint32_t baudrate);
void uart_puts (char *s);
void sendc(BYTE aa);

//-------------------------------------------------------------------------------------------------
void Timer1_init(void);
void delay_10ms(WORD t);

BYTE CheckCmd(char *ptrbuff);

signed char searchPos1(char *pt, BYTE kommaanz);
void Cmd_txt(void);
void Cmd_cls(void);
void Cmd_demo(void);
void Cmd_help(void);
void Cmd_black(void);
void Cmd_time(void);
void ClockShow(void);
void ClockTick(void);

WORD searchTxtMarker(char *pt);
BYTE clockshow;

//----------------------------------------------------------------------------------------
// m a i n 
//----------------------------------------------------------------------------------------
int main(void) {

// DDRx:   data direction registetr
// PORTx:  input / output
  DDRB = 0b00000001;
  DDRC = 0b00000000;
  DDRD = 0b01110110;

//----------------------------------------------------------------------------------------
  // Initialisierung der Zeiger
  lesezeiger = &puffer[0];
  schreibzeiger = &puffer[0];

  cli();
  InitUart(BaudRate);
  Timer1_init();
  sei();

  Init_LCD();
  ClearDisplay();

//----------------------------------------------------------------------------------------

  Cmd_demo();
  Cmd_help();

// ------------------------------------------------------------------------------------------------
// - while(1) -- while(1) -- while(1) -- while(1) -- while(1) -- while(1) -- while(1) -- while(1) -
// ------------------------------------------------------------------------------------------------
  while(1)
  {
    // ------------------------------------------------------------------------
    // zeitgesteuerte Funktionen:
    // ------------------------------------------------------------------------

    //-----------------------10ms----------------------------------------------
    if (Ovl10ms)
    {
      Ovl10ms = 0;
    }

    //-----------------------100ms---------------------------------------------
    if (Ovl100ms)
    {
      Ovl100ms = 0;
      if (RXfull==1) { CheckCmd((char *) puffer); }
    }

    //-----------------------1s------------------------------------------------
    if (Ovl1sec) 
    {
      Ovl1sec = 0;
      ClockTick();
      if (clockshow==1) { ClockShow(); }
    }

    //-----------------------10s-----------------------------------------------
    if (Ovl10sec) 
    {
      Ovl10sec = 0;
    }

    //-----------------------1min----------------------------------------------
    if (Ovl1min) 
    {
      Ovl1min = 0;
    }

  // --------------------------------------------------------------------------

  }  // --> while(1) {}

  return 0;         // never!
}                   // end of main


//---------------------------------------------------------------------------------


//----------------------------------------------------------------------------------------
// Init Timer1A 1ms Bsp für 8MHz: OCR1A = 8000000/256/1000-1 = 31,..
//----------------------------------------------------------------------------------------
void Timer1_init(void)
{
  TCCR1A  = 0x00;
  TCCR1B = ((1<<WGM12) | (1<<CS12));   // (CTC - clear counter on match) Prescaler 256 S.137 .doc8025.pdf
  OCR1A = (uint16_t) (F_CPU/256/1000-1);
  TIMSK1 = (1<<OCIE1A); 
}


//----------------------------------------------------------------------------------------
// ISR: 1ms Timer1 Interrupt
//----------------------------------------------------------------------------------------
ISR(TIMER1_COMPA_vect)
{
  freemilli++;
  z001ms++;
  if (z001ms > 9) { z001ms = 0;  z010ms++;  Ovl10ms = 1;  }
  if (z010ms > 9) { z010ms = 0;  z100ms++;  Ovl100ms = 1; }
  if (z100ms > 9) { z100ms = 0;  z01sec++;  Ovl1sec = 1;  }
  if (z01sec > 9) { z01sec = 0;  z10sec++;  Ovl10sec = 1; }
  if (z10sec==6)  { z10sec = 0;             Ovl1min = 1;  }
}


//----------------------------------------------------------------------------------------
// Initialisierung UART
//----------------------------------------------------------------------------------------
void InitUart(uint32_t Baud_Rate)
{
  UCSR0B = (0<<RXEN0);
  UBRR0H=(F_CPU/16/Baud_Rate-1) >> 8;
  UBRR0L=(F_CPU/16/Baud_Rate-1) & 0xFF;
  UCSR0A=0x00;
  UCSR0B = (1<<RXEN0)|(1<<TXEN0)|(1<<RXCIE0); // Enable receiver and transmitter 
  UCSR0C = ((1<<UCSZ01)|(1<<UCSZ00));         // Einstellen des Datenformats: 8 Datenbits, 1 Stoppbit 
}


//----------------------------------------------------------------------------------------
// USART - Transmit - String
//----------------------------------------------------------------------------------------
void uart_puts (char *s)
{
BYTE dd;
  while (*s) {   // loop until *s != NULL
    dd = *s;
    sendc(dd);
    s++;
  }
}

//----------------------------------------------------------------------------------------
// USART - Transmit - Char
//----------------------------------------------------------------------------------------
void sendc(BYTE aa)
{
	while(!(UCSR0A & (1 << UDRE0)));    // wait until UDR ready
	UDR0 = aa;                          // send character
}

 
//----------------------------------------------------------------------------------------
// USART Receive-Interrupt 
//----------------------------------------------------------------------------------------
ISR (USART_RX_vect) 
{
  while ( !(UCSR0A & (1<<RXC0)) );
  if (RXfull==0) {
    *schreibzeiger=UDR0;
    RXcnt++;
    if (*schreibzeiger <= 0x0A) { RXfull=1; }
    schreibzeiger++;
  }
}


//----------------------------------------------------------------------------------------
// Uhr: ss: Sekunden
//      mm: Minuten
//      hh: Stunden
//----------------------------------------------------------------------------------------
void ClockTick(void)
{
  if (ss<59) { ss++; }
  else 
  {
    ss=0;
    if (mm<59) { mm++; }
    else {
      mm=0;
      if (hh<23) { hh++; }
      else { hh=0; }
    }
  }
}


//-------------------------------------------------------------------------------------------------
// UART-Daten checken und Kommandos ausführen
//-------------------------------------------------------------------------------------------------
BYTE CheckCmd(char *ptrbuff)
{
BYTE err,i;
  err=0;
  if ( strncmp( (char*)(puffer+0),"?", 1 ) == 0 ) {  }
  else if ( strncmp( (char*)(puffer+0),"help",  4 ) == 0 ) { Cmd_help();  }
  else if ( strncmp( (char*)(puffer+0),"cls",   3 ) == 0 ) { Cmd_cls();   }
  else if ( strncmp( (char*)(puffer+0),"txt ",  4 ) == 0 ) { Cmd_txt();   }
  else if ( strncmp( (char*)(puffer+0),"black", 5 ) == 0 ) { Cmd_black(); }
  //else if ( strncmp( (char*)(puffer+0),"off",   3 ) == 0 ) {              }
  else if ( strncmp( (char*)(puffer+0),"demo",  4 ) == 0 ) { Cmd_demo();  }
  else if ( strncmp( (char*)(puffer+0),"time",  4 ) == 0 ) { Cmd_time();  }
  //else if ( strncmp( (char*)(puffer+0),"txtb",  4 ) == 0 ) {              }
  else { err = 1; }

  i=0;
  while ((*(puffer+i)>0x0D) && (i<20)) 
  {
    sendc(*(puffer+i));
    i++;
  }
  if (!err) { uart_puts(" ...ok!"); }
  else      { uart_puts(" ...error!"); }
  schreibzeiger=&puffer[0];
  RXfull=0;
  return(err);
}


//----------------------------------------------------------------------------------------
// Set Time: 'time "12:59" 0D 0A'
//----------------------------------------------------------------------------------------
void Cmd_time(void)
{
BYTE pos,h10,h01,m01,m10;

  pos =  searchTxtMarker((char*)lesezeiger)>>8;
  if (pos != 0)
  {
    h10 = (*(lesezeiger+pos+0) & 0x0F);
    h01 = (*(lesezeiger+pos+1) & 0x0F);
    m10 = (*(lesezeiger+pos+3) & 0x0F);
    m01 = (*(lesezeiger+pos+4) & 0x0F);
    hh = h10*10+h01;
    mm = m10*10+m01;
    ss = 0;
    clockshow = 1;                            // Uhranzeige aktivieren
  }

}


//----------------------------------------------------------------------------------------
// Uhr anzeigen 1.Zeile
//----------------------------------------------------------------------------------------
void ClockShow(void)
{
BYTE x,y,h10,h01,m01,m10,s10,s01;

  y = 1;
  x = 3;
  h10 = hh/10+'0';
  h01 = hh-hh/10*10+'0';
  m10 = mm/10+'0';
  m01 = mm-mm/10*10+'0';
  s10 = ss/10+'0';
  s01 = ss-ss/10*10+'0';

  PaintCharC64(h10,y,x++); sendc(h10);
  PaintCharC64(h01,y,x++); sendc(h01);
  PaintCharC64(':',y,x++); sendc(':');
  PaintCharC64(m10,y,x++); sendc(m10);
  PaintCharC64(m01,y,x++); sendc(m01);
  PaintCharC64(':',y,x++); sendc(':');
  PaintCharC64(s10,y,x++); sendc(s10);
  PaintCharC64(s01,y,x++); sendc(s01); sendc(0x0D); sendc(0x0A);
}


//----------------------------------------------------------------------------------------
// Pixeltest: alle an
//----------------------------------------------------------------------------------------
void Cmd_black(void)
{
  GotoLineRow(0,0);
  for (WORD i=0; i<((101*64)/8+8); i++) // eigentlich 101*64/8 aber Display-SOC 
  {                                     // unterstützt 0..101, also 102 Spalten pro Zeile 
    WriteDisplay(0xFF);                 // es sind aber nur 101 Spalten vorhanden!
  }
}

//----------------------------------------------------------------------------------------
// Display löschen
//----------------------------------------------------------------------------------------
void Cmd_cls(void)
{
  clockshow = 0;                        // Uhranzeige deaktivieren
  ClearDisplay();
}


//----------------------------------------------------------------------------------------
// Info
//----------------------------------------------------------------------------------------
void Cmd_help(void)
{
  WriteString(1,1,"************");
  WriteString(2,1,"            ");
  WriteString(3,1," A70 101x64 ");
  WriteString(4,1,"------------");
  WriteString(5,1,"            ");
  WriteString(6,1,"-> 19200 8N1");
  WriteString(7,1,"            ");
  WriteString(8,1,"************");
}


//----------------------------------------------------------------------------------------
// Demo: kleiner Font, großer Font, Schiff
//----------------------------------------------------------------------------------------
void Cmd_demo(void)
{
BYTE c=0;
  for (int x=0; x<12; x++) {
    for (int y=0; y<12; y++) {
      PaintCharC64(c+' ',1+x,1+y);
      c++;
    }
  }
  delay_10ms(200);
  ClearDisplay();
  c = 0;
  for (int x=0; x<7; x=x+2) {
    for (int y=0; y<10; y++) {
      PaintChar8x12(c+' ',10+y*10,x);
      c++;
    }
  }
  delay_10ms(200);
  ClearDisplay();
  for (int x=0; x<7; x=x+2) {
    for (int y=0; y<10; y++) {
      PaintChar8x12(c+' ',10+y*10,x);
      c++;
    }
  }
  delay_10ms(200);
  ClearDisplay();
  PaintPic(&Schiff[0],1,0);
  delay_10ms(200);
  ClearDisplay();
}


//----------------------------------------------------------------------------------------
// Textdarstellung
//----------------------------------------------------------------------------------------
void Cmd_txt(void)
{
BYTE x,y,pos,i;
volatile char *abs;
BYTE start,end;
WORD StartEnd;

  x = 0;
  y = 0;
  pos =  searchPos1((char*)lesezeiger,1);
  if (pos != 0) {

    if (*(lesezeiger+pos-3)<'0') { x = (*(lesezeiger+pos-2) & 0x0F); }
    else {
        x = (*(lesezeiger+pos-3) & 0x0F)*10;
        x = x + (*(lesezeiger+pos-2) & 0x0F);
    }

    if (*(lesezeiger+pos+1)<'0') { y = (*(lesezeiger+pos) & 0x0F); }
    else {
      if (*(lesezeiger+pos+2)<'0') {
        y = (*(lesezeiger+pos) & 0x0F)*10;
        y = y + (*(lesezeiger+pos+1) & 0x0F);
      }
    }
  }

  StartEnd = searchTxtMarker((char*)lesezeiger);
  start = StartEnd>>8;
  end = StartEnd & 0x00FF;

  abs=(char*)(lesezeiger+start);
  i=0;
    while ((*(abs+i)>0x19) && (*(abs+i)<=0x7F) && (i<(end-start)))
    {
      PaintCharC64(*(abs+i),y,x+i);
      i++;
    }
}


//----------------------------------------------------------------------------------------
//  Textmarker " Anfang + Ende
//----------------------------------------------------------------------------------------
WORD searchTxtMarker(char *pt)
{
BYTE start, end, i;
i=0;
start = 0;
end = 0;

  for (i=0; i<max; i++)
  {
    if ((*pt=='"') && (start == 0)) { start = i+1; }
    if (*pt=='"') { end = i; }
    pt++;
    if (*pt<0x20) break;;
  }
  return ((start<<8)+end);
}


//----------------------------------------------------------------------------------------
//  searchPos
//----------------------------------------------------------------------------------------
signed char searchPos1(char *pt, BYTE kommaanz)
{
BYTE i;

  for (i=0; i<max; i++)
  {
    if (*pt==',') { kommaanz--; }
    if (kommaanz==0) { return i+1; }           // die position nach dem letzten komma
    pt++;
    if (*pt<0x20) return 0;
  }
  return 0;
}


//-----------------------------------------------------------------------------
// q&d Delayfunktion
//
// ACHTUNG: Interrupts global aus!
//-----------------------------------------------------------------------------
void delay_10ms(WORD t)
{
  cli();
  for (WORD x=0; x<t; x++) {
    _delay_ms(10);
  }
  sei();
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
