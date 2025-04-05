#include<reg51.h>
#include<stdio.h>
/*----------------------------------------------------------------------------*/

#define LCD_Port P1			/* P1 port as data port */

sbit rs=P1^0;				/* Register select pin */
sbit rw=P1^1;        			/* Read/Write pin */
sbit en=P1^2;        			/* Latch Enable pin */

sbit DHT11=P3^0;		   	/* Connect DHT11 Sensor Pin to P3.0 Pin */

int Int_RH,Dec_RH,			/* 5 Varables to store 40 bits from DHT11 */
Int_Temp,Dec_Temp,
CheckSum; 

/*----------------------FUNCTIONS FOR DHT11-----------------------------------*/

void delay_20ms()			/* Delay function for request, more than 18ms */
{
	TMOD = 0x01;			/* Timer 0 mode1 (16-bit timer mode) */
	TH0 = 0xB8;			/* Load higher 8-bit in TH0 */
	TL0 = 0x00;			/* Load lower 8-bit in TL0 */
	TR0 = 1;			/* Start timer 0 */
	while(TF0 == 0);		/* Wait until timer 0 flag set */
	TR0 = 0;			/* Stop timer 0 */
	TF0 = 0;			/* Clear timer 0 flag */
}

void delay_30us() /* Delay function for checking bit value */
{
	TMOD = 0x01;			/* Timer 0 mode1 (16-bit timer mode) */
	TH0 = 0xFF;			/* Load higher 8-bit in TH0 */
	TL0 = 0xE5;			/* Load lower 8-bit in TL0 */
	TR0 = 1;			/* Start timer0 */
	while(TF0 == 0);		/* Wait until timer0 flag set */
	TR0 = 0;			/* Stop timer 0 */
	TF0 = 0;			/* Clear timer0 flag */
}

void Request()				/* 8051 sends  request */
{
	DHT11 = 0;			/* Pull down the voltage */
	delay_20ms();			/* wait for 20ms */
	DHT11 = 1;			/* Pull up the voltage */
}

void Response()				/* receive response from DHT11 */
{
	while(DHT11==1);	/* Wait until voltage is pulled down*/
	while(DHT11==0);	/* Wait for 80us after which voltage is pulled up*/
	while(DHT11==1);	/* Wait for 80us after which voltage is pulled down*/
}

int Receive_data()			/* receive data */
{
	int q,c=0;	
	for (q=0; q<8; q++)
	{
		while(DHT11==0);	/* check received bit 0 or 1 */
		delay_30us();
		
		if(DHT11 == 1)		/* if high pulse is greater than 30ms */
		c = (c<<1)|(0x01);	/* then its logic HIGH */
		
		else			/* otherwise its logic LOW */
		c = (c<<1);
		
		while(DHT11==1);
	}
	return c;
}

/*----------------------------------------------------------------------------*/

/*-------------------------FUNCTIONS FOR LCD 16X2-----------------------------*/

void delay(unsigned int count) 		/* Delay function for multiple of 1ms */
{
	int i ;
  for(i=0;i<count;i++)
  {
	TMOD = 0x01;			/* Timer 0 mode1 (16-bit timer mode) */
	TH0 = 0xFC; 			/* Load higher 8-bit in TH0 */
	TL0 = 0xF1;			/* Load lower 8-bit in TL0 */
	TR0 = 1;			/* Start timer0 */
	while(TF0 == 0);		/* Wait until timer0 flag set */
	TR0 = 0;			/* Stop timer 0 */
	TF0 = 0;			/* Clear timer 0 flag */
  }
}

void LCD_Command (char cmnd)   	/* LCD command funtion */
{
	LCD_Port =(LCD_Port & 0x0F) | (cmnd & 0xF0); 	/* sending upper nibble */
	rs=0;	 																				/* command reg. */
	rw=0;																					/* Write operation */
	en=1; 
	delay(1);
	en=0;
	delay(10);

	LCD_Port = (LCD_Port & 0x0F) | (cmnd << 4); 	 /* sending lower nibble */
	en=1; 																				
	delay(1);
	en=0;
	delay(101);
}
void LCD_Char (char char_data)				/* LCD data write function */
{
	LCD_Port =(LCD_Port & 0x0F) | (char_data & 0xF0); /* sending upper nibble */    
	rs=1;																							/*Data reg.*/
	rw=0;																							/*Write operation*/
	en=1;   				
	delay(1);
	en=0;
	delay(10);

	LCD_Port = (LCD_Port & 0x0F) | (char_data << 4);  /* sending lower nibble */
	en=1; 																						
	delay(1);
	en=0;
	delay(10);
}
void LCD_String (char str[] )				/* Send string to LCD function */
{
	int i;
	for(i=0;str[i]!='\0';i++)			/* Send each char of string till null terminator */
	{
		LCD_Char (str[i]);			/* Call LCD data write function */
	}
}

void LCD_Top_Bottom (char row, char *str)		/* Send string to LCD function */
{
	if (row == 1)
	LCD_Command(0x80);				/* Command of first row */
	
	else if (row == 2)
	LCD_Command(0xC0);				/* Command of second row */
	LCD_String(str);				/* Call LCD string function */
}

void LCD_Init (void)					/* LCD Initialize function */
{
	delay(20);					/* LCD Power On Initialization time >15ms */
	LCD_Command (0x02);				/* Initial command for 4-bit mode */
	LCD_Command (0x28);				/* Initialization of LCD in 2 line, 5X7, 4bit mode */
	LCD_Command (0x0C);				/* Display On Cursor Off */
	LCD_Command (0x01);				/* Clear display */
	LCD_Command (0x06);				/* Auto Increment cursor, Left align*/
	LCD_Command (0x80);				/* Cursor at home position */
}
/*----------------------------------------------------------------------------*/

/*-----------------------------MAIN FUNCTION----------------------------------*/
void main()
{
	unsigned char dat[16];
	LCD_Init();					/* initialize LCD */
	delay(1000);					/* wait 1sec for DHT11 to turn on*/					
	
	while(1)
	{		
		Request();				/* send start pulse */
		Response();				/* receive response */
		
		Int_RH	=	Receive_data();		/* store first eight bit in Int_RH */		
		Dec_RH	=	Receive_data();		/* store next eight bit in Dec_RH */	
		Int_Temp=	Receive_data();		/* store next eight bit in Int_Temp */
		Dec_Temp=	Receive_data();		/* store next eight bit in Dec_Temp */
		CheckSum=	Receive_data();		/* store next eight bit in CheckSum */

		if ((Int_RH + Dec_RH + Int_Temp + Dec_Temp) != CheckSum)
		{
			LCD_Top_Bottom(1,"Error"); 	/* Error checking*/
		}
		
		else
		{
			sprintf(dat,"Humidity   :%d %%",Int_RH);
			LCD_Top_Bottom(1,dat);
			sprintf(dat,"Temperature:%d C",Int_Temp);   
			LCD_Top_Bottom(2,dat);
		}		
		delay(1000);
	}	
}
/*----------------------------------------------------------------------------*/

