#include <n_home.h>

NetHome::NetHome (byte self_id, byte pin_tr, byte tx_ready_delay, unsigned long speed = 115200){
  _self_id = self_id;
  _pin_tr = pin_tr;
  _tx_ready_delay = tx_ready_delay;
  _speed = speed;

}

void NetHome::begin(){
  Serial.begin(_speed);
  pinMode(_pin_tr, OUTPUT);
  digitalWrite(_pin_tr, LOW);
}

void NetHome::transmite (byte id, byte com_c, byte nn_data) {
  digitalWrite(_pin_tr, HIGH);
  FastCRC8 CRC8;
  _nn = nn_data;
  _net_packet[0] = id;
  _net_packet[1] = _self_id;
  _net_packet[2] = com_c;
  _net_packet[3] = _nn;
  Serial.print('>');
  Serial.print(char(CRC8.smbus(_net_packet,4)));
  for (byte i = 0; i<4; i++) {
		Serial.print(char(_net_packet[i]));
	}
  if(_nn != 0) {
		delay(10);
		Serial.print(char(CRC8.smbus(_data, nn_data)));
		Serial.write(_data,nn_data);
	}
	delay(_tx_ready_delay);
	digitalWrite(_pin_tr, LOW);
}

void NetHome::setData(byte i, byte data_n){
  _data[i] = data_n;
}

byte NetHome::getData(byte i){
  return _data[i];
}

byte NetHome::getCom(){
  return (_com);
}

void NetHome::setTimeout(unsigned long timeout_tick){
  _timeout_tick = timeout_tick;
}

byte NetHome::recive(boolean mode){
  FastCRC8 CRC8;
  char ch;
  unsigned int count;             //счетчик байтов с линии
	boolean begin_of_packet = false;         //флаг начала пакета
	byte crc_incoming;
	byte crc_calc;
	unsigned long time_tick;
  //flag_data = false;                       //флаг наличия даты, пока никак не задействован
	time_tick = millis();
	Serial.setTimeout(_timeout_tick);
  while(true) {                            //слушаем "вечно"
  if (((millis()-time_tick) > _timeout_response) && mode){
    // pc.println("timeout_silent");
    return (timeout_silent);
  }
		if (Serial.available()) {
			ch = Serial.read();
			time_tick = millis();
			if (ch == '>') { //видим начало пакета
				Serial.readBytes(_net_packet,5); //читаем с линии пять байтов пакета
				if ((millis()-time_tick) > _timeout_tick) { //если вылетели по таймауту Serial, мы этого не узнаем, поэтому проверям таймаут руками
					// pc.println("timeout_error_packet");
					return(_timeout_error);
				}
        crc_incoming = _net_packet[0];
				crc_calc = CRC8.smbus(&_net_packet[1],4); //для проверки отбрасываем приходящее CRC в пакете
				if (crc_incoming == crc_calc) { //CRC верно
					if (_net_packet[1] == _self_id) { //наш ли пакет
						_alien_id = _net_packet[2];
						_com = _net_packet[3];
						_nn = _net_packet[4];
						if (_nn != 0) { //если 5 байт не нулевой, значит будет data
						  // pc.println("data_find");
							time_tick = millis();
							while(true) {
								if (Serial.available()) { //ловим дату
									crc_incoming = Serial.read(); //первый байт это crc
									Serial.readBytes(_data,_nn); //и читаем-пишем data
									break; //ломаем цикл даты
								}
								if ((millis()-time_tick) > _timeout_tick) { //страховка, если даты вообще не будет. проверка с времени последнего байта
									 // pc.println("data_silence");
									return (_data_error);
								}
							}
							//дата залилась, иначе уловие выше выбьет таймауту
							crc_calc = CRC8.smbus(_data,_nn); //crc даты
							if (crc_incoming == crc_calc) { //проверка на crc дату
								// flag_data = true;
								// pc.println("data_ok");
								return (_ok);
							}
							else {
								 // pc.println("data_crc_error");
								return (_data_error);
							}
						}
						else { //5 байт нулевой, даты не будет
							 // pc.println("packet_ok");
							return (_ok);
						}
					}
					else { // пакет не нам
						 // pc.println("wrong_id");
					}
				}
				else {
					 // pc.println("packet_crc_error");
					return (_packet_error);
				}
			}
		}
	}
}
