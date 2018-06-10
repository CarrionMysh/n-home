#include <FastCRC_tables.h>
#include <FastCRC.h>
#include <FastCRC_cpu.h>
//для передачи в линию используется хардварный serial
#define pin_tr 4            //пин transmission enable для max485
#define led_pin 13        //светодиод активности *debag
#define tx_ready_delay 20   //задержка для max485 (буфер? вообщем, неопределенность буфераmax485)
#define ask '!'
#define response '&'
#define heartbit '#'
#define self_id 02              //свой id

const byte value_data = 10; //размер пакета
byte status_rx = 0;
//статус приема, 0="ok"
//1="нет конца, таймаут"
//2="нет начала"
//3="неизвестный заголовок"
char data[value_data];
char data_tx[value_data];
boolean ask_f, response_f, heartbit_f;       //флаги заголовков

void setup() {
  Serial.begin(115200, SERIAL_8E1);
  pinMode(led_pin, OUTPUT);
  pinMode(pin_tr, OUTPUT);
  digitalWrite(led_pin, LOW);
  digitalWrite(pin_tr, LOW);
  ask_f = false;
  response_f = false;
  heartbit_f = false;
}

void loop() {

}

void net_main() {          //финальная и основная работа с "сетью"
  char ch[value_data - 4 - 1];
  rx_c();                           //слушаем сеть
  switch (status_rx) {
    case 1:
      ch[value_data - 4 - 1] = "99";
      tx_c(ch[value_data - 4 - 1]);
    case 2:
    //нет вариантов по обработке неверного заголовка
    case 3:
      ch[value_data - 4 - 1] = "99";
      tx_c(ch[value_data - 4 - 1]);
  }
}

void tx_c(char tx_s[value_data - 4 - 1] ) {             //-1 т.к. нумерация в массиве идет с 0
  data_tx[value_data] = '>' + response + self_id + tx_s[value_data - 4 - 1 + '<']; //формируем исходящий пакет
  tx_up();
  Serial.print(data_tx[value_data]);         //передаем в сеть данные
  tx_down();
}

byte com_c() {          //парсим содержимое, пропускаем data[0] - '>', data[1..2] - 'id'
  byte bytevar = 0;
  switch (data[1]) {      //проверка типа заголовка
    case ask:
      ask_f = true;
      response_f = false;         //видим ask
      if (!id_c()) return (0);             //вызываем id_c() если id не наш - выходим. здесь и ниже в процедуре
    //return (0) - означет что команд пакет не несет
    case response:
      ask_f = false;
      response_f = true;
      return (0);                             //увидели response - значит пакет есть ответ другого слэйва - игнорируем
defaut:
      ask_f = false;
      response_f = false;
      status_rx = 3;                       //заголовок неизвестный, выходим с status_rx=3
      return (0);
  }
  if (data[4] == heartbit) {     //продолжаем и проверяем есть ли в пакете heartbit
    heartbit_f = true;
    return (0);
  }
  //т.к. пакет адресован нам - дергаем команды
  bytevar = 10 * (data[4] - '0') + (data[5] - '0');
  return (bytevar);                 //выходим, возвращем код команды
}

boolean id_c() {           //проверяем свой/не свой id
  byte bytevar = 0;
  bytevar = 10 * (data[2] - '0') + (data[3] - '0');           //проверить содержание data[2]/data[3], должен быть id
  if (bytevar == self_id) return true; else return false;
}

boolean rx_c() {
  boolean flag_data;
  byte count;
  char ch;
  unsigned long timeout = 100; // таймаут приема
  unsigned long time_rx;     //переменная времени

  flag_data = false;
  count = 0;
  while (true) {
    if (Serial.available()) {
      flag_data = true;
      time_rx = millis();
      ch = Serial.read();
      data[count] = ch;
      //            debug.print("data[]="); debug.println(data[count]);
      //            debug.print("count="); debug.println(count);

      if (data[count] == '<') {       //видим '<' - конец пакета
        status_rx = 0;
        //                debug.println("-------------------------");
        break;
      }
      count++;
      if (count > 10) count = 0;
    }
    //пакет получили

    if (flag_data && ((millis() - time_rx) > timeout)) {        //были данные, но выходим по таймауту
      status_rx = 1;
      //            debug.println((millis() - time_rx));
      return false;
    }
  }
  if (data[0] != '>') {
    status_rx = 2;
    return false;
  }
  digitalWrite(led_pin, LOW);
  //    debug.println((millis() - time_rx));
  return true;
}

void tx_up() {
  digitalWrite(pin_tr, HIGH);
  digitalWrite(led_pin, HIGH);
}
void tx_down() {
  delay(tx_ready_delay);
  digitalWrite(pin_tr, LOW);
}
