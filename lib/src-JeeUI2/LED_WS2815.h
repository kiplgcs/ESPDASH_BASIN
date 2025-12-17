#include <HTTPClient.h> //Библиотека для API GET запроса из интернета о погоде.
#include <ArduinoJson.h>


#include <FastLED.h>
#include "GradientPalettes.h"

#define FASTLED_INTERRUPT_RETRY_COUNT 0

FASTLED_USING_NAMESPACE

#define ARRAY_SIZE(A) (sizeof(A) / sizeof((A)[0]))

#define DATA_PIN      27 // Пин управления "DIN" лентой
#define LED_TYPE      WS2812B //WS2812B // Тип 
#define COLOR_ORDER   GRB // RGB
#define NUM_LEDS      180 //313//144 // Количество светодиодов в ленте
#define BRIG          255// яркость 0 - 255
#define MILLI_AMPS         2000 //Максимально потребляемяая мощьность
#define FRAMES_PER_SECOND  100 //Скорость 
uint8_t speed = 30; //Начальная скорость - для некоторых функций sinelon();
CRGB leds[NUM_LEDS];


const uint8_t brightnessCount = 5;
uint8_t brightnessMap[brightnessCount] = { 16, 32, 64, 128, 255 };
uint8_t brightnessIndex = 5;

uint8_t secondsPerPalette = 30; //Время задержки на одном режиме

// ОХЛАЖДЕНИЕ: Насколько воздух охлаждается при подъеме?
// Меньше охлаждения = более высокое пламя. Больше охлаждения = более короткое пламя.
// По умолчанию 50, рекомендуемый диапазон 20-100
uint8_t cooling = 50;

// SPARKING: What chance (out of 255) is there that a new spark will be lit?
// Higher chance = more roaring fire.  Lower chance = more flickery fire.
// По умолчанию 120, рекомендуемый диапазон 50-200.
uint8_t sparking = 100;
extern const TProgmemRGBGradientPalettePtr gGradientPalettes[]; //градиентные палитры

uint8_t gCurrentPaletteNumber = 0; //Текущий номер палитры

CRGBPalette16 gCurrentPalette( CRGB::Black); //Текущая палитра
CRGBPalette16 gTargetPalette( gGradientPalettes[0] ); //Целевая палитра

 CRGBPalette16 IceColors_p = CRGBPalette16(CRGB::Black, CRGB::Blue, CRGB::Aqua, CRGB::White);

unsigned long autoPlayTimeout = 10; //тайм-аут автоматического воспроизведения
uint8_t currentPaletteIndex = 1; //текущий индекс палитры
uint8_t gHue = 0; //  вращающийся «основной цвет», используемый во многих узорах
 CRGB solidColor = CRGB::Red; //= CRGB::Blue;

void dimAll(byte value)
{
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i].nscale8(value);
  }
}

typedef void (*Pattern)();
typedef Pattern PatternList[];
typedef struct {
  Pattern pattern;
  String name;
} PatternAndName;
typedef PatternAndName PatternAndNameList[];

void addGlitter( uint8_t chanceOfGlitter)
{
  if ( random8() < chanceOfGlitter) {
    leds[ random16(NUM_LEDS) ] += CRGB::White;
  }
}


#include "Twinkles.h"
#include "TwinkleFOX.h"

void rainbow();
void rainbowWithGlitter();
void rainbowSolid();
void confetti();
void sinelon();
void bpm();
void juggle();
void fire();
void water();
void showSolidColor();
void pride();
void colorWaves();
void heatMap(CRGBPalette16 palette, bool up);
void colorwaves( CRGB* ledarray, uint16_t numleds, CRGBPalette16& palette);
void adjustPattern(bool up);
//void loadSettings();
byte power;
byte brightness;



PatternAndNameList patterns = {

  { pride,                  "Pride" },
  { colorWaves,             "Color Waves" },

  // // twinkle patterns
  { rainbowTwinkles,        "Rainbow Twinkles" },
  { snowTwinkles,           "Snow Twinkles" },
  { cloudTwinkles,          "Cloud Twinkles" },
  { incandescentTwinkles,   "Incandescent Twinkles" },

   
   { retroC9Twinkles,        "Retro C9 Twinkles" },
   { redWhiteTwinkles,       "Red & White Twinkles" },
   { blueWhiteTwinkles,      "Blue & White Twinkles" },
  { redGreenWhiteTwinkles,  "Red, Green & White Twinkles" },
  { fairyLightTwinkles,     "Fairy Light Twinkles" },
  { snow2Twinkles,          "Snow 2 Twinkles" },
  { hollyTwinkles,          "Holly Twinkles" },
  { iceTwinkles,            "Ice Twinkles" },
  { partyTwinkles,          "Party Twinkles" },
  { forestTwinkles,         "Forest Twinkles" },
  { lavaTwinkles,           "Lava Twinkles" },
  { fireTwinkles,           "Fire Twinkles" },
   { cloud2Twinkles,         "Cloud 2 Twinkles" },
   { oceanTwinkles,          "Ocean Twinkles" },

  { rainbow,                "Rainbow" },
  { rainbowWithGlitter,     "Rainbow With Glitter" },
  { rainbowSolid,           "Solid Rainbow" },
  { confetti,               "Confetti" },
  //{ sinelon,                "Sinelon" },
  //{ juggle,                 "Juggle" },
  { bpm,                    "Beat" }
  //{ fire,                   "Fire" }, //не работает почему-то
  //{ water,                  "Water" } //не работает почему-то
 
  // { showSolidColor,         "Solid Color" } // синий цвет - хрень какая-то
};


const uint8_t patternCount = ARRAY_SIZE(patterns);

typedef struct {
  CRGBPalette16 palette;
  String name;
} PaletteAndName;
typedef PaletteAndName PaletteAndNameList[];

const CRGBPalette16 palettes[] = {
  RainbowColors_p,
  RainbowStripeColors_p,
  CloudColors_p,
  LavaColors_p,
  OceanColors_p,
  ForestColors_p,
  PartyColors_p,
  HeatColors_p
};

const uint8_t paletteCount = ARRAY_SIZE(palettes);

const String paletteNames[paletteCount] = {
  "Rainbow",
  "Rainbow Stripe",
  "Cloud",
  "Lava",
  "Ocean",
  "Forest",
  "Party",
  "Heat",
};

#include "effects.h"

/**********************************************************/






void setup_WS2815() {


Serial.setDebugOutput(true);  // Включаем вывод отладочной информации в Serial (последовательный порт)

FastLED.addLeds<LED_TYPE, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
// Добавляем светодиодную ленту с помощью библиотеки FastLED.
// Выбираем тип светодиодов (LED_TYPE), пин данных (DATA_PIN) и порядок цветов (COLOR_ORDER).
// В данном случае, комментарий указывает на использование WS2812 (Neopixel).
// Если нужно использовать APA102 (Dotstar), можно закомментировать предыдущую строку и раскомментировать следующую.

FastLED.setDither(false);  // Отключаем диффузию цветов (постепенное изменение цветов)

FastLED.setCorrection(TypicalLEDStrip);  // Коррекция цветопередачи для типичной светодиодной ленты

FastLED.setBrightness(BRIG);  // Устанавливаем яркость светодиодов (BRIG)

FastLED.setMaxPowerInVoltsAndMilliamps(5, MILLI_AMPS);
// Устанавливаем максимальное потребление энергии для светодиодной ленты
// в вольтах (5V) и миллиамперах (MILLI_AMPS).

fill_solid(leds, NUM_LEDS, CRGB::Black);
// Заполняем все светодиоды черным цветом (выключенными).

FastLED.show();
// Отправляем установленные значения цветов на светодиодную ленту для отображения.

autoPlayTimeout = millis() + (autoplayDuration * 1000);
// Устанавливаем таймер для автоматического проигрывания (autoplay).
// Время автоматического проигрывания определяется переменной autoplayDuration в секундах.
// Таймер устанавливается в текущее время (millis()) плюс продолжительность автовоспроизведения,
// умноженную на 1000 для преобразования в миллисекунды.


  //patternCount1 = patternCount-1; // определяем колличество режимов работы LED (которые ранее определены в массиве "patterns ") для вывода на WEB морду

}




void loop_WS2815() {


      if (Pow_WS2815 && !ColorRGB){


      LEDS.setBrightness(new_bright);


      EVERY_N_SECONDS( secondsPerPalette ) {
      gCurrentPaletteNumber = addmod8( gCurrentPaletteNumber, 1, gGradientPaletteCount);
      gTargetPalette = gGradientPalettes[ gCurrentPaletteNumber ];
      }

      EVERY_N_MILLISECONDS(40) {
      nblendPaletteTowardPalette( gCurrentPalette, gTargetPalette, 8);
      gHue++;  // slowly cycle the "base color" through the rainbow - медленно чередуйте «основной цвет» через радугу
      }

      if (autoplay == true && (millis() > autoPlayTimeout)) {          //Таймер для autoplay - когда смена режимов через заданное время autoplayDuration1
      adjustPattern(true);
      autoPlayTimeout = millis() + (autoplayDuration * 1000);
      }

      patterns[currentPatternIndex].pattern();  //Передаем установленный режим 

      //FastLED.show();                           //передаем переопределенные значения -запускаем

     } if (Pow_WS2815 && ColorRGB) {

        LEDS.setBrightness(new_bright);
        r = number >> 16;
        g = number >> 8 & 0xFF;
        b = number & 0xFF;

        for (int i = 0; i < NUM_LEDS; i++){
        leds[i].setRGB(r, g, b); // Фиолетовый
        } // передаем всем светодиодам цвет.

        //FastLED.show();// Передаем цвета ленте.  

      } else if (!Pow_WS2815) {LEDS.setBrightness(0); /*FastLED.show();*/ /*digitalWrite(rele1, LOW); start_RGB=1;*/} // задаем яркость 0% и оключаем ленту
      

      
      FastLED.show(); // Передаем цвета ленте. 





      // if(Pow_WS28151 != Pow_WS2815) {Pow_WS28151=Pow_WS2815;
      // jee.var("Pow_WS2815", Pow_WS2815 ? "true" : "false");
      //  }

      // if(Saved_ColorRGB != ColorRGB) {Saved_ColorRGB = ColorRGB;
      // jee.var("ColorRGB", ColorRGB ? "true" : "false"); //Задавать цвет в ручную
      // }

      if(number1 != number) { number1= number;
      jee.var("ColorLED", String("#" + String(number, HEX))); //Выбор цвета
      // String hexString = String("#" + String(number, HEX));  // Преобразование long long в строку в шестнадцатеричной системе с добавлением символа "#"
      // hexString.toLowerCase();  // Преобразование строки к строчным буквам (например, F20D0D -> f20d0d)
      // jee.var("ColorLED", hexString);
      }

      if(new_bright1 != new_bright) {new_bright1= new_bright;
      jee.var("new_bright", String(new_bright)); //Яркость 
      }

      if(autoplay1 != autoplay) {autoplay1= autoplay;
      jee.var("autoplay", autoplay ? "true" : "false"); //Автоплей
      }

      if(autoplayDuration1 != autoplayDuration) {autoplayDuration1= autoplayDuration;
      jee.var("autoplayDuration", String(autoplayDuration)); //Через сколько сек. менять режим
      }

      if(RANDOMNO1 != RANDOMNO) {RANDOMNO1= RANDOMNO;
      // jee.var("RANDOMNO", String(RANDOMNO)); //Рандомный режим
      jee.var("RANDOMNO", RANDOMNO ? "true" : "false");
      }




}





void palettetest( CRGB* ledarray, uint16_t numleds, const CRGBPalette16& gCurrentPalette)
{
  static uint8_t startindex = 0;
  startindex--;
  fill_palette( ledarray, numleds, startindex, (256 / NUM_LEDS) + 1, gCurrentPalette, 255, LINEARBLEND);
}



void adjustPattern(bool up) {
  // Определение числа паттернов
  const int patternCount = 25;
  // Массив значений Index
bool indexArray[] = {
    Index0, Index1, Index2, Index3, Index4, Index5, Index6, Index7, Index8, Index9,
    Index10, Index11, Index12, Index13, Index14, Index15, Index16, Index17, Index18, Index19,
    Index20, Index21, Index22, Index23, Index24
  };


if (RANDOMNO) { // Активация рандомного режима
int randomIndex = random(0, 25);  // Генерируем случайный индекс от 0 до 24
// Массив указателей на переменные Index
bool* indexArray1[] = {&Index0, &Index1, &Index2, &Index3, &Index4, &Index5, &Index6, &Index7, &Index8, &Index9,
                      &Index10, &Index11, &Index12, &Index13, &Index14, &Index15, &Index16, &Index17, &Index18, &Index19,
                      &Index20, &Index21, &Index22, &Index23, &Index24};
*indexArray1[randomIndex] = !(*indexArray1[randomIndex]);  // Присваиваем противоположное значение выбранному элементу массива

  //new_bright = random(20, 255);

}


  // Увеличение или уменьшение currentPatternIndex
  if (up)
    currentPatternIndex++;
  else
    currentPatternIndex--;

  // Проверка условий и обновление currentPatternIndex
  if (currentPatternIndex < 0)
    currentPatternIndex = patternCount - 1;
  if (currentPatternIndex >= patternCount)
    currentPatternIndex = 0;

  // Обработка индексов Index
  while (!indexArray[currentPatternIndex]) {
    if (up)
      currentPatternIndex++;
    else
      currentPatternIndex--;

    if (currentPatternIndex < 0)
      currentPatternIndex = patternCount - 1;
    if (currentPatternIndex >= patternCount)
      currentPatternIndex = 0;
  }
}
