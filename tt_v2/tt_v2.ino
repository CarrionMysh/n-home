
//код мастера

#include <FastCRC_tables.h>
#include <FastCRC.h>
#include <FastCRC_cpu.h>
#include <SoftwareSerial.h>
//для передачи в линию используется хардварный serial
#define pin_tr 4                           //пин transmission enable для max485
#define pin_led 13                         //светодиод активности *debag
#define tx_ready_delay 1                   //задержка для max485 (буфер? вообщем, неопределенность буфераmax485)
#define tx_pc 5                            //serial pc alfa
#define rx_pc 6                            //serial pc alfa
FastCRC8 CRC8;
SoftwareSerial pc(rx_pc, tx_pc);           //инициализация Serial для получения команд с компа, для альфы
byte self_id=1;
byte alien_id;                             //id, полученный от слейва
byte ok=94;                                //рабочий вариант ответа 94 '^'
byte packet_error = 99;                    // ошибка целостности пакета
byte data_error = 100;                     //ошибка целостности пакета
byte timeout_error = 93;
byte timeout_silent = 92;
byte data[254];                            //массив для данных
byte net_packet[6];
byte response;
//devel on
boolean flag_net;
boolean flag_data;                        //флаг наличия в пакете данных
//devel off
byte com_m;                               //byte индификатор_команды
byte id_m;                                //byte id
byte nn;                                  //размер блока передаваемых данных
unsigned int timeout_packet;              //таймаут приема пакета, мс

void setup(){
	Serial.begin(115200);
	pc.begin(115200);
	pinMode(pin_led, OUTPUT);                                                                 //пин светодиода
	pinMode(pin_tr, OUTPUT);                                                                   //пин передачи
	digitalWrite(pin_led, LOW);
	digitalWrite(pin_tr, LOW);
	pc.println(F("Serial ok"));
	timeout_packet = 250;
	nn = 0;
	flag_net = false;
	flag_data = false;
}
void loop(){
	id_m = 10;
	com_m = 18;                        // "отключить реле"
	nn = 0;
	trans_com(id_m,com_m,nn);
	delay (2000);
	//_________________
	// id_m = 10;
	// com_m = 19;                     // "включить реле"
	// trans_com(id_m,com_m,ask,false);
	// delay (2000);
	//_________________

	id_m = 10;
	com_m = 10;                        //"включить""
	data[0] = 0;                       //из принципа обмена: 0 = "все"
	nn=1;                              //количество данных 1 байт
	trans_com(id_m,com_m,nn);
	delay (2000);
	//_________________
	id_m = 10;
	com_m = 11;                        // "выключить"...
	data[0] = 1;                       //...первую лампу...
	data[1] = 3;                       //...третью лампу...
	data[2] = 5;                       //...пятую лампу
	nn=3;                              // количество данных 2 байта
	trans_com(id_m,com_m,nn);
	delay(2000);
	//_________________
	id_m = 10;
	com_m = 11;                        // "выключить"...
	data[0] = 0;                       // "все"
	nn=1;
	trans_com(id_m,com_m,nn);
	delay(2000);
}

byte trans_com(byte id, byte com_c, byte nn_data){
//_________________
//секция передачи
//_________________
	digitalWrite(pin_led, HIGH);       //моргаем
	digitalWrite(pin_tr, HIGH);        //max485 на вещание
	net_packet[0] = id;
	net_packet[1] = self_id;
	net_packet[2] = com_c;
	net_packet[3] = nn_data;
	Serial.print('>');                 //отдаем начало пакета
	Serial.print(char(CRC8.smbus(net_packet,4)));      //и туда же crc
	for (byte i = 0; i<4; i++) {                       //и туда же содержимое пакета
		Serial.print(char(net_packet[i]));
	}
	if(nn_data != 0) {                 //если передаваемое в функцию кол-во даты не нулевое
		Serial.print(char(CRC8.smbus(data, nn_data)));  //то вещаем CRC даты
		for (byte i=0; i<nn_data; i++) {                //и саму дату
			Serial.print(char(data[i]));
		}
	}
	delay(tx_ready_delay);                            //задержка для финиша передачи rs485
	digitalWrite(pin_tr, LOW);                        //max485 на прослушку линии
	digitalWrite(pin_led, LOW);                       //гасим моргалку
//_________________
//секция прослушки
//после передачи пакета (+дата) ждем подтверждения приема от слэйва
//_________________
	char ch;
	unsigned int count;
	boolean begin_of_packet = false;
	byte crc_incoming;
	byte crc_calc;
	unsigned int timeout_tick = 50;          //от фонаря (пока)
	unsigned long time_tick = millis();
  unsigned int timeout_response = 50;
  unsigned long time_packet = millis();
	response = 0;                            //сие есть аналог com для слэйва, код ответа от слэйва
	flag_data = false;                       //флаг наличия даты, пока никак не задействован
	while(true) {
    if (begin_of_packet && (millis()-time_tick > timeout_tick)){  //проверка таймаута между байтами
      pc.println("timeout_error_tick1");
      return (timeout_error);
    }
    if (!begin_of_packet && (millis()-time_packet > timeout_packet)){
      pc.println("timeout_silent");
      return (timeout_silent);
    }
		if (Serial.available()) {
			ch = Serial.read();
			if ((ch == '>') && !begin_of_packet) {      //проверка на начало пакета
				begin_of_packet = true;
				count = 0;
				time_tick = millis();                  //здесь и далее - обновляем время для вновь пришедшего байта
			}
			if (begin_of_packet) {                   //и если начало пакета было, дальше пляшем от этого
				net_packet[count] = ch;
				time_tick = millis();
        pc.print("count=");pc.print(count);pc.print(" ch=");pc.println(net_packet[count]);
				if (count == 5) {                       //по "дизайну" у нас пакет длиной в шесть байт
					crc_incoming = net_packet[1];
					crc_calc = CRC8.smbus(&net_packet[2],4);
					if (crc_incoming == crc_calc) {
						if (net_packet[2] == self_id) {
							response = net_packet[4];
							alien_id = net_packet[3];
							nn = net_packet[5];
							if (nn != 0) {
								count = 0;
								while (count <= nn) {
                  if (millis()-time_tick > timeout_tick){
                    pc.println("timeout_error2");
                    return (timeout_error);
                  }
									if (Serial.available()) {
										ch = Serial.read();
										if (count == 0) {
											crc_incoming = ch;
										}
										else {
											data[count-1] = ch;
										}
										count++;
										time_tick = millis();
									}
								}
								crc_calc = CRC8.smbus(data,nn);
								if (crc_calc == crc_incoming) {
									flag_data = true;
                  pc.println("data_ok");
									return (ok);
								}
								else{
                  pc.println("data_crc_error");
									return (data_error);
								}
							}
              pc.println("packet_ok");
							return (ok);
						}
            pc.println("wrong_id");//не свой id. есть мысли логировать и чужие пакеты. пока пропускаем
					}
					else {
            pc.println("packet_crc_error");
						return (packet_error);
					}
				}
				count++;
			}
		}
	}
}
