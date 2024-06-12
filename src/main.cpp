
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <SoftwareSerial.h>
#include <DFRobotDFPlayerMini.h>

#include "symbols.h"

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

#define BUTTON_PIN 4
#define RX_PIN 3
#define TX_PIN 2

#define DUBLAJ_PIN 7

#define CARD_RED 0
#define CARD_BLACK 1

#define TO_RADIANS(degrees) ((degrees) * M_PI / 180.0)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
SoftwareSerial software_serial(RX_PIN, TX_PIN);
DFRobotDFPlayerMini player;

int frame = 0;
int state = 0;
int y = 0;
float offset = 0;
int displayed_symbols[15];
int speed = 6;
int balance = 0;
int last_win = 0;
int card = 0;

void generate_symbols()
{
  for (int i = 3; i < 13; i++)
  {
    displayed_symbols[i] = random(8) % epd_bitmap_allArray_LEN;
  }

  if (random(4) == 0)
  {
    displayed_symbols[13] = displayed_symbols[12];
    displayed_symbols[14] = displayed_symbols[12];
  }
  else
  {
    displayed_symbols[13] = random(8) % epd_bitmap_allArray_LEN;
    displayed_symbols[14] = random(8) % epd_bitmap_allArray_LEN;
  }
}

uint8_t button_pressed()
{
  return !(PIND & (1 << PD4));
}

uint8_t dublaj_pressed()
{
  return !(PIND & (1 << PD7));
}

void initADC() {
    // Set the reference voltage to AVcc with external capacitor at AREF pin
    ADMUX |= (1 << REFS0);
    ADMUX &= ~(1 << REFS1);

    // Set the ADC prescaler to 128 for 125kHz ADC clock frequency (16MHz / 128)
    ADCSRA |= (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);

    // Enable the ADC
    ADCSRA |= (1 << ADEN);
}

uint16_t readADC(uint8_t channel) {
    // Select the corresponding channel 0~7
    ADMUX = (ADMUX & 0xF0) | (channel & 0x0F);

    // Start the conversion
    ADCSRA |= (1 << ADSC);

    // Wait for the conversion to complete (ADSC becomes '0' again)
    while (ADCSRA & (1 << ADSC));

    // Read the ADC value
    return ADC;
}


void setup() {
  Serial.begin(9600);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  DDRD &= ~(1 << PD4);
  PORTD |= (1 << PD4);
  DDRD &= ~(1 << PD7);
  PORTD |= (1 << PD7);

  DDRD |= (1 << PD5);
  DDRD |= (1 << PD6);

  initADC();

  software_serial.begin(9600);

  if (player.begin(software_serial))
  {
    Serial.println(F("DFPlayer Mini online."));
    player.volume(30);
  }
  else
  {
    Serial.println(F("DFPlayer Mini not found."));
  }

  randomSeed(readADC(5));
  generate_symbols();
  for (int i = 0; i < 3; i++) {
    displayed_symbols[i] = random(8) % epd_bitmap_allArray_LEN;
  }
}

void loop() {
  display.clearDisplay();
  switch (state)
  {
    case 0:
      if (button_pressed())
      {
        state = 1;
        balance -= 100;
        player.loop(3);
        delay(100);
      }
      break;
    
    case 1:
      frame++;
      if (button_pressed())
      {
        frame = 180.f / speed + 1;
        delay(100);
      }
      offset = 168.f * ((sin(speed * TO_RADIANS(frame) - (M_PI / 2)) + 1.0f) / 2);
      if (frame > 180.f / speed)
      {
        state = 2;
      }
      break;

    case 2:
      player.stop();
      if (displayed_symbols[12] == displayed_symbols[13] && displayed_symbols[13] == displayed_symbols[14])
        {
          last_win = multipliers[displayed_symbols[12]] * 100;
        }
        else
        {
          last_win = 0;
        }
        if (last_win != 0)
        {
          state = 3;
          break;
        }
        state = 4;
      break;

    case 3:
      if (button_pressed())
      {
        state = 4;
        delay(100);
        break;
      }
      
      if (dublaj_pressed())
      {
        state = 5;
        card = random(2);
        delay(100);
        break;
      }
      break;

    case 4:
      PORTD &= ~(1 << PD5);
      PORTD &= ~(1 << PD6);
      if (last_win > 2000) {
        player.play(2);
      }
      balance += last_win;
      state = 0;
      frame = 0;
      offset = 0;
      for (int i = 0; i < 3; i++) {
        displayed_symbols[i] = displayed_symbols[i + 12];
      }
      generate_symbols();
      break;

      case 5:
        frame++;
        if (button_pressed())
        {
          if (card == CARD_BLACK) {
            last_win *= 2;
            player.play(1);
          }
          else {
            last_win = 0;
            player.play(4);
          }
          state = 4;
          delay(100);
          break;
        }
        if (dublaj_pressed())
        {
          if (card == CARD_RED) {
            last_win *= 2;
            player.play(1);
          }
          else {
            last_win = 0;
            player.play(4);
          }
          state = 4;
          delay(100);
          break;
        }
        break;
  }

  switch (state)
  {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
      for (int i = 0; i < 15; i++)
      {
        int Y = (y + offset) + 18 - (42 * (i / 3));
        if (Y < -22 || Y > 64) continue;
        display.drawBitmap(2 + 42 * (i % 3), Y, epd_bitmap_allArray[displayed_symbols[i]], 40, 40, WHITE);
      }
      break;
    case 5:
      display.setCursor(20,20);
      display.setTextSize(3);
      if (frame & 2)
      {
        PORTD &= ~(1 << PD5);
        PORTD |= (1 << PD6);
        display.print(F("RED"));
      }
      else
      {
        PORTD |= (1 << PD5);
        PORTD &= ~(1 << PD6);
        display.print(F("BLACK"));
      }
      display.setTextSize(1);
      display.setCursor(39, 48);
      display.print(F("miza: "));
      display.println((float)last_win/100);
      
      break;
  }
  
    
  for (int i = 0; i < 15; i++)
    display.drawLine(0, i, 128, i, BLACK);
  display.drawLine(0, 15, 128, 15, WHITE);
  
  display.setCursor(0,0);

  display.print(F("balance: "));
  display.println((float)balance/100);

  display.print(F("last win: "));
  display.println((float)last_win/100);

  display.display();
  
}