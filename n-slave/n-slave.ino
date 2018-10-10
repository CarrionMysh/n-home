//не проверяется правильность команды. при необходимости дописать блок проверки.
//соответствено, нет кода ошибки неверной команды.
//необходим блок формировки данных

#include <FastCRC.h>
//для передачи в линию используется хардварный serial
#define pin_tr 4            //пин transmission enable для max485
#define led_pin 13        //светодиод активности *debag
#define tx_ready_delay 20   //задержка для max485 (буфер? вообщем, неопределенность буфераmax485)
#define ask '!'
#define response '&'
#define heartbit '#'
#define self_id 02              //свой id
FastCRC8 CRC8;
const byte value_data = 10; //размер пакета, 6 символов - служебные
char data[value_data];
boolean ask_f, response_f, heartbit_f;       //флаги заголовков
byte end_char;                                          //номер символа '<' в пакете
byte com;                                                 //номер команды из пакета

void setup() {
  Serial.begin(9600, SERIAL_8E1);
  pinMode(led_pin, OUTPUT);
  pinMode(pin_tr, OUTPUT);
  digitalWrite(led_pin, LOW);
  digitalWrite(pin_tr, LOW);
  heartbit_f = false;
}

void loop() {
  com = net_c();                                                //получаем код команды  
  if (com = 10){ digitalWrite(led_pin, LOW);                //не нужно. чисто для проверки компиляции
    delay (2000);
    digitalWrite(led_pin, LOW);

  }
}

byte net_c() {
  if (!rx_c()) return;
  byte bytevar = 0;
  bytevar = 10 * (data[6] - '0') + (data[7] - '0');
  return bytevar;
}

void tx_c(char tx_s[value_data - 4] ) {             //-4: первые 4 симв. - служебные(>|CRC|header|)
  char data_tx[value_data];
  byte crc;
  data_tx[value_data] = response + self_id + tx_s[value_data - 4 + '<']; //формируем исходящий пакет
  crc = CRC8.smbus(data_tx, sizeof(data_tx));
  data_tx[value_data] = '>' + crc + data_tx[value_data]; //добавляем символ начала пакета+crc к пакету
  tx_up();
  Serial.print(data_tx[value_data]);         //передаем в сеть данные
  tx_down();
}

boolean crc_c() {                       //дергаем crc из пакета
  byte crc;
  char subdata[value_data - 3];
  if (data[1] >= 'A') {                   //считаем первый символ
    crc = 16 * (data[1] - ('A' - 10));
  } else {
    crc = 16 * (data[1] - '0');
  }
  if (data[2] >= 'A') {                  //считаем второй символ
    crc = crc + (data[2] - ('A' - 10));
  } else {
    crc = crc + (data[2] - '0');
  }
  for (byte i = 3; i <= end_char; i++) { //для проверки CRC8 передаем строку, начиная с заголовка, после суммы CRC8
    subdata[i - 3] = data[i];
  }
  if (CRC8.smbus(subdata, sizeof(subdata)) == crc) return true; else return false;
}

boolean id_c() {           //проверяем свой/не свой id
  byte bytevar = 0;
  bytevar = 10 * (data[2] - '0') + (data[3] - '0');           //проверить содержание data[2]/data[3], должен быть id
  if (bytevar == self_id) return true; else return false;
}

void tx_up() {
  digitalWrite(pin_tr, HIGH);
  digitalWrite(led_pin, HIGH);
}
void tx_down() {
  delay(tx_ready_delay);
  digitalWrite(pin_tr, LOW);
}

boolean rx_c() {          //получение с сети данных
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
      if (data[count] == '<') {       //видим '<' - конец пакета
        end_char = count;
        break;
      }
      count++;
      if (count > 10) count = 0;
    }
    //пакет получили
    if (flag_data && ((millis() - time_rx) > timeout)) {        //были данные, но выходим по таймауту
      return false;
    }
  }
  if (data[0] != '>') {                                     //некорректное начало пакета
    return false;
  }
  digitalWrite(led_pin, LOW);
  if (!crc_c()) return false;                             //вызываем проверку crc
  if ((data[3] != ask)  || !id_c()) return false;       //если не ask или не свой id - нахер с пляжа
  if (data[6] == heartbit) {                             //стучим сердечком, и ничего более
    tx_c(heartbit);
    return false;                                               //возвращем false, т.к. отстучали heartbit, парсинг команд не нужен
  }
}
