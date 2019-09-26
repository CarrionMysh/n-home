//test workmodule relay+75HC595
#include <TimerOne.h>
#define pin_relay 9
#define pin_74hc_data 8
#define pin_74hc_lock 7
#define pin_74hc_clock 6
volatile boolean zero_cross = 0;          //флаг перехода нуля
volatile byte count_step [7];             //счетчик числа проверок после прохождения нуля для каждого симмистора
//byte nn_triac;                          //переменная для выбора симмистора
volatile byte triac_level_bright[7];               //заданный уровень яркости 128..0 для 8 симмисторов
volatile byte bitsToSend = 0;                      //байт (карта) состояний симмисторов
byte dimming = 128;                       //количество уровней яркости 0 = ON, 128 = OFF
byte freqStep = 75;                       //This is the delay-per-brightness step in microseconds.
// This is the delay-per-brightness step in microseconds.
// For 60 Hz it should be 65
// It is calculated based on the frequency of your voltage supply (50Hz or 60Hz)
// and the number of brightness steps you want.
// Realize that there are 2 zerocrossing per cycle. This means
// zero crossing happens at 120Hz for a 60Hz supply or 100Hz for a 50Hz supply.
// To calculate freqStep divide the length of one full half-wave of the power
// cycle (in microseconds) by the number of brightness steps.
// (120 Hz=8333uS) / 128 brightness steps = 65 uS / brightness step
// (100Hz=10000uS) / 128 steps = 75uS/step

void setup() {
	pinMode(pin_relay, OUTPUT);
	pinMode(pin_74hc_data, OUTPUT);
	pinMode(pin_74hc_lock, OUTPUT);
	pinMode(pin_74hc_clock, OUTPUT);

	Timer1.initialize(freqStep);
	Timer1.attachInterrupt(dim_check, freqStep);
	attachInterrupt(0, zero_cross_c, RISING);

	digitalWrite(pin_relay, LOW);
	digitalWrite(pin_74hc_lock, LOW);
	digitalWrite(pin_74hc_data, LOW);
	digitalWrite(pin_74hc_clock, LOW);
	//debug
	digitalWrite(pin_relay, HIGH);    //включаем реле, подаем ввод на симмисторы
	triac_level_bright[0] = 0;
	triac_level_bright[1] = 128;
	triac_level_bright[2] = 128;
	triac_level_bright[3] = 128;
	triac_level_bright[4] = 128;
	triac_level_bright[5] = 128;
	triac_level_bright[6] = 128;
	triac_level_bright[7] = 128;
	//nn_triac = 1;                          //для дебага номер симмистора = 1 (приходящее значение)
	//debug_off
}

void hc74_c(byte nn_triac_sub, int triac_state){
	//byte bitsToSend = 0;                     //инициализируем и обнуляем байт, который зальется в 74HC595
	digitalWrite(pin_74hc_lock, LOW);
	bitWrite(bitsToSend, nn_triac_sub, triac_state); //меняем в 00000000 (bitsToSend) запускаем нужный симмистор, например, 01000000
	shiftOut(pin_74hc_data, pin_74hc_clock, MSBFIRST, bitsToSend);
	digitalWrite(pin_74hc_lock, HIGH);
}

void zero_cross_c(){
	zero_cross = true;
	for (byte i = 0; i <= 7; i++) {
		count_step [i] = 0;                   //обнуляем счетчик отсчетов (времени) для всех симмисторов
	}
  bitsToSend = 0;                         //обнуляем состояния симмисторов
}

void dim_check(){
	for (byte i = 0; i <= 7; i++) {
		if (count_step [i]>=triac_level_bright [i]) {         //сравниваем пройденный счетчик с номером нужной яркости симмистора
			hc74_c(i, HIGH);                          //включаем нужный симмистр
			count_step[i] = 0;                        //обнуляем счетчик для конкретного симмистора
		}
		else {
			count_step[i]++;                          //увеличиваем счетчик отсчетов
		}
	}
}

void loop(){

}
