
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
byte net_packet[value_data];
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
	flag_data = false;
	// //debug on
	// nn = 25;
	// for (byte i=0; i<nn; i++){
	// 	data[i]=i+65;
	// }
	// //debug off
}
void loop(){
	id_m = 10;
	com_m = 18;                    // "отключить реле"
	trans_com(id_m,com_m,ask,false);
  delay (2000);
  //_________________
  // id_m = 10;
  // com_m = 19;                    // "включить реле"
	// trans_com(id_m,com_m,ask,false);
	// delay (2000);
  //_________________
  id_m = 10;
  com_m = 10;                    //"включить""
  data[0] = 0;                   //из принципа обмена: 0 = "все"
  nn=1;                        //количество данных 1 байт
  trans_com(id_m,com_m,ask,true);
  delay (2000);
  //_________________
  id_m = 10;
  com_m = 11;                   // "выключить"...
  data[0] = 1;						    	//...первую лампу...
	data[1] = 3;									//...третью лампу...
  data[2] = 5;                  //...пятую лампу
  nn=3;                       // количество данных 2 байта
  trans_com(id_m,com_m,ask,true);
  delay(2000);
  //_________________
  id_m = 10;
  com_m = 11;                    // "выключить"...
  data[0] = 0;                   // "все"
  nn=1;
  trans_com(id_m,com_m,ask,true);
  delay(2000);
}

// void read_data(){
// 	pc.println();pc.print("readdata=");
// 	for (byte i=0; i<nn; i++){
// 		pc.print(char(data[i]));
// 	}
// 	pc.println();
// }

// void write_data(){
// 	nn = 5;
// 	for (byte i=0; i<nn; i++){
// 		data[i]=i+97;
// 	}
// }

void trans_com(byte id, byte com, char type_packet, boolean data_b){   //функция передачи в линию
	unsigned int mm;
	if (data_b) mm=(nn+4); else mm=3;
  //byte net_packet[mm];
  pc.println();
  pc.print("mm=");pc.println(mm);
  pc.print("nn=");pc.println(nn);
	pc.println();pc.println(F("_________________:")); pc.print(F(" id=")); pc.print(id); pc.print(F(" com=")); pc.println(com);
	digitalWrite(led_pin, HIGH);
	digitalWrite(pin_tr, HIGH);
	net_packet[0] = id;
	net_packet[1] = byte(type_packet);
	net_packet[2] = com;
  pc.println();
	if (data_b) {                               //если данные нужно передавать, дополнительно грузим в массив блок данных, количеством nn
                                 //если данные, то помещаем 4 байтом размер данных
		net_packet[3] = nn;
		for (byte i=0; i<nn; i++) {
			net_packet[4+i] = data[i];                 //данные - льём их в пакет
		}
		net_packet[(4+nn)] = byte('<');                                                                                                                                                                                                                  //и в конец помещаем '<'
	} else {net_packet[3] = byte('<');}        //иначе просто конец пакета

	Serial.print('>');
	//pc.print(" crc"); pc.println(CRC8.smbus(net_packet, mm));
  //pc.print("sizeof="); pc.println(sizeof(net_packet));
	Serial.print(char(CRC8.smbus(net_packet, mm)));
  pc.print("crc_outgoing="); pc.println(CRC8.smbus(net_packet, mm));
	for (byte i=0; i<=mm; i++) {
		Serial.print (char(net_packet[i]));
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
  //delete[] net_packet;
}


void recive_com(byte id){                      //прием пакета
	int count = 0;
	//char net_packet[value_data];
	count = 0;
	char ch;
	unsigned long time_n;
	boolean begin_of_packet;
	flag_data = false;
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
					byte crc_incoming;                                                                                              //начало функции высчитывания crc
					byte crc_calc;
					//byte buf[count-2]; //-2 crc идет вторым байтом, поэтому считаем с третьего
					//for (byte i=2; i<=count; i++) {
					//	buf[i-2] = byte(net_packet[i]);
					//}
					crc_incoming = byte(net_packet[1]);
					crc_calc = CRC8.smbus(&net_packet[2], count-2);        //конец функции подсчета crc
          pc.println();
          pc.print("CRC_incoming=");pc.println(crc_incoming);
          pc.print("CRC_calc=");pc.println(crc_calc);
          pc.print("count=");pc.println(count);
					if (crc_incoming == crc_calc) {
						if ((byte(net_packet[2])) == id) { //если id верный, то
							responce = byte (net_packet[4]);        //получаем команду
							if (net_packet[5] != '<') {             //проверяем наличие данных и если есть - пишем
								pc.print("data_nn=");pc.print(byte(net_packet[5]));
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
					} else {
            pc.println();
            pc.println("CRC_incoming error");
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
