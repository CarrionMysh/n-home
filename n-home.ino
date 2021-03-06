#include <FastCRC_tables.h>
#include <FastCRC.h>
#include <FastCRC_cpu.h>
#include <SoftwareSerial.h>
//для передачи в линию используется хардварный serial
#define pin_tr 4                                    //пин transmission enable для max485
#define led_pin 13                                //светодиод активности *debag
#define tx_ready_delay 20                   //задержка для max485 (буфер? вообщем, неопределенность буфераmax485)
#define tx_pc 5                                   //serial pc alfa
#define rx_pc 6                                   //serial pc alfa
#define value_data  10                        //размер пакета
//#define ask '!'
//#define response '&'
#define heartbit '*'
#define self_id 01              //свой id
FastCRC8 CRC8;
SoftwareSerial pc(rx_pc, tx_pc);        //инициализация Serial для получения команд с компа, для альфы
const char ask='!';
const char response= '*';
char data[value_data];
char* id_m[] = {"02", "03", "04", "05"}; //массив с id устройств, 01 - master
byte nn = 4;                                          //количество id
char* com_m[2][9] =
{
  {"01", "02", "03", "04", "05", "06", "07", "08", "09"},
  {"01", "02", "03", "04", "05", "06", "07", "08", "09"}
};
char inputSerial [value_data] = "";
byte count;                                           //указатель длины строки с pc.serial
boolean stringComplete = false;
boolean message_pc = true;
//коды ошибок:
//0 - ok
//1 - header
//2 - id
//20 - unknow


void setup() {
  Serial.begin(9600, SERIAL_8E1);
  pc.begin(9600);
  pinMode(led_pin, OUTPUT);         //пин светодиода
  pinMode(pin_tr, OUTPUT);          //пин передачи
  digitalWrite(led_pin, LOW);
  digitalWrite(pin_tr, LOW);
  pc.println(F("Serial ok"));
}

void loop() {
  pc_input();
  if (stringComplete == true) {
    transmite_com();
    pc.println("send ok?");
  }
}

void transmite_com() {
  char data_tx[value_data];
  char subbuf[count];
  byte crc;
  data_tx[value_data] = subbuf[count]; //формируем исходящий пакет
  crc = CRC8.smbus(data_tx, sizeof(data_tx));
  data_tx[value_data] = '>' + crc + data_tx[value_data] + '<'; //добавляем символ начала пакета+crc к пакету

  stringComplete = false;         //обнуляем флаг введеной с консоли команды
  tx_up();
  Serial.print(inputSerial);        //отправляем слэйвам команды введеную с консоли
  tx_down();
}

void tx_up() {
  digitalWrite(pin_tr, HIGH);
  digitalWrite(led_pin, HIGH);
}

void tx_down() {
  delay(tx_ready_delay);
  digitalWrite(pin_tr, LOW);
  digitalWrite(led_pin, LOW);
}

void pc_input() {                              //читалка с soft_serial
  char inChar;
  count = 0;
  pc.println(F("input ID and command in format idcommand (example: !0201)"));
//  if (message_pc) {                           //если флаг message_pc=true, тогда отображать текстовую приглашалку, чтобы не спамило в консоль
//    pc.println(F("input ID and command in format idcommand (example: !0201)"));
//    message_pc = false;
//  }
  while (true) {
    if (pc.available()) {
      count++;
      inChar = pc.read();
      if (inChar == 13) {
        stringComplete = true;              //ввод с pcSerial был и закончен.
        message_pc = false;
        inputSerial[count] = '\0';

       //debug
       pc.print(" count: ");pc.println(count);
       //debug
        if (verify_string() != 0) {   //если проверка не прошла - вызов по рекурсии pc_input()
          message_pc = true;
          pc_input();
        }
        break;
      }
      inputSerial [count] = inChar;
    }
  }
}

byte verify_string() {                 //верификация ввода команды с Serial_pc
  boolean flag = false;
  //проверка заголовка
  if (inputSerial[1] != ask) {
    pc.print("Wrong header = ");pc.println(byte(inputSerial[1]));
    return (1);
  }
  //проверка id
  for (byte i = 1; i <= nn; i++) {  //перебор id 
    char* sub[3];
    sub[3] = inputSerial[2] + inputSerial[3]+'\0';
    pc.print("inputSerial[2]");pc.println(inputSerial[2]);
    pc.print("inputSerial[3]");pc.println(inputSerial[3]);
    pc.print("check id, sub[2] = ");pc.println(sub[2]);
    if (sub[2] != id_m[i]) flag = false;
  }
  if (flag == false) {          //если в 1..2 байтах не встретился id из доступных
    pc.println("Wrong id");
    return (2);
  }
  return (0);
  //секция команды не проверяется, если что не так - слэйв вернет unknow command, конец пакета '%'  так же не проверяем, т.к. вручную его вводить не будем
}



