//----------------------------------------------------------------------------------------
// Displayroutinen für LCD Siemens A70 (3.3Volt / Controller: ?PCF8812 o.ä.)
//----------------------------------------------------------------------------------------
#include "LCD_A70_101x64.h"
//----------------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------------
//  Pinbelegung: LCD A60 101x64 Pixel (auf Rückseite geschaut)
//
//
//  -----------------------------------
// |                                   |
// |             O O O O O O O O O O   |
// |             | | | | | | | | | |   |
// |             | | | | | | | | | |   |
// |             L L L G V D C D R C   |
// |             e e e n c a l / e s   |
// |             d d d d c t k C s     |
// |             1   2           e     |
// |             + - +           t     |
// |                                   |
// |                                   |
// |                                   |
// |                                   |
// |                                   |
// |                                   |
// |                                   |
// |                                   |
// |                                   |
// |                                   |
// | Displayrückseite                  |
//  -----------------------------------
//
//
//----------------------------------------------------------------------------------------
//                                                               PortPin am µC        

#define Data_HI PORTD |=  (1<<PORTD5);        // Data=1;           PD5
#define Data_LO PORTD &= ~(1<<PORTD5);        // Data=0;

#define Clock_HI PORTD |=  (1<<PORTD4);       // Clock=1;          PD4
#define Clock_LO PORTD &= ~(1<<PORTD4);       // Clock=0;

#define Control_HI PORTD |=  (1<<PORTD2);     // Control=1; D/C    PD2
#define Control_LO PORTD &= ~(1<<PORTD2);     // Control=0;

#define Reset_HI PORTB |=  (1<<PORTB0);       // Reset=1;          PB0
#define Reset_LO PORTB &= ~(1<<PORTB0);       // Reset=0;  

#define ChipSelect_HI PORTD |=  (1<<PORTD6);  // ChipSelect=1;     PD6
#define ChipSelect_LO PORTD &= ~(1<<PORTD6);  // ChipSelect=0;

//----------------------------------------------------------------------------------------



//----------------------------------------------------------------------------------------
// init LCD incl. Reset
//----------------------------------------------------------------------------------------
void Init_LCD(void)
{
  Reset_LCD();

  SendCommand(0x21);  // Function set: extended instruction set
  SendCommand(0x14);  // Bias System
  SendCommand(0x0A);  // HV-gen stages
  SendCommand(0x05);  // Temperature Control
  SendCommand(0xCC);  // Contrast: 204
  SendCommand(0x20);  // Function set: standard instruction set
  SendCommand(0x11);  // VLCD programming range: high
  SendCommand(0x0C);  // Display control: normal (inverted = 0x0D)
}   


//----------------------------------------------------------------------------------------
// reset LCD
//----------------------------------------------------------------------------------------
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


//----------------------------------------------------------------------------------------
// Funktion stellt ein Zeichen dar 8x8  Zeile 1..8 Spalte 1..12
// column 00..16 Spalten 17 X
// row     0...9 Zeilen  10 Y Reihe
//----------------------------------------------------------------------------------------
void PaintCharC64(BYTE sign, BYTE zeile, BYTE spalte) 
{
BYTE y,tmp;
int pos;

  if ((zeile<9) && (spalte<13)) {
    GotoLineRow(zeile-1, 100-spalte*8);
    if ((sign<0x20) || (sign>0x7F)) { sign = 0x20; } 
    pos = 8*(sign-0x20);
    for (y=0; y<8; y++) 
    {  
      tmp = pgm_read_byte(&code8x8[pos++]);
      WriteDisplay(tmp);
    }
  }
}


//----------------------------------------------------------------------------------------
void PaintChar8x12(BYTE sign, BYTE column, BYTE row) 
{
BYTE CharCol_1, CharCol_2, CharCol_3, CharCol_4, CharCol_5;
BYTE CharCol_6, CharCol_7, CharCol_8, CharCol_9, CharCol_10, CharCol_11, CharCol_12;
BYTE char_r_y_p12, char_r_y_p34, char_r_y_p56, char_r_y_p78, char_r_y_p910, char_r_y_p1112;
BYTE pz, y;
unsigned int pos;
BYTE tmp;

  GotoLineRow(row, 101-column);
  
  if ((sign<0x20) || (sign>0x7F)) { sign = 0x20; } 
  pos = 12*(sign-0x20);

  CharCol_1 = pgm_read_byte(&code8x12[pos++]);
  CharCol_2 = pgm_read_byte(&code8x12[pos++]);
  CharCol_3 = pgm_read_byte(&code8x12[pos++]);
  CharCol_4 = pgm_read_byte(&code8x12[pos++]);
  CharCol_5 = pgm_read_byte(&code8x12[pos++]);

  CharCol_6 = pgm_read_byte(&code8x12[pos++]);
  CharCol_7 = pgm_read_byte(&code8x12[pos++]);
  CharCol_8 = pgm_read_byte(&code8x12[pos++]);
  CharCol_9 = pgm_read_byte(&code8x12[pos++]);
  CharCol_10 = pgm_read_byte(&code8x12[pos++]);
  CharCol_11 = pgm_read_byte(&code8x12[pos++]);
  CharCol_12 = pgm_read_byte(&code8x12[pos++]);

  pz = 0x01;                                                  //pixelzeile
  for (y=0; y<8; y++) 
  {
    char_r_y_p12=char_r_y_p34=char_r_y_p56=char_r_y_p78=char_r_y_p910=char_r_y_p1112=0;
    tmp = 0;
    if (CharCol_1 & pz)  { tmp |= 0x80; }
    if (CharCol_2 & pz)  { tmp |= 0x40; }
    if (CharCol_3 & pz)  { tmp |= 0x20; }
    if (CharCol_4 & pz)  { tmp |= 0x10; }
    if (CharCol_5 & pz)  { tmp |= 0x08; }    
    if (CharCol_6 & pz)  { tmp |= 0x04; }
    if (CharCol_7 & pz) { tmp |= 0x02; }
    if (CharCol_8 & pz) { tmp |= 0x01; }
    WriteDisplay(tmp);
    pz = pz << 1;
  }

  GotoLineRow(row+1, 101-column);

  pz = 0x01;                                                  //pixelzeile
  for (y=0; y<8; y++) 
  {
    char_r_y_p12=char_r_y_p34=char_r_y_p56=char_r_y_p78=char_r_y_p910=char_r_y_p1112=0;
    tmp = 0;
    if (CharCol_9 & pz)  { tmp |= 0x80; }
    if (CharCol_10 & pz)  { tmp |= 0x40; }
    if (CharCol_11 & pz)  { tmp |= 0x20; }
    if (CharCol_12 & pz)  { tmp |= 0x10; }
    WriteDisplay(tmp);
    pz = pz << 1;
  }
}


//----------------------------------------------------------------------------------------
// send a string
//
// xPos: 0..16 (17 Spalten a 6 Pixel breit)
// yPos: 0.. 9 (10 Zeilen a 8 Pixel breit)
// Str:  max 17 Zeichen 
//----------------------------------------------------------------------------------------
void WriteString(BYTE xPos, BYTE yPos, char *Str) 
{
BYTE c,st,s;

  st = 0;
  s = *Str;
  while ((*Str!=0) && (xPos<100) && (st<17)) 
  { 
    c = *Str;
    PaintCharC64(c,xPos,yPos);
    yPos++;
    Str++;
    st++;
  }
}


//----------------------------------------------------------------------------------------
// paint a picture
//----------------------------------------------------------------------------------------
void PaintPic(const volatile uint8_t *Ptr, BYTE line, BYTE row)
{
BYTE tmp,tm;
WORD i;
BYTE hoehe,breite;

  hoehe = pgm_read_byte(&Ptr[1]);
  breite = pgm_read_byte(&Ptr[2]);
  GotoLineRow(line,8);
  for (i=0; i<(hoehe*breite/8); i++) 
  {
    if ((i%breite==0) && (i>=breite)) 
    {
      line++;
      GotoLineRow(line,8);
    }
    tmp = pgm_read_byte(&Ptr[i+3]);
    tm=0;
    if (tmp & 0x80) tm |= 0x01;
    if (tmp & 0x40) tm |= 0x02;
    if (tmp & 0x20) tm |= 0x04;
    if (tmp & 0x10) tm |= 0x08;
    if (tmp & 0x08) tm |= 0x10;
    if (tmp & 0x04) tm |= 0x20;
    if (tmp & 0x02) tm |= 0x40;
    if (tmp & 0x01) tm |= 0x80;
    WriteDisplay(tm);
  }
}


//----------------------------------------------------------------------------------------
// löscht gesamtes Grafikdisplay
//----------------------------------------------------------------------------------------
void ClearDisplay(void)
{
  GotoLineRow(0,0);
  for (WORD i=0; i<((101*64)/8+8); i++) // eigentlich 101*64/8 aber Display-SOC 
  {                                     // unterstützt 0..101, also 102 Spalten pro Zeile 
    WriteDisplay(0x00);                 // es sind aber nur 101 Spalten vorhanden!
  }
}


//----------------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------------
void GotoLineRow(BYTE Line, BYTE Row) // Line: 0..7, Row 0..101;
{   
  SendCommand( 0b00101000);
  SendCommand((0b01000000 + Line));
  SendCommand((0b10000000 + Row ));
}


//----------------------------------------------------------------------------------------
//
//----------------------------------------------------------------------------------------
void SendCommand(BYTE Command)
{
BYTE i,s;

  s=0x80;

  Data_LO;
  Clock_LO;
  Control_LO;
  ChipSelect_LO;
  for (i=0; i<8; i++) 
  {
    if (Command & s) { Data_HI; } 
    else             { Data_LO; }
    s = s>>1;
    Clock_HI;
    Clock_LO;
  }
  ChipSelect_HI;
}


//----------------------------------------------------------------------------------------
// command or data to display
//----------------------------------------------------------------------------------------
void WriteDisplay(BYTE Value)
{          
  BYTE i,s;

  s=0x80;
  Data_LO;
  Clock_LO;
  Control_HI;
  ChipSelect_LO;

  for (i=0; i<8; i++) 
  {
    if (Value & s) { Data_HI; } 
    else           { Data_LO; }
    s = s>>1;
    Clock_HI;
    Clock_LO;
  }
  ChipSelect_HI;
}


//----------------------------------------------------------------------------------------
//----------------------------------------------------------------------------------------
