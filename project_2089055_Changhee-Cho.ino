#define BOUNCE_TIME 190
#include <LiquidCrystal_I2C.h>
#include <SimpleDHT.h>
#include <MsTimer2.h>
#include <ThreeWire.h>  // ThreeWrie.h, RtcDS1302.h는 Rtc by Makuna 라이브러리 내의 파일임
#include <RtcDS1302.h>

// HW핀 현황
int pinDHT11 = 12;
int mode_btn = 0; //아두이노 2번 핀
int alert_btn = 1; //아두이노 3번 핀
int btn1 = 8;
int btn2 = 9;
int btn3 = 10;
int btn4 = 11;
int buz = 4;
// CLK : 6번 핀
// DAT : 5번 핀
// RST : 7번 핀

char dow[7][4]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};

// 알람용 변수
int a_hour = 0;
int a_min = 0;
int a_now = 0;//low:am high:pm
int a_status = LOW;
int a_hour_t = 0;

// 시간변경용 변수
int c_hour = 0;
int c_hour_t = 0;
int c_min = 0;
int c_now = LOW;

// 날짜변경용 변수
int d_year = 2020;
int d_month = 1;
int d_date = 1;


volatile int alert_status=LOW;
volatile int changed = LOW;
volatile int mode_num = 0;
volatile unsigned long last_int_time1=0;
volatile unsigned long last_int_time2=0;


LiquidCrystal_I2C lcd(0x27,16,2);
SimpleDHT11 dht11(pinDHT11);
ThreeWire three_wire(5, 6, 7);
RtcDS1302<ThreeWire> Rtc(three_wire);

void alert() //MSTimer2 사용
{
  RtcDateTime now = Rtc.GetDateTime();
  int hour = now.Hour(); // 시
  int min = now.Minute(); // 분
  static int beep_status=LOW;
  if(a_status==HIGH && hour==a_hour && min==a_min && beep_status==LOW && alert_status==HIGH)
  {
    digitalWrite(buz,HIGH);
    beep_status=HIGH;
  }
  else
  {
    digitalWrite(buz,LOW);
    beep_status=LOW;
  }
  if(alert_status==LOW && hour>=a_hour && min>a_min && a_status==HIGH)
    alert_status=HIGH;
}
void setup() {
  // RTC 모듈 시작
  Rtc.Begin();

  // 현재 시간 설정
  Rtc.SetDateTime(RtcDateTime(__DATE__, __TIME__));

  // RTC 모듈에 쓰기 금지 모드인 경우
  if (Rtc.GetIsWriteProtected()) {
    Rtc.SetIsWriteProtected(false); // 쓰기 금지 모드 해제
  }
  
  // RTC 모듈이 동작중이 아닌 경우
  if (!Rtc.GetIsRunning()) {
    Rtc.SetIsRunning(true); // RTC 모듈 동작 시작
  }

  
  Serial.begin(9600);
  pinMode(2, INPUT_PULLUP);
  pinMode(3, INPUT_PULLUP);
  pinMode(btn1, INPUT_PULLUP);
  pinMode(btn2, INPUT_PULLUP);
  pinMode(btn3, INPUT_PULLUP);
  pinMode(btn4, INPUT_PULLUP);
  pinMode(buz, OUTPUT);
  lcd.init();
  lcd.backlight();
  MsTimer2::set(1000,alert);
  MsTimer2::start();
  attachInterrupt(mode_btn,change_mode,FALLING);
  attachInterrupt(alert_btn,alert_off,FALLING);
}

void loop() {
  if(changed == HIGH)
  {
    lcd.clear();
    changed=LOW;
  }
  switch(mode_num)
  {
    case 0:
      mode_1();
      break;
    case 1:
      mode_2();
      break;
    case 2:
      mode_3();
      break;
    case 3:
      mode_4();
      break;
    case 4:
      mode_5();
      break;
  }
}

void blink()
{
  static boolean colon_state=HIGH;
  if(colon_state == LOW)
  {
    lcd.print(":");
    colon_state=HIGH;
  }
  else
  {
    lcd.print(" ");
    colon_state = LOW;
  }
}

void change_mode()
{
  unsigned long int_time = millis();
  if(int_time - last_int_time1> BOUNCE_TIME)
  {
    mode_num++;
    changed = HIGH;
    if(mode_num == 5)
      mode_num = 0;
  }
  last_int_time1=int_time;
}

void alert_off()
{
  unsigned long int_time = millis();
  if(int_time - last_int_time2 > BOUNCE_TIME)
  {
    RtcDateTime now = Rtc.GetDateTime();
    int hour = now.Hour(); // 시
    int min = now.Minute(); // 분
    if(alert_status==HIGH&hour==a_hour&&min==a_min)
      alert_status=LOW;
  }
  last_int_time2=int_time;
}

void mode_1()
{
  // RTC 모듈의 현재 시간 얻기
  RtcDateTime now = Rtc.GetDateTime();

  // 변수 선언
  int year = now.Year(); // 년
  int month = now.Month(); // 월
  int day = now.Day(); // 일
  int DayOfWeek = now.DayOfWeek(); // 요일
  int hour = now.Hour(); // 시
  int minute = now.Minute(); // 분
  int second = now.Second(); // 초
  lcd.setCursor(0,0);
  lcd.print(year);
  lcd.print("-");
  if(month<10)
    lcd.print("0");
  lcd.print(month);
  lcd.print("-");
  if(day<10)
    lcd.print("0");
  lcd.print(day);
  lcd.print("(");
  lcd.print(dow[DayOfWeek]);
  lcd.print(")");
  lcd.setCursor(4,1);

  if((hour<10||(hour>12&&hour<22))&&(hour!=0))
    lcd.print("0");
  if(hour>=13)
    lcd.print(hour-12);
  else if(hour==0)
    lcd.print("12");
  else
    lcd.print(hour);
  blink();
  if(minute<10)
    lcd.print("0");
  lcd.print(minute);
  if(hour>=12)
    lcd.print(" PM");
  else
    lcd.print(" AM");
  if(a_status==HIGH)
  {
    lcd.setCursor(15,0);
    lcd.print("a");
  }
  delay(1000);
}

void mode_2()
{
  byte temperature = 0;
  byte humidity = 0;
  int err = SimpleDHTErrSuccess;
  if((err=dht11.read(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess) {
      Serial.print("Read DHT11 failed, err="); Serial.print(SimpleDHTErrCode(err));
      Serial.print(","); Serial.println(SimpleDHTErrDuration(err)); delay(1000);
      return;
    } //DHT11 샘플링 실패시 시리얼 모니터에 출력
  lcd.setCursor(0,0);
  lcd.print("Temperature:");
  lcd.print((int)temperature); lcd.print((char)223); lcd.print("C"); 
  lcd.setCursor(0,1);
  lcd.print("Humidity:    ");
  lcd.print((int)humidity); lcd.print("%");
  delay(1500);
  // DHT11 sampling rate is 1HZ;
}

void mode_3()
{
  if(a_status==LOW)
  {
    lcd.setCursor(0,0);
    lcd.print("Alarm Settings");
    lcd.setCursor(4,1);
    if(a_hour>0&&a_hour<10)
      lcd.print("0");
    else if(a_hour==0)
      lcd.print("12");
    if(a_hour!=0&&a_hour<=12)
      lcd.print(a_hour);
    if((a_hour>12))
    {
      if((a_hour-12)<10)
      {
        lcd.print("0");
        lcd.print(a_hour-12);
      }
      else if(a_hour==12)
        lcd.print("12");
      else
        lcd.print(a_hour-12);
    }
    lcd.print(":");
    if(a_min<10)
      lcd.print("0");
    lcd.print(a_min);
    if(a_now==HIGH)
      lcd.print(" PM");
    else
      lcd.print(" AM");
    lcd.setCursor(15,0);
    lcd.print("X");
    if(!digitalRead(btn1))
    {
      a_hour_t++;
      if(a_hour_t==12)
        a_hour_t=0;
      lcd.setCursor(4,1);
      if(a_hour_t<10&&a_hour_t>0)
        lcd.print("0");
      if(a_hour_t==0)
        lcd.print("12");
      else
        lcd.print(a_hour_t);
      delay(300);
    }
    if(!digitalRead(btn2))
    {
      a_min++;
      if(a_min==60)
        a_min=0;
      lcd.setCursor(7,1);
      if(a_min<10)
        lcd.print("0");
      lcd.print(a_min);
      delay(300);
    }
    if(!digitalRead(btn3))
    {
      a_now = !a_now;
      if(a_now == LOW)
        lcd.print(" AM");
      else
        lcd.print(" PM");
      delay(300);
    }
    if(!digitalRead(btn4))
    {
      if(a_status==LOW)
      {
        alert_status=HIGH;
        a_status=HIGH;
      }
      else
      {
        alert_status=LOW;
        a_status=LOW;
      }
      delay(300);
    }
    if(a_now==LOW)
      a_hour=a_hour_t;
    else
      a_hour=a_hour_t+12;
   }
   else
   {
      lcd.setCursor(0,0);
      lcd.print("AlarmActivated");
      lcd.setCursor(4,1);
      if(a_hour>0&&a_hour<10)
        lcd.print("0");
      else if(a_hour==0)
        lcd.print("12");
      if(a_hour!=0&&a_hour<=12)
        lcd.print(a_hour);
      if((a_hour>12))
      {
        if((a_hour-12)<10)
        {
          lcd.print("0");
          lcd.print(a_hour-12);
        }
        else if(a_hour==12)
          lcd.print("12");
        else
          lcd.print(a_hour-12);
      }
      lcd.print(":");
      if(a_min<10)
        lcd.print("0");
      lcd.print(a_min);
      if(a_now==HIGH)
        lcd.print(" PM");
      else
        lcd.print(" AM");
      lcd.setCursor(15,0);
      lcd.print("O");
        if(!digitalRead(btn4))
        {
          a_status=LOW;
          lcd.init();
          lcd.setCursor(0,0);
          lcd.print("Alarm Settings");
          lcd.setCursor(4,1);
          if(a_hour>0&&a_hour<10)
            lcd.print("0");
          else if(a_hour==0)
            lcd.print("12");
          if(a_hour!=0&&a_hour<=12)
           lcd.print(a_hour);
          if((a_hour>12))
          {
            if((a_hour-12)<10)
            {
              lcd.print("0");
              lcd.print(a_hour-12);
            }
            else if(a_hour==12)
              lcd.print("12");
            else
              lcd.print(a_hour-12);
          }
          lcd.print(":");
          if(a_min<10)
            lcd.print("0");
          lcd.print(a_min);
          if(a_now==HIGH)
            lcd.print(" PM");
          else
            lcd.print(" AM");
          lcd.setCursor(15,0);
          lcd.print("X");
          delay(300);
        }
  }
}

void mode_4()
{
  lcd.setCursor(0,0);
  lcd.print("Clock Settings");
    lcd.setCursor(4,1);
  if(c_hour>0&&c_hour<10)
    lcd.print("0");
  else if(c_hour==0)
    lcd.print("12");
  if(c_hour!=0&&c_hour<=12)
    lcd.print(c_hour);
  if((c_hour>12))
  {
    if((c_hour-12)<10)
    {
      lcd.print("0");
      lcd.print(c_hour-12);
    }
    else if(c_hour==12)
      lcd.print("12");
    else
      lcd.print(c_hour-12);
  }
  lcd.print(":");
  if(c_min<10)
    lcd.print("0");
  lcd.print(c_min);
  if(c_now==HIGH)
    lcd.print(" PM");
  else
    lcd.print(" AM");

  if(!digitalRead(btn1))
  {
    c_hour++;
    if(c_hour==12)
      c_hour=0;
    lcd.setCursor(4,1);
    if(c_hour<10&&c_hour>0)
      lcd.print("0");
    if(c_hour==0)
      lcd.print("12");
    else
      lcd.print(c_hour);
    delay(300);
  }
  if(!digitalRead(btn2))
  {
    c_min++;
    if(c_min==60)
      c_min=0;
    lcd.setCursor(7,1);
    if(c_min<10)
      lcd.print("0");
    lcd.print(c_min);
    delay(300);
  }
  if(!digitalRead(btn3))
  {
    c_now = !c_now;
    lcd.setCursor(10,1);
    if(c_now == LOW)
      lcd.print("AM");
    else
      lcd.print("PM");
    delay(300);
  }
  if(!digitalRead(btn4))
  {
    if(c_now==HIGH)
      c_hour+=12;
    RtcDateTime now = Rtc.GetDateTime();
    int year = now.Year(); // 년
    int month = now.Month(); // 월
    int day = now.Day(); // 일
    Rtc.SetDateTime(RtcDateTime(year,month,day,c_hour,c_min,0));
    lcd.init();
    lcd.setCursor(0,0);
    lcd.print("Clock Setting");
    lcd.setCursor(0,1);
    lcd.print("Completed");
    c_hour=0;
    c_min=0;
    c_hour_t=LOW;
    delay(2000);
    mode_num=0;
    lcd.init();
  }
}
void mode_5()
{
  lcd.setCursor(0,0);
  lcd.print("Date Settings");
  lcd.setCursor(3,1);
  lcd.print(d_year);
  lcd.print("-");
  if(d_month<10)
    lcd.print("0");
  lcd.print(d_month);
  lcd.print("-");
  if(d_date<10)
    lcd.print("0");
  lcd.print(d_date);
  if(!digitalRead(btn1))
  {
    d_year++;
    if(d_year==2031)
      d_year=2020;
    lcd.setCursor(3,1);
    lcd.print(d_year);
    delay(300);
  }
  if(!digitalRead(btn2))
  {
    d_month++;
    if(d_month==13)
      d_month=1;
    lcd.setCursor(8,1);
    if(d_month<10)
      lcd.print("0");
    lcd.print(d_month);
    delay(300);
  }
  if(!digitalRead(btn3))
  {
    d_date++;
    if(d_date==32)
      d_date=1;
    lcd.setCursor(11,1);
    if(d_date<10)
      lcd.print("0");
    lcd.print(d_date);
    delay(300);
  }
  if(!digitalRead(btn4))
  {
    RtcDateTime now = Rtc.GetDateTime();
    int hour = now.Hour(); // 시
    int minute = now.Minute(); // 분
    int second = now.Second(); // 초
    Rtc.SetDateTime(RtcDateTime(d_year,d_month,d_date,hour,minute,second));
    lcd.init();
    lcd.setCursor(0,0);
    lcd.print("Date Setting");
    lcd.setCursor(0,1);
    lcd.print("Completed");
    d_year=2020;
    d_month=1;
    d_date=1;
    delay(2000);
    mode_num=0;
    lcd.init();
  }
}
