typedef enum
{
   LCD_CMD  = 0,
   LCD_DATA = 1

} LcdCmdData;

typedef enum
{
   PIXEL_OFF =  0,
   PIXEL_ON  =  1,
   PIXEL_XOR =  2

} LcdPixelMode;

typedef enum
{
   FONT_1X = 1,
   FONT_2X = 2

} LcdFontSize;

void LcdSend(byte dados, LcdCmdData cd);
void lcd_init (void);
void lcd_clear (void);
void lcd_contrast ( byte contrast );


#define cs        PIN_B0 //
#define reset     PIN_B1 // 
#define dc        PIN_B2 // 
#define clk       PIN_B3 // 
#define data      PIN_B4 //

//*******************************************************************
//LCD initialization function
//no parameter
//no return
//*******************************************************************

void lcd_init(void){

   //output_b(0xFF);
   
   output_high(reset);

   output_low(reset);
   delay_ms(30);
   output_high(reset);

   output_high(cs);
       
   LcdSend( 0x21, LCD_CMD ); /* LCD Extended Commands. */
   LcdSend( 0xCF, LCD_CMD ); /* Set LCD Vop (Contrast).*/
   LcdSend( 0x06, LCD_CMD ); /* Set Temp coefficent. */
   LcdSend( 0x13, LCD_CMD ); /* LCD bias mode 1:48. */
   //LcdSend( 0x30, LCD_CMD);
   LcdSend( 0x28, LCD_CMD ); /* LCD Standard Commands,Horizontal addressing mode */
   LcdSend( 0x0C, LCD_CMD ); /* LCD in normal mode. */

   /* Clear display on first time use */
   lcd_clear();
}                

void LcdSend(byte dados, LcdCmdData cd){
   unsigned char i, _tmp;
   output_low(clk);      
   
   /*  Enable display controller (active low). */
   output_low(cs);
 
   if ( cd == LCD_DATA ){
       output_high(dc);
   }
   else{
       output_low(dc);
   }

   /*  Send data to display controller. */
   //SPDR = data;
   for (i=0; i<=7; i++)
   {   output_high(clk);    
    _tmp = dados >> 7;
    _tmp &= 0x01;
    //LCD_DIO = _tmp;
    output_bit(data, _tmp);
    //delay_ms(10);
    output_bit(clk, 0);
    //delay_ms(10);
    dados = dados << 1;
    };
   
   /* Disable display controller. */
   output_bit(cs, 1);
}                                

//*******************************************************************
// LCD clear function
//*******************************************************************
void lcd_clear ( void )
{
   long i;

   /*  Set base address according to LoWaterMark. */
   LcdSend( 0x80, LCD_CMD );  //Set base X address of RAM 0 (legal address 0..95)
   LcdSend( 0x40, LCD_CMD );  //Set base Y address of RAM 0 (legal address 0..7)

   /*  Serialize the display buffer. */
   for ( i = 0; i <= 767; i++ )
   {
       LcdSend( 0x00, LCD_DATA );
   }

}
