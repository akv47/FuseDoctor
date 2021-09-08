#include <avr/eeprom.h>

#define  HFUSE_8  0xD9  // дефолтные фьюзы для ATmega 8
#define  LFUSE_8  0xE1 // дефолтные фьюзы для ATmega 8
#define  HFUSE_48_88_168  0xDF  // дефолтные фьюзы для ATmega 168
#define  LFUSE_48_88_168  0x62  // дефолтные фьюзы для ATmega 168
#define  HFUSE_328  0xD9  // дефолтные фьюзы для ATmega 328
#define  LFUSE_328  0x62  // дефолтные фьюзы для ATmega 328
 
//Command Byte
#define Chip_Erase B10000000 
#define Write_Fuse_bits B01000000
#define Write_Lock_bits B00100000 
#define Write_Flash B00010000 
#define Write_EEPROM B00010001 
#define Read_Signature_Calibration_byte B00001000 
#define Read_Fuse_Lock_bits B00000100 
#define Read_Flash B00000010 
#define Read_EEPROM B00000011 

#define SignatureByte0 0
#define SignatureByte1 1
#define SignatureByte2 2
#define _HF 3
#define _LF 4

// Назначение пинов
#define  DATA  PORTD // PORTD порт данных
#define  DATAD  DDRD
#define  VCC  A1// 15
#define  RDY  9 // RDY/BSY сигнал от МК
#define  OE  10 
#define  WR  11  
#define  BS1  12 
#define  XA0  A3 //17  
#define  XA1  A4 //18 
#define  PAGEL A5  //19  
#define  RST  13 //Выход подачи 12 вольт на RESET
#define  BS2   8 
#define  XTAL1  A2 //16 
#define  BUTTON  A0 //14 Кнопка


byte  sb0, sb1, sb2, HF, LF;

void setup()  
{
  Info();
   
  DATA = 0x00;  
  DATAD = 0xFF; 
  pinMode(VCC, OUTPUT);
  pinMode(RDY, OUTPUT);
  pinMode(OE, OUTPUT);
  pinMode(WR, OUTPUT);
  pinMode(BS1, OUTPUT);
  pinMode(XA0, OUTPUT);
  pinMode(XA1, OUTPUT);
  pinMode(PAGEL, OUTPUT);
  pinMode(RST, OUTPUT); 
  pinMode(BS2, OUTPUT);
  pinMode(XTAL1, OUTPUT);

  pinMode(BUTTON, INPUT);
  digitalWrite(BUTTON, LOW);  
  digitalWrite(RDY, LOW);
   
  digitalWrite(RST, 1);  // Выключаем 12V
  digitalWrite(VCC, 0);
}

void loop()  
{
  int Jmp = analogRead(BUTTON);
   
  while( Jmp < 30) { Jmp = analogRead(BUTTON);   // Ждем пока кнопка не нажата
  }
   if (Jmp >=  30 && Jmp < 60){
                 HF = HFUSE_328; 
                 LF = LFUSE_328;
                }
   else  if (Jmp >= 60 && Jmp < 400)  {
                 HF = HFUSE_48_88_168; 
                 LF = LFUSE_48_88_168;
                }
   else  if (Jmp >= 400) {
                 HF = HFUSE_8; 
                 LF = LFUSE_8;
                }  
 /* Serial.begin(9600);
  delay(100);
  Serial.println();
  Serial.print("HighFuse: 0x"); Serial.println(HF, HEX); 
  Serial.print("LowFuse 0x"); Serial.println(LF, HEX);
  Serial.end();
  delay(1000);   */                                
  // Инициализация режима программирование
  digitalWrite(PAGEL, 0);
  digitalWrite(XA1, 0);
  digitalWrite(XA0, 0);
  digitalWrite(BS1, 0);
  digitalWrite(BS2, 0);
  digitalWrite(VCC, 0);
  delay(50); 
  // Вход в режим программировния
  digitalWrite(VCC, 1);  // Подаем питание на МК
  digitalWrite(WR, 1);  
  digitalWrite(OE, 1);
  delay(10);     //Ждем нормального фронта питания >1.8В

  digitalWrite(RST, 0);  // Включаем 12V
  delay(300);
 
 //стираем МК
  sendcmd(1, 0, 0, 0, Chip_Erase, 1); // отправляем команду стирания
  delay(500);

  //и LFUSE
  sendcmd(1, 0, 0, 0, Write_Fuse_bits, 0);  // отправим команду на запись FUSE
  sendcmd(0, 1, 0, 0, HF, 1);  // запись LFUSE

  // Запишем HFUSE  
  sendcmd(1, 0, 0, 0, Write_Fuse_bits, 0);  // отправим команду на запись FUSE
  sendcmd(0, 1, 1, 0, HF, 1);  // запись HFUSE 
  
  sendcmd(1, 0, 0, 0, Write_Fuse_bits, 0);  // отправим команду на запись FUSE
  sendcmd(0, 1, 0, 0, LF, 1);  // запись LFUSE

  //Прочитаем сигнатуру 
  
  sb0 = getSignature(0); // 1 байт сигнатуры
  eeprom_write_byte(SignatureByte0, sb0); 
    
  sb1 = getSignature(1); // 2 байт сигнатуры
  eeprom_write_byte(SignatureByte1, sb1);  
   
  sb2 = getSignature(2); // 3 байт сигнатуры
  eeprom_write_byte(SignatureByte2, sb2);

   //Прочитаем HighFuse
   HF = readFuse(1);
   eeprom_write_byte(_HF,  HF);
   
   //LowFuse
   LF = readFuse(0);
   eeprom_write_byte(_LF, LF);
   
  // выходим из режима программирования
  digitalWrite(RST, 1); // Выключаем 12V

  // выключаем все пины
  DATA = 0x00;
  digitalWrite(OE, 0);
  digitalWrite(WR, 0);
  digitalWrite(PAGEL, 0);
  digitalWrite(XA1, 0);
  digitalWrite(XA0, 0);
  digitalWrite(BS1, 0);
  digitalWrite(BS2, 0);
  digitalWrite(VCC, 0);

  delay(4000);  // задержка для кнопки
  
}


void sendcmd(bool b_XA1, bool b_XA0, bool b_BS1, bool b_BS2, byte command_data, bool b_WR)  // команда
{
  digitalWrite(XA1, b_XA1); 
  digitalWrite(XA0, b_XA0);
  digitalWrite(BS1, b_BS1);
  digitalWrite(BS2, b_BS2);
  DATA = command_data;
  digitalWrite(XTAL1, 1);  // пульс XTAL для отправки команды
  delay(10);
  digitalWrite(XTAL1, 0);
  delay(10);
  if (b_WR) {
     digitalWrite(WR, 0);
     delay(300);
     digitalWrite(WR, 1);
     delay(10);
  }
  digitalWrite(BS1, 0);
  digitalWrite(BS2, 0);
  delay(10);
}

byte getSignature( byte _adress )  {      //считывание сигнатуры
  byte FB;
  sendcmd(1, 0, 0, 0, Read_Signature_Calibration_byte, 0);
  sendcmd(0, 0, 0, 0, _adress, 0); 
  DATAD = 0x00; //на вход
  DATA = 0x00;
  digitalWrite(BS1, 0); 
  digitalWrite(OE, 0);
  delay(200);
  FB = PIND;
  digitalWrite(OE, 1);
  delay(10);
  DATAD = 0xFF; //на выход
  return FB;
}

byte readFuse(bool Bit_Fuse ){  //считывание HighFuse (Bit_Fuse=1)  и LowFuse (Bit_Fuse=0)
  byte FB;
   sendcmd(1, 0, 0, 0, Read_Fuse_Lock_bits, 0);  
   DATAD = 0x00; //на вход
   DATA = 0x00; 
   digitalWrite(BS1, Bit_Fuse);
   digitalWrite(BS2, Bit_Fuse);
   digitalWrite(OE, 0);
   delay(200);
   FB = PIND;
   digitalWrite(OE, 1);
   delay(10);
   DATAD = 0xFF; //на выход
   return FB;
}

void Info(){
  sb0 = eeprom_read_byte(SignatureByte0);
  sb1 = eeprom_read_byte(SignatureByte1);
  sb2 = eeprom_read_byte(SignatureByte2);
  HF = eeprom_read_byte(_HF);
  LF = eeprom_read_byte(_LF);
   
  Serial.begin(9600);
  Serial.println();
  Serial.print("Signature: 0x"); Serial.print(sb0, HEX); 
  Serial.print(" 0x"); Serial.print(sb1, HEX);   
  Serial.print(" 0x"); Serial.print(sb2, HEX);
  Serial.println();
  Serial.print("HighFuse: 0x"); Serial.println(HF, HEX); 
  Serial.print("LowFuse 0x"); Serial.println(LF, HEX);   
  Serial.end();
}
