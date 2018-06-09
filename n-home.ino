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
#define ask '!'
#define response '&'
#define heartbit "##"

SoftwareSerial pc(tx_pc, rx_pc);        //инициализация Serial для получения команд с компа, для альфы

char data[value_data];
String id_m[] = {"02", "03", "04", "05"}; //массив с id устройств, 01 - master
byte nn = 4;                                          //количество id
String com_m[2][9] =
{
  {"01", "02", "03", "04", "05", "06", "07", "08", "09"},
  {"01", "02", "03", "04", "05", "06", "07", "08", "09"}
};
String inputSerial = "";
boolean stringComplete = false;
boolean message_pc = true;
//коды ошибок:
//0 - ok
//1 - header
//2 - id
//20 - unknow

void setup() {
  Serial.begin(115200, SERIAL_8E1);
  pc.begin(9600);
  pinMode(led_pin, OUTPUT);
  pinMode(pin_tr, OUTPUT);
  digitalWrite(led_pin, LOW);
  digitalWrite(pin_tr, LOW);
  pc.println(F("Serial ok"));
}

void loop() {
  pc_input();
  if (stringComplete == true) transmite_com();
}

void transmite_com() {
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
  inputSerial = "";                              //обнуляем введеную ранее строчку с консоли
  if (message_pc) {                           //если флаг message_pc=true, тогда отображать текстовую приглашалку, чтобы не спамило в консоль
    pc.println(F("input ID and command in format idcommand (example: 0201)"));
    message_pc = false;
  }
  while (pc.available()) {
    char inChar = (char)pc.read();
    inputSerial += inChar;
    if (inChar == '\n') {
      stringComplete = true;              //ввод с pcSerial был и закончен.
      message_pc = false;
      pc.println("debug: " + inputSerial);
      if (verify_string() != 0) {   //если проверка не прошла - вызов по рекурсии pc_input()
        message_pc = true;
        pc_input();
      }
    }
  }
}


byte verify_string() {                 //верификация ввода команды с Serial_pc
  boolean flag = false;
  //проверка заголовка
  if (inputSerial.charAt(0) != ask || inputSerial.charAt(0) != response) {
    pc.println("Wrong header");
    return (1);
  }
  //проверка id
  for (byte i = 0; i < nn - 1; i++) {  //перебор id (nn-1, т.к. нумерация масива идет с 0)
    if (inputSerial.substring(1, 3) != id_m[i]) flag = false;
  }
  if (flag == false) {          //если в 1..2 байтах не встретился id из доступных
    pc.println("Wrong id");
    return (2);
  }
  return (0);
  //секция команды не проверяется, если что не так - слэйв вернет unknow command, конец пакета '%'  так же не проверяем, т.к. вручную его вводить не будем
}



