//код мастера

#include <SoftwareSerial.h>
//для передачи в линию используется хардварный serial
#define pin_tr 4                                    //пин transmission enable для max485
#define led_pin 13                                //светодиод активности *debag
#define tx_ready_delay 1                   //задержка для max485 (буфер? вообщем, неопределенность буфераmax485)
#define tx_pc 5                                   //serial pc alfa
#define rx_pc 6                                   //serial pc alfa
#define value_data  10                        //размер пакета
const char ask='!';
SoftwareSerial pc(rx_pc, tx_pc);        //инициализация Serial для получения команд с компа, для альфы

//debug
//debug
char* com_m[2][9] =
{
  {"01", "02", "03", "04", "05", "06", "07", "08", "09"},
  {"10", "11", "12", "13", "14", "15", "16", "17", "18"}};
char* id_m[2][9] =
{
  {"01", "02", "03", "04", "05", "06", "07", "08", "09"},
  {"10", "11", "12", "13", "14", "15", "16", "17", "18"}};


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

void trans_com(char id[], char com[], char id_command){			//функция передачи в линию
	digitalWrite(led_pin, HIGH);
	digitalWrite(pin_tr, HIGH);
	Serial.print('>');				//передача начала пакета
	Serial.print(id[0]);			//передача id слэйва [1 символ]
	Serial.print(id[1]);			//передача id слэйва [2 символ]
	Serial.print(id_command);		//передача индификатора комманды (ask/heatbit)
	byte count = 0;
	while (com[count]!='\0'){
		Serial.print(com[count]);
		count++;
	}
	Serial.print('<');				//передача конца пакета
	digitalWrite(led_pin, LOW);
	delay(tx_ready_delay);
	digitalWrite(pin_tr, LOW);
	
}