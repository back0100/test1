#include <EEPROM.h>
#include <TimerThree.h>
//*** PWM CONFIGURATION
#define PWM0    2
#define PWM1    3
#define PWM2    4
#define PWM3    5
#define PWM4    6
#define PWM5    7
#define PWM6    8
#define PWM7    9
//*** DI CONFIGURATION  25
#define START 22
#define STOP  23
#define RESET 24
#define MODE  25
#define AS11  26
#define AS12  27
#define AS13  28
#define AS21  29
#define AS22  30
#define AS23  31
#define AS24  32
#define AS31  33
#define AS32  34
#define AS41  35
#define AS42  36
//#define AS43  37   파손
#define AS51  38
#define AS52  39
#define AS61  40
#define AS62  41
#define DI_SEL1  42
#define DI_SEL2  43
#define DI_SEL3  44
#define DISPARE1  45
#define DISPARE2  46
//*** DO CONFIGURATION  7
#define SOL11 47
#define SOL21 48
#define SOL31 49
#define SOL32 50
#define SOL41 51
#define SOL51 52
#define SOL61 53
// AS ON = 1 / OFF = 0
// SOL ON = LOW / OFF = HIGH
//#define DO_SPARE1  52
//#define DO_SPARE2  53
#define MAX_SIZE    16
#define NUM_ADDR      0
#define INTERRUPT_INTERVAL  100000  // 100ms
#define SOLE21_DELAY_TIME      500
#define SOLE11_DELAY_TIME      1000
#define REFRESH_TIME    3000
#define TIMER_INTERVAL  100
#define LOOP_DELAY_TIME   200
unsigned int di_start = 0;
unsigned int di_stop = 0;
unsigned int di_reset = 0;
unsigned int di_mode = 0;
unsigned int di_as11 = 0;
unsigned int di_as12 = 0;
unsigned int di_as13 = 0;
unsigned int di_as21 = 0;
unsigned int di_as22 = 0;
unsigned int di_as23 = 0;
unsigned int di_as24 = 0;
unsigned int di_as31 = 0;
unsigned int di_as32 = 0;
unsigned int di_as41 = 0;
unsigned int di_as42 = 0;
//unsigned int di_as43 = 0; 파손
unsigned int di_as51 = 0;
unsigned int di_as52 = 0;
unsigned int di_as61 = 0;
unsigned int di_as62 = 0;
unsigned int di_sel1 = 0;
unsigned int di_sel2 = 0;
unsigned int di_sel3 = 0;

unsigned int do_sol11 = 0;
unsigned int do_sol21 = 0;
unsigned int do_sol31 = 0;
unsigned int do_sol32 = 0;
unsigned int do_sol41 = 0;
unsigned int do_sol51 = 0;
unsigned int do_sol61 = 0;


unsigned long int m_count = 0;
unsigned int start_flag = 0;
unsigned int m_step = 0;
unsigned int repeat_num = 0;
unsigned int repeat_cnt = 0;
unsigned int i = 0;
char strMsg[MAX_SIZE];
unsigned long int delay_count = 0;
unsigned long int stop_count = 0;
int blade_up_check_count = 0;     //칼날이 상승했다는 상태를 카운트 해주는 변수

void setup() {
    Serial.begin(115200);
    Serial1.begin(9600);

    m_step = 0;
    clean_LCD();
    m_count = EEPROM.read(NUM_ADDR);  // EEPROM에 저장된 작업갯수를 읽음
    delay(100);
    DSP_NUM(m_count);
    delay(100);

    Timer3.initialize(INTERRUPT_INTERVAL);  // 타이머 초기화: 작업중 딜레이 설정용
    for (i=START; i<= DI_SEL3; i++) // DI를 설정
    {
        pinMode(i, INPUT_PULLUP);
    }
    
    for (i=SOL11; i<= SOL61; i++)  // DO를 설정
    {
        pinMode(i, OUTPUT);
        digitalWrite(i, HIGH);
    } 
}

void loop() {
    //* DI 입력 체크, GND 입력이므로 IN 되었을 때 0이므로 반전시켜서 입력받음  
    di_start = !(digitalRead(START));
    di_stop = !(digitalRead(STOP));
    di_reset = !(digitalRead(RESET));
    di_mode = !(digitalRead(MODE));
    di_as11 = !(digitalRead(AS11));
    di_as12 = !(digitalRead(AS12));
    di_as13 = !(digitalRead(AS13));
    di_as21 = !(digitalRead(AS21));
    di_as22 = !(digitalRead(AS22));
    di_as23 = !(digitalRead(AS23));
    di_as24 = !(digitalRead(AS24));
    di_as31 = !(digitalRead(AS31));
    di_as32 = !(digitalRead(AS32));
    di_as41 = !(digitalRead(AS41));
    di_as42 = !(digitalRead(AS42));
    // di_as43 = !(digitalRead(AS43)); 파손
    di_as51 = !(digitalRead(AS51));
    di_as52 = !(digitalRead(AS52));
    di_as61 = !(digitalRead(AS61));
    di_as62 = !(digitalRead(AS62));
    di_sel1 = !(digitalRead(DI_SEL1));
    di_sel2 = !(digitalRead(DI_SEL2));
    di_sel3 = !(digitalRead(DI_SEL3));

    //*** 시작/ 정지 버튼 입력에 따른 처리: 정지를 누르면 start_flag = 0이 되어 홀딩 상태로 만듦
    if ( (di_start == 1) && (di_stop == 0) ) // 시작 버튼이 눌러지면 작업 계속
    {
        start_flag = 1; // 시작 플래그 세트    
    }
    else if ( (di_start == 0) && (di_stop == 1) ) // 정지 버튼이 눌러지면 홀딩 상태로 만듦
    {
        start_flag = 0; // 시작 플래그 리셋
        digitalWrite(SOL61, HIGH); // 칼날들기
        digitalWrite(SOL11, HIGH); // 투입기 복귀
        digitalWrite(SOL21, HIGH); // 센터링 해제
        stop_count++;  // 정지버튼을 길게 누르면
        if (stop_count >= (REFRESH_TIME/LOOP_DELAY_TIME) ) // 정지 버튼을 길게 누르면 모든 시퀀스를 빠져나와 초기화
        {
            for (i=SOL11; i<= SOL61; i++)  // DO를 설정
            {
                pinMode(i, OUTPUT);
                digitalWrite(i, HIGH); 
            }
            m_step = 0;     // 시퀀스 초기화
        }
    }
    else
    {
        stop_count = 0;  // 정지버튼이 눌러지지 않으면 정지 카운트 초기화
    }

    //*** 시작/정지 상태에 따른 작업 상태 디스플레이
    if (start_flag == 1)  // 시작 버튼이 눌러졌으면
    {
        blade_up_check_count = 0;
        if ( (m_step == 0) || (m_step == 1) || (m_step == 2) ) // 스텝 0, 1, 2는 시작/정지버튼 눌러짐에 상관없이 작업대기 디스플레이
            DSP_STATE("WAITING");  // 대기상태 디스플레이
        else                                                  // 나머지 스텝에서는 작업 중 디스플레이
            DSP_STATE("WORKING");   // 작업중 디스플레이
    }
    else if (start_flag == 0)   // 정지버튼이 눌러졌으면
    {
        if ( (m_step == 0) || (m_step == 1) || (m_step == 2) )    // 스텝 0, 1, 2는 시작/정지버튼 눌러짐에 상관없이 작업대기 디스플레이
            DSP_STATE("WAITING");  // 대기상태 디스플레이
        else                                                    // 나머지 스텝에서는 홀딩 디스플레이
            DSP_STATE("HOLDING");   // 작업 중지 중 디스플레이
    }

    //*** 리셋버튼 처리
    if (di_reset == 1) // 리셋 버튼이 눌러지면
    {
        m_count = 0;    // 수량 카운트 초기화
        EEPROM.write(NUM_ADDR, m_count);  // 초기화 후 EEPROM에 저장
        delay(100);
        DSP_NUM(m_count);                  // 작업 갯수 디스플레이
        delay(100);
        start_flag = 0; // 홀딩 여부 결정  
        m_step = 0;     // 시퀀스 초기화 여부 결정
    }

    //*** 작업시퀀스
    switch (m_step) 
    {
    case 0:   
        //if (start_flag == 1)
        //{ 
        digitalWrite(SOL31, LOW);  // 초기 시작
        m_step = 1;
        //} 
        break;
    case 1:
        if ( (di_as11 == 1) && (di_as21 == 1) && (di_as22 == 1) && (di_as23 == 1) && (di_as24 == 1) && (di_as31 == 1) 
        && (di_as41 == 1) && (di_as42 == 0) &&  (di_as51 == 1) && (di_as61 == 1) && (di_as62 == 1) )
        {
            m_step = 2;        
        }
        break;
    case 2:
        if ( (di_start == 1) && (start_flag == 1) )  // 시작버튼 입력이 들어올때까지 대기
        {        
            m_step = 3;            
        }
        break;
    case 3: // 무 투입
        if ( (di_as13 == 1) && (start_flag == 1) )
        {
            m_step = 301;
            
            if ( (di_sel1 == 1) && (di_sel2 == 0) && (di_sel3 == 0) ) { // 절삭 횟수 설정
                repeat_cnt = 1;
            }
            else if ( (di_sel1 == 0) && (di_sel2 == 1) && (di_sel3 == 0) ) {
                repeat_cnt = 2;
            }
            else if ( (di_sel1 == 0) && (di_sel2 == 0) && (di_sel3 == 1) ) {
                repeat_cnt = 3;
            }
            else {
                repeat_cnt = 0;
                m_step = 0;
            }

            if (m_step == 301) {
                digitalWrite(SOL11, LOW);
            }
        }
        break;
    case 301: // 무 투입 후 딜레이 추가
        if ((di_as12 == 1) && (start_flag == 1))
        {
            delay_count = 0;  // 딜레이 카운트를 위한 타이머 초기화
            Timer3.attachInterrupt(timerIsr); // 딜레이를 위한 타이머 온
            m_step = 4;
        }
        break;
    case 4: // 센터링
        if ((delay_count >= (SOLE11_DELAY_TIME/TIMER_INTERVAL)) && (di_as12 == 1) && (start_flag == 1))
        {
            Timer3.detachInterrupt();  // 딜레이 시간이 초과되면 타이머 오프
            digitalWrite(SOL21, LOW);
            delay_count = 0;  // 딜레이 카운트를 위한 타이머 초기화
            Timer3.attachInterrupt(timerIsr); // 딜레이를 위한 타이머 온
            m_step = 5;
        }
        break;
    case 5: // 딜레이 후 투입실린더 복귀 
        if ( delay_count >= (SOLE21_DELAY_TIME/TIMER_INTERVAL) ) //  딜레이 시간만큼 대기 (500/100)
        {
            Timer3.detachInterrupt( );  // 딜레이 시간이 초과되면 타이머 오프
            digitalWrite(SOL11, HIGH);
            m_step = 6;
        }
        break;
    case 6: // 도킹
        if ( (di_as11 == 1) && (start_flag == 1) )
        {
            digitalWrite(SOL31, HIGH);
            digitalWrite(SOL41, LOW);
            m_step = 7;
        }
        break;
    case 7: // 센터링 해제
        if ( (di_as42 == 1) && (start_flag == 1) )
        {
            digitalWrite(SOL21, HIGH);
            m_step = 8;
        }
        break;
    case 8: // 칼날 다운, 푸시하면서 무탈피
        if ( (di_as21 == 1) && (di_as22 == 1) && (di_as23 == 1) && (di_as24 == 1) && (start_flag == 1) )
        {
            digitalWrite(SOL31, HIGH);
            digitalWrite(SOL61, LOW);
            digitalWrite(SOL32, LOW);
            m_step = 9;
        }
        break;





        
    case 9: // 푸셔가 끝에 도달하면 칼날 업, 푸셔복귀
        if ( (di_as32 == 1) && (start_flag == 1) )
        {
            digitalWrite(SOL61, HIGH);
            if((di_as61 == 1) && (di_as62 == 1)){
                blade_up_check_count++;
            }
            else if((di_as61 == 0) || (di_as62 == 0)){
                blade_up_check_count = 0;
            }
            
            if ( (blade_up_check_count >= 5) && (start_flag == 1) )
            {
                digitalWrite(SOL31, LOW);
                digitalWrite(SOL32, HIGH);
                blade_up_check_count = 0;
                m_step = 101;
            }
        }
        break;
    case 101: // 푸셔 복귀 하면 회전
        if ( (di_as31 == 1) && (start_flag == 1) )
        {
            digitalWrite(SOL51, LOW);
            m_step = 11;
        }
        break;
    case 11: // 회전하면 칼날 다운, 푸시하면서 무탈피
        if ( (di_as52 == 1) && (start_flag == 1) )
        {
            digitalWrite(SOL61, LOW);
            digitalWrite(SOL31, HIGH);
            digitalWrite(SOL32, LOW);
            m_step = 12;
        }
        break;
    case 12: // 푸쉬 끝난 후 반복횟수 비교
        if ( (di_as32 == 1) && (start_flag == 1) )
        {
            repeat_cnt--;

            if (repeat_cnt == 0) {
                m_step = 13;
            }
            else {
                m_step = 16;
            }
        }
        break;
    case 13: // 도킹 해제, 칼날 복귀, 칼날 복귀 후 푸셔 복귀
        if (start_flag == 1) 
        {
            digitalWrite(SOL41, HIGH);
            digitalWrite(SOL61, HIGH);
            if ( (di_as61 == 1) && (di_as62 == 1) && (start_flag == 1) )
            {
                digitalWrite(SOL31, LOW);
                digitalWrite(SOL32, HIGH);
                m_step = 141;
            }
        }
        break;
    case 141: // 푸셔 복귀 후 회전
        if ( (di_as31 == 1) && (start_flag == 1) )
        {
            digitalWrite(SOL51, HIGH);
            m_step = 15;
        }
        break;
    case 15:
        if ( (di_as51 == 1) && (di_as31 == 1) && (start_flag == 1) )
        {
            m_count++;  // 수량을 더하고
            EEPROM.write(NUM_ADDR, m_count);  // 수량을 저장
            delay(100);
            DSP_NUM(m_count);                  // 작업 갯수 디스플레이
            
            if (di_mode == 1)  // 자동 모드
            {
                m_step = 3;
            }
            else if (di_mode == 0) // 수동 모드
            {
                start_flag = 0;
                m_step = 2;
            }           
        }
        break;
    case 16: // 반복루프, 칼날 업, 칼날 업 되면 푸셔 복귀
        if (start_flag == 1) 
        {
            digitalWrite(SOL61, HIGH);
            if ( (di_as61 == 1) && (di_as62 == 1) && (start_flag == 1) )
            {
                digitalWrite(SOL31, LOW);
                digitalWrite(SOL32, HIGH);
                m_step = 18;
            }
        }
        break;
    case 18: // 반복루프, 푸셔 복귀 후 회전
        if ( (di_as31 == 1) && (start_flag == 1) )
        {
            digitalWrite(SOL51, HIGH);
            m_step = 19;
        }
        break;
    case 19: // 반복루프, 회전 후 탈피시작
        if ( (di_as31 == 1) && (di_as51 == 1) && (start_flag == 1) )
        {
            m_step = 8;
        }
        break;    
    default:
        m_step = 0;
    }  // end of switch (m_step) 
    DSP_STEP(m_step);

    delay(LOOP_DELAY_TIME); // 50ms
    Serial.println("--------------------");
    Serial.print("di_start= ");
    Serial.println(di_start);
    Serial.print("di_stop= ");
    Serial.println(di_stop);
    Serial.print("di_reset= ");
    Serial.println(di_reset);
    Serial.print("di_mode= ");
    Serial.println(di_mode);
    Serial.print("di_as11= ");
    Serial.println(di_as11);
    Serial.print("di_as12= ");
    Serial.println(di_as12);
    Serial.print("di_as13= ");
    Serial.println(di_as13);
    Serial.print("di_as21= ");
    Serial.println(di_as21);
    Serial.print("di_as22= ");
    Serial.println(di_as22);
    Serial.print("di_as23= ");
    Serial.println(di_as23);
    Serial.print("di_as24= ");
    Serial.println(di_as24);
    Serial.print("di_as31= ");
    Serial.println(di_as31);
    Serial.print("di_as32= ");
    Serial.println(di_as32);
    Serial.print("di_as41= ");
    Serial.println(di_as41);
    Serial.print("di_as42= ");
    Serial.println(di_as42);
    //Serial.print("di_as43= "); 파손
    // Serial.println(di_as43); 파손
    Serial.print("di_as51= ");
    Serial.println(di_as51);
    Serial.print("di_as52= ");
    Serial.println(di_as52);
    Serial.print("di_as61= ");
    Serial.println(di_as61);
    Serial.print("di_as62= ");
    Serial.println(di_as62);
    Serial.print("di_sel1= ");
    Serial.println(di_sel1);
    Serial.print("di_sel2= ");
    Serial.println(di_sel2);
    Serial.print("di_sel3= ");
    Serial.println(di_sel3);
    Serial.print("m_step= ");
    Serial.println(m_step);
    Serial.print("repeat_cnt= ");
    Serial.println(repeat_cnt);
    Serial.println("--------------------");
    Serial.println("");  
    Serial.println("");
} // end of loop
void clean_LCD(void)
{
    empty();
    sprintf(strMsg, "%c%c", 0x1b, 0x43);
    send_LDC(strMsg);
    sprintf(strMsg, "%c%c", 0x1b, 0x73);
    send_LDC(strMsg);
}

void DSP_STATE(String str_msg)
{
    char msg[8];

    int i = 0;
    for (i=0; i<8; i++)
    {
        msg[i] = 0x00;
    }

    for (i=0; i<8; i++)
    {
        msg[i] = str_msg[i];
    }
    
    empty();
    sprintf(strMsg, "%c%c%c%c", 0x1b, 0x4c, 1, 0);
    send_LDC(strMsg);
    empty();
    sprintf(strMsg, " STATE: %s", msg);
    send_LDC(strMsg);
}

void DSP_ERROR(String str_msg)
{
    char msg[MAX_SIZE];

    int i = 0;
    for (i=0; i<MAX_SIZE; i++)
    {
        msg[i] = 0x00;
    }

    for (i=0; i<MAX_SIZE; i++)
    {
        msg[i] = str_msg[i];
    }
  
    empty();
    sprintf(strMsg, "%c%c%c%c", 0x1b, 0x4c, 1, 1);
    send_LDC(strMsg);
    empty();
    sprintf(strMsg, "%s", msg);
    send_LDC(strMsg);     
}

void DSP_NUM(unsigned long int num)
{
    empty();
    sprintf(strMsg, "%c%c%c%c", 0x1b, 0x4c, 1, 1);
    send_LDC(strMsg);
    empty();
    sprintf(strMsg, "%s:%4d", "NUM", num);
    send_LDC(strMsg);
}

void DSP_STEP(unsigned long int num)
{
    empty();
    sprintf(strMsg, "%c%c%c%c", 0x1b, 0x4c, 1, 2);
    send_LDC(strMsg);
    empty();
    sprintf(strMsg, "%s:%4d", "STEP", num);
    send_LDC(strMsg);
}

void empty()
{
    int i = 0;
    for(i=0; i<MAX_SIZE; i++)
    {
        strMsg[i] = 0x00;
    }
}

void send_LDC(String strVal)
{
    char cTemp[MAX_SIZE];
    int i = 0;

    for(i=0; i<MAX_SIZE; i++)
    {
        cTemp[i] = strVal[i];
    }
    Serial1.print(cTemp);
}

void timerIsr()  //타이머 3
{
    delay_count++;
}
