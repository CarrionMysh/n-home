//код слэйва

#include <FastCRC_tables.h>
#include <FastCRC.h>
#include <FastCRC_cpu.h>
#include <SoftwareSerial.h>
#include <SPI.h>
#include <TimerOne.h>
#define pin_tr 4            //пин transmission enable для max485
#define pin_relay 7
#define tx_ready_delay 1   //заержка для max485 (буфер? вообщем, неопределенность буфераmax485)
#define value_data  261                        //размер пакета packet+data
volatile byte debug = 0;                  //временная переменная
volatile boolean zero_cross = 0;          //флаг перехода нуля
volatile byte count_step [7];             //счетчик числа проверок после прохождения нуля для каждого симмистора
volatile byte triac_level_bright[7];               //заданный уровень яркости 128..0 для 8 симмисторов
volatile byte bitsToSend = 0;                      //байт (карта) состояний симмисторов
const byte step_bri = 128;                               //количество уровней яркости 0 = ON, 128 = OFF
volatile byte freqStep = 75;                       //This is the delay-per-brightness step in microseconds.
// This is the delay-per-brightness step in microseconds.
// For 60 Hz it should be 65
// It is calculated based on the frequency of your voltage supply (50Hz or 60Hz)
// and the number of brightness steps you want.
// Realize that there are 2 zerocrossing per cycle. This means
// zero crossing happens at 120Hz for a 60Hz supply or 100Hz for a 50Hz supply.
// To calculate freqStep divide the length of one full half-wave of the power
// cycle (in microseconds) by the number of brightness steps.
// (120 Hz=8333uS) / 128 brightness steps = 65 uS / brightness step
// (100Hz=10000uS) / 128 steps = 75uS/step
FastCRC8 CRC8;
byte for_master = 1;                            //id мастера
byte self_id=10;
const char ask='!';
byte ok=94;                             //рабочий вариант ответа 94 '^'
//devel on
boolean flag_net;                       //флаг получения пакета
boolean flag_data;    //флаг наличия в пакете данных
byte data[254];       //массив для данных
byte nn;              //размер блока ланных
byte triacs = 8;                        //количество симмисторов/каналов
//devel off
//devel on
#define tx_pc 5                                   //serial pc alfa
#define rx_pc 6                                   //serial pc alfa
SoftwareSerial pc(rx_pc, tx_pc);
//debug off
unsigned long timeout_packet;           //таймаут приема пакета, при наличии, мс
unsigned long timeout_tick;             //таймаут прослушки линии без пакета, чтобы не висеть постояно в цикле прослушки, мс
byte com=0;               //команда полученная с линии
void f
void setup() {
	SPI.begin();
	pinMode(PIN_SPI_SS, OUTPUT);
	Serial.begin(115200);
	pinMode(pin_relay, OUTPUT);
	pinMode(pin_tr, OUTPUT);
	Timer1.initialize(freqStep);
	Timer1.attachInterrupt(dim_check, freqStep);
	attachInterrupt(0, zero_cross_c, RISING);
	//digitalWrite(led_pin, LOW);
	digitalWrite(pin_tr, LOW);
	// digitalWrite(exec_pin1, LOW);
	digitalWrite(pin_relay, LOW);
	timeout_packet = 250;
	timeout_tick = 100;
	//debug on
	pc.begin(115200);
	triac_level_bright[0] = step_bri;
	triac_level_bright[1] = step_bri;
	triac_level_bright[2] = step_bri;
	triac_level_bright[3] = step_bri;
	triac_level_bright[4] = step_bri;
	triac_level_bright[5] = step_bri;
	triac_level_bright[6] = step_bri;
	triac_level_bright[7] = step_bri;
	//debug off
}

void loop(){
	recive_com();
	job();
}

void job(){                                     //обработчик команд
	switch (com) {
	case 10:                                      //"включить яркостью"
    digitalWrite(pin_relay, LOW);               //включаем реле
		if (data[0] == 0) {                             //"все"
			for (byte i = 0; i<triacs; i++) {
				triac_level_bright[i] = data[1];            //яркостью data[1]
			}
		}
		else {                                          //не "все", а именно...
			for (byte i = 2; i<nn; i++) {                //i=2 т.к. первый байт data[0] признак количества, второй data[1] - яркость
				triac_level_bright[data[i]] = data[1];      //дергаем номер симистора из блока данных, яркость data[1]
			}
		}
		break;
	case 11:                                          //"выключить"
		if (data[0] == 0) {                             //"все"
			for (byte i = 0; i<triacs; i++) {
				triac_level_bright[i] = step_bri;                //яркость 128 = выкл.
			}
		}
		else {                                          //не "все", а именно...
			for (byte i = 1; i<nn; i++) {                //количество ламп определяется количеством данных в data[] минус признак количества data[0]
				triac_level_bright[data[i]] = step_bri;     //дергаем из data[] номера симисторов и гасим их яркостью 128
			}
		}
		break;
	case 12:                                          //"убавить"
		if (data[0] == 0) {                             //"все"
			for (byte i = 0; i<triacs; i++) {
				int temp_bri = triac_level_bright[i] + data[1]; //проверям переполнение вычисленной яркости
				if (temp_bri > step_bri) temp_bri=step_bri;
				triac_level_bright[i] = temp_bri;               //и устанавливаем вычисленную яркость.
			}
		}
		else {                              //не "все", а именно...
			for (byte i = 2; i<nn; i++) {     //i=2 т.к. первый байт data[0] признак количества, второй data[1] - декремент яркости
				int temp_bri = triac_level_bright[data[i]] + data[1]; //проверям переполнение вычисленной яркости1
				if (temp_bri > step_bri) temp_bri=step_bri;
				triac_level_bright[data[i]] = temp_bri;               //и устанавливаем вычисленную яркость симмистора с номером из блока данных
			}
		}
		break;
	case 13:                        //"прибавить"
		if (data[0] == 0) {     //"все"
			for (byte i = 0; i<triacs; i++) {
				int temp_bri = triac_level_bright[i] - data[1]; //проверям переполнение вычисленной яркости. data[1] - инкремент яркости.
				if (temp_bri < 0 ) temp_bri=0; //'0' - максимальная яркость
				triac_level_bright[i] = temp_bri; //и устанавливаем вычисленную яркость.
			}
		}
		else {                       //не все, а именно...
			for (byte i=2; i <nn; i++) {
				int temp_bri = triac_level_bright[data[i]] - data[1];
				if (temp_bri < 0) temp_bri=0;
				triac_level_bright[data[i]] = temp_bri; //и устанавливаем вычисленную яркость симмистора с номером из блока данных
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
		break;
	case 19:                        //"включить реле"
		digitalWrite(pin_relay, LOW);
		break;
	}
	com = 0;                        //обнуляем значение команды после отработки
}

void hc74_c(byte nn_triac_sub, int triac_state){
	bitWrite(bitsToSend, nn_triac_sub, triac_state); //меняем в 00000000 (bitsToSend) запускаем нужный симмистор, например, 01000000
	digitalWrite(PIN_SPI_SS, LOW);         //разлочим 74HC595
	SPI.transfer(bitsToSend);
	digitalWrite(PIN_SPI_SS, HIGH);        //лочим 74HC595
}

void zero_cross_c(){
	for (byte i = 0; i <= 7; i++) {
		count_step [i] = 0;                   //обнуляем счетчик отсчетов (времени) для всех симмисторов
	}
	bitsToSend = 0;                        //обнуляем карту состояния симмисторов
	digitalWrite(PIN_SPI_SS, LOW);               //разлочим 74HC595
	SPI.transfer(bitsToSend);
	digitalWrite(PIN_SPI_SS, HIGH);              //лочим 74HC595
}

void dim_check(){
	for (byte i = 0; i <= 7; i++) {
		if (count_step [i]>=triac_level_bright [i]) {   //сравниваем пройденный счетчик с номером нужной яркости симмистора
			hc74_c(i, HIGH);                          //включаем нужный симмистр
			count_step[i] = 0;                        //обнуляем счетчик для конкретного симмистора
		}
		else {
			count_step[i]++;                          //увеличиваем счетчик отсчетов
		}
	}
}

void read_data(){
	pc.println(); pc.print("readdata=");
	for (byte i=0; i<nn; i++) {
		pc.print(char(data[i]));
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
			time_n_tick = millis();                          //услышали что-то в линии - сбросили таймер тика
			ch = Serial.read();                              //читаем что прилетело, заодно чистим буфер если сыпется мусор на линии
			if (ch == '>' && !begin_of_packet) {
				begin_of_packet = true;
			}
			if (begin_of_packet) {                           //если был начало пакета '>'
				net_packet[count] = ch;                        // пишем в пакет
				if (net_packet[count] == '<') {
					byte crc_incoming;                           //начало функции высчитывания crc
					byte crc_calc;
					byte buf[count-2];                           //-2 crc идет вторым байтом, поэтому считаем с третьего
					for (byte i=2; i<=count; i++) {
						buf[i-2] = byte(net_packet[i]);
					}
					crc_incoming = byte(net_packet[1]);
					crc_calc = CRC8.smbus(buf, sizeof(buf));        //конец функции подсчета crc
					if (crc_incoming == crc_calc) {
						if ((byte(net_packet[2])) == self_id) {       //если id верный, то
							com = byte (net_packet[4]);                 //получаем команду
							if (net_packet[5] != '<') {                 //проверяем наличие данных и если есть - пишем
								flag_data = true;
								flag_net = true;
								nn = net_packet[5];
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
					}
				}
				if ((millis()-time_n) > timeout_packet) {
					pc.println(); pc.println("begin timeout");
					break;
				} else count++;
			}
		}
		if (!begin_of_packet && ((millis()-time_n_tick) > timeout_tick)) {
			break;                           //хер знает
		}
	}
}

void response(byte id, byte com, char type_packet, boolean data_b){   //функция передачи в линию
	unsigned int mm;
	if (data_b) mm=(nn+4); else mm=3;
	byte packet [mm];
	digitalWrite(pin_tr, HIGH);          //поднимаем пин передачи max485
	packet[0] = id;                      //в пакет целевой id
	packet[1] = byte(type_packet);       //в пакет тип пакета
	packet[2] = com;                     //ответ для мастера
	if (data_b) {                        //если данные нужно передавать, дополнительно грузим в массив блок данных, количеством nn
		packet[3] = nn;                    //если данные, то помещаем 4 байтом размер данных
		for (byte i=0; i<nn; i++) {
			packet[4+i] = data[i];           //если данные - льём их в пакет
		}
		packet[4+nn] = byte('<');          //и заканчиваем блок данных концом пакета '<' и в конец помещаем '<'
	} else packet[3] = byte('<');        //иначе просто конец пакета
	Serial.print('>');
	Serial.print(char(CRC8.smbus(packet, sizeof(packet))));//считаем crc и передаем в линию
	for (byte i=0; i<=sizeof(packet); i++) {           //льем в линию остальной пакет
		Serial.print (char(packet[i]));
	}
	delay(tx_ready_delay);              //ждем когда max485 отдаст все в линию
	digitalWrite(pin_tr, LOW);          //опускаем пин передачи max485
}
