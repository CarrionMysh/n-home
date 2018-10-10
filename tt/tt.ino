//код мастера

#include <SoftwareSerial.h>
//для передачи в линию используется хардварный serial
#define pin_tr 4                                    //пин transmission enable для max485
#define led_pin 13                                //светодиод активности *debag
#define tx_ready_delay 1                   //задержка для max485 (буфер? вообщем, неопределенность буфераmax485)
#define tx_pc 5                                   //serial pc alfa
#define rx_pc 6                                   //serial pc alfa
#define value_data  10                        //размер пакета
const char ask='!';						//тип пакета
SoftwareSerial pc(rx_pc, tx_pc);        //инициализация Serial для получения команд с компа, для альфы
//devel on
boolean flag_net;
//devel off
char net_packet[value_data];
byte count;
char* com_m[2][9] =
{
  {"01", "02", "03", "04", "05", "06", "07", "08", "09"},
  {"10", "11", "12", "13", "14", "15", "16", "17", "18"}};
char* id_m[2][9] =
{
  {"01", "02", "03", "04", "05", "06", "07", "08", "09"},
  {"10", "11", "12", "13", "14", "15", "16", "17", "18"}};
byte responce;		//ответ от слейва полученный с линии

void setup() {
	Serial.begin(115200);
	pc.begin(115200);
	pinMode(led_pin, OUTPUT);         //пин светодиода
	pinMode(pin_tr, OUTPUT);          //пин передачи
	digitalWrite(led_pin, LOW);
	digitalWrite(pin_tr, LOW);
	pc.println(F("Serial ok"));
}

void loop(){

	trans_com(id_m[0][1],com_m[0][2],ask);
	delay (2000);
	trans_com(id_m[0][1],com_m[1][2],ask);
	delay (2000);
}

void trans_com(char id[], char com[], char type_packet){			//функция передачи в линию
	digitalWrite(led_pin, HIGH);
	digitalWrite(pin_tr, HIGH);
	Serial.print('>');				//передача начала пакета
	Serial.print(id[0]);			//передача id слэйва [1 символ]
	Serial.print(id[1]);			//передача id слэйва [2 символ]
	Serial.print(type_packet);		//передача типа пакета (ask/heatbit, например)
	byte count = 0;
	while (com[count]!='\0'){		//передача команды
		Serial.print(com[count]);
		count++;
	}
	Serial.print('<');				//передача конца пакета
	digitalWrite(led_pin, LOW);
	delay(tx_ready_delay);
	digitalWrite(pin_tr, LOW);	
}

void recive_com(){			//прием пакета
	byte count = 0;
	char ch;
	boolean begin_of_packet;
	begin_of_packet = false;
	//devel_on
	flag_net = false;
	//devel_off
	digitalWrite(pin_tr, LOW);
	while (true){
		if(Serial.available()){
			//digitalWrite(led_pin, HIGH);
			//devel_on
			flag_net = true;
			//devel_off
			ch = Serial.read();					//читаем что прилетело, заодно чистим буфер если сыпется мусор на линии
			if (ch == '>' && !begin_of_packet) {
				begin_of_packet = true;
			}
			if (begin_of_packet) {				//если был начало пакета '>'
				net_packet[count] = ch;				// пишем в пакет
				if (net_packet[count] == '<'){
					//digitalWrite(led_pin, LOW);
					net_packet[count+1]='\0';
					prep_responce_com();
					break;
				}
				count++;
			}
		}
	}
}

void prep_responce_com(){
	byte count = 0;
	while (net_packet[count] != '!'){		//ищем в пакете ask
		count++;
	}
	responce = 10 * (net_packet[count+1] - '0')+(net_packet[count+2] - '0');
}