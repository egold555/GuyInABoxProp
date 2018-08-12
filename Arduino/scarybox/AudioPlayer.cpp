#include <Arduino.h>
#include "AudioPlayer.h"
#include <SoftwareSerial.h>

static int8_t Send_buf[8] = {0};

SoftwareSerial mySerial = SoftwareSerial(-1,-1); //stupid

void setupAudio(int rx, int tx){
    mySerial = SoftwareSerial(rx, tx);
    mySerial.begin(9600);
    delay(500);//Wait chip initialization is complete
    sendAudioCommand(CMD_SEL_DEV, DEV_TF);//select the TF card  
    delay(200);//wait for 200ms 
}

void playAudio(int8_t file){
    sendAudioCommand(CMD_PLAY_WITHVOLUME, 0X6400 + file);
}

void sendAudioCommand(int8_t command, int16_t dat)
{
 delay(20);
 Send_buf[0] = 0x7e; //starting byte
 Send_buf[1] = 0xff; //version
 Send_buf[2] = 0x06; //the number of bytes of the command without starting byte and ending byte
 Send_buf[3] = command; //
 Send_buf[4] = 0x00;//0x00 = no feedback, 0x01 = feedback
 Send_buf[5] = (int8_t)(dat >> 8);//datah
 Send_buf[6] = (int8_t)(dat); //datal
 Send_buf[7] = 0xef; //ending byte
 for(uint8_t i=0; i<8; i++)//
 {
   mySerial.write(Send_buf[i]) ;//send bit to serial mp3
   //Serial.print(Send_buf[i],HEX);//send bit to serial monitor in pc
 }
 //Serial.println();
}