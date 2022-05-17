//ANTONIO DIMAMBRO & CONNER RICHE
#include <LiquidCrystal.h>
//LCD setup pins in order(RS, E, D4, D5, D6, D7) 
LiquidCrystal lcd(9, 8, 7, 6, 5, 4); 
unsigned char  timeRemaining;
unsigned short temperature;
//PortB registers
volatile unsigned char *port_b = (unsigned char *) 0x25;
volatile unsigned char *ddr_b  = (unsigned char *) 0x24;
volatile unsigned char *pin_b  = (unsigned char *) 0x23;
//PortD registers
volatile unsigned char *port_d = (unsigned char *) 0x2B;
volatile unsigned char *ddr_d  = (unsigned char *) 0x2A;
volatile unsigned char *pin_d  = (unsigned char *) 0x29;
//my_ADC registers
volatile unsigned char *my_adcsra = (unsigned char *) 0x7A;
volatile unsigned char *my_adcsrb = (unsigned char *) 0x7B;
volatile unsigned char *my_admux  = (unsigned char *) 0x7C;
volatile unsigned char *my_didr0  = (unsigned char *) 0x7E;
volatile unsigned short *my_adcl  = (unsigned short *) 0x78;

void setup()
{	
  	//Starting Time and temp 
  	timeRemaining = 30;
  	temperature = 50;
	lcd.begin(16, 2);
  	//Input 
	*ddr_b  &= 0xFB; 
  	//Set pull up resistor
	*port_b |= 0x04;
  	//Set output mode
  	*ddr_b  |= 0x18;
  	//Initialize output at 0
	*port_b &= 0xE7; 
  	//Set output mode
	*ddr_d  |= 0x0E; 
  	//Initialize output at 0
	*port_d &= 0xF1; 
  	//Start with auto-trigger enabled
  	*my_adcsra |= 0xE0;
  	*my_adcsra &= 0xF8;
  	//Running auto-trigger 
  	*my_adcsrb &= 0x40;
  	//Initialization of the admux
  	*my_admux &= 0x00;
  	*my_admux |= 0x40;
  	//Read at 0
  	*my_didr0 &= 0x00;
  	*my_didr0 |= 0x3E;
  	//Set trigger pin for Sensor
  	pinMode(13, OUTPUT);
}

void loop()
{
  	//Set trigger to ON
  	digitalWrite(13, HIGH);
  	//BUTTON IS PRESSED
	if(~(*pin_b) & 0x04)
	{   	
      	//Debouncer (like lab)
		unsigned int count = 0;
		while (count < 1000) {count++;}
		if (count >= 1000)
		{
			//BUTTON IS RELEASED
          	while (~(*pin_b) & 0x04) {}
			DcMotor (1);
          	//Read temp
          	temperature = GetTemp();
          	//start timer
          	timeRemaining = 30;
          	UpdateLcd();
          	//Is there an object (HAND)
			if(GetHandPosition() == 1)
			{
				//Start washing
              	WashTimer();
			}
			else
			{
				//If object is not seen
              	HandsMissing();
				if (timeRemaining > 0)
				{
					//Check if hands are now there
                  	WashTimer();
				}
              	//No hands? reset
			}
          	
          	//Close the system
            DcMotor (0);
          	//reset
  			lcd.clear();
		}
	}
  
  	
	/* Just in case timer bugs
	if (timeRemaining == 0)
    {
    	timeRemaining = 30;
    }
  	SetLcd (timeRemaining, temperature);
  	timeRemaining--;
	*/
}

void UpdateLcd ()
{
	//Get temperature
  	temperature = GetTemp();
  	//display values
	SetLcd(timeRemaining, temperature);
	
	return;
}

void SetLcd (unsigned char time, unsigned int temp)
{
  	//Prints on LCD
  	lcd.clear();
  	lcd.setCursor(0,0);
  	
  	lcd.print("Time:");
  	lcd.print(time);
  	lcd.print(" seconds");	
  	
  	lcd.setCursor(0,1);
  
  	lcd.print("Temp:");
  	lcd.print(temp);
  	lcd.print(" celsius");
  
  	delay(1000);
  	return;
}

void DcMotor (unsigned char direction)
{
  	//Clockwise shut it off
	if (direction == 0)
	{
		*port_b |= 0x08;
		*port_b &= 0xEF;		
	}
  	//Counter Clockwise turn it on
	else
	{
		*port_b |= 0x10;
		*port_b &= 0xF7;
	}
	
	delay(1000);
  	//Turn motor off
	*port_b &= 0xE7;
	
	return;
}
//Returns temp value
unsigned short GetTemp ()
{
  	//Read value
  	*my_adcsra |= 0x40;  
  	unsigned short valueTemp;
    
  	while ((*my_adcsra) & 0x40) {}
  
  	valueTemp = (*my_adcl & 0x03FF);
  
  	//Convert the adc value to Celsius 
  	if (valueTemp >= 102)
    {
  		valueTemp = (valueTemp - 102)/(2);
    }
  	else
    {
      	valueTemp = 0;
    }
  	//Not exact but close enough
  	return valueTemp;
}

//Do hands exist? 0 = no, 1 = yes
bool GetHandPosition()
{
	//reset
  	*ddr_d  |= 0x08; 
	*port_d &= 0xF7; 
	delayMicroseconds(2);
	
	*port_d |= 0x08; 
	delayMicroseconds(10); 
  
	*port_d &= 0xF7; 
	//Same from setup
	*ddr_d  &= 0xF7; 
	*port_d |= 0x08; 
  	//reset
	unsigned int value = 0;	
	//echo pin
	value = pulseIn(3, HIGH);
	//microseconds to centimeters conversion (distance)
	value = 0.01723 * value;
	if (value >= 336)
	{
      	//Hands are gone
		return 0;
	}
	
	else
	{
      	//Hands are here
		return 1;
	}
}


void WashTimer ()
{
  	//Still washing. timer is not done
	while (timeRemaining > 0)
	{
      	//Still have hands and still have time
		while ( (GetHandPosition() == 1) && (timeRemaining > 0) )
		{
			UpdateLcd();
          	//Actual timer delay is in function
			timeRemaining--;		
		} 
		
		if ( (GetHandPosition() != 1) && (timeRemaining > 0) )
		{
          	//Hands have left but there is still time
			HandsMissing();
		}
		
	}
	
	//Still have hands but time is out
	if (GetHandPosition() == 1)
	{
		*port_d |= 0x02; 
		
		delay(5000);
		*port_d &= 0xFD; 
	}
	
	return;
}

//Hands have left
void HandsMissing ()
{
	//reset but keep timing
  	*port_d |= 0x04; 
	timeRemaining = 30;
	UpdateLcd();
	timeRemaining--;
	
	while ( (GetHandPosition() != 1) && (timeRemaining > 0) )
	{
		//LED as an error light
      	*port_d ^= 0x04; 
		UpdateLcd();
		timeRemaining--;
	}
	//LED off
	*port_d &= 0xFB;
	
  	//Hands have returned and timer is below 10 so reset
	if ( (GetHandPosition() == 1) && (timeRemaining > 0) )
	{
      	//reset
		timeRemaining = 30;
	}
	
	return;
}
