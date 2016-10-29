/*
 * AutoLightingSystem.c
 *
 *  Created on: Oct 29, 2016
 *      Author: VinayDivakar
 */
#include <EEPROM.h>
#include "Wire.h"

/* Defines and Global Variables */
#define DS1307_ADDRESS 0x68
byte zero = 0x00;
int val = 0;

/* Structure to assign and store light trigger parameters */
typedef struct{
	int Adc_time;
	int Set_ON_Hour;
	int Set_ON_Mins;
	int Set_OFF_Hour;
	int Set_OFF_Mins;
	int Trigger_state;
}Time_triggerT, *Time_triggerP;

/* STATES */
typedef enum{
	CHECK_ON_STATE,
	CHECK_OFF_STATE,
	CHECK_ADC_STATE
} Light_stateT, *Light_stateP;

/* An object of typedef Time_triggerT */
Time_triggerT Light1;

void setup()
{
	/* Flag used prevent RTC from setting when powered ON */
	byte powerflag = 0;
	Wire.begin();
	Serial.begin(9600);
	powerflag = EEPROM.read(0x00);

	/* If powerflag is not equal to 5, set RTC time and date.
	 * Write a value of 5 to EEPROM at location  0x00
	 */
	if(powerflag!=5)
	{
		setDateTime();
		EEPROM.write(0x00,5);
	}

	/*Set Pin 13 as output */
	pinMode(13,OUTPUT);

	/* Set the auto light parameters */
	Light1.Set_ON_Hour = 18;
	Light1.Set_ON_Mins = 50;
	Light1.Set_OFF_Hour = 18;
	Light1.Set_OFF_Mins = 55;
	Light1.Adc_time = 1023;
	Light1.Trigger_state = CHECK_ADC_STATE;
}

void loop()
{
	printDate();
	delay(1000);
}

void setDateTime()
{
	byte second =      00; //0-59
	byte minute =      38; //0-59
	byte hour =        20; //0-23
	byte weekDay =     3; //1-7
	byte monthDay =    13; //1-31
	byte month =       9; //1-12
	byte year  =       16; //0-99
	Wire.beginTransmission(DS1307_ADDRESS);
	Wire.write(zero); //stop Oscillator
	Wire.write(decToBcd(second));
	Wire.write(decToBcd(minute));
	Wire.write(decToBcd(hour));
	Wire.write(decToBcd(weekDay));
	Wire.write(decToBcd(monthDay));
	Wire.write(decToBcd(month));
	Wire.write(decToBcd(year));
	Wire.write(zero); //start
	Wire.endTransmission();
}

byte decToBcd(byte val)
{
	// Convert normal decimal numbers to binary coded decimal
	return ( (val/10*16) + (val%10) );
}

byte bcdToDec(byte val)
{
	// Convert binary coded decimal to normal decimal numbers
	return ( (val/16*10) + (val%16) );
}

void printDate()
{

	// Reset the register pointer
	Wire.beginTransmission(DS1307_ADDRESS);
	Wire.write(zero);
	Wire.endTransmission();
	Wire.requestFrom(DS1307_ADDRESS, 7);
	int second = bcdToDec(Wire.read());
	int minute = bcdToDec(Wire.read());
	int hour = bcdToDec(Wire.read() & 0b111111); //24 hour time
	int weekDay = bcdToDec(Wire.read()); //0-6 -> sunday - Saturday
	int monthDay = bcdToDec(Wire.read());
	int month = bcdToDec(Wire.read());
	int year = bcdToDec(Wire.read());
	Serial.println("\r\n");
	Serial.println("Pot ADC:");
	val = analogRead(0);
	Serial.println(val);
	//print the date EG   3/1/11 23:59:59
	Serial.println("\r\n");
	Serial.println("Date:");
	Serial.print(month);
	Serial.print("/");
	Serial.print(monthDay);
	Serial.print("/");
	Serial.print(year);
	Serial.print(" ");
	Serial.println("\r\n");
	Serial.println("Time:");
	Serial.print(hour);
	Serial.print(":");
	Serial.print(minute);
	Serial.print(":");
	Serial.println(second);

	Check_Trigger_Time(&Light1,hour,minute,val);

}

/*Function Name : Check_Trigger_Time
 * Description:  State Machine program to continously monitor the light parameters
				 and control the LED/Light triggers
 * Parameters : 1. Time_TriggerP = Pointer to the structure with auto light parameters
 *              2. RTC_Hour = The hour value reterieved from the RTC
 *              3. RTC Min = The minutes value reterieved from the RTC
 *              4.  adc = The adc value set by the 10K POT
 */
void Check_Trigger_Time(Time_triggerP Light_Ptr,int RTC_Hour, int RTC_Min, int adc)
{
	switch(Light_Ptr->Trigger_state)
	{
	case CHECK_ADC_STATE:

		if(Light_Ptr->Adc_time == adc)
		{
			Serial.println("Intelligent Light Control Set for ON period of ");
			Serial.print(Light_Ptr->Set_ON_Hour);
			Serial.println(":");
			Serial.print(Light_Ptr->Set_ON_Mins);
			Serial.println(" To ");
			Serial.print(Light_Ptr->Set_OFF_Hour);
			Serial.println(":");
			Serial.print(Light_Ptr->Set_OFF_Mins);
			Light_Ptr->Trigger_state = CHECK_ON_STATE;
		}
		else
			Serial.println("No Valid Trigger point is set \r\n"); //End of ELSE if(Light_Ptr->Adc_time == adc)
		break;

	case CHECK_ON_STATE:

		if(Light_Ptr->Adc_time == adc)
		{
			if(Light_Ptr->Set_ON_Hour == RTC_Hour && Light_Ptr->Set_ON_Mins == RTC_Min)
			{
				Serial.println("Set ON Period Matched the Real Time RTC Values --> Trigger the Light ON....\r\n");

				/* Set the LED High */
				digitalWrite(13, HIGH);

				/* Set trigger state as ON_STATE */
				Light_Ptr->Trigger_state = CHECK_OFF_STATE;
			}
			else
				Serial.println("Time ON states yet to match for triggering the Light ON \r\n");
		}
		else
		{
			Serial.println("ADC trigger points not matching for ON states \r\n");
			digitalWrite(13, LOW);
			Light_Ptr->Trigger_state = CHECK_ADC_STATE;
		}//End of ELSE if(Light_Ptr->Adc_time == adc)
		break;

	case CHECK_OFF_STATE:
		if(Light_Ptr->Adc_time == adc)
		{
			if(Light_Ptr->Set_OFF_Hour == RTC_Hour && Light_Ptr->Set_OFF_Mins == RTC_Min)
			{
				Serial.println("Set OFF Period Matched the Real Time RTC Values --> Trigger the Light OFF....\r\n");

				/* Set the LED High */
				digitalWrite(13, LOW);

				/* Set trigger state as ON_STATE */
				Light_Ptr->Trigger_state = CHECK_ON_STATE;
			}
			else
				Serial.println("Time OFF states yet to match for triggering the Light OFF \r\n");
		}
		else
		{
			Serial.println("ADC trigger points not matching for OFF states \r\n");
			digitalWrite(13, LOW);
			Light_Ptr->Trigger_state = CHECK_ADC_STATE;
		}//End of ELSE if(Light_Ptr->Adc_time == adc)
		break;
	}//End of switch
}//End of Check_Trigger_Time






