#ifndef n_home_h
#define n_home_h
#include <Arduino.h>
#include <FastCRC_tables.h>
#include <FastCRC.h>
#include <FastCRC_cpu.h>
#include <SoftwareSerial.h>



class NetHome {
public:
  NetHome (byte self_id, byte pin_tr, byte tx_ready_delay, unsigned long speed = 115200);
  void begin();
  void transmite(byte id, byte com_c, byte nn_data = 0);
  byte recive(boolean mode);           //"true" -- постоянная прослушка линии
  void setData(byte i, byte data_n);
  byte getData(byte i);
  byte getCom();
  void setTimeout(unsigned long timeout_tick = 10);
private:
  unsigned long _timeout_response = 50;
  byte _self_id;
  byte _pin_tr;
  byte _tx_ready_delay;
  byte _nn;
  unsigned long _speed;
  byte _id;
  byte _com;
  byte _net_packet[6];
  byte _data[254];
  unsigned long _timeout_tick = 10;
  byte _ok=94;                               //рабочий вариант ответа 94 '^'
  byte _packet_error = 99;                   //ошибка целостности пакета
  byte _data_error = 100;                    //ошибка целостности пакета
  byte _timeout_error = 93;                  //ошибка по таймауту
  byte timeout_silent = 92;
  byte _alien_id;                            //id не свой
};

#endif
