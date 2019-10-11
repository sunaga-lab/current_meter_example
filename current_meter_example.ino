// CPU Freqency 160MHz

#include <Wire.h>
#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ESP8266WiFiMulti.h>

// コメント外すと波形のプロットができる
//#define PLOT_DATA_PRINT_CH 0
// コメント外すと周期時間が表示される
//#define DURATION_DATA_PRINT_CH 1

// LED点灯パターン
uint32_t LIMPTNS[] = {
  30, 0b1010010110100101, 
  28, 0b0000000000001010,
  26, 0b0000000000001100,
  24, 0b0000000000001111,
  22, 0b0000000011110101,
  20, 0b0000000011110011,
  18, 0b0000000011110000,
  17, 0b0000111110100000,
  15, 0b0000111111000000,
  14, 0b0000111100000000,
  12, 0b1111101000000000,
  10, 0b1111110000000000,
   8, 0b1111000000000000,
   6, 0b1110000000000000,
   5, 0b1100000000000000,
   0, 0b1000000000000000,
};


#define ADS1015_I2C_ADDR 0x48
// AINnの連続読み出しを開始
void ADS1015_start_cont(int n) {
  Wire.beginTransmission(ADS1015_I2C_ADDR);
  Wire.write(0x01);
  Wire.write(0b00000100 | ((n+1) << 4));    // OS=0,MUX=0nn,PGA=010(default),MODE=0(Cont)
  Wire.write(0b11100011);                   // DR=111(3300SPS),00011(No comparator)
  Wire.endTransmission();
  Wire.beginTransmission(ADS1015_I2C_ADDR);
  Wire.write(0x00);
  Wire.endTransmission();
}

// 連続読み出しを停止
void ADS1015_stop_cont() {
  Wire.beginTransmission(ADS1015_I2C_ADDR);
  Wire.write(0x01);
  Wire.write(0b01000101);                   // OS=0,MUX=100,PGA=010(default),MODE=1(single)
  Wire.write(0b11100011);                   // DR=111(3300SPS),00011(No comparator)
  Wire.endTransmission();
}

// 値の取得
int ADS1015_read_cont() {
  Wire.requestFrom(ADS1015_I2C_ADDR, 2);
  int16_t v = Wire.read() << 8;
  v |= Wire.read();
  return v / 16;
}

// サンプル数
const int NUM_SAMPLES = 749; // 160MHzで749サンプル取ると概ね240msになった

int g_led_ptn = 0;
void set_led(uint8_t counter) {
  for(int ledi = 0; ledi < 4; ledi++){
    int pcode = g_led_ptn >> ((3-ledi)*4 + counter) & 1;
    digitalWrite(12 + ledi, pcode);
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // LED出力ピン設定
  pinMode(12, OUTPUT);
  pinMode(13, OUTPUT);
  pinMode(14, OUTPUT);
  pinMode(15, OUTPUT);

  Serial.println("Start");
}

void loop() {
  int sq_sum[4];
#ifdef PLOT_DATA_PRINT_CH
  int16_t dataForPlot[NUM_SAMPLES];
#endif

  for(int ch = 0; ch < 4; ch++){
    sq_sum[ch] = 0;
  }

  for(int ch = 0; ch < 4; ch++){
    ESP.wdtFeed();
    set_led(ch);
    int start_millis = millis();
    ADS1015_start_cont(ch);
    for(int i = 0; i < NUM_SAMPLES; i++){
      delayMicroseconds(5);
      int16_t data = ADS1015_read_cont();
      sq_sum[ch] += data*data * 2;
#ifdef PLOT_DATA_PRINT_CH
      if(ch == PLOT_DATA_PRINT_CH){ dataForPlot[i] = data; }
#endif
    }
    int endmillis = millis() - start_millis;
#ifdef DURATION_DATA_PRINT_CH
    if(ch == DURATION_DATA_PRINT_CH) {
      Serial.printf("  DUR=%dms\n", endmillis);
    }
#endif
  }
#ifdef PLOT_DATA_PRINT_CH
  for(int i = 0; i < NUM_SAMPLES; i++){
    Serial.printf("  0, %d\n", dataForPlot[i]);
  }
#endif
  ADS1015_stop_cont();

  // センサーの値から電流値(A)への変換係数
  const float V_COEFF = (1.0 / 1024.0) * 30.0; 

  float sqavg[3];
  for(int i = 0; i < 3; i++) {
    sqavg[i] = sqrt((float)sq_sum[i] / (float)NUM_SAMPLES) * V_COEFF;
    Serial.printf(" - ch%d: A from Vrms=%.02f \n", i, sqavg[i]);
  }


  // LEDパターンの設定
  int N_LIMPTNS = sizeof LIMPTNS / sizeof LIMPTNS[0];
  g_led_ptn = 0;
  for(int i = 0; i < N_LIMPTNS; i += 2) {
    if(sqavg[0] > (float)LIMPTNS[i])  {
      g_led_ptn = LIMPTNS[i+1];
      break;
    }
  }

}
