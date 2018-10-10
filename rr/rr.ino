//код слэйва


#include <SoftwareSerial.h>

#define pin_tr 4            //пин transmission enable для max485
#define led_pin 13        //светодиод активности *debag
#define tx_ready_delay 1   //задержка для max485 (буфер? вообщем, неопределенность буфераmax485)
#define value_data  10                        //размер пакета
char self_id[2]="02";
const char ask='!';
//devel on
boolean flag_net;			//флаг получения пакета
//devel off
char com1[] = "100";		//команда номер 1, "зажечь светодиод"
//devel on
#define tx_pc 5                                   //serial pc alfa
#define rx_pc 6                                   //serial pc alfa

SoftwareSerial pc(rx_pc, tx_pc);
//debug off
char net_packet[value_data];
byte count;
char* com_m[5] ={"01", "02", "03", "04","05"};	//команды
byte com;		//команда полученная с линии

void setup() {
  Serial.begin(115200);
  pinMode(led_pin, OUTPUT);
  pinMode(pin_tr, OUTPUT);
  digitalWrite(led_pin, LOW);
  digitalWrite(pin_tr, LOW);
  count = 0 ;
  //debug on 
  pc.begin(115200);
  //debug off
}

void loop(){
	recive_com();
	//devel_on
	if (flag_net){
	pc.print("com: ");pc.println(com);}
	switch (com) {								//тестовая заготовка обработки пакетов
	    case 3:
	      digitalWrite(led_pin, HIGH);
	      delay(250);
	      digitalWrite(led_pin, LOW);
	      delay(250);
	      break;
	    case 12:
	      digitalWrite(led_pin, HIGH);
	      delay(75);
	      digitalWrite(led_pin, LOW);
	      delay(75);
	      break;
	}
	//devel_off
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
					prep_com();
					break;
				}
				count++;
			}
		}
	}
}

void prep_com(){
	byte count = 0;
	while (net_packet[count] != '!'){		//ищем в пакете ask
		count++;
	}
	com = 10 * (net_packet[count+1] - '0')+(net_packet[count+2] - '0');
}

void response(char com[], char type_packet){		//отвечаем
	digitalWrite(led_pin, HIGH);
	digitalWrite(pin_tr, HIGH);
	Serial.print('>');
	Serial.print(self_id[0]);
	Serial.print(self_id[1]);
	Serial.print(type_packet);
	Serial.print('9');								//отсылаем код 94 -- "ok!"
	Serial.print('4');
	Serial.print('<');
	delay(tx_ready_delay);
	digitalWrite(led_pin, LOW);
	digitalWrite(pin_tr, LOW);
}