//код слэйва


#include <SoftwareSerial.h>

#define pin_tr 4            //пин transmission enable для max485
#define led_pin 13        //светодиод активности *debag
#define tx_ready_delay 20   //задержка для max485 (буфер? вообщем, неопределенность буфераmax485)
#define value_data  10                        //размер пакета
#define self_id 02
const char ask='!';
boolean flag_net;			//флаг получения пакета
//debug
char com1[] = "100";		//команда номер 1, "зажечь светодиод"
#define tx_pc 5                                   //serial pc alfa
#define rx_pc 6                                   //serial pc alfa

SoftwareSerial pc(rx_pc, tx_pc);
//debug
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
  //debug 
  pc.begin(115200);
  //debug
}

void loop(){
	recive_com();
	//devel_on
	if (flag_net){
	pc.print("com: ");pc.println(com);}
	//devel_off
	switch (com) {
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
}

void recive_com(){
	byte count = 0;
	flag_net = false;
	digitalWrite(pin_tr, LOW);
	while (true){
		if(Serial.available()){
			//digitalWrite(led_pin, HIGH);
			flag_net = true;
			net_packet[count] = Serial.read();
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

void prep_com(){
	byte count = 0;
	while (net_packet[count] != '!'){		//ищем в пакете ask
		count++;
	}
	com = 10 * (net_packet[count+1] - '0')+(net_packet[count+2] - '0');
}