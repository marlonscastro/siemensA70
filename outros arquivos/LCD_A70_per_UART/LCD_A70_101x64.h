#ifndef LCD_A70_101x64_H
#define LCD_A70_101x64_H
//----------------------------------------------------------------------------------------
// LCD Siemens A60 Handy 101x64
//--------------------------------------------------------------------------------------
#include <avr/io.h>
#include <compat/deprecated.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <stdlib.h>

#include "Font_A70.h"

//--------------------------------------------------------------------------------------
#define ack  1
#define nack 0
//-------------------------------------------------------------------------------------------------
#define BYTE  unsigned char
#define WORD  unsigned short
#define DWORD unsigned long
//-------------------------------------------------------------------------------------------------

//--------------------------------------------------------------------------------------
void Reset_LCD(void);
void Init_LCD(void);
void SendCommand(BYTE Command);
void GotoLineRow(BYTE Line, BYTE Row);
void WriteDisplay(BYTE Value);

void ClearDisplay(void);
void WriteString(BYTE xPos, BYTE yPos, char *Str);
void PaintCharC64(BYTE sign, BYTE column, BYTE row);
void PaintChar8x12(BYTE sign, BYTE zeile, BYTE spalte);
void PaintPic(const volatile uint8_t *Ptr, BYTE column, BYTE row);
//--------------------------------------------------------------------------------------
#endif
