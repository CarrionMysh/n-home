//псевдомастер
#include <n_home.h>

#define self_id 10
#define pin_tr 4                          //пин transmission enable для max485
#define tx_ready_delay 1                  //задержка для max485 (буфер? вообщем, неопределенность буфераmax485)
#define pin_led 13                         //светодиод активности *debug
byte alien_id;
byte com;
byte nn;
//NetHome::NetHome (byte self_id, byte pin_tr, byte tx_ready_delay, unsigned int speed = 115200)
NetHome net(self_id, pin_tr, tx_ready_delay, 115200);

void setup() {
  net.begin();
  pinMode(pin_led, OUTPUT);
  digitalWrite(pin_led, LOW);
}

void loop() {
  alien_id = 22;
  com = 50;
  nn = 10;
  for(byte i=0; i<=10; i++){
    net.setData(i,i);
    }
  net.transmite(alien_id,com,nn);
  led_tr();
  if (net.recive(1) == 94) {
    if (net.getCom() == 94) led_ok();
  }
  delay(2000);
}

void led_ok(){
  digitalWrite(pin_led, HIGH);
  delay(50);
  digitalWrite(pin_led, LOW);
  delay(50);
  digitalWrite(pin_led, HIGH);
  delay(50);
  digitalWrite(pin_led, LOW);
}

void led_tr(){
  digitalWrite(pin_led, HIGH);
  delay(50);
  digitalWrite(pin_led, LOW);
}
