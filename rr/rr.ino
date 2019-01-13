//код слэйва

#include <FastCRC_tables.h>
#include <FastCRC.h>
#include <FastCRC_cpu.h>
#include <SoftwareSerial.h>

#define pin_tr 4            //пин transmission enable для max485
#define led_pin 13        //светодиод активности *debag
#define tx_ready_delay 1   //задержка для max485 (буфер? вообщем, неопределенность буфераmax485)
#define value_data  261                        //размер пакета packet+data
FastCRC8 CRC8;
byte for_master = 1;				//id мастера
byte self_id=10;
const char ask='!';
byte ok=94;                                                                                     //рабочий вариант ответа 94 '^'
//devel on
#define exec_pin1 9             //пин для реле
boolean flag_net;                       //флаг получения пакета
boolean flag_data;    //флаг наличия в пакете данных
byte data[254];       //массив для данных
byte nn;              //размер блока ланных
//devel off
//devel on
#define tx_pc 5                                   //serial pc alfa
#define rx_pc 6                                   //serial pc alfa
SoftwareSerial pc(rx_pc, tx_pc);
//debug off

unsigned long timeout_packet;           //таймаут приема пакета, при наличии, мс
unsigned long timeout_tick;							//таймаут прослушки линии без пакета, чтобы не висеть постояно в цикле прослушки, мс
//byte count;
byte com;               //команда полученная с линии

void setup() {
	Serial.begin(115200);
	pinMode(led_pin, OUTPUT);
	pinMode(pin_tr, OUTPUT);
	pinMode(exec_pin1, OUTPUT);
	digitalWrite(led_pin, LOW);
	digitalWrite(pin_tr, LOW);
	digitalWrite(exec_pin1, LOW);
	// count = 0;
	timeout_packet = 250;
	timeout_tick = 100;
	//debug on
	pc.begin(115200);
	//debug off
}

void loop(){
	recive_com();
	//devel_on
	if (flag_net) {
		pc.println("_________________");
		pc.print("com: "); pc.println(com);
	switch (com) {                                                          //тестовая заготовка обработки пакетов
	case 3:
		digitalWrite(exec_pin1, HIGH);
		digitalWrite(led_pin, HIGH);
		if (flag_data) read_data();																	//полчили данные и пишем их куда-то
		write_data();																	//где-то считали данные и пишем в дату
		response(self_id, ok, '&', true);
		break;
	case 12:
		digitalWrite(exec_pin1, LOW);
		digitalWrite(led_pin, LOW);
		response(self_id, ok, '&', false);
		break;
	}
	//devel_off
 }
}

void read_data(){
	pc.println();pc.print("readdata=");
	for (byte i=0; i<nn; i++){
		pc.print(char(data[i]));
	}
	pc.println();
}

void write_data(){
	nn = 10;
	for (byte i=0; i<nn; i++){
		data[i]=i+65;
	}
}

void recive_com(){                      //прием пакета
	int count = 0;
	char net_packet[value_data];
	count = 0;
	char ch;
	unsigned long time_n, time_n_tick;
	boolean begin_of_packet;
	flag_data = false;
	begin_of_packet = false;
	//devel_on
	flag_net = false;
	//devel_off
	digitalWrite(pin_tr, LOW);
	time_n = millis();
	time_n_tick = millis();
	while (true) {
		if(Serial.available()) {
			//devel_on
			//flag_net = true;
			//devel_off
			time_n_tick = millis();																	//услышали что-то в линии - сбросили таймер тика
			ch = Serial.read();                                     //читаем что прилетело, заодно чистим буфер если сыпется мусор на линии
			 //pc.print(ch);
			if (ch == '>' && !begin_of_packet) {
				begin_of_packet = true;

			}
			if (begin_of_packet) {                          //если был начало пакета '>'
				net_packet[count] = ch;                         // пишем в пакет
				if (net_packet[count] == '<') {
					byte crc_incoming;                                                                                              //начало функции высчитывания crc
					byte crc_calc;
					byte buf[count-2]; //-2 crc идет вторым байтом, поэтому считаем с третьего
					for (byte i=2; i<=count; i++) {
						buf[i-2] = byte(net_packet[i]);
					}
					crc_incoming = byte(net_packet[1]);
					crc_calc = CRC8.smbus(buf, sizeof(buf));        //конец функции подсчета crc
					if (crc_incoming == crc_calc) {
						if ((byte(net_packet[2])) == self_id) { //если id верный, то
							com = byte (net_packet[4]);        //получаем команду
							if (net_packet[5] != '<') {             //проверяем наличие данных и если есть - пишем
								// pc.print("data_nn=");pc.print(byte(net_packet[5]));
								flag_data = true;
								flag_net = true;
								nn = net_packet[5];
								// pc.println();pc.print("debug=");
								 for (byte i=0; i<nn; i++) {
									data[i] = net_packet[6+i];
								// 	pc.print(char(data[i]));
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
		if (!begin_of_packet && ((millis()-time_n_tick) > timeout_tick)) {
			// pc.println();pc.println("net timeout");
			break;																			//хер знает
		}
	}
}


// void recive_com(){                      //прием пакета
// 	char net_packet[value_data];
// 	int count = 0;
// 	char ch;
// 	unsigned long time_n;
// 	boolean begin_of_packet;
// 	begin_of_packet = false;
// 	//devel_on
// 	flag_net = false;
// 	//devel_off
// 	digitalWrite(pin_tr, LOW);
// 	while (true) {
// 		if(Serial.available()) {
// 			//devel_on
// 			flag_net = true;;
// 			//devel_off
// 			ch = Serial.read();                                     //читаем что прилетело, заодно чистим буфер если сыпется мусор на линии
// 			pc.print(ch);
// 			if (ch == '>' && !begin_of_packet) {
// 				begin_of_packet = true;
// 				time_n = millis()-1;
// 			}
// 			if (begin_of_packet) {                          //если был начало пакета '>'
// 				net_packet[count] = ch;                         // пишем в пакет
// 				if (net_packet[count] == '<') {
// 					// pc.println(); pc.print("crc_c="); pc.println(crc_c());
// 					byte crc_incoming;                                                                                              //начало функции высчитывания crc
// 					byte crc_calc;
// 					byte buf[count-2]; //-2 crc идет вторым байтом, поэтому считаем с третьего
// 					for (byte i=2; i<=count; i++) {
// 						buf[i-2] = byte(net_packet[i]);
// 					}
// 					crc_incoming = byte(net_packet[1]);
// 					crc_calc = CRC8.smbus(buf, sizeof(buf));        //конец функции подсчета crc
// 					if (crc_incoming == crc_calc) {
// 						if ((byte(net_packet[2])) == self_id) { //если id верный, то
// 							com = byte (net_packet[4]);                                             //получаем команду
// 							if (net_packet[5] != '<') {          //проверяем наличие data                                   //проверяем наличие данных и если есть - пишем
// 								flag_data = true;
// 								nn = net_packet[5];								 //получаем размер даты
// 								for (byte i=0; i<=nn; i++) {					// и пишем ее в глобальный массив data[]
// 									data[i] = net_packet[5+i];
// 								}
// 							} else flag_data = false;							//если даты нет, то успокаиваемся
// 							break;
// 						} else {
// 							begin_of_packet = false;							//id чужой, нам не нужен этот пакет
// 							count = 0;														//и сбрасываем счетчик байтов
// 						}
// 					}
// 				}
// 				if ((millis()-time_n) > timeout_packet) {		//проверка на таймаут, с момента '>'
// 					begin_of_packet = false;
// 					count = 0;
// 				} else count++;
// 			}
// 		}
// 	}
// }

void response(byte id, byte com, char type_packet, boolean data_b){   //функция передачи в линию
	unsigned int mm;
	if (data_b) mm=(nn+4); else mm=3;
	//pc.print("mm=");pc.println(mm);
	byte packet [mm];
	digitalWrite(led_pin, HIGH);																		//поднимаем пин инидикации
	digitalWrite(pin_tr, HIGH);																			//поднимаем пин передачи max485
	packet[0] = id;																									//в пакет целевой id
	packet[1] = byte(type_packet);																	//в пакет тип пакета
	packet[2] = com;																								//ответ для мастера
	if (data_b) {                               //если данные нужно передавать, дополнительно грузим в массив блок данных, количеством nn
		packet[3] = nn;                           //если данные, то помещаем 4 байтом размер данных
		for (byte i=0; i<nn; i++) {
			packet[4+i] = data[i];                                                  //если данные - льём их в пакет
		}
		packet[4+nn] = byte('<');                                                	//и заканчиваем блок данных концом пакета '<'                                                                                                                                                                                   //и в конец помещаем '<'
	} else packet[3] = byte('<');        //иначе просто конец пакета

	Serial.print('>');
	//pc.print("crc"); pc.println(CRC8.smbus(packet, sizeof(packet)));
	Serial.print(char(CRC8.smbus(packet, sizeof(packet))));										//считаем crc и передаем в линию
	for (byte i=0; i<=sizeof(packet); i++) {																	//льем в линию остальной пакет
		//pc.print(char(packet[i]));
		Serial.print (char(packet[i]));
	}

	digitalWrite(led_pin, LOW);																			//гасим пин инидикации
	delay(tx_ready_delay);																					//ждем когда max485 отдаст все в линию
	digitalWrite(pin_tr, LOW);																			//опускаем пин передачи max485
}
