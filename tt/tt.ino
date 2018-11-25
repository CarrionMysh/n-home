
//код мастера

#include <FastCRC_tables.h>
#include <FastCRC.h>
#include <FastCRC_cpu.h>
#include <SoftwareSerial.h>
//для передачи в линию используется хардварный serial
#define pin_tr 4                                    //пин transmission enable для max485
#define led_pin 13                                //светодиод активности *debag
#define tx_ready_delay 1                   //задержка для max485 (буфер? вообщем, неопределенность буфераmax485)
#define tx_pc 5                                   //serial pc alfa
#define rx_pc 6                                   //serial pc alfa
#define value_data  10                        //размер пакета
const char ask='!';      //тип пакета
FastCRC8 CRC8;
SoftwareSerial pc(rx_pc, tx_pc);        //инициализация Serial для получения команд с компа, для альфы
//devel on
boolean flag_net;
//devel off
//char net_packet[value_data];
byte com_m;             //byte индификатор_команды
byte id_m;             //byte id
byte data[256];           //массив для данных
byte nn;              //размер блока передаваемых данных
byte responce;  //ответ от слейва полученный с линии

void setup(){
	Serial.begin(115200);
	pc.begin(115200);
	pinMode(led_pin, OUTPUT);                                                                 //пин светодиода
	pinMode(pin_tr, OUTPUT);                                                                   //пин передачи
	digitalWrite(led_pin, LOW);
	digitalWrite(pin_tr, LOW);
	pc.println(F("Serial ok"));
	nn = 0;
}
void loop(){
	id_m = 10;
	com_m = 3;
	trans_com(id_m,com_m,ask,false);
	delay (2000);
	com_m = 12;
	trans_com(id_m,com_m,ask,false);
	delay (2000);
}

void trans_com(byte id, byte com, char type_packet, boolean data_b){   //функция передачи в линию
	byte mm;
	if (data_b) {mm=(nn+3);
	} else mm=3;
	byte packet [mm];
	//byte crc;
	//byte count = 0;
	digitalWrite(led_pin, HIGH);
	digitalWrite(pin_tr, HIGH);
	packet[0] = id;
	packet[1] = byte(type_packet);
	packet[2] = com;
	if (data_b) {                               //если данные нужно передавать, дополнительно грузим в массив блок данных, количеством nn
		for (byte i=0; i<=nn; i++) {
			packet[3+i] = data[i];
		}
		packet[3+nn] = byte('<');                                                                                                                                                                                                                                   //и в конец помещаем '<'
	} else packet[3] = byte('<');        //иначе просто конец пакета

	Serial.print('>');
	pc.print("crc");pc.println(CRC8.smbus(packet, sizeof(packet)));
	Serial.print(char(CRC8.smbus(packet, sizeof(packet))));
	for (byte i=0; i<=sizeof(packet); i++) {
		Serial.print (char(packet[i]));
	}

	digitalWrite(led_pin, LOW);
	delay(tx_ready_delay);
	digitalWrite(pin_tr, LOW);
}


// void recive_com(){   //прием пакета
// 	byte count = 0;
// 	char ch;
// 	boolean begin_of_packet;
// 	begin_of_packet = false;
// 	//devel_on
// 	flag_net = false;
// 	//devel_off
// 	digitalWrite(pin_tr, LOW);
// 	while (true) {
// 		if(Serial.available()) {
// 			//digitalWrite(led_pin, HIGH);
// 			//devel_on
// 			flag_net = true;
// 			//devel_off
// 			ch = Serial.read();                                                                                                                                                                                                                                                                                                                                                    //читаем что прилетело, заодно чистим буфер если сыпется мусор на линии
// 			if (ch == '>' && !begin_of_packet) {
// 				begin_of_packet = true;
// 			}
// 			if (begin_of_packet) {                                                                                                                                                                                                                                                                                                                                                    //если был начало пакета '>'
// 				net_packet[count] = ch;                                                                                                                                                                                                                                                                                                                                                                                                                                                                     // пишем в пакет
// 				if (net_packet[count] == '<') {
// 					//digitalWrite(led_pin, LOW);
// 					net_packet[count+1]='\0';
// 					prep_responce_com();
// 					break;
// 				}
// 				count++;
// 			}
// 		}
// 	}
// }
//
// void prep_responce_com(){
// 	byte count = 0;
// 	while (net_packet[count] != '!') {                                                                                                                  //ищем в пакете ask
// 		count++;
// 	}
// 	responce = 10 * (net_packet[count+1] - '0')+(net_packet[count+2] - '0');
// }
