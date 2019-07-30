/****************************************************/
/* This is only one example of code structure       */
/* OFFCOURSE this code can be optimized, but        */
/* the idea is let it so simple to be easy catch    */
/* where can do changes and look to the results     */
/****************************************************/
//set your clock speed
#define F_CPU 16000000UL
//these are the include files. They are outside the project folder
#include <avr/io.h>
//#include <iom1284p.h>
#include <util/delay.h>
#include <avr/interrupt.h>

// Standard Input/Output functions 1284
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <util/atomic.h>
#include <stdbool.h>
#include "stdint.h"
#include <Arduino.h>

#define VFD_in 7// If 0 write LCD, if 1 read of LCD
#define VFD_clk 8 // if 0 is a command, if 1 is a data0
#define VFD_stb 9 // Must be pulsed to LCD fetch data of bus

#define VFD_onGreen 4 // Led on/off Green
#define VFD_onRed 5 // Led on/off Red
#define VFD_net 6 // Led net

#define AdjustPins    PIND // before is C, but I'm use port C to VFC Controle signals

unsigned char DigitTo7SegEncoder(unsigned char digit, unsigned char common);

/*Global Variables Declarations*/
unsigned char hours = 0;
unsigned char minutes = 0;
unsigned char minute = 0;
unsigned char secs=0;
unsigned char seconds=0;
unsigned char milisec = 0;

unsigned char memory_secs=0;
unsigned char memory_minutes=0;

unsigned char number;
unsigned char numberA0;
unsigned char numberA1;
unsigned char numberB0;
unsigned char numberB1;
unsigned char numberC0;
unsigned char numberC1;
unsigned char numberD0;
unsigned char numberD1;
unsigned char numberE0;
unsigned char numberE1;
unsigned char numberF0;
unsigned char numberF1;

unsigned char grid;
unsigned char gridSegments = 0b00000001; // Here I define the number of GRIDs and Segments I'm using

unsigned int k=0;
boolean flag=true;
boolean flagSecs=false;


unsigned int segments[] ={
  //  This not respect the table for 7 segm, is like this: "bgchde.."......af"  // 
  //  This ship supporte 0-8 seg more 9-10 segm, see table in datasheet
  //  Only have max of 5 Grid.
       0b10101100,0b00000011,//0   
       0b10100000,0b00000000,//1     
       0b11001100,0b00000010,//2   
       0b11101000,0b00000010,//3    
       0b11100000,0b00000001,//4   
       0b01101000,0b00000011,//5   
       0b01101100,0b00000011,//6   
       0b10100000,0b00000010,//7   
       0b11101100,0b00000011,//8   
       0b11100000,0b00000011,//9
       0b00000000,0b00000000,//10 Empty display 
  };
 
void clear_VFD(void);

void pt6958_init(void){
  delayMicroseconds(200); //power_up delay
  // Note: Allways the first byte in the input data after the STB go to LOW is interpret as command!!!

  // Configure VFD display (grids)
  cmd_with_stb(gridSegments); // cmd 1 // PT6958 is fixed to 5 grids
  delayMicroseconds(1);
  // Write to memory display, increment address, normal operation
  cmd_with_stb(0b01000000);//(BIN(01000000));
  delayMicroseconds(1);
  // Address 00H - 15H ( total of 11*2Bytes=176 Bits)
  cmd_with_stb(0b11000000);//(BIN(01100110)); 
  delayMicroseconds(1);
  // set DIMM/PWM to value
  cmd_with_stb((0b10001000) | 7);//0 min - 7 max  )(0b01010000)
  delayMicroseconds(1);
}

void cmd_without_stb(unsigned char a)
{
  // send without stb
  unsigned char transmit = 7; //define our transmit pin
  unsigned char data = 170; //value to transmit, binary 10101010
  unsigned char mask = 1; //our bitmask
  
  data=a;
  //This don't send the strobe signal, to be used in burst data send
         for (mask = 00000001; mask>0; mask <<= 1) { //iterate through bit mask
           digitalWrite(VFD_clk, LOW);
                 if (data & mask){ // if bitwise AND resolves to true
                    digitalWrite(VFD_in, HIGH);
                 }
                 else{ //if bitwise and resolves to false
                   digitalWrite(VFD_in, LOW);
                 }
          delayMicroseconds(5);
          digitalWrite(VFD_clk, HIGH);
          delayMicroseconds(5);
         }
   //digitalWrite(VFD_clk, LOW);
}

void cmd_with_stb(unsigned char a)
{
  // send with stb
  unsigned char transmit = 7; //define our transmit pin
  unsigned char data = 170; //value to transmit, binary 10101010
  unsigned char mask = 1; //our bitmask
  
  data=a;
  
  //This send the strobe signal
  //Note: The first byte input at in after the STB go LOW is interpreted as a command!!!
  digitalWrite(VFD_stb, LOW);
  delayMicroseconds(1);
         for (mask = 00000001; mask>0; mask <<= 1) { //iterate through bit mask
           digitalWrite(VFD_clk, LOW);
           delayMicroseconds(1);
                 if (data & mask){ // if bitwise AND resolves to true
                    digitalWrite(VFD_in, HIGH);
                 }
                 else{ //if bitwise and resolves to false
                   digitalWrite(VFD_in, LOW);
                 }
          digitalWrite(VFD_clk, HIGH);
          delayMicroseconds(1);
         }
   digitalWrite(VFD_stb, HIGH);
   delayMicroseconds(1);
}

void test_LED(void){
  //This function is only to teste the led's ON green ON red and led of NET symbol of pannel.
   digitalWrite(VFD_onGreen, HIGH);
   delay(1000);
   digitalWrite(VFD_onGreen, LOW);
   digitalWrite(VFD_onRed, HIGH);
   delay(1000);
   digitalWrite(VFD_onRed, LOW);
   digitalWrite(VFD_net, HIGH);
   delay(1000);
   digitalWrite(VFD_net, LOW);
}

void test_VFD(void){
  /* 
  Here do a test for all segments of 6 grids
  each grid is controlled by a group of 2 bytes
  by these reason I'm send a burst of 2 bytes of
  data. The cycle for do a increment of 3 bytes on 
  the variable "i" on each test cycle of FOR.
  */
  // to test 6 grids is 6*3=18, the 8 gird result in 8*3=24.
 
  clear_VFD();
      
      digitalWrite(VFD_stb, LOW);
      delayMicroseconds(1);
      //cmd_with_stb(gridSegments); // PT6958 is fixed to 5 grids
      cmd_with_stb(0b01000000); // cmd 2 //Normal operation; Set pulse as 1/16
      
                    for (int n=0; n < 8; n++){
                                for (int i = 0; i < 8 ; i++){ //
                                              clear_VFD();
                                              digitalWrite(VFD_stb, LOW);
                                              delayMicroseconds(1);
                                              cmd_without_stb((0b11000000 | n)); //cmd 3 wich define the start address (00H to 15H)
                                              //   
                                              cmd_without_stb(0b00000001 << i); // 
                                              cmd_without_stb(0b00000000); // 
                                              //cmd_without_stb(0b00000001); // cmd 1// Command to define the number of grids and segments
                                              digitalWrite(VFD_stb, HIGH);
                                              delay(1);
                                              cmd_with_stb((0b10001000) | 7); //cmd 4
                                              delay(100);
                                }
                   }
}

void test_segments_and_grids(void){
  clear_VFD();
      
      digitalWrite(VFD_stb, LOW);
      delayMicroseconds(1);
      //cmd_with_stb(gridSegments); // cmd 1 // PT6958 is fixed to 5 grids
      cmd_with_stb(0b01000000); // cmd 2 //Normal operation; Set pulse as 1/16
              for (int n=0; n<8; n++){
                 Serial.println(n);
                       for (int i = 0; i < 8 ; i++){ // test base to 16 segm and 6 grids
                          digitalWrite(VFD_stb, LOW);
                          delayMicroseconds(1);
                            cmd_without_stb((0b11000000 | n)); //cmd 3 wich define the start address (00H to 15H)
                            cmd_without_stb(0b00000001 << i); // 
                            digitalWrite(VFD_stb, HIGH);
                           cmd_with_stb((0b10001000) | 7); //cmd 4
                          delay(200);
                       }
              }
}

void test_VFD_chkGrids(void){
  /* 
  Here do a test for all segments of 5 grids
  each grid is controlled by a group of 2 bytes
  by these reason I'm send a burst of 2 bytes of
  data. The cycle for do a increment of 3 bytes on 
  the variable "i" on each test cycle of FOR.
  */
  // to test 6 grids is 6*3=18, the 8 grid result in 8*3=24.
 
  clear_VFD();
      
      digitalWrite(VFD_stb, LOW);
      delayMicroseconds(1);
      //cmd_with_stb(gridSegments); // cmd 1 // PT6958 is fixed to 5 grids
      cmd_with_stb(0b01000000); // cmd 2 //Normal operation; Set pulse as 1/16
      
        digitalWrite(VFD_stb, LOW);
        delayMicroseconds(1);
        cmd_without_stb((0b11000000)); //cmd 3 wich define the start address (00H to 15H)
                   for (int i = 0; i < 6 ; i++){ // test base to 15 segm and 7 grids
                   cmd_without_stb(0b11111111); // Data to fill table 5*16 = 80 bits
                   cmd_without_stb(0b11111111); // Data to fill table 5*16 = 80 bits
                   }
          digitalWrite(VFD_stb, HIGH);
          delayMicroseconds(1);
          cmd_with_stb((0b10001000) | 7); //cmd 4
          delay(1);
          delay(100);
}

void test_VFD_grid(void){
  clear_VFD();
      //
      digitalWrite(VFD_stb, LOW);
      delayMicroseconds(1);
      //cmd_with_stb(gridSegments); // cmd 1 // PT6958 is fixed to 5 grids
      cmd_with_stb(0b01000000); // cmd 2 //Normal operation; Set pulse as 1/16
      //
      cmd_with_stb((0b11000000)); //cmd 3 wich define the start address (00H to 15H)
      for (int i = 0; i < 12 ; i=i+2){ // test base to 15 segm and 6 grids
          digitalWrite(VFD_stb, LOW);
          delayMicroseconds(1);
          cmd_without_stb((0b11000000) | i);
      
             cmd_without_stb(segments[i]); // Data to fill table 6*16 = 96 bits
             cmd_without_stb(segments[i+1]); // Data to fill table 6*16 = 96 bits
             digitalWrite(VFD_stb, HIGH);
             cmd_with_stb((0b10001000) | 7); //cmd 4
      
          delay(1);
          delay(500);
          digitalWrite(VFD_stb, LOW);
          delayMicroseconds(1);
          cmd_without_stb((0b11000000) | i);
          cmd_without_stb(0b00000000); // Data to fill table 6*16 = 96 bits
          cmd_without_stb(0b00000000); // Data to fill table 6*16 = 96 bits
          digitalWrite(VFD_stb, HIGH);
          cmd_with_stb((0b10001000) | 7); //cmd 4
      
        delay(1);
        delay(500);
      }
}

void clear_VFD(void)
{
  /*
  Here I clean all registers 
  Could be done only on the number of grid
  to be more fast. The 12 * 3 bytes = 36 registers
  */
      for (int n=0; n < 14; n++){  // important be 10, if not, bright the half of wells./this on the VFD of 6 grids)
        //cmd_with_stb(gridSegments); // cmd 1 // PT6958 is fixed to 5 grids
        cmd_with_stb(0b01000000); //       cmd 2 //Normal operation; Set pulse as 1/16
        digitalWrite(VFD_stb, LOW);
        delayMicroseconds(1);
            cmd_without_stb((0b11000000) | n); // cmd 3 //wich define the start address (00H to 15H)
            cmd_without_stb(0b00000000); // Data to fill table of 6 grids * 15 segm = 80 bits on the table
            //
            //cmd_with_stb((0b10001000) | 7); //cmd 4
            digitalWrite(VFD_stb, HIGH);
            delayMicroseconds(100);
     }
}

void setup() {
// put your setup code here, to run once:

// initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(9600);
  seconds = 0x00;
  minutes =0x00;
  hours = 0x00;

  /*CS12  CS11 CS10 DESCRIPTION
  0        0     0  Timer/Counter1 Disabled 
  0        0     1  No Prescaling
  0        1     0  Clock / 8
  0        1     1  Clock / 64
  1        0     0  Clock / 256
  1        0     1  Clock / 1024
  1        1     0  External clock source on T1 pin, Clock on Falling edge
  1        1     1  External clock source on T1 pin, Clock on rising edge
 */
  // initialize timer1 
  cli();           // disable all interrupts
  // initialize timer1 
  //noInterrupts();           // disable all interrupts
  TCCR1A = 0;
  TCCR1B = 0;// This initialisations is very important, to have sure the trigger take place!!!
  TCNT1  = 0;
  // Use 62499 to generate a cycle of 1 sex 2 X 0.5 Secs (16MHz / (2*256*(1+62449) = 0.5
  OCR1A = 62498;            // compare match register 16MHz/256/2Hz
  //OCR1A = 1500; // only to use in test, increment seconds to fast!
  TCCR1B |= (1 << WGM12);   // CTC mode
  TCCR1B |= ((1 << CS12) | (0 << CS11) | (0 << CS10));    // 256 prescaler 
  TIMSK1 |= (1 << OCIE1A);  // enable timer compare interrupt
 
  
// Note: this counts is done to a Arduino 1 with Atmega 328... Is possible you need adjust
// a little the value 62499 upper or lower if the clock have a delay or advnce on hours.
   
//  a=0x33;
//  b=0x01;

CLKPR=(0x80);
//Set PORT
DDRD = 0xFF;  // IMPORTANT: from pin 0 to 7 is port D, from pin 8 to 13 is port B
PORTD=0x00;
DDRB =0xFF;
PORTB =0x00;

pt6958_init();

test_VFD();

clear_VFD();

//only here I active the enable of interrupts to allow run the test of VFD
//interrupts();             // enable all interrupts
sei();
}
/******************************************************************/
/************************** Update Clock **************************/
/******************************************************************/
void send_update_clock(void){
    if (secs >=60){
      secs =0;
      minutes++;
      minute++;
    }
    if (minutes >=60){
      minutes =0;
      minute =0;
      hours++;
    }
    if (hours >=24){
      hours =0;
    }
  
    //*************************************************************
    DigitTo7SegEncoder(secs%10);
    numberA0=segments[number];
    numberA1=segments[number+1];
    DigitTo7SegEncoder(secs/10);
    numberB0=segments[number];
    numberB1=segments[number+1];
        if (flagSecs){
          wrMemory_VersionSecs();
        }
        else{
          wrMemory_VersionHours();
        }
    //*************************************************************
          if (flagSecs == true){
                DigitTo7SegEncoder(minute%10);
                numberC0=segments[number];
                numberC1=segments[number+1];
                DigitTo7SegEncoder(minute/10);
                numberD0=segments[number];
                numberD1=segments[number+1];
                }
          else{
                DigitTo7SegEncoder(minutes%10);
                numberC0=segments[number];
                numberC1=segments[number+1];
                DigitTo7SegEncoder(minutes/10);
                numberD0=segments[number];
                numberD1=segments[number+1];
          }
        if (flagSecs){
          wrMemory_VersionSecs();
        }
        else{
          wrMemory_VersionHours();
        }
    //**************************************************************
    DigitTo7SegEncoder(hours%10);
    numberE0=segments[number];
    numberE1=segments[number+1];
    DigitTo7SegEncoder(hours/10);
    numberF0=segments[number];
    numberF1=segments[number+1];
        if (flagSecs){
          wrMemory_VersionSecs();
        }
        else{
          wrMemory_VersionHours();
        }
    //**************************************************************

        if (flagSecs==true){
            if(minute==memory_minutes & secs==memory_secs){
             digitalWrite(10, HIGH);
             secs=0;
             minute=0;
            }
        }
}

void wrMemory_VersionSecs(){
//Serial.println(number,HEX);
 
  digitalWrite(VFD_stb, LOW);
  delayMicroseconds(10);
      //cmd_with_stb(gridSegments); // cmd 1 // PT6958 is fixed to 5 grids
      cmd_with_stb(0b01000000); // cmd 2 //Normal operation; Set pulse as 1/16
      
        digitalWrite(VFD_stb, LOW);
        delayMicroseconds(10);
        cmd_without_stb((0b11000000) | 0x00); //cmd 3 wich define the start address (00H to 15H)
          // Here you can adjuste which grid represent the values of clock
          // each grid use 2 bytes of memory registers
          //
          // The small board with the PT6958, have the sequence of GRID's as 4,2,3,1
          // by this reason is necessary check it.
          
          // Grid 1
          cmd_without_stb(numberA0);      // seconds unit
          cmd_without_stb(numberA1);      //
          
          // Grid 2
          if(flag==false){  // this IF is to blink the Point of middle of hours!
            cmd_without_stb(numberC0 | 0b00010000);      // Hours Units
            cmd_without_stb(numberC1);      // Only to used with digits drawed by 2 bytes.
           }
            else{
              cmd_without_stb(numberC0 | 0b00000000);      // Hours Units
              cmd_without_stb(numberC1);      // Only to used with digits drawed by 2 bytes.
            }
      
          // Grid 3
          cmd_without_stb(numberB0);      // seconds dozens  // dozens
          cmd_without_stb(numberB1);      //

          // Grid 4
          cmd_without_stb(numberD0);      // Minutes  dozens
          cmd_without_stb(numberD1);      // Only to used with digits drawed by 2 bytes.

      digitalWrite(VFD_stb, HIGH);
      delayMicroseconds(10);
      cmd_with_stb((0b10001000) | 7); //cmd 4
      delayMicroseconds(1);
}
void wrMemory_VersionHours(){
//Serial.println(number,HEX);
 
  digitalWrite(VFD_stb, LOW);
  delayMicroseconds(10);
      //cmd_with_stb(gridSegments); // cmd 1 // PT6958 is fixed to 5 grids
      cmd_with_stb(0b01000000); // cmd 2 //Normal operation; Set pulse as 1/16
      
        digitalWrite(VFD_stb, LOW);
        delayMicroseconds(10);
        cmd_without_stb((0b11000000) | 0x00); //cmd 3 wich define the start address (00H to 15H)
          // Here you can adjuste which grid represent the values of clock
          // each grid use 2 bytes of memory registers
          //
          // The small board with the PT6958, have the sequence of GRID's as 4,2,3,1
          // by this reason is necessary check it.
          
          // Grid 1
          cmd_without_stb(numberC0);      // seconds unit
          cmd_without_stb(numberC1);      //
          
          // Grid 2
          if(flag==false){  // this IF is to blink the Point of middle of hours!
            cmd_without_stb(numberE0 | 0b00010000);      // Hours Units
            cmd_without_stb(numberE1);      // Only to used with digits drawed by 2 bytes.
           }
            else{
              cmd_without_stb(numberE0 | 0b00000000);      // Hours Units
              cmd_without_stb(numberE1);      // Only to used with digits drawed by 2 bytes.
            }

          //Grid 3
          cmd_without_stb(numberD0);      // Minutes  dozens
          cmd_without_stb(numberD1);      // Only to used with digits drawed by 2 bytes.
          
          // Grid 4
          cmd_without_stb(numberF0);      // Minutes Units
          cmd_without_stb(numberF1);      // Only to used with digits drawed by 2 bytes.
          
      digitalWrite(VFD_stb, HIGH);
      delayMicroseconds(10);
      cmd_with_stb((0b10001000) | 7); //cmd 4
      delayMicroseconds(1);
}

void DigitTo7SegEncoder( unsigned char digit){
  // Note the array (segments[]) to draw the numbers is with 2 bytes!!! 20 chars, extract 2 by 2 the number you need. 
  switch(digit)
  {
    case 0:   number=0;       break;  // if remove the LongX, need put here the segments[x]
    case 1:   number=2;       break;
    case 2:   number=4;       break;
    case 3:   number=6;       break;
    case 4:   number=8;       break;
    case 5:   number=10;      break;
    case 6:   number=12;      break;
    case 7:   number=14;      break;
    case 8:   number=16;      break;
    case 9:   number=18;      break;
  }
} 
 
void adjustHMS(){
 // Important is necessary put a pull-up resistor to the VCC(+5VDC) to this pins (3, 4, 5)
 // if dont want adjust of the time comment the call of function on the loop
  /* Reset Seconds to 00 Pin number 3 Switch to GND*/
    if((AdjustPins & 0x08) == 0 )
    {
      _delay_ms(200);
      secs=00;
    }
    
    /* Set Minutes when SegCntrl Pin 4 Switch is Pressed*/
    if((AdjustPins & 0x10) == 0 )
    {
      _delay_ms(200);
      if(minutes < 59)
      minutes++;
      else
      minutes = 0;
    }
    /* Set Hours when SegCntrl Pin 5 Switch is Pressed*/
    if((AdjustPins & 0x20) == 0 )
    {
      _delay_ms(200);
      if(hours < 23)
      hours++;
      else
      hours = 0;
    }
}

void send7segm(){
  // This block is very important, it explain how solve the difference 
  // between segments from grids and segments wich start 0 or 1.
  
      //cmd_with_stb(gridSegments); // cmd 1 // This is not used, because the PT6958 is fixed to 5 grids and 10 segm
      cmd_with_stb(0b01000000); // cmd 2 //Normal operation; Set pulse as 1/16
      
        digitalWrite(VFD_stb, LOW);
        delayMicroseconds(1);
        cmd_without_stb((0b11000000)); //cmd 3 wich define the start address (00H to 15H)
        //
          cmd_without_stb(segments[k]);   // seconds unit
          cmd_without_stb(segments[k]);   // seconds dozens
          cmd_without_stb(segments[k]);     // minutes units
          cmd_without_stb(segments[k]);     // minutes dozens
          cmd_without_stb(segments[k]);     // hours units
          cmd_without_stb(segments[k]);     // hours dozens
          cmd_without_stb(segments[k]);     // hours third digit not used
          cmd_without_stb(segments[k]);     // hours dozens
          cmd_without_stb(segments[k]);     // hours third digit not used
          cmd_without_stb(segments[k]);     // hours dozens
          //cmd_without_stb(segments[k]);     // hours third digit not used
          //cmd_without_stb(segments[k]);     // hours dozens
      digitalWrite(VFD_stb, HIGH);
      cmd_with_stb((0b10001000) | 7); //cmd 4
      delay(1);
      delay(200);  
}

void readButtons(){
//Take special attention to the initialize digital pin LED_BUILTIN as an output.
//
int ledPin = 13;   // LED connected to digital pin 13
int inPin = 7;     // pushbutton connected to digital pin 7
int val = 0;       // variable to store the read value
int dataIn=0;

byte array[8] = {0,0,0,0,0,0,0,0};
byte together = 0;

unsigned char receive = 7; //define our transmit pin
unsigned char data = 0; //value to transmit, binary 10101010
unsigned char mask = 1; //our bitmask

array[0] = 1;

unsigned char btn1 = 0x41;

      digitalWrite(VFD_stb, LOW);
        delayMicroseconds(2);
      cmd_without_stb(0b01000010); // cmd 2 //Read Keys;Normal operation; Set pulse as 1/16
       // cmd_without_stb((0b11000000)); //cmd 3 wich define the start address (00H to 15H)
     // send without stb
  
  pinMode(7, INPUT);  // Important this point! Here I'm changing the direction of the pin to INPUT data.
  delayMicroseconds(2);
  //PORTD != B01010100; // this will set only the pins you want and leave the rest alone at
  //their current value (0 or 1), be careful setting an input pin though as you may turn 
  //on or off the pull up resistor  
  //This don't send the strobe signal, to be used in burst data send
         for (int z = 0; z < 5; z++){
             //for (mask=00000001; mask > 0; mask <<= 1) { //iterate through bit mask
                   for (int h =8; h > 0; h--) {
                      digitalWrite(VFD_clk, HIGH);  // Remember wich the read data happen when the clk go from LOW to HIGH! Reverse from write data to out.
                      delayMicroseconds(2);
                     val = digitalRead(inPin);
                      //digitalWrite(ledPin, val);    // sets the LED to the button's value
                           if (val & mask){ // if bitwise AND resolves to true
                             //Serial.print(val);
                            //data =data | (1 << mask);
                            array[h] = 1;
                           }
                           else{ //if bitwise and resolves to false
                            //Serial.print(val);
                           // data = data | (1 << mask);
                           array[h] = 0;
                           }
                    digitalWrite(VFD_clk, LOW);
                    delayMicroseconds(2);
                   } 
             
              Serial.print(z);  // All the lines of print is only used to debug, comment it, please!
              Serial.print(" - " );
                        
                                  for (int bits = 7 ; bits > -1; bits--) {
                                      Serial.print(array[bits]);
                                   }
                        
                          if (z==0){
                            if(array[1] == 1){
                                       if (flagSecs == true){
                                       secs++;
                                                if (secs == 60){
                                                minute ++;
                                                }
                                      }
                                      else{
                                        minutes++;
                                                if (minutes == 60){
                                                hours ++;
                                                }
                                      }
                            }
                          }
                          if (z==0){
                                if(array[4] == 1){
                                  if (flagSecs == true){
                                 secs--;
                                          if (secs == 0 and minute > 0){
                                          minute --;
                                          }
                                }
                                else{
                                  minutes--;
                                          if (minutes == 0 and hours > 0){
                                          hours --;
                                          }
                                }
                            }
                          }
                          if (z==3 & flagSecs==true){
                                if(array[2] == 1){
                                 secs=0;
                                 minute=0;
                                }
                          }
                          if (z==0 and flagSecs==true){
                                if(array[6] == 1){
                                 memory_secs=secs-1;
                                 memory_minutes=minute;
                                }
                          
                        }
                          if (z==0){
                            if(array[5] == 1){
                             flagSecs = !flagSecs;  // This change the app to hours or seconds
                            }
                        }
                        
                         
                        if (z==2){
                            if(array[4] == 1){
                              //digitalWrite(VFD_net, !digitalRead(VFD_net));
                          }
                        }
                        
                        if (z==3){
                           if(array[7] == 1){
                             //digitalWrite(VFD_onRed, !digitalRead(VFD_onRed));
                             //digitalWrite(VFD_onGreen, !digitalRead(VFD_onGreen));
                            }
                          }                        
                  Serial.println();
          }  // End of "for" of "z"
      Serial.println();  // This line is only used to debug, please comment it!

 digitalWrite(VFD_stb, HIGH);
 delayMicroseconds(2);
 cmd_with_stb((0b10001000) | 7); //cmd 4
 delayMicroseconds(2);
 pinMode(7, OUTPUT);  // Important this point!  // Important this point! Here I'm changing the direction of the pin to OUTPUT data.
 delay(1); 
}

void loop() {
// You can comment untill while cycle to avoid the test running.
// test_segments_and_grids();

    test_VFD();
    clear_VFD();
    test_LED();
  digitalWrite(VFD_onRed, HIGH); // To allow the swap pin value

   // Can use this cycle to teste all segments of VFD
   
       for(int h=0; h < 20; h++){
       k=h;
       send7segm();
       delay(50);
       }
  
  clear_VFD();
  
   while(1){
      send_update_clock();
      delay(100);
      readButtons();
      delay(100);
    digitalWrite(10, LOW);
   }

}

ISR(TIMER1_COMPA_vect)   {  //This is the interrupt request
                            // https://sites.google.com/site/qeewiki/books/avr-guide/timers-on-the-atmega328
      secs++;
      flag = !flag; 
} 
