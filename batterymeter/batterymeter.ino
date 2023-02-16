// ******* TEST settings ********
//#define TEST_BLINKY
//#define TEST_TYPES
//#define TEST_OLED

//#define DEBUG_MSG

// The serial interface. SCPI or more debug/dev-oriented
// ifdef COMMANDS_DEBUG: debug/dev commands enabled
// ifndef COMMANDS_DEBUG: SCPI commands enabled
//#define COMMANDS_DEBUG

// About PROGMEM: the compiler I use does not need PROGMEM, so I removed it on most places where you'd normally expect it.
// const declarations go to flash storage, but do not need memory mapping.

// TODO: look at TODOs below
// TODO allow key press interrupt the readings
// TODO show busy

// The different HW models
#define HW_V1_0 0
#define HW_V1_1 1

// what HW version this is for
#define HW_VERSION_ID HW_V1_1

#if HW_VERSION_ID == HW_V1_0
#define BODGE_REF_SW
#define BOARDHW_VERSION "1.0 with bodge"
#endif

#if HW_VERSION_ID == HW_V1_1
#define HW_VERSION "1.1"
#endif

#ifndef HW_VERSION
#error "You need to specify a HW version"
#endif

#ifdef TEST_BLINKY
#define NO_MAIN
#define NO_DISPLAY
#endif

#ifdef TEST_TYPES
#define NO_MAIN
#endif

#ifdef TEST_OLED
#define NO_MAIN
#endif

#define SW_VERSION "0.9d"

const char *AppName = "Batterymeter " SW_VERSION;

#include <Event.h>
#include <EEPROM.h>
#include <SparkFun_PCA9536_Arduino_Library.h>
#ifndef COMMANDS_DEBUG

// local version of SCPI lib, as I want to set the defines (and got some additions)
#define SCPI_MAX_TOKENS 30
#define SCPI_MAX_COMMANDS 25
#include "Vrekrer_scpi_parser_local.h"
#endif

#pragma region Arduino IDE PROGRAMMER settings
// ************************************************************************************************
// ******* Arduino IDE PROGRAMMER settings ********
// ************************************************************************************************

// This must be compiled from Arduino IDE for now, as
// * https://registry.platformio.org/platforms/platformio/atmelmegaavr is not supporting the 3226 yet (will be soon)
// * serialupdi in avrdude is not mainline yet (will be soon)

// Config:
// Board: MegaTinyCore Attiny 3226/....
// Chip: ATtiny3226
// Clock: 10MHz internal (because of 3.3V)
// Millis: TCB1 (I need TCA)
// printf(): full (haven't found a way to check that on compile time)
// Wire: Master or Slave
// Programmer: SerialUPDI SLOW

// !!!!!!! During programming, force the power on with the jumper, as the device otherwise switches off during programming !!!!!!

#ifndef ARDUINO_AVR_ATtiny3226
#error "Wrong CPU:  must be ATtiny3226"
#endif

#ifndef MILLIS_USE_TIMERB1
#error "I Need Timer B1 for millis"
#endif

#if F_CPU != 10000000L
#error "Wrong clock speed"
#endif
#pragma endregion

#pragma region Pinout and other hardware

// Types:
// float, double, int32_t, long = 4 bytes
// int64_t = 8 bytes
// int = 2 bytes

// ************************************************************************************************
// ******* Pinout ********
// ************************************************************************************************

// Pins 3226
//        func               | alt                         | debug use
//--------------------------+-----------------------------+----------------------
// 1  VDD                    |                             |
// 2  PA4 SIGNAL_OUT         | AIN4  !SS     WO4           |
// 3  PA5 free               | PA5 AIN5  VREFA   WO5 0.WO  | PA5 debug AC sampler
// 4  PA6 OUT_SW             | PA6 AIN6                    |
// 5  PA7 CPU_PWR_ON         | PA7 AIN7                    |
// 6  PB5 SHORT_SW           | PB5 AIN8          WO2'      |
// 7  PB4 ADC_IN_SW          | PB4 AIN9  !RESET' WO1'      |
// 8  RxD                    | PB3               WO0'      |
// 9  TxD                    | PB2               WO2       |
// 10 SDA                    | PB1 AIN10         WO1       |
// 11 SCL                    | PB0 AIN11         WO0       |
// 12 PC0 REF_SW_A           | PC0 AIN10 SCK'              |
// 13 PC1 REF_SW_B           | PC1 AIN13 MISO'             |
// 14 PC2 free               | PC2 AIN14 MOSI'             | EVOUTC debug AC sampler
// 15 PC3 SHORT_DISABLE      | PC3 AIN15 !SS'              |
// 16 UPDI                   | PA0 !RESET                  |
// 17 AIN1 ADC_INP           | PA1 MOSI                    |
// 18 AIN2 ADC_INN           | PA2 MISO                    |
// 19 AIN3 ADC_DC            | PA3 AIN3  SCK WO3  1.WO     |
// 20 GND                    |

// AIN[15:8] can not be used as negative ADC input for differential measurements.

// Signal levels:
// ADC_IN_SW : 1 = REF to ADC_INx, 0 = SENSE to ADC_INx, 1 = BATT to ADC_DC, 0 = TEST to ADC_DC
// SHORT_SW : 0 = REF, 1 = SENSE
// SHORT_DISABLE : 0 = short, 1 = no short
// BTN1 : 0 = pressed
// BTN2 : 0 = pressed
// REF_SW_A : see below
// REF_SW_B : see below
// OUT_SW : 1 = SIGNAL_OUT = DCIS, 0 = SIGNAL = AC
// CPU_PWR_ON : 1 = ON, 0 = OFF

// REF_SW_A = EN_1mA
// REF_SW_B = EN_10mA
// --- AND(REF_SW_A,REF_SW_B) = EN_100mA

//        SW_A  SW_B EN_1mA EN_10mA EN_100mA
// 100uA  0     0    0      0       0
//   1mA  1     0    1      0       0
//  10mA  0     1    x      1       0
// 100mA  1     1    x      x       1
//

#define SIGNAL_OUT PIN_PA4
//#define AC_SIGNAL_DEBUG_TRIGGERPIN_ON_PC2
//#define AC_SIGNAL_DEBUG_WAITPIN_ON_PA5

#define OUT_SW PIN_PA6
#define CPU_PWR_ON PIN_PA7
#define SHORT_SW PIN_PB5
#define ADC_IN_SW PIN_PB4

#define REF_SW_A PIN_PC0
#define REF_SW_B PIN_PC1

#ifdef BODGE_REF_SW
#ifdef AC_SIGNAL_DEBUG_TRIGGERPIN_ON_PC2
#error "cannot bodge and debug"
#endif
#ifdef AC_SIGNAL_DEBUG_WAITPIN_ON_PA5
#error "cannot bodge and debug"
#endif
#endif

#define SHORT_DISABLE PIN_PC3

#define ADC_INP PIN_PA1
#define ADC_INN PIN_PA2
#define ADC_DC PIN_PA3

// what type of display?
#define OLED_128x32
//#define OLED_96x16

#pragma endregion

#pragma region Global settings
// ************************************************************************************************
// ******* Global settings ********
// ************************************************************************************************

// MIN must be 0, as I cannot print negative values now.
// Volts
#define VOLT_MIN 0
#define VOLT_MAX 25

// MIN must be 0, as I cannot print negative values now.
// Ohms
#define ESR_MIN 0
#define ESR_MAX 20

// The overflow is determined by 2 factors:
// * ADC Overflow, normally only when the input/test signal clips. The reference signal should never clip.
// * a suitable full scale display value. This depends on the ratio between Ref and Test, but also the scale.
// Since the 4 scales are in decades, underflow is simply the overflow limit / 10
// In other words:
// Reference signal should never clip, but should be fairly high to allow for more resolution
// Input/test signal clip should only happen above the full scale display value.

// 12 bit ADC, but count on 11 bits. That is 2048. So about 3.5 digits
// Below, the highest range full scale value is to be given. Make it a nice round (integer) number.
// Adapt this to the amplification and the ADC reference voltage.
// Make sure you have some margin before:
//  * ADC clipping
//  * rail voltage clipping
#define ESR_FULLSCALE 100

// Some strings that appear at different spots.
const char strERROR[] = "ERROR";
const char strTimeOut[] = "timeout";
const char strOverrun[] = "overrun";
const char strLowV[] = "low v";
const char strOverrange[] = "overrange";
const char strShortProblem[] = "short problem";
const char strOverload[] = "overload";
const char strOver[] = "over";
const char strAUTO[] = "AUTO";
const char strON[] = "ON";
const char strOFF[] = "OFF";

#pragma endregion

#pragma region Settings
// ************************************************************************************************
// Settings
// ************************************************************************************************

#define SETTINGS_VERSION 2
struct Settings {
    // unint16_t: 0..65536
    uint8_t version;
    uint16_t iInternalVoltageMilliVoltAt1V;  // millivolts for 1 V theoretical ADC input
    uint16_t iInputVoltageMilliVoltAt1V;     // millivolts for 1 V theoretical ADC input
    int16_t  iInputVoltageOffset;            // ADC volts input offset
    uint16_t iRange0_1mOhms;                 // reference resistor 100uA range, times 1mOhm
    uint16_t iRange1_100uOhms;               // reference resistor 1mA range, times 100uOhm
    uint16_t iRange2_10uOhms;                // reference resistor 10mA range, times 10uOhm
    uint16_t iRange3_1uOhms;                 // reference resistor 100mA range, times 1uOhm

    uint16_t iT1_usecs;                      // DCIS T1, usecs
#define DCIS_T1_usecs_MIN 100
#define DCIS_T1_usecs_MAX 500
#define DCIS_T1_usecs_DEFAULT 250
    uint32_t iT2_usecs;                      // DCIS T2, usecs
#define DCIS_T2_usecs_MIN 100
#define DCIS_T2_usecs_MAX 50000L
#define DCIS_T2_usecs_DEFAULT 10000
    uint16_t iTPause_usecs;                  // DCIS T1-T2 pause, usecs
#define DCIS_TP_usecs_MIN 5000
#define DCIS_TP_usecs_MAX 10000
#define DCIS_TP_usecs_DEFAULT 5000

    uint8_t  iPLF_50Hz;                      // True if power line Frequency = 50Hz

    uint16_t ispare;

    uint16_t iInternalVoltageMilliVoltMin;   // min level in mV for the battery symbol
    uint16_t iInternalVoltageMilliVoltMax;   // max level in mV for the battery symbol
    uint16_t check;                          // CRC16 or some other checksum. 
}
  mySettings;


uint16_t settingsCalcCheck(void) {
    #define SETTINGS_CHECK 0x5A5A

    // TODO calculate CRC16
    return SETTINGS_CHECK;
}

/**
 * @brief Write the settings to EEPROM.
 * To be called after changing the settings.
 */
void writeSettings(void) {
    mySettings.check = settingsCalcCheck();
    EEPROM.put(0, mySettings);
}

// Forward decl
void dcisValidateSettings(void);

/**
 * @brief Read the settings from EEPROM.
 * To be called once at startup.
 */
void readSettings(void) {
    EEPROM.get(0, mySettings);

    if ((mySettings.version != SETTINGS_VERSION) || (mySettings.check != settingsCalcCheck())) {
        // init settings
        mySettings.version = SETTINGS_VERSION;
        
        // 1.00
        // 10.03
        mySettings.iInternalVoltageMilliVoltAt1V = 1000;
        mySettings.iInputVoltageMilliVoltAt1V = 10047;
        mySettings.iInputVoltageOffset = 60;

        // 45	47,018
        // 5	4,9771
        // 0,5	0,4703
        // 0,05	0,050
        mySettings.iRange0_1mOhms = 47018;
        mySettings.iRange1_100uOhms = 49771;
        mySettings.iRange2_10uOhms = 47030;
        mySettings.iRange3_1uOhms = 50000;

        // DCIS timings, usecs
        mySettings.iT1_usecs = DCIS_T1_usecs_DEFAULT;
        mySettings.iT2_usecs = DCIS_T2_usecs_DEFAULT;
        mySettings.iTPause_usecs = DCIS_TP_usecs_DEFAULT;

        mySettings.iPLF_50Hz = 1;
        dcisValidateSettings();

        // Alkaline: 1100 - 1500
        // NiMH: 0900-1300
        // but do not go below 800, as the MCP1642 does not go below that
        mySettings.iInternalVoltageMilliVoltMin = 900;
        mySettings.iInternalVoltageMilliVoltMax = 1500;

        writeSettings();
    }
}

/**
 * @brief Translate a voltage to a battery percentage, based on the used battery type
 *
 * @param v Internal battery voltage
 * @return int the percentage
 */
int settingsTranslateBattVoltageToPercentage(double v) {
    int battLowmV = mySettings.iInternalVoltageMilliVoltMin;
    int battHighmV = mySettings.iInternalVoltageMilliVoltMax;

    // no rounding needed, this is imprecise enough
    int mv = v * 1000.0;
    if (mv < battLowmV)
        mv = battLowmV;
    if (mv > battHighmV)
        mv = battHighmV;

    // Consider it linear. Good enough.
    return map(mv, battLowmV, battHighmV, 0, 100);
}

bool settingsShow4Digits(void) {
    // TODO: get requested precision for impedance from settings. 3 or 4 digits
    return true;
}

#pragma endregion

#pragma region IO Control
// ************************************************************************************************
// IO Control
// ************************************************************************************************

// CPU_PWR_ON : 1 = ON, 0 = OFF
inline void setPowerOn(void) { digitalWrite(CPU_PWR_ON, HIGH); }
inline void setPowerOff(void) { digitalWrite(CPU_PWR_ON, LOW); }

// OUT_SW : 1 = SIGNAL_OUT = DCIS, 0 = SIGNAL = AC
inline void setTestModeDCIS(void) { digitalWrite(OUT_SW, HIGH); }
inline void setTestModeAC(void) { digitalWrite(OUT_SW, LOW); }

// Forward decl
void setup_ADC(bool);

// ADC_IN_SW : 1 = REF to ADC_INx, 0 = SENSE to ADC_INx, 1 = BATT to ADC_DC, 0 = TEST to ADC_DC
enum ADCin : uint8_t { adcin_input = 0,
                       adcin_ref = 1,
                       adcin_batt = 1 };
/**
 * @brief Set the ADC input mux.
 * To be used for diff input (AC and DCIS) and single input (DC) measurements.
 * @param input the ADC input to be used. See enum ADCin
 */
inline void setADCInMode(uint8_t input) { digitalWrite(ADC_IN_SW, input); }
// (Arduino IDE does not allow enums as function parameters....)

// SHORT_SW : 0 = REF, 1 = SENSE
// SHORT_DISABLE : 0 = short, 1 = no short

/**
 * @brief Read the scale to be applied on single ended readings (DC)
 *
 * @param input The item to measure. See enum ADCin
 * @param pOffset the offset to be applied to the raw value.
 * @return double the scale factor
 */
double settingsGetDCScale(uint8_t input, int *pOffset) {
    // get the scales from EEPROM
    if (input == adcin_input) {
        if (pOffset)
            *pOffset = mySettings.iInputVoltageOffset;
        return mySettings.iInputVoltageMilliVoltAt1V / 1000.0;
    } else {
        if (pOffset)
            *pOffset = 0;
        return mySettings.iInternalVoltageMilliVoltAt1V / 1000.0;
    }
}

/**
 * @brief Force a short on the Ref input
 */
inline void setShortOnRef(void) {
    digitalWrite(SHORT_SW, LOW);
    digitalWrite(SHORT_DISABLE, LOW);
}

/**
 * @brief Force a short on the Sense input
 */
inline void setShortOnInput(void) {
    digitalWrite(SHORT_SW, HIGH);
    digitalWrite(SHORT_DISABLE, LOW);
}

inline void setShortOff(void) {
    digitalWrite(SHORT_DISABLE, HIGH);
}

struct adcShortResults {
    int duration;
    int32_t rawp;
    int32_t rawn;
};

#define SHORT_MAX_DURATION_MS 1000
#define SHORT_MIN_DURATION_MS 25

// for shorting, I measure 2 signals: INP and INN
// I accept signals that are maximum SHORT_MAX_DIFFERENCE apart, 
// and where the average of the 2 is in the band (GNDA_ON_12BIT +/- SHORT_MAX_OFFSET)
// Signals should be around VCC/2, on a 2.048V 12 bit scale, so (3.3 / 2) * (4096/2.048) = 3300
#define SHORT_MAX_DIFFERENCE 200
#define SHORT_MAX_OFFSET 300
#define GNDA_ON_12BIT 3300

#define SHORT_ULIMIT GNDA_ON_12BIT + SHORT_MAX_OFFSET + SHORT_MAX_DIFFERENCE
#define SHORT_LLIMIT GNDA_ON_12BIT - SHORT_MAX_OFFSET - SHORT_MAX_DIFFERENCE

/**
 * @brief Force a short on a specified input, up to a max duration
 *
 * @param bInput true for the Test input
 * @param bLoad true for load at 50%
 * @param pResults see struct adcShortResults
 * @return approximation of the duration. -1 if short was impossible in the maximum time
 */
int setShortWithDuration(bool bInput, bool bLoad, struct adcShortResults *pResults) {
    int32_t rawp = 0;
    int32_t rawn = 0;
    int counterOK = 0;

    if (pResults) {
        pResults->duration = 0;
        pResults->rawp = 0;
        pResults->rawn = 0;
    }

    // setup
    if (bInput)
        setShortOnInput();
    else
        setShortOnRef();
    digitalWriteFast(SIGNAL_OUT, LOW);
    if (bLoad)
        setTestModeAC();  // force 50% current
    else
        setTestModeDCIS();  // force 0% current
    setup_ADC(true);
    if (bInput)
        setADCInMode(adcin_input);
    else
        setADCInMode(adcin_ref);

    // throw first measurement away
    rawp = analogReadEnh(ADC_INP, ADC_NATIVE_RESOLUTION, 0);  // is about 20us

    for (int i = 0; i < SHORT_MAX_DURATION_MS; i++) {
        rawp = analogReadEnh(ADC_INP, ADC_NATIVE_RESOLUTION, 0);
        rawn = analogReadEnh(ADC_INN, ADC_NATIVE_RESOLUTION, 0);
        if ((rawp > SHORT_ULIMIT) || (rawp < SHORT_LLIMIT) ||
            (rawn > SHORT_ULIMIT) || (rawn < SHORT_LLIMIT) || 
            (abs(rawn-rawp) > SHORT_MAX_DIFFERENCE))
            counterOK = 0;
        else
            counterOK++;
        if (counterOK > SHORT_MIN_DURATION_MS) {
            setShortOff();
            // Serial.printf(" P: %lu, N: %lu",rawp,rawn);
            if (pResults) {
                pResults->duration = i;
                pResults->rawp = rawp;
                pResults->rawn = rawn;
            }
            return i;
        }
        delayMicroseconds(960);
    }

    setShortOff();
    if (pResults) {
        pResults->duration = -1;
        pResults->rawp = rawp;
        pResults->rawn = rawn;
    }
    return -1;
}

/**
 * @brief Forces a short on the input while in rest, but no current draw.
 * The reverse is setShortOff();
 */
void setShortInRest(void) {
    setShortOnInput();
    digitalWriteFast(SIGNAL_OUT, LOW);
    setTestModeDCIS();  // force 0 current
}

// Current ranges:
enum IRange : uint8_t { range_min = 0,
                        range_100uA = 0,
                        range_1mA = 1,
                        range_10mA = 2,
                        range_100mA = 3,
                        range_max = range_100mA };
//        SW_A  SW_B EN_1mA EN_10mA EN_100mA
// 100uA  0     0    0      0       0
//   1mA  1     0    1      0       0
//  10mA  0     1    x      1       0
// 100mA  1     1    x      x       1

/**
 * @brief The last set current range.
 * To be used only by getIRange and setIRange
 *
 */
static uint8_t _iCurrentRange = 0;

/**
 * @brief Read the scale to be applied on ratiometric (AC or DCIS) readings.
 * Offset is 0, due to the style of measurement.
 *
 * @param range the selected range. Get that from getIRange()
 * @return double: the scale factor for the runAC.. and runDCIS.. functions.
 */
double settingsGetRatio(uint8_t range) {
    // get the scales from EEPROM

    double scale = 0.0;
    // use the range to scale
    switch (range) {
        case range_100uA:
            scale = mySettings.iRange0_1mOhms / 1000.0;
            break;
        case range_1mA:
            scale = mySettings.iRange1_100uOhms / 10000.0;
            break;
        case range_10mA:
            scale = mySettings.iRange2_10uOhms / 100000.0;
            break;
        case range_100mA:
            scale = mySettings.iRange3_1uOhms / 1000000.0;
            break;
        default:
            scale = 1.0;
    }
    return scale;
}

/**
 * @brief Get the current range set, from local var to make it faster
 *
 * @return enum IRange
 */
inline uint8_t getIRange(void) { return _iCurrentRange; }

/**
 * @brief Set the current range
 *
 * @param range enum IRange
 */
inline void setIRange(uint8_t range) {
    digitalWrite(REF_SW_A, range & 1);
    digitalWrite(REF_SW_B, range & 2);
    _iCurrentRange = range;
#ifdef BODGE_REF_SW
    //         PA5   PC2
    //  100uA    1     1
    //    1mA    0     1
    //   10mA    1     0
    //  100mA    0     0
    digitalWrite(PIN_PA5, !(range & 1));
    digitalWrite(PIN_PC2, !(range & 2));
#endif
}

/**
 * @brief return the Current range as a string
 *
 * @param range enum IRange (Arduino IDE does not allow enums as function parameters....)
 * @return fixed string (4 chars wide)
 */
const char *szgetIRange(uint8_t range) {
    switch (range) {
        case range_100uA:
            return "100uA";
        case range_1mA:
            return "  1mA";
        case range_10mA:
            return " 10mA";
        case range_100mA:
            return "100mA";
        default:
            return "???mA";
    }
}

/**
 * @brief return the presently set current range as a string
 *
 * @return fixed string
 */
const char *szgetIRange(void) {
    return szgetIRange(getIRange());
}

/**
 * @brief Get the resistance upper limit value for overflow detection
 *
 * @param range enum IRange (Arduino IDE does not allow enums as function parameters....)
 * @return float
 */
float getRUpperLimit(uint8_t range) {
    switch (range) {
        case range_100uA:
            return 1000.0; // upper limit is absent really, this is the display limit
        case range_1mA:
            return ESR_FULLSCALE / 10.0;
        case range_10mA:
            return ESR_FULLSCALE / 100.0;
        case range_100mA:
            return ESR_FULLSCALE / 1000.0;
        default:
            return (float)ESR_FULLSCALE;
    }
}

/**
 * @brief Get the resistance under limit value for underflow detection.
 * Roughly upperlimit / 10, with a small hysteresis added
 *
 * @param range enum IRange (Arduino IDE does not allow enums as function parameters....)
 * @return float
 */
float getRUnderLimit(uint8_t range) {
    switch (range) {
        case range_100uA:
            return ESR_FULLSCALE / 10.1; 
        case range_1mA:
            return ESR_FULLSCALE / 110.0;
        case range_10mA:
            return ESR_FULLSCALE / 1100.0;
        case range_100mA:
            return 0.0; // this is the display limit
        default:
            return (float)ESR_FULLSCALE;
    }
}

#define RANGE_AS_PICTOS
/**
 * @brief return the Resistance range as a string
 * This must be aligned with screenFormatR and getRUpperLimit
 * It uses the specialised XXSFONT, so some characters are relocated
 *
 * @param range enum IRange (Arduino IDE does not allow enums as function parameters....)
 * @param isAuto if on autoranging
 * @param isTopline if on first line of range info
 * @return variable length string (max 3 chars wide)
 */
const char *szgetRRange(uint8_t range, bool isAuto, bool isTopline) {
#ifdef RANGE_AS_PICTOS
    // range pictos start at "2", and "Auto" = "/"
    if (!isTopline) {
        if (isAuto)
            return "/";  // the sign for "Auto"
        else
            return "";
    }
    switch (range) {
        case range_100uA:
            return "2";
        case range_1mA:
            return "3";
        case range_10mA:
            return "4";
        case range_100mA:
            return "5";
        default:
            return "/";
    }
#else
    (void) isTopline;
    // TODO adapt for top line, if I use this method
    // range as text
    if (isAuto)
        return "/";  // the sign for "Auto"
    switch (range) {
        case range_100uA:
            return "100";
        case range_1mA:
            return "10";
        case range_10mA:
            return "1";
        case range_100mA:
            return "0.1";
        default:
            return "/";
    }
#endif
}

/**
 * @brief Go up in range (100uA -> 100mA)
 *
 * @return bool when False: could not move, already to the limit
 */
uint8_t setIRangeUp(void) {
    uint8_t r = getIRange();
    if (r < range_max) {
        r++;
        setIRange(r);
        return true;
    }
    return false;
}

/**
 * @brief Go down in range (100mA -> 100uA)
 *
 * @return bool when False: could not move, already to the limit
 */
uint8_t setIRangeDown(void) {
    uint8_t r = getIRange();
    if (r > range_min) {
        r--;
        setIRange(r);
        return true;
    }
    return false;
}

// Early IO pin setup. Not allowed to do serial or screen printing
void setupIOPins(void) {
    // urgent stuff
    pinMode(CPU_PWR_ON, OUTPUT);
    setPowerOn();

    // The rest
    pinMode(SIGNAL_OUT, OUTPUT);
    digitalWriteFast(SIGNAL_OUT, LOW);
    pinMode(OUT_SW, OUTPUT);
    setTestModeDCIS();  // force 0 current

    pinMode(ADC_IN_SW, OUTPUT);
    setADCInMode(adcin_batt);

    pinMode(REF_SW_A, OUTPUT);
    pinMode(REF_SW_B, OUTPUT);
#ifdef BODGE_REF_SW
    pinMode(PIN_PA5, OUTPUT);
    pinMode(PIN_PC2, OUTPUT);
#endif
    setIRange(range_min);

    pinMode(SHORT_DISABLE, OUTPUT);
    setShortOff();
    pinMode(SHORT_SW, OUTPUT);
    // no need to set it, as short is off
}

#pragma endregion

#pragma region general ADC Handling
// ************************************************************************************************
// ADC Handling
// ************************************************************************************************

// forward declarations
double getExternalVoltage(void);
bool haveEnoughVoltageToTest(void);
bool haveEnoughVoltageToTest(double voltage);

/* if the read value indicates that a lower current range must be used */
#define bm_adcDiffSampleResults_overrange 0x01
/* if the read value indicates that a higher current range must be used */
#define bm_adcDiffSampleResults_underrange 0x02
/* if the input voltage is too low to allow for good readings */
#define bm_adcDiffSampleResults_undervoltage 0x04
/* if ADC overload was detected. May be the result of inadequate input shorting */
#define bm_adcDiffSampleResults_overload 0x10
/* if ADC overrun (sampling rate too low). Programming or clock problem. */
#define bm_adcDiffSampleResults_overrun 0x20
/* if ADC timeout. Programming or processor problem. */
#define bm_adcDiffSampleResults_timeout 0x40
/* if shorting was not possible. Just retry */
#define bm_adcDiffSampleResults_shortproblem 0x80

// This is for AC and DC
#define ADC_MAX_SCALE_MV 2048
#define ADC_REFERENCE INTERNAL2V048
// Diff measurements 12 bits min/max
#define MIN_12Bits -2048
#define MAX_12Bits 2047

// Setup the ADC, also initalises all other ADC stuff
// If you want fast ADC, you MUST call this every time you switch to fast.
// Reason: errata 2.2.1: Low Latency Mode Must Be Set Before Changing ADC Configuration
void setup_ADC(bool fast = false) {
    if (fast) {
        ADCPowerOptions(LOW_LAT_ON);  // low latency on. Maximum power consumption, minimum ADC delays.

        analogReference(ADC_REFERENCE);  // INTERNAL1V024 INTERNAL2V048 INTERNAL2V5 // 2,048 seems the most accurate on the chip.
        // On top of that, with 2.048 I will match the 12 bit result: 2048 = 2048mV
        analogReadResolution(12);  // this is the max value and the default value, so why set it?
        // we have a low impedance input, so sample duration can be short.
        analogSampleDuration(2);  // the lowest
        // analogSampleDuration(15); // default

        // analogClockSpeed(-1);    // set to default: 2500. But can set 300 to 3000 kHz
        analogClockSpeed(3000);  // The fastest, but apparently does not work. I get 2500 back.
    } else {
        ADCPowerOptions(LOW_LAT_OFF);  // low latency off.

        analogReference(ADC_REFERENCE);  // INTERNAL1V024 INTERNAL2V048 INTERNAL2V5 // 2,048 seems the most accurate on the chip.
        // On top of that, with 2.048 I will match the 12 bit result: 2048 = 2048mV
        analogReadResolution(12);  // this is the max value and the default value, so why set it?
        // we have a low impedance input, so sample duration can be short.
        analogSampleDuration(15);  // default

        analogClockSpeed(-1);  // set to default: 2500. But can set 300 to 3000 kHz
    }

#ifdef DEBUG_MSG
    Serial.printf("Set ADC ReadResolution to %d, SampleDuration to %d and ClockSpeed to %d\n", 12, getAnalogSampleDuration(), analogClockSpeed());
#endif
}

#define MIN_TEST_VOLTAGE_V 1.1
/**
 * @brief Test if there is enough voltage at the input to do AC or DCIS tests.
 *
 * @return true if OK
 */
bool haveEnoughVoltageToTest(void) {
    return (getExternalVoltage() > MIN_TEST_VOLTAGE_V);
}

/**
 * @brief Test if there is enough voltage at the input to do AC or DCIS tests.
 *
 * @param voltage The read voltage. Should come from getExternalVoltage()
 * @return true if OK
 */
bool haveEnoughVoltageToTest(double voltage) {
    return (voltage > MIN_TEST_VOLTAGE_V);
}

/**
 * @brief Translate sampling result flags to a string for display or serial print use.
 *
 * @param resultFlags see bm_adcDiffSampleResults_....
 * @param reduced if True, simplify output (for screen)
 * @return const char* NULL when no error
 */
const char *sampleResultToString(uint8_t resultFlags, bool reduced) {
    // The resultflags:
    // In sequence:
    // 1) timeout or overrun: programmer or HW problem. Bad. Results mean nothing now.: show ERR
    // 2) undervoltage: Bad. Results mean nothing now.: show LOW V
    // 3) overrange or shortproblem or overload: show overload (I might have a value but it may not fit on screen)
    // 4) underrange or nothing: Show the value, correctly ranged. Underrange means something for the autoranger, but not for the display

    if (resultFlags & bm_adcDiffSampleResults_timeout) {
        if (reduced)
            return strERROR;
        else
            return strTimeOut;
    } else if (resultFlags & bm_adcDiffSampleResults_overrun) {
        if (reduced)
            return strERROR;
        else
            return strOverrun;
    } else if (resultFlags & bm_adcDiffSampleResults_undervoltage) {
        return strLowV;
    } else if (resultFlags & bm_adcDiffSampleResults_overrange) {
        if (reduced)
            return strOver;
        else
            return strOverrange;
    } else if (resultFlags & bm_adcDiffSampleResults_shortproblem) {
        if (reduced)
            return strOver;
        else
            return strShortProblem;
    } else if (resultFlags & bm_adcDiffSampleResults_overload) {
        if (reduced)
            return strOver;
        else
            return strOverload;
    }
    return NULL;
}

#pragma endregion

#pragma region 1kHz tone generator and AC sampler

// ************************************************************************************************
// 1kHz tone generator
// ************************************************************************************************

// inspired by https://github.com/SpenceKonde/megaTinyCore/blob/master/megaavr/extras/TakingOverTCA0.md, example 2

void setup_ToneGen(void) {
#if defined(MILLIS_USE_TIMERA0) || defined(__AVR_ATtinyxy2__)
#error "This sketch takes over TCA0, don't use for millis here.  Pin mappings on 8-pin parts are different"
#endif

    takeOverTCA0();  // This replaces disabling and resetting the timer, required previously.
}

// pin can be:  0 or PB4 or PB5. (WO1' or WO2') Cannot use PB0..PB3, as they collide with I2C or serial
// if 0: just starts TCA0, which can then be used by EVSYS with OVF. Note that PWM will be useless then
void startToneGen(int pin, unsigned long freqInHz = 16000, byte dutyCycle = 50) {
    unsigned long tempperiod = (F_CPU / freqInHz);
    unsigned long cmpval = 0;
    byte presc = 0;

    if ((pin != 0) && (pin != PIN_PB4) && (pin != PIN_PB5)) {
        Serial.printf("FATAL ERROR: cannot set ToneGen on pin %d, as it is an illegal pin\n", pin);
        return;
    }

    if (pin != 0) {
        pinMode(pin, OUTPUT);
    }

    // 21.3.3.4.3 Single-Slope PWM Generation
    // The compare channels can be used for waveform generation on the corresponding port pins. The following requirements must be met to make the waveform visible on the connected port pin:
    // 1. A Waveform Generation mode must be selected by writing the Waveform Generation Mode (WGMODE) bit field in the TCAn.CTRLB register.
    // 2. The compare channels used must be enabled (CMPnEN = 1 in TCAn.CTRLB). This will override the output value for the corresponding pin.
    //    An alternative pin can be selected by configuring the Port Multiplexer (PORTMUX). Refer to the PORTMUX section for details.
    // 3. The direction for the associated port pin n must be configured in the Port peripheral as an output.
    // 4. Optional: Enable the inverted waveform output for the associated port pin n. Refer to the PORT section for details.

    // For single-slope Pulse-Width Modulation (PWM) generation, the period (T) is controlled by the TCAn.PER register, while the values of the TCAn.CMPn registers control the duty cycles of the generated waveforms.
    // CMPn = BOTTOM will produce a static low signal on WOn while CMPn > TOP will produce a static high signal on WOn.

    while (tempperiod > 65536 && presc < 7) {
        presc++;
        tempperiod = tempperiod >> (presc > 4 ? 2 : 1);
    }
    cmpval = map(dutyCycle, 0, 100, 0, tempperiod);

    // set the mode: single slope WG
    TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_SINGLESLOPE_gc;
    // set period and prescaler
    TCA0.SINGLE.PER = tempperiod;
    TCA0.SINGLE.CTRLA = (presc << 1);

    // If pins are needed:
    // Set Alternative outputs
    // Set output and the correct comparator (and therefore the correct pin number)
    // also set the PWM value
    if (pin == PIN_PB4) {
        PORTMUX.TCAROUTEA = PORTMUX_TCA01_ALT1_gc;
        TCA0.SINGLE.CTRLB |= TCA_SINGLE_CMP1EN_bm;
        TCA0.SINGLE.CMP1 = cmpval;
    }
    if (pin == PIN_PB5) {
        PORTMUX.TCAROUTEA = PORTMUX_TCA02_ALT1_gc;
        TCA0.SINGLE.CTRLB |= TCA_SINGLE_CMP2EN_bm;
        TCA0.SINGLE.CMP2 = cmpval;
    }

    // and then enable the timer
    TCA0.SINGLE.CTRLA |= TCA_SINGLE_ENABLE_bm;

    // Serial.printf("Set ToneGen(%d,%luHz,%d%%). TCA0.SINGLE.CTRLA=%02x, TCA0.SINGLE.PER=%lu, F_CPU=%lu\n",pin, (unsigned long)freqInHz, (int)dutyCycle, (int) TCA0.SINGLE.CTRLA, (unsigned long)tempperiod, (unsigned long)F_CPU);
}

void stopToneGen(void) {
    TCA0.SINGLE.CTRLA = 0;
}

// ************************************************************************************************
// AC sampler (1khz)
// ************************************************************************************************

// The AC sampler sends an AC current (with a small DC component) through the test subject,
// and determines the ratio between the voltage over a known resistance and the voltage over the test subject.
// The voltage drop stays below 10mV to avoid some chemical effects that influence the test subject impedance.
// The DC component is half of the maximum current, and should have no effect on the test result.
// Signal interpretation is done with the goertzel algorithm, as it is fast and has fairly good noise rejection.
// I create the 1kHz signal on the fly, in lockstep with the sampling, to make sure goertzel stays on the center frequency.

/**
 * @brief Results from differential measurements for AC sampling
 */
struct acSampleResults {
    double vRef;                      /**< AC voltage reading over the reference, via Goertzel */
    double vTest;                     /**< AC voltage reading over the input, via Goertzel */
    uint8_t rangeUsed;                /**< The range used for the results. May be different that the present range! */
    int32_t minSampleRef;             /**< minimum reading over the reference, ADC raw value */
    int32_t maxSampleRef;             /**< maximum reading over the reference, ADC raw value */
    int32_t minSampleTest;            /**< minimum reading over the input, ADC raw value */
    int32_t maxSampleTest;            /**< maximum reading over the input, ADC raw value */
    struct adcShortResults shortRef;  /**< Results of the short on Ref */
    struct adcShortResults shortTest; /**< Results of the short on Test */
    double vDCInput;                  /**< DC input voltage before test */
    uint8_t resultFlags;              /**< 0 = OK. For bitmap, see bm_adcDiffSampleResults_... values */
};

double getImpedance(struct acSampleResults *pResults, uint8_t measureFlags, bool autoRange);

// Configuration:
// samples per period. Does NOT have to be a power of 2, since I use Goertzel.
// This is all about a 10+kHz sampling, and 1kHz target frequency
// Above 14kHz sample rate, I must buffer first, as the code will be too slow. And buffering is limited by memory (I use 2 bytes per sample)
// I have about 20us left per sample. (without the error detection)
// So stay below 14kHz
// must be an even number
#define AC_SAMPLES_PER_PERIOD 10
// CALCULATE THIS!!! coeff = 2 * cos(2 * pi * freq/sample_rate) = 2 * cos(2 * pi / AC_SAMPLES_PER_PERIOD)
#if AC_SAMPLES_PER_PERIOD == 10
#define GOERTZEL_COEFF (1.618033988749895)
#endif

#if AC_SAMPLES_PER_PERIOD == 12
#define GOERTZEL_COEFF (1.732050807568877)
#endif

#if AC_SAMPLES_PER_PERIOD == 14
#define GOERTZEL_COEFF (1.801937735804838)
#endif

#if AC_SAMPLES_PER_PERIOD == 16
#define GOERTZEL_COEFF (1.847759065022574)
#endif

// let AC signal settle at the start of the sampling. Will be done in 10-15 cycles, but add some margin
#define AC_PERIODS_LEAD_IN 25

// let measure amplitude of the AC signal. Cannot do that while doing the real sampling unfortunately
#define AC_PERIODS_RANGE_LEAD_IN 5

// when using external AC signal, sync up the phase for max N periods
// Remove this define in production
//#define DEBUG_EXT_SIGNAL_SYNC 5

// periods to be sampled. Does NOT have to be a power of 2, since I use Goertzler.
// But the other requirements are:
//  * AC_PERIODS_SAMPLED*AC_SAMPLES_PER_PERIOD needs to be divisible by 4
//  * The more samples, the better noise suppression
//  * Why not make it multiples of 20, for 50Hz PLFNR or 16.66666 for 60Hz PLFNR
// 1 period of 1kHz = 1 ms
// 100 (0.1sec) suits both 50Hz and 60Hz: 5 cycles of 50Hz, 6 cycles of 60Hz
#define AC_PERIODS_SAMPLED 100

// So total duration = (AC_PERIODS_LEAD_IN + AC_PERIODS_RANGE_LEAD_IN + AC_PERIODS_SAMPLED) * 1 ms

#define NR_AC_SAMPLES (AC_PERIODS_SAMPLED * AC_SAMPLES_PER_PERIOD)

// ADC error situation detection via interrupts, so in global var
int had_ADC_ERR;

ISR(ADC0_ERROR_vect) {
    byte flags = ADC0.INTFLAGS;
    had_ADC_ERR = 1;
    // clear flags
    ADC0.INTFLAGS = flags;
}

/**
 * @brief Assist millis handling around the AC sampling.
 * The AC sampling halts the timers.
 * To be used only by _startACSample and _stopACSample
 */
static uint32_t _samplerOldMillis;

/**
 * @brief start the ADC sampling. Switches the ADC to high speed,
 * disables regular interrupts, activates interrups regarding ADC overflows
 */
void _startACSample(void) {
// I should use TIMER_B1, but this code can also work with timers disabled.
#ifndef MILLIS_USE_TIMERNONE
    _samplerOldMillis = millis();
    stop_millis();
    /* Expected usage:
     * uint32_t _samplerOldMillis=millis();
     * stop_millis();
     * user_code_that_messes with timer
     * set_millis(_samplerOldMillis+estimated_time_spent_above)
     * restart millis(); */
#endif

    setTestModeAC();
    setup_ADC(true);
    setup_ToneGen();

    digitalWrite(SIGNAL_OUT, LOW);

#ifdef AC_SIGNAL_DEBUG_WAITPIN_ON_PA5
    pinMode(PIN_PA5, OUTPUT);
    digitalWrite(PIN_PA5, LOW);
#endif

    // Throw the first away, used for setting the ADC thingies
    analogReadDiff(ADC_INP, ADC_INN, ADC_NATIVE_RESOLUTION, 0);  // is about 20us

    // Serial.printf("startACSample. ADC0.COMMAND=%02x, ADC0.MUXPOS=%02x, ADC0.MUXNEG=%02x\n",(unsigned int)ADC0.COMMAND,(unsigned int)ADC0.MUXPOS,(unsigned int)ADC0.MUXNEG);

    startToneGen(0, AC_SAMPLES_PER_PERIOD * 1000);  // AC_SAMPLES_PER_PERIOD * 1kHz

    Event0.set_generator(gen::tca0_ovf_lunf);  // Set the TCA0 counter overflow as trigger for Event0
    ADC0.COMMAND |= 4;                         // start on EVENT_TRIGGER
    Event0.set_user(user::adc0_start);         // this provokes an event trigger signal to be sent to ADC0 upon Event0, which is generated by tca0_ovf_lunf

#ifdef AC_SIGNAL_DEBUG_TRIGGERPIN_ON_PC2
    // debug
    Event0.set_user(user::evoutc_pin_pc2);  // this provokes a 100ns pulse on PC2
#endif

    had_ADC_ERR = 0;
    ADC0.INTCTRL = ADC_RESOVR_bm;  // interrupt on RESOVR error condition

    // Start the event channel
    Event0.start();
}

/**
 * @brief stop the ADC sampling.
 * Sets regular interrupts, removes specific ADC interrupts
 */
void _stopACSample(void) {
    Event0.stop();
    ADC0.COMMAND &= 0xf0;  // stop  EVENT_TRIGGER starting
    ADC0.INTCTRL = 0;      // stop interrupt on all error conditions

    ADCPowerOptions(LOW_LAT_OFF);  // low latency off. Minimum power consumption, maximum ADC delays.
    setTestModeDCIS();             // the AC filter gives a spike when ending the cycle, so remove the AC signal ASAP.
    digitalWrite(SIGNAL_OUT, LOW);

    stopToneGen();
    // Serial.printf("stopACSample. ADC0.COMMAND=%02x, ADC0.MUXPOS=%02x, ADC0.MUXNEG=%02x\n",(unsigned int)ADC0.COMMAND,(unsigned int)ADC0.MUXPOS,(unsigned int)ADC0.MUXNEG);

// I should use TIMER_B1, but this code can also work with timers disabled.
#ifndef MILLIS_USE_TIMERNONE
    set_millis(_samplerOldMillis);  // I should to set_millis(_samplerOldMillis+estimated_time_spent_above), but that is complicated and little gain.
    restart_millis();
#endif
}

inline int32_t _analogReadEnhLastPart(void) __attribute__((always_inline));
int32_t _analogReadEnhLastPart(void) {
#ifdef AC_SIGNAL_DEBUG_WAITPIN_ON_PA5
    VPORTA.OUT |= 0x20;
#endif
    // fast polling on end of conversion
    uint16_t adc_loopcounter = 1;
    while (!(ADC0.INTFLAGS & ADC_RESRDY_bm)) {
        adc_loopcounter++;
        if (!adc_loopcounter) {
            had_ADC_ERR = 2;
            return 0;
        }
    };
#ifdef AC_SIGNAL_DEBUG_WAITPIN_ON_PA5
    VPORTA.OUT &= 0xDF;
#endif

    return ADC0.RESULT;
}

// lead-in AC signal out. Required for the AC filter amp to settle.
#define bm_runACSample_LeadIn 0x01
// measure Ref
#define bm_runACSample_Ref 0x02
// measure Input
#define bm_runACSample_Test 0x04
// short input and ref during lead-in to the test. This does not take up extra time, contrary to setShortWithDuration, but influences the load
#define bm_runACSample_ShortDuring 0x10
// short input and ref before the test. This does take some time, via setShortWithDuration
#define bm_runACSample_ShortBefore 0x20
// 30 sec long AC signal out, no measurement, amp is connected to test during the run, preceded by a regular short, but no abandon if input voltage is low
#define bm_runACSample_longACTest 0x40
// 30 sec long AC signal out, no measurement, amp is connected to ref during the run, preceded by a regular short, but no abandon if input voltage is low
#define bm_runACSample_longACRef 0x80

// nothing set = default set.
#define bm_runACSample_Default 0x27

/**
 * @brief The main AC test function.
 * Run a 1kHz sample, with a ratiometric approach
 * By default, it does the following:
 *  * read the DC voltage
 *  * short the inputs for the diff amp, while loading the test subject at 50% of the test current. This way the input caps adapt to the DC voltage.
 *  * validate that the DC voltage is still OK
 *  * start the AC current signal, and allow for settling
 *  * measure the AC voltage over the reference resistor, using Goertzel
 *  * measure the AC voltage over the test subject, using Goertzel
 *  * shut down the AC current
 *  * calculate the impedance
 * Make sure you wait 25 msec before calling this again, when in a loop. The 25 msec is for the AC output filter to settle down.
 * @param pResults The struct that holds the detailed results. See struct acSampleResults
 * @param offset The offset to be applied to the raw diff value
 * @param scale The scale to be applied to the raw diff value
 * @param upperlimit The upper limit of the returned values before overrange is to be signalled. 
 * @param underlimit The lower limit of the returned values before underrange is to be signalled. 
 * @param measureFlags See bm_runACSample_...
 * @return the impedance, in Ohm
 */
double runACSample(struct acSampleResults *pResults, double offset, double scale, float upperlimit, float underlimit, uint8_t measureFlags) {
    byte j = 0;

    if (pResults) {
        pResults->vRef = 0.0;
        pResults->vTest = 0.0;
        pResults->vDCInput = 0.0;
        pResults->rangeUsed = getIRange();
        pResults->minSampleRef = MAX_12Bits;
        pResults->maxSampleRef = MIN_12Bits;
        pResults->minSampleTest = MAX_12Bits;
        pResults->maxSampleTest = MIN_12Bits;
        memset(&pResults->shortRef, 0, sizeof(pResults->shortRef));
        memset(&pResults->shortTest, 0, sizeof(pResults->shortTest));
        pResults->resultFlags = 0;
    }

    double Q0R = 0.0;
    double Q1R = 0.0;
    double Q2R = 0.0;

    double Q0T = 0.0;
    double Q1T = 0.0;
    double Q2T = 0.0;

    int32_t minSampleR = MAX_12Bits;
    int32_t maxSampleR = MIN_12Bits;

    int32_t minSampleT = MAX_12Bits;
    int32_t maxSampleT = MIN_12Bits;

    int32_t v = 0;

    bool lowVoltage = false;

    // default measureFlags
    if (measureFlags == 0)
        measureFlags = bm_runACSample_Default;

    // if long AC test, everything else is off.
    bool longTest = ((measureFlags & bm_runACSample_longACRef) || (measureFlags & bm_runACSample_longACRef));
    // clean up input flags if needed
    if (measureFlags & bm_runACSample_longACRef)
        measureFlags = bm_runACSample_longACRef | bm_runACSample_ShortBefore;
    if (measureFlags & bm_runACSample_longACRef)
        measureFlags = bm_runACSample_longACTest | bm_runACSample_ShortBefore;

    // Remember the input voltage
    double vDCInput = getExternalVoltage();

    // Short if wanted
    if (measureFlags & bm_runACSample_ShortBefore) {
        struct adcShortResults *p;
        int r1, r2;
        p = NULL;
        if (pResults)
            p = &(pResults->shortRef);
        r1 = setShortWithDuration(false, true, p);
        if (pResults)
            p = &(pResults->shortTest);
        r2 = setShortWithDuration(true, true, p);

        // any short problems? If so, cancel the test
        if ((r1 < 0) || (r2 < 0)) {
            // cancel the test, unless I am doing a long test
            if (!longTest)
                measureFlags = 0;
        }
    }

    // enough voltage to test? Test also after the short:
    // * the battery might suffer from that, I don't want to break the results due to a very bad battery
    // * when open circuit, I read a phantom voltage
    // So: also interpret after a short.
    // But: I just loaded the input protection of multiple Ohms, lowering the DC input voltage significantly. The DC measurement curcuit will take a rather long time to get back.
    // So if the voltage is bad, remember the voltage from here. If good: remember the earlier, unloaded voltage
    double vDCInput2 = getExternalVoltage();
    if (!haveEnoughVoltageToTest(vDCInput2)) {
        lowVoltage = true;
        // and cancel the test
        vDCInput = vDCInput2;
        // and cancel the test, unless I am doing a long test
        if (!longTest)
            measureFlags = 0;
    }

    _startACSample();

    // !!!!!!!!!!! From here on, I need utmost speed. So there will be some code duplication

    had_ADC_ERR = 0;

    if (measureFlags & bm_runACSample_ShortDuring) {
        setShortOnRef();
    } else {
        setShortOff();
    }
    setADCInMode(adcin_ref);  // have to set something....

    // Serial.printf("runACSample. ADC0.COMMAND=%02x, ADC0.MUXPOS=%02x, ADC0.MUXNEG=%02x\n",(unsigned int)ADC0.COMMAND,(unsigned int)ADC0.MUXPOS,(unsigned int)ADC0.MUXNEG);

    // debug Sync up
#ifdef DEBUG_EXT_SIGNAL_SYNC
    int32_t v_old = 0;
    int32_t v_mid = 0;
    int32_t minSample = MAX_12Bits;
    int32_t maxSample = MIN_12Bits;
    for (int i = 0; (i < (DEBUG_EXT_SIGNAL_SYNC * AC_SAMPLES_PER_PERIOD)) && (!had_ADC_ERR); i++) {
        v = _analogReadEnhLastPart();

        if (i < ((DEBUG_EXT_SIGNAL_SYNC - 2) * AC_SAMPLES_PER_PERIOD)) {
            if (v > maxSample)
                maxSample = v;
            if (v < minSample)
                minSample = v;
            v_mid = minSample + ((maxSample - minSample) / 2);
            v_old = v;
        } else {
            if ((v_old < v_mid) && (v >= v_mid)) {
                break;
            } else {
                v_old = v;
            }
        }

        // not maintaining the wave form, as we have an external wave form here.
    }
#endif

    // digitalWriteFast(SAMPLESIGNAL_PIN, HIGH);
    //  Lead in
    long int lead_in_duration = 0;
    if (measureFlags & bm_runACSample_LeadIn)
        lead_in_duration = AC_PERIODS_LEAD_IN * AC_SAMPLES_PER_PERIOD;
    // or long AC run
    if ((measureFlags & bm_runACSample_longACRef) || (measureFlags & bm_runACSample_longACTest)) {
        // lead_in_duration = (AC_PERIODS_LEAD_IN + AC_PERIODS_RANGE_LEAD_IN + AC_PERIODS_SAMPLED) * AC_SAMPLES_PER_PERIOD;
        lead_in_duration = 30000L * AC_SAMPLES_PER_PERIOD;
        setShortOff();  // no shorting during this test
        if (measureFlags & bm_runACSample_longACRef)
            setADCInMode(adcin_ref);
        else
            setADCInMode(adcin_input);
        // and skip all the rest of the code, now that I'm done testing.
        measureFlags = 0;
    }
    for (int i = 0; (i < lead_in_duration) && (!had_ADC_ERR); i++) {
        // read ADC but throw away the result, just to keep timing equivalent (the timer drives the ADC)
        _analogReadEnhLastPart();

        // maintain the output wave form, on fixed delay after the ADC reading, as that is on a fixed clock
        j++;
        j %= AC_SAMPLES_PER_PERIOD;
        if (j >= (AC_SAMPLES_PER_PERIOD / 2))
            digitalWriteFast(SIGNAL_OUT, HIGH);
        else
            digitalWriteFast(SIGNAL_OUT, LOW);
    }

    // Preparation has been done.
    if (measureFlags & bm_runACSample_Ref) {
        // if short is wanted, short input from here
        if (measureFlags & bm_runACSample_ShortDuring)
            setShortOnInput();

        // Now read the reference
        setADCInMode(adcin_ref);

        // ranging
        for (int i = 0; (i < (AC_PERIODS_RANGE_LEAD_IN * AC_SAMPLES_PER_PERIOD)) && (!had_ADC_ERR); i++) {
            v = _analogReadEnhLastPart();

            // maintain the output wave form, on fixed delay after the ADC reading, as that is on a fixed clock
            j++;
            j %= AC_SAMPLES_PER_PERIOD;
            if (j >= (AC_SAMPLES_PER_PERIOD / 2))
                digitalWriteFast(SIGNAL_OUT, HIGH);
            else
                digitalWriteFast(SIGNAL_OUT, LOW);

            // Then do the calculations, variable duration
            if (v > maxSampleR)
                maxSampleR = v;
            if (v < minSampleR)
                minSampleR = v;
        }

        // The real sampling
        // calculate Goertzel on the fly
        for (int i = 0; (i < NR_AC_SAMPLES) && (!had_ADC_ERR); i++) {
            v = _analogReadEnhLastPart();

            // maintain the output wave form, on fixed delay after the ADC reading, as that is on a fixed clock
            j++;
            j %= AC_SAMPLES_PER_PERIOD;
            if (j >= (AC_SAMPLES_PER_PERIOD / 2))
                digitalWriteFast(SIGNAL_OUT, HIGH);
            else
                digitalWriteFast(SIGNAL_OUT, LOW);

            // Then do the calculations, variable duration
            // If I add ranging here, I am down from 20us headroom to 5 us, which is too close to call, so no ranging here.
            // AVR Tiny uses 32 bit single precision fp. That is good enough for 4 digits resolution. I do not have double or long double
            // also, going through integers only makes it worse and is not significantly faster
            Q0R = (GOERTZEL_COEFF * Q1R) - Q2R - (double)v;
            Q2R = Q1R;
            Q1R = Q0R;
        }
    }

    // switch off the short before reading the input.
    setShortOff();

    if (measureFlags & bm_runACSample_Test) {
        // Now read the test input
        setADCInMode(adcin_input);

        // ranging
        for (int i = 0; (i < (AC_PERIODS_RANGE_LEAD_IN * AC_SAMPLES_PER_PERIOD)) && (!had_ADC_ERR); i++) {
            v = _analogReadEnhLastPart();

            // maintain the output wave form, on fixed delay after the ADC reading, as that is on a fixed clock
            j++;
            j %= AC_SAMPLES_PER_PERIOD;
            if (j >= (AC_SAMPLES_PER_PERIOD / 2))
                digitalWriteFast(SIGNAL_OUT, HIGH);
            else
                digitalWriteFast(SIGNAL_OUT, LOW);

            // Then do the calculations, variable duration
            if (v > maxSampleT)
                maxSampleT = v;
            if (v < minSampleT)
                minSampleT = v;
        }

        // The real sampling
        // calculate Goertzel on the fly
        for (int i = 0; (i < NR_AC_SAMPLES) && (!had_ADC_ERR); i++) {
            v = _analogReadEnhLastPart();

            // maintain the output wave form, on fixed delay after the ADC reading, as that is on a fixed clock
            j++;
            j %= AC_SAMPLES_PER_PERIOD;
            if (j >= (AC_SAMPLES_PER_PERIOD / 2))
                digitalWriteFast(SIGNAL_OUT, HIGH);
            else
                digitalWriteFast(SIGNAL_OUT, LOW);

            // Then do the calculations, variable duration
            // If I add ranging here, I am down from 20us headroom to 5 us, which is too close to call, so no ranging here.
            Q0T = (GOERTZEL_COEFF * Q1T) - Q2T - (double)v;
            Q2T = Q1T;
            Q1T = Q0T;
        }
    }

    // Closing off, doing the calculations

    // digitalWriteFast(SAMPLESIGNAL_PIN, LOW);
    _stopACSample();

    setADCInMode(adcin_ref);

    double amplR = sqrt((Q1R * Q1R) + (Q2R * Q2R) - (GOERTZEL_COEFF * Q1R * Q2R)) / (NR_AC_SAMPLES / 4);
    double amplT = sqrt((Q1T * Q1T) + (Q2T * Q2T) - (GOERTZEL_COEFF * Q1T * Q2T)) / (NR_AC_SAMPLES / 4);
    bool haveRatio = (measureFlags & bm_runACSample_Ref) && (measureFlags & bm_runACSample_Test);

    double rval = 0.0;
    if (haveRatio) {
        if (amplR != 0) {
            rval = offset + (scale * (amplT / amplR));
        }
    }

    if (rval < 0.0)
        rval = 0.0;

    if (pResults) {
        // copy the readings
        pResults->vRef = amplR;
        pResults->vTest = amplT;
        pResults->vDCInput = vDCInput;
        pResults->rangeUsed = getIRange();

        pResults->minSampleRef = minSampleR;
        pResults->maxSampleRef = maxSampleR;
        pResults->minSampleTest = minSampleT;
        pResults->maxSampleTest = maxSampleT;

        bool bRefOverflow = false;
        bool bTestOverflow = false;

        bRefOverflow = (minSampleR <= MIN_12Bits) || (maxSampleR >= MAX_12Bits);
        bTestOverflow = (minSampleT <= MIN_12Bits) || (maxSampleT >= MAX_12Bits);

        // determine resultFlags (0 by default)
        if (lowVoltage)
            pResults->resultFlags |= bm_adcDiffSampleResults_undervoltage;

        if ((pResults->shortRef.duration < 0) || (pResults->shortTest.duration < 0))
            pResults->resultFlags |= bm_adcDiffSampleResults_shortproblem;

        if (had_ADC_ERR & 1)
            pResults->resultFlags |= bm_adcDiffSampleResults_overrun;
        if (had_ADC_ERR & 2)
            pResults->resultFlags |= bm_adcDiffSampleResults_timeout;

        if (bRefOverflow || bTestOverflow)
            pResults->resultFlags |= bm_adcDiffSampleResults_overload;

        // TODO detect amplR wrong values
        // Should be at about half max range

        // determine range issues. But ignore ref overflow or lowVoltage here, as anything can happen then
        if (haveRatio && !bRefOverflow && !lowVoltage) {
            if (bTestOverflow)
                pResults->resultFlags |= bm_adcDiffSampleResults_overrange;
            else {
                if (rval >= upperlimit)
                    pResults->resultFlags |= bm_adcDiffSampleResults_overrange;
                if (rval < underlimit)
                    pResults->resultFlags |= bm_adcDiffSampleResults_underrange;
            }
        }
    }

    return rval;
}

/**
 * @brief Get test device AC 1kHz Impedance
 * When autoranging is requested, I do one measurement,
 * determine if I'm out of range, adapt the range for the next measurement,
 * but still present the results. That way, the UI is more reactive.
 *
 * @param pResults See runACSample
 * @param measureFlags See runACSample
 * @param autoRange if true, do autorange
 * @return the impedance in Ohm. Use the pDiag to find out any error conditions
 */
double getImpedance(struct acSampleResults *pResults, uint8_t measureFlags, bool autoRange) {
    double offset = 0;
    uint8_t range = getIRange();
    double scale = settingsGetRatio(range);
    float upperlimit = getRUpperLimit(range);
    float underlimit = getRUnderLimit(range);

    double v = runACSample(pResults, offset, scale, upperlimit, underlimit, measureFlags);
    // do the autoranging
    if (autoRange) {
        if (pResults) {
            if ((pResults->resultFlags & bm_adcDiffSampleResults_overrange) ||
                (pResults->resultFlags & bm_adcDiffSampleResults_overload) ||
                (pResults->resultFlags & bm_adcDiffSampleResults_shortproblem))
                setIRangeDown();
            else if (pResults->resultFlags & bm_adcDiffSampleResults_underrange)
                setIRangeUp();
        }
    }
    return v;
}

#pragma endregion

#pragma region DCIS sampler
// ************************************************************************************************
// DCIS Sampler
// ************************************************************************************************

// PLFNR/Power Line Frequency Noise Rejection:
// AC sampling has superb noise rejection, due to the very narrow bandpass filter around 1kHz (really, it is a couple of Hz wide).
// DCIS does not have that. Since the measurement times are fixed, I must do averaging.
// Sample time = Settings.iT1_usecs + Settings.iT2_usecs + Settings.iTPause_usecs (by default 250 + 10000 + 5000 = 15.25msec)
// Inter-sample time must be at least 5ms
// One power line cycle is 20ms (in most countries) or 16.66..ms (like in that country where they still use feet to mesure things)
// So in order to average on alternating power line polarity, I need:
//  1) to space the measurements on 1.5 cycles: 30ms or 25ms.
//  2) to always have an even number of measurements.

// I could do the timing via interrupts, but via millis() it is easier and more portable.

// The number of full DCIS samples for Power Line Frequency Noise Rejection (PLFNR). Must be even.
#define DCIS_PLFNR_NR_SAMPLES 10
// the sample spacing when doing PLFNR, is to be adapted to your line frequency. 
// Must be N + 1/2 times power line cycle period. 
// 50Hz is easy: each increment is 20ms. But the 60Hz period is not an integer, so I must do 50ms steps there 
// N=1, 50Hz: 30, and 60Hz: 25
// N=2, 50Hz: 50, and 60Hz: 41,66
// N=3, 50Hz: 70, and 60Hz: 58,33
// N=4, 50Hz: 90, and 60Hz: 75
// .....
#define DCIS_PLFNR_SAMPLE_SPACING_MS_50HZ 30
#define DCIS_PLFNR_SAMPLE_SPACING_MS_60HZ 25
#define DCIS_PLFNR_SAMPLE_SPACING_STEP_MS_50HZ 20
#define DCIS_PLFNR_SAMPLE_SPACING_STEP_MS_60HZ 50

/**
 * @brief raw ADC values associated to 1 voltage drop measurement
 */
struct vDrop {
    int32_t sRefBefore;
    int32_t sRefAfter;
    int32_t sTestBefore;
    int32_t sTestAfter;
};

/**
 * @brief Results from differential measurements for DCIS sampling
 */
struct dcisSampleResults {
    double vDCInput;                        /**< DC input voltage before test */
    double rB;                              /**< Rbulk value, scaled */
    double rSEI;                            /**< RSEI value, scaled */
    uint8_t rangeUsed;                      /**< The range used for the results. May be different that the present range! */
    uint8_t nrSamples;                      /**< The number of samples in vDrop (0..DCIS_PLFNR_NR_SAMPLES) */
    struct vDrop t1[DCIS_PLFNR_NR_SAMPLES]; /**< voltage drop data for t1 */
    struct vDrop t2[DCIS_PLFNR_NR_SAMPLES]; /**< voltage drop data for t2 */
    struct adcShortResults shortRef;        /**< Results of the short on Ref */
    struct adcShortResults shortTest;       /**< Results of the short on Test */
    uint8_t resultFlags;                    /**< 0 = OK. For bitmap, see bm_adcDiffSampleResults_... values */
};

// Flags for runDCISSample:
// measure the T1 period
#define bm_runDCISSample_t1 0x01
// measure the T2 period
#define bm_runDCISSample_t2 0x02
// With Power Line Frequency Noise Rejection
#define bm_runDCISSample_PLFNR 0x04
// short input and ref before the test. This does take some time, via setShortWithDuration
#define bm_runDCISSample_ShortBefore 0x20
// 1 sec long DCIS signal out, no measurement, amp is connected to ref during the run, preceded by a regular short, but no abandon if input voltage is low
#define bm_runDCISSample_longTest 0x40
// 1 sec long DCIS signal out, no measurement, amp is connected to ref during the run, preceded by a regular short, but no abandon if input voltage is low
#define bm_runDCISSample_longRef 0x80

// nothing set = default set.
#define bm_runDCISSample_Default 0x27

// ADC sample duration, in microseconds. This is a full sample, not a sped up sample like in AC
#define DCIS_ADCSAMPLE_DURATION_USEC 80

// the number of milliseconds between starts of DCIS samples. See DCIS_PLFNR_SAMPLE_SPACING_MS...
uint16_t dcisSampleSpacingMs = DCIS_PLFNR_SAMPLE_SPACING_MS_50HZ;

/**
 * @brief Valudate the settings for DCIS and calculate dcisSampleSpacingMs
 * 
 */
void dcisValidateSettings(void) {
    uint16_t f = DCIS_PLFNR_SAMPLE_SPACING_MS_50HZ;
    if (!mySettings.iPLF_50Hz)
        f = DCIS_PLFNR_SAMPLE_SPACING_MS_60HZ;

    if (mySettings.iT1_usecs < DCIS_T1_usecs_MIN)
        mySettings.iT1_usecs = DCIS_T1_usecs_MIN;
    if (mySettings.iT1_usecs > DCIS_T1_usecs_MAX)
        mySettings.iT1_usecs = DCIS_T1_usecs_MAX;

    if (mySettings.iT2_usecs < DCIS_T2_usecs_MIN)
        mySettings.iT2_usecs = DCIS_T2_usecs_MIN;
    if (mySettings.iT2_usecs > DCIS_T2_usecs_MAX)
        mySettings.iT2_usecs = DCIS_T2_usecs_MAX;

    if (mySettings.iTPause_usecs < DCIS_TP_usecs_MIN)
        mySettings.iTPause_usecs = DCIS_TP_usecs_MIN;
    if (mySettings.iTPause_usecs > DCIS_TP_usecs_MAX)
        mySettings.iTPause_usecs = DCIS_TP_usecs_MAX;

    // the spacing betwqeen starts of samples must be N + 1/2 times the power line frequency
    uint16_t sample_duration_ms = ((unsigned long)mySettings.iT1_usecs + (unsigned long)mySettings.iT2_usecs + (unsigned long)mySettings.iTPause_usecs + (unsigned long)DCIS_TP_usecs_MIN) / 1000;
    while (sample_duration_ms > f) {
        if (mySettings.iPLF_50Hz) {
            f += DCIS_PLFNR_SAMPLE_SPACING_STEP_MS_50HZ;
        } else {
            f += DCIS_PLFNR_SAMPLE_SPACING_STEP_MS_60HZ;
        }
    }
    dcisSampleSpacingMs = f;
}

/**
 * @brief Do the Voltage Drop measurement for a given time.
 * Fast ADC mode must have been set. But timers must still be available.
 *
 * @param durationuSec The duration of the drop test in uSecs
 * @param pV struct vDrop, output
 * @param measureFlags 0 or bm_runDCISSample_longRef or bm_runDCISSample_longTest
 */
void getVoltageDrop(unsigned long durationuSec, struct vDrop *pV, uint8_t measureFlags) {
    if (!pV)
        return;
    pV->sRefBefore = 0;
    pV->sRefAfter = 0;
    pV->sTestBefore = 0;
    pV->sTestAfter = 0;

    bool readRef = true;
    bool readInput = true;
    bool longTest = ((measureFlags & bm_runDCISSample_longRef) || (measureFlags & bm_runDCISSample_longTest));
    if (longTest) {
        readRef = (measureFlags & bm_runDCISSample_longRef);
        readInput = (measureFlags & bm_runDCISSample_longTest);
    } else {
        // validate input param
        if (durationuSec <= (2 * DCIS_ADCSAMPLE_DURATION_USEC))
            return;
    }

    // switch off the short before reading the input.
    setShortOff();

    // init output signal
    digitalWriteFast(SIGNAL_OUT, LOW);
    setTestModeDCIS();  // force 0% current

    // do a test ADC read, throw away
    digitalWriteFast(SIGNAL_OUT, LOW);

    setADCInMode(adcin_ref);

    // Throw the first away
    analogReadDiff(ADC_INP, ADC_INN, ADC_NATIVE_RESOLUTION, 0);  // is about DCIS_ADCSAMPLE_DURATION_USEC

    if (readRef) {
        setADCInMode(adcin_ref);
        pV->sRefBefore = analogReadDiff(ADC_INP, ADC_INN, ADC_NATIVE_RESOLUTION, 0);
    }
    if (readInput) {
        setADCInMode(adcin_input);
        pV->sTestBefore = analogReadDiff(ADC_INP, ADC_INN, ADC_NATIVE_RESOLUTION, 0);
    }

    // setADCInMode(adcin_ref);

    digitalWriteFast(SIGNAL_OUT, HIGH);
    if (longTest) {
        // 1 sec, 1.000 msecs
        delay(1000L);
    } else {
        if (durationuSec > 50000L)
            delay(durationuSec / 1000L);
        else
            delayMicroseconds(durationuSec - (2 * DCIS_ADCSAMPLE_DURATION_USEC));
    }

    if (readRef) {
        setADCInMode(adcin_ref);
        pV->sRefAfter = analogReadDiff(ADC_INP, ADC_INN, ADC_NATIVE_RESOLUTION, 0);
    }
    if (readInput) {
        setADCInMode(adcin_input);
        pV->sTestAfter = analogReadDiff(ADC_INP, ADC_INN, ADC_NATIVE_RESOLUTION, 0);
    }
    digitalWriteFast(SIGNAL_OUT, LOW);
    setADCInMode(adcin_ref);
}

/**
 * @brief The main DCIS test function.
 * Measures the Rbulk and RSEI values via a ratiometric approach
 * By default, it does the following:
 *  1) read the DC voltage
 *  2) short the inputs for the diff amp, while loading the test subject at 0% of the test current. This way the input caps adapt to the DC voltage.
 *  3) do a voltage drop test for 0.25ms
 *  4) pause 5ms
 *  5) do a voltage drop test for 10msec
 *  6) calculate the Rbulk and RSEI values
 *
 * for Power Line Frequency Noise Rejection, it repeats this all a couple of times at precisely timed moments.
 *
 * A voltage drop test is done as follows:
 *  1) set current load at 0
 *  2) measure the voltage over the Reference resistor
 *  3) measure the voltage over the test subject
 *  4) apply the load current
 *  5) wait the designated time minus the ADC sampling times that will follow:
 *  6) measure the voltage over the Reference resistor
 *  7) measure the voltage over the test subject
 *  8) set current load at 0
 *
 * Repeated calls to the function must be spaced a bit (ideally > 250ms) in order to let the test subject settle.
 * @param pResults The struct that holds the detailed results. See struct dcisSampleResults
 * @param offset The offset to be applied to the raw diff values
 * @param scale The scale to be applied to the raw diff values
 * @param upperlimit The upper limit of the returned values before overrange is to be signalled. 
 * @param underlimit The lower limit of the returned values before underrange is to be signalled. 
* @param measureFlags see bm_runDCIS..., equivalent to runACSample
 * @return Rbulk, in Ohm
 */
double runDCISSample(struct dcisSampleResults *pResults, double offset, double scale, float upperlimit, float underlimit, uint8_t measureFlags) {
    struct vDrop t1[DCIS_PLFNR_NR_SAMPLES];
    struct vDrop t2[DCIS_PLFNR_NR_SAMPLES];
    bool lowVoltage = false;

    dcisValidateSettings();

    memset(&t1, 0, sizeof(t1));
    memset(&t2, 0, sizeof(t2));

    if (pResults) {
        pResults->rB = 0.0;
        pResults->rSEI = 0.0;
        pResults->vDCInput = 0.0;
        pResults->rangeUsed = getIRange();
        pResults->nrSamples = 0;
        memset(&(pResults->t1), 0, sizeof(pResults->t1));
        memset(&(pResults->t2), 0, sizeof(pResults->t2));
        memset(&pResults->shortRef, 0, sizeof(pResults->shortRef));
        memset(&pResults->shortTest, 0, sizeof(pResults->shortTest));
        pResults->resultFlags = 0;
    }

    // default measureFlags
    if (measureFlags == 0)
        measureFlags = bm_runDCISSample_Default;

    // if long DCIS, everything else is off.
    bool longTest = ((measureFlags & bm_runDCISSample_longRef) || (measureFlags & bm_runDCISSample_longTest));
    // clean up input flags if needed
    if (measureFlags & bm_runDCISSample_longRef)
        measureFlags = bm_runDCISSample_longRef | bm_runDCISSample_ShortBefore;
    if (measureFlags & bm_runDCISSample_longTest)
        measureFlags = bm_runDCISSample_longTest | bm_runDCISSample_ShortBefore;

    // Remember the input voltage
    double vDCInput = getExternalVoltage();
    if (!haveEnoughVoltageToTest(vDCInput)) {
        lowVoltage = true;
        // and cancel the test, unless I am doing a long test
        if (!longTest)
            measureFlags = 0;
    }

    // Short if wanted
    if (measureFlags & bm_runDCISSample_ShortBefore) {
        struct adcShortResults *p;
        int r1, r2;
        p = NULL;
        if (pResults)
            p = &(pResults->shortRef);
        r1 = setShortWithDuration(false, false, p);
        if (pResults)
            p = &(pResults->shortTest);
        r2 = setShortWithDuration(true, false, p);

        // any short problems? If so, cancel the test
        if ((r1 < 0) || (r2 < 0)) {
            // cancel the test, unless I am doing a long test
            if (!longTest)
                measureFlags = 0;
        }
    }

    // reset interrupt flags. This is very probably not needed here, as I do not force ADC sampling via clock.
    had_ADC_ERR = 0;

    // Set up ADC for fast reading
    setup_ADC(true);

    int nrRuns = 0;

    if ((measureFlags & bm_runDCISSample_longRef) || (measureFlags & bm_runDCISSample_longTest)) {
        // This is for the long tests. The above code is equivalent to "if (longTest)", but allows cancelling halfway
        nrRuns = 1;
        // 1 sec DCIS signal on the chosen input
        getVoltageDrop(0, &t1[0], measureFlags);
    } else {
        if (measureFlags) {
            nrRuns = DCIS_PLFNR_NR_SAMPLES;
            if (!(measureFlags & bm_runDCISSample_PLFNR))
                nrRuns = 1;
            unsigned long start = millis();

            int i = nrRuns;
            while (i > 0) {
                i--;
                // Sample
                if (measureFlags & bm_runDCISSample_t1) {
                    getVoltageDrop(mySettings.iT1_usecs, &t1[i], 0);
                    delayMicroseconds(mySettings.iTPause_usecs);
                }
                if (measureFlags & bm_runDCISSample_t2) {
                    getVoltageDrop(mySettings.iT2_usecs, &t2[i], 0);
                }
                // interval handling:
                if (i > 0) {
                    // sleep until start + DCIS_PLFNR_SAMPLE_SPACING_MS_x0Hz
                    while (millis() - start < dcisSampleSpacingMs)
                        ;  // this takes overflow into account.
                    start += dcisSampleSpacingMs;
                }
            }
        }
    }

    int32_t T1amplR = 0;
    int32_t T1amplT = 0;
    int32_t T2amplR = 0;
    int32_t T2amplT = 0;
    for (int i = 0; i < nrRuns; i++) {
        T1amplR += (t1[i].sRefAfter - t1[i].sRefBefore);
        T1amplT += (t1[i].sTestBefore - t1[i].sTestAfter);  // yes, reversed.
        T2amplR += (t2[i].sRefAfter - t2[i].sRefBefore);
        T2amplT += (t2[i].sTestBefore - t2[i].sTestAfter);  // yes, reversed.
    }
    if (nrRuns) {
        T1amplR = T1amplR / nrRuns;
        T1amplT = T1amplT / nrRuns;
        T2amplR = T2amplR / nrRuns;
        T2amplT = T2amplT / nrRuns;
    }

    double T1R = 0.0;
    double T2R = 0.0;
    if (T1amplR != 0) {
        T1R = offset + (scale * ((double)T1amplT / (double)T1amplR));
    }
    if (T2amplR != 0) {
        T2R = offset + (scale * ((double)T2amplT / (double)T2amplR));
    }

    // Serial.printf("\n>>> T1R=%f;T2R=%f;T1amplR=%ld;T1amplT=%ld;T2amplR=%ld;T2amplT=%ld;offset=%f;scale=%f\n",T1R,T2R,T1amplR,T1amplT,T2amplR,T2amplT,offset,scale);

    if (pResults) {
        pResults->rB = T1R;
        if (T2R > T1R)
            pResults->rSEI = T2R - T1R;
        else
            pResults->rSEI = 0.0;
        pResults->vDCInput = vDCInput;
        pResults->rangeUsed = getIRange();
        pResults->nrSamples = nrRuns;
        memcpy(&pResults->t1, &t1, sizeof(pResults->t1));
        memcpy(&pResults->t2, &t2, sizeof(pResults->t2));

        bool bBeforeOverflow = false;
        bool bRefOverflow = false;
        bool bTestOverflow = false;

        for (int i = 0; i < nrRuns; i++) {
            if ((t1[i].sRefBefore <= MIN_12Bits) || (t1[i].sRefBefore >= MAX_12Bits) ||
                (t2[i].sRefBefore <= MIN_12Bits) || (t2[i].sRefBefore >= MAX_12Bits) ||
                (t1[i].sTestBefore <= MIN_12Bits) || (t1[i].sTestBefore >= MAX_12Bits) ||
                (t2[i].sTestBefore <= MIN_12Bits) || (t2[i].sTestBefore >= MAX_12Bits))
                bBeforeOverflow = true;
            if ((t1[i].sRefAfter <= MIN_12Bits) || (t1[i].sRefAfter >= MAX_12Bits) ||
                (t2[i].sRefAfter <= MIN_12Bits) || (t2[i].sRefAfter >= MAX_12Bits))
                bRefOverflow = true;
            if ((t1[i].sTestAfter <= MIN_12Bits) || (t1[i].sTestAfter >= MAX_12Bits) ||
                (t2[i].sTestAfter <= MIN_12Bits) || (t2[i].sTestAfter >= MAX_12Bits))
                bTestOverflow = true;
        }

        // determine resultFlags (0 by default)
        if (lowVoltage)
            pResults->resultFlags |= bm_adcDiffSampleResults_undervoltage;

        if ((pResults->shortRef.duration < 0) || (pResults->shortTest.duration < 0) || bBeforeOverflow)
            pResults->resultFlags |= bm_adcDiffSampleResults_shortproblem;

        if (had_ADC_ERR & 1)
            pResults->resultFlags |= bm_adcDiffSampleResults_overrun;
        if (had_ADC_ERR & 2)
            pResults->resultFlags |= bm_adcDiffSampleResults_timeout;

        if (bRefOverflow || bTestOverflow || bBeforeOverflow)
            pResults->resultFlags |= bm_adcDiffSampleResults_overload;

        // TODO detect amplR wrong values
        // Should be at about half max range

        // determine range issues. But ignore ref overflow or lowVoltage here, as anything can happen then
        if (!bRefOverflow && !lowVoltage) {
            if (bTestOverflow)
                pResults->resultFlags |= bm_adcDiffSampleResults_overrange;
            else {
                // any of the 2 values can be in overflow: go up
                if ((pResults->rB >= upperlimit) || (pResults->rSEI >= upperlimit))
                    pResults->resultFlags |= bm_adcDiffSampleResults_overrange;

                // when both are in underflow, go down
                if ((pResults->rB < underlimit) && (pResults->rSEI < underlimit))
                    pResults->resultFlags |= bm_adcDiffSampleResults_underrange;
            }
        }
    }

    return T1R;
}

/**
 * @brief Get test device DCIS Impedances
 * When autoranging is requested, I do one measurement,
 * determine if I'm out of range, adapt the range for the next measurement,
 * but still present the results. That way, the UI is more reactive.
 *
 * @param pResults See runDCISSample
 * @param measureFlags See runDCISSample
 * @param autoRange if true, do autorange
 * @return Rbulk, in Ohm. Use the pResults to find out any error conditions
 */
double getDCISInfo(struct dcisSampleResults *pResults, uint8_t measureFlags, bool autoRange) {
    double offset = 0;
    uint8_t range = getIRange();
    double scale = settingsGetRatio(range);
    float upperlimit = getRUpperLimit(range);
    float underlimit = getRUnderLimit(range);

    double v = runDCISSample(pResults, offset, scale, upperlimit, underlimit, measureFlags);
    if (autoRange) {
        if (pResults) {
            if ((pResults->resultFlags & bm_adcDiffSampleResults_overrange) ||
                (pResults->resultFlags & bm_adcDiffSampleResults_overload) ||
                (pResults->resultFlags & bm_adcDiffSampleResults_shortproblem))
                setIRangeDown();
            else if (pResults->resultFlags & bm_adcDiffSampleResults_underrange)
                setIRangeUp();
        }
    }
    return v;
}

#pragma endregion

#pragma region DC sampler
// ************************************************************************************************
// DC Sampler
// ************************************************************************************************

// The main functions from this:
double getInternalBatteryVoltage(void);
int getInternalBatteryPercentage(void);
double getExternalVoltage(void);

// TODO check for errors: < 0

/**
 * @brief run a DC sample
 * Expects the DC input mux to be set already.
 * @param input the ADC input to be used. See enum ADCin
 * @param offset The offset to be applied to the raw ADC value
 * @param scale The scale to be applied to the raw ADC value
 * @return The DC value, in Volts
 */
double runDCSample(uint8_t input, int offset, double scale) {
    int32_t rawv = 0;
// ADC nr of bits
#define ADC_RESOLUTION 16
// ADC_RESOLUTION bits, unsigned
#define adc_max_val_single (1UL << ADC_RESOLUTION)

// Lower value read from the ADC, below which it is no longer linear. 
#define ADC_LOWERLIMIT 200

    // set multiplexer
    setADCInMode(input);

    setup_ADC(false);
    // first reading is always off, because the cap at the input is too big. Just read and ignore.
    rawv = analogReadEnh(ADC_DC, ADC_RESOLUTION, 0);  // Throw reading away

    // Then read correctly
    rawv = analogReadEnh(ADC_DC, ADC_RESOLUTION, 0);

    // debug statements that allow raw value to be exposed for calibration:
    // But note that the ADC is not highly linear. Best to measure several values and define the offset and scale via fitting.

    //Serial.printf("2: ADC_MAX_SCALE_MV: %d, rawv: %ld; offset:%d, scale: %f, adc_max_val_single: %ld ",ADC_MAX_SCALE_MV,(long)rawv,offset,(float)scale,(long)adc_max_val_single);
    //if (scale > 2.0) Serial.printf("rawv: %ld ",rawv);
    
    if (rawv < ADC_LOWERLIMIT) return 0.0;
    return (float)ADC_MAX_SCALE_MV * (scale / 1000.0) * (float)(rawv - offset) / adc_max_val_single;
}

// read Battery Voltage (in Volts)
double getInternalBatteryVoltage(void) {
    int input = adcin_batt;
    // set scales properly
    int offset;
    double scale = settingsGetDCScale(input, &offset);
    // read ADC
    return runDCSample(input, offset, scale);
}

// read Battery Voltage (in percentage)
int getInternalBatteryPercentage(void) {
    double v = getInternalBatteryVoltage();
    int p = settingsTranslateBattVoltageToPercentage(v);
    // Serial.printf("BattV:%.2f => %d%%; ",v,p);
    return p;
}

// read Input Voltage (in Volts)
double getExternalVoltage(void) {
    int input = adcin_input;
    // set scales properly
    int offset;
    double scale = settingsGetDCScale(input, &offset);
    // read ADC
    return runDCSample(input, offset, scale);
}

#pragma endregion

#pragma region ioExpander

PCA9536 ioExpander;
bool ioExpanderOK;

void setup_ioexpander(void) {
    // Initialize the PCA9536 with a begin function
    ioExpanderOK = false;
    if (ioExpander.begin() == false) {
        Serial.println("PCA9536 not detected.");
        // TODO: I should reboot
    } else {
        ioExpanderOK = true;
        for (int i = 0; i < 3; i++) {
            // pinMode can be used to set an I/O as OUTPUT or INPUT
            ioExpander.pinMode(i, INPUT);
        }
        ioExpander.pinMode(3, OUTPUT);
    }
}

// The buttons.
enum btns : uint8_t { btn_none = 0,
                      btn_up,
                      btn_down,
                      btn_press,
                      btn_error
};

/**
 * @brief Get any Buttons pressed
 *
 * @return see enum btns
 */
uint8_t getButtons(void) {
    // 0 = pressed
    if (!ioExpanderOK) return btn_error;
    if (!ioExpander.digitalRead(0)) return btn_up;
    if (!ioExpander.digitalRead(1)) return btn_down;
    if (!ioExpander.digitalRead(2)) return btn_press;
    return btn_none;
}

/**
 * @brief control the led (only on HW 1.1+)
 *
 * @param state 1 = on
 */
void setLed(bool state) {
    if (!ioExpanderOK) return;
    ioExpander.digitalWrite(3, !state);
}

#pragma endregion

#pragma region Screen
#ifndef NO_DISPLAY
// ************************************************************************************************
// ********************* Screen *****************
// ************************************************************************************************
#include <Tiny4kOLED.h>

// great tools for testing: 
//  https://wokwi.com/projects/319464160996885075
//  https://github.com/jdmorise/TTF2BMH / https://github.com/hb020/TTF2BMH 
//  https://github.com/datacute/Tiny4kOLED

#ifdef OLED_128x32
// The display I use is a 128x32: JMD 0.91A, or from other suppliers
#define screen_width 128
#define screen_height 32

#define ssd1306_init_sequence tiny4koled_init_128x32r
#endif

#if 0

// The code to move a font 1 pixel down is here:
//int main() {
//    int i,j;
//    unsigned char ch1,ch2;
//    unsigned char out[sizeof(ssd1306xled_font11x16)];
//    memset(&out[0],0,sizeof(out));
//    int nr = sizeof(ssd1306xled_font11x16) / 22;
//    for (i = 0; i < nr; i++) {
//        for (j = 0; j < 11; j++) {
//            ch1 = ssd1306xled_font11x16[(i*22) + j];
//            ch2 = ssd1306xled_font11x16[(i*22) + j + 11];
//            ch2 = ch2 << 1;
//            if (ch1 & 0x80) ch2 |= 1;
//            ch1 = ch1 << 1;
//            out[(i*22) + j] = ch1;
//            out[(i*22) + j + 11] = ch2;
//        }
//    }
//    for (i = 0; i < nr; i++) {
//        for (j = 0; j < 22; j++) {
//            printf("0x%02x, ",out[(i*22) + j]);
//        }
//        printf(" // char %c\n",i + ' ');
//    }
//    return 0;
//}

#endif

// Noto Mono font, Free for personal and for commercial use.
// Noto Mono is licensed under the SIL Open Font License (OFL)
// https://www.1001fonts.com/noto-mono-font.html
// Generated with TTF2BMH, with arguments src/ttf2bmh.py -s 16 -fh 18 -O -14 -a baseline --ascii --variable_width -p -T -f ~/Library/Fonts/
// Font Noto Mono
// Font Size: 16 * variable
// Adaptations:
//  - ^ replaced by Ohm sign
//  - made space, and all digits the same width
//  - improved rendering of 0,1,2,3,8,9,R
//  - added extra spacing around the . (but still a LOT less than monospaced)
//  - inter-character spacing 2
//

const uint8_t ssd1306xled_fontNoto_Mono_16xvariable[] = {
    0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,
    0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,  // char
    0b11111110, 0b11111110,
    0b00110000, 0b00110011,  // char !
    0b00011110, 0b00011110, 0b00000000, 0b00000000, 0b00011110, 0b00011110,
    0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,  // char "
    0b00000000, 0b00010000, 0b11010000, 0b11111100, 0b00010110, 0b00010000, 0b00010000, 0b11010000, 0b11111100, 0b00010110, 0b00010000,
    0b00000010, 0b00110010, 0b00011111, 0b00000010, 0b00000010, 0b00000010, 0b00110010, 0b00011111, 0b00000010, 0b00000010, 0b00000000,  // char #
    0b00111000, 0b01111000, 0b11001100, 0b11000100, 0b11111111, 0b10000100, 0b10000100, 0b00001100, 0b00000000,
    0b00011000, 0b00011000, 0b00010000, 0b00010000, 0b01111111, 0b00010000, 0b00011001, 0b00001111, 0b00001110,  // char $
    0b00111100, 0b01100110, 0b01000010, 0b01100110, 0b00111100, 0b11000000, 0b01110000, 0b00001110, 0b00000010, 0b00000000, 0b00000000,
    0b00000000, 0b00000000, 0b00100000, 0b00111000, 0b00000111, 0b00000001, 0b00011110, 0b00110011, 0b00100001, 0b00110011, 0b00011110,  // char %
    0b00000000, 0b00111100, 0b11111110, 0b11000010, 0b11000010, 0b01111110, 0b00011100, 0b00000000, 0b00000000, 0b00000000,
    0b00011110, 0b00111111, 0b00110001, 0b00100000, 0b00100011, 0b00110111, 0b00011100, 0b00011111, 0b00110011, 0b00100000,  // char &
    0b00011110, 0b00011110,
    0b00000000, 0b00000000,  // char '
    0b11000000, 0b11111000, 0b00011100, 0b00000110, 0b00000010,
    0b00011111, 0b01111111, 0b11100000, 0b10000000, 0b00000000,  // char (
    0b00000010, 0b00000110, 0b00011100, 0b11111000, 0b11000000,
    0b00000000, 0b10000000, 0b11100000, 0b01111111, 0b00011111,  // char )
    0b00001000, 0b00001000, 0b11001000, 0b01101000, 0b00011111, 0b01111000, 0b11001000, 0b00001000, 0b00001100,
    0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,  // char *
    0b01000000, 0b01000000, 0b01000000, 0b11111000, 0b01000000, 0b01000000, 0b01000000, 0b01000000,
    0b00000000, 0b00000000, 0b00000000, 0b00000111, 0b00000000, 0b00000000, 0b00000000, 0b00000000,  // char +
    0b00000000, 0b00000000, 0b00000000,
    0b11000000, 0b11110000, 0b00110000,  // char ,
    0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,
    0b00000001, 0b00000001, 0b00000001, 0b00000001, 0b00000001,  // char -
    0b00000000, 0b00000000, 0b00000000, 0b00000000,
    0b00000000, 0b00110000, 0b00110000, 0b00000000,  // char .
    0b00000000, 0b00000000, 0b00000000, 0b11000000, 0b01111000, 0b00011110, 0b00000010,
    0b00100000, 0b00111100, 0b00001111, 0b00000001, 0b00000000, 0b00000000, 0b00000000,  // char /
    0b11110000, 0b11111100, 0b00000110, 0b00000010, 0b00000010, 0b00000010, 0b00000110, 0b11111100, 0b11110000,
    0b00000111, 0b00011111, 0b00110000, 0b00100000, 0b00100000, 0b00100000, 0b00110000, 0b00011111, 0b00000111,  // char 0
    0b00000000, 0b00000000, 0b00010000, 0b00001000, 0b00000100, 0b11111110, 0b11111110, 0b00000000, 0b00000000,
    0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00111111, 0b00111111, 0b00000000, 0b00000000,  // char 1
    0b00000100, 0b00000100, 0b00000110, 0b00000010, 0b00000010, 0b00000010, 0b10000110, 0b11111100, 0b00111000,
    0b00110000, 0b00111000, 0b00101000, 0b00101100, 0b00100110, 0b00100011, 0b00100001, 0b00100000, 0b00100000,  // char 2
    0b00000100, 0b00000100, 0b10000110, 0b10000010, 0b10000010, 0b10000010, 0b01000110, 0b01111100, 0b00111000,
    0b00010000, 0b00110000, 0b00100000, 0b00100000, 0b00100000, 0b00110001, 0b00110001, 0b00011111, 0b00001110,  // char 3
    0b00000000, 0b00000000, 0b11000000, 0b01100000, 0b00110000, 0b00011100, 0b11111110, 0b11111110, 0b00000000,
    0b00000110, 0b00000111, 0b00000101, 0b00000100, 0b00000100, 0b00000100, 0b00111111, 0b00111111, 0b00000100,  // char 4
    0b01111110, 0b01111110, 0b01000010, 0b01000010, 0b01000010, 0b11000010, 0b11000010, 0b10000010, 0b00000000,
    0b00110000, 0b00110000, 0b00100000, 0b00100000, 0b00100000, 0b00110000, 0b00110000, 0b00011111, 0b00001111,  // char 5
    0b11100000, 0b11111000, 0b10001100, 0b01000110, 0b01000110, 0b01000010, 0b11000010, 0b10000010, 0b00000000,
    0b00000111, 0b00011111, 0b00110000, 0b00110000, 0b00100000, 0b00100000, 0b00110000, 0b00011111, 0b00001111,  // char 6
    0b00000010, 0b00000010, 0b00000010, 0b00000010, 0b00000010, 0b11000010, 0b11110010, 0b00011110, 0b00000110,
    0b00000000, 0b00000000, 0b00100000, 0b00111000, 0b00001110, 0b00000011, 0b00000000, 0b00000000, 0b00000000,  // char 7
    0b00011000, 0b00111100, 0b11000110, 0b11000010, 0b11000010, 0b11000010, 0b01000110, 0b00111100, 0b00011000,
    0b00001110, 0b00011111, 0b00110001, 0b00100000, 0b00100000, 0b00100000, 0b00110001, 0b00011111, 0b00001110,  // char 8
    0b01111000, 0b11111100, 0b10000110, 0b00000010, 0b00000010, 0b00000010, 0b10000110, 0b11111100, 0b11110000,
    0b00000000, 0b00100000, 0b00100001, 0b00100001, 0b00110001, 0b00110001, 0b00011000, 0b00001111, 0b00000011,  // char 9
    0b00110000, 0b00110000,
    0b00110000, 0b00110000,  // char :
    0b00000000, 0b00110000, 0b00110000,
    0b11000000, 0b11110000, 0b00110000,  // char ;
    0b10000000, 0b11000000, 0b01000000, 0b01100000, 0b00100000, 0b00100000, 0b00110000, 0b00010000, 0b00011000,
    0b00000000, 0b00000001, 0b00000001, 0b00000011, 0b00000010, 0b00000010, 0b00000110, 0b00000100, 0b00001100,  // char <
    0b00100000, 0b00100000, 0b00100000, 0b00100000, 0b00100000, 0b00100000, 0b00100000, 0b00100000, 0b00100000,
    0b00000010, 0b00000010, 0b00000010, 0b00000010, 0b00000010, 0b00000010, 0b00000010, 0b00000010, 0b00000010,  // char =
    0b00011000, 0b00010000, 0b00110000, 0b00100000, 0b00100000, 0b01100000, 0b01000000, 0b11000000, 0b10000000,
    0b00001100, 0b00000100, 0b00000110, 0b00000010, 0b00000010, 0b00000011, 0b00000001, 0b00000001, 0b00000000,  // char >
    0b00000100, 0b00000110, 0b00000010, 0b10000010, 0b10000010, 0b01000110, 0b01111100, 0b00111100,
    0b00000000, 0b00000000, 0b00110010, 0b00110011, 0b00000000, 0b00000000, 0b00000000, 0b00000000,  // char ?
    0b11100000, 0b00111000, 0b00001100, 0b10000110, 0b01100010, 0b00100010, 0b00100010, 0b11100110, 0b00000100, 0b00011000, 0b11110000,
    0b00001111, 0b00111000, 0b01100000, 0b11000111, 0b10001100, 0b10001000, 0b10001100, 0b10000111, 0b11001000, 0b00001100, 0b00000111,  // char @
    0b00000000, 0b00000000, 0b10000000, 0b11110000, 0b00111110, 0b00000110, 0b00111110, 0b11110000, 0b10000000, 0b00000000, 0b00000000,
    0b00100000, 0b00111100, 0b00001111, 0b00000011, 0b00000010, 0b00000010, 0b00000010, 0b00000011, 0b00001111, 0b00111100, 0b00100000,  // char A
    0b11111110, 0b11111110, 0b01000010, 0b01000010, 0b01000010, 0b11000010, 0b11000110, 0b10111100, 0b00111100,
    0b00111111, 0b00111111, 0b00100000, 0b00100000, 0b00100000, 0b00110000, 0b00110000, 0b00011111, 0b00001111,  // char B
    0b11110000, 0b11111000, 0b00001100, 0b00000110, 0b00000110, 0b00000010, 0b00000010, 0b00000110, 0b00000100,
    0b00000111, 0b00001111, 0b00011000, 0b00110000, 0b00110000, 0b00100000, 0b00100000, 0b00110000, 0b00110000,  // char C
    0b11111110, 0b11111110, 0b00000010, 0b00000010, 0b00000110, 0b00000110, 0b00001100, 0b11111000, 0b11110000,
    0b00111111, 0b00111111, 0b00100000, 0b00100000, 0b00110000, 0b00110000, 0b00011000, 0b00001111, 0b00000011,  // char D
    0b11111110, 0b11111110, 0b01000010, 0b01000010, 0b01000010, 0b01000010, 0b01000010, 0b01000010,
    0b00111111, 0b00111111, 0b00100000, 0b00100000, 0b00100000, 0b00100000, 0b00100000, 0b00100000,  // char E
    0b11111110, 0b11111110, 0b01000010, 0b01000010, 0b01000010, 0b01000010, 0b01000010, 0b01000010,
    0b00111111, 0b00111111, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,  // char F
    0b11110000, 0b11111000, 0b00001100, 0b00000110, 0b00000110, 0b01000010, 0b01000110, 0b11000110, 0b11000000,
    0b00000111, 0b00001111, 0b00011000, 0b00110000, 0b00110000, 0b00100000, 0b00110000, 0b00111111, 0b00011111,  // char G
    0b11111110, 0b11111110, 0b01000000, 0b01000000, 0b01000000, 0b01000000, 0b01000000, 0b11111110, 0b11111110,
    0b00111111, 0b00111111, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00111111, 0b00111111,  // char H
    0b00000010, 0b00000010, 0b11111110, 0b11111110, 0b00000010, 0b00000010, 0b00000010,
    0b00100000, 0b00100000, 0b00111111, 0b00111111, 0b00100000, 0b00100000, 0b00100000,  // char I
    0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b11111110, 0b11111110,
    0b00110000, 0b00110000, 0b00100000, 0b00100000, 0b00110000, 0b00110000, 0b00011111, 0b00001111,  // char J
    0b11111110, 0b11111110, 0b11000000, 0b11100000, 0b00110000, 0b00011000, 0b00001100, 0b00000110, 0b00000010,
    0b00111111, 0b00111111, 0b00000000, 0b00000000, 0b00000011, 0b00000110, 0b00011100, 0b00110000, 0b00100000,  // char K
    0b11111110, 0b11111110, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,
    0b00111111, 0b00111111, 0b00100000, 0b00100000, 0b00100000, 0b00100000, 0b00100000, 0b00100000,  // char L
    0b11111110, 0b11111110, 0b11110000, 0b00000000, 0b00000000, 0b10000000, 0b01110000, 0b11111110, 0b11111110,
    0b00111111, 0b00111111, 0b00000000, 0b00001111, 0b00110000, 0b00001111, 0b00000000, 0b00111111, 0b00111111,  // char M
    0b11111110, 0b11000110, 0b00011000, 0b01110000, 0b11000000, 0b00000000, 0b00000000, 0b11111110, 0b11111110,
    0b00111111, 0b00111111, 0b00000000, 0b00000000, 0b00000001, 0b00000111, 0b00001100, 0b00110001, 0b00111111,  // char N
    0b11110000, 0b11111100, 0b00001110, 0b00000110, 0b00000010, 0b00000110, 0b00001110, 0b11111100, 0b11110000,
    0b00000111, 0b00011111, 0b00111000, 0b00110000, 0b00100000, 0b00110000, 0b00111000, 0b00011111, 0b00000111,  // char O
    0b11111110, 0b11111110, 0b10000010, 0b10000010, 0b10000010, 0b10000110, 0b11000110, 0b01111100, 0b00111000,
    0b00111111, 0b00111111, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,  // char P
    0b11110000, 0b11111100, 0b00001110, 0b00000110, 0b00000010, 0b00000110, 0b00001110, 0b11111100, 0b11110000,
    0b00000111, 0b00011111, 0b00111000, 0b00110000, 0b00100000, 0b01110000, 0b11111000, 0b10011111, 0b10000111,  // char Q
    0b11111110, 0b11111110, 0b10000110, 0b10000110, 0b10000110, 0b10000110, 0b11000110, 0b01111100, 0b00111000, 0b00000000,
    0b00111111, 0b00111111, 0b00000000, 0b00000000, 0b00000001, 0b00000011, 0b00000110, 0b00011100, 0b00110000, 0b00100000,  // char R
    0b00111000, 0b01111100, 0b11000110, 0b11000110, 0b10000010, 0b10000010, 0b10000110, 0b00000110, 0b00000100,
    0b00110000, 0b00110000, 0b00100000, 0b00100000, 0b00100000, 0b00110001, 0b00110001, 0b00011111, 0b00001110,  // char S
    0b00000010, 0b00000010, 0b00000010, 0b00000010, 0b11111110, 0b11111110, 0b00000010, 0b00000010, 0b00000010, 0b00000010,
    0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00111111, 0b00111111, 0b00000000, 0b00000000, 0b00000000, 0b00000000,  // char T
    0b11111110, 0b11111110, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b11111110, 0b11111110,
    0b00001111, 0b00011111, 0b00110000, 0b00110000, 0b00100000, 0b00110000, 0b00110000, 0b00011111, 0b00001111,  // char U
    0b00000010, 0b00011110, 0b11111000, 0b11000000, 0b00000000, 0b00000000, 0b00000000, 0b11000000, 0b11111000, 0b00011110, 0b00000010,
    0b00000000, 0b00000000, 0b00000000, 0b00000111, 0b00011110, 0b00110000, 0b00111110, 0b00000111, 0b00000000, 0b00000000, 0b00000000,  // char V
    0b00011110, 0b11111110, 0b00000000, 0b00000000, 0b11000000, 0b00100000, 0b11000000, 0b00000000, 0b00000000, 0b11111110, 0b00011110,
    0b00000000, 0b00111111, 0b00110000, 0b00001110, 0b00000011, 0b00000000, 0b00000011, 0b00011110, 0b00110000, 0b00111111, 0b00000000,  // char W
    0b00000010, 0b00000110, 0b00011100, 0b00110000, 0b01100000, 0b11000000, 0b01100000, 0b00110000, 0b00001100, 0b00000110, 0b00000010,
    0b00100000, 0b00110000, 0b00011100, 0b00000110, 0b00000011, 0b00000000, 0b00000011, 0b00000110, 0b00011100, 0b00110000, 0b00100000,  // char X
    0b00000110, 0b00011100, 0b01110000, 0b11000000, 0b10000000, 0b11100000, 0b00111000, 0b00001110, 0b00000010,
    0b00000000, 0b00000000, 0b00000000, 0b00111111, 0b00111111, 0b00000000, 0b00000000, 0b00000000, 0b00000000,  // char Y
    0b00000010, 0b00000010, 0b00000010, 0b00000010, 0b11000010, 0b01100010, 0b00111010, 0b00001110, 0b00000110,
    0b00110000, 0b00111000, 0b00101110, 0b00100011, 0b00100001, 0b00100000, 0b00100000, 0b00100000, 0b00100000,  // char Z
    0b11111110, 0b11111110, 0b00000010, 0b00000010,
    0b11111111, 0b11111111, 0b00000000, 0b00000000,  // char [
    0b00000010, 0b00011110, 0b01111000, 0b11000000, 0b00000000, 0b00000000, 0b00000000,
    0b00000000, 0b00000000, 0b00000000, 0b00000001, 0b00001111, 0b00111100, 0b00100000,  // char (backslash)
    0b00000010, 0b00000010, 0b11111110, 0b11111110,
    0b00000000, 0b00000000, 0b11111111, 0b11111111,  // char ]
    0b11111000, 0b11111110, 0b00000111, 0b00000011, 0b00000011, 0b00000011, 0b00000011, 0b00000111, 0b11111110, 0b11111000,
    0b00110001, 0b00110111, 0b00111100, 0b00110000, 0b00000000, 0b00000000, 0b00110000, 0b00111100, 0b00110111, 0b00110001,  // char Ω, somewhat widened
    // 0b00000000,0b11000000,0b01110000,0b00011100,0b00000110,0b00011100,0b01110000,0b11000000,0b00000000,
    // 0b00000001,0b00000001,0b00000000,0b00000000,0b00000000,0b00000000,0b00000000,0b00000001,0b00000001, // char ^
    0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,
    0b10000000, 0b10000000, 0b10000000, 0b10000000, 0b10000000, 0b10000000, 0b10000000, 0b10000000,  // char _
    0b00000001, 0b00000011, 0b00000100,
    0b00000000, 0b00000000, 0b00000000,  // char `
    0b00000000, 0b00110000, 0b00110000, 0b00010000, 0b00010000, 0b00110000, 0b11110000, 0b11100000,
    0b00011110, 0b00111110, 0b00100011, 0b00100001, 0b00100001, 0b00010001, 0b00011111, 0b00111111,  // char a
    0b11111111, 0b11111111, 0b00100000, 0b00010000, 0b00010000, 0b00010000, 0b00110000, 0b11100000, 0b11000000,
    0b00111111, 0b00011111, 0b00010000, 0b00100000, 0b00100000, 0b00100000, 0b00110000, 0b00011111, 0b00001111,  // char b
    0b11000000, 0b11100000, 0b00110000, 0b00010000, 0b00010000, 0b00010000, 0b00110000,
    0b00001111, 0b00011111, 0b00110000, 0b00100000, 0b00100000, 0b00100000, 0b00110000,  // char c
    0b11000000, 0b11100000, 0b00110000, 0b00010000, 0b00010000, 0b00100000, 0b11111111, 0b11111111,
    0b00001111, 0b00011111, 0b00110000, 0b00100000, 0b00100000, 0b00010000, 0b00011111, 0b00111111,  // char d
    0b10000000, 0b11100000, 0b00110000, 0b00010000, 0b00010000, 0b00010000, 0b00110000, 0b11100000, 0b11000000,
    0b00000111, 0b00011111, 0b00010001, 0b00110001, 0b00100001, 0b00100001, 0b00100001, 0b00110001, 0b00000001,  // char e
    0b00010000, 0b00010000, 0b00010000, 0b11111110, 0b11111111, 0b00010001, 0b00010001, 0b00010001, 0b00000011,
    0b00000000, 0b00000000, 0b00000000, 0b00111111, 0b00111111, 0b00000000, 0b00000000, 0b00000000, 0b00000000,  // char f
    0b00000000, 0b11100000, 0b11100000, 0b00010000, 0b00010000, 0b00010000, 0b00010000, 0b11110000, 0b11110000, 0b00010000,
    0b11000000, 0b11100001, 0b00111111, 0b00010110, 0b00010010, 0b00010010, 0b00010010, 0b00010001, 0b11110001, 0b11100000,  // char g
    0b11111111, 0b11111111, 0b00100000, 0b00010000, 0b00010000, 0b00110000, 0b11110000, 0b11100000,
    0b00111111, 0b00111111, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00111111, 0b00111111,  // char h
    0b00000000, 0b00010000, 0b00010000, 0b11110011, 0b11110011, 0b00000000, 0b00000000, 0b00000000,
    0b00100000, 0b00100000, 0b00100000, 0b00111111, 0b00111111, 0b00100000, 0b00100000, 0b00100000,  // char i
    0b00010000, 0b00010000, 0b00010000, 0b11110011, 0b11110011,
    0b00000000, 0b00000000, 0b00000000, 0b11111111, 0b11111111,  // char j
    0b11111111, 0b11111111, 0b00000000, 0b10000000, 0b01000000, 0b00100000, 0b00010000, 0b00000000,
    0b00111111, 0b00111110, 0b00000011, 0b00000010, 0b00001100, 0b00011000, 0b00110000, 0b00100000,  // char k
    0b00000000, 0b00000001, 0b00000001, 0b11111111, 0b11111111, 0b00000000, 0b00000000, 0b00000000,
    0b00100000, 0b00100000, 0b00100000, 0b00111111, 0b00111111, 0b00100000, 0b00100000, 0b00100000,  // char l
    0b11110000, 0b11100000, 0b00010000, 0b11110000, 0b11100000, 0b00010000, 0b11110000, 0b11100000,
    0b00111111, 0b00111111, 0b00000000, 0b00111111, 0b00111111, 0b00000000, 0b00111111, 0b00111111,  // char m
    0b11110000, 0b11100000, 0b00100000, 0b00010000, 0b00010000, 0b00110000, 0b11110000, 0b11100000,
    0b00111111, 0b00111111, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00111111, 0b00111111,  // char n
    0b11000000, 0b11100000, 0b00110000, 0b00010000, 0b00010000, 0b00010000, 0b00110000, 0b11100000, 0b11000000,
    0b00000111, 0b00011111, 0b00110000, 0b00100000, 0b00100000, 0b00100000, 0b00110000, 0b00011111, 0b00001111,  // char o
    0b11110000, 0b11110000, 0b00100000, 0b00010000, 0b00010000, 0b00010000, 0b00110000, 0b11100000, 0b11000000,
    0b11111111, 0b11111111, 0b00010000, 0b00100000, 0b00100000, 0b00100000, 0b00110000, 0b00011111, 0b00001111,  // char p
    0b11000000, 0b11100000, 0b00110000, 0b00010000, 0b00010000, 0b00100000, 0b11100000, 0b11110000,
    0b00001111, 0b00011111, 0b00110000, 0b00100000, 0b00100000, 0b00010000, 0b11111111, 0b11111111,  // char q
    0b11110000, 0b11000000, 0b00100000, 0b00010000, 0b00010000, 0b00010000, 0b00110000,
    0b00111111, 0b00111111, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,  // char r
    0b11100000, 0b11110000, 0b00010000, 0b00010000, 0b00010000, 0b00110000, 0b00110000,
    0b00110000, 0b00110001, 0b00100001, 0b00100011, 0b00110011, 0b00111110, 0b00011100,  // char s
    0b00010000, 0b00010000, 0b11111100, 0b11111110, 0b00010000, 0b00010000, 0b00010000,
    0b00000000, 0b00000000, 0b00011111, 0b00111111, 0b00100000, 0b00100000, 0b00100000,  // char t
    0b11110000, 0b11110000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b11110000, 0b11110000,
    0b00011111, 0b00111111, 0b00110000, 0b00100000, 0b00100000, 0b00010000, 0b00011111, 0b00111111,  // char u
    0b00010000, 0b11110000, 0b10000000, 0b00000000, 0b00000000, 0b00000000, 0b10000000, 0b11110000, 0b00010000,
    0b00000000, 0b00000000, 0b00000111, 0b00111110, 0b00100000, 0b00111100, 0b00000111, 0b00000000, 0b00000000,  // char v
    0b01110000, 0b11100000, 0b00000000, 0b00000000, 0b11100000, 0b00110000, 0b11100000, 0b00000000, 0b00000000, 0b11100000, 0b01110000,
    0b00000000, 0b00001111, 0b00110000, 0b00011110, 0b00000001, 0b00000000, 0b00000001, 0b00011110, 0b00110000, 0b00001111, 0b00000000,  // char w
    0b00000000, 0b00110000, 0b01100000, 0b11000000, 0b00000000, 0b11000000, 0b01100000, 0b00110000, 0b00000000,
    0b00100000, 0b00110000, 0b00011100, 0b00000111, 0b00000011, 0b00000111, 0b00011100, 0b00110000, 0b00100000,  // char x
    0b00010000, 0b11110000, 0b11000000, 0b00000000, 0b00000000, 0b00000000, 0b11000000, 0b11110000, 0b00010000,
    0b00000000, 0b00000000, 0b00000111, 0b10011110, 0b11100000, 0b00011110, 0b00000111, 0b00000000, 0b00000000,  // char y
    0b00010000, 0b00010000, 0b00010000, 0b00010000, 0b10010000, 0b01110000, 0b00110000,
    0b00110000, 0b00111000, 0b00101110, 0b00100011, 0b00100001, 0b00100000, 0b00100000,  // char z
    0b00000000, 0b00000000, 0b10000000, 0b11111100, 0b01111110, 0b00000110, 0b00000010,
    0b00000001, 0b00000001, 0b00000011, 0b11111110, 0b11111100, 0b10000000, 0b00000000,  // char {
    0b11111111,
    0b11111111,  // char |
    0b00000010, 0b00000110, 0b01111110, 0b11111100, 0b10000000, 0b00000000, 0b00000000,
    0b00000000, 0b10000000, 0b11111100, 0b11111110, 0b00000011, 0b00000001, 0b00000001,  // char }
    0b01000000, 0b01100000, 0b00100000, 0b01100000, 0b01000000, 0b11000000, 0b10000000, 0b11000000, 0b01000000,
    0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000, 0b00000000,  // char ~
};

const uint8_t TinyOLEDFont_Noto_Mono_16xvariable_widths[] = {
    9, 2, 6, 11, 9, 11, 10, 2, 5, 5, 9, 8, 3, 5, 4, 7,     // 16 widths
    9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 2, 3, 9, 9, 9, 8,        // 16 widths
    11, 11, 9, 9, 9, 8, 8, 9, 9, 7, 8, 9, 8, 9, 9, 9,      // 16 widths
    9, 9, 10, 9, 10, 9, 11, 11, 11, 9, 9, 4, 7, 4, 10, 8,  // 16 widths
    3, 8, 9, 7, 8, 9, 9, 10, 8, 8, 5, 8, 8, 8, 8, 9,       // 16 widths
    9, 8, 7, 7, 7, 8, 9, 11, 9, 9, 7, 7, 1, 7, 9,          // 16 widths
};

const uint16_t TinyOLEDFont_Noto_Mono_16xvariable_widths_16s[] = {
    9 + 2 + 6 + 11 + 9 + 11 + 10 + 2 + 5 + 5 + 9 + 8 + 3 + 5 + 4 + 7,
    9 + 9 + 9 + 9 + 9 + 9 + 9 + 9 + 9 + 9 + 2 + 3 + 9 + 9 + 9 + 8,
    11 + 11 + 9 + 9 + 9 + 8 + 8 + 9 + 9 + 7 + 8 + 9 + 8 + 9 + 9 + 9,
    9 + 9 + 10 + 9 + 10 + 9 + 11 + 11 + 11 + 9 + 9 + 4 + 7 + 4 + 10 + 8,
    3 + 8 + 9 + 7 + 8 + 9 + 9 + 10 + 8 + 8 + 5 + 8 + 8 + 8 + 8 + 9,
    9 + 8 + 7 + 7 + 7 + 8 + 9 + 11 + 9 + 9 + 7 + 7 + 1 + 7 + 9};

const DCfont TinyOLEDFontNoto_Mono_16xvariable = {
    (uint8_t *)ssd1306xled_fontNoto_Mono_16xvariable,
    0,        // character width in pixels
    2,        // character height in pages (8 pixels)
    32, 126,  // ASCII extents
    (uint16_t *)TinyOLEDFont_Noto_Mono_16xvariable_widths_16s,
    (uint8_t *)TinyOLEDFont_Noto_Mono_16xvariable_widths,
    2  // spacing
};

#define FONTNOTO_MONO_16XVARIABLE (&TinyOLEDFontNoto_Mono_16xvariable)

// If I want to tweak the rendering, I have to override void SSD1306Device::renderOriginalSize(uint8_t c)

//**********************

/* Standard ASCII 6x8 font, adapted from ssd1306xled_font6x8 and lowered 1 pixel */
// TODO: Some characters are too low now, and are cut off.
const uint8_t ssd1306xled_font6x8_low[] PROGMEM = {
    0x0, 0x0, 0x0, 0x0, 0x0, 0x0,       // 0
    0x0, 0x0, 0x0, 0x5E, 0x0, 0x0,      // ! 1
    0x0, 0x0, 0xE, 0x0, 0xE, 0x0,       // " 2
    0x0, 0x28, 0xFE, 0x28, 0xFE, 0x28,  // # 3
    0x0, 0x48, 0x54, 0xFE, 0x54, 0x24,  // $ 4
    0x0, 0xC4, 0xC8, 0x10, 0x26, 0x46,  // % 5
    0x0, 0x6C, 0x92, 0xAA, 0x44, 0xA0,  // & 6
    0x0, 0x0, 0xA, 0x6, 0x0, 0x0,       // ' 7
    0x0, 0x0, 0x38, 0x44, 0x82, 0x0,    // ( 8
    0x0, 0x0, 0x82, 0x44, 0x38, 0x0,    // ) 9
    0x0, 0x28, 0x10, 0x7C, 0x10, 0x28,  // * 10
    0x0, 0x10, 0x10, 0x7C, 0x10, 0x10,  // + 11
                                        //    0x0, 0x0, 0x0, 0x140, 0xC0, 0x0,       // , 12
    0x0, 0x0, 0x0, 0x40, 0xC0, 0x0,     // , 12
    0x0, 0x10, 0x10, 0x10, 0x10, 0x10,  // - 13
    0x0, 0x0, 0xC0, 0xC0, 0x0, 0x0,     // . 14
    0x0, 0x40, 0x20, 0x10, 0x8, 0x4,    // / 15
    0x0, 0x7C, 0xA2, 0x92, 0x8A, 0x7C,  // 0 16
    0x0, 0x0, 0x84, 0xFE, 0x80, 0x0,    // 1 17
    0x0, 0x84, 0xC2, 0xA2, 0x92, 0x8C,  // 2 18
    0x0, 0x42, 0x82, 0x8A, 0x96, 0x62,  // 3 19
    0x0, 0x30, 0x28, 0x24, 0xFE, 0x20,  // 4 20
    0x0, 0x4E, 0x8A, 0x8A, 0x8A, 0x72,  // 5 21
    0x0, 0x78, 0x94, 0x92, 0x92, 0x60,  // 6 22
    0x0, 0x2, 0xE2, 0x12, 0xA, 0x6,     // 7 23
    0x0, 0x6C, 0x92, 0x92, 0x92, 0x6C,  // 8 24
    0x0, 0xC, 0x92, 0x92, 0x52, 0x3C,   // 9 25
    0x0, 0x0, 0x6C, 0x6C, 0x0, 0x0,     // : 26
    0x0, 0x0, 0xAC, 0x6C, 0x0, 0x0,     // ; 27
    0x0, 0x10, 0x28, 0x44, 0x82, 0x0,   // < 28
    0x0, 0x28, 0x28, 0x28, 0x28, 0x28,  // = 29
    0x0, 0x0, 0x82, 0x44, 0x28, 0x10,   // > 30
    0x0, 0x4, 0x2, 0xA2, 0x12, 0xC,     // ? 31
    0x0, 0x64, 0x92, 0xB2, 0xA2, 0x7C,  // @ 32
    0x0, 0xF8, 0x24, 0x22, 0x24, 0xF8,  // A 33
    0x0, 0xFE, 0x92, 0x92, 0x92, 0x6C,  // B 34
    0x0, 0x7C, 0x82, 0x82, 0x82, 0x44,  // C 35
    0x0, 0xFE, 0x82, 0x82, 0x44, 0x38,  // D 36
    0x0, 0xFE, 0x92, 0x92, 0x92, 0x82,  // E 37
    0x0, 0xFE, 0x12, 0x12, 0x12, 0x2,   // F 38
    0x0, 0x7C, 0x82, 0x92, 0x92, 0xF4,  // G 39
    0x0, 0xFE, 0x10, 0x10, 0x10, 0xFE,  // H 40
    0x0, 0x0, 0x82, 0xFE, 0x82, 0x0,    // I 41
    0x0, 0x40, 0x80, 0x82, 0x7E, 0x2,   // J 42
    0x0, 0xFE, 0x10, 0x28, 0x44, 0x82,  // K 43
    0x0, 0xFE, 0x80, 0x80, 0x80, 0x80,  // L 44
    0x0, 0xFE, 0x4, 0x18, 0x4, 0xFE,    // M 45
    0x0, 0xFE, 0x8, 0x10, 0x20, 0xFE,   // N 46
    0x0, 0x7C, 0x82, 0x82, 0x82, 0x7C,  // O 47
    0x0, 0xFE, 0x12, 0x12, 0x12, 0xC,   // P 48
    0x0, 0x7C, 0x82, 0xA2, 0x42, 0xBC,  // Q 49
    0x0, 0xFE, 0x12, 0x32, 0x52, 0x8C,  // R 50
    0x0, 0x8C, 0x92, 0x92, 0x92, 0x62,  // S 51
    0x0, 0x2, 0x2, 0xFE, 0x2, 0x2,      // T 52
    0x0, 0x7E, 0x80, 0x80, 0x80, 0x7E,  // U 53
    0x0, 0x3E, 0x40, 0x80, 0x40, 0x3E,  // V 54
    0x0, 0x7E, 0x80, 0x70, 0x80, 0x7E,  // W 55
    0x0, 0xC6, 0x28, 0x10, 0x28, 0xC6,  // X 56
    0x0, 0xE, 0x10, 0xE0, 0x10, 0xE,    // Y 57
    0x0, 0xC2, 0xA2, 0x92, 0x8A, 0x86,  // Z 58
    0x0, 0x0, 0xFE, 0x82, 0x82, 0x0,    // [ 59
    0x0, 0x4, 0x8, 0x10, 0x20, 0x40,    // \ 60
    0x0, 0x0, 0x82, 0x82, 0xFE, 0x0,    // ] 61
    // 0x0, 0x8, 0x4, 0x2, 0x4, 0x8,          // ^ 62
    0b00000000,
    0b10111100,
    0b11000010,
    0b00000010,
    0b11000010,
    0b10111100,                         // char Ω
    0x0, 0x80, 0x80, 0x80, 0x80, 0x80,  // _ 63
    0x0, 0x0, 0x2, 0x4, 0x8, 0x0,       // ' 64
    0x0, 0x40, 0xA8, 0xA8, 0xA8, 0xF0,  // a 65
    0x0, 0xFE, 0x90, 0x88, 0x88, 0x70,  // b 66
    0x0, 0x70, 0x88, 0x88, 0x88, 0x40,  // c 67
    0x0, 0x70, 0x88, 0x88, 0x90, 0xFE,  // d 68
    0x0, 0x70, 0xA8, 0xA8, 0xA8, 0x30,  // e 69
    0x0, 0x10, 0xFC, 0x12, 0x2, 0x4,    // f 70
                                        //    0x0, 0x30, 0x148, 0x148, 0x148, 0xF8,  // g 71
    0x0, 0x30, 0x48, 0x48, 0x48, 0xF8,  // g 71
    0x0, 0xFE, 0x10, 0x8, 0x8, 0xF0,    // h 72
    0x0, 0x0, 0x88, 0xFA, 0x80, 0x0,    // i 73
                                        //    0x0, 0x80, 0x100, 0x108, 0xFA, 0x0,    // j 74
    0x0, 0x80, 0x00, 0x08, 0xFA, 0x0,   // j 74
    0x0, 0xFE, 0x20, 0x50, 0x88, 0x0,   // k 75
    0x0, 0x0, 0x82, 0xFE, 0x80, 0x0,    // l 76
    0x0, 0xF8, 0x8, 0x30, 0x8, 0xF0,    // m 77
    0x0, 0xF8, 0x10, 0x8, 0x8, 0xF0,    // n 78
    0x0, 0x70, 0x88, 0x88, 0x88, 0x70,  // o 79
                                        //    0x0, 0x1F8, 0x48, 0x48, 0x48, 0x30,    // p 80
    0x0, 0xF8, 0x48, 0x48, 0x48, 0x30,  // p 80
                                        //    0x0, 0x30, 0x48, 0x48, 0x30, 0x1F8,    // q 81
    0x0, 0x30, 0x48, 0x48, 0x30, 0xF8,  // q 81
    0x0, 0xF8, 0x10, 0x8, 0x8, 0x10,    // r 82
    0x0, 0x90, 0xA8, 0xA8, 0xA8, 0x40,  // s 83
    0x0, 0x8, 0x7E, 0x88, 0x80, 0x40,   // t 84
    0x0, 0x78, 0x80, 0x80, 0x40, 0xF8,  // u 85
    0x0, 0x38, 0x40, 0x80, 0x40, 0x38,  // v 86
    0x0, 0x78, 0x80, 0x60, 0x80, 0x78,  // w 87
    0x0, 0x88, 0x50, 0x20, 0x50, 0x88,  // x 88
                                        //    0x0, 0x38, 0x140, 0x140, 0x140, 0xF8,  // y 89
    0x0, 0x38, 0x40, 0x40, 0x40, 0xF8,  // y 89
    0x0, 0x88, 0xC8, 0xA8, 0x98, 0x88,  // z 90
    0x0, 0x10, 0x6C, 0x82, 0x82, 0x0,   // { 91
    0x0, 0x0, 0x0, 0xFE, 0x0, 0x0,      // | 92
    0x0, 0x0, 0x82, 0x82, 0x6C, 0x10,   // } 93
    0x0, 0x10, 0x8, 0x10, 0x20, 0x10,   // ~ 94
};

// ----------------------------------------------------------------------------

const DCfont TinyOLED4kfont6x8_low = {
    (uint8_t *)ssd1306xled_font6x8_low,
    6,       // character width in pixels
    1,       // character height in pages (8 pixels)
    32, 126,  // ASCII extents
    (uint16_t *)NULL, (uint8_t *)NULL, 0 // these are useless here, only used for proportional font.
};

// for backwards compatibility
#define SCREEN_SMALLFONT (&TinyOLED4kfont6x8_low)

//**********************

/* ultra small proportional font, no space below */
const uint8_t ssd1306xled_XXSfont_8xvariable_bitmaps[] = {
    0b10000000,                                                                                                  // char .
    0b11111000, 0b00100100, 0b00100010, 0b00100100, 0b11111000,                                                  // char /, replaced by "A" sign
    0b01111000, 0b10000100, 0b10000100, 0b01111000,                                                              // char 0
    0b00001000, 0b11111100,                                                                                      // char 1
    0, 0b01111100, 0b01111100, 0, 0b00010000, 0b00010000, 0, 0b00010000, 0b00010000, 0, 0b00010000, 0b00010000,  // '2': iRange 0
    0, 0b00010000, 0b00010000, 0, 0b01111100, 0b01111100, 0, 0b00010000, 0b00010000, 0, 0b00010000, 0b00010000,  // '3': iRange 1
    0, 0b00010000, 0b00010000, 0, 0b00010000, 0b00010000, 0, 0b01111100, 0b01111100, 0, 0b00010000, 0b00010000,  // '4': iRange 2
    0, 0b00010000, 0b00010000, 0, 0b00010000, 0b00010000, 0, 0b00010000, 0b00010000, 0, 0b01111100, 0b01111100,  // '5': iRange 3

};

const uint8_t ssd1306xled_XXSfont_8xvariable_widths[] = {
    1, 5, 4, 2, 12, 12, 12, 12};

const uint16_t ssd1306xled_XXSfont_8xvariable_widths_16s[] = {
    1 + 5 + 4 + 2 + 12 + 12 + 12 + 12};

const DCfont ssd1306xled_XXSfont_8xvariable = {
    (uint8_t *)ssd1306xled_XXSfont_8xvariable_bitmaps,
    0,       // character width in pixels
    1,       // character height in pages (8 pixels)
    46, 53,  // ASCII extents
    (uint16_t *)ssd1306xled_XXSfont_8xvariable_widths_16s,
    (uint8_t *)ssd1306xled_XXSfont_8xvariable_widths,
    1  // spacing
};

#define SCREEN_XXSFONT (&ssd1306xled_XXSfont_8xvariable)

// ----------------------------------------------------------------------------

// What to print on what line
#define SCREEN_LINE_MV 0
#define SCREEN_LINE_IMP 2
#define SCREEN_LINE_RBULK 0
#define SCREEN_LINE_RSEI 2
#define SCREEN_LINE_BATTERY 0
#define SCREEN_LINE_RANGE1 1
#define SCREEN_LINE_RANGE2 2

#define SCREEN_BATTERY_LENGTH 12
#define SCREEN_RANGE1_LENGTH 18
#define SCREEN_RANGE2_LENGTH 6
#define SCREEN_START_COLUMN_BATTERY (screen_width - SCREEN_BATTERY_LENGTH)
#define SCREEN_START_COLUMN_RANGE1 (screen_width - SCREEN_RANGE1_LENGTH)
#define SCREEN_START_COLUMN_RANGE2 (screen_width - SCREEN_RANGE2_LENGTH)

// The following is an combination of the SCREEN_LINE_xxx and the SCREEN_START_COLUMN_xxx above
#define SCREEN_END_COLUMN_0 SCREEN_START_COLUMN_BATTERY
#define SCREEN_END_COLUMN_1 SCREEN_START_COLUMN_RANGE1
#define SCREEN_END_COLUMN_2 SCREEN_START_COLUMN_RANGE2
#define SCREEN_END_COLUMN_3 screen_width

#define SCREEN_BIGFONT FONTNOTO_MONO_16XVARIABLE

// The start of the values
#define SCREEN_START_COLUMN_VALUE 0
// The end of the unit. Must be somewhat before SCREEN_START_COLUMN_BATTERY
#define SCREEN_END_COLUMN_UNIT 80
// character spacing. This was a bit hard to read with the standard functions, so just define it. This must match the font. Monospaced: 0, if not DCFont.spacing.
#define SCREEN_CHAR_SPACING 2
// This is where type starts. Must be well before SCREEN_START_COLUMN_BATTERY and somewhat after SCREEN_END_COLUMN_UNIT
#define SCREEN_START_COLUMN_TYPE (SCREEN_END_COLUMN_UNIT + 10)

/**
 * @brief Wipe a section or the remainder of the screen
 *
 * @param end The end column. if < 0: go top the end of the screen.
 * @param line The line on the screen (8 pixels high!)
 */
void screenWipeRest(int end, int line) {
    int x = oled.getCursorX();
    // Wipe 2 lines high.
    for (int i = line; i <= line + 1; i++) {
        int e = screen_width;
        // end was given?
        if (end >= 0)
            // yes
            e = end;
        else {
            // no. Get it from config
            switch (i) {
                case 0:
                    e = SCREEN_END_COLUMN_0;
                    break;
                case 1:
                    e = SCREEN_END_COLUMN_1;
                    break;
                case 2:
                    e = SCREEN_END_COLUMN_2;
                    break;
                case 3:
                    e = SCREEN_END_COLUMN_3;
            }
        }
        if (e > x) {
            oled.setCursor(x, i);
            // The following lines might be replaced by oled.fillLength(0,e - x)
            oled.startData();
            oled.repeatData(0x00, e - x);
            oled.endData();
        }
    }
}

void screenPrintRemote(void) {
    int line = SCREEN_LINE_MV;
    
    oled.setFont(SCREEN_BIGFONT);
    oled.setCursor(SCREEN_START_COLUMN_VALUE,line);
    oled.print("Remote");
    screenWipeRest(-1, line);

    line = SCREEN_LINE_IMP;
    oled.setCursor(SCREEN_START_COLUMN_VALUE,line);
    screenWipeRest(-1, line);
}

void screenPrintV(double v) {
    const int line = SCREEN_LINE_MV;

    char str[10];

    // big font for the number
    // There is no ranging here, so just 1 format
    oled.setFont(SCREEN_BIGFONT);
    oled.setCursor(SCREEN_START_COLUMN_VALUE, line);
    if (v < VOLT_MAX) {
        if (v < VOLT_MIN)
            v = VOLT_MIN;
        sprintf(str, "%5.2f", v);
        oled.print(str);
    } else {
        // overflow
        oled.print("++++++");
    }
    // now print the unit
    const char *unit = "V";
    int e = oled.getCharacterWidth(unit[0]);
    screenWipeRest(SCREEN_END_COLUMN_UNIT - e, line);
    oled.setCursor(SCREEN_END_COLUMN_UNIT - e, line);
    oled.print(unit);
    screenWipeRest(-1, line);
}

/**
 * @brief print the float resistance value to a string
 * Uses ^ as symbol for Ohm
 *
 * @param str string to print to. Must be wide enough.
 * @param unit unit string. Must be 3 chars wide.
 * @param range enum IRange (Arduino IDE does not allow enums as function parameters....)
 * @param ohm the value to print
 */
void screenFormatR(char *str, char *unit, uint8_t range, double ohm) {
    bool show4Digits = settingsShow4Digits();
    const char *fmt = "";

    switch (range) {
        case range_100uA:
            if (show4Digits)
                fmt = "%5.2f";
            else
                fmt = "%4.1f";
            sprintf(str, fmt, ohm);
            strcpy(unit, "^");
            break;
        case range_1mA:
            if (show4Digits)
                fmt = "%5.3f";
            else
                fmt = "%4.2f";
            sprintf(str, fmt, ohm);
            strcpy(unit, "^");
            break;
        case range_10mA:
            if (show4Digits)
                fmt = "%5.1f";
            else
                fmt = "%3.0f";
            sprintf(str, fmt, ohm * 1000.0);
            strcpy(unit, "m^");
            break;
        case range_100mA:
            if (show4Digits)
                fmt = "%5.2f";
            else
                fmt = "%4.1f";
            sprintf(str, fmt, ohm * 1000.0);
            strcpy(unit, "m^");
            break;
        default:
            sprintf(str, "???");
            strcpy(unit, "");
    }
}

// The resistance type to print.
enum RType : uint8_t { rtype_imp = 0,
                       rtype_rbulk,
                       rtype_rsei };
/**
 * @brief Print a resistance value on screen
 *
 * @param ohm the value to print
 * @param rtype enum RType, the type. This also determines where on screen I print it.
 * @param irange enum IRange, the range that is selected. This determines the format.
 * @param resultFlags see bm_adcDiffSampleResults_... values
 */
void screenPrintImp(double ohm, uint8_t rtype, uint8_t irange, uint8_t resultFlags) {
    char str[10];
    char unit[3];

    int line = SCREEN_LINE_IMP;
    if (rtype == rtype_rbulk)
        line = SCREEN_LINE_RBULK;
    if (rtype == rtype_rsei)
        line = SCREEN_LINE_RSEI;

    // The resultflags:
    // In sequence:
    // 1) timeout or overrun: programmer or HW problem. Bad. Results mean nothing now.: show ERR
    // 2) undervoltage: Bad. Results mean nothing now.: show LOW V
    // 3) overrange or shortproblem or overload: show overload (I might have a value but it may not fit on screen)
    // 4) underrange or nothing: Show the value, correctly ranged. Underrange means something for the autoranger, but not for the display

    // big font for the number
    oled.setFont(SCREEN_BIGFONT);
    oled.setCursor(SCREEN_START_COLUMN_VALUE, line);

    const char *errstr = sampleResultToString(resultFlags, true);
    if (errstr) {
        oled.print(errstr);
    } else {
        if (ohm < 0.0)
            ohm = 0.0;
        // print the value
        screenFormatR(str, unit, irange, ohm);
        oled.print(str);
        // print the unit
        // I do width calculations the hard and dirty way
        int e = 0;
        if (unit[0])
            e += oled.getCharacterWidth(unit[0]);
        if (unit[1]) {
            e += SCREEN_CHAR_SPACING;
            e += oled.getCharacterWidth(unit[1]);
        }
        screenWipeRest(SCREEN_END_COLUMN_UNIT - e, line);
        oled.setCursor(SCREEN_END_COLUMN_UNIT - e, line);
        oled.print(unit);
    }

    // fill in blanks if needed
    screenWipeRest(SCREEN_START_COLUMN_TYPE, line);
    oled.setCursor(SCREEN_START_COLUMN_TYPE, line);
    // finish with the type of measurement
    oled.print("R");
    int x = oled.getCursorX() - SCREEN_CHAR_SPACING;  // yeah, a dirty trick to reduce the space to the subscript
    oled.setFont(SCREEN_SMALLFONT);
    oled.setCursor(x, line);

    if (rtype == rtype_rbulk) {
        // Rb (or RΩ)
        oled.print(" ");  // wipe the top line. It is 1 char wide
        oled.setCursor(x, line + 1);
        oled.print("b");  // b for bulk. Other option: ^ for the ohm symbol
    } else if (rtype == rtype_rsei) {
        // Rsei
        oled.print("   ");  // wipe the top line. It is 3 chars wide
        oled.setCursor(x, line + 1);
        oled.print("SEI");
    } else {
        // 1kHz impedance
        oled.print(" "); 
        oled.setCursor(x, line + 1);
        oled.print("~");
    }
    // Messy code here, I should really do a clean on a line basis instead of going through screenWipeRest
    screenWipeRest(-1, line);
}

/**
 * @brief Print the battery level, top right corner.
 * at < 25%, and not charging, it will flash the battery symbol
 * when charging, it will animate the battery symbol
 * Charging symbol is not implemented hardware wise, as when the CPU is on, 
 * it consumes more than the charging current, so the battery still discharges.
 *
 * @param percentage battery percentage
 * @param amCharging true when charging. 
 */
void screenPrintBattery(int percentage, bool amCharging) {
    static int animate_state = 0;
    static unsigned long animate_start;

    // The battery symbol:
    // ..XXXXXXXXXX < top right corner
    // ..X        X
    // .XX        X
    // .XX        X
    // ..X        X
    // ..XXXXXXXXXX
    // ............
    // ............

    const uint8_t tip = 0b00001100;    // tip of the outline
    const uint8_t bar = 0b00111111;    // start and end outline
    const uint8_t full = 0b00111111;   // outline + "full"
    const uint8_t empty = 0b00100001;  // only the outline

    if (percentage < 0)
        percentage = 0;

    if (percentage > 100)
        percentage = 100;

    if (amCharging) {
        // animate the battery, present state to full in max 2 secs, so animate +5% every 100ms
        if (animate_state >= 100) {
            animate_state = percentage;
            // set start counter
            animate_start = millis();
        }
        if (animate_state < percentage)
            animate_state = percentage;

        // move ahead every 100ms?
        if (millis() - animate_start >= 100) {
            animate_state += 5;
            if (animate_state > 100)
                animate_state = 100;
            animate_start = millis();  // yes, this can stutter, but millis can be interrupted regularly during measurement, so this is safer
        }
        // then fake the percentage (guaranteed max = 100)
        percentage = animate_state;
    } else {
        // flash the battery symbol, 1sec
        if (percentage < 25) {
            if ((millis() % 1000) < 500) {
                // wipe the battery symbol
                oled.setCursor(SCREEN_START_COLUMN_BATTERY, SCREEN_LINE_BATTERY);  // top right
                oled.startData();
                for (int i = SCREEN_BATTERY_LENGTH; i > 0; i--) {
                    oled.sendData(0);
                }
                oled.endData();
                return;
            }
        }
    }

    // if N = body size of the battery, make the level in the range (N-1)..0, and only 100 = N
    // N = SCREEN_BATTERY_LENGTH - 4
    int level = (percentage * (SCREEN_BATTERY_LENGTH - 4)) / 100;

    oled.setCursor(SCREEN_START_COLUMN_BATTERY, SCREEN_LINE_BATTERY);  // top right
    oled.startData();
    // 0
    oled.sendData(0);  // empty
    // 1
    oled.sendData(tip);
    // 2
    oled.sendData(bar);
    for (int i = SCREEN_BATTERY_LENGTH - 4; i > 0; i--) {
        if (level >= i)
            oled.sendData(full);
        else
            oled.sendData(empty);
    }
    // last
    oled.sendData(bar);
    oled.endData();
}

/**
 * @brief Print the active range on screen, one line at a time
 * 
 * @param range the active range
 * @param isAuto true if ranging is automatic
 * @param isTopline if this is the top line
 */
void screenPrintRangeSub(uint8_t range, bool isAuto, bool isTopline) {
    int line, startpos;
    if (isTopline) {
      line = SCREEN_LINE_RANGE1;
      startpos = SCREEN_START_COLUMN_RANGE1;
    }
    else {
      line = SCREEN_LINE_RANGE2;
      startpos = SCREEN_START_COLUMN_RANGE2;
    }
    oled.setFont(SCREEN_XXSFONT);
    oled.setCursor(startpos, line);
    const char *cptr = szgetRRange(range, isAuto, isTopline);

    // right align the text. But I must do it the "hard" way, as getTextWidth only takes PROGMEM stuff.
    // int w = oled.getTextWidth(cptr);
    int w = -1;  // and eat up the trailing space in advance.
    const char *dptr = cptr;
    while (*dptr) {
        w += oled.getCharacterWidth(*dptr);
        w += SCREEN_XXSFONT->spacing;
        dptr++;
    }
    if (w < 0) w = 0; // if nothing was to be printed, then don't print
    oled.fillLength(0, screen_width - w - startpos);
    if (w > 0)
        oled.print(cptr);
}

/**
 * @brief Print the active range on screen
 *
 * @param range the active range
 * @param isAuto true if ranging is automatic
 */
void screenPrintRange(uint8_t range, bool isAuto) {
    screenPrintRangeSub(range, isAuto, true);
    screenPrintRangeSub(range, isAuto, false);
}

void setup_screen(void) {
    oled.begin(0, 0, screen_width, screen_height, sizeof(ssd1306_init_sequence), ssd1306_init_sequence);
    oled.clear();
    oled.on();
}

#endif

#pragma endregion

#pragma region TESTS

#ifdef TEST_OLED
void setup() {
    Serial.begin(115200, (SERIAL_8N1));
    Serial.printf("TEST_OLED\n");
    setup_screen();
    oled.setContrast(20);
    // Set entire memory to hatched (fixed size: 128x(8x8), independent of the display size)
    for (uint8_t y = 0; y < 8; y++) {
        oled.setCursor(0, y);
        oled.startData();
        for (uint8_t x = 0; x < 128; x += 2) {
            oled.sendData(0b10101010);
            oled.sendData(0b01010101);
        }
        oled.endData();
    }
}

void loop() {
    fillScreen();
}

#if 1
int nr = 0;
bool dtype = false;
double v = 0.01;
void fillScreen() {
    nr++;
    v = v * 1.1;
    if (nr > 100) {
        nr = 0;
        v = 0.01;
        dtype = !dtype;
    }
    // some fake ranging based on the bewlo multiplication factor. 360 = full range now (when 80 = full range)
    uint8_t range = 0;
    if (v <= 36) range++;
    if (v <= 3.6) range++;
    if (v <= 0.36) range++;

    if (dtype) {
        screenPrintV(v);
        screenPrintImp(v * 0.2, rtype_imp, range, 0);
    } else {
        screenPrintImp(v * 0.21, rtype_rbulk, range, 0);
        screenPrintImp(v * 0.22, rtype_rsei, range, 0);
    }
    // screenPrintESR(mv % 10000, false);
    // screenPrintESR(10000 - (mv % 10000), true);
    screenPrintBattery(nr, false);
    delay(1);
    //  uint8_t startScrollPage = 1;
    //  uint8_t endScrollPage = 2;
    //  uint8_t startScrollColumn = 8;
    //  uint8_t endScrollColumn = startScrollColumn + screen_width - 16;
    //  for (uint8_t x = 0; x < screen_width - 16; x++)
    //  {
    //    delay(50);
    //    oled.scrollContentRight(startScrollPage, endScrollPage, startScrollColumn, endScrollColumn);
    //  }
}

#else
#define CHAR_MIN 32
#define CHAR_MAX 127

int nr = CHAR_MIN;
void fillScreen() {
    char str1[12];
    char str2[12];
    nr++;
    if (nr > CHAR_MAX)
        nr = CHAR_MIN;
    for (int i = 0; i < 11; i++) {
        int n = nr + i;
        if (n > 127)
            n = CHAR_MIN;
        str1[i] = n;
        n = nr + 11 + i;
        if (n > 127)
            n = CHAR_MIN;
        str2[i] = n;
    }
    str1[11] = 0;
    str2[11] = 0;

    oled.setFont(SCREEN_BIGFONT);
    oled.setCursor(0, 0);
    oled.print(str1);
    oled.setCursor(0, 2);
    oled.print(str2);
    delay(100);
}

#endif

#endif

#ifdef TEST_BLINKY
#define MYPIN 1

int loopnr;

int offset = 0;
int maxnr = 11;

void setup() {
    for (int i = 0; i < maxnr; i++) {
        pinMode(i + offset, OUTPUT);
        digitalWrite(i + offset, HIGH);
    }
    loopnr = 0;
}

void loop() {
    loopnr += 1;
    for (int i = 0; i < maxnr; i++) {
        if (loopnr % (i + 1) == 0) {
            int x = loopnr / (i + 1);
            if (x & 1) {
                digitalWrite(i + offset, HIGH);
            } else {
                digitalWrite(i + offset, LOW);
            }
        }
    }
    delay(10);
}
#endif

#ifdef TEST_TYPES
void setup() {
    setup_screen();
    // Set entire memory to hatched (fixed size: 128x(8x8), independent of the display size)
    for (uint8_t y = 0; y < 8; y++) {
        oled.setCursor(0, y);
        oled.startData();
        for (uint8_t x = 0; x < 128; x += 2) {
            oled.sendData(0b10101010);
            oled.sendData(0b01010101);
        }
        oled.endData();
    }
    oled.setFont(SCREEN_SMALLFONT);
    oled.setCursor(0, 0);
}

void loop() {
    float fl = 1.12345678901234567890;
    double db = 0.0;
    int32_t i32 = 0;
    int64_t i64 = 0;
    int i = 0;
    long int l = 0;

    char str[80];
    oled.setCursor(0, 0);
    sprintf(str, "fl: %1.15f", fl);  // 1.123456800000
    oled.print(str);
    oled.setCursor(0, 1);
    sprintf(str, "fl: %d, db: %d ", sizeof(fl), sizeof(db));  // 4 4
    oled.print(str);
    oled.setCursor(0, 2);
    sprintf(str, "32: %d, 64: %d ", sizeof(i32), sizeof(i64));  // 4 8
    oled.print(str);
    oled.setCursor(0, 3);
    sprintf(str, "i: %d, l: %d ", sizeof(i), sizeof(l));  // 2 4
    oled.print(str);
}
#endif
#pragma endregion

#pragma region UserInterface

// Forward declarations:
void setup_CommandParser(void);
void loop_CommandParser(void);

// The run time user settings:

/**
 * @brief do I have auto ranging enabled?
 * To be used only by getAutoRange,setAutoRange, moveRangeUpOrDown (and toggleRange)
 */
static bool _bInAutoRange = false;

/**
 * @brief do I add additional info on the serial output?
 * Set and unset from SCPI commands.
 */
static bool bDiagOutput = false;

enum localCmds : uint8_t { localCmd_none = 0,
                           localCmd_AC = 1,
                           localCmd_DCIS = 2 };

/**
 * @brief do I have local control?
 * Should be the case for button controlled commands, may not be the case for SCPI or serial debug commands.
 * When remote control, turn on led.
 * Any local button action except for ramge switching, will disable remote control.
 */
static uint8_t _LocalCommand = localCmd_none;

inline void setLocalCommand(uint8_t cmd) { 
  if (cmd != _LocalCommand) setLed(cmd == localCmd_none); 
  _LocalCommand = cmd; 
}
inline void setRemoteControl(void) { _LocalCommand = localCmd_none; setLed(true); }
inline uint8_t getLocalCommand(void) { return _LocalCommand; }

// The temp stuff:

/**
 * @brief keyboard temp variable
 */
static uint8_t _lastBtn = btn_none;
static uint32_t _lastBtnMillis;

// The functions:
/**
 * @brief see if auto ranging is enabled
 *
 * @return true is autorange is set
 */
bool getAutoRange(void) { return _bInAutoRange; }

/**
 * @brief Enables or disabled auto ranging.
 * Also sets the actual range to the minimum.
 * @param b
 */
void setAutoRange(bool b) {
    _bInAutoRange = b;
    setIRange(range_min);
}

/**
 * @brief move the range up or down from the UI.
 * if auto: will go to the present range, but non auto
 * if not auto: will move up or down. When at the end: go to auto
 *
 * @param up true: min->max->auto. false: max->min->auto
 */
void moveRangeUpOrDown(bool up) {
    if (_bInAutoRange) {
        _bInAutoRange = false; // do not reset the range
    } else {
        if (up) {
            if (!setIRangeUp()) {
                _bInAutoRange = true;
            }
        } else {
            if (!setIRangeDown()) {
                _bInAutoRange = true;
            }            
        }
    }
}

/**
 * @brief Set the Range from the UI.
 * Can toggle through all ranges or can go manual up or down
 * Should only be used by debug code
 *
 * @param r 0: toggle through ranges (auto->min->max->auto....), > 0: manual up, <0: manual down
 */
void toggleRange(int r) {
    if (r == 0) {
        // Cycle through the ranges
        if (_bInAutoRange) {
            setAutoRange(false);
        } else {
            if (!setIRangeUp()) {
                setAutoRange(true);
            }
        }
    } else {
        _bInAutoRange = false;  // force manual and force the range
        if (r > 0)
            setIRangeUp();
        else
            setIRangeDown();
    }
}

/**
 * @brief Splash screen
 *
 * @param str The string to show on screen
 */
void showHello(const char *str) {
    oled.setFont(SCREEN_SMALLFONT);
    oled.setCursor(0, 0);
    oled.print(str);
    Serial.printf("%s\n", str);  // you will not see this when the device starts, as the CH340 chip will take some time to start up
}

/**
 * @brief Set the up UI functions
 *
 */
void setup_UI(void) {
    _lastBtn = btn_none;

    setAutoRange(true);
    showHello(AppName); // Useless, serial interface will not be ready yet.
    setup_CommandParser();
    setLocalCommand(localCmd_AC);
}

const char str_vDCInput[] = ", vDCInput=%0.5f";
const char str_rAC[] = ", rAC=%0.5f";
const char str_rB[] = ", rB=%0.5f";
const char str_rSEI[] = ", rSEI=%0.5f";
const char str_rangeUsed[] = ", rangeUsed=%d";
const char str_resultFlags[] = ", resultFlags=0x%02x";
const char str_shortRef_duration[] = ", shortRef.duration=%d";
const char str_shortRef_rawp[] = ", shortRef.rawp=%ld";
const char str_shortRef_rawn[] = ", shortRef.rawn=%ld";
const char str_shortTest_duration[] = ", shortTest.duration=%d";
const char str_shortTest_rawp[] = ", shortTest.rawp=%ld";
const char str_shortTest_rawn[] = ", shortTest.rawn=%ld";
const char str_nrSamples[] = ", nrSamples=%d";
const char str_vRef[] = ", vRef=%0.5f";
const char str_vTest[] = ", vTest=%0.5f";
const char str_minSampleRef[] = ", minSampleRef=%ld";
const char str_maxSampleRef[] = ", maxSampleRef=%ld";
const char str_minSampleTest[] = ", minSampleTest=%ld";
const char str_maxSampleTest[] = ", maxSampleTest=%ld";

/**
 * @brief execute and show a DCIS measurement
 * 
 * @param interface The interface to dump output to, apart from screen
 * @param islocal if called from local UI
 */
void measureAndShowDCIS(Stream &interface, bool isFromUI) {
    struct dcisSampleResults r;
    double v = getDCISInfo(&r, 0, getAutoRange());
    
    screenPrintImp(v, rtype_rbulk, r.rangeUsed, r.resultFlags);
    screenPrintImp(r.rSEI, rtype_rsei, r.rangeUsed, r.resultFlags);
    screenPrintRange(r.rangeUsed, getAutoRange());    
    
    if (isFromUI) interface.print("DCIS: ");
    streamPrintImp(v, r.resultFlags, interface);
    interface.print(", ");
    streamPrintImp(r.rSEI, r.resultFlags, interface);
    interface.printf(", %0.2f V", r.vDCInput);

    if (bDiagOutput) {
        // dump the results
        interface.printf(str_vDCInput, r.vDCInput);
        interface.printf(str_rB, r.rB);
        interface.printf(str_rSEI, r.rSEI);
        interface.printf(str_rangeUsed, r.rangeUsed);
        interface.printf(str_resultFlags, r.resultFlags);
        interface.printf(str_shortRef_duration, r.shortRef.duration);
        interface.printf(str_shortRef_rawp, r.shortRef.rawp);
        interface.printf(str_shortRef_rawn, r.shortRef.rawn);
        interface.printf(str_shortTest_duration, r.shortTest.duration);
        interface.printf(str_shortTest_rawp, r.shortTest.rawp);
        interface.printf(str_shortTest_rawn, r.shortTest.rawn);
        interface.printf(str_nrSamples, r.nrSamples);
        
        // struct vDrop t1[DCIS_PLFNR_NR_SAMPLES]; /**< voltage drop data for t1 */
        // struct vDrop t2[DCIS_PLFNR_NR_SAMPLES]; /**< voltage drop data for t2 */
    }
    interface.print("\n");
}

/**
 * @brief execute and show a AC measurement
 * 
 * @param interface The interface to dump output to, apart from screen
 * @param islocal if called from local UI
 */
void measureAndShowAC(Stream &interface, bool isFromUI) {
    struct acSampleResults r;
    double v = getImpedance(&r, 0, getAutoRange());

    screenPrintV(r.vDCInput);
    screenPrintImp(v, rtype_imp, r.rangeUsed, r.resultFlags);
    screenPrintRange(r.rangeUsed, getAutoRange());

    if (isFromUI) interface.print("AC: ");
    streamPrintImp(v, r.resultFlags, interface);
    interface.printf(", %0.2f V", r.vDCInput);

    if (bDiagOutput) {
        // dump the results
        interface.printf(str_vDCInput, r.vDCInput);
        interface.printf(str_rAC, v);
        interface.printf(str_vRef, r.vRef);
        interface.printf(str_vTest, r.vTest);
        interface.printf(str_rangeUsed, r.rangeUsed);
        interface.printf(str_resultFlags, r.resultFlags);
        interface.printf(str_shortRef_duration, r.shortRef.duration);
        interface.printf(str_shortRef_rawp, r.shortRef.rawp);
        interface.printf(str_shortRef_rawn, r.shortRef.rawn);
        interface.printf(str_shortTest_duration, r.shortTest.duration);
        interface.printf(str_shortTest_rawp, r.shortTest.rawp);
        interface.printf(str_shortTest_rawn, r.shortTest.rawn);
        interface.printf(str_minSampleRef, r.minSampleRef);
        interface.printf(str_maxSampleRef, r.maxSampleRef);
        interface.printf(str_minSampleTest, r.minSampleTest);
        interface.printf(str_maxSampleTest, r.maxSampleTest);
    }
    interface.print("\n");
}
/**
 * @brief run the UI
 * Read keys
 * do "background" screen updates: battery level and range settings
 *
 */
void loop_UI(void) {
    // Read physical buttons and serial commands

    //         if (millis() - animate_start >= 100)

    loop_CommandParser();

    // See if I have a button press
    // With button repeater (that has bugs, but this was the easiest)
    uint8_t newBtn = getButtons();
    if ((newBtn != _lastBtn) || 
        ((newBtn == _lastBtn) && (_lastBtn != btn_none) && (_lastBtn != btn_error) && (millis() - _lastBtnMillis >= 1000))
       ) {
        // Serial.printf("BTN: %d\n",newBtn);
        _lastBtn = newBtn;
        if ((_lastBtn != btn_none) && (_lastBtn != btn_error)) {
            _lastBtnMillis = millis();
//#define PRESS_FOR_RANGING
#ifdef PRESS_FOR_RANGING            
            if (newBtn == btn_press) {
                toggleRange(0);
                screenPrintRange(getIRange(), getAutoRange()); // do that here, so I get immediate visual feedback, before the measurement is done.
            }
            if (newBtn == btn_up) {
                // Serial.print("UP ");
                setLocalCommand(localCmd_AC);
            }
            if (newBtn == btn_down) {
                // Serial.print("DOWN ");
                setLocalCommand(localCmd_DCIS);
            }
#else
            if (newBtn == btn_press) {
                if (getLocalCommand() != localCmd_AC)
                    setLocalCommand(localCmd_AC);
                else
                    setLocalCommand(localCmd_DCIS);
            }
            if ((newBtn == btn_up) || (newBtn == btn_down)) {
                moveRangeUpOrDown(newBtn == btn_up);
                screenPrintRange(getIRange(), getAutoRange()); // do that here, so I get immediate visual feedback, before the measurement is done.
            }
#endif
        }
    }

    switch (getLocalCommand()) {
      case localCmd_DCIS:
          measureAndShowDCIS(Serial, true);
          break;      
      case localCmd_AC:
          measureAndShowAC(Serial, true);
          break;
      default:
          // local control is off
          screenPrintRemote();
    }

    screenPrintBattery(getInternalBatteryPercentage(), false);
}

#pragma endregion

#pragma region Serial SCPI Commands
#ifndef NO_MAIN
#ifndef COMMANDS_DEBUG
// ************************************************************************************************
// SCPI command handler
// ************************************************************************************************

SCPI_Parser my_instrument;

// forward definitions:
void scpiCmdIdentify(SCPI_C commands, SCPI_P parameters, Stream &interface);
void scpiCmdUsage(SCPI_C commands, SCPI_P parameters, Stream &interface);
void scpiCmdGetLastEror(SCPI_C commands, SCPI_P parameters, Stream &interface);
void scpiCmdMeasureV(SCPI_C commands, SCPI_P parameters, Stream &interface);
void scpiCmdMeasureR(SCPI_C commands, SCPI_P parameters, Stream &interface);
void scpiCmdSetDiag(SCPI_C commands, SCPI_P parameters, Stream &interface);
void scpiCmdSetRange(SCPI_C commands, SCPI_P parameters, Stream &interface);
void scpiCmdQueryRange(SCPI_C commands, SCPI_P parameters, Stream &interface);
void scpiCmdSetCAL(SCPI_C commands, SCPI_P parameters, Stream &interface);
void scpiCmdSaveCAL(SCPI_C commands, SCPI_P parameters, Stream &interface);
void scpiCmdQueryCAL(SCPI_C commands, SCPI_P parameters, Stream &interface);
void scpiCmdNOP(SCPI_C commands, SCPI_P parameters, Stream &interface);

struct scpiCmdDefinition {
    const char *command;
    const char *explanation;
    SCPI_caller_t caller;
};

const scpiCmdDefinition scpiCmdDefinitions[] = {
    {"*IDN?", "Identify", &scpiCmdIdentify},
    {"*HELP?", "Usage", &scpiCmdUsage},
    {"SYST:ERR?", "Last error message", &scpiCmdGetLastEror},
    {"SYST:DIAG:ON", "Diagnostic output on", &scpiCmdSetDiag},
    {"SYST:DIAG:OFF", "Diagnostic output off", &scpiCmdSetDiag},
    {"MEAS:VOLT:INP?", "Input voltage", &scpiCmdMeasureV},
    {"MEAS:VOLT:INT?", "Internal voltage", &scpiCmdMeasureV},
    {"MEAS:RES:AC?", "1kHz input impedance", &scpiCmdMeasureR},
    {"MEAS:RES:DCIS?", "DCIS impedance values.", &scpiCmdMeasureR},
    {"CONF:RANGE", "{AUTO|1..4} \tSet current range", &scpiCmdSetRange},
    {"CONF:RANGE?", "Get current range", &scpiCmdQueryRange},
    {"CAL:INPV", "{NR1} \tSet input V factor (mV@1V)", &scpiCmdSetCAL},
    {"CAL:INPO", "{NR1} \tSet input V offset", &scpiCmdSetCAL},
    {"CAL:INTV", "{NR1} \tSet internal V factor (mV@1V)", &scpiCmdSetCAL},
    {"CAL:VMIN", "{NR1} \tSet internal V gauge min (mV)", &scpiCmdSetCAL},
    {"CAL:VMAX", "{NR1} \tSet internal V gauge max (mV)", &scpiCmdSetCAL},
    {"CAL:R#", "{NR1} \t# = 1..4 \tSet ref resistor (1:mOhm..4:uOhm)", &scpiCmdSetCAL},
    {"CAL:T#", "{NR1} \t# = 1,2,PAUSE \tSet DCIS timings (us)", &scpiCmdSetCAL},
    {"CAL:PLF", "{NR1} \tSet power line Frequency (50 or 60)", &scpiCmdSetCAL},
    {"CAL:SAVE","Save cal values",&scpiCmdSaveCAL},
    {"DIAG:CAL?", "Get cal values", &scpiCmdQueryCAL}
  };


// *IDN? => <manufacturer>,<model>,<HW version>,<SW version>
// MEASure:VOLTage:INPut? => <getExternalVoltage()>
// MEASure:VOLTage:INTernal? => <getInternalBatteryVoltage()>, <getInternalBatteryPercentage()>
// MEASure:RESistance:AC? => <R1kHz>
// MEASure:RESistance:DCIS? => <Rt1>, <Rt2>
// CONFigure:RANGE {AUTO|(1:4)}
// CONFigure:RANGE? => (AUTO|1:4)
// CAL:INPV {<NR1>}
// CAL:INTV {<NR1>}
// CAL:VMIN {<NR1>}
// CAL:VMAX {<NR1>}
// CAL:R<(1:4)> {<NR1l>}
// CAL:T<(1,2,PAUSE)> {NR1}
// CAL:SAVE
// DIAG:CAL? => "key"=value[,"key"=value[...]]
// *HELP? => <list of possible commands and explanation>

// TODO: If I have space, add there mandatory commands. They can all be NOP:
// *CLS Clear Status Command 10.3
// *ESE Standard Event Status Enable Command 10.10
// *ESE? Standard Event Status Enable Query 10.11
// *ESR? Standard Event Status Register Query 10.12
// *OPC Operation Complete Command 10.18
// *OPC? Operation Complete Query 10.19
// *RST Reset Command 10.32
// *SRE Service Request Enable Command 10.34
// *SRE? Service Request Enable Query 10.35
// *STB? Read Status Byte Query 10.36
// *TST? Self-Test Query 10.38
// *WAI Wait-to-Continue Command 10.39
// :SYSTem
//  :ERRor 21.8
//   [:NEXT]? 21.8.3e (see Note 1) 1996
//  :VERSion? 19.16 (see Note 2) 1991 PH
// :STATus 18 5
//  :OPERation
//   [:EVENt]?
//   :CONDition?
//   :ENABle
//   :ENABle?
//  :QUEStionable
//   [:EVENt]?
//   :CONDition?
//   :ENABle
//   :ENABle?
//  :PRESet

// ATTENTION DEVELOPER:
// The tables are pre-allocated fropm the library itself. And they are costly in memory usage.
// to check: enable last line in scpiCmdPrintUsage, got to SCPI, SYSTem:DIAG:ON, then *HELP?
// right now: 26 tokens, 21 commands.

#ifndef SCPI_MAX_TOKENS
#error Somehow the SCPI max tokens are not defined
#else
#if SCPI_MAX_TOKENS != 30
#error SCPI_MAX_TOKENS does not have a good value. If you use Arduino IDE, Launch with 'arduino --pref compiler.cpp.extra_flags=-DSCPI_MAX_TOKENS'
#endif
#endif

#ifndef SCPI_MAX_COMMANDS
#error Somehow the SCPI max commands are not defined
#else
#if SCPI_MAX_COMMANDS != 25
#error SCPI_MAX_COMMANDS does not have a good value. If you use Arduino IDE, Launch with 'arduino --pref compiler.cpp.extra_flags=-SCPI_MAX_COMMANDS'
#endif
#endif

/**
 * @brief Set the up CommandParser
 */
void setup_CommandParser(void) {
    bDiagOutput = false;
    for (unsigned int i = 0; i < sizeof(scpiCmdDefinitions) / sizeof(scpiCmdDefinition); i++) {
        my_instrument.RegisterCommand(scpiCmdDefinitions[i].command, scpiCmdDefinitions[i].caller);
    }
    my_instrument.SetErrorHandler(&scpiCmdErrorHandler);

}

/**
 * @brief Do the command parser loop. To be called from the UI loop function.
 */
void loop_CommandParser(void) {
    my_instrument.ProcessInput(Serial, "\n",'\r');
}

/**
 * @brief Print the command to serial output
 *
 * @param commands the commands givem
 * @param parameters the parameters given
 * @param interface
 * @param suffix Suffix to print.
 */
void scpiCmdDebugPrintCommand(SCPI_C commands, SCPI_P parameters, Stream &interface, const char *suffix) {
    interface.print(F("Command '"));
    for (int i = 0; i < commands.Size(); i++) {
        if (i != 0)
            interface.print(F(":"));
        interface.print(commands[i]);
    }
    if (parameters.Size() > 0)
        interface.print(" ");
    for (int i = 0; i < parameters.Size(); i++) {
        if (i != 0)
            interface.print(",");
        interface.print(parameters[i]);
    }
    interface.print("'");
    interface.println(suffix);
}

/**
 * @brief Print the scpi commands usage to the output
 * Will stop local control
 *
 * @param interface
 */
void scpiCmdPrintUsage(Stream &interface) {
    setRemoteControl();
  
    interface.println(F("Allowed Commands:"));
    for (unsigned int i = 0; i < sizeof(scpiCmdDefinitions) / sizeof(scpiCmdDefinition); i++) {
        interface.print(scpiCmdDefinitions[i].command);
        if (scpiCmdDefinitions[i].explanation[0] == '{')
            interface.print(F(" "));
        else
            interface.print(F("\t"));
        interface.println(scpiCmdDefinitions[i].explanation);
    }
    interface.println();

    // if (bDiagOutput) my_instrument.PrintDebugInfo();
}

/**
 * @brief The error handler for SCPI
 * Will stop local control in some cases
 *
 * @param commands
 * @param parameters
 * @param interface
 */
void scpiCmdErrorHandler(SCPI_C commands, SCPI_P parameters, Stream &interface) {
    // This function is called every time an error occurs
    // suppress compiler warnings
    (void) commands;
    (void) parameters;

    switch (my_instrument.last_error) {
        case my_instrument.ErrorCode::BufferOverflow:
            interface.println(F("ERROR: Buffer overflow."));
            setRemoteControl();
            break;
        case my_instrument.ErrorCode::Timeout:
            interface.println(F("ERROR: Comm timeout."));
            setRemoteControl();
            break;
        case my_instrument.ErrorCode::UnknownCommand:
            interface.println(F("ERROR: Unknown command. Use '*HELP?' for the list of allowed commands."));
            setRemoteControl();
            break;
        case my_instrument.ErrorCode::NoError:
            interface.println(F("OK"));
            break;
    }
    /* The error type is stored in my_instrument.last_error
       Possible errors are:
         SCPI_Parser::ErrorCode::NoError
         SCPI_Parser::ErrorCode::UnknownCommand
         SCPI_Parser::ErrorCode::Timeout
         SCPI_Parser::ErrorCode::BufferOverflow
    */

    /* For BufferOverflow errors, the rest of the message, still in the interface
    buffer or not yet received, will be processed later and probably
    trigger another kind of error.
    Here we flush the incomming message*/
    if (my_instrument.last_error == SCPI_Parser::ErrorCode::BufferOverflow) {
        delay(2);
        while (interface.available()) {
            delay(2);
            interface.read();
        }
    }
}

/**
 * @brief identify the device
 * Will stop local control
 * 
 * @param commands
 * @param parameters
 * @param interface
 */
void scpiCmdIdentify(SCPI_C commands, SCPI_P parameters, Stream &interface) {
    // suppress compiler warnings
    (void) commands;
    (void) parameters;

    setRemoteControl();
  
    interface.println(F("DIY, Battery Meter, HW v" HW_VERSION ", SW v" SW_VERSION));
}

/**
 * @brief Show usage
 * Will stop local control
 *
 * @param commands
 * @param parameters
 * @param interface
 */
void scpiCmdUsage(SCPI_C commands, SCPI_P parameters, Stream &interface) {
    // suppress compiler warnings
    (void) commands;
    (void) parameters;

    setRemoteControl();
      
    scpiCmdPrintUsage(interface);
}

/**
 * @brief Get the last SCPI command parser error
 * Will stop local control
 *
 * @param commands
 * @param parameters
 * @param interface
 */
void scpiCmdGetLastEror(SCPI_C commands, SCPI_P parameters, Stream &interface) {
    // suppress compiler warnings
    (void) commands;
    (void) parameters;

    setRemoteControl();
    
    switch (my_instrument.last_error) {
        case my_instrument.ErrorCode::BufferOverflow:
            interface.println(F("Buffer overflow"));
            break;
        case my_instrument.ErrorCode::Timeout:
            interface.println(F("Comm. timeout"));
            break;
        case my_instrument.ErrorCode::UnknownCommand:
            interface.println(F("Unknown command"));
            break;
        case my_instrument.ErrorCode::NoError:
            interface.println(F("No Error"));
            break;
    }
    my_instrument.last_error = my_instrument.ErrorCode::NoError;
}

/**
 * @brief Control diagnostics output
 * Will stop local control
 *
 * @param commands
 * @param parameters
 * @param interface
 */
void scpiCmdSetDiag(SCPI_C commands, SCPI_P parameters, Stream &interface) {
     // suppress compiler warnings
    (void) parameters;
    (void) interface;

    setRemoteControl();
    
    String header = String(commands.Last());
    header.toUpperCase();
    if (header.startsWith("ON")) {
        bDiagOutput = true;
    } else {
        bDiagOutput = false;
    }
}

/**
 * @brief Measure Voltage, one measurement per call.
 * Will stop local control
 *
 * @param commands
 * @param parameters
 * @param interface
 */
void scpiCmdMeasureV(SCPI_C commands, SCPI_P parameters, Stream &interface) {
    // suppress compiler warnings
    (void) commands;
    (void) parameters;
    
    setRemoteControl();
      
    String header = String(commands.Last());
    header.toUpperCase();
    if (header.startsWith("INT")) {
        interface.printf("%5.4f, %d\n", getInternalBatteryVoltage(), getInternalBatteryPercentage());
    } else {
        interface.printf("%5.4f\n", getExternalVoltage());
    }
}

/**
 * @brief Print a resistance value on the given interface.
 * The logic in this function should mostly match screenPrintImp()
 *
 * @param ohm the value to print
 * @param resultFlags see bm_adcDiffSampleResults_... values
 * @param interface the interface to print to
 */
void streamPrintImp(double ohm, uint8_t resultFlags, Stream &interface) {
    const char *errstr = sampleResultToString(resultFlags, false);
    if (errstr) {
        interface.print(errstr);
    } else {
        if (ohm < 0.0)
            ohm = 0.0;
        // print the value
        interface.printf("%0.5f", ohm);
    }
}

/**
 * @brief Measure Impedance, one measurement per call
 * Will stop local control
 *
 * @param commands
 * @param parameters
 * @param interface
 */
void scpiCmdMeasureR(SCPI_C commands, SCPI_P parameters, Stream &interface) {
     // suppress compiler warnings
    (void) parameters;

    setRemoteControl();
    
    String header = String(commands.Last());
    header.toUpperCase();
    if (header.startsWith("DC")) {
        measureAndShowDCIS(interface,false);
    } else {
        measureAndShowAC(interface,false);
    }
}

/**
 * @brief Set the instrument measurement range
 * Does not stop local control, if it was active
 *
 * @param commands
 * @param parameters
 * @param interface
 */
void scpiCmdSetRange(SCPI_C commands, SCPI_P parameters, Stream &interface) {
    // cmdDebugPrintCommand(commands,parameters);
    // suppress compiler warnings
    (void) commands;
    (void) interface;
    bool bOK = false;

    if (parameters.Size() > 0) {
        char ch = parameters[0][0];
        if (ch == 'A' || ch == 'a') {
            setAutoRange(true);
            bOK = true;
        } else {
            // 1 digit input, easy.
            if ((ch >= ('1' + range_min)) && (ch <= ('1' + range_max)) && (parameters[0][1] == 0)) {
                ch -= '1';
                setAutoRange(false);
                setIRange((int)ch);
                bOK = true;
            }
        }
        screenPrintRange(getIRange(), getAutoRange()); // do that here, so I get immediate visual feedback, before the measurement is done.
        if (bOK)
            return;
    }
    my_instrument.last_error = my_instrument.ErrorCode::UnknownCommand;
}

/**
 * @brief Get the instrument measurement range
 * Does not stop local control, if it was active 
 *
 * @param commands
 * @param parameters
 * @param interface
 */
void scpiCmdQueryRange(SCPI_C commands, SCPI_P parameters, Stream &interface) {
    // suppress compiler warnings
    (void) commands;
    (void) parameters;

    if (getAutoRange())
        interface.println(strAUTO);
    else {
        int v = getIRange();
        interface.printf("%d\n", v + 1);
    }
}

/**
 * @brief Set the calibration settings
 * Will stop local control
 *
 * @param commands
 * @param parameters
 * @param interface
 */
void scpiCmdSetCAL(SCPI_C commands, SCPI_P parameters, Stream &interface) {
    long int v;

    // suppress compiler warnings
    (void) commands;
    (void) interface;

    setRemoteControl();

    if (parameters.Size() == 1) {
        v = 0;
        v = atol(parameters[0]); // in fact, I need uint16_t atou(..), but atol also covers that
        if (v != 0) {
            String header = String(commands.Last());
            header.toUpperCase();
            if (header.startsWith("INPV")) {
                mySettings.iInputVoltageMilliVoltAt1V = v;
                return;
            }
            if (header.startsWith("INPO")) {
                if (v < -500)
                    v = -500;
                if (v > 500)
                    v = 500;
                mySettings.iInputVoltageOffset = v;
                return;
            }            
            if (header.startsWith("INTV")) {
                mySettings.iInternalVoltageMilliVoltAt1V = v;
                return;
            }
            if (header.startsWith("R1")) {
                mySettings.iRange0_1mOhms = v;
                return;
            }
            if (header.startsWith("R2")) {
                mySettings.iRange1_100uOhms = v;
                return;
            }
            if (header.startsWith("R3")) {
                mySettings.iRange2_10uOhms = v;
                return;
            }
            if (header.startsWith("R4")) {
                mySettings.iRange3_1uOhms = v;
                return;
            }
            if (header.startsWith("T1")) {
                mySettings.iT1_usecs = v;
                dcisValidateSettings();
                return;
            }
            if (header.startsWith("T2")) {
                mySettings.iT2_usecs = v;
                dcisValidateSettings();
                return;
            }
            if (header.startsWith("TP")) {
                mySettings.iTPause_usecs = v;
                dcisValidateSettings();                
                return;
            }
            if (header.startsWith("PLF")) {
                mySettings.iPLF_50Hz = (v != 60);
                dcisValidateSettings();
                return;
            }
            if (header.startsWith("VMIN")) {
                mySettings.iInternalVoltageMilliVoltMin = v;
                return;
            }
            if (header.startsWith("VMAX")) {
                mySettings.iInternalVoltageMilliVoltMax = v;
                return;
            }          
        }
    }
    my_instrument.last_error = my_instrument.ErrorCode::UnknownCommand;
}

/**
 * @brief Get the calibration settings
 * Will stop local control
 *
 * @param commands
 * @param parameters
 * @param interface
 */
void scpiCmdQueryCAL(SCPI_C commands, SCPI_P parameters, Stream &interface) {
    // suppress compiler warnings
    (void) commands;
    (void) parameters;

    setRemoteControl();
      
    interface.printf("version=%d, intv=%u, inpv=%u, inpo=%d, r1=%u, r2=%u, r3=%u, r4=%u, t1=%u, t2=%lu, tpause=%u, plf=%u, vmin=%u, vmax=%u\n",
                     mySettings.version,
                     mySettings.iInternalVoltageMilliVoltAt1V,
                     mySettings.iInputVoltageMilliVoltAt1V,
                     mySettings.iInputVoltageOffset,
                     mySettings.iRange0_1mOhms,
                     mySettings.iRange1_100uOhms,
                     mySettings.iRange2_10uOhms,
                     mySettings.iRange3_1uOhms,
                     mySettings.iT1_usecs,
                     (unsigned long)mySettings.iT2_usecs,
                     mySettings.iTPause_usecs,
                     mySettings.iPLF_50Hz?50:60,
                     //dcisSampleSpacingMs, // not a setting, but derived from the settings
                     mySettings.iInternalVoltageMilliVoltMin,
                     mySettings.iInternalVoltageMilliVoltMax);
}

/**
 * @brief Save cal settings
 * Will stop local control
 *
 * @param commands
 * @param parameters
 * @param interface
 */
void scpiCmdSaveCAL(SCPI_C commands, SCPI_P parameters, Stream &interface) {
    // suppress compiler warnings
    (void) commands;
    (void) parameters;
    (void) interface;

    setRemoteControl();
    
    writeSettings();
}

/**
 * @brief SCPI NOP
 * Will stop local control
 *
 * @param commands
 * @param parameters
 * @param interface
 */
void scpiCmdNOP(SCPI_C commands, SCPI_P parameters, Stream &interface) {
  
    setRemoteControl();
      
    scpiCmdDebugPrintCommand(commands, parameters, interface, " is not implemented yet.");
}

#endif
#endif
#pragma endregion

#pragma region Serial Debug / Dev Commands
#ifndef NO_MAIN
#ifdef COMMANDS_DEBUG
// ************************************************************************************************
// Debug commands
// ************************************************************************************************

// Forward defs of the commands
void showCommandMenu(void);
void cmdShortref(void);
void cmdShortinput(void);
void cmdToggleShortduringTest(void);
void cmdToggleShortbeforeTest(void);
void cmdToggleShortinRest(void);
void cmdSetrangeUp(void);
void cmdSetrangeDown(void);
void cmdOff(void);
void cmdSetLoop(void);

void cmdReadDCinput(void);
void cmdReadDCbatt(void);
void cmdACSignalRef(void);
void cmdACSignalInput(void);
void cmdACSignalLeadIn(void);
void cmdReadACref(void);
void cmdReadACinput(void);
void cmdReadACfull(void);
void cmdReadDCISt1(void);
void cmdReadDCISt2(void);
void cmdReadDCISfull(void);
void cmdDCISSignalRef(void);
void cmdDCISSignalInput(void);

// The commands that can be issued.
enum cmds : uint8_t { cmd_none = 0,
                      cmd_help,
                      cmd_shortref,
                      cmd_shortinput,
                      cmd_shortduringTest,
                      cmd_shortbeforeTest,
                      cmd_shortinRest,
                      cmd_setrangeUp,
                      cmd_setrangeCycle,
                      cmd_setrangeDown,
                      cmd_off,
                      cmd_setloop,
                      cmd_readDCinput,
                      cmd_readDCbatt,
                      cmd_ACsignalRef,
                      cmd_ACsignalInput,
                      cmd_ACsignalleadin,
                      cmd_readACref,
                      cmd_readACinput,
                      cmd_readACfull,
                      cmd_readDCISt1,
                      cmd_readDCISt2,
                      cmd_readDCISfull,
                      cmd_DCISsignalRef,
                      cmd_DCISsignalInput
};

typedef struct {
    uint8_t ch;
    uint8_t cmd;
    const char *expl;
    void (*func)(void);
} cmd_t;

/**
 * @brief The last command
 */
static uint8_t lastCmd = cmd_none;

// the toggles
int cmdLoop = 0;
int cmdShortDuringTest = 0;
int cmdShortBeforeTest = 1;
int cmdShortInRest = 1;

static const cmd_t Commands[] = {
    {'?', cmd_help, "Help", &showCommandMenu},
    {'z', cmd_shortref, "Short Ref", &cmdShortref},
    {'Z', cmd_shortinput, "Short Input", &cmdShortinput},
    {'s', cmd_shortduringTest, "toggle shorting during test", &cmdToggleShortduringTest},
    {'S', cmd_shortbeforeTest, "toggle shorting before test", &cmdToggleShortbeforeTest},
    {'=', cmd_shortinRest, "toggle shorting in rest", &cmdToggleShortinRest},

    {'>', cmd_setrangeUp, "Range Up", &cmdSetrangeUp},
    {'|', cmd_setrangeCycle, "Range Cycle", &cmdSetrangeCycle},
    {'<', cmd_setrangeDown, "Range Down", &cmdSetrangeDown},

    {'0', cmd_off, "OFF", &cmdOff},

    {'l', cmd_setloop, "toggle loop (for the commands below)", &cmdSetLoop},

    {'d', cmd_readDCinput, "Read DC Input", &cmdReadDCinput},
    {'D', cmd_readDCbatt, "Read DC Batt", &cmdReadDCbatt},
    {'`', cmd_ACsignalRef, "AC signal 30s, amp on Ref", &cmdACsignalRef},
    {'~', cmd_ACsignalInput, "AC signal 30s, amp on Input", &cmdACsignalInput},
    {'L', cmd_ACsignalleadin, "AC signal only lead-in", &cmdACsignalleadin},
    {'a', cmd_readACref, "Read AC only Ref", &cmdReadACref},
    {'A', cmd_readACinput, "Read AC only Input", &cmdReadACinput},
    {'i', cmd_readACfull, "Read AC Impedance", &cmdReadACfull},
    {'1', cmd_readDCISt1, "Read DCIS only t1", &cmdReadDCISt1},
    {'2', cmd_readDCISt2, "Read DCIS only t2", &cmdReadDCISt2},
    {'3', cmd_readDCISfull, "Read DCIS", &cmdReadDCISfull},
    {'-', cmd_DCISsignalRef, "DCIS signal 1s, amp on Ref", &cmdDCISSignalRef},
    {'_', cmd_DCISsignalInput, "DCIS signal 1s, amp on Input", &cmdDCISSignalInput},

};

void showCommandMenu(void) {
    Serial.printf("%s, HW %s\n", AppName, HW_VERSION);
    Serial.printf("All commands are single letter commands. Options:\n");
    size_t nrCmds = sizeof(Commands) / sizeof(Commands[0]);
    for (unsigned int i = 0; i < nrCmds; i++) {
        bool v = false;
        bool havetoggle = false;
        switch (Commands[i].cmd) {
            case cmd_shortduringTest:
                v = cmdShortDuringTest;
                havetoggle = true;
                break;
            case cmd_shortbeforeTest:
                v = cmdShortBeforeTest;
                havetoggle = true;
                break;
            case cmd_shortinRest:
                v = cmdShortInRest;
                havetoggle = true;
                break;
            case cmd_setloop:
                v = cmdLoop;
                havetoggle = true;
                break;
        }
        if (havetoggle)
            Serial.printf("%c : %s (is %s)\n", Commands[i].ch, Commands[i].expl, v ? "ON" : "OFF");
        else
            Serial.printf("%c : %s\n", Commands[i].ch, Commands[i].expl);
    }
    const char *cptr = NULL;
    int b = getButtons();
    switch (b) {
        case btn_none:
            cptr = "-";
            break;
        case btn_up:
            cptr = "UP";
            break;
        case btn_down:
            cptr = "DOWN";
            break;
        case btn_press:
            cptr = "PRESS";
            break;
        default:
            cptr = "ERROR";
    }
    Serial.printf("Buttons: %s\n", cptr);
}
/**
 * @brief Get a command from the serial link
 *
 * @param lastCmd the last command. See enum cmds
 * @return enum cmds
 */
uint8_t getCommand(uint8_t lastCmd) {
    uint8_t ch;
    if (Serial.available() > 0) {
        // read the incoming byte:
        ch = Serial.read();
        size_t nrCmds = sizeof(Commands) / sizeof(Commands[0]);
        for (unsigned int i = 0; i < nrCmds; i++) {
            if (ch == Commands[i].ch) {
                return Commands[i].cmd;
            }
        }
        return cmd_help;
    }
    if (cmdLoop) {
        switch (lastCmd) {
            case cmd_shortref:
            case cmd_shortinput:
            case cmd_shortduringTest:
            case cmd_shortbeforeTest:
            case cmd_shortinRest:
            case cmd_setrangeUp:
            case cmd_setrangeCycle:
            case cmd_setrangeDown:
            case cmd_help:
            case cmd_off:
            case cmd_setloop:
                // don't loop without reason
                return cmd_none;
            default:
                return lastCmd;
        };
    } else {
        return cmd_none;
    }
}

void runCommand(uint8_t cmd) {
    if (cmd == cmd_none) {
        if (cmdShortInRest)
            setShortInRest();
        else
            setShortOff();
        return;
    }
    size_t nrCmds = sizeof(Commands) / sizeof(Commands[0]);
    for (unsigned int i = 0; i < nrCmds; i++) {
        if (cmd == Commands[i].cmd) {
            if (cmd != cmd_help)
                Serial.printf("%s ", Commands[i].expl);
            Commands[i].func();
            Serial.printf("\n");
            return;
        }
    }
}

void cmdOff(void) {
    Serial.printf(" Switching off.....\n");
    setPowerOff();
}

void cmdSetLoop(void) {
    cmdLoop = !cmdLoop;
    Serial.printf(" Set to: %s", cmdLoop ? "ON" : "OFF");
}

void cmdShortref(void) {
    struct adcShortResults r;
    int i = setShortWithDuration(false, false, &r);
    if (i < 0)
        Serial.printf(" TIMEOUT (%5ld,%5ld)", r.rawn, r.rawp);
    else
        Serial.printf(" took %d ms (%5ld,%5ld)", i, r.rawn, r.rawp);
};

void cmdShortinput(void) {
    struct adcShortResults r;
    int i = setShortWithDuration(true, false, &r);
    if (i < 0)
        Serial.printf(" TIMEOUT (%5ld,%5ld)", r.rawn, r.rawp);
    else
        Serial.printf(" took %d ms (%5ld,%5ld)", i, r.rawn, r.rawp);
};

void cmdToggleShortduringTest(void) {
    cmdShortDuringTest = !cmdShortDuringTest;
    Serial.printf(" Set to: %s", cmdShortDuringTest ? "ON" : "OFF");
}

void cmdToggleShortbeforeTest(void) {
    cmdShortBeforeTest = !cmdShortBeforeTest;
    Serial.printf(" Set to: %s", cmdShortBeforeTest ? "ON" : "OFF");
}

void cmdToggleShortinRest(void) {
    cmdShortInRest = !cmdShortInRest;
    Serial.printf(" Set to: %s", cmdShortInRest ? "ON" : "OFF");
}

void cmdSetrangeCycle(void) {
    toggleRange(0);
};

void cmdSetrangeUp(void) {
    toggleRange(1);
};

void cmdSetrangeDown(void) {
    toggleRange(-1);
};

void cmdReadDCinput(void) {
    Serial.printf("; %5.4fV", getExternalVoltage());
};

void cmdReadDCbatt(void) {
    Serial.printf("; %5.4fV; %d%%", getInternalBatteryVoltage(), getInternalBatteryPercentage());
};

void _cmdReadAC(uint8_t measureFlags) {
    struct acSampleResults r;
    measureFlags |= bm_runACSample_LeadIn;
    if (cmdShortDuringTest)
        measureFlags |= bm_runACSample_ShortDuring;
    if (cmdShortBeforeTest)
        measureFlags |= bm_runACSample_ShortBefore;
    double v = getImpedance(&r, measureFlags, getAutoRange());

    screenPrintV(r.vDCInput);
    screenPrintImp(v, rtype_imp, r.rangeUsed, r.resultFlags);

    Serial.printf("getImpedance(%02x); %s; ", measureFlags, szgetIRange(r.rangeUsed));
    Serial.printf("imp: %0.5f; ", v);
    Serial.printf("DCV: %0.2f; ", r.vDCInput);
    if (r.resultFlags) {
        Serial.printf("ERROR: %s%s%s%s%s%s%s ",
                      r.resultFlags & bm_adcDiffSampleResults_overload ? "OVL " : "",
                      r.resultFlags & bm_adcDiffSampleResults_underrange ? "R>> " : "",
                      r.resultFlags & bm_adcDiffSampleResults_overrange ? "R<< " : "",
                      r.resultFlags & bm_adcDiffSampleResults_overrun ? "OVR " : "",
                      r.resultFlags & bm_adcDiffSampleResults_shortproblem ? "SHO " : "",
                      r.resultFlags & bm_adcDiffSampleResults_timeout ? "TIM " : "",
                      r.resultFlags & bm_adcDiffSampleResults_undervoltage ? "UND " : "");
    }

    Serial.printf("\nR: %5.1f (%5ld,%5ld); ", r.vRef, r.minSampleRef, r.maxSampleRef);
    Serial.printf("T: %5.1f (%5ld,%5ld); ", r.vTest, r.minSampleTest, r.maxSampleTest);
    Serial.printf("shortRef %d ms (%5ld,%5ld); ", r.shortRef.duration, r.shortRef.rawn, r.shortRef.rawp);
    Serial.printf("shortInp %d ms (%5ld,%5ld); ", r.shortTest.duration, r.shortTest.rawn, r.shortTest.rawp);
}

void cmdACsignalRef(void) {
    _cmdReadAC(bm_runACSample_longACRef);
}
void cmdACsignalInput(void) {
    _cmdReadAC(bm_runACSample_longACTest);
}
void cmdACsignalleadin(void) {
    _cmdReadAC(0);
}
void cmdReadACref(void) {
    _cmdReadAC(bm_runACSample_Ref);
};
void cmdReadACinput(void) {
    _cmdReadAC(bm_runACSample_Test);
};
void cmdReadACfull(void) {
    _cmdReadAC(bm_runACSample_Ref | bm_runACSample_Test);
};

void _cmdReadDCIS(uint8_t measureFlags) {
    struct dcisSampleResults r;

    if (cmdShortBeforeTest)
        measureFlags |= bm_runDCISSample_ShortBefore;

    double v = getDCISInfo(&r, measureFlags, getAutoRange());
    screenPrintImp(v, rtype_rbulk, r.rangeUsed, r.resultFlags);
    screenPrintImp(r.rSEI, rtype_rsei, r.rangeUsed, r.resultFlags);

    Serial.printf("getDCISInfo(%02x); %s; ", measureFlags, szgetIRange(r.rangeUsed));
    Serial.printf("Rbulk: %0.5f; ", v);
    Serial.printf("RSEI: %0.5f; ", r.rSEI);
    Serial.printf("DCV: %0.2f; ", r.vDCInput);
    Serial.printf("Samples: %d; ", r.nrSamples);
    if (r.resultFlags) {
        Serial.printf("ERROR: %s%s%s%s%s%s%s ",
                      r.resultFlags & bm_adcDiffSampleResults_overload ? "OVL " : "",
                      r.resultFlags & bm_adcDiffSampleResults_underrange ? "R>> " : "",
                      r.resultFlags & bm_adcDiffSampleResults_overrange ? "R<< " : "",
                      r.resultFlags & bm_adcDiffSampleResults_overrun ? "OVR " : "",
                      r.resultFlags & bm_adcDiffSampleResults_shortproblem ? "SHO " : "",
                      r.resultFlags & bm_adcDiffSampleResults_timeout ? "TIM " : "",
                      r.resultFlags & bm_adcDiffSampleResults_undervoltage ? "UND " : "");
    }
    for (int i = 0; i < r.nrSamples; i++) {
        Serial.printf("\nT1[%d]_R: %5ld (%5ld,%5ld);", i, r.t1[i].sRefAfter - r.t1[i].sRefBefore, r.t1[i].sRefBefore, r.t1[i].sRefAfter);
        Serial.printf("T1[%d]_T: %5ld (%5ld,%5ld);", i, r.t1[i].sTestBefore - r.t1[i].sTestAfter, r.t1[i].sTestBefore, r.t1[i].sTestAfter);  // yes, reversed
        Serial.printf("T2[%d]_R: %5ld (%5ld,%5ld);", i, r.t2[i].sRefAfter - r.t2[i].sRefBefore, r.t2[i].sRefBefore, r.t2[i].sRefAfter);
        Serial.printf("T2[%d]_T: %5ld (%5ld,%5ld);", i, r.t2[i].sTestBefore - r.t2[i].sTestAfter, r.t2[i].sTestBefore, r.t2[i].sTestAfter);  // yes, reversed
    }
    Serial.printf("\nshortRef %d ms (%5ld,%5ld); ", r.shortRef.duration, r.shortRef.rawn, r.shortRef.rawp);
    Serial.printf("shortInp %d ms (%5ld,%5ld); ", r.shortTest.duration, r.shortTest.rawn, r.shortTest.rawp);
}

void cmdReadDCISt1(void) {
    _cmdReadDCIS(bm_runDCISSample_t1);
}
void cmdReadDCISt2(void) {
    _cmdReadDCIS(bm_runDCISSample_t2);
}
void cmdReadDCISfull(void) {
    _cmdReadDCIS(bm_runDCISSample_t1 | bm_runDCISSample_t2 | bm_runDCISSample_PLFNR);
}
void cmdDCISSignalRef(void) {
    _cmdReadDCIS(bm_runDCISSample_longRef);
}
void cmdDCISSignalInput(void) {
    _cmdReadDCIS(bm_runDCISSample_longTest);
}

// void showADCOnScreen(void) {
//     // a test loop for ADC showing
//     int32_t v1 = 0;
//     int32_t v2 = 0;
//     int32_t rawv = 0;
// #define ADC_RESOLUTION 16
//     const uint16_t adc_max_scale = ADC_MAX_SCALE_MV; // in mV, hardcoded
//     const int32_t adc_max_val_single = (1UL << ADC_RESOLUTION); // ADC_RESOLUTION bits, unsigned
//     const int32_t adc_max_val_diff = adc_max_val_single / 2; // ADC_RESOLUTION bits, signed
//     const float adc_multiplier = 1.0;
//     char buffer[40];

//     rawv = analogReadEnh(PIN_PA1, ADC_RESOLUTION, 0);
//     v1 = round(((float)adc_max_scale * (float)rawv) / (adc_multiplier * (float)adc_max_val_single));

//     rawv = analogReadDiff(PIN_PA1,PIN_PA2, ADC_RESOLUTION, 0);
//     v2 = round(((float)adc_max_scale * (float)rawv) / (adc_multiplier * (float)adc_max_val_diff));

//     oled.setFont(FONT6X8);
//     oled.setCursor(0, 0);
//     sprintf(buffer,"%05ld mV PA1",v1);
//     oled.print(buffer);
//     oled.setCursor(0, 1);
//     sprintf(buffer,"%05ld mV PA1-PA2",v2); // often non-zero, so calibrate that runtime! and 10k input impedance, not great.
//     oled.print(buffer);
//     //Serial.println(v);
// }

void setup_CommandParser(void) {
    lastCmd = cmd_none;
}

void loop_CommandParser(void) {
    lastCmd = getCommand(lastCmd);
    runCommand(lastCmd);
}

#endif
#pragma endregion

#pragma region Main
// ************************************************************************************************
// Main setup and loop
// ************************************************************************************************

void setup_serial(void) {
    Serial.begin(115200, (SERIAL_8N1));
}

void setup() {
    readSettings();
    setupIOPins();
    setup_serial();
    setup_screen();
    setup_ioexpander();
    setup_UI();
    setup_ADC(false);
}

void loop() {
    loop_UI();
    delayMicroseconds(10000);}
#endif

#pragma endregion
