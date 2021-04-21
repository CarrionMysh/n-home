//псевдослэйв
#include <n_home.h>

#define self_id 22
#define pin_tr 4                          //пин transmission enable для max485
#define tx_ready_delay 1                  //задержка для max485 (буфер? вообщем, неопределенность буфераmax485)
#define pin_led 13                         //светодиод активности *debug
byte alien_id;
byte com;
byte nn;
byte state_rx;

NetHome net(self_id, pin_tr, tx_ready_delay, 115200);


void setup() {
  net.begin();
  pinMode(pin_led, OUTPUT);
  digitalWrite(pin_led, LOW);

}

void loop() {
    state_rx = net.recive(0);
    if (state_rx == 94 || state_rx == 100 ){
      led_ok();
      com = net.getCom();
      alien_id = net.getID();
      //бла-бла-бла, исполнительный блок
      net.transmite(alien_id, 94, false);
      led_tr();
    }
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
