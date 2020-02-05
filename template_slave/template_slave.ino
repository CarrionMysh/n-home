//код слэйва
//для дебага раскомментировать pc.
#include <FastCRC_tables.h>
#include <FastCRC.h>
#include <FastCRC_cpu.h>
#include <SoftwareSerial.h>
#define pin_tr 4                          //пин transmission enable для max485
#define tx_ready_delay 1                  //задержка для max485 (буфер? вообщем, неопределенность буфераmax485)
#define pin_relay 7                       //пин для реле
#define tx_pc 5                           //serial pc alfa
#define rx_pc 6
volatile byte triac_level_bright[7];      //заданный уровень яркости 128..0 для 8 симмисторов
const byte step_bri = 128;                //количество уровней яркости 0 = ON, 128 = OFF
byte triacs = 8;                          //количество симмисторов/каналов
byte self_id=10;                          //собвственный id
byte alien_id;                            //id не свой
byte ok=94;                               //рабочий вариант ответа 94 '^'
byte packet_error = 99;                   //ошибка целостности пакета
byte data_error = 100;                    //ошибка целостности пакета
byte timeout_error = 93;                  //ошибка по таймауту
byte data[254];                           //массив для данных
byte net_packet[6];                       //массив пакета
byte nn;                                  //размер блока ланных
byte com;                                 //команда полученная с линии
boolean flag_net;                         //флаг получения пакета/unused
boolean flag_data;                        //флаг наличия в пакете данных/unused
FastCRC8 CRC8;                            //инициализация функции CRC
SoftwareSerial pc(rx_pc, tx_pc);          //Serial на комп. в основе для дебага

void setup() {
	Serial.begin(115200);
  pc.begin(115200);
	pinMode(pin_tr, OUTPUT);
	pinMode(pin_relay, OUTPUT);
	digitalWrite(pin_tr, LOW);
	digitalWrite(pin_relay, LOW);
}

void loop(){
	recive_com();
	job();
}

void job(){                                    //обработчик команд

}

byte recive_com(){                             //прием пакета
	char ch;
	unsigned int count;                          //счетчик байтов с линии
	boolean begin_of_packet = false;             //флаг начала пакета
	byte crc_incoming;
	byte crc_calc;
	unsigned long timeout_tick = 10;             //таймаут между двумя байтами
	unsigned long time_tick;                     //время прихода последнего байта
	flag_data = false;                           //флаг наличия даты, пока никак не задействован
	time_tick = millis();
	Serial.setTimeout(timeout_tick);
	while(true) {                                //слушаем "вечно"
		if (Serial.available()) {
			ch = Serial.read();
			time_tick = millis();
			if (ch == '>') {                         //видим начало пакета
				Serial.readBytes(net_packet,5);        //читаем с линии пять байтов пакета
				if ((millis()-time_tick) > timeout_tick) {    //если вылетели по таймауту Serial, мы этого не узнаем, поэтому проверям таймаут руками
					pc.println("timeout_error_packet");
					return(timeout_error);
				}
				crc_incoming = net_packet[0];
				crc_calc = CRC8.smbus(&net_packet[1],4);      //для проверки отбрасываем приходящее CRC в пакете
				if (crc_incoming == crc_calc) {               //CRC верно
					if (net_packet[1] == self_id) {             //наш ли пакет
						alien_id = net_packet[2];
						com = net_packet[3];
						nn = net_packet[4];
						if (nn != 0) {                            //если 5 байт не нулевой, значит будет data
							pc.println("data_find");
							time_tick = millis();
							while(true) {
								if (Serial.available()) {             //ловим дату
									crc_incoming = Serial.read();       //первый байт это crc
									Serial.readBytes(data,nn);          //и читаем-пишем data
									break;                              //ломаем цикл даты
								}
								if ((millis()-time_tick) > timeout_tick) {      //страховка, если даты вообще не будет. проверка с времени последнего байта
									pc.println("data_silence");
									return (data_error);
								}
							}
							//дата залилась, иначе уловие выше выбьет таймауту
							crc_calc = CRC8.smbus(data,nn);          //crc даты
							if (crc_incoming == crc_calc) {          //проверка на crc дату
								flag_data = true;
								pc.println("data_ok");
								return (ok);
							}
							else {
								pc.println("data_crc_error");
								return (data_error);
							}
						}
						else {                                      //5 байт нулевой, даты не будет, возвращаем "ок"
							pc.println("packet_ok");
							return (ok);
						}
					}
					else {                                        // пакет не нам
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

  //секция ответа
void response(byte id, byte com_c, byte nn_data){
	digitalWrite(pin_tr, HIGH);
	net_packet[0] = id;
	net_packet[1] = self_id;
	net_packet[2] = com_c;
	net_packet[3] = nn_data;
	Serial.print('>');
	Serial.print(char(CRC8.smbus(net_packet,4)));
	for (byte i = 0; i<4; i++) {
		Serial.print(char(net_packet[i]));
	}
	if(nn_data != 0) {
		delay(10);
		Serial.print(char(CRC8.smbus(data, nn_data)));
		Serial.write(data,nn_data);
	}
	delay(tx_ready_delay);
	digitalWrite(pin_tr, LOW);
}
