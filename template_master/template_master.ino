
//код мастера
//для передачи в линию используется хардварный serial
#include <FastCRC_tables.h>
#include <FastCRC.h>
#include <FastCRC_cpu.h>
#include <SoftwareSerial.h>
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
	pinMode(pin_led, OUTPUT);              //пин светодиода
	pinMode(pin_tr, OUTPUT);               //пин передачи
	digitalWrite(pin_led, LOW);
	digitalWrite(pin_tr, LOW);
	pc.println(F("Serial ok"));
	timeout_packet = 250;
	nn = 0;
	flag_net = false;
	flag_data = false;

}
void loop(){
  //пример on
	id_m = 10;
	com_m = 18;                        // "отключить реле"
	nn = 0;
	trans_com(id_m,com_m);
	delay (2000);
  //пример off

}

byte trans_com(byte id, byte com_c){
//
//секция передачи
//
	digitalWrite(pin_led, HIGH);       //моргаем
	digitalWrite(pin_tr, HIGH);        //max485 на вещание
	net_packet[0] = id;
	net_packet[1] = self_id;
	net_packet[2] = com_c;
	net_packet[3] = nn;
	Serial.print('>');                                 //отдаем начало пакета
	Serial.print(char(CRC8.smbus(net_packet,4)));      //и туда же crc
	for (byte i = 0; i<4; i++) {                       //и туда же содержимое пакета
		Serial.print(char(net_packet[i]));
	}
	if(nn != 0) {                                //если передаваемое в функцию кол-во даты не нулевое
		Serial.print(char(CRC8.smbus(data, nn)));  //то вещаем CRC даты
		for (byte i=0; i<nn; i++) {                //и саму дату
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
	unsigned int timeout_tick = 10;          //от фонаря (пока)
	unsigned long time_tick = millis();
	unsigned int timeout_response = 50;
	Serial.setTimeout(timeout_tick);
	response = 0;                            //сие есть аналог com для слэйва, код ответа от слэйва
	flag_data = false;                       //флаг наличия даты, пока никак не задействован
	while(true) {                            //слушаем "вечно"
		if ((millis()-time_tick) > timeout_response) {
			pc.println("timeout_silent");
			return (timeout_silent);
		}
		if (Serial.available()) {
			ch = Serial.read();
			time_tick = millis();
			if (ch == '>') {                                //видим начало пакета
				Serial.readBytes(net_packet,5);               //читаем с линии пять байтов пакета
				if ((millis()-time_tick) > timeout_tick) {    //если вылетели по таймауту Serial, мы этого не узнаем, поэтому проверям таймаут руками
					pc.println("timeout_error_packet");
					return(timeout_error);
				}
				crc_incoming = net_packet[0];
				crc_calc = CRC8.smbus(&net_packet[1],4);       //для проверки отбрасываем приходящее CRC в пакете
				if (crc_incoming == crc_calc) {                //CRC верно
					if (net_packet[1] == self_id) {              //наш ли пакет
						alien_id = net_packet[2];
						response = net_packet[3];
						nn = net_packet[4];
						if (nn != 0) {                             //если 5 байт не нулевой, значит будет data
							pc.println("data_find");
							time_tick = millis();
      //ловим дату
							while(true) {
								if (Serial.available()) {
									crc_incoming = Serial.read();        //первый байт это crc
									Serial.readBytes(data,nn);           //и читаем-пишем data
									break;                               //ломаем цикл даты
								}
								if ((millis()-time_tick) > timeout_tick) { //страховка, если даты вообще не будет. проверка с времени последнего байта
									pc.println("data_silence");
									return (data_error);
								}
							}
							//дата залилась, иначе уловие выше выбьет таймауту
							crc_calc = CRC8.smbus(data,nn);            //crc даты
							if (crc_incoming == crc_calc) {            //проверка на crc дату
								flag_data = true;
								pc.println("data_ok");
								pc.print("data[nn]="); pc.println(data[nn]);
								return (ok);
							}
							else {
								pc.println("data_crc_error");
								return (data_error);
							}
						}
						else { //5 байт нулевой, даты не будет
							pc.println("packet_ok");
							return (ok);
						}
					}
					else { // пакет не нам
						pc.println("wrong_id");
					}
				}
				else {
					pc.println("packet_crc_error");
					return (packet_error);
				}
			}
		}
	}
}
