#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>

// ===== Config =====
#define NUM_DELAYS 6 //ห้ามเปลี่ยน
#define PUMP_PIN 33 //Pinปั้ม
int delayPins[NUM_DELAYS] = {13, 12, 14, 27, 26, 25}; //กำหนด pin วาวล์ ไล่ 1,2,3,4,5,6
int delayDuration[NUM_DELAYS] = {10,10,0,10,10,10}; // กำหนดเวลาจะให้วาวล์ทำงาน หน่วยวินาที (30 = 30วินาที)(60=1นาที) ไล่ตามลำดับ{วาวล์1,วาวล์2,วาวล์3,วาวล์4,วาวล์5,วาวล์6} ไม่ใช้งานให้ใส่ 0
//วิธีคำนวณ นาที ให้นำ วินาที*60 เช่น 1*60 = 1นาที, 2*60 = 2นาที
//วิธีคำนวณ ชั่วโมง ใหนนำ วินาที*60*60 เช่น 1*60*60=1ชั่วโมง, 2*60*60=2ชั่วโมง
int preStart = 2; // ให้ Delay ตัวถัดไปเริ่มก่อน 2 วิ ป้องกันท่อแตก ปรับเปลี่ยนได้(หน่วยวินาที)
int pumpStartOffset = 2; // Pump start หลัง Delay1 2 วิ
int pumpStopOffset = 2;  // Pump stop ก่อน Delay6 2 วิ

// Schedule: ชั่วโมง, นาที
int schedule[][2] = { {9,0}, {9,50}, {14,43}, {20,0} }; //กำหนดเวลาทำงาน {ชั่วโมง,นาที} เช่น {9,0} = จะเริ่มทำงานเวลา 09:00 น. , {15,48} =  จะเริ่มทำงานเวลา 15:48 น.

#define NUM_SCHEDULES (sizeof(schedule)/sizeof(schedule[0]))

RTC_DS3231 rtc; //นาฬิกา
LiquidCrystal_I2C lcd(0x27,20,4); //จอ


struct DelayState{
  bool running;
  DateTime startTime;
};
DelayState delays[NUM_DELAYS];
bool pumpRunning=false;
unsigned long lastBlinkMillis=0;
bool blinkState=false;
bool scheduleRunToday[NUM_SCHEDULES];
static int lastDay=-1;

// ===== Setup =====
void setup(){
  Serial.begin(115200);

  pinMode(PUMP_PIN,OUTPUT);
  digitalWrite(PUMP_PIN,LOW);
  for(int i=0;i<NUM_DELAYS;i++){
    pinMode(delayPins[i],OUTPUT);
    digitalWrite(delayPins[i],LOW);
    delays[i].running=false;
  }

  Wire.begin();
  lcd.init();
  lcd.backlight();

  if(!rtc.begin()){
    lcd.print("RTC error");
    while(1);
  }

  for(int i=0;i<NUM_SCHEDULES;i++) scheduleRunToday[i]=false;
}

// ===== Loop =====
void loop(){
  DateTime now = rtc.now(); 
  // DateTime now = rtc.now() + TimeSpan(7*3600);

  if(now.day() != lastDay){
    lastDay = now.day();
    for(int i=0;i<NUM_SCHEDULES;i++) scheduleRunToday[i]=false;
  }

  checkSchedule(now);
  updateDelays(now);
  updatePump(now);
  updateLCD(now);

  delay(200);
}


void startDelays(DateTime startDelayTime){
  delays[0].startTime = startDelayTime;
  delays[0].running = false;
  for(int i=1;i<NUM_DELAYS;i++){
    delays[i].startTime = delays[i-1].startTime + TimeSpan(delayDuration[i-1]-preStart);
    delays[i].running = false;
  }

  pumpRunning=false;
  digitalWrite(PUMP_PIN,LOW);

  for(int i=0;i<NUM_DELAYS;i++) digitalWrite(delayPins[i],LOW);
}

void updateDelays(DateTime now){
  for(int i=0;i<NUM_DELAYS;i++){
    if(!delays[i].running && now.unixtime() >= delays[i].startTime.unixtime()){
      delays[i].running=true;
      digitalWrite(delayPins[i],HIGH);
    }
    if(delays[i].running && now.unixtime() >= delays[i].startTime.unixtime() + delayDuration[i]){
      delays[i].running=false;
      digitalWrite(delayPins[i],LOW);
    }
  }
}

void updatePump(DateTime now){
  if(!pumpRunning && now.unixtime() >= delays[0].startTime.unixtime() + pumpStartOffset){
    pumpRunning=true;
    digitalWrite(PUMP_PIN,HIGH);
  }
  if(pumpRunning && now.unixtime() >= delays[NUM_DELAYS-1].startTime.unixtime() + delayDuration[NUM_DELAYS-1] - pumpStopOffset){
    pumpRunning=false;
    digitalWrite(PUMP_PIN,LOW);
  }
}

void checkSchedule(DateTime now){
  for(int i=0;i<NUM_SCHEDULES;i++){
    DateTime sch(now.year(), now.month(), now.day(), schedule[i][0], schedule[i][1],0);
    if(!scheduleRunToday[i]){
      if(now.unixtime() >= sch.unixtime()){
        startDelays(sch); 
        scheduleRunToday[i]=true;
        break;
      }
    }
  }
}

// ===== Last / Next Run =====
DateTime getLastRun(DateTime now){
  for(int i=NUM_SCHEDULES-1;i>=0;i--){
    DateTime sch(now.year(), now.month(), now.day(), schedule[i][0], schedule[i][1],0);
    if(now.unixtime() >= sch.unixtime()) return sch;
  }
  return DateTime(now.year(), now.month(), now.day()-1, schedule[NUM_SCHEDULES-1][0], schedule[NUM_SCHEDULES-1][1],0);
}

DateTime getNextRun(DateTime now){
  for(int i=0;i<NUM_SCHEDULES;i++){
    DateTime sch(now.year(), now.month(), now.day(), schedule[i][0], schedule[i][1],0);
    if(now.unixtime() < sch.unixtime()) return sch;
  }
  return DateTime(now.year(), now.month(), now.day()+1, schedule[0][0], schedule[0][1],0);
}

// ===== LCD =====
void updateLCD(DateTime now){
  lcd.setCursor(0,0);
  lcd.printf("%02d/%02d/%02d %02d:%02d:%02d  ",
             now.day(), now.month(), now.year()%100,
             now.hour(), now.minute(), now.second());

  DateTime lastRunDT = getLastRun(now);
  lcd.setCursor(0,1);
  lcd.printf("Last: %02d:%02d     ", lastRunDT.hour(), lastRunDT.minute());

  DateTime nextRunDT = getNextRun(now);
  lcd.setCursor(0,2);
  lcd.printf("Next: %02d:%02d     ", nextRunDT.hour(), nextRunDT.minute());

  lcd.setCursor(0,3);
  unsigned long currentMillis = millis();
  if(currentMillis - lastBlinkMillis >=500){
    lastBlinkMillis=currentMillis;
    blinkState = !blinkState;
  }

  bool anyRunning=false;
  for(int i=0;i<NUM_DELAYS;i++){
    if(delays[i].running){
      anyRunning=true;
      if(blinkState){
        int secRemain = (delays[i].startTime.unixtime() + delayDuration[i]) - now.unixtime();
        lcd.printf("Delay%d %2ds  ", i+1, secRemain);
      } else lcd.print("               ");
      break;
    }
  }
  if(!anyRunning){
    if(blinkState) lcd.print("Waiting...       ");
    else lcd.print("               ");
  }
}
