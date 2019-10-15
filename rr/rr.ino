//код слэйва

#include <FastCRC_tables.h>
#include <FastCRC.h>
#include <FastCRC_cpu.h>
#include <SoftwareSerial.h>

#define pin_tr 4                          //пин transmission enable для max485
//#define led_pin 13                        //светодиод активности *debag
#define tx_ready_delay 1                  //задержка для max485 (буфер? вообщем, неопределенность буфераmax485)
#define value_data  261                   //размер пакета packet+data
FastCRC8 CRC8;

volatile byte triac_level_bright[7];      //заданный уровень яркости 128..0 для 8 симмисторов
const byte step_bri = 128;                //количество уровней яркости 0 = ON, 128 = OFF
byte triacs = 8;                          //количество симмисторов/каналов

byte for_master = 1;                      //id мастера
byte self_id=10;
const char ask='!';
byte ok=94;                     //рабочий вариант ответа 94 '^'
#define pin_relay 7             //пин для реле
boolean flag_net;               //флаг получения пакета
boolean flag_data;    //флаг наличия в пакете данных
byte data[254];       //массив для данных
byte net_packet[value_data];
byte nn;              //размер блока ланных
//devel on
#define tx_pc 5                                   //serial pc alfa
#define rx_pc 6                                   //serial pc alfa
SoftwareSerial pc(rx_pc, tx_pc);
//debug off

unsigned long timeout_packet;           //таймаут приема пакета, при наличии, мс
unsigned long timeout_tick;             //таймаут прослушки линии без пакета, чтобы не висеть постояно в цикле прослушки, мс
byte com;                               //команда полученная с линии

void setup() {
	Serial.begin(115200);
  //pinMode(led_pin, OUTPUT);
	pinMode(pin_tr, OUTPUT);
	pinMode(pin_relay, OUTPUT);
	//digitalWrite(led_pin, LOW);
	digitalWrite(pin_tr, LOW);
	digitalWrite(pin_relay, LOW);
	timeout_packet = 250;
	timeout_tick = 100;
	//debug on
	pc.begin(115200);
	//debug off
}

void loop(){
	recive_com();
  job();
	// //devel_on
	// if (flag_net) {
	//      pc.println("_________________");
	//      pc.print("com: "); pc.println(com);
	// switch (com) {                                                          //тестовая заготовка обработки пакетов
	// case 3:
	//      digitalWrite(pin_relay, HIGH);
	//      digitalWrite(led_pin, HIGH);
	//      if (flag_data) read_data();										//полчили данные и пишем их куда-то
	//      write_data();																	//где-то считали данные и пишем в дату
	//      response(self_id, ok, '&', true);
	//      break;
	// case 12:
	//      digitalWrite(pin_relay, LOW);
	//      digitalWrite(led_pin, LOW);
	//      response(self_id, ok, '&', false);
	//      break;
	// }
	// //devel_off
	// }
}

void job(){                                     //обработчик команд
	if (flag_net) {
    pc.println();
    pc.println("_________________");
    pc.print("com: "); pc.println(com);
		switch (com) {
		case 10:                              //"включить яркостью"
			digitalWrite(pin_relay, LOW); //включаем реле
			if (data[0] == 0) {                     //"все"
				for (byte i = 0; i<triacs; i++) {
					triac_level_bright[i] = data[1];    //яркостью data[1]
				}
			}
			else {                                  //не "все", а именно...
				for (byte i = 2; i<nn; i++) {        //i=2 т.к. первый байт data[0] признак количества, второй data[1] - яркость
					triac_level_bright[data[i]] = data[1]; //дергаем номер симистора из блока данных, яркость data[1]
				}
			}
      read_data();
      response(self_id, ok, '&', false);
			break;
		case 11:                                  //"выключить"
			if (data[0] == 0) {                     //"все"
				for (byte i = 0; i<triacs; i++) {
					triac_level_bright[i] = step_bri;        //яркость 128 = выкл.
				}
			}
			else {                                  //не "все", а именно...
				for (byte i = 1; i<nn; i++) {        //количество ламп определяется количеством данных в data[] минус признак количества data[0]
					triac_level_bright[data[i]] = step_bri; //дергаем из data[] номера симисторов и гасим их яркостью 128
				}
			}
      read_data();
      response(self_id, ok, '&', false);
			break;
		case 12:                                  //"убавить"
			if (data[0] == 0) {                     //"все"
				for (byte i = 0; i<triacs; i++) {
					int temp_bri = triac_level_bright[i] + data[1]; //проверям переполнение вычисленной яркости
					if (temp_bri > step_bri) temp_bri=step_bri;
					triac_level_bright[i] = temp_bri;       //и устанавливаем вычисленную яркость.
				}
			}
			else {                      //не "все", а именно...
				for (byte i = 2; i<nn; i++) { //i=2 т.к. первый байт data[0] признак количества, второй data[1] - декремент яркости
					int temp_bri = triac_level_bright[data[i]] + data[1]; //проверям переполнение вычисленной яркости1
					if (temp_bri > step_bri) temp_bri=step_bri;
					triac_level_bright[data[i]] = temp_bri;       //и устанавливаем вычисленную яркость симмистора с номером из блока данных
				}
			}
			break;
		case 13:                //"прибавить"
			if (data[0] == 0) { //"все"
				for (byte i = 0; i<triacs; i++) {
					int temp_bri = triac_level_bright[i] - data[1]; //проверям переполнение вычисленной яркости. data[1] - инкремент яркости.
					if (temp_bri < 0 ) temp_bri=0; //'0' - максимальная яркость
					triac_level_bright[i] = temp_bri; //и устанавливаем вычисленную яркость.
				}
			}
			else {               //не все, а именно...
				for (byte i=2; i <nn; i++) {
					int temp_bri = triac_level_bright[data[i]] - data[1];
					if (temp_bri < 0) temp_bri=0;
					triac_level_bright[data[i]] = temp_bri; //и устанавливаем вычисленную яркость симмистора с номером из блока данных
				}
			}
			break;
		case 14:                //"калибровка"
			//пока пусто//
			break;
		case 15:                //"выполнить эффект"
			//пока пусто//
			break;
		case 16:                //"установить эффект включения"
			//пока пусто//
			break;
		case 17:                //"установить эффект выключения"
			//пока пусто//
			break;
		case 18:                //"отключить реле"
			digitalWrite(pin_relay, HIGH);
      response(self_id, ok, '&', false);
			break;
		case 19:                //"включить реле"
			digitalWrite(pin_relay, LOW);
      response(self_id, ok, '&', false);
			break;
		}
		com = 0;                //обнуляем значение команды после отработки
	}
}

void read_data(){
	pc.println(); pc.print("readdata=");
	for (byte i=0; i<nn; i++) {
		pc.print(char(data[i]+48));
	}
	pc.println();
}

void write_data(){
	nn = 10;
	for (byte i=0; i<nn; i++) {
		data[i]=i+65;
	}
}

void recive_com(){                      //прием пакета
	int count = 0;
	//char net_packet[value_data];
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
			time_n_tick = millis();                                                                                                                                 //услышали что-то в линии - сбросили таймер тика
			ch = Serial.read();                                     //читаем что прилетело, заодно чистим буфер если сыпется мусор на линии
			if (ch == '>' && !begin_of_packet) {
				begin_of_packet = true;

			}
			if (begin_of_packet) {                          //если был начало пакета '>'
				net_packet[count] = ch;                         // пишем в пакет
				if (net_packet[count] == '<') {
					byte crc_incoming;                                                                                              //начало функции высчитывания crc
					byte crc_calc;
					// byte buf[count-2]; //-2 crc идет вторым байтом, поэтому считаем с третьего
					// for (byte i=2; i<=count; i++) {
					// 	buf[i-2] = byte(net_packet[i]);
					// }
					crc_incoming = byte(net_packet[1]);
					crc_calc = CRC8.smbus(&net_packet[2], count-2);        //конец функции подсчета crc
          pc.println();
          pc.print("CRC_incoming=");pc.println(crc_incoming);
          pc.print("CRC_calc=");pc.println(crc_calc);
          pc.print("count=");pc.println(count);
					if (crc_incoming == crc_calc) {
						if ((byte(net_packet[2])) == self_id) { //если id верный, то
							com = byte (net_packet[4]);        //получаем команду
							if (net_packet[5] != '<') {             //проверяем наличие данных и если есть - пишем
								 pc.print("data_nn=");pc.print(byte(net_packet[5]));
								flag_data = true;
								flag_net = true;
								nn = net_packet[5];
								// pc.println();pc.print("debug=");
								for (byte i=0; i<nn; i++) {
									data[i] = net_packet[6+i];
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
					pc.println(); pc.println("begin timeout");
					break;
					// begin_of_packet = false;
					// count = 0;
				} else count++;
			}
		}
		if (!begin_of_packet && ((millis()-time_n_tick) > timeout_tick)) {
			// pc.println();pc.println("net timeout");
			break;                                                                                                                                                  //хер знает
		}
	}
}

void response(byte id, byte com, char type_packet, boolean data_b){   //функция передачи в линию
	unsigned int mm;
	if (data_b) mm=(nn+4); else mm=3;
	//pc.print("mm=");pc.println(mm);
	//byte packet [mm];
	// digitalWrite(led_pin, HIGH);                                                                                                                                            //поднимаем пин инидикации
	digitalWrite(pin_tr, HIGH);                                                                                                                                                     //поднимаем пин передачи max485
	net_packet[0] = id;                                                                                                                                                                                                 //в пакет целевой id
	net_packet[1] = byte(type_packet);                                                                                                                                  //в пакет тип пакета
	net_packet[2] = com;                                                                                                                                                                                                //ответ для мастера
	if (data_b) {                            //если данные нужно передавать, дополнительно грузим в массив блок данных, количеством nn
		net_packet[3] = nn;                    //если данные, то помещаем 4 байтом размер данных
		for (byte i=0; i<nn; i++) {
			net_packet[4+i] = data[i];           //если данные - льём их в пакет
		}
		net_packet[4+nn] = byte('<');          //и заканчиваем блок данных концом пакета '<'                                                                                                                                                                                   //и в конец помещаем '<'
	} else net_packet[3] = byte('<');        //иначе просто конец пакета

	Serial.print('>');
	//pc.print("crc"); pc.println(CRC8.smbus(net_packet, sizeof(net_packet)));
	Serial.print(char(CRC8.smbus(net_packet, mm)));      //считаем crc и передаем в линию
  pc.print("crc_outgoing="); pc.println(CRC8.smbus(net_packet, mm));
	for (byte i=0; i<=mm; i++) {                                                                                                                                        //льем в линию остальной пакет
		//pc.print(char(net_packet[i]));
		Serial.print (char(net_packet[i]));
	}
  pc.println();

	//digitalWrite(led_pin, LOW);                                                                                                                                                     //гасим пин инидикации
	delay(tx_ready_delay);                                                                                                                                                                  //ждем когда max485 отдаст все в линию
	digitalWrite(pin_tr, LOW);
  //delete[] packet;                                                                                                                                                    //опускаем пин передачи max485
}
