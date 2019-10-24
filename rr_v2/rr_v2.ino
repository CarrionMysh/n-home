//код слэйва

#include <FastCRC_tables.h>
#include <FastCRC.h>
#include <FastCRC_cpu.h>
#include <SoftwareSerial.h>

#define pin_tr 4                          //пин transmission enable для max485
#define tx_ready_delay 1                  //задержка для max485 (буфер? вообщем, неопределенность буфераmax485)
#define pin_relay 7             //пин для реле
#define tx_pc 5                                   //serial pc alfa
#define rx_pc 6
volatile byte triac_level_bright[7];      //заданный уровень яркости 128..0 для 8 симмисторов
const byte step_bri = 128;                //количество уровней яркости 0 = ON, 128 = OFF
byte triacs = 8;                          //количество симмисторов/каналов
byte self_id=10;
byte alien_id;
byte ok=94;                     //рабочий вариант ответа 94 '^'
byte packet_error = 99;         // ошибка целостности пакета
byte data_error = 100;          //ошибка целостности пакета
byte timeout_error = 93;
byte data[254];       //массив для данных
byte net_packet[5];
byte nn;              //размер блока ланных
byte com;                               //команда полученная с линии
unsigned int timeout_packet = 10;
boolean flag_net;               //флаг получения пакета
boolean flag_data;    //флаг наличия в пакете данных
FastCRC8 CRC8;
SoftwareSerial pc(rx_pc, tx_pc);                                   //serial_pc

void setup() {
	Serial.begin(115200);
	pinMode(pin_tr, OUTPUT);
	pinMode(pin_relay, OUTPUT);
	digitalWrite(pin_tr, LOW);
	digitalWrite(pin_relay, LOW);
	pc.begin(115200);
}

void loop(){
	recive_com();
	job();
}

void job(){                                     //обработчик команд
	pc.println();
	pc.println("_________________");
	pc.print("com: "); pc.println(com);
	switch (com) {
	case 10:                                      //"включить яркостью"
		digitalWrite(pin_relay, LOW);         //включаем реле
		if (data[0] == 0) {                             //"все"
			for (byte i = 0; i<triacs; i++) {
				triac_level_bright[i] = data[1];            //яркостью data[1]
			}
		}
		else {                                          //не "все", а именно...
			for (byte i = 2; i<nn; i++) {                //i=2 т.к. первый байт data[0] признак количества, второй data[1] - яркость
				triac_level_bright[data[i]] = data[1];         //дергаем номер симистора из блока данных, яркость data[1]
			}
		}
		response(self_id, ok, 0);
		break;
	case 11:                                          //"выключить"
		if (data[0] == 0) {                             //"все"
			for (byte i = 0; i<triacs; i++) {
				triac_level_bright[i] = step_bri;                //яркость 128 = выкл.
			}
		}
		else {                                          //не "все", а именно...
			for (byte i = 1; i<nn; i++) {                //количество ламп определяется количеством данных в data[] минус признак количества data[0]
				triac_level_bright[data[i]] = step_bri;         //дергаем из data[] номера симисторов и гасим их яркостью 128
			}
		}
		response(self_id, ok, 0);
		break;
	case 12:                                          //"убавить"
		if (data[0] == 0) {                             //"все"
			for (byte i = 0; i<triacs; i++) {
				int temp_bri = triac_level_bright[i] + data[1];         //проверям переполнение вычисленной яркости
				if (temp_bri > step_bri) temp_bri=step_bri;
				triac_level_bright[i] = temp_bri;               //и устанавливаем вычисленную яркость.
			}
		}
		else {                              //не "все", а именно...
			for (byte i = 2; i<nn; i++) {         //i=2 т.к. первый байт data[0] признак количества, второй data[1] - декремент яркости
				int temp_bri = triac_level_bright[data[i]] + data[1];         //проверям переполнение вычисленной яркости1
				if (temp_bri > step_bri) temp_bri=step_bri;
				triac_level_bright[data[i]] = temp_bri;               //и устанавливаем вычисленную яркость симмистора с номером из блока данных
			}
		}
		break;
	case 13:                        //"прибавить"
		if (data[0] == 0) {         //"все"
			for (byte i = 0; i<triacs; i++) {
				int temp_bri = triac_level_bright[i] - data[1];         //проверям переполнение вычисленной яркости. data[1] - инкремент яркости.
				if (temp_bri < 0 ) temp_bri=0;         //'0' - максимальная яркость
				triac_level_bright[i] = temp_bri;         //и устанавливаем вычисленную яркость.
			}
		}
		else {                       //не все, а именно...
			for (byte i=2; i <nn; i++) {
				int temp_bri = triac_level_bright[data[i]] - data[1];
				if (temp_bri < 0) temp_bri=0;
				triac_level_bright[data[i]] = temp_bri;         //и устанавливаем вычисленную яркость симмистора с номером из блока данных
			}
		}
		break;
	case 14:                        //"калибровка"
		//пока пусто//
		break;
	case 15:                        //"выполнить эффект"
		//пока пусто//
		break;
	case 16:                        //"установить эффект включения"
		//пока пусто//
		break;
	case 17:                        //"установить эффект выключения"
		//пока пусто//
		break;
	case 18:                        //"отключить реле"
		digitalWrite(pin_relay, HIGH);
		response(self_id, ok, 0);
		break;
	case 19:                        //"включить реле"
		digitalWrite(pin_relay, LOW);
		response(self_id, ok, 0);
		break;
	}
	com = 0;                        //обнуляем значение команды после отработки
}

byte recive_com(){                //прием пакета
	char ch;
	unsigned int count;             //счетчик байтов с линии
	boolean begin_of_packet = false;         //флаг начала пакета
	byte crc_incoming;
	byte crc_calc;
	unsigned int timeout_tick = 50;          //таймаут между двумя байтами
	unsigned long time_tick = millis();
	// flag_net = false;
	flag_data = false;                       //флаг наличия даты, пока никак не задействован
	while(true) {                            //слушаем "вечно"
		if (begin_of_packet && (millis()-time_tick > timeout_tick)) {     //проверям таймаут
			return(timeout_error);
		}
		if (Serial.available()) {
			ch=Serial.read();
			if (ch = '>' && !begin_of_packet) {  //проверка на начала пакета
				begin_of_packet = true;
				count = 0;
				time_tick = millis();
			}
			if (begin_of_packet) {               //и если начало пакета было, дальше пляшем от этого
				net_packet[count] = ch;
				time_tick = millis();
				if (count == 5) {                  //по "дизайну" у нас пакет длиной в пять байт
					crc_incoming = net_packet[1];
					crc_calc = CRC8.smbus(&net_packet[2],4); //в пакете отбрасываем признак начала пакета и CRC
					if (crc_incoming == crc_calc) {          //проверяем crc
						if(net_packet[2] == self_id) {         //проверяем нам ли пакет
							com = net_packet[4];
							alien_id = net_packet[3];
							nn = net_packet[5];
							if (nn != 0) {                       //если 6 байт не нулевой, значит будет дата
								count = 0;                         //и заново начинаем принимать, уже дату
								while (count <= nn) {              //и покрутилось все вновь, CRC+nn
									if (millis()-time_tick > timeout_tick) {   //проверка на таймаут
										return(timeout_error);
									}
									if (Serial.available()) {
										ch = Serial.read();
										if (count == 0) {              //первый байт после пакета - CRC блока даты
											crc_incoming = ch;
										}
										else {
											data[count-1] = ch;          //иначе пишем дату
										}
										count++;
										time_tick = millis();
									}
								}
								crc_calc = CRC8.smbus(data,nn);    //crc даты
								if (crc_incoming == crc_calc) {
									flag_data = true;
									return (ok);
								}
								else{
									return (data_error);
								}
							}
							return (ok);
						} //не наш id -- не реагируем.
					}
					else {
						return (packet_error);
					}
				}
				count++;                      //счетчик байтов. все проверили, отработали - щелкнули
			}
		}
	}
}

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
		Serial.print(char(CRC8.smbus(data, nn_data)));
		for (byte i=0; i<nn_data; i++) {
			Serial.print(char(data[i]));
		}
	}
	delay(tx_ready_delay);
	digitalWrite(pin_tr, LOW);
}
