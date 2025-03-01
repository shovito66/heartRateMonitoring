/*  Group 7 | UoM | ENTC 17Batch  */
#ifndef F_CPU
#define F_CPU 8000000UL // 16 MHz clock speed
#endif

/*
#define D0 eS_PORTD0
#define D1 eS_PORTD1
#define D2 eS_PORTD2
#define D3 eS_PORTD3
*/

#define D4 eS_PORTD4
#define D5 eS_PORTD5
#define D6 eS_PORTD6
#define D7 eS_PORTD7
#define RS eS_PORTC6
#define EN eS_PORTC7

#include <avr/io.h>
#include <util/delay.h>

#include "stdlib.h"
#include "string.h"
#include "lcd.h"

/* bluetooth module */
#include "USART_RS232_H_file.h"		/* include USART library */

 

///////////lcd r kaj

void lcd_disp(char data_points[],int r,int c,char w[]){ 

	if(w=="clear")Lcd4_Clear();
	Lcd4_Set_Cursor(r,c);
	Lcd4_Write_String(data_points);
}
///////////lcd r kaj



void ADC_Init(){
	DDRA=0x00;			/* Make ADC port as input */
	ADCSRA = 0x87;			/* Enable ADC, fr/128  */
	ADMUX = 0x41;			/* Vref: Avcc, ADC channel: 0 */
}


///////////adc r kaj

int ADC_Read(char channel){
	int Ain,AinLow;
	
	ADMUX=ADMUX|(channel & 0x0f);	/* Set input channel to read */

	ADCSRA |= (1<<ADSC);		/* Start conversion */
	while((ADCSRA&(1<<ADIF))==0);	/* Monitor end of conversion interrupt */
	_delay_us(10);

	AinLow = (int)ADCL;		/* Read lower byte*/
	Ain = (int)ADCH*256;		/* Read higher 2 bits and 
					Multiply with weight */
	Ain = Ain + AinLow;				
	return(Ain);			/* Return digital value*/
}


//adc r kaj
/////////

/*************** threshold ***************/
double m=0,c=0; // gradient and slope

double sumit(int data_points[],int length){
	 int i;
	 double sum=0;
	 for(int i=1;i<=length;i++)sum+=data_points[i];

	 return sum;
}

double xysum(int data_points[],int length){
	 int i;
	 double sum=0;
	 for(int i=1;i<=length;i++)sum+=(i)*data_points[i];

	 return sum;
}

void regression(int data_points[],double n){
	double squarex= (n)*(n+1)*(2*n+1)/6.0;
	double xbar = (n+1)/2.0;
	double ybar = sumit(data_points,n)/n;

	m=(xysum(data_points,n)- n*xbar*ybar)/( squarex - n*xbar*xbar );
	c=ybar-m*xbar;
}
/*************** end threshold ***************/

void playBuzzer(){
	DDRB = 0xFF;
	PORTB = 0xFF;        // Turn ON the Buzzer conneted to PORTC
	_delay_ms(3000);      // Wait for some time
	PORTB = 0x00;        // Turn OFF the Buzzer connected to PORTC
	_delay_ms(3000);      // Wait for some time
}

int main(void){
    DDRD = 0xFF;  // #
	DDRC = 0xFF;  //for lcd
	DDRA = 0x00;  //Analog input
	
    DDRB = 0xff;							/* make PORT as output port */
    USART_Init(9600);						/* initialize USART with 9600 baud rate */

	
	
	ADC_Init();
	//ADMUX = 0b01100000; // Configure ADC to be left justified, use AVCC as reference, and select ADC0 as ADC input
	//ADCSRA = 0b10000111; // Enable the ADC and set the prescaler to max value (128)


	Lcd4_Init(); //Initializing the LCD screen


	_delay_ms(6000);  //1000
	

	int i;
	int thresh=300;
	int count=0;
	int count2=0; //modified peak counting algo
	
	
	/*  timing data */
		double sampling_rate = 0.100 ;	   	// actually this is the _delay_ms val
		int time_limit = 10 ;  				 //in seconds
		int se=time_limit/(sampling_rate*2);
		int data_points[se+1];

	data_points[0]=0;
	/* /timing data */


	
	/* for debugging purposes -> h and l records the peaks */
	int h=0;
	int l=1023;
	
	char val[6]; //temporary variable for itoa

	
	for(i=0;i<time_limit/(sampling_rate*2);i++){	
		data_points[i+1]=ADC_Read(0);
		
		itoa(data_points[i+1],val,10);
		//lcd_disp(val,1,0,"");
		
		///value to bluetooth module///
		
		USART_SendString(val);
		USART_SendString("\n");
		
		///value to bluetooth module///
		
		lcd_disp("Measuring.. *_*",1,0,"clear");
		
		int k=16*(data_points[i+1]-200)/375;
		
		char anim[16]="";
		
		int h=0;
		for(h=0;h<k;h++){
			anim[h]='~';
		}
		itoa(k,val,10);
		lcd_disp(anim,2,0,"");
		_delay_ms(sampling_rate*100); //100
	
	}


	regression(data_points,se);

	//recorded wave form
	for(i=0;i<time_limit/(sampling_rate*2);i++){	
		thresh=(i+2)*m+c;

		int a=data_points[i+1];//current value
		char temp[11]="";
		char ccount[3];

		if(a>h)h=a; //max peak
		if(a<l)l=a; //min peak
		
		if(a>thresh){
			count+=1; //peak counting]
			if(a>data_points[i] && a>data_points[i+2]){
				count2+=1;
			}
			
		}
		
	
	}



	//displaying the heart rate
	char bpm[16]="Heart Rate : ";
	int HeartRatevalue;
	
	lcd_disp(bpm,1,0,"clear");
	
	itoa(count2*(60/time_limit),val,10);
	HeartRatevalue=count2*(60/time_limit);
	if(HeartRatevalue>80) 
		playBuzzer();
	strcat(val," bpm");
	lcd_disp(val,2,4,"");
	
	
	_delay_ms(2000);
	
	USART_SendString("Project Heart Beat Monitoring System\n Heart Beat:");
	USART_SendString(val);
	USART_SendString("\n");
	if(HeartRatevalue>80){
		USART_SendString("HEART ATTACK!!!!!!!");	
	}
	
}