//код слэйва

#include <FastCRC_tables.h>
#include <FastCRC.h>
#include <FastCRC_cpu.h>
#include <SoftwareSerial.h>

#define pin_tr 4            //пин transmission enable для max485
#define led_pin 13        //светодиод активности *debag
#define tx_ready_delay 1   //задержка для max485 (буфер? вообщем, неопределенность буфераmax485)
#define value_data  10                        //размер пакета
FastCRC8 CRC8;
byte self_id=10;
const char ask='!';
//devel on
#define exec_pin1 9		//пин для реле
boolean flag_net;			//флаг получения пакета
//devel off
//char com1[] = "100";		//команда номер 1, "зажечь светодиод"
//devel on
#define tx_pc 5                                   //serial pc alfa
#define rx_pc 6                                   //serial pc alfa

SoftwareSerial pc(rx_pc, tx_pc);
//debug off
char net_packet[value_data];
unsigned long timeout_packet;		//таймаут приема пакета, мс
byte count;
//char* com_m[5] ={"01", "02", "03", "04","05"};	//команды
byte com;		//команда полученная с линии

void setup() {
  Serial.begin(115200);
  pinMode(led_pin, OUTPUT);
  pinMode(pin_tr, OUTPUT);
  pinMode(exec_pin1, OUTPUT);
  digitalWrite(led_pin, LOW);
  digitalWrite(pin_tr, LOW);
  digitalWrite(exec_pin1, LOW);
  count = 0 ;
  timeout_packet = 250;
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
	      digitalWrite(exec_pin1, HIGH);
	      digitalWrite(led_pin, HIGH);
	      break;
	    case 12:
	      digitalWrite(exec_pin1, LOW);
	      digitalWrite(led_pin, LOW);
	      break;
	}
	//devel_off
}

void recive_com(){			//прием пакета
	count = 0;
	char ch;
	unsigned long time_n;
	boolean begin_of_packet;
	begin_of_packet = false;
	//devel_on
	flag_net = false;
	//devel_off
	digitalWrite(pin_tr, LOW);
	while (true){
		if(Serial.available()){
			//devel_on
			flag_net = true;;
			//devel_off
			ch = Serial.read();					//читаем что прилетело, заодно чистим буфер если сыпется мусор на линии
      pc.print(ch);
			if (ch == '>' && !begin_of_packet) {
				begin_of_packet = true;
				time_n = millis()-1;
			}
			if (begin_of_packet) {				//если был начало пакета '>'
				net_packet[count] = ch;				// пишем в пакет
				if (net_packet[count] == '<'){
          pc.println();pc.print("crc_c=");pc.println(crc_c());
          if (crc_c()){
  					if (verf_id()){        //вызываем проверку id
              prep_com();
  					       break;
            } else {
              begin_of_packet = false;
              count = 0;
              }
            }
          }

				if ((millis()-time_n) > timeout_packet) {
					begin_of_packet = false;
					count = 0;
				} else	count++;
			}
		}
	}
}

boolean crc_c(){            //проверка crc
  byte crc_incoming;
  byte crc_calc;
  byte buf[count-2];  //-2 crc идет вторым байтом, поэтому считаем с третьего
  // pc.println();
  // pc.print("buf[i]");
  for (byte i=2; i<=count; i++){
    buf[i-2] = byte(net_packet[i]);
    // pc.print(buf[i-2]);
  }
  // pc.println();
  crc_incoming = byte(net_packet[1]);
  crc_calc = CRC8.smbus(buf, sizeof(buf));
  // pc.print("crc_incoming=");pc.println(crc_incoming);
  // pc.print("crc_calc=");pc.println(crc_calc);

  if (crc_incoming == crc_calc) return true; else return false;

}
boolean verf_id(){        //проверка id
  if ((byte(net_packet[2])) == self_id) return true; else return false;
}
void prep_com(){
  com = byte (net_packet[4]);
}
// void response(char com[], char type_packet){		//отвечаем
// 	digitalWrite(led_pin, HIGH);
// 	digitalWrite(pin_tr, HIGH);
// 	Serial.print('>');
// 	Serial.print(self_id[0]);
// 	Serial.print(self_id[1]);
// 	Serial.print(type_packet);
// 	Serial.print('9');								//отсылаем код 94 -- "ok!"
// 	Serial.print('4');
// 	Serial.print('<');
// 	delay(tx_ready_delay);
// 	digitalWrite(led_pin, LOW);
// 	digitalWrite(pin_tr, LOW);
// }
