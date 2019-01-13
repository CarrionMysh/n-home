
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
#define value_data  261                       //размер пакета packet+data
const char ask='!';      //тип пакета
FastCRC8 CRC8;
SoftwareSerial pc(rx_pc, tx_pc);        //инициализация Serial для получения команд с компа, для альфы
byte self_id=1;
byte alien_id;                                          //id, полученный от слейва
byte data[254];           //массив для данных
//devel on
boolean flag_net;
boolean flag_data;    //флаг наличия в пакете данных
//devel off
byte com_m;             //byte индификатор_команды
byte id_m;             //byte id
byte nn;              //размер блока передаваемых данных
byte responce=0;  //ответ от слейва полученный с линии
unsigned long timeout_packet;           //таймаут приема пакета, мс

void setup(){
	Serial.begin(115200);
	pc.begin(115200);
	pinMode(led_pin, OUTPUT);                                                                 //пин светодиода
	pinMode(pin_tr, OUTPUT);                                                                   //пин передачи
	digitalWrite(led_pin, LOW);
	digitalWrite(pin_tr, LOW);
	pc.println(F("Serial ok"));
	timeout_packet = 250;
	nn = 0;
	flag_net = false;
}
void loop(){
	id_m = 10;
	com_m = 3;
	trans_com(id_m,com_m,ask,false);
		pc.println();pc.print("data=");
		// pc.print(data[0]);
		// pc.print(data[1]);
		for (byte i=0; i<=nn; i++){
			pc.print(char(data[i]));
		}
		pc.println();
	delay (2000);
	com_m = 12;
	trans_com(id_m,com_m,ask,false);
	delay (2000);
}

void trans_com(byte id, byte com, char type_packet, boolean data_b){   //функция передачи в линию
	byte mm;
	if (data_b) {mm=(nn+3); } else mm=3;
	byte packet [mm];
	pc.println();pc.print(F("Transmit:")); pc.print(F(" id=")); pc.print(id); pc.print(F(" com=")); pc.println(com);
	digitalWrite(led_pin, HIGH);
	digitalWrite(pin_tr, HIGH);
	packet[0] = id;
	packet[1] = byte(type_packet);
	packet[2] = com;
	if (data_b) {                               //если данные нужно передавать, дополнительно грузим в массив блок данных, количеством nn
		packet[3] = nn;                                                                                         //если данные, то помещаем 4 байтом размер данных
		for (byte i=0; i<=nn; i++) {
			packet[4+i] = data[i];                                                  //если данные - льём их в пакет
		}
		packet[4+nn] = byte('<');                                                                                                                                                                                                                                   //и в конец помещаем '<'
	} else packet[3] = byte('<');        //иначе просто конец пакета

	Serial.print('>');
	//pc.print(" crc"); pc.println(CRC8.smbus(packet, sizeof(packet)));
	Serial.print(char(CRC8.smbus(packet, sizeof(packet))));
	//pc.print("sizeof="); pc.println(sizeof(packet));
	for (byte i=0; i<=sizeof(packet); i++) {
		Serial.print (char(packet[i]));
	}

	digitalWrite(led_pin, LOW);
	delay(tx_ready_delay);
	digitalWrite(pin_tr, LOW);

	recive_com(id);
	if (flag_net){
		pc.println();pc.print(F("response: "));pc.print(F("id="));pc.print(alien_id);pc.print(F(" com="));pc.println(responce);
	}
	else{
		pc.println();pc.println(F("not response"));
	}
}


void recive_com(byte id){                      //прием пакета
	int count = 0;
	char net_packet[value_data];
	count = 0;
	char ch;
	unsigned long time_n;
	boolean begin_of_packet;
	begin_of_packet = false;
	//devel_on
	flag_net = false;
	//devel_off
	digitalWrite(pin_tr, LOW);
	time_n = millis();
	while (true) {
		if(Serial.available()) {
			//devel_on
			//flag_net = true;
			//devel_off
			ch = Serial.read();                                     //читаем что прилетело, заодно чистим буфер если сыпется мусор на линии
			//pc.print(ch);
			if (ch == '>' && !begin_of_packet) {
				begin_of_packet = true;

			}
			if (begin_of_packet) {                          //если был начало пакета '>'
				net_packet[count] = ch;                         // пишем в пакет
				if (net_packet[count] == '<') {
					// pc.println(); pc.print("crc_c="); pc.println(crc_c());
					byte crc_incoming;                                                                                              //начало функции высчитывания crc
					byte crc_calc;
					byte buf[count-2]; //-2 crc идет вторым байтом, поэтому считаем с третьего
					for (byte i=2; i<=count; i++) {
						buf[i-2] = byte(net_packet[i]);
					}
					crc_incoming = byte(net_packet[1]);
					crc_calc = CRC8.smbus(buf, sizeof(buf));        //конец функции подсчета crc
					if (crc_incoming == crc_calc) {
						if ((byte(net_packet[2])) == id) { //если id верный, то
							responce = byte (net_packet[4]);        //получаем команду
							if (net_packet[5] != '<') {             //проверяем наличие данных и если есть - пишем
								pc.print("data_nn=");pc.print(byte(net_packet[5]));
								flag_data = true;
								flag_net = true;
								nn = net_packet[5];
								pc.println();pc.print("debug=");
								for (byte i=0; i<nn; i++) {
									data[i] = net_packet[6+i];
									pc.print(char(data[i]));
								}
							} else {
								flag_data = false;
								flag_net = true;
							}
							break;
						} else {
							begin_of_packet = false;
							count = 0;
						}
					}
				}
				if ((millis()-time_n) > timeout_packet) {
					pc.println();pc.println("begin timeout");
					break;
					// begin_of_packet = false;
					// count = 0;
				} else count++;
			}
		}
		if ((millis()-time_n) > timeout_packet) {
			pc.println();pc.println("net timeout");
			break;
		}
	}
}
