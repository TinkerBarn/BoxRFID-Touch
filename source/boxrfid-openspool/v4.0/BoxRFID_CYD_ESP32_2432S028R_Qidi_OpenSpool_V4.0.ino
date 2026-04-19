/*
  BoxRFID Touch UI - ESP32-2432S028R (DIYMalls / CYD) + PN532 I2C
  ===========================================================================
  Version: V4.0

  Hardware:
    - ESP32-2432S028R (CYD 2.8" Resistive Touch, DIYmalls.com)
        - TFT: ILI9341 via TFT_eSPI
        - Touch: XPT2046 on separate VSPI bus
    - PN532: I2C via CN1 (SCL=IO22, SDA=IO27, 3.3V, GND)

  Libraries (Arduino IDE):
    - TFT_eSPI (Bodmer)
    - XPT2046_Touchscreen (Paul Stoffregen)
    - Adafruit PN532
    - Adafruit BusIO

  TFT_eSPI config:
    - Ensure your selected User_Setup matches the CYD DIYmalls pins (RNT setup file).

*/

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>
#include <ArduinoJson.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <SD.h>

#include <Preferences.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include <XPT2046_Touchscreen.h>

// OpenSpool save tier type for Arduino auto-prototypes
typedef uint8_t OpenSpoolSaveTier;
static const OpenSpoolSaveTier OS_TIER_NONE = 0;
static const OpenSpoolSaveTier OS_TIER_BASIC = 1;
static const OpenSpoolSaveTier OS_TIER_STANDARD = 2;
static const OpenSpoolSaveTier OS_TIER_ALL = 3;

// ==================== PN532 (I2C on CN1) ====================
#define PN532_SDA 27
#define PN532_SCL 22
Adafruit_PN532 nfc(PN532_SDA, PN532_SCL);

// ==================== TFT ====================
TFT_eSPI tft = TFT_eSPI();

// ==================== Touch ====================
#define XPT2046_IRQ  36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK  25
#define XPT2046_CS   33
#define SD_MOSI_PIN  23
#define SD_MISO_PIN  19
#define SD_SCK_PIN   18
#define SD_CS_PIN    5

SPIClass touchscreenSPI = SPIClass(VSPI);
SPIClass sdSPI = SPIClass(VSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);
WiFiServer uploadServer(80);

// ==================== Preferences ====================
Preferences prefs;

// Touch calibration storage
static const char* PREF_NS_TOUCH = "touch";
static const char* PREF_MINX = "minx";
static const char* PREF_MAXX = "maxx";
static const char* PREF_MINY = "miny";
static const char* PREF_MAXY = "maxy";
static const char* PREF_HAS_CAL = "hascal";

// UI storage
static const char* PREF_NS_UI = "ui";
static const char* PREF_UI_BLOB = "cfg";
static const char* PREF_NS_WIFI = "wifi";
static const char* PREF_NS_QIDI_CFG = "qdcfg";
static const char* PREF_LANG = "lang";
static const char* PREF_DISPLAY_INV = "dispinv";
static const char* PREF_AUTO_READ = "autoread";
static const char* PREF_DEFAULT_MODE = "defmode";
static const char* PREF_LAST_MODE = "lastmode";
static const char* PREF_LAST_QIDI_PRINTER = "lastqidi";
static const char* PREF_SCREENSAVER_MODE = "ssmode";
static const char* PREF_BRIGHTNESS = "bright";
static const char* PREF_OS_STD_NOZZLE = "osstdnoz";
static const char* PREF_OS_U1_BED = "osu1bed";
static const char* PREF_OS_U1_ALPHA = "osu1alp";
static const char* PREF_OS_U1_WEIGHT = "osu1wgt";
static const char* PREF_OS_U1_DIAM = "osu1dia";
static const char* PREF_OS_U1_ADDC = "osu1add";
static const char* PREF_OS_READ_INT = "osrdint";
static const char* PREF_WIFI_ENABLED = "wifi_on";
static const char* PREF_WIFI_SSID = "ssid";
static const char* PREF_WIFI_PASS = "pass";
static const char* PREF_QIDI_USE_CFG_P4 = "cfgp4";
static const char* PREF_QIDI_USE_CFG_Q2 = "cfgq2";
static const char* PREF_QIDI_USE_CFG_M4 = "cfgm4";

// Material storage
static const char* PREF_NS_MAT_QIDI = "matq";
static const char* PREF_NS_MAT_QIDI_Q2 = "matq2";
static const char* PREF_NS_MAT_QIDI_M4 = "matm4";
static const char* PREF_NS_MAT_OS = "mato";

// Manufacturer storage
static const char* PREF_NS_MFG_QIDI = "mfgq";
static const char* PREF_NS_MFG_QIDI_Q2 = "mfgq2";
static const char* PREF_NS_MFG_QIDI_M4 = "mfgm4";
static const char* PREF_NS_MFG_OS = "mfgo";
static const char* PREF_NS_VAR_OS = "varo";
static const char* PREF_QIDI_PRINTER = "qprinter";
static const char* PREF_LIST_BLOB = "blob";

// ==================== Touch calibration defaults ====================
static const int DEF_TS_MINX = 200;
static const int DEF_TS_MAXX = 3700;
static const int DEF_TS_MINY = 240;
static const int DEF_TS_MAXY = 3800;

static int TS_MINX = DEF_TS_MINX;
static int TS_MAXX = DEF_TS_MAXX;
static int TS_MINY = DEF_TS_MINY;
static int TS_MAXY = DEF_TS_MAXY;

static bool TOUCH_SWAP_XY = false;
static bool TOUCH_INVERT_X = false;
static bool TOUCH_INVERT_Y = false;

// ==================== Display ====================
static const uint8_t TFT_ROT = 1;
static int TFT_W = 320;
static int TFT_H = 240;

static const int UI_HEADER_H = 32;
static const int UI_STATUS_H = 24;

static const char* APP_VERSION = "V4.0";

static const int BL_PWM_FREQ = 5000;
static const int BL_PWM_RES = 8;

static uint8_t displayBrightness = 80;
static uint8_t currentBrightnessApplied = 80;


// ==================== RFID data format ====================
static const uint8_t DATA_BLOCK = 4;

// ==================== Manufacturer IDs ====================
enum ManufacturerId : uint8_t {
  MFG_GENERIC = 0,
  MFG_QIDI    = 1
};

enum TagMode : uint8_t { TAGMODE_QIDI = 0, TAGMODE_OPENSPOOL = 1 };
enum QidiPrinterModel : uint8_t { QIDI_MODEL_PLUS4 = 0, QIDI_MODEL_Q2 = 1, QIDI_MODEL_MAX4 = 2 };
static TagMode currentTagMode = TAGMODE_QIDI;
static TagMode defaultTagMode = TAGMODE_QIDI;
static QidiPrinterModel qidiPrinterModel = QIDI_MODEL_PLUS4;
static uint8_t setupPage = 0;

enum ScreensaverMode : uint8_t { SCREENSAVER_30S = 0, SCREENSAVER_1MIN = 1, SCREENSAVER_5MIN = 2, SCREENSAVER_10MIN = 3, SCREENSAVER_OFF = 4 };
static ScreensaverMode screensaverMode = SCREENSAVER_5MIN;
static bool screensaverActive = false;
static uint32_t lastUserActivityMs = 0;
static uint32_t lastScreensaverMoveMs = 0;
static int screensaverTextX = 20;
static int screensaverTextY = 120;

// ==================== Databases ====================
static const uint8_t MAX_MATERIALS = 128;
static const uint8_t MAX_MANUFACTURERS = 64;
static const uint8_t MAX_VARIANTS = 24;
static const uint8_t ITEM_NAME_MAX = 32;

static const char* OFFICIAL_CFG_DIR = "/qidi";
static const char* OFFICIAL_CFG_DIR_PLUS4 = "/qidi/plus4";
static const char* OFFICIAL_CFG_DIR_Q2 = "/qidi/q2";
static const char* OFFICIAL_CFG_DIR_MAX4 = "/qidi/max4";
static const char* OFFICIAL_CFG_PATH_PLUS4 = "/qidi/plus4/officiall_filas_list.cfg";
static const char* OFFICIAL_CFG_PATH_Q2 = "/qidi/q2/officiall_filas_list.cfg";
static const char* OFFICIAL_CFG_PATH_MAX4 = "/qidi/max4/officiall_filas_list.cfg";
static const uint32_t WIFI_CONNECT_TIMEOUT_MS = 8000UL;
static const uint32_t WIFI_RETRY_INTERVAL_MS = 15000UL;
static const uint32_t WIFI_HTTP_READ_TIMEOUT_MS = 8000UL;
static const uint32_t SD_SPI_FREQ_HZ = 4000000UL;
static const size_t WIFI_HTTP_MAX_UPLOAD_SIZE = 32768UL;
static const size_t WIFI_HTTP_CHUNK_SIZE = 384UL;

struct RuntimeItem {
  bool active;
  char name[ITEM_NAME_MAX + 1];
  uint16_t nozzleMin;
  uint16_t nozzleMax;
  uint16_t bedMin;
  uint16_t bedMax;
};

static RuntimeItem gMaterials[MAX_MATERIALS + 1];          // 1..50
static RuntimeItem gManufacturers[MAX_MANUFACTURERS + 1];  // 0..50
static RuntimeItem gVariants[MAX_VARIANTS + 1];            // 1..24

struct DefaultMaterialItem {
  const char* name;
  uint8_t val;
  uint16_t minT;
  uint16_t maxT;
};

static const DefaultMaterialItem DEFAULT_MATERIALS_QIDI[] = {
  {"PLA", 1, 190, 240},
  {"PETG", 41, 220, 270},
  {"ABS", 11, 240, 280},
  {"ASA", 18, 240, 280},
  {"ABS-GF", 12, 240, 280},
  {"ABS-Metal", 13, 240, 280},
  {"ABS-Odorless", 14, 240, 280},
  {"ASA-AERO", 19, 240, 280},
  {"PA12-CF", 25, 260, 300},
  {"PAHT-CF", 30, 300, 320},
  {"PAHT-GF", 31, 300, 320},
  {"PC/ABS-FR", 34, 260, 280},
  {"PET-CF", 37, 280, 320},
  {"PET-GF", 38, 280, 320},
  {"PETG Basic", 39, 240, 280},
  {"PETG Translucent", 45, 240, 280},
  {"PETG-Though", 40, 240, 275},
  {"PLA Basic", 7, 190, 240},
  {"PLA Matte", 2, 190, 240},
  {"PLA Matte Basic", 8, 190, 230},
  {"PLA Metal", 3, 190, 240},
  {"PLA Silk", 4, 190, 240},
  {"PLA-CF", 5, 210, 250},
  {"PLA-Wood", 6, 190, 240},
  {"PPS-CF", 44, 300, 350},
  {"PVA", 47, 210, 250},
  {"Support For PAHT", 32, 260, 280},
  {"Support For PET/PA", 33, 260, 280},
  {"TPU", 50, 200, 250},
  {"TPU-AERO", 49, 200, 250},
  {"UltraPA", 24, 260, 300},
  {"UltraPA-CF25", 26, 300, 320},
};
static const uint16_t DEFAULT_MATERIALS_QIDI_COUNT = sizeof(DEFAULT_MATERIALS_QIDI) / sizeof(DEFAULT_MATERIALS_QIDI[0]);

static const DefaultMaterialItem DEFAULT_MATERIALS_QIDI_Q2[] = {
  {"PLA Rapido", 1, 190, 240},
  {"PLA Matte", 2, 190, 240},
  {"PLA Metal", 3, 190, 240},
  {"PLA Silk", 4, 190, 240},
  {"PLA-CF", 5, 210, 250},
  {"PLA-Wood", 6, 190, 240},
  {"PLA Basic", 7, 190, 240},
  {"PLA Matte Basic", 8, 190, 230},
  {"Support For PLA", 10, 210, 240},
  {"ABS Rapido", 11, 240, 280},
  {"ABS-GF", 12, 240, 280},
  {"ABS-Metal", 13, 240, 280},
  {"ABS-Odorless", 14, 240, 280},
  {"ASA", 18, 240, 280},
  {"ASA-Aero", 19, 240, 280},
  {"ASA-CF", 20, 240, 280},
  {"PC", 23, 240, 280},
  {"UltraPA", 24, 260, 300},
  {"PA-CF", 25, 260, 300},
  {"UltraPA-CF25", 26, 300, 320},
  {"PA12-CF", 27, 260, 300},
  {"PAHT-CF", 30, 300, 320},
  {"PAHT-GF", 31, 300, 320},
  {"Support For PAHT", 32, 260, 280},
  {"Support For PET/PA", 33, 260, 280},
  {"PC/ABS-FR", 34, 260, 280},
  {"TPEE", 35, 230, 260},
  {"PEBA", 36, 230, 260},
  {"PET-CF", 37, 280, 320},
  {"PET-GF", 38, 280, 320},
  {"PETG Basic", 39, 240, 280},
  {"PETG-Tough", 40, 240, 275},
  {"PETG Rapido", 41, 220, 270},
  {"PETG-CF", 42, 240, 270},
  {"PETG-GF", 43, 240, 270},
  {"PPS-CF", 44, 300, 350},
  {"PETG Translucent", 45, 240, 280},
  {"PPS-GF", 46, 300, 350},
  {"PVA", 47, 210, 250},
  {"TPU-AERO 64D", 48, 200, 250},
  {"TPU-Aero", 49, 200, 250},
  {"TPU 95A-HF", 50, 200, 250},
};
static const uint16_t DEFAULT_MATERIALS_QIDI_Q2_COUNT = sizeof(DEFAULT_MATERIALS_QIDI_Q2) / sizeof(DEFAULT_MATERIALS_QIDI_Q2[0]);

static const DefaultMaterialItem DEFAULT_MATERIALS_OS[] = {
  {"PLA", 1, 190, 220},
  {"PETG", 2, 220, 250},
  {"ABS", 3, 230, 260},
  {"ASA", 4, 240, 270},
  {"TPU", 5, 210, 230},
  {"PA", 6, 240, 270},
  {"PA12", 7, 240, 270},
  {"PC", 8, 270, 310},
  {"PEEK", 9, 360, 400},
  {"PVA", 10, 190, 220},
  {"HIPS", 11, 230, 250},
  {"PCTG", 12, 220, 250},
  {"PLA-CF", 13, 190, 220},
  {"PETG-CF", 14, 230, 260},
  {"PA-CF", 15, 250, 280},
};
static const uint16_t DEFAULT_MATERIALS_OS_COUNT = sizeof(DEFAULT_MATERIALS_OS) / sizeof(DEFAULT_MATERIALS_OS[0]);

struct DefaultManufacturerItem {
  const char* name;
  uint8_t val;
};

static const DefaultManufacturerItem DEFAULT_MANUFACTURERS_QIDI[] = {
  {"Generic", 0},
  {"QIDI", 1},
};
static const uint16_t DEFAULT_MANUFACTURERS_QIDI_COUNT = sizeof(DEFAULT_MANUFACTURERS_QIDI) / sizeof(DEFAULT_MANUFACTURERS_QIDI[0]);

static const DefaultManufacturerItem DEFAULT_MANUFACTURERS_OS[] = {
  {"Generic", 0},
  {"Snapmaker", 1},
  {"SUNLU", 2},
  {"eSun", 3},
  {"Jayo", 4},
  {"QIDI", 5},
  {"Bambu Lab", 6},
  {"Polymaker", 7},
  {"TECBEARS", 8},
  {"GIANTARM", 9},
  {"HATCHBOX", 10},
  {"Overture", 11},
  {"Prusament", 12},
  {"TINMORRY", 13},
  {"Kingroon", 14},
  {"Elegoo", 15},
  {"Creality", 16},
  {"Deeplee", 17},
  {"ANYCUBIC", 18},
  {"FLASHFORGE", 19},
  {"CC3D", 20},
  {"ZIRO", 21},
};
static const uint16_t DEFAULT_MANUFACTURERS_OS_COUNT = sizeof(DEFAULT_MANUFACTURERS_OS) / sizeof(DEFAULT_MANUFACTURERS_OS[0]);

static const uint8_t DISPLAY_ORDER_MANUFACTURERS_OS[] = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21
};
static const uint16_t DISPLAY_ORDER_MANUFACTURERS_OS_COUNT = sizeof(DISPLAY_ORDER_MANUFACTURERS_OS) / sizeof(DISPLAY_ORDER_MANUFACTURERS_OS[0]);


// ==================== Language ====================
enum UiLang : uint8_t { LANG_DE=0, LANG_EN=1, LANG_ES=2, LANG_PT=3, LANG_FR=4, LANG_IT=5, LANG_COUNT=6 };
static UiLang uiLang = LANG_EN;

static const char* const LANG_NAMES[LANG_COUNT] = {
  "Deutsch", "English", "Espanol", "Portugues", "Francais", "Italiano"
};

enum UiStrId : uint16_t {
  STR_MAIN_TITLE,
  STR_READ_TAG,
  STR_WRITE_TAG,
  STR_AUTO_ON,
  STR_AUTO_OFF,
  STR_READY_READ,
  STR_CONFIGURE,
  STR_SELECT_MATERIAL,
  STR_SELECT_COLOR,
  STR_WAIT_TAG,
  STR_NFC_BUSY,
  STR_NO_TAG,
  STR_AUTH_FAILED,
  STR_READ_FAILED,
  STR_READ_TAG_DETECTED,
  STR_WRITE_FAILED,
  STR_WRITE_OK,
  STR_AUTO_TAG_DETECTED,
  STR_BACK,
  STR_TAG_INFO_TITLE,
  STR_LABEL_MANUFACTURER,
  STR_LABEL_MATERIAL,
  STR_LABEL_COLOR,
  STR_PN532_NOT_FOUND,
  STR_ACTION_READ,
  STR_ACTION_WRITE,
  STR_SETUP,
  STR_LANGUAGE,
  STR_SELECT_LANGUAGE,
  STR_TOUCH_CALIBRATION,
  STR_CALIBRATE,
  STR_CALIBRATE_HINT,
  STR_CALIBRATION_SAVED,
  STR_CALIBRATION_ABORTED,
  STR_FACTORY_DEFAULTS,
  STR_FACTORY_RESET_DONE,
  STR_COLOR_WHITE,
  STR_COLOR_BLACK,
  STR_COLOR_GRAY,
  STR_COLOR_LIGHT_GREEN,
  STR_COLOR_MINT,
  STR_COLOR_BLUE,
  STR_COLOR_PINK,
  STR_COLOR_YELLOW,
  STR_COLOR_GREEN,
  STR_COLOR_LIGHT_BLUE,
  STR_COLOR_DARK_BLUE,
  STR_COLOR_LAVENDER,
  STR_COLOR_LIME,
  STR_COLOR_ROYAL_BLUE,
  STR_COLOR_SKY_BLUE,
  STR_COLOR_VIOLET,
  STR_COLOR_ROSE,
  STR_COLOR_RED,
  STR_COLOR_BEIGE,
  STR_COLOR_SILVER,
  STR_COLOR_BROWN,
  STR_COLOR_KHAKI,
  STR_COLOR_ORANGE,
  STR_COLOR_BRONZE,
  STR_COUNT
};

static const char* const UI_STR[LANG_COUNT][STR_COUNT] = {
  {
    "BoxRFID - Hauptmenue","Tag Lesen","Tag Schreiben","Auto: AN","Auto: AUS","Bereit zum Lesen",
    "Konfiguration","Material auswaehlen","Farbe auswaehlen","Warte auf Tag...","NFC beschaeftigt!","Kein Tag gefunden","Auth fehlgeschlagen!",
    "Lesen fehlgeschlagen!","Lesen: Tag erkannt","Schreiben fehlgeschlagen!","Schreiben erfolgreich!","Auto: Tag erkannt","Zurueck",
    "Tag Informationen","Hersteller","Material","Farbe","FEHLER: PN532 nicht gefunden!","Lesen","Schreiben","Einstellungen","Sprache","Sprache waehlen",
    "Kalibrierung","Kalibrieren","Druecke jeden Punkt mind. 1 Sekunde!","Kalibrierung gespeichert","Kalibrierung abgebrochen",
    "Werkseinstellung","Werkseinstellungen geladen","Weiss","Schwarz","Grau","Hellgruen","Mint","Blau","Pink","Gelb","Gruen","Hellblau",
    "Dunkelblau","Lavendel","Lime","Royalblau","Himmelblau","Violett","Rosa","Rot","Beige","Silber","Braun","Khaki","Orange","Bronze"
  },
  {
    "BoxRFID - Main Menu","Read Tag","Write Tag","Auto: ON","Auto: OFF","Ready to read",
    "Configure","Select material","Select color","Waiting for tag...","NFC busy!","No tag found","Auth failed!",
    "Read failed!","Read: tag detected","Write failed!","Write successful!","Auto: tag detected","Back",
    "Tag information","Manufacturer","Material","Color","ERROR: PN532 not found!","Read","Write","Settings","Language","Select language",
    "Calibration","Calibrate","Press each point for at least 1 second!","Calibration saved","Calibration aborted",
    "Factory default","Factory defaults restored","White","Black","Gray","Light Green","Mint","Blue","Pink","Yellow","Green","Light Blue",
    "Dark Blue","Lavender","Lime","Royal Blue","Sky Blue","Violet","Rose","Red","Beige","Silver","Brown","Khaki","Orange","Bronze"
  },
  {
    "BoxRFID - Menu","Leer Tag","Escribir Tag","Auto: ON","Auto: OFF","Listo para leer",
    "Configurar","Elegir material","Elegir color","Esperando tag...","NFC ocupado!","No se encontro tag","Fallo de auth!",
    "Fallo de lectura!","Leido: tag detectado","Fallo de escritura!","Escritura OK!","Auto: tag detectado","Atras",
    "Info del tag","Fabricante","Material","Color","ERROR: PN532 no encontrado!","Leer","Escribir","Ajustes","Idioma","Elegir idioma",
    "Calibracion","Calibrar","Manten cada punto al menos 1 segundo!","Calibracion guardada","Calibracion cancelada",
    "Ajustes fabrica","Ajustes restaurados","Blanco","Negro","Gris","Verde claro","Menta","Azul","Rosa","Amarillo","Verde","Azul claro",
    "Azul oscuro","Lavanda","Lima","Azul rey","Azul cielo","Violeta","Rosado","Rojo","Beige","Plata","Marron","Caqui","Naranja","Bronce"
  },
  {
    "BoxRFID - Menu","Ler Tag","Escrever Tag","Auto: ON","Auto: OFF","Pronto para ler",
    "Configurar","Escolher material","Escolher cor","Aguardando tag...","NFC ocupado!","Nenhum tag","Falha de auth!",
    "Falha na leitura!","Leitura: tag detectado","Falha na escrita!","Escrita OK!","Auto: tag detectado","Voltar",
    "Info do tag","Fabricante","Material","Cor","ERRO: PN532 nao encontrado!","Ler","Escrever","Definicoes","Idioma","Selecionar idioma",
    "Calibracao","Calibrar","Pressione cada ponto por 1 segundo!","Calibracao salva","Calibracao cancelada",
    "Padrao fabrica","Padrao restaurado","Branco","Preto","Cinza","Verde claro","Menta","Azul","Rosa","Amarelo","Verde","Azul claro",
    "Azul escuro","Lavanda","Lima","Azul royal","Azul ceu","Violeta","Rose","Vermelho","Bege","Prata","Marrom","Caqui","Laranja","Bronze"
  },
  {
    "BoxRFID - Menu","Lire Tag","Ecrire Tag","Auto: ON","Auto: OFF","Pret a lire",
    "Configurer","Choisir mat.","Choisir couleur","Attente du tag...","NFC occupe!","Aucun tag","Auth echouee!",
    "Lecture echouee!","Lu: tag detecte","Ecriture echouee!","Ecriture OK!","Auto: tag detecte","Retour",
    "Info du tag","Fabricant","Materiau","Couleur","ERREUR: PN532 introuvable!","Lire","Ecrire","Reglages","Langue","Choisir langue",
    "Calibration","Calibrer","Appuyez 1 seconde sur chaque point!","Calibration enregistree","Calibration annulee",
    "Param. usine","Parametres restaures","Blanc","Noir","Gris","Vert clair","Menthe","Bleu","Rose","Jaune","Vert","Bleu clair",
    "Bleu fonce","Lavande","Citron vert","Bleu royal","Bleu ciel","Violet","Rose","Rouge","Beige","Argent","Marron","Kaki","Orange","Bronze"
  },
  {
    "BoxRFID - Menu principale","Leggi tag","Scrivi tag","Auto: ON","Auto: OFF","Pronto per leggere",
    "Configura","Seleziona materiale","Seleziona colore","In attesa del tag...","NFC occupato!","Nessun tag trovato","Auth fallita!",
    "Lettura fallita!","Lettura: tag rilevato","Scrittura fallita!","Scrittura riuscita!","Auto: tag rilevato","Indietro",
    "Informazioni tag","Produttore","Materiale","Colore","ERRORE: PN532 non trovato!","Leggi","Scrivi","Impostazioni","Lingua","Seleziona lingua",
    "Calibrazione","Calibra","Premi ogni punto per almeno 1 secondo!","Calibrazione salvata","Calibrazione annullata",
    "Impost. fabbrica","Impostazioni ripristinate","Bianco","Nero","Grigio","Verde chiaro","Menta","Blu","Rosa","Giallo","Verde","Azzurro",
    "Blu scuro","Lavanda","Lime","Blu reale","Blu cielo","Viola","Rosato","Rosso","Beige","Argento","Marrone","Khaki","Arancione","Bronzo"
  }
};

static inline const char* TR(UiStrId id) {
  return UI_STR[(uint8_t)uiLang][(uint16_t)id];
}

// ==================== Extra translated text ====================
static const char* const TXT_MATERIAL[LANG_COUNT]          = {"Material","Material","Material","Material","Materiau","Materiale"};
static const char* const TXT_MANUFACTURER[LANG_COUNT]      = {"Hersteller","Manufacturer","Fabricante","Fabricante","Fabricant","Produttore"};
static const char* const TXT_CALIBRATION[LANG_COUNT]       = {"Kalibrierung","Calibration","Calibracion","Calibracao","Calibration","Calibrazione"};
static const char* const TXT_APP_PREFIX[LANG_COUNT]        = {"App: ","App: ","App: ","App: ","App: ","App: "};
static const char* const TXT_DISPLAY_INV[LANG_COUNT]        = {"Inversion","Inversion","Inversion","Inversao","Inversion","Inversione"};
static const char* const TXT_DEFAULT_MODE[LANG_COUNT]        = {"Default Mode","Default Mode","Modo por defecto","Modo padrao","Mode par defaut","Modalita predefinita"};
static const char* const TXT_PRINTER_MODEL[LANG_COUNT]       = {"Druckermodell","Printer model","Modelo impresora","Modelo impressora","Modele imprimante","Modello stampante"};
static const char* const TXT_QIDI_PLUS4[LANG_COUNT]          = {"Plus 4","Plus 4","Plus 4","Plus 4","Plus 4","Plus 4"};
static const char* const TXT_QIDI_Q2[LANG_COUNT]             = {"Q2","Q2","Q2","Q2","Q2","Q2"};
static const char* const TXT_QIDI_MAX4[LANG_COUNT]           = {"Max 4","Max 4","Max 4","Max 4","Max 4","Max 4"};
static const char* const TXT_SCREENSAVER[LANG_COUNT]         = {"Bildschirmschoner","Screensaver","Protector","Protetor","Ecran veille","Salvaschermo"};
static const char* const TXT_BRIGHTNESS[LANG_COUNT]         = {"Helligkeit","Brightness","Brillo","Brilho","Luminosite","Luminosita"};
static const char* const TXT_SAVER_30S[LANG_COUNT]           = {"30s","30s","30s","30s","30s","30s"};
static const char* const TXT_SAVER_1MIN[LANG_COUNT]          = {"1min","1min","1min","1min","1min","1min"};
static const char* const TXT_SAVER_5MIN[LANG_COUNT]          = {"5min","5min","5min","5min","5min","5min"};
static const char* const TXT_SAVER_10MIN[LANG_COUNT]         = {"10min","10min","10min","10min","10min","10min"};
static const char* const TXT_ON[LANG_COUNT]                 = {"AN","ON","ON","ON","ON","ON"};
static const char* const TXT_OFF[LANG_COUNT]                = {"AUS","OFF","OFF","OFF","OFF","OFF"};
static const char* const TXT_BRAND[LANG_COUNT]              = {"Hersteller","Brand","Marca","Marca","Marque","Marca"};
static const char* const TXT_TYPE[LANG_COUNT]               = {"Typ","Type","Tipo","Tipo","Type","Tipo"};
static const char* const TXT_SUBTYPE[LANG_COUNT]            = {"Subtype","Subtype","Subtipo","Subtipo","Sous-type","Sottotipo"};
static const char* const TXT_COLOR_HEX[LANG_COUNT]          = {"Farbe","Color","Color","Cor","Couleur","Colore"};
static const char* const TXT_NOZZLE_MIN[LANG_COUNT]         = {"Nozzle min","Nozzle min","Boquilla min","Bico min","Buse min","Ugello min"};
static const char* const TXT_NOZZLE_TEMP[LANG_COUNT]        = {"Nozzle Temp.","Nozzle Temp.","Temp. boquilla","Temp. bico","Temp. buse","Temp. ugello"};
static const char* const TXT_NOZZLE_MAX[LANG_COUNT]         = {"Nozzle max","Nozzle max","Boquilla max","Bico max","Buse max","Ugello max"};
static const char* const TXT_BED_MIN[LANG_COUNT]            = {"Bed min","Bed min","Cama min","Mesa min","Lit min","Piatto min"};
static const char* const TXT_BED_MAX[LANG_COUNT]            = {"Bed max","Bed max","Cama max","Mesa max","Lit max","Piatto max"};
static const char* const TXT_ALPHA[LANG_COUNT]              = {"Deckkraft","Opacity","Opacidad","Opacidade","Opacité","Opacità"};
static const char* const TXT_WEIGHT_G[LANG_COUNT]           = {"Gewicht (g)","Weight (g)","Peso (g)","Peso (g)","Poids (g)","Peso (g)"};
static const char* const TXT_DIAMETER[LANG_COUNT]           = {"Durchmesser","Diameter","Diametro","Diametro","Diametre","Diametro"};
static const char* const TXT_ADD_COLOR1[LANG_COUNT]         = {"Zusatzfarbe 1","Add color 1","Color adic. 1","Cor adic. 1","Couleur add. 1","Colore agg. 1"};
static const char* const TXT_ADD_COLOR2[LANG_COUNT]         = {"Zusatzfarbe 2","Add color 2","Color adic. 2","Cor adic. 2","Couleur add. 2","Colore agg. 2"};
static const char* const TXT_ADD_COLOR3[LANG_COUNT]         = {"Zusatzfarbe 3","Add color 3","Color adic. 3","Cor adic. 3","Couleur add. 3","Colore agg. 3"};
static const char* const TXT_ADD_COLOR4[LANG_COUNT]         = {"Zusatzfarbe 4","Add color 4","Color adic. 4","Cor adic. 4","Couleur add. 4","Colore agg. 4"};
static const char* const TXT_ORCA_NAME[LANG_COUNT]          = {"Orca Name","Orca Name","Nombre Orca","Nome Orca","Nom Orca","Nome Orca"};
static const char* const TXT_SLICER[LANG_COUNT]             = {"Slicer","Slicer","Slicer","Slicer","Slicer","Slicer"};
static const char* const TXT_FILAMENT_PROFILE[LANG_COUNT]   = {"Filamentprofil Name","Filament profile name","Nombre perfil filamento","Nome perfil filamento","Nom profil filament","Nome profilo filamento"};
static const char* const TXT_WRITE[LANG_COUNT]              = {"Schreiben","Write","Escribir","Escrever","Ecrire","Scrivi"};
static const char* const TXT_TAG_CLEAR[LANG_COUNT]          = {"Tag loeschen","Clear tag","Borrar tag","Limpar tag","Effacer tag","Cancella tag"};
static const char* const TXT_NUMPAD[LANG_COUNT]             = {"Zahlen","Numbers","Numeros","Numeros","Nombres","Numeri"};
static const char* const TXT_TAG_DETECTED[LANG_COUNT]       = {"Tag erkannt","Tag detected","Tag detectado","Tag detectado","Tag detecte","Tag rilevato"};
static const char* const TXT_READING_TAG[LANG_COUNT]       = {"Tag lesen...","Reading Tag...","Leyendo tag...","Lendo tag...","Lecture du tag...","Lettura tag..."};
static const char* const TXT_WRITING_TAG[LANG_COUNT]       = {"Tag schreiben...","Writing Tag...","Escribiendo tag...","Escrevendo tag...","Ecriture du tag...","Scrittura tag..."};
static const char* const TXT_TAG_READ[LANG_COUNT]          = {"Tag gelesen","Tag read","Tag leido","Tag lido","Tag lu","Tag letto"};
static const char* const TXT_READ_CANCELED[LANG_COUNT]      = {"Tag lesen abgebrochen","Tag read canceled","Lectura cancelada","Leitura cancelada","Lecture annulee","Lettura annullata"};
static const char* const TXT_MEM_WARN_TITLE[LANG_COUNT]     = {"Speicherwarnung","Memory warning","Aviso memoria","Aviso memoria","Alerte memoire","Avviso memoria"};
static const char* const TXT_MEM_WARN_STD1[LANG_COUNT]      = {"Nicht alle Werte passen auf den Tag.","Not all values fit on the tag.","No todos los valores caben en el tag.","Nem todos os valores cabem na tag.","Toutes les valeurs ne tiennent pas sur le tag.","Non tutti i valori entrano nel tag."};
static const char* const TXT_MEM_WARN_STD2[LANG_COUNT]      = {"Es werden Basiswerte + Standard", "Basic + standard values will be saved", "Se guardaran valores basicos + estandar", "Serao gravados valores basicos + padrao", "Les valeurs de base + standard seront enregistrees", "Saranno salvati valori base + standard"};
static const char* const TXT_MEM_WARN_STD3[LANG_COUNT]      = {"gespeichert.",".",".",".",".","."};
static const char* const TXT_MEM_WARN_BASIC1[LANG_COUNT]    = {"Nur die Basiswerte passen auf den Tag.","Only the basic values fit on the tag.","Solo caben los valores basicos.","So os valores basicos cabem na tag.","Seules les valeurs de base tiennent sur le tag.","Solo i valori base entrano nel tag."};
static const char* const TXT_MEM_WARN_BASIC2[LANG_COUNT]    = {"Optionale Felder werden weggelassen.","Optional fields will be omitted.","Se omitiran los campos opcionales.","Campos opcionais serao omitidos.","Les champs optionnels seront ignores.","I campi opzionali saranno omessi."};
static const char* const TXT_NO_SPACE1[LANG_COUNT]          = {"Zu viele Daten fuer diesen Tag.","Too much data for this tag.","Demasiados datos para este tag.","Dados demais para esta tag.","Trop de donnees pour ce tag.","Troppi dati per questo tag."};
static const char* const TXT_NO_SPACE2[LANG_COUNT]          = {"Bitte weniger Werte eingeben.","Please enter fewer values.","Introduzca menos valores.","Introduza menos valores.","Veuillez saisir moins de valeurs.","Inserisci meno valori."};

static const char* const TXT_OS_STANDARD[LANG_COUNT]       = {"OpenSpool Standard","OpenSpool Standard","OpenSpool Estandar","OpenSpool Padrao","OpenSpool Standard","OpenSpool Standard"};
static const char* const TXT_OS_U1[LANG_COUNT]             = {"OpenSpool Snapmaker U1","OpenSpool Snapmaker U1","OpenSpool Snapmaker U1","OpenSpool Snapmaker U1","OpenSpool Snapmaker U1","OpenSpool Snapmaker U1"};
static const char* const TXT_OS_EXTENDED[LANG_COUNT]       = {"OpenSpool Erweitert","OpenSpool Extended","OpenSpool Avanzado","OpenSpool Avancado","OpenSpool Avance","OpenSpool Avanzato"};
static const char* const TXT_TAG_RESET_VALUES[LANG_COUNT]   = {"Tag Werte zuruecksetzen","Reset tag values","Restablecer valores","Redefinir valores","Reinitialiser valeurs","Reimposta valori"};
static const char* const TXT_VARIANT[LANG_COUNT]           = {"Variante","Variant","Variante","Variante","Variante","Variante"};
static const char* const TXT_NONE[LANG_COUNT]              = {"Keine","None","Ninguno","Nenhum","Aucun","Nessuno"};
static const char* const TXT_CP_BTN[LANG_COUNT]            = {"Farbwahl","Colorpicker","Colorpicker","Colorpicker","Colorpicker","Colorpicker"};
static const char* const TXT_CP_TITLE[LANG_COUNT]          = {"Farbauswahl","Color Picker","Selector de Color","Seletor de Cor","Selecteur Couleur","Selettore Colore"};
static const char* const TXT_SET_TAG_INFO[LANG_COUNT]      = {"Tag Informationen setzen","Set Tag Information","Configurar info tag","Definir info tag","Definir info tag","Imposta info tag"};
static const char* const TXT_SHOW_NOZZLE_INFO[LANG_COUNT]  = {"Nozzle Temp anzeigen","Show nozzle temp","Mostrar temp. boquilla","Mostrar temp. bico","Afficher temp. buse","Mostra temp. ugello"};
static const char* const TXT_SHOW_BED_INFO[LANG_COUNT]     = {"Bed Temp anzeigen","Show bed temp","Mostrar temp. cama","Mostrar temp. mesa","Afficher temp. lit","Mostra temp. piatto"};
static const char* const TXT_SHOW_ALPHA_INFO[LANG_COUNT]   = {"Deckkraft anzeigen","Show opacity","Mostrar opacidad","Mostrar opacidade","Afficher opacité","Mostra opacità"};
static const char* const TXT_ALPHA_HEX[LANG_COUNT]         = {"Deckkraft HEX","Opacity HEX","Opacidad HEX","Opacidade HEX","Opacité HEX","Opacità HEX"};
static const char* const TXT_SHOW_WEIGHT_INFO[LANG_COUNT]  = {"Gewicht anzeigen","Show weight","Mostrar peso","Mostrar peso","Afficher poids","Mostra peso"};
static const char* const TXT_SHOW_DIAM_INFO[LANG_COUNT]    = {"Durchmesser anzeigen","Show diameter","Mostrar diametro","Mostrar diametro","Afficher diametre","Mostra diametro"};
static const char* const TXT_SHOW_ADDC_INFO[LANG_COUNT]    = {"Weitere Farben anzeigen","Show extra colors","Mostrar colores extra","Mostrar cores extras","Afficher couleurs extras","Mostra colori extra"};
static const char* const TXT_INTERVAL[LANG_COUNT]          = {"Intervall","Interval","Intervalo","Intervalo","Intervalle","Intervallo"};
static const char* const TXT_ADDC_TITLE[LANG_COUNT]        = {"Zusatzfarben","Additional Colors","Colores extra","Cores extras","Couleurs extras","Colori extra"};
static const char* const TXT_ADD_COLOR1_SHORT[LANG_COUNT]  = {"Z-Farbe 1","Add col 1","Col ext 1","Cor ext 1","Coul add 1","Col agg 1"};
static const char* const TXT_ADD_COLOR2_SHORT[LANG_COUNT]  = {"Z-Farbe 2","Add col 2","Col ext 2","Cor ext 2","Coul add 2","Col agg 2"};
static const char* const TXT_ADD_COLOR3_SHORT[LANG_COUNT]  = {"Z-Farbe 3","Add col 3","Col ext 3","Cor ext 3","Coul add 3","Col agg 3"};
static const char* const TXT_ADD_COLOR4_SHORT[LANG_COUNT]  = {"Z-Farbe 4","Add col 4","Col ext 4","Cor ext 4","Coul add 4","Col agg 4"};
static const char* const TXT_ACTIVE[LANG_COUNT]            = {"Aktiv","Active","Activo","Ativo","Actif","Attivo"};

static const char* const TXT_MATERIAL_LIST[LANG_COUNT]     = {"Materialliste","Material list","Lista materiales","Lista materiais","Liste materiaux","Lista materiali"};
static const char* const TXT_MATERIAL_EDIT[LANG_COUNT]     = {"Material aendern","Edit material","Editar material","Editar material","Modifier materiau","Modifica materiale"};
static const char* const TXT_MATERIAL_NEW[LANG_COUNT]      = {"Neues Material","New material","Nuevo material","Novo material","Nouveau materiau","Nuovo materiale"};
static const char* const TXT_MATERIAL_RESET[LANG_COUNT]    = {"Auslieferungszustand","Factory default","Restaurar fabrica","Restaurar fabrica","Etat usine","Stato di fabbrica"};

static const char* const TXT_MFG_LIST[LANG_COUNT]          = {"Herstellerliste","Manufacturer list","Lista fabricantes","Lista fabricantes","Liste fabricants","Lista produttori"};
static const char* const TXT_MFG_EDIT[LANG_COUNT]          = {"Hersteller aendern","Edit manufacturer","Editar fabricante","Editar fabricante","Modifier fabricant","Modifica produttore"};
static const char* const TXT_MFG_NEW[LANG_COUNT]           = {"Neuer Hersteller","New manufacturer","Nuevo fabricante","Novo fabricante","Nouveau fabricant","Nuovo produttore"};
static const char* const TXT_MFG_RESET[LANG_COUNT]         = {"Auslieferungszustand","Factory default","Restaurar fabrica","Restaurar fabrica","Etat usine","Stato di fabbrica"};

static const char* const TXT_VARIANT_LIST[LANG_COUNT]      = {"Variantenliste","Variant list","Lista variantes","Lista variantes","Liste variantes","Lista varianti"};
static const char* const TXT_VARIANT_EDIT[LANG_COUNT]      = {"Variante aendern","Edit variant","Editar variante","Editar variante","Modifier variante","Modifica variante"};
static const char* const TXT_VARIANT_NEW[LANG_COUNT]       = {"Neue Variante","New variant","Nueva variante","Nova variante","Nouvelle variante","Nuova variante"};
static const char* const TXT_VARIANT_RESET[LANG_COUNT]     = {"Auslieferungszustand","Factory default","Restaurar fabrica","Restaurar fabrica","Etat usine","Stato di fabbrica"};
static const char* const TXT_PAGE_MATERIALS[LANG_COUNT]    = {"Materialien","Materials","Materiales","Materiais","Materiaux","Materiali"};
static const char* const TXT_PAGE_VARIANTS[LANG_COUNT]     = {"Varianten","Variants","Variantes","Variantes","Variantes","Varianti"};

static const char* const TXT_SELECT_ITEM[LANG_COUNT]       = {"Auswaehlen","Select","Seleccionar","Selecionar","Choisir","Seleziona"};
static const char* const TXT_NUMBER[LANG_COUNT]            = {"Nummer:","Number:","Numero:","Numero:","Numero:","Numero:"};
static const char* const TXT_NAME[LANG_COUNT]              = {"Name:","Name:","Nombre:","Nome:","Nom:","Nome:"};
static const char* const TXT_SAVE[LANG_COUNT]              = {"Speichern","Save","Guardar","Salvar","Enregistrer","Salva"};
static const char* const TXT_DELETE[LANG_COUNT]            = {"Loeschen","Delete","Eliminar","Excluir","Supprimer","Elimina"};
static const char* const TXT_DELETE_Q1_VAR[LANG_COUNT]     = {"Variante wirklich","Really delete this","Eliminar realmente esta","Excluir realmente esta","Supprimer vraiment cette","Eliminare davvero questa"};
static const char* const TXT_DELETE_Q2_VAR[LANG_COUNT]     = {"loeschen?","variant?","variante?","variante?","variante ?","variante?"};
static const char* const TXT_DELETE_Q1_MAT[LANG_COUNT]     = {"Material wirklich","Really delete this","Eliminar realmente este","Excluir realmente este","Supprimer vraiment ce","Eliminare davvero questo"};
static const char* const TXT_DELETE_Q2_MAT[LANG_COUNT]     = {"loeschen?","material?","material?","material?","materiau ?","materiale?"};
static const char* const TXT_DELETE_Q1_MFG[LANG_COUNT]     = {"Hersteller wirklich","Really delete this","Eliminar realmente este","Excluir realmente este","Supprimer vraiment ce","Eliminare davvero questo"};
static const char* const TXT_DELETE_Q2_MFG[LANG_COUNT]     = {"loeschen?","manufacturer?","fabricante?","fabricante?","fabricant ?","produttore?"};
static const char* const TXT_CHANGE_NAME[LANG_COUNT]       = {"Name aendern","Change name","Cambiar nombre","Alterar nome","Changer nom","Cambia nome"};
static const char* const TXT_ENTER_NAME[LANG_COUNT]        = {"Name eingeben","Enter name","Introducir nombre","Inserir nome","Entrer nom","Inserisci nome"};
static const char* const TXT_CHOOSE_FREE[LANG_COUNT]       = {"Freie Nummer waehlen","Choose free number","Elegir numero libre","Escolher numero livre","Choisir numero libre","Scegli numero libero"};
static const char* const TXT_CONFIRM[LANG_COUNT]           = {"Bitte bestaetigen","Please confirm","Confirmar","Confirmar","Confirmer","Confermare"};
static const char* const TXT_YES[LANG_COUNT]               = {"Ja","Yes","Si","Sim","Oui","Si"};
static const char* const TXT_NO[LANG_COUNT]                = {"Nein","No","No","Nao","Non","No"};
static const char* const TXT_NOTICE[LANG_COUNT]            = {"Bitte beachten!","Please note!","Atencion!","Atencao!","Attention !","Attenzione!"};
static const char* const TXT_NOTE_STATUS[LANG_COUNT]       = {"Hinweis","Notice","Aviso","Aviso","Avis","Avviso"};
static const char* const TXT_OK[LANG_COUNT]                = {"OK","OK","OK","OK","OK","OK"};

static const char* const TXT_MAT_RESET_Q1[LANG_COUNT]      = {"Materialliste auf","Reset material list","Restaurar lista","Restaurar lista","Reinitialiser la liste","Ripristinare elenco"};
static const char* const TXT_MAT_RESET_Q2[LANG_COUNT]      = {"Auslieferungszustand","to factory default","de materiales","de materiais","des materiaux","materiali"};
static const char* const TXT_MAT_RESET_Q3[LANG_COUNT]      = {"zuruecksetzen?","settings?","de fabrica?","de fabrica?","usine ?","di fabbrica?"};

static const char* const TXT_MFG_RESET_Q1[LANG_COUNT]      = {"Herstellerliste auf","Reset manufacturer list","Restaurar lista","Restaurar lista","Reinitialiser la liste","Ripristinare elenco"};
static const char* const TXT_MFG_RESET_Q2[LANG_COUNT]      = {"Auslieferungszustand","to factory default","de fabricantes","de fabricantes","des fabricants","produttori"};
static const char* const TXT_MFG_RESET_Q3[LANG_COUNT]      = {"zuruecksetzen?","settings?","de fabrica?","de fabrica?","usine ?","di fabbrica?"};

static const char* const TXT_VAR_RESET_Q1[LANG_COUNT]      = {"Variantenliste auf","Reset variant list","Restaurar lista","Restaurar lista","Reinitialiser la liste","Ripristinare elenco"};
static const char* const TXT_VAR_RESET_Q2[LANG_COUNT]      = {"Auslieferungszustand","to factory default","de variantes","de variantes","des variantes","varianti"};
static const char* const TXT_VAR_RESET_Q3[LANG_COUNT]      = {"zuruecksetzen?","settings?","de fabrica?","de fabrica?","usine ?","di fabbrica?"};

static const char* const TXT_FACTORY_RESET_TITLE[LANG_COUNT]  = {"Werkseinstellung","Factory default","Ajustes fabrica","Padrao fabrica","Param. usine","Impost. fabbrica"};
static const char* const TXT_FACTORY_RESET_Q1[LANG_COUNT]     = {"Wollen Sie alle","Reset all settings","Restablecer todos","Repor todas","Reinitialiser tous","Ripristinare tutte le"};
static const char* const TXT_FACTORY_RESET_Q2[LANG_COUNT]     = {"Einstellungen auf","to factory default","los ajustes a","as definicoes para","les parametres","impostazioni di"};
static const char* const TXT_FACTORY_RESET_Q3[LANG_COUNT]     = {"Auslieferungszustand zuruecksetzen?","settings?","fabrica?","fabrica?","usine ?","fabbrica?"};

static const char* const TXT_MAT_NOTICE1[LANG_COUNT]       = {"Damit das Material","For this material","Para que el material","Para que o material","Pour que le materiau","Per rendere il materiale"};
static const char* const TXT_MFG_NOTICE1[LANG_COUNT]       = {"Damit der Hersteller","For this manufacturer","Para que el fabricante","Para que o fabricante","Pour que le fabricant","Per rendere il produttore"};
static const char* const TXT_NOTICE2[LANG_COUNT]           = {"im Drucker verfuegbar","to be available","este disponible","fique disponivel","soit disponible","disponibile sulla"};
static const char* const TXT_NOTICE3[LANG_COUNT]           = {"ist, officiall_filas_list.cfg","on the printer,","en la impresora,","na impressora,","sur l'imprimante,","stampante,"};
static const char* const TXT_NOTICE4[LANG_COUNT]           = {"in Klipper anpassen, speichern und neu starten","edit officiall_filas_list.cfg in Klipper, save and restart","ajuste officiall_filas_list.cfg en Klipper, guarde y reinicie","ajuste officiall_filas_list.cfg no Klipper, salve e reinicie","modifiez officiall_filas_list.cfg dans Klipper, enregistrez et redemarrez","modificare officiall_filas_list.cfg in Klipper, salvare e riavviare"};

static const char* const TXT_KEYBOARD[LANG_COUNT]          = {"Tastatur","Keyboard","Teclado","Teclado","Clavier","Tastiera"};
static const char* const TXT_CANCEL[LANG_COUNT]            = {"Abbrechen","Cancel","Cancelar","Cancelar","Annuler","Annulla"};
static const char* const TXT_SPACE[LANG_COUNT]             = {"Leerz.","Space","Espacio","Espaco","Espace","Spazio"};
static const char* const TXT_CLEAR[LANG_COUNT]             = {"Loesch.","Clear","Borrar","Limpar","Effacer","Pulisci"};
static const char* const TXT_BKSP[LANG_COUNT]              = {"Bksp","Bksp","Bksp","Bksp","Bksp","Bksp"};
static const char* const TXT_NAME_REQUIRED[LANG_COUNT]     = {"Name eingeben","Enter name","Introducir nombre","Inserir nome","Entrer nom","Inserisci nome"};
static const char* const TXT_ALL_FIELDS_REQUIRED1[LANG_COUNT] = {"Alle Felder muessen","All fields must be","Todos los campos deben","Todos os campos devem","Tous les champs doivent","Tutti i campi devono"};
static const char* const TXT_ALL_FIELDS_REQUIRED2[LANG_COUNT] = {"bearbeitet werden","edited","completarse","ser preenchidos","etre renseignes","essere compilati"};
static const char* const TXT_FIXED_ITEMS[LANG_COUNT]       = {"QIDI/Generic gesperrt","QIDI/Generic locked","QIDI/Generic bloqueado","QIDI/Generic bloqueado","QIDI/Generic verrouille","QIDI/Generic bloccato"};
static const char* const TXT_WIFI[LANG_COUNT]              = {"WLAN","Wi-Fi","Wi-Fi","Wi-Fi","Wi-Fi","Wi-Fi"};
static const char* const TXT_WIFI_SSID[LANG_COUNT]         = {"SSID","SSID","SSID","SSID","SSID","SSID"};
static const char* const TXT_WIFI_PASSWORD[LANG_COUNT]     = {"Kennwort","Password","Password","Password","Password","Password"};
static const char* const TXT_WIFI_IP[LANG_COUNT]           = {"IP-Adresse","IP address","IP address","IP address","IP address","IP address"};
static const char* const TXT_SD_READY[LANG_COUNT]          = {"bereit","ready","ready","ready","ready","ready"};
static const char* const TXT_SD_MISSING[LANG_COUNT]        = {"nicht bereit","not ready","not ready","not ready","not ready","not ready"};
static const char* const TXT_OFFICIAL_CFG[LANG_COUNT]      = {"officiall_filas_list.cfg","officiall_filas_list.cfg","officiall_filas_list.cfg","officiall_filas_list.cfg","officiall_filas_list.cfg","officiall_filas_list.cfg"};
static const char* const TXT_OFFICIAL_USE[LANG_COUNT]      = {"Datei verwenden","Use file","Use file","Use file","Use file","Use file"};
static const char* const TXT_OFFICIAL_SOURCE[LANG_COUNT]   = {"QIDI-Dateien","QIDI files","QIDI files","QIDI files","QIDI files","QIDI files"};
static const char* const TXT_OFFICIAL_NOFILE[LANG_COUNT]   = {"keine Datei","no file","no file","no file","no file","no file"};
static const char* const TXT_OFFICIAL_ACTIVE[LANG_COUNT]   = {"aktiv","active","active","active","active","active"};
static const char* const TXT_OFFICIAL_LOCKED[LANG_COUNT]   = {"Durch QIDI-Datei gesperrt","Locked by QIDI file","Locked by QIDI file","Locked by QIDI file","Locked by QIDI file","Locked by QIDI file"};
static const char* const TXT_WIFI_REQUIRED1[LANG_COUNT]    = {"WLAN aktivieren und","Enable Wi-Fi and","Enable Wi-Fi and","Enable Wi-Fi and","Enable Wi-Fi and","Enable Wi-Fi and"};
static const char* const TXT_WIFI_REQUIRED2[LANG_COUNT]    = {"mit Netzwerk verbinden","connect to a network","connect to a network","connect to a network","connect to a network","connect to a network"};

static inline const char* LTXT(const char* const arr[LANG_COUNT]) {
  return arr[(uint8_t)uiLang];
}

// ==================== Colors ====================
struct ColorItem {
  uint8_t id;
  uint16_t rgb565;
  UiStrId labelId;
};

static const ColorItem COLORS[] = {
  {  1, 0xFFFF,          STR_COLOR_WHITE },
  {  2, 0x0000,          STR_COLOR_BLACK },
  {  3, 0xC638,          STR_COLOR_GRAY },
  {  4, 0xA7EA,          STR_COLOR_LIGHT_GREEN },
  {  5, 0x96F2,          STR_COLOR_MINT },
  {  6, 0x2B9F,          STR_COLOR_BLUE },
  {  7, 0xF3F2,          STR_COLOR_PINK },
  {  8, 0xFEA0,          STR_COLOR_YELLOW },
  {  9, 0x4DE9,          STR_COLOR_GREEN },
  { 10, 0x7DDF,          STR_COLOR_LIGHT_BLUE },
  { 11, 0x10B3,          STR_COLOR_DARK_BLUE },
  { 12, 0xC51F,          STR_COLOR_LAVENDER },
  { 13, 0xB7E0,          STR_COLOR_LIME },
  { 14, 0x43DC,          STR_COLOR_ROYAL_BLUE },
  { 15, 0x867F,          STR_COLOR_SKY_BLUE },
  { 16, 0xA2DF,          STR_COLOR_VIOLET },
  { 17, 0xFBB6,          STR_COLOR_ROSE },
  { 18, 0xE1A6,          STR_COLOR_RED },
  { 19, 0xF7BB,          STR_COLOR_BEIGE },
  { 20, 0xC618,          STR_COLOR_SILVER },
  { 21, 0x79E0,          STR_COLOR_BROWN },
  { 22, 0xB5A0,          STR_COLOR_KHAKI },
  { 23, 0xFBE0,          STR_COLOR_ORANGE },
  { 24, 0xA145,          STR_COLOR_BRONZE }
};
static const uint16_t COLORS_COUNT = sizeof(COLORS) / sizeof(COLORS[0]);

static SemaphoreHandle_t gNfcMutex = nullptr;

// ==================== UI state ====================
enum UIState {
  UI_MAIN,
  UI_READ,
  UI_WRITE,
  UI_PICK_MAT,
  UI_PICK_COLOR,
  UI_PICK_SUBTYPE,
  UI_PICK_DIAMETER,
  UI_PICK_MFG,
  UI_COLOR_PICKER,
  UI_SETUP,
  UI_LANG_SELECT,
  UI_OS_TAGINFO_CONFIG,

  UI_MAT_MENU,
  UI_MAT_EDIT_LIST,
  UI_MAT_EDIT_DETAIL,
  UI_MAT_ADD_LIST,
  UI_MAT_ADD_DETAIL,
  UI_MAT_RESET_CONFIRM,
  UI_MAT_DELETE_CONFIRM,
  UI_VAR_EDIT_LIST,
  UI_VAR_EDIT_DETAIL,
  UI_VAR_ADD_LIST,
  UI_VAR_ADD_DETAIL,
  UI_VAR_RESET_CONFIRM,
  UI_VAR_DELETE_CONFIRM,

  UI_MFG_MENU,
  UI_MFG_EDIT_LIST,
  UI_MFG_EDIT_DETAIL,
  UI_MFG_ADD_LIST,
  UI_MFG_ADD_DETAIL,
  UI_MFG_RESET_CONFIRM,
  UI_MFG_DELETE_CONFIRM,

  UI_FACTORY_RESET_CONFIRM,
  UI_SD_FORMAT_CONFIRM,

  UI_MESSAGE_OK,
  UI_KEYBOARD
};

enum KeyboardMode : uint8_t { KB_UPPER=0, KB_LOWER=1, KB_NUM=2, KB_HEX=3, KB_SYM1=4, KB_SYM2=5 };
enum NoticeKind : uint8_t { NOTICE_MATERIAL=0, NOTICE_MANUFACTURER=1 };

static UIState ui = UI_MAIN;
static UIState uiBeforeKeyboard = UI_MAIN;

static bool wifiEnabled = false;
static char wifiSsid[33] = "";
static char wifiPassword[65] = "";
static bool useOfficialListPlus4 = false;
static bool useOfficialListQ2 = false;
static bool useOfficialListMax4 = false;
static bool sdReady = false;
static bool sdAvailable = false;
static bool wifiServerStarted = false;
static bool wifiConnectPending = false;
static bool officialListPlus4Available = false;
static bool officialListQ2Available = false;
static bool officialListMax4Available = false;
enum VspiOwner : uint8_t { VSPI_OWNER_UNKNOWN = 0, VSPI_OWNER_SD = 1, VSPI_OWNER_TOUCH = 2 };
static VspiOwner activeVspiOwner = VSPI_OWNER_UNKNOWN;
static bool touchSpiReady = false;
static uint32_t lastWifiRetryMs = 0;
static uint32_t wifiConnectStartedMs = 0;
static bool wifiUploadActive = false;
static QidiPrinterModel wifiUploadModel = QIDI_MODEL_PLUS4;
static size_t wifiUploadOffset = 0;
static File wifiUploadFile;

static uint8_t selMatVal = 1;
static int matPage = 0;
static int pickMfgPage = 0;
static int pickSubtypePage = 0;
static const int ITEMS_PER_PAGE = 8;

// Color picker state
static uint16_t cpHue = 0;          // 0-359
static uint8_t  cpSat = 255;        // 0-255
static uint8_t  cpVal = 255;        // 0-255
static char     cpHex[8] = "#FFFFFF";
static char     cpHexEdit[7] = "FFFFFF";
static bool     cpHexEditActive = false;
static bool     cpHexExactLocked = false;
static UIState  cpReturnState = UI_WRITE;

static int selColIdx = 0;
static uint8_t selMfg = MFG_QIDI;

static bool displayInversionEnabled = false;
static bool autoDetectEnabled = true;
static uint32_t lastAutoCheck = 0;
static const uint32_t AUTO_CHECK_INTERVAL = 150;

static bool readResultPending = false;
static bool readPopupVisible = false;
static bool readOpenSpoolDetailsVisible = false;
static String openSpoolReadBrand = "-";
static String openSpoolReadMaterial = "-";
static uint8_t openSpoolReadPage = 0;
static uint8_t osTagInfoConfigPage = 0;
static uint8_t osReadIntervalSec = 2;
static unsigned long openSpoolPopupHoldUntil = 0;
static uint32_t openSpoolReadPageLastSwitchMs = 0;
static uint8_t readPopupMisses = 0;
static uint32_t readLastSeen = 0;

static bool autoPanelVisible = false;
static uint32_t autoLastSeen = 0;
static uint8_t autoLastMat = 0xFF;
static uint8_t autoLastCol = 0xFF;
static uint8_t autoLastMfg = 0xFF;
static uint8_t autoLastOsUid[10] = {0};
static uint8_t autoLastOsUidLen = 0;
static uint32_t openSpoolReadCancelUntil = 0;
static bool openSpoolCancelShown = false;

static bool needRedraw = true;

// material UI
static int matListPage = 0;
static int matFreePage = 0;
static uint8_t editMatVal = 1;
static char editMatName[ITEM_NAME_MAX + 1] = {0};
static uint8_t addMatVal = 1;
static char addMatName[ITEM_NAME_MAX + 1] = {0};
static char editMatMin[4] = {0};
static char editMatMax[4] = {0};
static char addMatMin[4] = {0};
static char addMatMax[4] = {0};
static char editMatBedMin[4] = {0};
static char editMatBedMax[4] = {0};
static char addMatBedMin[4] = {0};
static char addMatBedMax[4] = {0};
static uint8_t matMenuPage = 0;

// variant UI
static int varListPage = 0;
static int varFreePage = 0;
static uint8_t editVarVal = 1;
static char editVarName[ITEM_NAME_MAX + 1] = {0};
static uint8_t addVarVal = 1;
static char addVarName[ITEM_NAME_MAX + 1] = {0};

// manufacturer UI
static int mfgListPage = 0;
static int mfgFreePage = 0;
static uint8_t editMfgVal = 2;
static char editMfgName[ITEM_NAME_MAX + 1] = {0};
static uint8_t addMfgVal = 2;
static char addMfgName[ITEM_NAME_MAX + 1] = {0};

// OpenSpool write fields
static uint8_t openSpoolWritePage = 0;
static bool openSpoolProfileU1 = false;
static bool osInfoStdNozzleEnabled = true;
static bool osInfoU1BedEnabled = true;
static bool osInfoU1AlphaEnabled = true;
static bool osInfoU1WeightEnabled = false;
static bool osInfoU1DiameterEnabled = false;
static bool osInfoU1AddColorsEnabled = false;
static bool osTagInfoConfigU1 = false;
static char osSubtype[ITEM_NAME_MAX + 1] = "Basic";
static char osAlpha[3] = "FF";
static char osMinTemp[8] = "";
static char osMaxTemp[8] = "";
static char osBedMinTemp[8] = "";
static char osBedMaxTemp[8] = "";
static char osWeight[8] = "";
static char osDiameter[8] = "";
static char osColorHex[8] = "#FFFFFF";
static char osAddColor1[8] = "";
static char osAddColor2[8] = "";
static char osAddColor3[8] = "";
static char osAddColor4[8] = "";

struct OpenSpoolDraft {
  uint8_t selMfg;
  uint8_t selMatVal;
  uint8_t selColIdx;
  char osSubtype[ITEM_NAME_MAX + 1];
  char osAlpha[3];
  char osMinTemp[8];
  char osMaxTemp[8];
  char osBedMinTemp[8];
  char osBedMaxTemp[8];
  char osWeight[8];
  char osDiameter[8];
  char osColorHex[8];
  char osAddColor1[8];
  char osAddColor2[8];
  char osAddColor3[8];
  char osAddColor4[8];
};

struct UiSettingsBlob {
  uint8_t version;
  uint8_t lang;
  uint8_t defaultMode;
  uint8_t qidiPrinter;
  uint8_t screensaverMode;
  uint8_t brightness;
  uint8_t flags;
  uint8_t readInterval;
};

static const uint8_t UI_SETTINGS_BLOB_VERSION = 1;
static const uint8_t UI_FLAG_DISPLAY_INV    = 0x01;
static const uint8_t UI_FLAG_AUTO_READ      = 0x02;
static const uint8_t UI_FLAG_OS_STD_NOZZLE  = 0x04;
static const uint8_t UI_FLAG_OS_U1_BED      = 0x08;
static const uint8_t UI_FLAG_OS_U1_ALPHA    = 0x10;
static const uint8_t UI_FLAG_OS_U1_WEIGHT   = 0x20;
static const uint8_t UI_FLAG_OS_U1_DIAM     = 0x40;
static const uint8_t UI_FLAG_OS_U1_ADDC     = 0x80;

static OpenSpoolDraft osDraftStandard = {};
static OpenSpoolDraft osDraftU1 = {};
static bool osDraftsInitialized = false;

// message box
static String messageTitle = "";
static String messageLine1 = "";
static String messageLine2 = "";
static String messageLine3 = "";
static String messageLine4 = "";
static UIState messageOkNextState = UI_MAIN;

// keyboard
static char* kbTargetBuffer = nullptr;
static uint8_t kbTargetMaxLen = ITEM_NAME_MAX;
static KeyboardMode kbMode = KB_UPPER;
static bool kbForceNumeric = false;
static bool kbAllowDot = false;
static bool kbStrictNumeric = false;
static bool kbHexOnly = false;
static bool kbAllowExtendedAscii = false;
static char* osColorEditTarget = nullptr;

static const char* const OS_SUBTYPES[] = {"", "Basic", "Matte", "SnapSpeed", "Silk", "Support", "HF", "95A", "95A HF"};
static const uint8_t OS_SUBTYPE_COUNT = sizeof(OS_SUBTYPES)/sizeof(OS_SUBTYPES[0]);

static const char* const DEFAULT_OS_VARIANTS[] = {"Basic", "Matte", "SnapSpeed", "Silk", "Support", "HF", "95A", "95A HF"};
static const uint8_t DEFAULT_OS_VARIANT_COUNT = sizeof(DEFAULT_OS_VARIANTS)/sizeof(DEFAULT_OS_VARIANTS[0]);

struct MaterialPreset { const char* name; int minT; int maxT; int bedMin; int bedMax; };
static const MaterialPreset OS_MATERIAL_PRESETS[] = {
  {"PLA",190,220,50,60},{"PETG",220,250,70,80},{"ABS",230,260,90,110},{"ASA",240,270,90,110},
  {"TPU",210,230,30,60},{"PA",240,270,70,90},{"PA12",240,270,70,90},{"PC",270,310,100,120},
  {"PEEK",360,400,120,150},{"PVA",190,220,50,60},{"HIPS",230,250,90,110},{"PCTG",220,250,70,80},
  {"PLA-CF",190,220,50,60},{"PETG-CF",230,260,70,80},{"PA-CF",250,280,70,90}
};

enum OpenSpoolWritePageKind : uint8_t {
  OS_PAGE_SELECT = 0,
  OS_PAGE_BASE,
  OS_PAGE_STD_NOZZLE,
  OS_PAGE_U1_CORE,
  OS_PAGE_U1_ALPHA,
  OS_PAGE_U1_WEIGHT,
  OS_PAGE_U1_EXTRA,
  OS_PAGE_SLICER
};


// ==================== Prototypes ====================
static void drawStatus(const char* msg, uint16_t color);
static bool waitForTagUID(uint8_t* uid, uint8_t& uidLen, uint32_t timeoutMs);
static bool authBlockWithDefaultKeyA(uint8_t* uid, uint8_t uidLen, uint8_t block);
static bool readBlock(uint8_t block, uint8_t* data);
static bool writeBlock(uint8_t block, const uint8_t* data);
static void drawTagInfoPopup(uint8_t matID, uint8_t colID, uint8_t mfgID);

static const char* tagModeLabel();
static void saveDefaultMode(TagMode mode);
static void saveActiveMode(TagMode mode);
static void saveActiveQidiPrinterModel(QidiPrinterModel model);
static void saveAllSetupPreferences();
static void saveUiSettingsBlob();
static void showSimpleMessage(const String& title, const String& l1, const String& l2 = "", const String& l3 = "", const String& l4 = "", int nextState = UI_READ);
static bool openSpoolReadTag(String& brand, String& material, String& subtype, String& colorHex);
static bool openSpoolWriteTag();
static void uiRedrawIfNeeded();

static void loadMaterials();
static void resetMaterialsToDefault();
static void saveMaterialToPrefs(uint8_t val);
static void ensureSelectedMaterialValid();
static String materialNameByVal(uint8_t val);
static void loadVariants();
static void resetVariantsToDefault();
static void saveVariantToPrefs(uint8_t val);
static String variantNameByVal(uint8_t val);
static int getActiveVariantCount();
static int getFreeVariantCount();
static uint8_t getActiveVariantByIndex(int idx);
static uint8_t getFreeVariantByIndex(int idx);
static void drawOpenSpoolMaterialDetailScreen(const char* title, bool addMode);
static bool openSpoolMaterialFieldsValid(const char* nameBuf, const char* minBuf, const char* maxBuf, const char* bedMinBuf, const char* bedMaxBuf, bool requireBed);
static const MaterialPreset* findOpenSpoolMaterialPreset(const char* name);

static void loadManufacturers();
static void resetManufacturersToDefault();
static void saveManufacturerToPrefs(uint8_t val);
static String manufacturerNameByVal(uint8_t val);
static void saveWifiSettings();
static void saveOfficialListFlags();
static void loadWirelessSettings();
static void selectSdSpi();
static void selectTouchSpi();
static bool ensureSdAccess(bool forceRemount = false);
static void initSdCard();
static void refreshOfficialListAvailability();
static bool isOfficialListAvailable(QidiPrinterModel model);
static bool isOfficialListEnabled(QidiPrinterModel model);
static bool isOfficialListActiveForCurrentQidiModel();
static bool formatSdCardStorage();
static bool removeSdPathRecursive(const String& path);
static const char* officialListPathForModel(QidiPrinterModel model);
static void removeOfficialCfgFilesForModel(QidiPrinterModel model);
static bool ensureOfficialCfgDirectories();
static bool loadQidiMaterialsFromOfficialCfg();
static bool loadQidiManufacturersFromOfficialCfg();
static bool parseOfficialCfgFile(bool loadMaterialsFromFile, bool loadManufacturersFromFile);
static void applyWifiState(bool forceReconnect = false);
static void handleWifiTasks();
static void buildWifiIpLabel(char* out, size_t outSize);
static void buildWifiUploadUrl(char* out, size_t outSize);
static void buildWifiPasswordMask(char* out, size_t outSize);
static void ensureWifiServerStarted();
static void stopWifiServer();
static void handleWifiHttpClient();
static void sendHttpResponse(WiFiClient& client, const char* status, const char* contentType, const String& body);
static void closeHttpClient(WiFiClient& client);
static const char* beginOfficialCfgUpload(QidiPrinterModel targetModel);
static const char* appendOfficialCfgChunk(QidiPrinterModel targetModel, size_t offset, WiFiClient& client, size_t contentLength);
static const char* finishOfficialCfgUpload(QidiPrinterModel targetModel);
static void reloadModeDatabases();

static String trimName18(const String& s);
static void showNotice(NoticeKind kind, UIState nextState);
static void openKeyboardForBufferExtended(char* target, uint8_t maxLen, UIState returnState);
static void openKeyboardForBufferNumeric(char* target, uint8_t maxLen, UIState returnState);
static void openKeyboardForBufferNumericDot(char* target, uint8_t maxLen, UIState returnState);
static void openKeyboardForBufferHex(char* target, uint8_t maxLen, UIState returnState);
static void applyOpenSpoolMaterialPreset();
static void saveOpenSpoolTagInfoSettings();
static bool findOpenSpoolNdefTlv(const uint8_t* buf, size_t len, size_t& ndefOffset, size_t& ndefLen);
static uint8_t getOpenSpoolDisplayPageCount();
static uint8_t getOpenSpoolWritePageCount();
static OpenSpoolWritePageKind getOpenSpoolWritePageKind(uint8_t writePage);
static void drawOpenSpoolTagInfoConfigScreen();
static uint8_t getSetupPageCount();
static void normalizeSetupPage();
static void initOpenSpoolDrafts();
static void saveCurrentOpenSpoolDraft();
static void loadOpenSpoolDraft(bool u1Mode);
static void saveQidiPrinterModel(QidiPrinterModel model);
static const char* qidiPrinterModelLabel(QidiPrinterModel model);
static const char* qidiPrinterModelLabel();
static String currentQidiModeText();
static QidiPrinterModel nextQidiPrinterModel(QidiPrinterModel model);
static QidiPrinterModel qidiModelFromRequestPath(const String& requestPath);
static void drawPickSubtypeScreen();
static void drawCheckboxToggle(int x, int y, int w, int h, bool checked);
static String qidiCfgStatusLabel(QidiPrinterModel model);
static uint16_t qidiCfgStatusColor(QidiPrinterModel model);
static void drawQidiCfgSetupRow(int y, QidiPrinterModel model);
static void showQidiCfgInfo(QidiPrinterModel model);
static void drawKeyboardScreen();
static bool keyboardHandleTouch(int x, int y);
static void drawColorPickerScreen();
static void drawCpSquare();
static void drawCpHueBar();
static void drawCpPreview();
static void syncCpHex();
static void initColorPickerFromHex(const char* hex);
static void applyCpHexEdit();

// ==================== NFC mutex ====================
struct NfcLock {
  bool locked;
  NfcLock(uint32_t timeoutMs = 2000) : locked(false) {
    if (gNfcMutex) locked = (xSemaphoreTake(gNfcMutex, pdMS_TO_TICKS(timeoutMs)) == pdTRUE);
  }
  ~NfcLock() {
    if (locked && gNfcMutex) xSemaphoreGive(gNfcMutex);
  }
};

// ==================== Helpers ====================
static void safeCopy(char* dst, const char* src, size_t dstSize) {
  if (!dst || dstSize == 0) return;
  if (!src) src = "";
  strncpy(dst, src, dstSize - 1);
  dst[dstSize - 1] = '\0';
}

static String trimName18(const String& s) {
  if (s.length() <= 18) return s;
  return s.substring(0, 18);
}

static const char* shortAddColorLabel(uint8_t idx) {
  switch (idx) {
    case 1: return LTXT(TXT_ADD_COLOR1_SHORT);
    case 2: return LTXT(TXT_ADD_COLOR2_SHORT);
    case 3: return LTXT(TXT_ADD_COLOR3_SHORT);
    default: return LTXT(TXT_ADD_COLOR4_SHORT);
  }
}

static String openSpoolAddColorButtonLabel(uint8_t idx, const char* value) {
  String s = String(shortAddColorLabel(idx)) + ": ";
  s += String(value && value[0] ? value : "-");
  return s;
}

static String openSpoolReadIntervalLabel() {
  return String(LTXT(TXT_INTERVAL)) + " " + String((int)osReadIntervalSec) + " sec";
}

static String openSpoolReadProfileName() {
  String brand = openSpoolReadBrand;
  String type = openSpoolReadMaterial;
  String subtype = String(osSubtype);
  brand.trim(); type.trim(); subtype.trim();
  String s = brand;
  if (type.length()) { if (s.length()) s += " "; s += type; }
  if (subtype.length()) { if (s.length()) s += " "; s += subtype; }
  return s;
}

static const char* tagModeLabel() {
  return (currentTagMode == TAGMODE_QIDI) ? "QIDI" : "OpenSpool";
}

static const char* qidiPrinterModelLabel(QidiPrinterModel model) {
  switch (model) {
    case QIDI_MODEL_Q2:   return LTXT(TXT_QIDI_Q2);
    case QIDI_MODEL_MAX4: return LTXT(TXT_QIDI_MAX4);
    default:              return LTXT(TXT_QIDI_PLUS4);
  }
}

static const char* qidiPrinterModelLabel() {
  return qidiPrinterModelLabel(qidiPrinterModel);
}

static String currentQidiModeText() {
  return String("QIDI ") + qidiPrinterModelLabel();
}

static QidiPrinterModel nextQidiPrinterModel(QidiPrinterModel model) {
  switch (model) {
    case QIDI_MODEL_PLUS4: return QIDI_MODEL_Q2;
    case QIDI_MODEL_Q2:    return QIDI_MODEL_MAX4;
    default:               return QIDI_MODEL_PLUS4;
  }
}

static QidiPrinterModel qidiModelFromRequestPath(const String& requestPath) {
  if (requestPath.indexOf("model=q2") >= 0) return QIDI_MODEL_Q2;
  if (requestPath.indexOf("model=max4") >= 0) return QIDI_MODEL_MAX4;
  return QIDI_MODEL_PLUS4;
}

static void selectSdSpi() {
  if (activeVspiOwner == VSPI_OWNER_SD) return;
  if (activeVspiOwner == VSPI_OWNER_TOUCH) {
    touchscreenSPI.end();
    activeVspiOwner = VSPI_OWNER_UNKNOWN;
  }
  pinMode(XPT2046_CS, OUTPUT);
  digitalWrite(XPT2046_CS, HIGH);
  pinMode(SD_CS_PIN, OUTPUT);
  digitalWrite(SD_CS_PIN, HIGH);
  sdSPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  activeVspiOwner = VSPI_OWNER_SD;
}

static void selectTouchSpi() {
  if (!touchSpiReady) return;
  if (wifiUploadActive) return;
  if (activeVspiOwner == VSPI_OWNER_TOUCH) return;
  if (activeVspiOwner == VSPI_OWNER_SD) {
    SD.end();
    sdSPI.end();
    sdReady = false;
    activeVspiOwner = VSPI_OWNER_UNKNOWN;
  }
  pinMode(SD_CS_PIN, OUTPUT);
  digitalWrite(SD_CS_PIN, HIGH);
  pinMode(XPT2046_CS, OUTPUT);
  digitalWrite(XPT2046_CS, HIGH);
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  ts.begin(touchscreenSPI);
  ts.setRotation(TFT_ROT);
  ts.isrWake = true;
  activeVspiOwner = VSPI_OWNER_TOUCH;
}

static bool ensureSdAccess(bool forceRemount) {
  if (forceRemount) {
    if (activeVspiOwner == VSPI_OWNER_TOUCH) {
      touchscreenSPI.end();
    }
    SD.end();
    sdSPI.end();
    sdReady = false;
    activeVspiOwner = VSPI_OWNER_UNKNOWN;
  } else if (sdReady) {
    selectSdSpi();
    return true;
  }
  selectSdSpi();
  sdReady = SD.begin(SD_CS_PIN, sdSPI, SD_SPI_FREQ_HZ);
  sdAvailable = sdReady;
  return sdReady;
}

static bool& officialListEnabledRef(QidiPrinterModel model) {
  switch (model) {
    case QIDI_MODEL_Q2:   return useOfficialListQ2;
    case QIDI_MODEL_MAX4: return useOfficialListMax4;
    default:              return useOfficialListPlus4;
  }
}

static bool& officialListAvailableRef(QidiPrinterModel model) {
  switch (model) {
    case QIDI_MODEL_Q2:   return officialListQ2Available;
    case QIDI_MODEL_MAX4: return officialListMax4Available;
    default:              return officialListPlus4Available;
  }
}

static const char* officialListPathForModel(QidiPrinterModel model) {
  switch (model) {
    case QIDI_MODEL_Q2:   return OFFICIAL_CFG_PATH_Q2;
    case QIDI_MODEL_MAX4: return OFFICIAL_CFG_PATH_MAX4;
    default:              return OFFICIAL_CFG_PATH_PLUS4;
  }
}

static bool ensureOfficialCfgDirectories() {
  if (!SD.exists(OFFICIAL_CFG_DIR) && !SD.mkdir(OFFICIAL_CFG_DIR)) return false;
  if (!SD.exists(OFFICIAL_CFG_DIR_Q2) && !SD.mkdir(OFFICIAL_CFG_DIR_Q2)) return false;
  if (!SD.exists(OFFICIAL_CFG_DIR_PLUS4) && !SD.mkdir(OFFICIAL_CFG_DIR_PLUS4)) return false;
  if (!SD.exists(OFFICIAL_CFG_DIR_MAX4) && !SD.mkdir(OFFICIAL_CFG_DIR_MAX4)) return false;
  return true;
}

static void removeOfficialCfgFilesForModel(QidiPrinterModel model) {
  if (!ensureSdAccess(false)) return;
  const char* preferredPath = officialListPathForModel(model);
  if (SD.exists(preferredPath)) SD.remove(preferredPath);
}

static bool isOfficialListAvailable(QidiPrinterModel model) {
  return officialListAvailableRef(model);
}

static bool isOfficialListEnabled(QidiPrinterModel model) {
  return officialListEnabledRef(model);
}

static bool isOfficialListActiveForCurrentQidiModel() {
  if (currentTagMode != TAGMODE_QIDI) return false;
  return isOfficialListEnabled(qidiPrinterModel) && isOfficialListAvailable(qidiPrinterModel);
}

static void buildWifiPasswordMask(char* out, size_t outSize) {
  if (!outSize) return;
  size_t len = strlen(wifiPassword);
  if (!len) {
    safeCopy(out, "-", outSize);
    return;
  }
  size_t fillLen = min(len, outSize - 1);
  memset(out, '*', fillLen);
  out[fillLen] = '\0';
}

static void buildWifiIpLabel(char* out, size_t outSize) {
  if (!outSize) return;
  if (!wifiEnabled) {
    safeCopy(out, LTXT(TXT_OFF), outSize);
    return;
  }
  if (WiFi.status() == WL_CONNECTED) {
    IPAddress ip = WiFi.localIP();
    snprintf(out, outSize, "%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
    return;
  }
  safeCopy(out, strlen(wifiSsid) ? "..." : "-", outSize);
}

static void buildWifiUploadUrl(char* out, size_t outSize) {
  if (!outSize) return;
  if (WiFi.status() != WL_CONNECTED) {
    safeCopy(out, "-", outSize);
    return;
  }
  IPAddress ip = WiFi.localIP();
  snprintf(out, outSize, "http://%u.%u.%u.%u", ip[0], ip[1], ip[2], ip[3]);
}

static uint8_t getSetupPageCount() {
  return 5;
}

static void normalizeSetupPage() {
  uint8_t setupPages = getSetupPageCount();
  if (setupPage >= setupPages) setupPage = 0;
}

static String normalizeAlphaHex(const char* s) {
  String x = String(s ? s : "");
  x.trim();
  x.replace(" ", "");
  x.toUpperCase();
  if (x.startsWith("#")) x = x.substring(1);
  String clean = "";
  for (size_t i = 0; i < x.length(); i++) {
    char c = x[i];
    if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F')) clean += c;
    if (clean.length() >= 2) break;
  }
  if (!clean.length()) return "";
  while (clean.length() < 2) clean = "0" + clean;
  return clean;
}

static bool parseAlphaByte(const char* s, uint8_t& out) {
  String x = normalizeAlphaHex(s);
  if (x.length() != 2) return false;
  char* endptr = nullptr;
  long v = strtol(x.c_str(), &endptr, 16);
  if (!endptr || *endptr != 0 || v < 0 || v > 255) return false;
  out = (uint8_t)v;
  return true;
}

static void setAlphaFromByte(uint8_t value) {
  snprintf(osAlpha, sizeof(osAlpha), "%02X", value);
}

static int alphaPercentFromByte(uint8_t value) {
  return (int)((value * 100UL + 127UL) / 255UL);
}

static uint8_t alphaByteFromPercent(int percent) {
  percent = constrain(percent, 0, 100);
  return (uint8_t)((percent * 255UL + 50UL) / 100UL);
}

static void applyBrightnessPercent(uint8_t percent) {
  if (percent > 100) percent = 100;
  currentBrightnessApplied = percent;
  uint32_t duty = (uint32_t)percent * 255UL / 100UL;
  ledcWrite(TFT_BL, duty);
}

static void initBacklight() {
  ledcAttach(TFT_BL, BL_PWM_FREQ, BL_PWM_RES);
  applyBrightnessPercent(displayBrightness);
}

static void saveBrightness(uint8_t percent) {
  prefs.begin(PREF_NS_UI, false);
  prefs.putUChar(PREF_BRIGHTNESS, percent);
  prefs.end();
}

static const char* brightnessLabel() {
  static char buf[8];
  snprintf(buf, sizeof(buf), "%u%%", (unsigned)displayBrightness);
  return buf;
}

static void cycleBrightness() {
  if (displayBrightness >= 100 || displayBrightness < 10) displayBrightness = 10;
  else displayBrightness = (uint8_t)(displayBrightness + 10);
  saveAllSetupPreferences();
  if (!screensaverActive) applyBrightnessPercent(displayBrightness);
}

static void noteUserActivity() {
  lastUserActivityMs = millis();
  if (screensaverActive) {
    screensaverActive = false;
    applyBrightnessPercent(displayBrightness);
  }
}

static uint32_t screensaverTimeoutMs() {
  switch (screensaverMode) {
    case SCREENSAVER_30S: return 30000UL;
    case SCREENSAVER_1MIN: return 60000UL;
    case SCREENSAVER_5MIN: return 300000UL;
    case SCREENSAVER_10MIN: return 600000UL;
    case SCREENSAVER_OFF: default: return 0UL;
  }
}

static const char* screensaverModeLabel() {
  switch (screensaverMode) {
    case SCREENSAVER_30S: return LTXT(TXT_SAVER_30S);
    case SCREENSAVER_1MIN: return LTXT(TXT_SAVER_1MIN);
    case SCREENSAVER_5MIN: return LTXT(TXT_SAVER_5MIN);
    case SCREENSAVER_10MIN: return LTXT(TXT_SAVER_10MIN);
    case SCREENSAVER_OFF: default: return LTXT(TXT_OFF);
  }
}

static void chooseScreensaverPosition() {
  tft.setTextDatum(TL_DATUM);
  int textW = tft.textWidth("BoxRFID", 4);
  const int textH = 26;
  const int leftPad = 8;
  const int topPad = 8;
  const int bottomLimit = TFT_H - UI_STATUS_H - textH - topPad;
  int minX = leftPad;
  int maxX = max(minX, TFT_W - textW - leftPad);
  int minY = topPad;
  int maxY = max(minY, bottomLimit);
  screensaverTextX = random(minX, maxX + 1);
  screensaverTextY = random(minY, maxY + 1);
}

static void drawScreensaver() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.setTextDatum(TL_DATUM);
  tft.drawString("BoxRFID", screensaverTextX, screensaverTextY, 4);
}

static bool screensaverTagPresent() {
  uint8_t uid[10], uidLen = 0;
  NfcLock lock(15);
  if (!lock.locked) return false;
  // 500 ms timeout: long enough for reliable tag detection,
  // short enough to keep the screensaver responsive.
  return nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLen, 500);
}

static bool screensaverTouchPresent() {
  if (wifiUploadActive) return false;
  selectTouchSpi();
  ts.isrWake = true;
  if (!ts.touched()) return false;
  TS_Point p = ts.getPoint();
  (void)p;
  return true;
}

static void screensaverTick() {
  uint32_t now = millis();

  // --- Touch-Erkennung (sehr schnell, kein Blocking) ---
  if (screensaverTouchPresent()) {
    noteUserActivity();
    needRedraw = true;
    // Drain loop with timeout - prevents getting stuck because of IRQ noise
    uint32_t drainStart = millis();
    while ((ts.tirqTouched() || ts.touched()) && (millis() - drainStart < 800)) {
      delay(10);
    }
    return;
  }

  // --- Tag detection: check only once per second (screensaverTagPresent
  //     can block for up to 500 ms - calling it too often hurts touch response) ---
  static uint32_t lastTagCheckMs = 0;
  if (now - lastTagCheckMs >= 1000UL) {
    lastTagCheckMs = now;
    if (screensaverTagPresent()) {
      noteUserActivity();
      needRedraw = true;
      return;
    }
    // Re-read millis() after the NFC block for an accurate interval
    now = millis();
  }

  // --- Move the text every 10 seconds ---
  if (lastScreensaverMoveMs == 0 || (now - lastScreensaverMoveMs) >= 10000UL) {
    chooseScreensaverPosition();
    drawScreensaver();
    lastScreensaverMoveMs = millis();
  }
}
static const char* currentMatPrefsNs() {
  if (currentTagMode == TAGMODE_QIDI) {
    if (qidiPrinterModel == QIDI_MODEL_Q2) return PREF_NS_MAT_QIDI_Q2;
    if (qidiPrinterModel == QIDI_MODEL_MAX4) return PREF_NS_MAT_QIDI_M4;
    return PREF_NS_MAT_QIDI;
  }
  return PREF_NS_MAT_OS;
}

static const char* currentMfgPrefsNs() {
  if (currentTagMode == TAGMODE_QIDI) {
    if (qidiPrinterModel == QIDI_MODEL_Q2) return PREF_NS_MFG_QIDI_Q2;
    if (qidiPrinterModel == QIDI_MODEL_MAX4) return PREF_NS_MFG_QIDI_M4;
    return PREF_NS_MFG_QIDI;
  }
  return PREF_NS_MFG_OS;
}

static const char* currentVarPrefsNs() {
  return PREF_NS_VAR_OS;
}

static bool loadListBlob(const char* nsName, void* data, size_t dataSize) {
  prefs.begin(nsName, true);
  size_t blobLen = prefs.getBytesLength(PREF_LIST_BLOB);
  bool ok = false;
  if (blobLen == dataSize) {
    ok = (prefs.getBytes(PREF_LIST_BLOB, data, dataSize) == dataSize);
  }
  prefs.end();
  return ok;
}

static void saveListBlob(const char* nsName, const void* data, size_t dataSize) {
  prefs.begin(nsName, false);
  prefs.clear();
  prefs.putBytes(PREF_LIST_BLOB, data, dataSize);
  prefs.end();
}

static const DefaultMaterialItem* currentDefaultMaterials(uint16_t& count) {
  if (currentTagMode == TAGMODE_QIDI) {
    if (qidiPrinterModel == QIDI_MODEL_Q2) {
      count = DEFAULT_MATERIALS_QIDI_Q2_COUNT;
      return DEFAULT_MATERIALS_QIDI_Q2;
    }
    count = DEFAULT_MATERIALS_QIDI_COUNT;
    return DEFAULT_MATERIALS_QIDI;
  }
  count = DEFAULT_MATERIALS_OS_COUNT;
  return DEFAULT_MATERIALS_OS;
}

static const DefaultManufacturerItem* currentDefaultManufacturers(uint16_t& count) {
  if (currentTagMode == TAGMODE_QIDI) {
    count = DEFAULT_MANUFACTURERS_QIDI_COUNT;
    return DEFAULT_MANUFACTURERS_QIDI;
  }
  count = DEFAULT_MANUFACTURERS_OS_COUNT;
  return DEFAULT_MANUFACTURERS_OS;
}

static void reloadModeDatabases() {
  loadMaterials();
  loadVariants();
  loadManufacturers();
  ensureSelectedMaterialValid();
  if (currentTagMode == TAGMODE_QIDI) {
    if (!gManufacturers[MFG_QIDI].active) selMfg = MFG_QIDI;
  } else {
    if (!gManufacturers[selMfg].active) selMfg = MFG_GENERIC;
  }
}

static const MaterialPreset* findOpenSpoolMaterialPreset(const char* name) {
  if (!name || !*name) return nullptr;
  for (const auto& p : OS_MATERIAL_PRESETS) {
    if (strcasecmp(name, p.name) == 0) return &p;
  }
  return nullptr;
}

static void applyOpenSpoolMaterialPreset() {
  if (selMatVal < 1 || selMatVal > MAX_MATERIALS) {
    osMinTemp[0] = osMaxTemp[0] = osBedMinTemp[0] = osBedMaxTemp[0] = '\0';
    return;
  }
  const MaterialPreset* p = findOpenSpoolMaterialPreset(gMaterials[selMatVal].name);
  if (gMaterials[selMatVal].nozzleMin > 0) snprintf(osMinTemp, sizeof(osMinTemp), "%u", gMaterials[selMatVal].nozzleMin);
  else if (p) snprintf(osMinTemp, sizeof(osMinTemp), "%d", p->minT);
  else osMinTemp[0] = '\0';

  if (gMaterials[selMatVal].nozzleMax > 0) snprintf(osMaxTemp, sizeof(osMaxTemp), "%u", gMaterials[selMatVal].nozzleMax);
  else if (p) snprintf(osMaxTemp, sizeof(osMaxTemp), "%d", p->maxT);
  else osMaxTemp[0] = '\0';

  if (gMaterials[selMatVal].bedMin > 0) snprintf(osBedMinTemp, sizeof(osBedMinTemp), "%u", gMaterials[selMatVal].bedMin);
  else if (p) snprintf(osBedMinTemp, sizeof(osBedMinTemp), "%d", p->bedMin);
  else osBedMinTemp[0] = '\0';

  if (gMaterials[selMatVal].bedMax > 0) snprintf(osBedMaxTemp, sizeof(osBedMaxTemp), "%u", gMaterials[selMatVal].bedMax);
  else if (p) snprintf(osBedMaxTemp, sizeof(osBedMaxTemp), "%d", p->bedMax);
  else osBedMaxTemp[0] = '\0';

  if (p && gMaterials[selMatVal].bedMin == 0 && gMaterials[selMatVal].bedMax == 0 &&
      osBedMinTemp[0] == '\0' && osBedMaxTemp[0] == '\0') {
    snprintf(osBedMinTemp, sizeof(osBedMinTemp), "%d", p->bedMin);
    snprintf(osBedMaxTemp, sizeof(osBedMaxTemp), "%d", p->bedMax);
  }
}

static void resetOpenSpoolFieldsToDefault() {
  openSpoolProfileU1 = false;
  selMfg = MFG_GENERIC;
  selMatVal = 1;
  safeCopy(osColorHex, "#FFFFFF", sizeof(osColorHex));
  osSubtype[0] = '\0';
  safeCopy(osAlpha, "FF", sizeof(osAlpha));
  osMinTemp[0] = '\0';
  osMaxTemp[0] = '\0';
  osBedMinTemp[0] = '\0';
  osBedMaxTemp[0] = '\0';
  osWeight[0] = '\0';
  safeCopy(osDiameter, "1.75", sizeof(osDiameter));
  osAddColor1[0] = '\0';
  osAddColor2[0] = '\0';
  osAddColor3[0] = '\0';
  osAddColor4[0] = '\0';
  selColIdx = 0;
  applyOpenSpoolMaterialPreset();
}

static void saveCurrentOpenSpoolDraft() {
  OpenSpoolDraft& draft = openSpoolProfileU1 ? osDraftU1 : osDraftStandard;
  draft.selMfg = selMfg;
  draft.selMatVal = selMatVal;
  draft.selColIdx = selColIdx;
  safeCopy(draft.osSubtype, osSubtype, sizeof(draft.osSubtype));
  safeCopy(draft.osAlpha, osAlpha, sizeof(draft.osAlpha));
  safeCopy(draft.osMinTemp, osMinTemp, sizeof(draft.osMinTemp));
  safeCopy(draft.osMaxTemp, osMaxTemp, sizeof(draft.osMaxTemp));
  safeCopy(draft.osBedMinTemp, osBedMinTemp, sizeof(draft.osBedMinTemp));
  safeCopy(draft.osBedMaxTemp, osBedMaxTemp, sizeof(draft.osBedMaxTemp));
  safeCopy(draft.osWeight, osWeight, sizeof(draft.osWeight));
  safeCopy(draft.osDiameter, osDiameter, sizeof(draft.osDiameter));
  safeCopy(draft.osColorHex, osColorHex, sizeof(draft.osColorHex));
  safeCopy(draft.osAddColor1, osAddColor1, sizeof(draft.osAddColor1));
  safeCopy(draft.osAddColor2, osAddColor2, sizeof(draft.osAddColor2));
  safeCopy(draft.osAddColor3, osAddColor3, sizeof(draft.osAddColor3));
  safeCopy(draft.osAddColor4, osAddColor4, sizeof(draft.osAddColor4));
}

static void loadOpenSpoolDraft(bool u1Mode) {
  OpenSpoolDraft& draft = u1Mode ? osDraftU1 : osDraftStandard;
  openSpoolProfileU1 = u1Mode;
  selMfg = draft.selMfg;
  selMatVal = draft.selMatVal;
  selColIdx = draft.selColIdx;
  safeCopy(osSubtype, draft.osSubtype, sizeof(osSubtype));
  safeCopy(osAlpha, draft.osAlpha, sizeof(osAlpha));
  safeCopy(osMinTemp, draft.osMinTemp, sizeof(osMinTemp));
  safeCopy(osMaxTemp, draft.osMaxTemp, sizeof(osMaxTemp));
  safeCopy(osBedMinTemp, draft.osBedMinTemp, sizeof(osBedMinTemp));
  safeCopy(osBedMaxTemp, draft.osBedMaxTemp, sizeof(osBedMaxTemp));
  safeCopy(osWeight, draft.osWeight, sizeof(osWeight));
  safeCopy(osDiameter, draft.osDiameter, sizeof(osDiameter));
  safeCopy(osColorHex, draft.osColorHex, sizeof(osColorHex));
  if (osColorHex[0] == '\0') safeCopy(osColorHex, "#FFFFFF", sizeof(osColorHex));
  safeCopy(osAddColor1, draft.osAddColor1, sizeof(osAddColor1));
  safeCopy(osAddColor2, draft.osAddColor2, sizeof(osAddColor2));
  safeCopy(osAddColor3, draft.osAddColor3, sizeof(osAddColor3));
  safeCopy(osAddColor4, draft.osAddColor4, sizeof(osAddColor4));
  ensureSelectedMaterialValid();
  syncSelectionFromOpenSpoolColor();
  openSpoolWritePage = 1;
}

static void initOpenSpoolDrafts() {
  bool savedProfile = openSpoolProfileU1;
  resetOpenSpoolFieldsToDefault();
  saveCurrentOpenSpoolDraft();
  openSpoolProfileU1 = true;
  resetOpenSpoolFieldsToDefault();
  saveCurrentOpenSpoolDraft();
  loadOpenSpoolDraft(savedProfile);
  osDraftsInitialized = true;
}

static uint16_t colorTextForBg(uint16_t bg) {
  if (bg == TFT_BLACK || bg == TFT_BLUE || bg == TFT_RED || bg == 0x0011 || bg == 0x435C || bg == 0x801F || bg == 0x79E0 || bg == 0xA145) return TFT_WHITE;
  return TFT_BLACK;
}

static void fillButton(int x, int y, int w, int h, uint16_t fill, uint16_t border, const String& txt, uint16_t txtColor = TFT_WHITE, int font = 2) {
  tft.fillRoundRect(x, y, w, h, 6, fill);
  tft.drawRoundRect(x, y, w, h, 6, border);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(txtColor, fill);
  tft.drawString(txt, x + w / 2, y + h / 2, font);
  tft.setTextDatum(TL_DATUM);
}

static void drawHeader(const String& title) {
  tft.fillRect(0, 0, TFT_W, UI_HEADER_H, TFT_DARKCYAN);
  tft.drawFastHLine(0, UI_HEADER_H - 1, TFT_W, TFT_WHITE);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_DARKCYAN);
  tft.drawString(title, TFT_W / 2, UI_HEADER_H / 2, 2);
  tft.setTextDatum(TL_DATUM);
}

static void drawStatusBarFrame() {
  tft.drawFastHLine(0, TFT_H - UI_STATUS_H, TFT_W, TFT_DARKGREY);
}

static void drawStatus(const char* msg, uint16_t color) {
  tft.fillRect(0, TFT_H - UI_STATUS_H + 1, TFT_W, UI_STATUS_H - 1, TFT_BLACK);
  tft.setTextColor(color, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(String(msg), TFT_W / 2, TFT_H - UI_STATUS_H / 2, 2);
  tft.setTextDatum(TL_DATUM);
}

static void drawStatusAppVersion() {
  tft.fillRect(0, TFT_H - UI_STATUS_H + 1, TFT_W, UI_STATUS_H - 1, TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(APP_VERSION, TFT_W / 2, TFT_H - UI_STATUS_H / 2, 2);
  tft.setTextDatum(TL_DATUM);
}

static void drawMainMenuStatus() {
  tft.fillRect(0, TFT_H - UI_STATUS_H + 1, TFT_W, UI_STATUS_H - 1, TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.drawString("BoxRFID OpenSpool Edition by TinkerBarn", TFT_W / 2, TFT_H - UI_STATUS_H / 2, 1);
  tft.setTextDatum(TL_DATUM);
}

static int findColorById(uint8_t id) {
  for (uint16_t i = 0; i < COLORS_COUNT; i++) {
    if (COLORS[i].id == id) return (int)i;
  }
  return -1;
}

static int getActiveMaterialCount() {
  int c = 0;
  for (uint8_t i = 1; i <= MAX_MATERIALS; i++) if (gMaterials[i].active) c++;
  return c;
}

static int getFreeMaterialCount() {
  int c = 0;
  for (uint8_t i = 1; i <= MAX_MATERIALS; i++) if (!gMaterials[i].active) c++;
  return c;
}

static uint8_t getActiveMaterialByIndex(int idx) {
  int n = 0;
  for (uint8_t i = 1; i <= MAX_MATERIALS; i++) {
    if (gMaterials[i].active) {
      if (n == idx) return i;
      n++;
    }
  }
  return 0;
}

static uint8_t getFreeMaterialByIndex(int idx) {
  int n = 0;
  for (uint8_t i = 1; i <= MAX_MATERIALS; i++) {
    if (!gMaterials[i].active) {
      if (n == idx) return i;
      n++;
    }
  }
  return 0;
}

static int getEditableManufacturerCount() {
  if (currentTagMode == TAGMODE_OPENSPOOL) {
    int c = 0;
    bool seen[MAX_MANUFACTURERS + 1] = {false};
    for (uint16_t k = 0; k < DISPLAY_ORDER_MANUFACTURERS_OS_COUNT; k++) {
      uint8_t i = DISPLAY_ORDER_MANUFACTURERS_OS[k];
      if (i <= MAX_MANUFACTURERS) {
        seen[i] = true;
        if (gManufacturers[i].active) c++;
      }
    }
    for (uint8_t i = 0; i <= MAX_MANUFACTURERS; i++) {
      if (!seen[i] && gManufacturers[i].active) c++;
    }
    return c;
  }
  int c = 0;
  for (uint8_t i = 2; i <= MAX_MANUFACTURERS; i++) if (gManufacturers[i].active) c++;
  return c;
}

static int getFreeManufacturerCount() {
  if (currentTagMode == TAGMODE_OPENSPOOL) {
    int c = 0;
    bool seen[MAX_MANUFACTURERS + 1] = {false};
    for (uint16_t k = 0; k < DISPLAY_ORDER_MANUFACTURERS_OS_COUNT; k++) {
      uint8_t i = DISPLAY_ORDER_MANUFACTURERS_OS[k];
      if (i <= MAX_MANUFACTURERS) {
        seen[i] = true;
        if (!gManufacturers[i].active) c++;
      }
    }
    for (uint8_t i = 0; i <= MAX_MANUFACTURERS; i++) {
      if (!seen[i] && !gManufacturers[i].active) c++;
    }
    return c;
  }
  int c = 0;
  for (uint8_t i = 2; i <= MAX_MANUFACTURERS; i++) if (!gManufacturers[i].active) c++;
  return c;
}

static int getAllManufacturerCount() {
  int c = 0;
  for (uint8_t i = 0; i <= MAX_MANUFACTURERS; i++) if (gManufacturers[i].active) c++;
  return c;
}

static uint8_t getEditableManufacturerByIndex(int idx) {
  if (currentTagMode == TAGMODE_OPENSPOOL) {
    int n = 0;
    bool seen[MAX_MANUFACTURERS + 1] = {false};
    for (uint16_t k = 0; k < DISPLAY_ORDER_MANUFACTURERS_OS_COUNT; k++) {
      uint8_t i = DISPLAY_ORDER_MANUFACTURERS_OS[k];
      if (i <= MAX_MANUFACTURERS) {
        seen[i] = true;
        if (gManufacturers[i].active) {
          if (n == idx) return i;
          n++;
        }
      }
    }
    for (uint8_t i = 0; i <= MAX_MANUFACTURERS; i++) {
      if (!seen[i] && gManufacturers[i].active) {
        if (n == idx) return i;
        n++;
      }
    }
    return 0;
  }
  int n = 0;
  for (uint8_t i = 2; i <= MAX_MANUFACTURERS; i++) {
    if (gManufacturers[i].active) {
      if (n == idx) return i;
      n++;
    }
  }
  return 0;
}

static uint8_t getFreeManufacturerByIndex(int idx) {
  if (currentTagMode == TAGMODE_OPENSPOOL) {
    int n = 0;
    bool seen[MAX_MANUFACTURERS + 1] = {false};
    for (uint16_t k = 0; k < DISPLAY_ORDER_MANUFACTURERS_OS_COUNT; k++) {
      uint8_t i = DISPLAY_ORDER_MANUFACTURERS_OS[k];
      if (i <= MAX_MANUFACTURERS) {
        seen[i] = true;
        if (!gManufacturers[i].active) {
          if (n == idx) return i;
          n++;
        }
      }
    }
    for (uint8_t i = 0; i <= MAX_MANUFACTURERS; i++) {
      if (!seen[i] && !gManufacturers[i].active) {
        if (n == idx) return i;
        n++;
      }
    }
    return 0;
  }
  int n = 0;
  for (uint8_t i = 2; i <= MAX_MANUFACTURERS; i++) {
    if (!gManufacturers[i].active) {
      if (n == idx) return i;
      n++;
    }
  }
  return 0;
}

static uint8_t getAllManufacturerByIndex(int idx) {
  int n = 0;

  if (currentTagMode == TAGMODE_OPENSPOOL) {
    bool seen[MAX_MANUFACTURERS + 1] = {false};

    for (uint16_t k = 0; k < DISPLAY_ORDER_MANUFACTURERS_OS_COUNT; k++) {
      uint8_t i = DISPLAY_ORDER_MANUFACTURERS_OS[k];
      if (i <= MAX_MANUFACTURERS) {
        seen[i] = true;
        if (gManufacturers[i].active) {
          if (n == idx) return i;
          n++;
        }
      }
    }

    for (uint8_t i = 0; i <= MAX_MANUFACTURERS; i++) {
      if (!seen[i] && gManufacturers[i].active) {
        if (n == idx) return i;
        n++;
      }
    }
    return 0;
  }

  for (uint8_t i = 0; i <= MAX_MANUFACTURERS; i++) {
    if (gManufacturers[i].active) {
      if (n == idx) return i;
      n++;
    }
  }
  return 0;
}

// ==================== Materials ====================
static String materialNameByVal(uint8_t val) {
  if (val >= 1 && val <= MAX_MATERIALS && gMaterials[val].active) return String(gMaterials[val].name);
  return "Unknown";
}

static void ensureSelectedMaterialValid() {
  if (selMatVal >= 1 && selMatVal <= MAX_MATERIALS && gMaterials[selMatVal].active) return;
  for (uint8_t i = 1; i <= MAX_MATERIALS; i++) {
    if (gMaterials[i].active) {
      selMatVal = i;
      return;
    }
  }
  selMatVal = 1;
}

static void saveMaterialToPrefs(uint8_t val) {
  if (val < 1 || val > MAX_MATERIALS) return;
  saveListBlob(currentMatPrefsNs(), gMaterials, sizeof(gMaterials));
}

static void resetMaterialsToDefault() {
  for (uint8_t i = 1; i <= MAX_MATERIALS; i++) {
    gMaterials[i].active = false;
    gMaterials[i].name[0] = '\0';
    gMaterials[i].nozzleMin = 0;
    gMaterials[i].nozzleMax = 0;
    gMaterials[i].bedMin = 0;
    gMaterials[i].bedMax = 0;
  }
  uint16_t defCount = 0;
  const DefaultMaterialItem* defs = currentDefaultMaterials(defCount);
  for (uint16_t i = 0; i < defCount; i++) {
    uint8_t v = defs[i].val;
    if (v >= 1 && v <= MAX_MATERIALS) {
      gMaterials[v].active = true;
      safeCopy(gMaterials[v].name, defs[i].name, sizeof(gMaterials[v].name));
      if (defs[i].minT > 0) gMaterials[v].nozzleMin = defs[i].minT;
      if (defs[i].maxT > 0) gMaterials[v].nozzleMax = defs[i].maxT;
      if (gMaterials[v].nozzleMin == 0 || gMaterials[v].nozzleMax == 0) {
        const MaterialPreset* p = findOpenSpoolMaterialPreset(defs[i].name);
        if (p) {
        if (gMaterials[v].nozzleMin == 0) gMaterials[v].nozzleMin = p->minT;
          if (gMaterials[v].nozzleMax == 0) gMaterials[v].nozzleMax = p->maxT;
        }
      }
      const MaterialPreset* p = findOpenSpoolMaterialPreset(defs[i].name);
      if (p) {
        gMaterials[v].bedMin = p->bedMin;
        gMaterials[v].bedMax = p->bedMax;
      }
    }
  }
  saveListBlob(currentMatPrefsNs(), gMaterials, sizeof(gMaterials));
  ensureSelectedMaterialValid();
}

static void loadMaterials() {
  if (currentTagMode == TAGMODE_QIDI && loadQidiMaterialsFromOfficialCfg()) return;
  for (uint8_t i = 1; i <= MAX_MATERIALS; i++) {
    gMaterials[i].active = false;
    gMaterials[i].name[0] = '\0';
    gMaterials[i].nozzleMin = 0;
    gMaterials[i].nozzleMax = 0;
    gMaterials[i].bedMin = 0;
    gMaterials[i].bedMax = 0;
  }
  bool anyStored = loadListBlob(currentMatPrefsNs(), gMaterials, sizeof(gMaterials));
  for (uint8_t i = 1; i <= MAX_MATERIALS; i++) {
    if (gMaterials[i].active) {
      const MaterialPreset* p = findOpenSpoolMaterialPreset(gMaterials[i].name);
      if (p) {
        if (gMaterials[i].nozzleMin == 0) gMaterials[i].nozzleMin = p->minT;
        if (gMaterials[i].nozzleMax == 0) gMaterials[i].nozzleMax = p->maxT;
        if (gMaterials[i].bedMin == 0) gMaterials[i].bedMin = p->bedMin;
        if (gMaterials[i].bedMax == 0) gMaterials[i].bedMax = p->bedMax;
      }
    }
  }
  if (!anyStored) resetMaterialsToDefault();
  ensureSelectedMaterialValid();
}

// ==================== Manufacturers ====================
static String manufacturerNameByVal(uint8_t val) {
  if (val <= MAX_MANUFACTURERS && gManufacturers[val].active) return String(gManufacturers[val].name);
  return "Unknown";
}

static void saveManufacturerToPrefs(uint8_t val) {
  if (val > MAX_MANUFACTURERS) return;
  saveListBlob(currentMfgPrefsNs(), gManufacturers, sizeof(gManufacturers));
}

static void resetManufacturersToDefault() {
  for (uint8_t i = 0; i <= MAX_MANUFACTURERS; i++) {
    gManufacturers[i].active = false;
    gManufacturers[i].name[0] = '\0';
  }
  uint16_t defCount = 0;
  const DefaultManufacturerItem* defs = currentDefaultManufacturers(defCount);
  for (uint16_t i = 0; i < defCount; i++) {
    uint8_t v = defs[i].val;
    if (v <= MAX_MANUFACTURERS) {
      gManufacturers[v].active = true;
      safeCopy(gManufacturers[v].name, defs[i].name, sizeof(gManufacturers[v].name));
    }
  }
  saveListBlob(currentMfgPrefsNs(), gManufacturers, sizeof(gManufacturers));
  if (!gManufacturers[selMfg].active) selMfg = (currentTagMode == TAGMODE_OPENSPOOL ? MFG_GENERIC : MFG_QIDI);
}


static String variantNameByVal(uint8_t val) {
  if (val >= 1 && val <= MAX_VARIANTS && gVariants[val].active) return String(gVariants[val].name);
  return String("");
}

static void saveVariantToPrefs(uint8_t val) {
  if (val < 1 || val > MAX_VARIANTS) return;
  saveListBlob(currentVarPrefsNs(), gVariants, sizeof(gVariants));
}

static void resetVariantsToDefault() {
  for (uint8_t i = 1; i <= MAX_VARIANTS; i++) {
    gVariants[i].active = false;
    gVariants[i].name[0] = '\0';
    gVariants[i].nozzleMin = 0;
    gVariants[i].nozzleMax = 0;
  }
  for (uint8_t i = 0; i < DEFAULT_OS_VARIANT_COUNT && i < MAX_VARIANTS; i++) {
    uint8_t v = i + 1;
    gVariants[v].active = true;
    safeCopy(gVariants[v].name, DEFAULT_OS_VARIANTS[i], sizeof(gVariants[v].name));
  }
  saveListBlob(currentVarPrefsNs(), gVariants, sizeof(gVariants));

  bool subtypeFound = false;
  for (uint8_t i = 1; i <= MAX_VARIANTS; i++) {
    if (gVariants[i].active && strcmp(gVariants[i].name, osSubtype) == 0) { subtypeFound = true; break; }
  }
  if (!subtypeFound && DEFAULT_OS_VARIANT_COUNT > 0) safeCopy(osSubtype, DEFAULT_OS_VARIANTS[0], sizeof(osSubtype));
}

static void loadVariants() {
  for (uint8_t i = 1; i <= MAX_VARIANTS; i++) {
    gVariants[i].active = false;
    gVariants[i].name[0] = '\0';
    gVariants[i].nozzleMin = 0;
    gVariants[i].nozzleMax = 0;
  }
  bool anyStored = loadListBlob(currentVarPrefsNs(), gVariants, sizeof(gVariants));
  if (!anyStored) resetVariantsToDefault();

  bool subtypeFound = false;
  for (uint8_t i = 1; i <= MAX_VARIANTS; i++) {
    if (gVariants[i].active && strcmp(gVariants[i].name, osSubtype) == 0) { subtypeFound = true; break; }
  }
  if (!subtypeFound) {
    for (uint8_t i = 1; i <= MAX_VARIANTS; i++) {
      if (gVariants[i].active) { safeCopy(osSubtype, gVariants[i].name, sizeof(osSubtype)); subtypeFound = true; break; }
    }
  }
  if (!subtypeFound) osSubtype[0] = '\0';
}

static int getActiveVariantCount() {
  int c = 0;
  for (uint8_t i = 1; i <= MAX_VARIANTS; i++) if (gVariants[i].active) c++;
  return c;
}

static int getFreeVariantCount() {
  int c = 0;
  for (uint8_t i = 1; i <= MAX_VARIANTS; i++) if (!gVariants[i].active) c++;
  return c;
}

static uint8_t getActiveVariantByIndex(int idx) {
  int n = 0;
  for (uint8_t i = 1; i <= MAX_VARIANTS; i++) {
    if (gVariants[i].active) {
      if (n == idx) return i;
      n++;
    }
  }
  return 1;
}

static uint8_t getFreeVariantByIndex(int idx) {
  int n = 0;
  for (uint8_t i = 1; i <= MAX_VARIANTS; i++) {
    if (!gVariants[i].active) {
      if (n == idx) return i;
      n++;
    }
  }
  return 1;
}

static void loadManufacturers() {
  if (currentTagMode == TAGMODE_QIDI && loadQidiManufacturersFromOfficialCfg()) return;
  for (uint8_t i = 0; i <= MAX_MANUFACTURERS; i++) {
    gManufacturers[i].active = false;
    gManufacturers[i].name[0] = '\0';
  }
  bool anyStored = loadListBlob(currentMfgPrefsNs(), gManufacturers, sizeof(gManufacturers));
  if (!anyStored) resetManufacturersToDefault();

  bool repaired = false;
  if (!gManufacturers[MFG_GENERIC].active || strlen(gManufacturers[MFG_GENERIC].name) == 0) {
    gManufacturers[MFG_GENERIC].active = true;
    safeCopy(gManufacturers[MFG_GENERIC].name, "Generic", sizeof(gManufacturers[MFG_GENERIC].name));
    repaired = true;
  }
  if (currentTagMode == TAGMODE_QIDI) {
    if (!gManufacturers[MFG_QIDI].active || strlen(gManufacturers[MFG_QIDI].name) == 0) {
      gManufacturers[MFG_QIDI].active = true;
      safeCopy(gManufacturers[MFG_QIDI].name, "QIDI", sizeof(gManufacturers[MFG_QIDI].name));
      repaired = true;
    }
  } else {
    if (!gManufacturers[1].active || strlen(gManufacturers[1].name) == 0 || strcmp(gManufacturers[1].name, "QIDI") == 0) {
      gManufacturers[1].active = true;
      safeCopy(gManufacturers[1].name, "Snapmaker", sizeof(gManufacturers[1].name));
      repaired = true;
    }
  }
  if (repaired) saveListBlob(currentMfgPrefsNs(), gManufacturers, sizeof(gManufacturers));
  if (!gManufacturers[selMfg].active) selMfg = (currentTagMode == TAGMODE_OPENSPOOL ? MFG_GENERIC : MFG_QIDI);
}

// ==================== Preferences helpers ====================
static void saveCalibration(int minx, int maxx, int miny, int maxy) {
  prefs.begin(PREF_NS_TOUCH, false);
  prefs.putInt(PREF_MINX, minx);
  prefs.putInt(PREF_MAXX, maxx);
  prefs.putInt(PREF_MINY, miny);
  prefs.putInt(PREF_MAXY, maxy);
  prefs.putBool(PREF_HAS_CAL, true);
  prefs.end();
}

static void clearCalibrationPrefs() {
  prefs.begin(PREF_NS_TOUCH, false);
  prefs.clear();
  prefs.end();
  TS_MINX = DEF_TS_MINX;
  TS_MAXX = DEF_TS_MAXX;
  TS_MINY = DEF_TS_MINY;
  TS_MAXY = DEF_TS_MAXY;
}

static void loadCalibration() {
  prefs.begin(PREF_NS_TOUCH, true);
  bool has = prefs.getBool(PREF_HAS_CAL, false);
  if (has) {
    int minx = prefs.getInt(PREF_MINX, TS_MINX);
    int maxx = prefs.getInt(PREF_MAXX, TS_MAXX);
    int miny = prefs.getInt(PREF_MINY, TS_MINY);
    int maxy = prefs.getInt(PREF_MAXY, TS_MAXY);
    bool ok = (minx >= 0 && maxx <= 4095 && miny >= 0 && maxy <= 4095 && abs(maxx - minx) > 500 && abs(maxy - miny) > 500);
    if (ok) {
      TS_MINX = minx; TS_MAXX = maxx; TS_MINY = miny; TS_MAXY = maxy;
    } else {
      TS_MINX = DEF_TS_MINX; TS_MAXX = DEF_TS_MAXX; TS_MINY = DEF_TS_MINY; TS_MAXY = DEF_TS_MAXY;
    }
  } else {
    TS_MINX = DEF_TS_MINX; TS_MAXX = DEF_TS_MAXX; TS_MINY = DEF_TS_MINY; TS_MAXY = DEF_TS_MAXY;
  }
  prefs.end();
}

static void applyDisplayInversion() {
  tft.invertDisplay(displayInversionEnabled);
}

static void saveLanguage(UiLang lang) {
  (void)lang;
  saveUiSettingsBlob();
}

static void saveDisplayInversion(bool enabled) {
  (void)enabled;
  saveUiSettingsBlob();
}

static void saveAutoRead(bool enabled) {
  (void)enabled;
  saveUiSettingsBlob();
}

static void saveDefaultMode(TagMode mode) {
  (void)mode;
  saveUiSettingsBlob();
}

static void saveActiveMode(TagMode mode) {
  (void)mode;
  saveUiSettingsBlob();
}

static void saveQidiPrinterModel(QidiPrinterModel model) {
  (void)model;
  saveUiSettingsBlob();
}

static void saveActiveQidiPrinterModel(QidiPrinterModel model) {
  (void)model;
  saveUiSettingsBlob();
}

static void saveScreensaverMode(ScreensaverMode mode) {
  (void)mode;
  saveUiSettingsBlob();
}

static void saveOpenSpoolTagInfoSettings() {
  saveUiSettingsBlob();
}

static void saveWifiSettings() {
  prefs.begin(PREF_NS_WIFI, false);
  prefs.putBool(PREF_WIFI_ENABLED, wifiEnabled);
  prefs.putString(PREF_WIFI_SSID, wifiSsid);
  prefs.putString(PREF_WIFI_PASS, wifiPassword);
  prefs.end();
}

static void saveOfficialListFlags() {
  prefs.begin(PREF_NS_QIDI_CFG, false);
  prefs.putBool(PREF_QIDI_USE_CFG_P4, useOfficialListPlus4);
  prefs.putBool(PREF_QIDI_USE_CFG_Q2, useOfficialListQ2);
  prefs.putBool(PREF_QIDI_USE_CFG_M4, useOfficialListMax4);
  prefs.end();
}

static void loadWirelessSettings() {
  prefs.begin(PREF_NS_WIFI, true);
  wifiEnabled = prefs.getBool(PREF_WIFI_ENABLED, false);
  String ssid = prefs.getString(PREF_WIFI_SSID, "");
  String pass = prefs.getString(PREF_WIFI_PASS, "");
  prefs.end();
  safeCopy(wifiSsid, ssid.c_str(), sizeof(wifiSsid));
  safeCopy(wifiPassword, pass.c_str(), sizeof(wifiPassword));

  prefs.begin(PREF_NS_QIDI_CFG, true);
  useOfficialListPlus4 = prefs.getBool(PREF_QIDI_USE_CFG_P4, false);
  useOfficialListQ2 = prefs.getBool(PREF_QIDI_USE_CFG_Q2, false);
  useOfficialListMax4 = prefs.getBool(PREF_QIDI_USE_CFG_M4, false);
  prefs.end();
}

static void saveUiSettingsBlob() {
  UiSettingsBlob blob = {};
  blob.version = UI_SETTINGS_BLOB_VERSION;
  blob.lang = (uint8_t)uiLang;
  blob.defaultMode = (uint8_t)defaultTagMode;
  blob.qidiPrinter = (uint8_t)qidiPrinterModel;
  blob.screensaverMode = (uint8_t)screensaverMode;
  blob.brightness = displayBrightness;
  blob.flags =
      (displayInversionEnabled ? UI_FLAG_DISPLAY_INV : 0) |
      (autoDetectEnabled ? UI_FLAG_AUTO_READ : 0) |
      (osInfoStdNozzleEnabled ? UI_FLAG_OS_STD_NOZZLE : 0) |
      (osInfoU1BedEnabled ? UI_FLAG_OS_U1_BED : 0) |
      (osInfoU1AlphaEnabled ? UI_FLAG_OS_U1_ALPHA : 0) |
      (osInfoU1WeightEnabled ? UI_FLAG_OS_U1_WEIGHT : 0) |
      (osInfoU1DiameterEnabled ? UI_FLAG_OS_U1_DIAM : 0) |
      (osInfoU1AddColorsEnabled ? UI_FLAG_OS_U1_ADDC : 0);
  blob.readInterval = osReadIntervalSec;
  prefs.begin(PREF_NS_UI, false);
  prefs.clear();
  prefs.putBytes(PREF_UI_BLOB, &blob, sizeof(blob));
  prefs.end();
}

static void saveAllSetupPreferences() {
  saveUiSettingsBlob();
  saveWifiSettings();
  saveOfficialListFlags();
}

static String cfgTrimmed(String s) {
  s.trim();
  if (s.endsWith("\r")) s.remove(s.length() - 1);
  s.trim();
  if (s.startsWith("\"") && s.endsWith("\"") && s.length() >= 2) {
    s = s.substring(1, s.length() - 1);
    s.trim();
  }
  return s;
}

static int extractTrailingNumber(const String& text) {
  int start = -1;
  for (int i = 0; i < (int)text.length(); i++) {
    if (isDigit((unsigned char)text[i])) {
      if (start < 0) start = i;
    } else if (start >= 0) {
      return text.substring(start, i).toInt();
    }
  }
  if (start >= 0) return text.substring(start).toInt();
  return -1;
}

static uint16_t parseCfgU16(const String& text) {
  long value = text.toInt();
  if (value < 0) value = 0;
  if (value > 65535L) value = 65535L;
  return (uint16_t)value;
}

static void refreshOfficialListAvailability() {
  if (!ensureSdAccess(true)) {
    sdAvailable = false;
    officialListPlus4Available = false;
    officialListQ2Available = false;
    officialListMax4Available = false;
    selectTouchSpi();
    return;
  }
  sdAvailable = true;
  officialListPlus4Available = SD.exists(OFFICIAL_CFG_PATH_PLUS4);
  officialListQ2Available = SD.exists(OFFICIAL_CFG_PATH_Q2);
  officialListMax4Available = SD.exists(OFFICIAL_CFG_PATH_MAX4);
  selectTouchSpi();
}

static bool removeSdPathRecursive(const String& path) {
  if (!path.length() || path == "/") return true;
  String normalizedPath = path;
  if (!normalizedPath.startsWith("/")) normalizedPath = "/" + normalizedPath;
  if (!SD.exists(normalizedPath.c_str())) return true;

  File entry = SD.open(normalizedPath.c_str());
  if (!entry) return false;
  bool isDir = entry.isDirectory();
  entry.close();

  if (!isDir) {
    yield();
    return SD.remove(normalizedPath.c_str()) || !SD.exists(normalizedPath.c_str());
  }

  while (true) {
    yield();
    File dir = SD.open(normalizedPath.c_str());
    if (!dir || !dir.isDirectory()) {
      if (dir) dir.close();
      return false;
    }
    File child = dir.openNextFile();
    if (!child) {
      dir.close();
      break;
    }
    String childPath = String(child.name());
    if (!childPath.startsWith("/")) {
      childPath = (normalizedPath == "/") ? (String("/") + childPath) : (normalizedPath + "/" + childPath);
    }
    child.close();
    dir.close();
    if (!removeSdPathRecursive(childPath)) return false;
  }

  return SD.rmdir(normalizedPath.c_str()) || !SD.exists(normalizedPath.c_str());
}

static bool formatSdCardStorage() {
  if (!ensureSdAccess(true)) {
    sdReady = false;
    sdAvailable = false;
    selectTouchSpi();
    return false;
  }

  while (true) {
    yield();
    File root = SD.open("/");
    if (!root || !root.isDirectory()) {
      if (root) root.close();
      selectTouchSpi();
      return false;
    }
    File child = root.openNextFile();
    if (!child) {
      root.close();
      break;
    }
    String childPath = String(child.name());
    if (!childPath.startsWith("/")) childPath = "/" + childPath;
    child.close();
    root.close();
    if (!removeSdPathRecursive(childPath)) {
      selectTouchSpi();
      return false;
    }
  }

  bool dirsOk = ensureOfficialCfgDirectories();
  useOfficialListPlus4 = false;
  useOfficialListQ2 = false;
  useOfficialListMax4 = false;
  saveOfficialListFlags();
  refreshOfficialListAvailability();
  sdReady = sdAvailable;
  if (currentTagMode == TAGMODE_QIDI) reloadModeDatabases();
  selectTouchSpi();
  return dirsOk;
}

static void initSdCard() {
  pinMode(SD_CS_PIN, OUTPUT);
  digitalWrite(SD_CS_PIN, HIGH);
  sdReady = ensureSdAccess(true);
  sdAvailable = sdReady;
  if (sdReady) {
    ensureOfficialCfgDirectories();
  }
  refreshOfficialListAvailability();
}

static void sendHttpResponse(WiFiClient& client, const char* status, const char* contentType, const String& body) {
  client.print(F("HTTP/1.1 "));
  client.print(status);
  client.print(F("\r\nContent-Type: "));
  client.print(contentType);
  client.print(F("\r\nConnection: close\r\nContent-Length: "));
  client.print(body.length());
  client.print(F("\r\n\r\n"));
  client.print(body);
}

static void closeHttpClient(WiFiClient& client) {
  delay(20);
  client.stop();
}

static const char* beginOfficialCfgUpload(QidiPrinterModel targetModel) {
  if (!ensureOfficialCfgDirectories()) {
    selectTouchSpi();
    return "SD dir create failed";
  }
  const char* path = officialListPathForModel(targetModel);
  if (wifiUploadActive) {
    if (wifiUploadFile) wifiUploadFile.close();
    wifiUploadActive = false;
    removeOfficialCfgFilesForModel(wifiUploadModel);
  }

  removeOfficialCfgFilesForModel(targetModel);

  wifiUploadFile = SD.open(path, FILE_WRITE);
  if (!wifiUploadFile) {
    static char errorBuf[96];
    snprintf(errorBuf, sizeof(errorBuf), "SD open failed: %s", path);
    selectTouchSpi();
    return errorBuf;
  }
  wifiUploadActive = true;
  wifiUploadModel = targetModel;
  wifiUploadOffset = 0;
  return nullptr;
}

static const char* appendOfficialCfgChunk(QidiPrinterModel targetModel, size_t offset, WiFiClient& client, size_t contentLength) {
  if (!wifiUploadActive) return "Upload not initialized";
  if (targetModel != wifiUploadModel) return "Upload model mismatch";
  if (offset != wifiUploadOffset) return "Upload offset mismatch";
  if ((offset + contentLength) > WIFI_HTTP_MAX_UPLOAD_SIZE) return "Upload too large";
  selectSdSpi();

  uint8_t buf[WIFI_HTTP_CHUNK_SIZE];
  size_t remaining = contentLength;
  while (remaining > 0) {
    size_t want = min(remaining, (size_t)sizeof(buf));
    size_t got = client.readBytes((char*)buf, want);
    if (got == 0) {
      if (wifiUploadFile) wifiUploadFile.close();
      wifiUploadActive = false;
      removeOfficialCfgFilesForModel(targetModel);
      selectTouchSpi();
      return "Read timeout";
    }

    size_t written = wifiUploadFile.write(buf, got);
    if (written != got) {
      if (wifiUploadFile) wifiUploadFile.close();
      wifiUploadActive = false;
      removeOfficialCfgFilesForModel(targetModel);
      selectTouchSpi();
      return "SD write failed";
    }

    remaining -= got;
    wifiUploadOffset += got;
  }
  return nullptr;
}

static const char* finishOfficialCfgUpload(QidiPrinterModel targetModel) {
  if (!wifiUploadActive) return "Upload not initialized";
  if (targetModel != wifiUploadModel) return "Upload model mismatch";
  selectSdSpi();
  if (wifiUploadFile) {
    wifiUploadFile.flush();
    wifiUploadFile.close();
  }
  wifiUploadActive = false;
  wifiUploadOffset = 0;
  selectTouchSpi();
  return nullptr;
}

static void ensureWifiServerStarted() {
  if (wifiServerStarted || WiFi.status() != WL_CONNECTED) return;
  uploadServer.begin();
  uploadServer.setNoDelay(true);
  wifiServerStarted = true;
}

static void stopWifiServer() {
  if (wifiUploadActive) {
    selectSdSpi();
    if (wifiUploadFile) wifiUploadFile.close();
    removeOfficialCfgFilesForModel(wifiUploadModel);
    wifiUploadActive = false;
    wifiUploadOffset = 0;
    selectTouchSpi();
  }
  if (!wifiServerStarted) return;
  uploadServer.stop();
  wifiServerStarted = false;
}

static void handleWifiHttpClient() {
  if (!wifiServerStarted || WiFi.status() != WL_CONNECTED) return;

  WiFiClient client = uploadServer.accept();
  if (!client) return;

  uint32_t requestStart = millis();
  while (!client.available() && client.connected() && (millis() - requestStart) < WIFI_HTTP_READ_TIMEOUT_MS) {
    delay(1);
  }
  if (!client.available()) {
    sendHttpResponse(client, "408 Request Timeout", "text/plain; charset=utf-8", "Request timeout");
    closeHttpClient(client);
    return;
  }

  client.setTimeout(WIFI_HTTP_READ_TIMEOUT_MS);

  String requestLine = client.readStringUntil('\n');
  requestLine.trim();
  int firstSpace = requestLine.indexOf(' ');
  int secondSpace = requestLine.indexOf(' ', firstSpace + 1);
  if (firstSpace <= 0 || secondSpace <= firstSpace) {
    closeHttpClient(client);
    return;
  }

  String method = requestLine.substring(0, firstSpace);
  String requestPath = requestLine.substring(firstSpace + 1, secondSpace);
  size_t contentLength = 0;

  while (client.connected()) {
    String line = client.readStringUntil('\n');
    if (line == "\r" || line.length() == 0) break;
    line.trim();
    String lowerLine = line;
    lowerLine.toLowerCase();
    if (lowerLine.startsWith("content-length:")) {
      contentLength = (size_t)line.substring(15).toInt();
    }
  }

  if (method == "GET" && (requestPath == "/" || requestPath.startsWith("/?"))) {
    String html;
    html.reserve(2500);
    html += F("<!doctype html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>");
    html += F("<title>QIDI Upload</title></head><body><h1>QIDI Upload</h1><p><code>officiall_filas_list.cfg</code></p><p>SD: ");
    html += sdAvailable ? LTXT(TXT_SD_READY) : LTXT(TXT_SD_MISSING);
    html += F("</p><p>Q2</p><input id='q2' type='file' accept='.cfg'><button onclick=\"u('q2')\">Upload Q2</button>");
    html += F("<p>Plus 4</p><input id='plus4' type='file' accept='.cfg'><button onclick=\"u('plus4')\">Upload Plus 4</button>");
    html += F("<p>Max 4</p><input id='max4' type='file' accept='.cfg'><button onclick=\"u('max4')\">Upload Max 4</button>");
    html += F("<p id='msg'></p><script>");
    html += F("async function u(m){const i=document.getElementById(m),o=document.getElementById('msg');");
    html += F("if(!i.files.length){o.textContent='Datei fehlt';return;}o.textContent='Upload...';");
    html += F("try{const txt=await i.files[0].text();");
    html += F("let r=await fetch('/upload_begin?model='+m,{method:'POST',body:''});let t=await r.text();if(!r.ok)throw new Error(t);");
    html += F("for(let off=0;off<txt.length;off+=");
    html += String(WIFI_HTTP_CHUNK_SIZE);
    html += F("){const part=txt.slice(off,off+");
    html += String(WIFI_HTTP_CHUNK_SIZE);
    html += F(");r=await fetch('/upload_chunk?model='+m+'&offset='+off,{method:'POST',headers:{'Content-Type':'text/plain;charset=utf-8'},body:part});t=await r.text();if(!r.ok)throw new Error(t);}");
    html += F("r=await fetch('/upload_end?model='+m,{method:'POST',body:''});t=await r.text();if(!r.ok)throw new Error(t);o.textContent=t;");
    html += F("}catch(e){o.textContent=e&&e.message?e.message:'Fehler';}}</script>");
    html += F("</body></html>");
    sendHttpResponse(client, "200 OK", "text/html; charset=utf-8", html);
    closeHttpClient(client);
    return;
  }

  if (method == "POST" && requestPath.startsWith("/upload_begin")) {
    if (!ensureSdAccess(true)) {
      sendHttpResponse(client, "503 Service Unavailable", "text/plain; charset=utf-8", "SD fehlt");
      closeHttpClient(client);
      return;
    }
    QidiPrinterModel targetModel = qidiModelFromRequestPath(requestPath);
    const char* uploadError = beginOfficialCfgUpload(targetModel);
    if (!uploadError) sendHttpResponse(client, "200 OK", "text/plain; charset=utf-8", "OK");
    else sendHttpResponse(client, "400 Bad Request", "text/plain; charset=utf-8", uploadError);
    closeHttpClient(client);
    return;
  }

  if (method == "POST" && requestPath.startsWith("/upload_chunk")) {
    if (contentLength > WIFI_HTTP_CHUNK_SIZE) {
      sendHttpResponse(client, "413 Payload Too Large", "text/plain; charset=utf-8", "Chunk gross");
      closeHttpClient(client);
      return;
    }
    QidiPrinterModel targetModel = qidiModelFromRequestPath(requestPath);
    int offsetPos = requestPath.indexOf("offset=");
    size_t offset = 0;
    if (offsetPos >= 0) offset = (size_t)requestPath.substring(offsetPos + 7).toInt();
    const char* uploadError = appendOfficialCfgChunk(targetModel, offset, client, contentLength);
    if (!uploadError) sendHttpResponse(client, "200 OK", "text/plain; charset=utf-8", "OK");
    else sendHttpResponse(client, "400 Bad Request", "text/plain; charset=utf-8", uploadError);
    closeHttpClient(client);
    return;
  }

  if (method == "POST" && requestPath.startsWith("/upload_end")) {
    QidiPrinterModel targetModel = qidiModelFromRequestPath(requestPath);
    const char* uploadError = finishOfficialCfgUpload(targetModel);
    if (!uploadError) {
      refreshOfficialListAvailability();
      if (currentTagMode == TAGMODE_QIDI) reloadModeDatabases();
      selectTouchSpi();
      sendHttpResponse(client, "200 OK", "text/plain; charset=utf-8", "Fertig");
    } else {
      sendHttpResponse(client, "400 Bad Request", "text/plain; charset=utf-8", uploadError);
    }
    closeHttpClient(client);
    return;
  }

  sendHttpResponse(client, "404 Not Found", "text/plain; charset=utf-8", "Not found");
  closeHttpClient(client);
}

static void applyWifiState(bool forceReconnect) {
  if (!wifiEnabled || strlen(wifiSsid) == 0) {
    stopWifiServer();
    wifiConnectPending = false;
    WiFi.disconnect(true, true);
    return;
  }

  if (WiFi.status() == WL_CONNECTED && !forceReconnect) {
    ensureWifiServerStarted();
    return;
  }

  stopWifiServer();
  WiFi.mode(WIFI_STA);
  if (forceReconnect) WiFi.disconnect(true, true);
  WiFi.begin(wifiSsid, wifiPassword);
  wifiConnectPending = true;
  wifiConnectStartedMs = millis();
  lastWifiRetryMs = wifiConnectStartedMs;
}

static void handleWifiTasks() {
  if (!wifiEnabled || strlen(wifiSsid) == 0) {
    wifiConnectPending = false;
    stopWifiServer();
    return;
  }

  wl_status_t wifiStatus = WiFi.status();
  uint32_t now = millis();

  if (wifiStatus == WL_CONNECTED) {
    wifiConnectPending = false;
    ensureWifiServerStarted();
    handleWifiHttpClient();
    return;
  }

  stopWifiServer();

  if (wifiConnectPending) {
    if (now - wifiConnectStartedMs >= WIFI_CONNECT_TIMEOUT_MS) {
      wifiConnectPending = false;
      WiFi.disconnect(false, true);
    }
    return;
  }

  if (now - lastWifiRetryMs >= WIFI_RETRY_INTERVAL_MS) {
    applyWifiState(true);
  }
}

static bool parseOfficialCfgFile(bool loadMaterialsFromFile, bool loadManufacturersFromFile) {
  if (currentTagMode != TAGMODE_QIDI) return false;
  if (!isOfficialListActiveForCurrentQidiModel()) return false;
  if (!ensureSdAccess(true)) {
    selectTouchSpi();
    return false;
  }

  File file = SD.open(officialListPathForModel(qidiPrinterModel), FILE_READ);
  if (!file) {
    selectTouchSpi();
    return false;
  }

  if (loadMaterialsFromFile) {
    for (uint8_t i = 1; i <= MAX_MATERIALS; i++) {
      gMaterials[i].active = false;
      gMaterials[i].name[0] = '\0';
      gMaterials[i].nozzleMin = 0;
      gMaterials[i].nozzleMax = 0;
      gMaterials[i].bedMin = 0;
      gMaterials[i].bedMax = 0;
    }
  }
  if (loadManufacturersFromFile) {
    for (uint8_t i = 0; i <= MAX_MANUFACTURERS; i++) {
      gManufacturers[i].active = false;
      gManufacturers[i].name[0] = '\0';
    }
  }

  String section = "";
  int filaId = -1;
  String filaName = "";
  String filaType = "";
  uint16_t filaMinTemp = 0;
  uint16_t filaMaxTemp = 0;
  uint16_t filaBedMin = 0;
  uint16_t filaBedMax = 0;
  bool anyMaterials = false;
  bool anyManufacturers = false;

  auto commitFila = [&]() {
    if (!loadMaterialsFromFile) return;
    if (filaId < 1 || filaId > MAX_MATERIALS) return;
    String label = filaName.length() ? filaName : filaType;
    label = cfgTrimmed(label);
    if (label.length() == 0) return;
    gMaterials[filaId].active = true;
    safeCopy(gMaterials[filaId].name, label.c_str(), sizeof(gMaterials[filaId].name));
    gMaterials[filaId].nozzleMin = filaMinTemp;
    gMaterials[filaId].nozzleMax = filaMaxTemp;
    gMaterials[filaId].bedMin = filaBedMin;
    gMaterials[filaId].bedMax = filaBedMax;
    anyMaterials = true;
  };

  while (file.available()) {
    String rawLine = file.readStringUntil('\n');
    int semicolon = rawLine.indexOf(';');
    int hash = rawLine.indexOf('#');
    int commentPos = -1;
    if (semicolon >= 0 && hash >= 0) commentPos = min(semicolon, hash);
    else if (semicolon >= 0) commentPos = semicolon;
    else if (hash >= 0) commentPos = hash;
    if (commentPos >= 0) rawLine.remove(commentPos);
    String line = cfgTrimmed(rawLine);
    if (line.length() == 0) continue;

    if (line.startsWith("[") && line.endsWith("]")) {
      commitFila();
      section = cfgTrimmed(line.substring(1, line.length() - 1));
      String lowerSection = section;
      lowerSection.toLowerCase();
      if (lowerSection.startsWith("fila")) {
        filaId = extractTrailingNumber(lowerSection);
        filaName = "";
        filaType = "";
        filaMinTemp = 0;
        filaMaxTemp = 0;
        filaBedMin = 0;
        filaBedMax = 0;
      } else {
        filaId = -1;
      }
      continue;
    }

    int eq = line.indexOf('=');
    if (eq < 0) continue;
    String key = cfgTrimmed(line.substring(0, eq));
    String value = cfgTrimmed(line.substring(eq + 1));
    String lowerSection = section;
    String lowerKey = key;
    lowerSection.toLowerCase();
    lowerKey.toLowerCase();

    if (loadManufacturersFromFile && lowerSection == "vendor_list") {
      int vendorId = extractTrailingNumber(lowerKey);
      if (vendorId >= 0 && vendorId <= MAX_MANUFACTURERS && value.length() > 0) {
        gManufacturers[vendorId].active = true;
        safeCopy(gManufacturers[vendorId].name, value.c_str(), sizeof(gManufacturers[vendorId].name));
        anyManufacturers = true;
      }
      continue;
    }

    if (loadMaterialsFromFile && lowerSection.startsWith("fila")) {
      if (lowerKey == "filament") filaName = value;
      else if (lowerKey == "type") filaType = value;
      else if (lowerKey == "min_temp") filaMinTemp = parseCfgU16(value);
      else if (lowerKey == "max_temp") filaMaxTemp = parseCfgU16(value);
      else if (lowerKey == "box_min_temp") filaBedMin = parseCfgU16(value);
      else if (lowerKey == "box_max_temp") filaBedMax = parseCfgU16(value);
    }
  }

  commitFila();
  file.close();
  selectTouchSpi();

  if (loadManufacturersFromFile) {
    if (!gManufacturers[MFG_GENERIC].active) {
      gManufacturers[MFG_GENERIC].active = true;
      safeCopy(gManufacturers[MFG_GENERIC].name, "Generic", sizeof(gManufacturers[MFG_GENERIC].name));
    }
    if (!gManufacturers[MFG_QIDI].active) {
      gManufacturers[MFG_QIDI].active = true;
      safeCopy(gManufacturers[MFG_QIDI].name, "QIDI", sizeof(gManufacturers[MFG_QIDI].name));
    }
  }

  if (loadMaterialsFromFile && anyMaterials) ensureSelectedMaterialValid();
  if (loadManufacturersFromFile && !gManufacturers[selMfg].active) selMfg = MFG_QIDI;
  return loadMaterialsFromFile ? anyMaterials : anyManufacturers;
}

static bool loadQidiMaterialsFromOfficialCfg() {
  return parseOfficialCfgFile(true, false);
}

static bool loadQidiManufacturersFromOfficialCfg() {
  return parseOfficialCfgFile(false, true);
}

static void resetAllModeDatabasesToDefault() {
  TagMode savedMode = currentTagMode;
  QidiPrinterModel savedModel = qidiPrinterModel;

  currentTagMode = TAGMODE_QIDI;
  qidiPrinterModel = QIDI_MODEL_PLUS4;
  resetMaterialsToDefault();
  resetManufacturersToDefault();

  currentTagMode = TAGMODE_QIDI;
  qidiPrinterModel = QIDI_MODEL_Q2;
  resetMaterialsToDefault();
  resetManufacturersToDefault();

  currentTagMode = TAGMODE_QIDI;
  qidiPrinterModel = QIDI_MODEL_MAX4;
  resetMaterialsToDefault();
  resetManufacturersToDefault();

  currentTagMode = TAGMODE_OPENSPOOL;
  resetMaterialsToDefault();
  resetManufacturersToDefault();
  resetVariantsToDefault();

  currentTagMode = savedMode;
  qidiPrinterModel = savedModel;
}

static void clearLanguagePrefs() {
  prefs.begin(PREF_NS_UI, false);
  prefs.clear();
  prefs.end();
  uiLang = LANG_EN;
  displayInversionEnabled = false;
  autoDetectEnabled = true;
  defaultTagMode = TAGMODE_QIDI;
  qidiPrinterModel = QIDI_MODEL_PLUS4;
  screensaverMode = SCREENSAVER_5MIN;
  displayBrightness = 80;
  osInfoStdNozzleEnabled = true;
  osInfoU1BedEnabled = true;
  osInfoU1AlphaEnabled = true;
  osInfoU1WeightEnabled = false;
  osInfoU1DiameterEnabled = false;
  osInfoU1AddColorsEnabled = false;
  osReadIntervalSec = 2;
  wifiEnabled = false;
  wifiSsid[0] = '\0';
  wifiPassword[0] = '\0';
  useOfficialListPlus4 = false;
  useOfficialListQ2 = false;
  useOfficialListMax4 = false;
}

static void loadLanguage() {
  loadWirelessSettings();
  prefs.begin(PREF_NS_UI, true);
  size_t blobLen = prefs.getBytesLength(PREF_UI_BLOB);
  bool loadedFromBlob = false;
  if (blobLen == sizeof(UiSettingsBlob)) {
    UiSettingsBlob blob = {};
    if (prefs.getBytes(PREF_UI_BLOB, &blob, sizeof(blob)) == sizeof(blob) &&
        blob.version == UI_SETTINGS_BLOB_VERSION) {
      uint8_t v = blob.lang;
      uint8_t dm = blob.defaultMode;
      uint8_t qm = blob.qidiPrinter;
      uint8_t sm = blob.screensaverMode;
      uint8_t br = blob.brightness;
      uint8_t ri = blob.readInterval;
      if (v >= LANG_COUNT) v = (uint8_t)LANG_EN;
      if (dm > (uint8_t)TAGMODE_OPENSPOOL) dm = (uint8_t)TAGMODE_QIDI;
      if (qm > (uint8_t)QIDI_MODEL_MAX4) qm = (uint8_t)QIDI_MODEL_PLUS4;
      if (sm > (uint8_t)SCREENSAVER_OFF) sm = (uint8_t)SCREENSAVER_5MIN;
      if (br < 10 || br > 100 || (br % 10) != 0) br = 80;
      if (ri < 1 || ri > 4) ri = 2;
      uiLang = (UiLang)v;
      defaultTagMode = (TagMode)dm;
      qidiPrinterModel = (QidiPrinterModel)qm;
      screensaverMode = (ScreensaverMode)sm;
      displayBrightness = br;
      displayInversionEnabled = (blob.flags & UI_FLAG_DISPLAY_INV) != 0;
      autoDetectEnabled = (blob.flags & UI_FLAG_AUTO_READ) != 0;
      osInfoStdNozzleEnabled = (blob.flags & UI_FLAG_OS_STD_NOZZLE) != 0;
      osInfoU1BedEnabled = (blob.flags & UI_FLAG_OS_U1_BED) != 0;
      osInfoU1AlphaEnabled = (blob.flags & UI_FLAG_OS_U1_ALPHA) != 0;
      osInfoU1WeightEnabled = (blob.flags & UI_FLAG_OS_U1_WEIGHT) != 0;
      osInfoU1DiameterEnabled = (blob.flags & UI_FLAG_OS_U1_DIAM) != 0;
      osInfoU1AddColorsEnabled = (blob.flags & UI_FLAG_OS_U1_ADDC) != 0;
      osReadIntervalSec = ri;
      currentTagMode = defaultTagMode;
      loadedFromBlob = true;
    }
  }

  if (!loadedFromBlob) {
    uint8_t v = prefs.getUChar(PREF_LANG, (uint8_t)LANG_EN);
    displayInversionEnabled = prefs.getBool(PREF_DISPLAY_INV, false);
    autoDetectEnabled = prefs.getBool(PREF_AUTO_READ, true);
    uint8_t dm = prefs.getUChar(PREF_DEFAULT_MODE, (uint8_t)TAGMODE_QIDI);
    uint8_t qm = prefs.getUChar(PREF_QIDI_PRINTER, (uint8_t)QIDI_MODEL_PLUS4);
    uint8_t sm = prefs.getUChar(PREF_SCREENSAVER_MODE, (uint8_t)SCREENSAVER_5MIN);
    uint8_t br = prefs.getUChar(PREF_BRIGHTNESS, 80);
    osInfoStdNozzleEnabled = prefs.getBool(PREF_OS_STD_NOZZLE, true);
    osInfoU1BedEnabled = prefs.getBool(PREF_OS_U1_BED, true);
    osInfoU1AlphaEnabled = prefs.getBool(PREF_OS_U1_ALPHA, true);
    osInfoU1WeightEnabled = prefs.getBool(PREF_OS_U1_WEIGHT, false);
    osInfoU1DiameterEnabled = prefs.getBool(PREF_OS_U1_DIAM, false);
    osInfoU1AddColorsEnabled = prefs.getBool(PREF_OS_U1_ADDC, false);
    osReadIntervalSec = prefs.getUChar(PREF_OS_READ_INT, 2);
    if (v >= LANG_COUNT) v = (uint8_t)LANG_EN;
    if (dm > (uint8_t)TAGMODE_OPENSPOOL) dm = (uint8_t)TAGMODE_QIDI;
    if (qm > (uint8_t)QIDI_MODEL_MAX4) qm = (uint8_t)QIDI_MODEL_PLUS4;
    if (sm > (uint8_t)SCREENSAVER_OFF) sm = (uint8_t)SCREENSAVER_5MIN;
    if (br < 10 || br > 100 || (br % 10) != 0) br = 80;
    if (osReadIntervalSec < 1 || osReadIntervalSec > 4) osReadIntervalSec = 2;
    uiLang = (UiLang)v;
    defaultTagMode = (TagMode)dm;
    qidiPrinterModel = (QidiPrinterModel)qm;
    screensaverMode = (ScreensaverMode)sm;
    displayBrightness = br;
    currentTagMode = defaultTagMode;
  }
  prefs.end();
  if (!loadedFromBlob) saveUiSettingsBlob();
}

static void factoryResetSettings() {
  clearCalibrationPrefs();
  clearLanguagePrefs();
  currentTagMode = TAGMODE_QIDI;
  defaultTagMode = TAGMODE_QIDI;
  qidiPrinterModel = QIDI_MODEL_PLUS4;
  screensaverMode = SCREENSAVER_5MIN;
  displayBrightness = 80;
  osInfoStdNozzleEnabled = true;
  osInfoU1BedEnabled = true;
  osInfoU1AlphaEnabled = true;
  osInfoU1WeightEnabled = false;
  osInfoU1DiameterEnabled = false;
  osInfoU1AddColorsEnabled = false;
  osReadIntervalSec = 2;
  wifiEnabled = false;
  wifiSsid[0] = '\0';
  wifiPassword[0] = '\0';
  useOfficialListPlus4 = false;
  useOfficialListQ2 = false;
  useOfficialListMax4 = false;
  saveAllSetupPreferences();
  resetAllModeDatabasesToDefault();
  currentTagMode = TAGMODE_QIDI;
  defaultTagMode = TAGMODE_QIDI;
  qidiPrinterModel = QIDI_MODEL_PLUS4;
  saveAllSetupPreferences();
  reloadModeDatabases();
  resetOpenSpoolFieldsToDefault();
  osDraftsInitialized = false;
  initOpenSpoolDrafts();
  drawStatus(TR(STR_FACTORY_RESET_DONE), TFT_GREEN);
  needRedraw = true;
}

// ==================== Touch functions ====================
static bool getTouchRaw(int& rx, int& ry, int& rz) {
  if (wifiUploadActive) return false;
  selectTouchSpi();
  ts.isrWake = true;
  if (!ts.touched()) return false;
  TS_Point p = ts.getPoint();
  rx = p.x; ry = p.y; rz = p.z;
  return true;
}

static bool getTouchXY(int& x, int& y) {
  int rx, ry, rz;
  if (!getTouchRaw(rx, ry, rz)) return false;

  if (TOUCH_SWAP_XY) { int tmp = rx; rx = ry; ry = tmp; }

  x = map(rx, TS_MINX, TS_MAXX, 0, TFT_W);
  y = map(ry, TS_MINY, TS_MAXY, 0, TFT_H);

  if (TOUCH_INVERT_X) x = TFT_W - x;
  if (TOUCH_INVERT_Y) y = TFT_H - y;

  x = constrain(x, 0, TFT_W - 1);
  y = constrain(y, 0, TFT_H - 1);
  return true;
}

// ==================== Calibration UI ====================
static void drawCrosshair(int x, int y, uint16_t color) {
  const int s = 10;
  tft.drawLine(x - s, y, x + s, y, color);
  tft.drawLine(x, y - s, x, y + s, color);
  tft.drawCircle(x, y, 14, color);
}

static void drawCenteredHint(const char* msg) {
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  String s = String(msg);
  if ((int)tft.textWidth(s, 2) <= TFT_W - 40) {
    tft.drawCentreString(s, TFT_W / 2, TFT_H / 2 - 10, 2);
  } else {
    int split = s.lastIndexOf(' ');
    if (split < 0) split = s.length() / 2;
    String a = s.substring(0, split);
    String b = s.substring(split);
    b.trim();
    tft.drawCentreString(a, TFT_W / 2, TFT_H / 2 - 18, 2);
    tft.drawCentreString(b, TFT_W / 2, TFT_H / 2 + 4, 2);
  }
  tft.setTextDatum(TL_DATUM);
}

static bool captureCalibrationPoint(int targetX, int targetY, uint32_t holdMs, int radiusPx, int& outRawX, int& outRawY) {
  uint32_t start = millis();
  bool inZone = false;
  uint32_t inZoneSince = 0;
  int64_t sumX = 0, sumY = 0;
  int samples = 0;

  while (millis() - start < 25000) {
    int sx, sy;
    if (getTouchXY(sx, sy)) {
      int dx = sx - targetX;
      int dy = sy - targetY;
      if (dx * dx + dy * dy <= radiusPx * radiusPx) {
        if (!inZone) {
          inZone = true;
          inZoneSince = millis();
          sumX = 0; sumY = 0; samples = 0;
        }
        int rx, ry, rz;
        if (getTouchRaw(rx, ry, rz)) {
          if (TOUCH_SWAP_XY) { int tmp = rx; rx = ry; ry = tmp; }
          sumX += rx;
          sumY += ry;
          samples++;
        }
        uint32_t held = millis() - inZoneSince;
        int bx = 20, by = TFT_H - UI_STATUS_H - 20;
        int barW = TFT_W - 40, barH = 10;
        tft.drawRect(bx, by, barW, barH, TFT_DARKGREY);
        int fill = (held >= holdMs) ? barW : (int)(barW * (float)held / (float)holdMs);
        tft.fillRect(bx + 1, by + 1, max(0, fill - 2), barH - 2, TFT_GREEN);
        if (held >= holdMs && samples >= 5) {
          outRawX = (int)(sumX / samples);
          outRawY = (int)(sumY / samples);
          return true;
        }
      } else {
        inZone = false;
      }
    } else {
      inZone = false;
    }
    delay(15);
  }
  return false;
}

static void calibrateTouch() {
  const int margin = 18;
  const int pts[4][2] = {
    { margin, margin },
    { TFT_W - margin, margin },
    { TFT_W - margin, TFT_H - margin },
    { margin, TFT_H - margin }
  };

  int rawX[4] = {0}, rawY[4] = {0};

  for (int i = 0; i < 4; i++) {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.drawString(TR(STR_CALIBRATE), TFT_W / 2, 10, 2);
    tft.setTextDatum(TL_DATUM);
    drawCenteredHint(TR(STR_CALIBRATE_HINT));
    char stepBuf[16];
    snprintf(stepBuf, sizeof(stepBuf), "%d/4", i + 1);
    tft.setTextColor(TFT_YELLOW);
    tft.drawRightString(stepBuf, TFT_W - 6, 6, 2);
    drawCrosshair(pts[i][0], pts[i][1], TFT_RED);

    int rx = 0, ry = 0;
    if (!captureCalibrationPoint(pts[i][0], pts[i][1], 1000, 32, rx, ry)) {
      tft.fillScreen(TFT_BLACK);
      drawCenteredHint(TR(STR_CALIBRATION_ABORTED));
      delay(1200);
      return;
    }
    rawX[i] = rx;
    rawY[i] = ry;
    drawCrosshair(pts[i][0], pts[i][1], TFT_GREEN);
    delay(350);
  }

  int minx = (rawX[0] + rawX[3]) / 2;
  int maxx = (rawX[1] + rawX[2]) / 2;
  int miny = (rawY[0] + rawY[1]) / 2;
  int maxy = (rawY[2] + rawY[3]) / 2;
  if (minx > maxx) { int t = minx; minx = maxx; maxx = t; }
  if (miny > maxy) { int t = miny; miny = maxy; maxy = t; }

  TS_MINX = minx; TS_MAXX = maxx; TS_MINY = miny; TS_MAXY = maxy;
  saveCalibration(TS_MINX, TS_MAXX, TS_MINY, TS_MAXY);

  tft.fillScreen(TFT_BLACK);
  drawCenteredHint(TR(STR_CALIBRATION_SAVED));
  delay(1200);
}

// ==================== NFC helpers ====================
static bool waitForTagUID(uint8_t* uid, uint8_t& uidLen, uint32_t timeoutMs) {
  uint32_t start = millis();
  uidLen = 0;
  while (millis() - start < timeoutMs) {
    if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLen, 50)) return true;
    delay(10);
  }
  return false;
}

static bool authBlockWithDefaultKeyA(uint8_t* uid, uint8_t uidLen, uint8_t block) {
  uint8_t keya[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
  return nfc.mifareclassic_AuthenticateBlock(uid, uidLen, block, 0, keya);
}

static bool readBlock(uint8_t block, uint8_t* data) {
  return nfc.mifareclassic_ReadDataBlock(block, data);
}

static bool writeBlock(uint8_t block, const uint8_t* data) {
  return nfc.mifareclassic_WriteDataBlock(block, (uint8_t*)data);
}

// ==================== OpenSpool helpers ====================
static const uint8_t NTAG_FIRST_USER_PAGE = 4;
static const uint8_t NTAG_LAST_USER_PAGE  = 129;
static const size_t  NTAG_USER_BYTES      = (NTAG_LAST_USER_PAGE - NTAG_FIRST_USER_PAGE + 1) * 4;
static const uint8_t NTAG_CC_PAGE         = 3;
static const uint8_t NTAG_CC[4]           = {0xE1, 0x10, 0x3E, 0x00};

static bool detectIsoA(uint8_t* uid, uint8_t& uidLen, uint16_t tries) {
  uidLen = 0;
  for (uint16_t i = 0; i < tries; i++) {
    if (nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLen)) {
      delay(10);
      return true;
    }
    delay(15);
  }
  return false;
}

static bool readPageRetry(uint8_t page, uint8_t* out) {
  for (uint8_t i = 0; i < 5; i++) {
    if (nfc.ntag2xx_ReadPage(page, out)) return true;
    delay(20);
  }
  return false;
}

static bool writePageRetry(uint8_t page, const uint8_t* in) {
  uint8_t tmp[4];
  memcpy(tmp, in, 4);
  for (uint8_t i = 0; i < 5; i++) {
    if (nfc.ntag2xx_WritePage(page, tmp)) return true;
    delay(20);
  }
  return false;
}

static bool ensureOpenSpoolCC() {
  uint8_t page[4];
  if (!readPageRetry(NTAG_CC_PAGE, page)) return false;
  if (memcmp(page, NTAG_CC, 4) == 0) return true;
  if (!writePageRetry(NTAG_CC_PAGE, NTAG_CC)) return false;
  if (!readPageRetry(NTAG_CC_PAGE, page)) return false;
  return memcmp(page, NTAG_CC, 4) == 0;
}

static bool readOpenSpoolUserArea(uint8_t* out) {
  memset(out, 0x00, NTAG_USER_BYTES);
  size_t offset = 0;
  for (uint8_t p = NTAG_FIRST_USER_PAGE; p <= NTAG_LAST_USER_PAGE; p++) {
    if (!readPageRetry(p, out + offset)) return false;
    offset += 4;
    size_t ndefOffset = 0, ndefLen = 0;
    if (findOpenSpoolNdefTlv(out, offset, ndefOffset, ndefLen)) {
      size_t needed = ndefOffset + ndefLen + 1;
      if (offset >= needed) return true;
    }
  }
  return true;
}

static bool writeOpenSpoolUserArea(const uint8_t* in, size_t len) {
  uint8_t page[4];
  size_t offset = 0;
  size_t bytesToWrite = len;
  if (bytesToWrite < 4) bytesToWrite = 4;
  size_t pageCount = (bytesToWrite + 3) / 4;
  if (pageCount > (NTAG_LAST_USER_PAGE - NTAG_FIRST_USER_PAGE + 1)) pageCount = (NTAG_LAST_USER_PAGE - NTAG_FIRST_USER_PAGE + 1);
  for (uint8_t p = NTAG_FIRST_USER_PAGE; p < NTAG_FIRST_USER_PAGE + pageCount; p++) {
    for (uint8_t i = 0; i < 4; i++) page[i] = (offset < len) ? in[offset++] : 0x00;
    if (!writePageRetry(p, page)) return false;
    delay(2);
  }
  return true;
}

static bool buildOpenSpoolNdefFromJson(const String& json, uint8_t* out, size_t outMax, size_t& outLen) {
  const char* mime = "application/json";
  const uint8_t typeLen = (uint8_t)strlen(mime);
  const size_t payloadLen = json.length();
  const bool shortRecord = payloadLen <= 255;
  const size_t recordLen = 1 + 1 + (shortRecord ? 1 : 4) + typeLen + payloadLen;
  const bool shortTlv = recordLen <= 254;
  const size_t totalLen = 1 + (shortTlv ? 1 : 3) + recordLen + 1;
  if (totalLen > outMax) return false;

  size_t p = 0;
  out[p++] = 0x03;
  if (shortTlv) {
    out[p++] = (uint8_t)recordLen;
  } else {
    out[p++] = 0xFF;
    out[p++] = (uint8_t)((recordLen >> 8) & 0xFF);
    out[p++] = (uint8_t)(recordLen & 0xFF);
  }

  out[p++] = shortRecord ? 0xD2 : 0xC2; // MB|ME|SR? + MIME
  out[p++] = typeLen;
  if (shortRecord) {
    out[p++] = (uint8_t)payloadLen;
  } else {
    out[p++] = (uint8_t)((payloadLen >> 24) & 0xFF);
    out[p++] = (uint8_t)((payloadLen >> 16) & 0xFF);
    out[p++] = (uint8_t)((payloadLen >> 8) & 0xFF);
    out[p++] = (uint8_t)(payloadLen & 0xFF);
  }

  memcpy(out + p, mime, typeLen); p += typeLen;
  memcpy(out + p, json.c_str(), payloadLen); p += payloadLen;
  out[p++] = 0xFE;
  outLen = p;
  return true;
}

static bool findOpenSpoolNdefTlv(const uint8_t* buf, size_t len, size_t& ndefOffset, size_t& ndefLen) {
  size_t i = 0;
  while (i < len) {
    uint8_t t = buf[i];
    if (t == 0x00) { i++; continue; }
    if (t == 0xFE || i + 1 >= len) return false;
    if (t == 0x03) {
      uint8_t l = buf[i + 1];
      if (l == 0xFF) {
        if (i + 3 >= len) return false;
        ndefLen = ((size_t)buf[i + 2] << 8) | buf[i + 3];
        ndefOffset = i + 4;
      } else {
        ndefLen = l;
        ndefOffset = i + 2;
      }
      return (ndefOffset + ndefLen <= len);
    }
    uint8_t l = buf[i + 1];
    if (l == 0xFF) {
      if (i + 3 >= len) return false;
      size_t longLen = ((size_t)buf[i + 2] << 8) | buf[i + 3];
      i += 4 + longLen;
    } else {
      i += 2 + l;
    }
  }
  return false;
}

static bool parseOpenSpoolMimeRecord(const uint8_t* ndef, size_t ndefLen, String& mime, String& payload) {
  if (ndefLen < 3) return false;
  uint8_t hdr = ndef[0];
  bool sr = hdr & 0x10;
  bool il = hdr & 0x08;
  uint8_t tnf = hdr & 0x07;
  if (tnf != 0x02) return false;
  size_t p = 1;
  uint8_t typeLen = ndef[p++];
  uint32_t payloadLen = 0;
  if (sr) {
    if (p >= ndefLen) return false;
    payloadLen = ndef[p++];
  } else {
    if (p + 3 >= ndefLen) return false;
    payloadLen = ((uint32_t)ndef[p] << 24) | ((uint32_t)ndef[p + 1] << 16) | ((uint32_t)ndef[p + 2] << 8) | ndef[p + 3];
    p += 4;
  }
  uint8_t idLen = 0;
  if (il) {
    if (p >= ndefLen) return false;
    idLen = ndef[p++];
  }
  if (p + typeLen + idLen + payloadLen > ndefLen) return false;
  mime = ""; payload = "";
  for (uint8_t i = 0; i < typeLen; i++) mime += (char)ndef[p + i];
  p += typeLen + idLen;
  for (uint32_t i = 0; i < payloadLen; i++) payload += (char)ndef[p + i];
  return true;
}

static String colorHexFrom565(uint16_t c) {
  uint8_t r = ((c >> 11) & 0x1F) * 255 / 31;
  uint8_t g = ((c >> 5) & 0x3F) * 255 / 63;
  uint8_t b = (c & 0x1F) * 255 / 31;
  char buf[8];
  snprintf(buf, sizeof(buf), "#%02X%02X%02X", r, g, b);
  return String(buf);
}

static void syncOpenSpoolColorFromSelection() {
  safeCopy(osColorHex, colorHexFrom565(COLORS[selColIdx].rgb565).c_str(), sizeof(osColorHex));
}

static String normalizeHexColor(const char* s, bool withHash) {
  String x = String(s ? s : "");
  x.trim();
  x.replace(" ", "");
  x.toUpperCase();
  if (x.startsWith("#")) x = x.substring(1);
  while (x.length() < 6) x = "0" + x;
  if (x.length() > 6) x = x.substring(0, 6);
  return withHash ? String("#") + x : x;
}

static String colorHexById(uint8_t id, bool withHash) {
  for (size_t i = 0; i < COLORS_COUNT; i++) {
    if (COLORS[i].id == id) {
      return normalizeHexColor(colorHexFrom565(COLORS[i].rgb565).c_str(), withHash);
    }
  }
  return withHash ? String("#000000") : String("000000");
}

static bool parseColor565FromHex(const char* s, uint16_t& out565) {
  String x = normalizeHexColor(s, false);
  if (x.length() != 6) return false;
  char* endptr = nullptr;
  long v = strtol(x.c_str(), &endptr, 16);
  if (!endptr || *endptr != 0) return false;
  uint8_t r = (v >> 16) & 0xFF;
  uint8_t g = (v >> 8) & 0xFF;
  uint8_t b = v & 0xFF;
  out565 = tft.color565(r, g, b);
  return true;
}

static String openSpoolOrcaNamePreview() {
  String brand = manufacturerNameByVal(selMfg);
  String type = materialNameByVal(selMatVal);
  String subtype = String(osSubtype);
  brand.trim(); type.trim(); subtype.trim();
  String s = brand;
  if (type.length()) { if (s.length()) s += " "; s += type; }
  if (subtype.length()) { if (s.length()) s += " "; s += subtype; }
  return s;
}

static void syncSelectionFromOpenSpoolColor() {
  String target = normalizeHexColor(osColorHex, true);
  for (size_t i = 0; i < COLORS_COUNT; i++) {
    if (target.equalsIgnoreCase(colorHexFrom565(COLORS[i].rgb565))) {
      selColIdx = i;
      return;
    }
  }
}

// ==================== Color Picker helpers ====================
// Color picker layout constants (320x240 display)
#define CP_SQ_X   18
#define CP_SQ_Y   68
#define CP_SQ_W   172
#define CP_SQ_H   110
#define CP_HUE_X  18
#define CP_HUE_Y  183
#define CP_HUE_W  284
#define CP_HUE_H  16
#define CP_PRV_X  220
#define CP_PRV_Y  68
#define CP_PRV_W  80
#define CP_PRV_H  60

static void hsvToRgb888(uint16_t h, uint8_t s, uint8_t v, uint8_t& r, uint8_t& g, uint8_t& b) {
  // h: 0-359, s: 0-255, v: 0-255
  if (s == 0) { r = g = b = v; return; }
  uint16_t region = h / 60;
  uint32_t remainder = ((uint32_t)(h % 60) * 255) / 60;
  uint8_t p = (uint32_t)v * (255 - s) / 255;
  uint8_t q = (uint32_t)v * (255 - ((uint32_t)s * remainder / 255)) / 255;
  uint8_t t = (uint32_t)v * (255 - ((uint32_t)s * (255 - remainder) / 255)) / 255;
  switch (region) {
    case 0: r = v; g = t; b = p; break;
    case 1: r = q; g = v; b = p; break;
    case 2: r = p; g = v; b = t; break;
    case 3: r = p; g = q; b = v; break;
    case 4: r = t; g = p; b = v; break;
    default: r = v; g = p; b = q; break;
  }
}

static void rgb888ToHsv(uint8_t r, uint8_t g, uint8_t b, uint16_t& h, uint8_t& s, uint8_t& v) {
  uint8_t maxc = max(max(r, g), b);
  uint8_t minc = min(min(r, g), b);
  v = maxc;
  if (maxc == 0) { s = 0; h = 0; return; }
  uint16_t delta = maxc - minc;
  s = (uint32_t)delta * 255 / maxc;
  if (s == 0) { h = 0; return; }
  int32_t hRaw = 0;
  if (maxc == r)      hRaw = (int32_t)60 * (int32_t)(g - b) / (int32_t)delta;
  else if (maxc == g) hRaw = (int32_t)60 * (int32_t)(b - r) / (int32_t)delta + 120;
  else                hRaw = (int32_t)60 * (int32_t)(r - g) / (int32_t)delta + 240;
  if (hRaw < 0) hRaw += 360;
  if (hRaw >= 360) hRaw -= 360;
  h = (uint16_t)hRaw;
}

static void syncCpHex() {
  uint8_t r, g, b;
  hsvToRgb888(cpHue, cpSat, cpVal, r, g, b);
  snprintf(cpHex, sizeof(cpHex), "#%02X%02X%02X", r, g, b);
  cpHexExactLocked = false;
}

static void initColorPickerFromHex(const char* hex) {
  String s = normalizeHexColor(hex ? hex : "#FFFFFF", false);
  while (s.length() < 6) s += '0';
  int r = strtol(s.substring(0, 2).c_str(), nullptr, 16);
  int g = strtol(s.substring(2, 4).c_str(), nullptr, 16);
  int b = strtol(s.substring(4, 6).c_str(), nullptr, 16);
  rgb888ToHsv((uint8_t)r, (uint8_t)g, (uint8_t)b, cpHue, cpSat, cpVal);
  snprintf(cpHex, sizeof(cpHex), "#%02X%02X%02X", r, g, b);
  cpHexExactLocked = true;
}

static void applyCpHexEdit() {
  String s = String(cpHexEdit);
  String clean = "";
  for (size_t i = 0; i < s.length(); i++) {
    char c = toupper((unsigned char)s[i]);
    bool ok = ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F'));
    if (ok) clean += c;
    if (clean.length() >= 6) break;
  }
  if (clean.length() == 6) {
    String hx = "#" + clean;
    initColorPickerFromHex(hx.c_str());
    safeCopy(cpHex, hx.c_str(), sizeof(cpHex));
    cpHexExactLocked = true;
  }
  cpHexEditActive = false;
}

static void drawCpHueBar() {
  tft.fillRect(CP_HUE_X - 2, CP_HUE_Y - 3, CP_HUE_W + 4, CP_HUE_H + 6, TFT_BLACK);

  // Draw a true pixel-accurate hue gradient so the touch position matches
  // the visual color along the full bar width.
  for (int dx = 0; dx < CP_HUE_W; dx++) {
    uint16_t h = (uint16_t)((dx * 359UL) / max(1, CP_HUE_W - 1));
    uint8_t r, g, b;
    hsvToRgb888(h, 255, 255, r, g, b);
    tft.drawFastVLine(CP_HUE_X + dx, CP_HUE_Y, CP_HUE_H, tft.color565(r, g, b));
  }
  tft.drawRect(CP_HUE_X, CP_HUE_Y, CP_HUE_W, CP_HUE_H, TFT_WHITE);

  int hx = CP_HUE_X + (int)((cpHue * (uint32_t)(CP_HUE_W - 1)) / 359UL);
  hx = constrain(hx, CP_HUE_X + 1, CP_HUE_X + CP_HUE_W - 2);
  tft.drawFastVLine(hx,     CP_HUE_Y - 3, CP_HUE_H + 6, TFT_WHITE);
  tft.drawFastVLine(hx - 1, CP_HUE_Y - 3, CP_HUE_H + 6, TFT_BLACK);
  tft.drawFastVLine(hx + 1, CP_HUE_Y - 3, CP_HUE_H + 6, TFT_BLACK);
}

static void drawCpSquare() {
  // Draw HSV square: X=saturation(0→255), Y=value(255→0), 4x4 blocks
  const int bW = 4, bH = 4;
  for (int by = 0; by * bH < CP_SQ_H; by++) {
    uint8_t val = 255 - (uint8_t)constrain((int)((by * bH + bH / 2) * 256 / CP_SQ_H), 0, 255);
    for (int bx = 0; bx * bW < CP_SQ_W; bx++) {
      uint8_t sat = (uint8_t)constrain((int)((bx * bW + bW / 2) * 256 / CP_SQ_W), 0, 255);
      uint8_t r, g, b;
      hsvToRgb888(cpHue, sat, val, r, g, b);
      tft.fillRect(CP_SQ_X + bx * bW, CP_SQ_Y + by * bH, bW, bH, tft.color565(r, g, b));
    }
  }
  tft.drawRect(CP_SQ_X, CP_SQ_Y, CP_SQ_W, CP_SQ_H, TFT_WHITE);
  // Draw cursor circle at current sat/val position
  int cx = CP_SQ_X + (int)((float)cpSat / 255.0f * (CP_SQ_W - 1));
  int cy = CP_SQ_Y + (int)((1.0f - (float)cpVal / 255.0f) * (CP_SQ_H - 1));
  cx = constrain(cx, CP_SQ_X + 4, CP_SQ_X + CP_SQ_W - 5);
  cy = constrain(cy, CP_SQ_Y + 4, CP_SQ_Y + CP_SQ_H - 5);
  tft.drawCircle(cx, cy, 6, TFT_BLACK);
  tft.drawCircle(cx, cy, 5, TFT_WHITE);
}

static void drawCpPreview() {
  uint8_t r, g, b;
  hsvToRgb888(cpHue, cpSat, cpVal, r, g, b);
  uint16_t previewColor = tft.color565(r, g, b);
  tft.fillRect(CP_PRV_X, CP_PRV_Y, CP_PRV_W, CP_PRV_H, previewColor);
  tft.drawRect(CP_PRV_X, CP_PRV_Y, CP_PRV_W, CP_PRV_H, TFT_WHITE);
  // Editable hex field below preview
  const int hexY = CP_PRV_Y + CP_PRV_H + 4;
  tft.fillRect(CP_PRV_X, hexY, CP_PRV_W, 18, TFT_BLACK);
  tft.drawRect(CP_PRV_X, hexY, CP_PRV_W, 18, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(cpHex, CP_PRV_X + CP_PRV_W / 2, hexY + 9, 2);
  tft.setTextDatum(TL_DATUM);
}

static void drawColorPickerScreen() {
  tft.fillScreen(TFT_BLACK);
  drawHeader(LTXT(TXT_CP_TITLE));
  // Nav buttons
  fillButton(CP_SQ_X, 35, 80, 28, TFT_DARKGREY, TFT_WHITE, TR(STR_BACK), TFT_WHITE, 2);
  fillButton(220, 35, 80, 28, TFT_DARKGREEN, TFT_WHITE, LTXT(TXT_OK), TFT_WHITE, 2);
  // Draw HSV square
  drawCpSquare();
  // Draw hue bar
  drawCpHueBar();
  // Draw preview + hex
  drawCpPreview();
  drawStatusBarFrame();
  drawStatus(LTXT(TXT_CP_TITLE), TFT_WHITE);
}

static bool strToIntIfPresent(const char* s, int& out) {
  String t = String(s ? s : "");
  t.trim();
  if (!t.length()) return false;
  out = t.toInt();
  return true;
}

static bool strToFloatIfPresent(const char* s, float& out) {
  String t = String(s ? s : "");
  t.trim();
  if (!t.length()) return false;
  out = t.toFloat();
  return true;
}

static bool openSpoolReadTagPresent(String& brand, String& material, String& subtype, String& colorHex) {
  delay(10);
  // Clear previously read OpenSpool values first, so missing fields on the next tag
  // do not keep stale values from an earlier tag.
  brand = "-";
  material = "-";
  subtype = "";
  colorHex = "-";
  openSpoolReadBrand = "-";
  openSpoolReadMaterial = "-";
  osSubtype[0] = '\0';
  osAlpha[0] = '\0';
  osMinTemp[0] = '\0';
  osMaxTemp[0] = '\0';
  osBedMinTemp[0] = '\0';
  osBedMaxTemp[0] = '\0';
  osWeight[0] = '\0';
  osDiameter[0] = '\0';
  osColorHex[0] = '\0';
  osAddColor1[0] = '\0';
  osAddColor2[0] = '\0';
  osAddColor3[0] = '\0';
  osAddColor4[0] = '\0';

  static uint8_t buf[NTAG_USER_BYTES];
  if (!readOpenSpoolUserArea(buf)) return false;
  size_t ndefOffset = 0, ndefLen = 0;
  if (!findOpenSpoolNdefTlv(buf, sizeof(buf), ndefOffset, ndefLen)) return false;
  String mime, payload;
  if (!parseOpenSpoolMimeRecord(buf + ndefOffset, ndefLen, mime, payload)) return false;
  if (mime != "application/json") return false;
  JsonDocument doc;
  if (deserializeJson(doc, payload)) return false;
  if (!doc["protocol"].is<String>()) return false;
  if (String((const char*)doc["protocol"]) != "openspool") return false;
  brand = doc["brand"].is<String>() ? String((const char*)doc["brand"]) : String("-");
  material = doc["type"].is<String>() ? String((const char*)doc["type"]) : String("-");
  subtype = doc["subtype"].is<String>() ? String((const char*)doc["subtype"]) : String("");
  colorHex = doc["color_hex"].is<String>() ? String((const char*)doc["color_hex"]) : String("-");
  openSpoolReadBrand = brand;
  openSpoolReadMaterial = material;
  if (doc["subtype"].is<String>()) safeCopy(osSubtype, ((const char*)doc["subtype"]), sizeof(osSubtype));
  if (doc["alpha"].is<String>()) safeCopy(osAlpha, ((const char*)doc["alpha"]), sizeof(osAlpha));
  if (doc["min_temp"].is<int>()) snprintf(osMinTemp, sizeof(osMinTemp), "%d", (int)doc["min_temp"]);
  else if (doc["min_temp"].is<String>()) safeCopy(osMinTemp, ((const char*)doc["min_temp"]), sizeof(osMinTemp));
  if (doc["max_temp"].is<int>()) snprintf(osMaxTemp, sizeof(osMaxTemp), "%d", (int)doc["max_temp"]);
  else if (doc["max_temp"].is<String>()) safeCopy(osMaxTemp, ((const char*)doc["max_temp"]), sizeof(osMaxTemp));
  safeCopy(osColorHex, normalizeHexColor(colorHex.c_str(), true).c_str(), sizeof(osColorHex));
  if (doc["bed_min_temp"].is<int>()) snprintf(osBedMinTemp, sizeof(osBedMinTemp), "%d", (int)doc["bed_min_temp"]);
  else if (doc["bed_min_temp"].is<String>()) safeCopy(osBedMinTemp, ((const char*)doc["bed_min_temp"]), sizeof(osBedMinTemp));
  if (doc["bed_max_temp"].is<int>()) snprintf(osBedMaxTemp, sizeof(osBedMaxTemp), "%d", (int)doc["bed_max_temp"]);
  else if (doc["bed_max_temp"].is<String>()) safeCopy(osBedMaxTemp, ((const char*)doc["bed_max_temp"]), sizeof(osBedMaxTemp));
  if (doc["weight"].is<int>()) snprintf(osWeight, sizeof(osWeight), "%d", (int)doc["weight"]);
  else if (doc["weight"].is<String>()) safeCopy(osWeight, ((const char*)doc["weight"]), sizeof(osWeight));
  if (doc["diameter"].is<float>() || doc["diameter"].is<double>()) snprintf(osDiameter, sizeof(osDiameter), "%.2f", (double)doc["diameter"]);
  else if (doc["diameter"].is<String>()) safeCopy(osDiameter, ((const char*)doc["diameter"]), sizeof(osDiameter));
  if (doc["additional_color_hexes"].is<JsonArrayConst>()) {
    const char* vals[4] = {"","","",""};
    int idx = 0;
    for (JsonVariantConst v : doc["additional_color_hexes"].as<JsonArrayConst>()) {
      if (idx >= 4) break;
      if (v.is<String>()) vals[idx++] = v.as<const char*>();
    }
    safeCopy(osAddColor1, normalizeHexColor(vals[0], true).c_str(), sizeof(osAddColor1));
    safeCopy(osAddColor2, normalizeHexColor(vals[1], true).c_str(), sizeof(osAddColor2));
    safeCopy(osAddColor3, normalizeHexColor(vals[2], true).c_str(), sizeof(osAddColor3));
    safeCopy(osAddColor4, normalizeHexColor(vals[3], true).c_str(), sizeof(osAddColor4));
  }
  syncSelectionFromOpenSpoolColor();
  return true;
}

static bool openSpoolReadTag(String& brand, String& material, String& subtype, String& colorHex) {
  uint8_t uid[10] = {0};
  uint8_t uidLen = 0;
  if (!detectIsoA(uid, uidLen, 120)) return false;
  return openSpoolReadTagPresent(brand, material, subtype, colorHex);
}

static uint8_t getOpenSpoolDisplayPageCount() {
  if (!openSpoolProfileU1) return osInfoStdNozzleEnabled ? 3 : 2;
  uint8_t count = 3;
  if (osInfoU1AlphaEnabled) count++;
  if (osInfoU1WeightEnabled || osInfoU1DiameterEnabled) count++;
  if (osInfoU1AddColorsEnabled) count++;
  return count;
}

static uint8_t getOpenSpoolWritePageCount() {
  return (uint8_t)(getOpenSpoolDisplayPageCount() + 1);
}

static OpenSpoolWritePageKind getOpenSpoolWritePageKind(uint8_t writePage) {
  if (writePage == 0) return OS_PAGE_SELECT;

  uint8_t page = 1;
  if (writePage == page) return OS_PAGE_BASE;
  page++;

  if (!openSpoolProfileU1) {
    if (osInfoStdNozzleEnabled && writePage == page) return OS_PAGE_STD_NOZZLE;
    if (osInfoStdNozzleEnabled) page++;
    if (writePage == page) return OS_PAGE_SLICER;
    return OS_PAGE_BASE;
  }

  if (writePage == page) return OS_PAGE_U1_CORE;
  page++;

  if (osInfoU1AlphaEnabled) {
    if (writePage == page) return OS_PAGE_U1_ALPHA;
    page++;
  }

  if (osInfoU1WeightEnabled || osInfoU1DiameterEnabled) {
    if (writePage == page) return OS_PAGE_U1_WEIGHT;
    page++;
  }
  if (osInfoU1AddColorsEnabled) {
    if (writePage == page) return OS_PAGE_U1_EXTRA;
    page++;
  }
  if (writePage == page) return OS_PAGE_SLICER;

  return OS_PAGE_BASE;
}

static void buildOpenSpoolDoc(JsonDocument& doc, OpenSpoolSaveTier tier) {
  doc.clear();
  doc["protocol"] = "openspool";
  doc["version"] = "1.0";
  doc["type"] = materialNameByVal(selMatVal);
  doc["color_hex"] = normalizeHexColor(osColorHex, true);

  String brand = manufacturerNameByVal(selMfg); brand.trim();
  if (brand.length()) doc["brand"] = brand;

  if ((openSpoolProfileU1 || osInfoStdNozzleEnabled) && tier >= OS_TIER_STANDARD) {
    int iv = 0;
    if (strToIntIfPresent(osMinTemp, iv)) doc["min_temp"] = String(iv);
    if (strToIntIfPresent(osMaxTemp, iv)) doc["max_temp"] = String(iv);
  }

  if (openSpoolProfileU1 && tier >= OS_TIER_STANDARD) {
    int iv = 0;
    String subtype = String(osSubtype); subtype.trim();
    if (subtype.length()) doc["subtype"] = subtype;
    if (osInfoU1AlphaEnabled) {
      String alpha = normalizeAlphaHex(osAlpha);
      if (!alpha.length()) alpha = "FF";
      doc["alpha"] = alpha;
    }
    if (osInfoU1BedEnabled) {
      if (strToIntIfPresent(osBedMinTemp, iv)) doc["bed_min_temp"] = String(iv);
      if (strToIntIfPresent(osBedMaxTemp, iv)) doc["bed_max_temp"] = String(iv);
    }
  }

  if (openSpoolProfileU1 && tier >= OS_TIER_ALL) {
    int iv = 0; float fv = 0.0f;
    if (osInfoU1WeightEnabled && strToIntIfPresent(osWeight, iv)) doc["weight"] = String(iv);
    if (osInfoU1DiameterEnabled && strToFloatIfPresent(osDiameter, fv)) {
      String ds = String(fv, 2);
      while (ds.endsWith("0")) ds.remove(ds.length() - 1);
      if (ds.endsWith(".")) ds.remove(ds.length() - 1);
      doc["diameter"] = ds;
    }

    String c1 = normalizeHexColor(osAddColor1, true); if (!String(osAddColor1).length()) c1 = "";
    String c2 = normalizeHexColor(osAddColor2, true); if (!String(osAddColor2).length()) c2 = "";
    String c3 = normalizeHexColor(osAddColor3, true); if (!String(osAddColor3).length()) c3 = "";
    String c4 = normalizeHexColor(osAddColor4, true); if (!String(osAddColor4).length()) c4 = "";
    if (osInfoU1AddColorsEnabled && (c1.length() || c2.length() || c3.length() || c4.length())) {
      JsonArray arr = doc["additional_color_hexes"].to<JsonArray>();
      if (c1.length()) arr.add(c1);
      if (c2.length()) arr.add(c2);
      if (c3.length()) arr.add(c3);
      if (c4.length()) arr.add(c4);
    }
  }
}

static bool buildOpenSpoolTierPayload(OpenSpoolSaveTier tier, String& json, size_t& ndefLen) {
  JsonDocument doc;
  buildOpenSpoolDoc(doc, tier);
  json = "";
  serializeJson(doc, json);
  uint8_t tmp[NTAG_USER_BYTES];
  return buildOpenSpoolNdefFromJson(json, tmp, sizeof(tmp), ndefLen);
}

static OpenSpoolSaveTier selectBestOpenSpoolTier(String& jsonOut, size_t& ndefLenOut) {
  if (!openSpoolProfileU1) {
    if (osInfoStdNozzleEnabled && buildOpenSpoolTierPayload(OS_TIER_STANDARD, jsonOut, ndefLenOut)) return OS_TIER_STANDARD;
    if (buildOpenSpoolTierPayload(OS_TIER_BASIC, jsonOut, ndefLenOut)) return OS_TIER_BASIC;
    return OS_TIER_NONE;
  }
  if (buildOpenSpoolTierPayload(OS_TIER_ALL, jsonOut, ndefLenOut)) return OS_TIER_ALL;
  if (buildOpenSpoolTierPayload(OS_TIER_STANDARD, jsonOut, ndefLenOut)) return OS_TIER_STANDARD;
  if (buildOpenSpoolTierPayload(OS_TIER_BASIC, jsonOut, ndefLenOut)) return OS_TIER_BASIC;
  return OS_TIER_NONE;
}

static bool openSpoolWriteTagPresent(OpenSpoolSaveTier& savedTier) {
  delay(10);
  if (!ensureOpenSpoolCC()) return false;
  delay(10);

  String json;
  size_t ndefLen = 0;
  savedTier = selectBestOpenSpoolTier(json, ndefLen);
  if (savedTier == OS_TIER_NONE) return false;

  uint8_t buf[NTAG_USER_BYTES];
  memset(buf, 0x00, sizeof(buf));
  if (!buildOpenSpoolNdefFromJson(json, buf, sizeof(buf), ndefLen)) return false;
  return writeOpenSpoolUserArea(buf, ndefLen);
}

static bool openSpoolWriteTag() {
  uint8_t uid[10] = {0};
  uint8_t uidLen = 0;
  OpenSpoolSaveTier savedTier = OS_TIER_NONE;
  if (!detectIsoA(uid, uidLen, 120)) return false;
  return openSpoolWriteTagPresent(savedTier);
}

static void showSimpleMessage(const String& title, const String& l1, const String& l2, const String& l3, const String& l4, int nextState) {
  messageTitle = title;
  messageLine1 = l1;
  messageLine2 = l2;
  messageLine3 = l3;
  messageLine4 = l4;
  messageOkNextState = (UIState)nextState;
  ui = UI_MESSAGE_OK;
  needRedraw = true;
}

// ==================== Notice ====================
static void showNotice(NoticeKind kind, UIState nextState) {
  if (currentTagMode != TAGMODE_QIDI) {
    ui = nextState;
    needRedraw = true;
    return;
  }
  messageTitle = LTXT(TXT_NOTICE);
  messageLine1 = (kind == NOTICE_MATERIAL) ? LTXT(TXT_MAT_NOTICE1) : LTXT(TXT_MFG_NOTICE1);
  messageLine2 = LTXT(TXT_NOTICE2);
  messageLine3 = LTXT(TXT_NOTICE3);
  messageLine4 = LTXT(TXT_NOTICE4);
  messageOkNextState = (UIState)nextState;
  ui = UI_MESSAGE_OK;
  needRedraw = true;
}

// ==================== Tag info popup ====================
static void drawTagInfoPopup(uint8_t matID, uint8_t colID, uint8_t mfgID) {
  int cIdx = findColorById(colID);

  int x = 8;
  int y = 42;
  int w = TFT_W - 16;
  int h = TFT_H - 42 - UI_STATUS_H - 6;

  tft.fillRoundRect(x, y, w, h, 8, TFT_NAVY);
  tft.drawRoundRect(x, y, w, h, 8, TFT_WHITE);

  tft.setTextColor(TFT_WHITE, TFT_NAVY);
  String title = String(TR(STR_TAG_INFO_TITLE));
  if (currentTagMode == TAGMODE_QIDI) title += " " + currentQidiModeText();
  tft.drawCentreString(title, TFT_W / 2, y + 6, 2);

  String mfg = manufacturerNameByVal(mfgID);
  if (!mfg.length()) mfg = String("-");
  String mat = materialNameByVal(matID);
  if (!mat.length()) mat = String("-");

  int cy = y + 42;

  tft.setTextColor(TFT_WHITE, TFT_NAVY);
  tft.drawCentreString(mfg, TFT_W / 2, cy, 4);
  cy += 34;

  tft.setTextColor(TFT_WHITE, TFT_NAVY);
  tft.drawCentreString(mat, TFT_W / 2, cy, 4);
  cy += 34;

  int boxW = 140;
  int boxH = 32;
  int boxX = TFT_W / 2 - boxW / 2;
  uint16_t fill = (cIdx >= 0) ? COLORS[cIdx].rgb565 : TFT_DARKGREY;
  uint16_t textCol = colorTextForBg(fill);

  tft.fillRoundRect(boxX, cy, boxW, boxH, 6, fill);
  tft.drawRoundRect(boxX, cy, boxW, boxH, 6, TFT_WHITE);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(textCol, fill);
  tft.drawString((cIdx >= 0) ? TR(COLORS[cIdx].labelId) : String(TR(STR_LABEL_COLOR)), boxX + boxW / 2, cy + boxH / 2, 2);
  tft.setTextDatum(TL_DATUM);
}

// ==================== Tag operations ====================
static void performReadQidi() {
  drawStatus(TR(STR_WAIT_TAG), TFT_YELLOW);

  NfcLock lock(2500);
  if (!lock.locked) {
    drawStatus(TR(STR_NFC_BUSY), TFT_RED);
    delay(1200);
    needRedraw = true;
    return;
  }

  uint8_t uid[10] = {0};
  uint8_t uidLen = 0;

  if (!waitForTagUID(uid, uidLen, 3000)) {
    drawStatus(TR(STR_NO_TAG), TFT_RED);
    delay(1200);
    needRedraw = true;
    return;
  }

  if (uidLen != 4) {
    drawStatus(TR(STR_NO_TAG), TFT_RED);
    delay(1200);
    needRedraw = true;
    return;
  }

  if (!authBlockWithDefaultKeyA(uid, uidLen, DATA_BLOCK)) {
    drawStatus(TR(STR_AUTH_FAILED), TFT_RED);
    delay(1200);
    needRedraw = true;
    return;
  }

  uint8_t data[16] = {0};
  if (!readBlock(DATA_BLOCK, data)) {
    drawStatus(TR(STR_READ_FAILED), TFT_RED);
    delay(1200);
    needRedraw = true;
    return;
  }

  drawTagInfoPopup(data[0], data[1], data[2]);
  noteUserActivity();
  drawStatus(TR(STR_READ_TAG_DETECTED), TFT_GREEN);

  readPopupVisible = true;
  readPopupMisses = 0;
  readLastSeen = millis();
  readResultPending = true;
}

static void performWriteQidi() {
  ensureSelectedMaterialValid();
  drawStatus(TR(STR_WAIT_TAG), TFT_YELLOW);

  NfcLock lock(2500);
  if (!lock.locked) {
    drawStatus(TR(STR_NFC_BUSY), TFT_RED);
    delay(1200);
    needRedraw = true;
    return;
  }

  uint8_t uid[10] = {0};
  uint8_t uidLen = 0;

  if (!waitForTagUID(uid, uidLen, 3000)) {
    drawStatus(TR(STR_NO_TAG), TFT_RED);
    delay(1200);
    needRedraw = true;
    return;
  }

  if (uidLen != 4) {
    drawStatus(TR(STR_NO_TAG), TFT_RED);
    delay(1200);
    needRedraw = true;
    return;
  }

  if (!authBlockWithDefaultKeyA(uid, uidLen, DATA_BLOCK)) {
    drawStatus(TR(STR_AUTH_FAILED), TFT_RED);
    delay(1200);
    needRedraw = true;
    return;
  }

  uint8_t data[16] = {0};
  data[0] = selMatVal;
  data[1] = COLORS[selColIdx].id;
  data[2] = selMfg;

  if (!writeBlock(DATA_BLOCK, data)) {
    drawStatus(TR(STR_WRITE_FAILED), TFT_RED);
    delay(1200);
    needRedraw = true;
    return;
  }

  noteUserActivity();
  drawStatus(TR(STR_WRITE_OK), TFT_GREEN);
  delay(1200);
  needRedraw = true;
}

static void performRead() {
  if (currentTagMode == TAGMODE_QIDI) {
    performReadQidi();
    return;
  }

  drawStatus(TR(STR_WAIT_TAG), TFT_YELLOW);

  NfcLock lock(2500);
  if (!lock.locked) {
    drawStatus(TR(STR_NFC_BUSY), TFT_RED);
    delay(1200);
    needRedraw = true;
    return;
  }

  uint8_t uid[10] = {0};
  uint8_t uidLen = 0;
  if (!waitForTagUID(uid, uidLen, 3000)) {
    drawStatus(TR(STR_NO_TAG), TFT_RED);
    delay(1200);
    needRedraw = true;
    return;
  }

  drawStatus(TXT_READING_TAG[uiLang], TFT_YELLOW);
  delay(60);

  String brand, material, subtype, colorHex;
  if (!openSpoolReadTagPresent(brand, material, subtype, colorHex)) {
    drawStatus(TR(STR_READ_FAILED), TFT_RED);
    delay(1200);
    needRedraw = true;
    return;
  }

  noteUserActivity();
  drawStatus(TR(STR_READ_TAG_DETECTED), TFT_GREEN);
  openSpoolReadPage = 0;
  openSpoolReadPageLastSwitchMs = millis();
  openSpoolPopupHoldUntil = millis() + 1800;
  readOpenSpoolDetailsVisible = true;
  readPopupVisible = true;
  readPopupMisses = 0;
  readLastSeen = millis();
  readResultPending = true;
  ui = UI_READ;
  needRedraw = true;
}

static void performWrite() {
  if (currentTagMode == TAGMODE_QIDI) {
    performWriteQidi();
    return;
  }

  if (!osDraftsInitialized) initOpenSpoolDrafts();
  saveCurrentOpenSpoolDraft();

  drawStatus(TR(STR_WAIT_TAG), TFT_YELLOW);

  NfcLock lock(2500);
  if (!lock.locked) {
    drawStatus(TR(STR_NFC_BUSY), TFT_RED);
    delay(1200);
    needRedraw = true;
    return;
  }

  uint8_t uid[10] = {0};
  uint8_t uidLen = 0;
  if (!waitForTagUID(uid, uidLen, 3000)) {
    drawStatus(TR(STR_NO_TAG), TFT_RED);
    delay(1200);
    needRedraw = true;
    return;
  }

  drawStatus(TXT_WRITING_TAG[uiLang], TFT_YELLOW);
  delay(60);

  OpenSpoolSaveTier savedTier = OS_TIER_NONE;
  if (!openSpoolWriteTagPresent(savedTier)) {
    String probeJson; size_t probeLen = 0;
    OpenSpoolSaveTier best = selectBestOpenSpoolTier(probeJson, probeLen);
    if (best == OS_TIER_NONE) {
      showSimpleMessage(LTXT(TXT_MEM_WARN_TITLE), LTXT(TXT_NO_SPACE1), LTXT(TXT_NO_SPACE2), "", "", UI_WRITE);
      return;
    }
    drawStatus(TR(STR_WRITE_FAILED), TFT_RED);
    delay(1200);
    needRedraw = true;
    return;
  }

  if (openSpoolProfileU1) {
    if (savedTier == OS_TIER_STANDARD) {
      showSimpleMessage(LTXT(TXT_MEM_WARN_TITLE), LTXT(TXT_MEM_WARN_STD1), LTXT(TXT_MEM_WARN_STD2), LTXT(TXT_MEM_WARN_STD3), TR(STR_WRITE_OK), UI_WRITE);
      return;
    }
    if (savedTier == OS_TIER_BASIC) {
      showSimpleMessage(LTXT(TXT_MEM_WARN_TITLE), LTXT(TXT_MEM_WARN_BASIC1), LTXT(TXT_MEM_WARN_BASIC2), TR(STR_WRITE_OK), "", UI_WRITE);
      return;
    }
  }

  noteUserActivity();
  drawStatus(TR(STR_WRITE_OK), TFT_GREEN);
  delay(1200);
  needRedraw = true;
}

// ==================== Auto-detect ====================
static void clearAutoTagPanel() {
  needRedraw = true;
}

static void drawAutoTagPanel(uint8_t matID, uint8_t colID, uint8_t mfgID) {
  drawTagInfoPopup(matID, colID, mfgID);
}

static String joinIfAny(const String& a, const String& b, const String& sep = " / ") {
  if (a.length() && b.length()) return a + sep + b;
  return a.length() ? a : b;
}

static void drawColorInfoBox(int x, int y, int w, int h, const String& hexValue) {
  String hx = normalizeHexColor(hexValue.c_str(), true);
  uint16_t bg = TFT_DARKGREY;
  uint16_t fg = TFT_WHITE;
  if (hexValue.length()) {
    parseColor565FromHex(hx.c_str(), bg);
    fg = colorTextForBg(bg);
  }
  tft.fillRoundRect(x, y, w, h, 6, bg);
  tft.drawRoundRect(x, y, w, h, 6, TFT_WHITE);
  tft.setTextColor(fg, bg);
  tft.drawCentreString(hexValue.length() ? hx : String("-"), x + w / 2, y + 7, 2);
}

static void drawOsInfoRow(int y, const char* label, const String& value) {
  tft.setTextColor(TFT_YELLOW, TFT_NAVY);
  tft.drawString(label, 18, y, 2);
  tft.setTextColor(TFT_WHITE, TFT_NAVY);
  tft.drawRightString(value.length() ? value : String("-"), TFT_W - 18, y, 2);
}

static String openSpoolNozzleRangeText() {
  String nozMin = String(osMinTemp).length() ? String(osMinTemp) : String("");
  String nozMax = String(osMaxTemp).length() ? String(osMaxTemp) : String("");
  if (nozMin.length() && nozMax.length()) return nozMin + String("C - ") + nozMax + String("C");
  if (nozMin.length()) return nozMin + String("C");
  if (nozMax.length()) return nozMax + String("C");
  return String("");
}

static String openSpoolBedRangeText() {
  String bedMin = String(osBedMinTemp).length() ? String(osBedMinTemp) : String("");
  String bedMax = String(osBedMaxTemp).length() ? String(osBedMaxTemp) : String("");
  if (bedMin.length() && bedMax.length()) return bedMin + String("C - ") + bedMax + String("C");
  if (bedMin.length()) return bedMin + String("C");
  if (bedMax.length()) return bedMax + String("C");
  return String("");
}

static uint8_t getOpenSpoolReadPageCount() {
  uint8_t count = 1;
  if (openSpoolNozzleRangeText().length() || openSpoolBedRangeText().length() || String(osAlpha).length()) count++;
  if (String(osWeight).length() || String(osDiameter).length()) count++;
  if (String(osAddColor1).length() || String(osAddColor2).length() || String(osAddColor3).length() || String(osAddColor4).length()) count++;
  if (openSpoolOrcaNamePreview().length()) count++;
  return count;
}

static void drawOpenSpoolPageDots(int x, int y, int w, uint8_t pageCount, uint8_t currentPage) {
  if (pageCount <= 1) return;
  const int dot = 8;
  const int gap = 10;
  int totalW = pageCount * dot + (pageCount - 1) * gap;
  int startX = x + (w - totalW) / 2;
  for (uint8_t i = 0; i < pageCount; i++) {
    int dx = startX + i * (dot + gap);
    uint16_t fill = (i == currentPage) ? TFT_WHITE : TFT_DARKGREY;
    tft.fillRoundRect(dx, y, dot, dot, 4, fill);
    tft.drawRoundRect(dx, y, dot, dot, 4, TFT_WHITE);
  }
}

static void drawOpenSpoolPopupPage() {
  int x = 8, y = 42, w = TFT_W - 16, h = TFT_H - 42 - UI_STATUS_H - 6;
  uint8_t pageCount = getOpenSpoolReadPageCount();
  if (openSpoolReadPage >= pageCount) openSpoolReadPage = 0;

  tft.fillRoundRect(x, y, w, h, 8, TFT_NAVY);
  tft.drawRoundRect(x, y, w, h, 8, TFT_WHITE);
  tft.setTextColor(TFT_WHITE, TFT_NAVY);
  tft.drawCentreString("OpenSpool Tag", TFT_W / 2, y + 6, 2);
  drawOpenSpoolPageDots(x, y + h - 20, w, pageCount, openSpoolReadPage);

  String mfg = openSpoolReadBrand;
  if (!mfg.length()) mfg = String("-");
  String mat = openSpoolReadMaterial;
  if (!mat.length()) mat = String("-");
  String var = String(osSubtype).length() ? String(osSubtype) : String("");
  String matVar = mat;
  if (var.length()) matVar += " " + var;
  if (openSpoolReadPage == 0) {
    int cy = y + 40;

    tft.setTextColor(TFT_WHITE, TFT_NAVY);
    tft.drawCentreString(mfg, TFT_W / 2, cy, 4);
    cy += 34;

    tft.setTextColor(TFT_WHITE, TFT_NAVY);
    tft.drawCentreString(matVar, TFT_W / 2, cy, 4);
    cy += 26;

    int boxW = 140;
    int boxH = 32;
    int boxX = TFT_W / 2 - boxW / 2;
    drawColorInfoBox(boxX, cy, boxW, boxH, String(osColorHex));
    return;
  }

  uint8_t pageIndex = 1;
  const bool hasTempsPage = openSpoolNozzleRangeText().length() || openSpoolBedRangeText().length() || String(osAlpha).length();
  const bool hasDimPage = String(osWeight).length() || String(osDiameter).length();
  const bool hasAddColorsPage = String(osAddColor1).length() || String(osAddColor2).length() || String(osAddColor3).length() || String(osAddColor4).length();
  const bool hasSlicerPage = openSpoolReadProfileName().length() > 0;

  if (hasTempsPage) {
    if (openSpoolReadPage == pageIndex) {
      int rowY = y + 44;
      String nozzle = openSpoolNozzleRangeText();
      if (nozzle.length()) { drawOsInfoRow(rowY, (String(LTXT(TXT_NOZZLE_MIN)) + " / " + LTXT(TXT_NOZZLE_MAX)).c_str(), nozzle); rowY += 34; }
      String bed = openSpoolBedRangeText();
      if (bed.length()) { drawOsInfoRow(rowY, (String(LTXT(TXT_BED_MIN)) + " / " + LTXT(TXT_BED_MAX)).c_str(), bed); rowY += 34; }
      if (String(osAlpha).length()) {
        uint8_t alphaByte = 0xFF;
        parseAlphaByte(osAlpha, alphaByte);
        drawOsInfoRow(rowY, LTXT(TXT_ALPHA), normalizeAlphaHex(osAlpha) + " (" + String(alphaPercentFromByte(alphaByte)) + "%)");
      }
      return;
    }
    pageIndex++;
  }

  if (hasDimPage) {
    if (openSpoolReadPage == pageIndex) {
      int rowY = y + 60;
      if (String(osWeight).length()) { drawOsInfoRow(rowY, LTXT(TXT_WEIGHT_G), String(osWeight) + " g"); rowY += 38; }
      if (String(osDiameter).length()) { drawOsInfoRow(rowY, LTXT(TXT_DIAMETER), String(osDiameter) + " mm"); }
      return;
    }
    pageIndex++;
  }

  if (hasAddColorsPage && openSpoolReadPage == pageIndex) {
    tft.setTextColor(TFT_YELLOW, TFT_NAVY);
    tft.drawCentreString(LTXT(TXT_ADDC_TITLE), TFT_W / 2, y + 28, 2);
    auto drawAddButton = [&](int bx, int by, int bw, int bh, const char* value) {
      if (!value || !value[0]) return;
      uint16_t fill = TFT_DARKGREY;
      parseColor565FromHex(value, fill);
      fillButton(bx, by, bw, bh, fill, TFT_WHITE, String(value), colorTextForBg(fill), 2);
    };
    const int btnW = 130;
    const int btnH = 32;
    const int leftX = 18;
    const int rightX = 172;
    const int topY = y + 58;
    const int bottomY = y + 102;
    drawAddButton(leftX, topY, btnW, btnH, osAddColor1);
    drawAddButton(rightX, topY, btnW, btnH, osAddColor2);
    drawAddButton(leftX, bottomY, btnW, btnH, osAddColor3);
    drawAddButton(rightX, bottomY, btnW, btnH, osAddColor4);
    return;
  }
  if (hasAddColorsPage) pageIndex++;

  if (hasSlicerPage && openSpoolReadPage == pageIndex) {
    tft.setTextColor(TFT_WHITE, TFT_NAVY);
    tft.drawCentreString(LTXT(TXT_SLICER), TFT_W / 2, y + 34, 4);
    tft.setTextColor(TFT_WHITE, TFT_NAVY);
    tft.drawCentreString(LTXT(TXT_FILAMENT_PROFILE), TFT_W / 2, y + 62, 2);
    fillButton(18, y + 84, 284, 32, TFT_MAROON, TFT_WHITE, openSpoolReadProfileName(), TFT_WHITE, 2);
  }
}

static void drawOpenSpoolAutoPanel() {
  drawOpenSpoolPopupPage();
}

static void tickOpenSpoolReadPaging() {
  uint8_t pageCount = getOpenSpoolReadPageCount();
  if (pageCount <= 1) return;
  if (millis() < openSpoolPopupHoldUntil) return;
  uint32_t intervalMs = (uint32_t)((osReadIntervalSec < 1) ? 1 : osReadIntervalSec) * 1000UL;
  if (millis() - openSpoolReadPageLastSwitchMs < intervalMs) return;
  openSpoolReadPage = (openSpoolReadPage + 1) % pageCount;
  openSpoolReadPageLastSwitchMs = millis();
  needRedraw = true;
}

static void autoDetectTick() {
  if (openSpoolReadCancelUntil) {
    if (millis() < openSpoolReadCancelUntil) {
      if (!openSpoolCancelShown) {
        drawStatus(TXT_READ_CANCELED[uiLang], TFT_RED);
        openSpoolCancelShown = true;
      }
      return;
    }
    openSpoolReadCancelUntil = 0;
    openSpoolCancelShown = false;
    needRedraw = true;
  }

  if (!autoDetectEnabled) {
    if (autoPanelVisible) {
      clearAutoTagPanel();
      autoPanelVisible = false;
      autoLastMat = autoLastCol = autoLastMfg = 0xFF;
      autoLastOsUidLen = 0;
      memset(autoLastOsUid, 0, sizeof(autoLastOsUid));
      openSpoolReadCancelUntil = 0;
      openSpoolCancelShown = false;
      drawMainMenuStatus();
    }
    return;
  }

  if (ui != UI_MAIN) return;

  if (autoPanelVisible) {
    if (currentTagMode == TAGMODE_OPENSPOOL && millis() < openSpoolPopupHoldUntil) {
      return;
    }
    const unsigned long popupTimeout = (currentTagMode == TAGMODE_OPENSPOOL) ? 1200UL : 300UL;
    if (millis() - autoLastSeen > popupTimeout) {
      clearAutoTagPanel();
      autoPanelVisible = false;
      autoLastMat = autoLastCol = autoLastMfg = 0xFF;
      autoLastOsUidLen = 0;
      memset(autoLastOsUid, 0, sizeof(autoLastOsUid));
      return;
    }
  }

  if (millis() - lastAutoCheck < AUTO_CHECK_INTERVAL) return;
  lastAutoCheck = millis();

  NfcLock lock(150);
  if (!lock.locked) return;

  if (currentTagMode == TAGMODE_QIDI) {
    uint8_t uid[10] = {0};
    uint8_t uidLen = 0;
    if (!waitForTagUID(uid, uidLen, 80)) return;
    if (uidLen != 4) return;
    if (!authBlockWithDefaultKeyA(uid, uidLen, DATA_BLOCK)) return;

    uint8_t data[16] = {0};
    if (!readBlock(DATA_BLOCK, data)) return;

    uint8_t matID = data[0];
    uint8_t colID = data[1];
    uint8_t mfgID = data[2];
    autoLastSeen = millis();

    if (!autoPanelVisible || matID != autoLastMat || colID != autoLastCol || mfgID != autoLastMfg) {
      noteUserActivity();
  drawStatus(TXT_TAG_DETECTED[uiLang], TFT_GREEN);
      drawStatus(TXT_READING_TAG[uiLang], TFT_YELLOW);
      drawAutoTagPanel(matID, colID, mfgID);
      autoPanelVisible = true;
      autoLastMat = matID;
      autoLastCol = colID;
      autoLastMfg = mfgID;
      noteUserActivity();
  drawStatus(TR(STR_AUTO_TAG_DETECTED), TFT_GREEN);
    }
    return;
  }

  uint8_t uid[10] = {0};
  uint8_t uidLen = 0;
  if (!waitForTagUID(uid, uidLen, 80)) return;
  if (uidLen != 7) return;
  autoLastSeen = millis();

  bool sameOsTag = autoPanelVisible && (uidLen == autoLastOsUidLen) && (memcmp(uid, autoLastOsUid, uidLen) == 0);
  if (sameOsTag) return;

  noteUserActivity();
  drawStatus(TXT_TAG_DETECTED[uiLang], TFT_GREEN);
  drawStatus(TXT_READING_TAG[uiLang], TFT_YELLOW);

  String brand, material, subtype, colorHex;
  if (!openSpoolReadTagPresent(brand, material, subtype, colorHex)) {
    openSpoolReadCancelUntil = millis() + 2000;
    openSpoolCancelShown = false;
    autoPanelVisible = false;
    autoLastOsUidLen = 0;
    memset(autoLastOsUid, 0, sizeof(autoLastOsUid));
    return;
  }

  autoLastSeen = millis();
  memcpy(autoLastOsUid, uid, uidLen);
  autoLastOsUidLen = uidLen;
  openSpoolReadPage = 0;
  openSpoolReadPageLastSwitchMs = millis();
  drawOpenSpoolAutoPanel();
  autoPanelVisible = true;
  autoLastMat = 0;
  autoLastCol = 0;
  autoLastMfg = 0;
  noteUserActivity();
  drawStatus(TXT_TAG_READ[uiLang], TFT_GREEN);
}

static void readAutoReturnTick() {
  if (ui != UI_READ) return;
  if (!readResultPending) return;
  if (!readPopupVisible) return;
  if (currentTagMode == TAGMODE_OPENSPOOL && readOpenSpoolDetailsVisible && millis() < openSpoolPopupHoldUntil) return;

  if (millis() - lastAutoCheck >= AUTO_CHECK_INTERVAL) {
    lastAutoCheck = millis();
    bool tagPresent = false;
    NfcLock lock(80);
    if (lock.locked) {
      uint8_t uid[10] = {0};
      uint8_t uidLen = 0;
      if (waitForTagUID(uid, uidLen, 30)) {
        tagPresent = true;
        readLastSeen = millis();
        readPopupMisses = 0;
      }
    }
    if (!tagPresent) {
      if (readPopupMisses < 255) readPopupMisses++;
    }
  }

  if (readPopupMisses >= 6 && millis() - readLastSeen > 900) {
    readPopupVisible = false;
    readResultPending = false;
    readOpenSpoolDetailsVisible = false;
    openSpoolPopupHoldUntil = 0;
    readPopupMisses = 0;
    readPopupMisses = 0;
    needRedraw = true;
  }
}

// ==================== UI drawing ====================
static void drawMainScreen() {
  tft.fillScreen(TFT_BLACK);
  drawHeader(TR(STR_MAIN_TITLE));

  fillButton(20, 50, 140, 50, autoDetectEnabled ? TFT_DARKGREEN : TFT_DARKGREY, TFT_WHITE, TR(STR_READ_TAG), TFT_WHITE, 2);
  fillButton(160, 50, 140, 50, TFT_MAROON, TFT_WHITE, TR(STR_WRITE_TAG), TFT_WHITE, 2);
  fillButton(20, 110, 140, 50, TFT_DARKCYAN, TFT_WHITE, TR(STR_SETUP), TFT_WHITE, 2);
  fillButton(160, 110, 140, 50, (currentTagMode == TAGMODE_QIDI) ? TFT_DARKGREEN : TFT_NAVY, TFT_WHITE,
             tagModeLabel(), TFT_WHITE, 2);

  drawStatusBarFrame();
  drawMainMenuStatus();
}

static void drawOpenSpoolReadDetailsPanel() {
  drawOpenSpoolPopupPage();
}

static void drawReadScreen() {
  tft.fillScreen(TFT_BLACK);
  drawHeader(TR(STR_ACTION_READ));
  fillButton(10, 50, 100, 40, TFT_DARKGREY, TFT_WHITE, TR(STR_BACK), TFT_WHITE, 2);
  fillButton(210, 50, 100, 40, TFT_DARKGREEN, TFT_WHITE, TR(STR_READ_TAG), TFT_WHITE, 2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  if (currentTagMode == TAGMODE_OPENSPOOL && readOpenSpoolDetailsVisible) {
    drawOpenSpoolReadDetailsPanel();
    noteUserActivity();
  drawStatus(TXT_TAG_READ[uiLang], TFT_GREEN);
    return;
  }
  tft.drawCentreString(TR(STR_READY_READ), TFT_W / 2, 112, 2);
  String modeLabel = (currentTagMode == TAGMODE_QIDI) ? currentQidiModeText() : String(tagModeLabel());
  tft.drawCentreString(modeLabel, TFT_W / 2, 138, 2);
  drawStatusBarFrame();
  drawStatus(modeLabel.c_str(), TFT_WHITE);
}

static void drawOpenSpoolColorFieldButton(int x, int y, int w, int h, const char* value) {
  String hx = normalizeHexColor(value, true);
  uint16_t fill = TFT_DARKGREY;
  uint16_t txt = TFT_WHITE;
  if (String(value).length()) {
    parseColor565FromHex(hx.c_str(), fill);
    txt = colorTextForBg(fill);
  }
  fillButton(x, y, w, h, fill, TFT_WHITE, String(value).length() ? hx : String("-"), txt, 2);
}

static void drawOpenSpoolAlphaPage(bool fullPage) {
  const int sliderX = 28;
  const int sliderY = 104;
  const int sliderW = 264;
  const int sliderH = 14;
  const int hexY = 172;
  uint8_t alphaByte = 0xFF;
  bool hasAlpha = parseAlphaByte(osAlpha, alphaByte);
  int alphaPercent = alphaPercentFromByte(alphaByte);
  int knobX = sliderX + (int)((alphaByte * (uint32_t)(sliderW - 1)) / 255UL);
  knobX = constrain(knobX, sliderX, sliderX + sliderW - 1);

  if (fullPage) {
    tft.fillRect(0, UI_HEADER_H + 31, TFT_W, TFT_H - (UI_HEADER_H + 31) - UI_STATUS_H, TFT_BLACK);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString(LTXT(TXT_ALPHA), 18, 76, 2);
  tft.drawString(LTXT(TXT_ALPHA_HEX), 18, 156, 2);
  } else {
    tft.fillRect(0, 72, TFT_W, 76, TFT_BLACK);
    tft.fillRect(0, 168, TFT_W, 34, TFT_BLACK);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString(LTXT(TXT_ALPHA), 18, 76, 2);
  tft.drawString(LTXT(TXT_ALPHA_HEX), 18, 156, 2);
  }

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawRightString(String(alphaPercent) + "%", 302, 76, 2);

  tft.fillRect(sliderX - 2, sliderY - 6, sliderW + 4, sliderH + 12, TFT_BLACK);
  tft.drawRect(sliderX, sliderY, sliderW, sliderH, TFT_WHITE);
  tft.fillRect(sliderX + 1, sliderY + 1, sliderW - 2, sliderH - 2, TFT_DARKGREY);
  int fillW = (int)((alphaByte * (uint32_t)(sliderW - 2)) / 255UL);
  if (fillW > 0) tft.fillRect(sliderX + 1, sliderY + 1, fillW, sliderH - 2, TFT_DARKGREEN);
  tft.fillRect(knobX - 3, sliderY - 5, 7, sliderH + 10, TFT_WHITE);
  tft.drawRect(knobX - 3, sliderY - 5, 7, sliderH + 10, TFT_BLACK);

  tft.setTextColor(TFT_DARKGREY, TFT_BLACK);
  tft.drawString("0%", sliderX - 10, sliderY + sliderH + 6, 2);
  tft.drawRightString("100%", sliderX + sliderW + 10, sliderY + sliderH + 6, 2);

  fillButton(18, hexY, 284, 26, TFT_DARKGREY, TFT_WHITE, hasAlpha ? normalizeAlphaHex(osAlpha) : String("-"), TFT_WHITE, 2);
}

static void drawCheckboxRow(int x, int y, int w, int h, const char* label, bool checked) {
  fillButton(x, y, w, h, checked ? TFT_DARKGREEN : TFT_DARKGREY, TFT_WHITE, "", TFT_WHITE, 2);
  int boxSize = h - 14;
  int boxX = x + 12;
  int boxY = y + (h - boxSize) / 2;
  tft.drawRect(boxX, boxY, boxSize, boxSize, TFT_WHITE);
  if (checked) {
    tft.fillRect(boxX + 3, boxY + 3, boxSize - 6, boxSize - 6, TFT_WHITE);
  }
  tft.setTextColor(TFT_WHITE, checked ? TFT_DARKGREEN : TFT_DARKGREY);
  tft.drawString(label, boxX + boxSize + 12, y + 10, 2);
  tft.drawRightString(checked ? LTXT(TXT_ON) : LTXT(TXT_OFF), x + w - 12, y + 10, 2);
}

static void drawCheckboxToggle(int x, int y, int w, int h, bool checked) {
  fillButton(x, y, w, h, checked ? TFT_DARKGREEN : TFT_DARKGREY, TFT_WHITE, "", TFT_WHITE, 2);
  int boxSize = h - 14;
  int boxX = x + 10;
  int boxY = y + (h - boxSize) / 2;
  tft.drawRect(boxX, boxY, boxSize, boxSize, TFT_WHITE);
  if (checked) {
    tft.fillRect(boxX + 3, boxY + 3, boxSize - 6, boxSize - 6, TFT_WHITE);
  }
  tft.setTextColor(TFT_WHITE, checked ? TFT_DARKGREEN : TFT_DARKGREY);
  tft.drawRightString(checked ? LTXT(TXT_ON) : LTXT(TXT_OFF), x + w - 10, y + 10, 2);
}

static String qidiCfgStatusLabel(QidiPrinterModel model) {
  String modelLabel = String(qidiPrinterModelLabel(model));
  bool available = isOfficialListAvailable(model);

  if (uiLang == LANG_DE) {
    if (!available) return modelLabel + " CFG nicht vorhanden";
    return modelLabel + " CFG vorhanden";
  }

  if (!available) return modelLabel + " CFG missing";
  return modelLabel + " CFG present";
}

static uint16_t qidiCfgStatusColor(QidiPrinterModel model) {
  bool available = isOfficialListAvailable(model);
  if (!available) return TFT_MAROON;
  return TFT_DARKGREEN;
}

static void drawQidiCfgSetupRow(int y, QidiPrinterModel model) {
  const int infoX = 20;
  const int infoW = 200;
  const int toggleX = 228;
  const int toggleW = 72;
  const bool enabled = isOfficialListEnabled(model) && isOfficialListAvailable(model);

  fillButton(infoX, y, infoW, 32, qidiCfgStatusColor(model), TFT_WHITE, qidiCfgStatusLabel(model), TFT_WHITE, 2);
  drawCheckboxToggle(toggleX, y, toggleW, 32, enabled);
}

static void showQidiCfgInfo(QidiPrinterModel model) {
  String title = String("QIDI ") + qidiPrinterModelLabel(model);
  String line1 = (uiLang == LANG_DE) ? "Klipper Datei laden:" : "Load file from Klipper:";
  String line2 = "official_filas_list.cfg";
  char uploadUrl[32];
  buildWifiUploadUrl(uploadUrl, sizeof(uploadUrl));
  String line3 = (WiFi.status() == WL_CONNECTED)
                   ? ((uiLang == LANG_DE) ? "Mit Browser hochladen:" : "Upload with browser:")
                   : ((uiLang == LANG_DE) ? "Aktiviere WLAN um mit Browser" : "Enable WiFi to upload");
  String line4 = (WiFi.status() == WL_CONNECTED)
                   ? String(uploadUrl)
                   : ((uiLang == LANG_DE) ? "die Datei hochzuladen." : "the file with browser.");
  showSimpleMessage(title, line1, line2, line3, line4, UI_SETUP);
}

static void drawWriteScreen() {
  ensureSelectedMaterialValid();

  tft.fillScreen(TFT_BLACK);
  drawHeader(TR(STR_ACTION_WRITE));

  const int topY = UI_HEADER_H + 3;
  const int topH = 28;

  if (currentTagMode == TAGMODE_QIDI) {
    fillButton(8, topY, 80, topH, TFT_DARKGREY, TFT_WHITE, TR(STR_BACK), TFT_WHITE, 2);
    fillButton(TFT_W - 8 - 110, topY, 110, topH, TFT_DARKGREEN, TFT_WHITE, TR(STR_WRITE_TAG), TFT_WHITE, 2);
    const int labelX = 12;
    const int btnX = 122;
    const int btnW = TFT_W - btnX - 12;
    const int row1Y = 78;
    const int row2Y = 122;
    const int row3Y = 166;

    tft.setTextColor(TFT_YELLOW);
    tft.drawString(String(TR(STR_LABEL_MANUFACTURER)) + ":", labelX, row1Y + 9, 2);
    tft.drawString(String(TR(STR_LABEL_MATERIAL)) + ":", labelX, row2Y + 9, 2);
    tft.drawString(String(TR(STR_LABEL_COLOR)) + ":", labelX, row3Y + 11, 2);

    fillButton(btnX, row1Y, btnW, 34, TFT_NAVY, TFT_WHITE, trimName18(manufacturerNameByVal(selMfg)), TFT_WHITE, 2);
    fillButton(btnX, row2Y, btnW, 34, TFT_DARKCYAN, TFT_WHITE, trimName18(materialNameByVal(selMatVal)), TFT_WHITE, 2);

    uint16_t cfill = COLORS[selColIdx].rgb565;
    uint16_t ctxt  = colorTextForBg(cfill);
    fillButton(btnX, row3Y, btnW, 38, cfill, TFT_WHITE, TR(COLORS[selColIdx].labelId), ctxt, 2);

    drawStatusBarFrame();
    String modeLabel = currentQidiModeText();
    drawStatus(modeLabel.c_str(), TFT_WHITE);
    return;
  }

  const int navY = UI_HEADER_H + 3;
  const int navH = 28;
  const int backX = 8;
  const int backW = 80;
  const int writeW = 92;
  const int writeX = TFT_W - 8 - writeW;
  const int arrowW = 42;
  const uint8_t osPages = getOpenSpoolWritePageCount();
  const uint8_t osDisplayPages = getOpenSpoolDisplayPageCount();
  if (openSpoolWritePage >= osPages) openSpoolWritePage = 0;
  OpenSpoolWritePageKind pageKind = getOpenSpoolWritePageKind(openSpoolWritePage);

  fillButton(backX, navY, backW, navH, TFT_DARKGREY, TFT_WHITE, TR(STR_BACK), TFT_WHITE, 2);
  if (openSpoolWritePage > 0) {
    fillButton(writeX, navY, writeW, navH, TFT_DARKGREEN, TFT_WHITE, LTXT(TXT_WRITE), TFT_WHITE, 2);
    if (osDisplayPages > 1) {
      const int leftX = 96;
      const int rightX = writeX - arrowW - 8;
      fillButton(leftX, navY, arrowW, navH, TFT_DARKGREY, TFT_WHITE, "<", TFT_WHITE, 2);
      fillButton(rightX, navY, arrowW, navH, TFT_DARKGREY, TFT_WHITE, ">", TFT_WHITE, 2);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.drawCentreString(String(openSpoolWritePage) + "/" + String(osDisplayPages), (leftX + arrowW + rightX) / 2, navY + 6, 2);
    }
  }

  if (openSpoolWritePage == 0) {
    const int btnX = 16;
    const int btnW = TFT_W - 32;
    const int btnH = 38;
    const int gapY = 10;
    int y1 = navY + navH + 14;
    int y2 = y1 + btnH + gapY;
    int y3 = y2 + btnH + gapY;
    fillButton(btnX, y1, btnW, btnH, openSpoolProfileU1 ? TFT_DARKGREY : TFT_DARKGREEN, TFT_WHITE, LTXT(TXT_OS_STANDARD), TFT_WHITE, 2);
    fillButton(btnX, y2, btnW, btnH, openSpoolProfileU1 ? TFT_DARKGREEN : TFT_DARKGREY, TFT_WHITE, LTXT(TXT_OS_U1), TFT_WHITE, 2);
    fillButton(btnX, y3, btnW, btnH, TFT_NAVY, TFT_WHITE, LTXT(TXT_TAG_RESET_VALUES), TFT_WHITE, 2);
  } else if (pageKind == OS_PAGE_BASE) {
    const int btnX = 122;
    const int btnW = TFT_W - btnX - 12;
    const int row1Y = 78;
    const int row2Y = 122;
    const int row3Y = 166;

    tft.setTextColor(TFT_YELLOW);
    tft.drawString(String(LTXT(TXT_BRAND)) + ":", 12, row1Y + 9, 2);
    tft.drawString(String(LTXT(TXT_TYPE)) + ":", 12, row2Y + 9, 2);
    tft.drawString(String(LTXT(TXT_COLOR_HEX)) + ":", 12, row3Y + 11, 2);

    fillButton(btnX, row1Y, btnW, 34, TFT_NAVY, TFT_WHITE, trimName18(manufacturerNameByVal(selMfg)), TFT_WHITE, 2);
    fillButton(btnX, row2Y, btnW, 34, TFT_DARKCYAN, TFT_WHITE, trimName18(materialNameByVal(selMatVal)), TFT_WHITE, 2);

    uint16_t cfill = TFT_DARKGREY;
    parseColor565FromHex(osColorHex, cfill);
    fillButton(btnX, row3Y, btnW, 38, cfill, TFT_WHITE, normalizeHexColor(osColorHex, true), colorTextForBg(cfill), 2);
  } else if (pageKind == OS_PAGE_STD_NOZZLE) {
    tft.fillRect(0, UI_HEADER_H + 31, TFT_W, TFT_H - (UI_HEADER_H + 31) - UI_STATUS_H, TFT_BLACK);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString(LTXT(TXT_NOZZLE_MIN), 18, 96, 2);
    tft.drawString(LTXT(TXT_NOZZLE_MAX), 162, 96, 2);
    fillButton(18, 112, 140, 26, TFT_DARKGREY, TFT_WHITE, String(osMinTemp).length()?String(osMinTemp):String("-"), TFT_WHITE, 2);
    fillButton(162, 112, 140, 26, TFT_DARKGREY, TFT_WHITE, String(osMaxTemp).length()?String(osMaxTemp):String("-"), TFT_WHITE, 2);
  } else if (pageKind == OS_PAGE_U1_CORE) {
    tft.fillRect(0, UI_HEADER_H + 31, TFT_W, TFT_H - (UI_HEADER_H + 31) - UI_STATUS_H, TFT_BLACK);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString(LTXT(TXT_VARIANT), 18, 76, 2);
    fillButton(18, 92, 284, 26, TFT_MAROON, TFT_WHITE, String(osSubtype).length() ? String(osSubtype) : String(LTXT(TXT_NONE)), TFT_WHITE, 2);

    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString(LTXT(TXT_NOZZLE_MIN), 18, 124, 2);
    tft.drawString(LTXT(TXT_NOZZLE_MAX), 162, 124, 2);
    fillButton(18, 140, 140, 26, TFT_DARKGREY, TFT_WHITE, String(osMinTemp).length()?String(osMinTemp):String("-"), TFT_WHITE, 2);
    fillButton(162, 140, 140, 26, TFT_DARKGREY, TFT_WHITE, String(osMaxTemp).length()?String(osMaxTemp):String("-"), TFT_WHITE, 2);

    if (osInfoU1BedEnabled) {
      tft.setTextColor(TFT_YELLOW, TFT_BLACK);
      tft.drawString(LTXT(TXT_BED_MIN), 18, 172, 2);
      tft.drawString(LTXT(TXT_BED_MAX), 162, 172, 2);
      fillButton(18, 188, 140, 26, TFT_DARKGREY, TFT_WHITE, String(osBedMinTemp).length()?String(osBedMinTemp):String("-"), TFT_WHITE, 2);
      fillButton(162, 188, 140, 26, TFT_DARKGREY, TFT_WHITE, String(osBedMaxTemp).length()?String(osBedMaxTemp):String("-"), TFT_WHITE, 2);
    }
  } else if (pageKind == OS_PAGE_U1_ALPHA) {
    drawOpenSpoolAlphaPage(true);
  } else if (pageKind == OS_PAGE_U1_WEIGHT) {
    tft.fillRect(0, UI_HEADER_H + 31, TFT_W, TFT_H - (UI_HEADER_H + 31) - UI_STATUS_H, TFT_BLACK);
    int rowY = 76;
    if (osInfoU1WeightEnabled) {
      tft.setTextColor(TFT_YELLOW, TFT_BLACK);
      tft.drawString(LTXT(TXT_WEIGHT_G), 18, rowY, 2);
      fillButton(18, rowY + 16, 284, 26, TFT_DARKGREY, TFT_WHITE, String(osWeight).length()?String(osWeight):String("-"), TFT_WHITE, 2);
      rowY += 48;
    }
    if (osInfoU1DiameterEnabled) {
      tft.setTextColor(TFT_YELLOW, TFT_BLACK);
      tft.drawString(LTXT(TXT_DIAMETER), 18, rowY, 2);
      String diam = String(osDiameter).length() ? String(osDiameter) : String("1.75");
      fillButton(18, rowY + 16, 284, 26, TFT_NAVY, TFT_WHITE, diam + " mm", TFT_WHITE, 2);
    }
  } else if (pageKind == OS_PAGE_U1_EXTRA) {
    tft.fillRect(0, UI_HEADER_H + 31, TFT_W, TFT_H - (UI_HEADER_H + 31) - UI_STATUS_H, TFT_BLACK);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString(LTXT(TXT_ADD_COLOR1), 18, 76, 2);
    tft.drawString(LTXT(TXT_ADD_COLOR2), 162, 76, 2);
    drawOpenSpoolColorFieldButton(18, 92, 140, 26, osAddColor1);
    drawOpenSpoolColorFieldButton(162, 92, 140, 26, osAddColor2);
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawString(LTXT(TXT_ADD_COLOR3), 18, 124, 2);
    tft.drawString(LTXT(TXT_ADD_COLOR4), 162, 124, 2);
    drawOpenSpoolColorFieldButton(18, 140, 140, 26, osAddColor3);
    drawOpenSpoolColorFieldButton(162, 140, 140, 26, osAddColor4);
  } else {
    tft.fillRect(0, UI_HEADER_H + 31, TFT_W, TFT_H - (UI_HEADER_H + 31) - UI_STATUS_H, TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawCentreString(LTXT(TXT_SLICER), TFT_W / 2, 74, 4);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawCentreString(LTXT(TXT_FILAMENT_PROFILE), TFT_W / 2, 102, 2);
    fillButton(18, 124, 284, 32, TFT_MAROON, TFT_WHITE, openSpoolOrcaNamePreview(), TFT_WHITE, 2);
  }

  drawStatusBarFrame();
  drawStatus("OpenSpool", TFT_WHITE);
}

static void drawSetupScreen() {
  normalizeSetupPage();
  tft.fillScreen(TFT_BLACK);
  drawHeader(String(TR(STR_SETUP)) + " " + tagModeLabel());

  fillButton(8, 40, 88, 28, TFT_DARKGREY, TFT_WHITE, TR(STR_BACK), TFT_WHITE, 2);
  String pageLabel = String(setupPage + 1) + "/" + String(getSetupPageCount());
  fillButton(TFT_W - 64, 40, 56, 28, TFT_NAVY, TFT_WHITE, pageLabel, TFT_WHITE, 2);

  if (setupPage == 0) {
    bool qidiLocked = isOfficialListActiveForCurrentQidiModel();
    if (qidiLocked) {
      fillButton(36, 78, 248, 34, TFT_DARKGREY, TFT_WHITE, LTXT(TXT_OFFICIAL_LOCKED), TFT_WHITE, 2);
      fillButton(36, 122, 115, 34, TFT_DARKGREY, TFT_WHITE, TR(STR_LANGUAGE), TFT_WHITE, 2);
      fillButton(169, 122, 115, 34, defaultTagMode == TAGMODE_QIDI ? TFT_DARKGREEN : TFT_NAVY, TFT_WHITE, defaultTagMode == TAGMODE_QIDI ? "QIDI" : "OpenSpool", TFT_WHITE, 2);
      fillButton(36, 166, 248, 34, TFT_DARKGREEN, TFT_WHITE, String(LTXT(TXT_OFFICIAL_CFG)) + " " + qidiPrinterModelLabel(), TFT_WHITE, 2);
    } else {
      fillButton(36, 78, 115, 34, TFT_NAVY, TFT_WHITE, LTXT(TXT_MANUFACTURER), TFT_WHITE, 2);
      fillButton(169, 78, 115, 34, TFT_DARKCYAN, TFT_WHITE, LTXT(TXT_MATERIAL), TFT_WHITE, 2);
      fillButton(36, 122, 115, 34, TFT_DARKGREY, TFT_WHITE, TR(STR_LANGUAGE), TFT_WHITE, 2);
      fillButton(169, 122, 115, 34, defaultTagMode == TAGMODE_QIDI ? TFT_DARKGREEN : TFT_NAVY, TFT_WHITE, defaultTagMode == TAGMODE_QIDI ? "QIDI" : "OpenSpool", TFT_WHITE, 2);
      if (currentTagMode == TAGMODE_QIDI) {
        fillButton(36, 166, 248, 34, TFT_DARKGREY, TFT_WHITE, String("QIDI ") + qidiPrinterModelLabel(), TFT_WHITE, 2);
      }
    }
  } else if (setupPage == 1) {
    String saverLabel = String(LTXT(TXT_SCREENSAVER)) + " " + screensaverModeLabel();
    String brightLabel = String(LTXT(TXT_BRIGHTNESS)) + " " + brightnessLabel();
    fillButton(36, 92, 248, 34, TFT_NAVY, TFT_WHITE, saverLabel, TFT_WHITE, 2);
    fillButton(36, 144, 248, 34, TFT_NAVY, TFT_WHITE, brightLabel, TFT_WHITE, 2);
  } else if (setupPage == 2 && currentTagMode == TAGMODE_QIDI) {
    if (!sdAvailable) {
      fillButton(20, 92, 280, 34, TFT_MAROON, TFT_WHITE, (uiLang == LANG_DE) ? "Keine MicroSD Karte gefunden" : "No MicroSD card found", TFT_WHITE, 2);
      fillButton(20, 136, 280, 34, TFT_DARKGREY, TFT_WHITE, (uiLang == LANG_DE) ? "Bitte MicroSD einlegen" : "Please insert MicroSD", TFT_WHITE, 2);
      fillButton(20, 180, 280, 34, TFT_DARKGREY, TFT_WHITE, (uiLang == LANG_DE) ? "und Seite erneut oeffnen" : "and reopen this page", TFT_WHITE, 2);
    } else {
      drawQidiCfgSetupRow(72, QIDI_MODEL_Q2);
      drawQidiCfgSetupRow(110, QIDI_MODEL_PLUS4);
      drawQidiCfgSetupRow(148, QIDI_MODEL_MAX4);
      fillButton(20, 188, 280, 24, TFT_MAROON, TFT_WHITE, (uiLang == LANG_DE) ? "SD-Karte formatieren" : "Format SD card", TFT_WHITE, 2);
    }
  } else if (setupPage == 2) {
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawCentreString(LTXT(TXT_SET_TAG_INFO), TFT_W / 2, 82, 2);
    fillButton(36, 112, 248, 34, TFT_NAVY, TFT_WHITE, LTXT(TXT_OS_STANDARD), TFT_WHITE, 2);
    fillButton(36, 156, 248, 34, TFT_NAVY, TFT_WHITE, LTXT(TXT_OS_EXTENDED), TFT_WHITE, 2);
  } else if (setupPage == 3) {
    char wifiMask[sizeof(wifiPassword)];
    char wifiIp[24];
    buildWifiPasswordMask(wifiMask, sizeof(wifiMask));
    buildWifiIpLabel(wifiIp, sizeof(wifiIp));
    String wifiLabel = String(LTXT(TXT_WIFI)) + " " + LTXT(wifiEnabled ? TXT_ON : TXT_OFF);
    fillButton(21, 70, 278, 32, wifiEnabled ? TFT_DARKGREEN : TFT_DARKGREY, TFT_WHITE, wifiLabel, TFT_WHITE, 2);
    fillButton(21, 106, 278, 32, TFT_NAVY, TFT_WHITE, String(LTXT(TXT_WIFI_SSID)) + " " + trimName18(String(strlen(wifiSsid) ? wifiSsid : "-")), TFT_WHITE, 2);
    fillButton(21, 142, 278, 32, TFT_NAVY, TFT_WHITE, String(LTXT(TXT_WIFI_PASSWORD)) + " " + trimName18(String(wifiMask)), TFT_WHITE, 2);
    fillButton(21, 178, 278, 32, WiFi.status() == WL_CONNECTED ? TFT_DARKCYAN : TFT_DARKGREY, TFT_WHITE, String(LTXT(TXT_WIFI_IP)) + " " + trimName18(String(wifiIp)), TFT_WHITE, 2);
  } else {
    String invLabel = String(LTXT(TXT_DISPLAY_INV)) + " " + LTXT(displayInversionEnabled ? TXT_ON : TXT_OFF);
    fillButton(36, 78, 248, 34, displayInversionEnabled ? TFT_DARKGREEN : TFT_DARKGREY, TFT_WHITE, invLabel, TFT_WHITE, 2);
    fillButton(36, 122, 115, 34, TFT_DARKGREY, TFT_WHITE, LTXT(TXT_CALIBRATION), TFT_WHITE, 2);
    fillButton(169, 122, 115, 34, TFT_MAROON, TFT_WHITE, TR(STR_FACTORY_DEFAULTS), TFT_WHITE, 2);
  }

  drawStatusBarFrame();
  drawStatusAppVersion();
}

static void drawOpenSpoolTagInfoConfigScreen() {
  tft.fillScreen(TFT_BLACK);
  drawHeader(TR(STR_TAG_INFO_TITLE));
  fillButton(8, 40, 88, 28, TFT_DARKGREY, TFT_WHITE, TR(STR_BACK), TFT_WHITE, 2);

  const char* modeLabel = osTagInfoConfigU1 ? LTXT(TXT_OS_EXTENDED) : LTXT(TXT_OS_STANDARD);

  if (osTagInfoConfigU1) {
    fillButton(TFT_W - 64, 40, 56, 28, TFT_NAVY, TFT_WHITE, String(osTagInfoConfigPage + 1) + "/2", TFT_WHITE, 2);
    if (osTagInfoConfigPage == 0) {
      drawCheckboxRow(24, 82, 272, 30, LTXT(TXT_SHOW_BED_INFO), osInfoU1BedEnabled);
      drawCheckboxRow(24, 118, 272, 30, LTXT(TXT_SHOW_ALPHA_INFO), osInfoU1AlphaEnabled);
      drawCheckboxRow(24, 154, 272, 30, LTXT(TXT_SHOW_WEIGHT_INFO), osInfoU1WeightEnabled);
    } else {
      drawCheckboxRow(24, 82, 272, 30, LTXT(TXT_SHOW_DIAM_INFO), osInfoU1DiameterEnabled);
      drawCheckboxRow(24, 118, 272, 30, LTXT(TXT_SHOW_ADDC_INFO), osInfoU1AddColorsEnabled);
      fillButton(24, 154, 272, 30, TFT_NAVY, TFT_WHITE, openSpoolReadIntervalLabel(), TFT_WHITE, 2);
    }
  } else {
    drawCheckboxRow(24, 118, 272, 34, LTXT(TXT_SHOW_NOZZLE_INFO), osInfoStdNozzleEnabled);
  }

  drawStatusBarFrame();
  drawStatus(modeLabel, TFT_WHITE);
}

static void drawLangSelectScreen() {
  tft.fillScreen(TFT_BLACK);
  drawHeader(TR(STR_SELECT_LANGUAGE));
  fillButton(8, 35, 80, 28, TFT_DARKGREY, TFT_WHITE, TR(STR_BACK), TFT_WHITE, 2);

  const int cols = 2, rows = 3, side = 12, gapX = 8, gapY = 8;
  const int top = 70, bottom = TFT_H - UI_STATUS_H - 10;
  const int availH = bottom - top;
  int btnH = (availH - (rows - 1) * gapY) / rows;
  if (btnH > 34) btnH = 34;
  if (btnH < 22) btnH = 22;
  const int totalH = rows * btnH + (rows - 1) * gapY;
  const int y0 = top + max(0, (availH - totalH) / 2);
  const int btnW = (TFT_W - 2 * side - (cols - 1) * gapX) / cols;

  int idx = 0;
  for (int r = 0; r < rows; r++) {
    for (int c = 0; c < cols; c++) {
      if (idx >= (int)LANG_COUNT) break;
      int bx = side + c * (btnW + gapX);
      int by = y0 + r * (btnH + gapY);
      fillButton(bx, by, btnW, btnH, ((int)uiLang == idx) ? TFT_DARKGREEN : TFT_DARKGREY, TFT_WHITE, LANG_NAMES[idx], TFT_WHITE, 2);
      idx++;
    }
  }

  drawStatusBarFrame();
  drawStatus(TR(STR_SELECT_LANGUAGE), TFT_WHITE);
}

static void drawPickMatScreen() {
  tft.fillScreen(TFT_BLACK);
  drawHeader(TR(STR_SELECT_MATERIAL));
  fillButton(8, 35, 80, 28, TFT_DARKGREY, TFT_WHITE, TR(STR_BACK), TFT_WHITE, 2);
  fillButton(TFT_W - 120, 33, 52, 34, TFT_DARKGREY, TFT_WHITE, "<", TFT_WHITE, 2);
  fillButton(TFT_W - 60, 33, 52, 34, TFT_DARKGREY, TFT_WHITE, ">", TFT_WHITE, 2);

  int total = getActiveMaterialCount();
  int pages = max(1, (total + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE);
  if (matPage >= pages) matPage = pages - 1;
  if (matPage < 0) matPage = 0;

  const int cols = 2, rows = 4, w = 146, h = 32, gapX = 12, gapY = 6, x0 = 8, y0 = 70;
  int startIdx = matPage * ITEMS_PER_PAGE;
  int idx = startIdx;

  for (int r = 0; r < rows && idx < total; r++) {
    for (int c = 0; c < cols && idx < total; c++) {
      uint8_t matVal = getActiveMaterialByIndex(idx);
      fillButton(x0 + c * (w + gapX), y0 + r * (h + gapY), w, h,
                 (matVal == selMatVal) ? TFT_DARKGREEN : TFT_DARKCYAN, TFT_WHITE,
                 trimName18(materialNameByVal(matVal)), TFT_WHITE, 2);
      idx++;
    }
  }

  drawStatusBarFrame();
  drawStatus(TR(STR_SELECT_MATERIAL), TFT_WHITE);
}

static void drawPickColorScreen() {
  tft.fillScreen(TFT_BLACK);
  drawHeader(TR(STR_SELECT_COLOR));
  fillButton(8, 35, 80, 28, TFT_DARKGREY, TFT_WHITE, TR(STR_BACK), TFT_WHITE, 2);
  if (currentTagMode == TAGMODE_OPENSPOOL) {
    fillButton(232, 35, 80, 28, TFT_NAVY, TFT_WHITE, LTXT(TXT_CP_BTN), TFT_WHITE, 2);
  }

  const int cols = 6, rows = 4, boxW = 46, boxH = 28, gapX = 5, gapY = 7, x0 = 8, y0 = 72;
  int idx = 0;
  for (int r = 0; r < rows && idx < (int)COLORS_COUNT; r++) {
    for (int c = 0; c < cols && idx < (int)COLORS_COUNT; c++) {
      int bx = x0 + c * (boxW + gapX);
      int by = y0 + r * (boxH + gapY);
      tft.fillRoundRect(bx, by, boxW, boxH, 5, COLORS[idx].rgb565);
      tft.drawRoundRect(bx, by, boxW, boxH, 5, (idx == selColIdx) ? TFT_YELLOW : TFT_WHITE);
      idx++;
    }
  }

  drawStatusBarFrame();
  drawStatus(TR(STR_SELECT_COLOR), TFT_WHITE);
}

static void drawPickSubtypeScreen() {
  tft.fillScreen(TFT_BLACK);
  drawHeader(LTXT(TXT_VARIANT));
  fillButton(8, 35, 80, 28, TFT_DARKGREY, TFT_WHITE, TR(STR_BACK), TFT_WHITE, 2);
  fillButton(TFT_W - 120, 35, 52, 28, TFT_DARKGREY, TFT_WHITE, "<", TFT_WHITE, 2);
  fillButton(TFT_W - 60, 35, 52, 28, TFT_DARKGREY, TFT_WHITE, ">", TFT_WHITE, 2);

  const int cols = 2, rows = 3, w = 146, h = 34, gapX = 12, gapY = 10, x0 = 8, y0 = 78;
  const int itemsPerPage = cols * rows;
  const int total = 1 + getActiveVariantCount();
  const int pages = max(1, (total + itemsPerPage - 1) / itemsPerPage);
  if (pickSubtypePage < 0) pickSubtypePage = 0;
  if (pickSubtypePage >= pages) pickSubtypePage = pages - 1;

  int idx = pickSubtypePage * itemsPerPage;
  for (int r = 0; r < rows && idx < total; r++) {
    for (int c = 0; c < cols && idx < total; c++) {
      String label = (idx == 0) ? String(LTXT(TXT_NONE)) : variantNameByVal(getActiveVariantByIndex(idx - 1));
      bool selected = (idx == 0) ? (String(osSubtype).length() == 0) : (String(osSubtype) == label);
      fillButton(x0 + c*(w+gapX), y0 + r*(h+gapY), w, h, selected ? TFT_DARKGREEN : TFT_DARKGREY, TFT_WHITE, label, TFT_WHITE, 2);
      idx++;
    }
  }

  // Page indicator removed as requested
  drawStatusBarFrame();
  drawStatus(LTXT(TXT_VARIANT), TFT_WHITE);
}

static void drawPickMfgScreen() {
  tft.fillScreen(TFT_BLACK);
  drawHeader(LTXT(TXT_MANUFACTURER));
  fillButton(8, 35, 80, 28, TFT_DARKGREY, TFT_WHITE, TR(STR_BACK), TFT_WHITE, 2);
  fillButton(TFT_W - 120, 33, 52, 34, TFT_DARKGREY, TFT_WHITE, "<", TFT_WHITE, 2);
  fillButton(TFT_W - 60, 33, 52, 34, TFT_DARKGREY, TFT_WHITE, ">", TFT_WHITE, 2);

  int total = getAllManufacturerCount();
  int pages = max(1, (total + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE);
  if (pickMfgPage >= pages) pickMfgPage = pages - 1;
  if (pickMfgPage < 0) pickMfgPage = 0;

  const int cols = 2, rows = 4, w = 146, h = 32, gapX = 12, gapY = 6, x0 = 8, y0 = 70;
  int startIdx = pickMfgPage * ITEMS_PER_PAGE;
  int idx = startIdx;

  for (int r = 0; r < rows && idx < total; r++) {
    for (int c = 0; c < cols && idx < total; c++) {
      uint8_t id = getAllManufacturerByIndex(idx);
      fillButton(x0 + c * (w + gapX), y0 + r * (h + gapY), w, h,
                 (id == selMfg) ? TFT_DARKGREEN : TFT_NAVY, TFT_WHITE,
                 trimName18(manufacturerNameByVal(id)), TFT_WHITE, 2);
      idx++;
    }
  }

  drawStatusBarFrame();
  drawStatus(LTXT(TXT_MANUFACTURER), TFT_WHITE);
}

static void drawItemMenuScreen(const char* title, const char* listTitle, const char* editTitle, const char* addTitle, const char* resetTitle) {
  tft.fillScreen(TFT_BLACK);
  drawHeader(title);
  fillButton(8, 35, 80, 28, TFT_DARKGREY, TFT_WHITE, TR(STR_BACK), TFT_WHITE, 2);

  if (currentTagMode == TAGMODE_QIDI && isOfficialListActiveForCurrentQidiModel()) {
    fillButton(40, 82, 240, 34, TFT_DARKGREY, TFT_WHITE, LTXT(TXT_OFFICIAL_LOCKED), TFT_WHITE, 2);
    fillButton(40, 124, 240, 34, TFT_DARKGREEN, TFT_WHITE, String(LTXT(TXT_OFFICIAL_CFG)) + " " + qidiPrinterModelLabel(), TFT_WHITE, 2);
    fillButton(40, 166, 240, 34, TFT_NAVY, TFT_WHITE, LTXT(TXT_OFFICIAL_SOURCE), TFT_WHITE, 2);
    drawStatusBarFrame();
    drawStatus(listTitle, TFT_WHITE);
    return;
  }

  if (currentTagMode == TAGMODE_OPENSPOOL && ui == UI_MAT_MENU) {
    bool variantsPage = (matMenuPage == 1);
    fillButton(TFT_W - 88, 35, 80, 28, TFT_NAVY, TFT_WHITE, variantsPage ? "2/2" : "1/2", TFT_WHITE, 2);
    fillButton(40, 68, 240, 34, TFT_DARKCYAN, TFT_WHITE, variantsPage ? LTXT(TXT_VARIANT_EDIT) : editTitle, TFT_WHITE, 2);
    fillButton(40, 110, 240, 34, TFT_NAVY, TFT_WHITE, variantsPage ? LTXT(TXT_VARIANT_NEW) : addTitle, TFT_WHITE, 2);
    fillButton(40, 152, 240, 34, TFT_MAROON, TFT_WHITE, variantsPage ? LTXT(TXT_VARIANT_RESET) : resetTitle, TFT_WHITE, 2);
    drawStatusBarFrame();
    drawStatus(variantsPage ? LTXT(TXT_PAGE_VARIANTS) : LTXT(TXT_PAGE_MATERIALS), TFT_WHITE);
  } else {
    fillButton(40, 68, 240, 34, TFT_DARKCYAN, TFT_WHITE, editTitle, TFT_WHITE, 2);
    fillButton(40, 110, 240, 34, TFT_NAVY, TFT_WHITE, addTitle, TFT_WHITE, 2);
    fillButton(40, 152, 240, 34, TFT_MAROON, TFT_WHITE, resetTitle, TFT_WHITE, 2);
    drawStatusBarFrame();
    drawStatus(listTitle, TFT_WHITE);
  }
}

static int getPageCountForTotal(int total) {
  return max(1, (total + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE);
}

static void drawPageToggleButton(int currentPage, int totalPages) {
  if (totalPages <= 1) return;
  fillButton(TFT_W - 88, 35, 80, 28, TFT_NAVY, TFT_WHITE, String(currentPage + 1) + "/" + String(totalPages), TFT_WHITE, 2);
}

static void drawMaterialListScreen(const char* title, const char* statusText, int page, bool freeMode) {
  tft.fillScreen(TFT_BLACK);
  drawHeader(title);
  fillButton(8, 35, 80, 28, TFT_DARKGREY, TFT_WHITE, TR(STR_BACK), TFT_WHITE, 2);

  int total = freeMode ? getFreeMaterialCount() : getActiveMaterialCount();
  int pages = getPageCountForTotal(total);
  drawPageToggleButton(page, pages);
  const int cols = 2, rows = 4, w = 146, h = 32, gapX = 12, gapY = 6, x0 = 8, y0 = 70;
  int startIdx = page * ITEMS_PER_PAGE;
  int idx = startIdx;
  for (int r = 0; r < rows && idx < total; r++) {
    for (int c = 0; c < cols && idx < total; c++) {
      uint8_t v = freeMode ? getFreeMaterialByIndex(idx) : getActiveMaterialByIndex(idx);
      String label = freeMode ? String(v) : (String(v) + ": " + trimName18(materialNameByVal(v)));
      fillButton(x0 + c * (w + gapX), y0 + r * (h + gapY), w, h, TFT_DARKCYAN, TFT_WHITE, label, TFT_WHITE, 2);
      idx++;
    }
  }
  drawStatusBarFrame();
  drawStatus(statusText, TFT_WHITE);
}

static void drawVariantListScreen(const char* title, const char* statusText, int page, bool freeMode) {
  tft.fillScreen(TFT_BLACK);
  drawHeader(title);
  fillButton(8, 35, 80, 28, TFT_DARKGREY, TFT_WHITE, TR(STR_BACK), TFT_WHITE, 2);

  int total = freeMode ? getFreeVariantCount() : getActiveVariantCount();
  int pages = getPageCountForTotal(total);
  drawPageToggleButton(page, pages);
  const int cols = 2, rows = 4, w = 146, h = 32, gapX = 12, gapY = 6, x0 = 8, y0 = 70;
  int startIdx = page * ITEMS_PER_PAGE;
  int idx = startIdx;
  for (int r = 0; r < rows && idx < total; r++) {
    for (int c = 0; c < cols && idx < total; c++) {
      uint8_t v = freeMode ? getFreeVariantByIndex(idx) : getActiveVariantByIndex(idx);
      String label = freeMode ? String(v) : (String(v) + ": " + trimName18(variantNameByVal(v)));
      fillButton(x0 + c * (w + gapX), y0 + r * (h + gapY), w, h, TFT_DARKCYAN, TFT_WHITE, label, TFT_WHITE, 2);
      idx++;
    }
  }
  drawStatusBarFrame();
  drawStatus(statusText, TFT_WHITE);
}

static void drawManufacturerListScreen(const char* title, const char* statusText, int page, bool freeMode) {
  tft.fillScreen(TFT_BLACK);
  drawHeader(title);
  fillButton(8, 35, 80, 28, TFT_DARKGREY, TFT_WHITE, TR(STR_BACK), TFT_WHITE, 2);

  int total = freeMode ? getFreeManufacturerCount() : getEditableManufacturerCount();
  int pages = getPageCountForTotal(total);
  drawPageToggleButton(page, pages);
  const int cols = 2, rows = 4, w = 146, h = 32, gapX = 12, gapY = 6, x0 = 8, y0 = 70;
  int startIdx = page * ITEMS_PER_PAGE;
  int idx = startIdx;
  for (int r = 0; r < rows && idx < total; r++) {
    for (int c = 0; c < cols && idx < total; c++) {
      uint8_t v = freeMode ? getFreeManufacturerByIndex(idx) : getEditableManufacturerByIndex(idx);
      String label = freeMode ? String(v) : (String(v) + ": " + trimName18(manufacturerNameByVal(v)));
      fillButton(x0 + c * (w + gapX), y0 + r * (h + gapY), w, h, TFT_DARKCYAN, TFT_WHITE, label, TFT_WHITE, 2);
      idx++;
    }
  }
  drawStatusBarFrame();
  drawStatus(statusText, TFT_WHITE);
}

static void drawItemDetailScreen(const char* title, uint8_t value, const char* name, const char* buttonText, const char* statusText, bool showDeleteButton = false) {
  tft.fillScreen(TFT_BLACK);
  drawHeader(title);
  fillButton(8, 35, 80, 28, TFT_DARKGREY, TFT_WHITE, TR(STR_BACK), TFT_WHITE, 2);
  if (showDeleteButton) {
    fillButton(120, 35, 80, 28, TFT_MAROON, TFT_WHITE, LTXT(TXT_DELETE), TFT_WHITE, 2);
  }
  fillButton(TFT_W - 88, 35, 80, 28, TFT_DARKGREEN, TFT_WHITE, LTXT(TXT_SAVE), TFT_WHITE, 2);

  tft.setTextColor(TFT_YELLOW);
  tft.drawString(LTXT(TXT_NUMBER), 16, 78, 2);
  tft.drawString(LTXT(TXT_NAME), 16, 118, 2);

  fillButton(120, 72, 180, 30, TFT_DARKGREY, TFT_WHITE, String(value), TFT_WHITE, 2);
  fillButton(120, 112, 180, 30, TFT_NAVY, TFT_WHITE, trimName18(String(name)), TFT_WHITE, 2);
  fillButton(70, 160, 180, 34, TFT_DARKCYAN, TFT_WHITE, buttonText, TFT_WHITE, 2);

  drawStatusBarFrame();
  drawStatus(statusText, TFT_WHITE);
}


static bool openSpoolMaterialFieldsValid(const char* nameBuf, const char* minBuf, const char* maxBuf, const char* bedMinBuf, const char* bedMaxBuf, bool requireBed) {
  if (!nameBuf || !nameBuf[0]) return false;
  if (!minBuf || !minBuf[0]) return false;
  if (!maxBuf || !maxBuf[0]) return false;
  if (requireBed) {
    if (!bedMinBuf || !bedMinBuf[0]) return false;
    if (!bedMaxBuf || !bedMaxBuf[0]) return false;
  }
  return true;
}

static void drawOpenSpoolMaterialDetailScreen(const char* title, bool addMode) {
  const char* nameBuf = addMode ? addMatName : editMatName;
  const char* minBuf = addMode ? addMatMin : editMatMin;
  const char* maxBuf = addMode ? addMatMax : editMatMax;
  const char* bedMinBuf = addMode ? addMatBedMin : editMatBedMin;
  const char* bedMaxBuf = addMode ? addMatBedMax : editMatBedMax;
  tft.fillScreen(TFT_BLACK);
  drawHeader(title);
  fillButton(8, 35, 80, 28, TFT_DARKGREY, TFT_WHITE, TR(STR_BACK), TFT_WHITE, 2);
  if (!addMode) {
    fillButton(120, 35, 80, 28, TFT_MAROON, TFT_WHITE, LTXT(TXT_DELETE), TFT_WHITE, 2);
  }
  fillButton(TFT_W - 88, 35, 80, 28, TFT_DARKGREEN, TFT_WHITE, LTXT(TXT_SAVE), TFT_WHITE, 2);

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString(LTXT(TXT_NAME), 16, 72, 2);
  fillButton(120, 66, 180, 28, TFT_NAVY, TFT_WHITE, trimName18(String(nameBuf[0] ? nameBuf : LTXT(TXT_ENTER_NAME))), TFT_WHITE, 2);

  tft.fillRect(12, 108, 144, 20, TFT_BLACK);
  tft.fillRect(166, 108, 140, 20, TFT_BLACK);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString(LTXT(TXT_NOZZLE_MIN), 16, 112, 2);
  tft.drawString(LTXT(TXT_NOZZLE_MAX), 170, 112, 2);
  fillButton(18, 132, 132, 26, TFT_DARKGREY, TFT_WHITE, String(minBuf[0] ? minBuf : "000"), TFT_WHITE, 2);
  fillButton(170, 132, 132, 26, TFT_DARKGREY, TFT_WHITE, String(maxBuf[0] ? maxBuf : "000"), TFT_WHITE, 2);

  tft.fillRect(12, 164, 144, 20, TFT_BLACK);
  tft.fillRect(166, 164, 140, 20, TFT_BLACK);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString(LTXT(TXT_BED_MIN), 16, 168, 2);
  tft.drawString(LTXT(TXT_BED_MAX), 170, 168, 2);
  fillButton(18, 188, 132, 22, TFT_DARKGREY, TFT_WHITE, String(bedMinBuf[0] ? bedMinBuf : "000"), TFT_WHITE, 2);
  fillButton(170, 188, 132, 22, TFT_DARKGREY, TFT_WHITE, String(bedMaxBuf[0] ? bedMaxBuf : "000"), TFT_WHITE, 2);

  drawStatusBarFrame();
  drawStatus(title, TFT_WHITE);
}

static void drawConfirmScreen(const char* title, const char* l1, const char* l2, const char* l3) {
  tft.fillScreen(TFT_BLACK);
  drawHeader(title);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(l1, TFT_W / 2, 80, 2);
  tft.drawString(l2, TFT_W / 2, 104, 2);
  tft.drawString(l3, TFT_W / 2, 128, 2);
  tft.setTextDatum(TL_DATUM);
  fillButton(45, 160, 90, 34, TFT_DARKGREEN, TFT_WHITE, LTXT(TXT_YES), TFT_WHITE, 2);
  fillButton(185, 160, 90, 34, TFT_MAROON, TFT_WHITE, LTXT(TXT_NO), TFT_WHITE, 2);
  drawStatusBarFrame();
  drawStatus(LTXT(TXT_CONFIRM), TFT_WHITE);
}

static void drawDeleteConfirmScreen(const char* title, const char* l1, const char* l2) {
  tft.fillScreen(TFT_BLACK);
  drawHeader(title);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(l1, TFT_W / 2, 88, 2);
  tft.drawString(l2, TFT_W / 2, 112, 2);
  tft.setTextDatum(TL_DATUM);
  fillButton(45, 160, 90, 34, TFT_DARKGREY, TFT_WHITE, LTXT(TXT_CANCEL), TFT_WHITE, 2);
  fillButton(185, 160, 90, 34, TFT_MAROON, TFT_WHITE, LTXT(TXT_DELETE), TFT_WHITE, 2);
  drawStatusBarFrame();
  drawStatus(LTXT(TXT_CONFIRM), TFT_WHITE);
}

static void drawMessageOkScreen() {
  tft.fillScreen(TFT_BLACK);
  drawHeader(messageTitle);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(messageLine1, TFT_W / 2, 60, 2);
  tft.drawString(messageLine2, TFT_W / 2, 82, 2);
  tft.drawString(messageLine3, TFT_W / 2, 104, 2);
  if (messageLine4.length() > 0) {
    if ((int)tft.textWidth(messageLine4, 2) > TFT_W - 20) {
      int split = messageLine4.lastIndexOf(' ');
      if (split < 0) split = messageLine4.length() / 2;
      String a = messageLine4.substring(0, split);
      String b = messageLine4.substring(split);
      b.trim();
      tft.drawString(a, TFT_W / 2, 126, 2);
      tft.drawString(b, TFT_W / 2, 148, 2);
    } else {
      tft.drawString(messageLine4, TFT_W / 2, 126, 2);
    }
  }
  tft.setTextDatum(TL_DATUM);

  fillButton(110, 176, 100, 28, TFT_DARKGREEN, TFT_WHITE, LTXT(TXT_OK), TFT_WHITE, 2);
  drawStatusBarFrame();
  drawStatus(LTXT(TXT_NOTE_STATUS), TFT_WHITE);
}

// ==================== Keyboard ====================
static void openKeyboardForBufferExtended(char* target, uint8_t maxLen, UIState returnState) {
  kbTargetBuffer = target;
  kbTargetMaxLen = maxLen;
  uiBeforeKeyboard = returnState;
  kbForceNumeric = false;
  kbStrictNumeric = false;
  kbAllowDot = false;
  kbHexOnly = false;
  kbAllowExtendedAscii = true;
  kbMode = KB_UPPER;
  ui = UI_KEYBOARD;
  needRedraw = true;
}

static void openKeyboardForBufferNumeric(char* target, uint8_t maxLen, UIState returnState) {
  kbTargetBuffer = target;
  kbTargetMaxLen = maxLen;
  uiBeforeKeyboard = returnState;
  kbForceNumeric = true;
  kbStrictNumeric = true;
  kbAllowDot = false;
  kbHexOnly = false;
  kbAllowExtendedAscii = false;
  kbMode = KB_NUM;
  ui = UI_KEYBOARD;
  needRedraw = true;
}

static void openKeyboardForBufferNumericDot(char* target, uint8_t maxLen, UIState returnState) {
  kbTargetBuffer = target;
  kbTargetMaxLen = maxLen;
  uiBeforeKeyboard = returnState;
  kbForceNumeric = true;
  kbStrictNumeric = true;
  kbAllowDot = true;
  kbHexOnly = false;
  kbAllowExtendedAscii = false;
  kbMode = KB_NUM;
  ui = UI_KEYBOARD;
  needRedraw = true;
}

static void openKeyboardForBufferHex(char* target, uint8_t maxLen, UIState returnState) {
  kbTargetBuffer = target;
  kbTargetMaxLen = maxLen;
  uiBeforeKeyboard = returnState;
  kbForceNumeric = true;
  kbStrictNumeric = false;
  kbAllowDot = false;
  kbHexOnly = true;
  kbAllowExtendedAscii = false;
  kbMode = KB_HEX;
  ui = UI_KEYBOARD;
  needRedraw = true;
}

static const char* const KB_EXT_SYM1_ROW1[7] = {
  "!", "\"", "$", "%", "&", "@", "?"
};
static const char* const KB_EXT_SYM1_ROW2[7] = {
  ";", ":", ",", ".", "-", "_", "+"
};
static const char* const KB_EXT_SYM2_ROW1[7] = {
  "(", ")", "=", "*", "#", "<", ">"
};
static const char* const KB_EXT_SYM2_ROW2[7] = {
  "{", "}", "|", "\\", "~", "'", "/"
};

static void keyboardAppendText(const char* txt) {
  if (!kbTargetBuffer || !txt) return;
  size_t len = strlen(kbTargetBuffer);
  while (*txt && len < kbTargetMaxLen) {
    kbTargetBuffer[len++] = *txt++;
  }
  kbTargetBuffer[len] = '\0';
}

static void keyboardAppendChar(char ch) {
  char tmp[2] = { ch, '\0' };
  keyboardAppendText(tmp);
}

static void keyboardClearBuffer() {
  if (kbTargetBuffer) kbTargetBuffer[0] = '\0';
}

static void keyboardBackspace() {
  if (!kbTargetBuffer) return;
  size_t len = strlen(kbTargetBuffer);
  if (len == 0) return;
  do {
    len--;
  } while (len > 0 && (((uint8_t)kbTargetBuffer[len] & 0xC0) == 0x80));
  kbTargetBuffer[len] = '\0';
}

static void drawKeyboardKey(int x, int y, int w, int h, const String& txt, uint16_t fill = TFT_DARKGREY) {
  fillButton(x, y, w, h, fill, TFT_WHITE, txt, TFT_WHITE, 2);
}

static void drawKeyboardScreen() {
  tft.fillScreen(TFT_BLACK);
  drawHeader(LTXT(TXT_KEYBOARD));

  fillButton(8, 35, 80, 28, TFT_MAROON, TFT_WHITE, LTXT(TXT_CANCEL), TFT_WHITE, 2);
  fillButton(TFT_W - 88, 35, 80, 28, TFT_DARKGREEN, TFT_WHITE, LTXT(TXT_OK), TFT_WHITE, 2);

  tft.drawRoundRect(8, 68, TFT_W - 16, 30, 4, TFT_WHITE);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  String current = kbTargetBuffer ? String(kbTargetBuffer) : "";
  tft.drawString(current, 12, 76, 2);

  const int keyW = 28, keyH = 24, gap = 2;
  const int row1Y = 106, row2Y = 132, row3Y = 158, row4Y = 186;

  if (kbMode == KB_HEX) {
    const int hxW = 44, hxH = 30, hxGap = 8;
    const int hxLeft = 10;
    const int hxRow1Y = 106, hxRow2Y = 144, hxRow3Y = 182;
    const char* row1 = "123456";
    const char* row2 = "7890AB";
    const char* row3 = "CDEF";
    for (int i = 0; i < 6; i++) drawKeyboardKey(hxLeft + i * (hxW + hxGap), hxRow1Y, hxW, hxH, String(row1[i]));
    for (int i = 0; i < 6; i++) drawKeyboardKey(hxLeft + i * (hxW + hxGap), hxRow2Y, hxW, hxH, String(row2[i]));
    for (int i = 0; i < 4; i++) drawKeyboardKey(58 + i * (hxW + hxGap), hxRow3Y, hxW, hxH, String(row3[i]));
    drawKeyboardKey(10, hxRow3Y, 44, hxH, LTXT(TXT_CLEAR), TFT_MAROON);
    drawKeyboardKey(266, hxRow3Y, 44, hxH, LTXT(TXT_BKSP), TFT_MAROON);
  } else if (kbMode == KB_SYM1 || kbMode == KB_SYM2) {
      const char* const* row1 = (kbMode == KB_SYM1) ? KB_EXT_SYM1_ROW1 : KB_EXT_SYM2_ROW1;
      const char* const* row2 = (kbMode == KB_SYM1) ? KB_EXT_SYM1_ROW2 : KB_EXT_SYM2_ROW2;
      const int symW = 37, symGap = 2;
      const int symX0 = (TFT_W - (7 * symW + 6 * symGap)) / 2;

      for (int i = 0; i < 7; i++) {
        int bx = symX0 + i * (symW + symGap);
        drawKeyboardKey(bx, row1Y, symW, keyH, String(row1[i]), TFT_DARKGREY);
        drawKeyboardKey(bx, row2Y, symW, keyH, String(row2[i]), TFT_DARKGREY);
      }

      drawKeyboardKey(10, row3Y, 64, keyH, "ABC", TFT_NAVY);
      drawKeyboardKey(78, row3Y, 64, keyH, "abc", TFT_NAVY);
      drawKeyboardKey(146, row3Y, 64, keyH, "123", TFT_NAVY);
      drawKeyboardKey(214, row3Y, 96, keyH, (kbMode == KB_SYM1) ? "SYM2" : "SYM1", TFT_DARKGREEN);

      drawKeyboardKey(10, row4Y, 72, keyH, LTXT(TXT_CLEAR), TFT_MAROON);
      drawKeyboardKey(86, row4Y, 148, keyH, LTXT(TXT_SPACE), TFT_DARKCYAN);
      drawKeyboardKey(238, row4Y, 72, keyH, LTXT(TXT_BKSP), TFT_MAROON);
  } else if (kbMode == KB_UPPER || kbMode == KB_LOWER) {
    const char* row1 = (kbMode == KB_UPPER) ? "QWERTYUIOP" : "qwertyuiop";
    const char* row2 = (kbMode == KB_UPPER) ? "ASDFGHJKL"  : "asdfghjkl";
    const char* row3 = (kbMode == KB_UPPER) ? "ZXCVBNM"    : "zxcvbnm";

    for (int i = 0; i < 10; i++) drawKeyboardKey(10 + i * (keyW + gap), row1Y, keyW, keyH, String(row1[i]));
    for (int i = 0; i < 9; i++)  drawKeyboardKey(24 + i * (keyW + gap), row2Y, keyW, keyH, String(row2[i]));

    drawKeyboardKey(10, row3Y, 42, keyH, (kbMode == KB_UPPER) ? "ABC" : "abc", TFT_NAVY);
    for (int i = 0; i < 7; i++)  drawKeyboardKey(54 + i * (keyW + gap), row3Y, keyW, keyH, String(row3[i]));
    drawKeyboardKey(54 + 7 * (keyW + gap), row3Y, 46, keyH, "SYM", TFT_NAVY);
    drawKeyboardKey(10, row4Y, 48, keyH, "123", TFT_NAVY);
    drawKeyboardKey(62, row4Y, 56, keyH, LTXT(TXT_CLEAR), TFT_MAROON);
    drawKeyboardKey(122, row4Y, 126, keyH, LTXT(TXT_SPACE), TFT_DARKCYAN);
    drawKeyboardKey(252, row4Y, 58, keyH, LTXT(TXT_BKSP), TFT_MAROON);
  } else {
    const char* row1 = "1234567890";
    for (int i = 0; i < 10; i++) drawKeyboardKey(10 + i * (keyW + gap), row1Y, keyW, keyH, String(row1[i]));
    if (kbStrictNumeric) {
      drawKeyboardKey(24, row2Y, 84, keyH, kbAllowDot ? "." : "", kbAllowDot ? TFT_DARKCYAN : TFT_DARKGREY);
      drawKeyboardKey(112, row2Y, 96, keyH, LTXT(TXT_CLEAR), TFT_MAROON);
      drawKeyboardKey(212, row2Y, 84, keyH, LTXT(TXT_BKSP), TFT_MAROON);
    } else {
      drawKeyboardKey(24, row2Y, 40, keyH, "-", TFT_DARKCYAN);
      drawKeyboardKey(68, row2Y, 40, keyH, "_", TFT_DARKCYAN);
      drawKeyboardKey(112, row2Y, 40, keyH, ".", TFT_DARKCYAN);
      drawKeyboardKey(156, row2Y, 40, keyH, ",", TFT_DARKCYAN);
      drawKeyboardKey(200, row2Y, 40, keyH, "@", TFT_DARKCYAN);
      drawKeyboardKey(244, row2Y, 40, keyH, "/", TFT_DARKCYAN);
      drawKeyboardKey(56, row3Y, 64, keyH, "ABC", TFT_NAVY);
      drawKeyboardKey(128, row3Y, 64, keyH, "abc", TFT_NAVY);
      drawKeyboardKey(200, row3Y, 64, keyH, "SYM", TFT_NAVY);
      drawKeyboardKey(10, row4Y, 72, keyH, LTXT(TXT_CLEAR), TFT_MAROON);
      drawKeyboardKey(86, row4Y, 148, keyH, LTXT(TXT_SPACE), TFT_DARKCYAN);
      drawKeyboardKey(238, row4Y, 72, keyH, LTXT(TXT_BKSP), TFT_MAROON);
    }
  }

  drawStatusBarFrame();
  if (kbMode == KB_UPPER) drawStatus("ABC", TFT_WHITE);
  else if (kbMode == KB_LOWER) drawStatus("abc", TFT_WHITE);
  else if (kbMode == KB_HEX) drawStatus("HEX", TFT_WHITE);
  else if (kbMode == KB_SYM1) drawStatus("SYM1", TFT_WHITE);
  else if (kbMode == KB_SYM2) drawStatus("SYM2", TFT_WHITE);
  else drawStatus(kbStrictNumeric ? LTXT(TXT_NUMPAD) : "123", TFT_WHITE);
}

static bool keyboardHandleTouch(int x, int y) {
  if (x >= 8 && x < 88 && y >= 35 && y < 63) {
    kbForceNumeric = false;
    kbStrictNumeric = false;
    kbAllowDot = false;
    kbHexOnly = false;
    kbAllowExtendedAscii = false;
    ui = uiBeforeKeyboard;
    if (uiBeforeKeyboard == UI_COLOR_PICKER && cpHexEditActive) applyCpHexEdit();
    if (uiBeforeKeyboard == UI_SETUP && (kbTargetBuffer == wifiSsid || kbTargetBuffer == wifiPassword)) {
      saveWifiSettings();
      applyWifiState(true);
    }
    needRedraw = true;
    return true;
  }
  if (x >= TFT_W - 88 && x < TFT_W - 8 && y >= 35 && y < 63) {
    kbForceNumeric = false;
    kbStrictNumeric = false;
    kbAllowDot = false;
    kbHexOnly = false;
    kbAllowExtendedAscii = false;
    ui = uiBeforeKeyboard;
    if (uiBeforeKeyboard == UI_COLOR_PICKER && cpHexEditActive) applyCpHexEdit();
    if (uiBeforeKeyboard == UI_SETUP && (kbTargetBuffer == wifiSsid || kbTargetBuffer == wifiPassword)) {
      saveWifiSettings();
      applyWifiState(true);
    }
    needRedraw = true;
    return true;
  }

  const int keyW = 28, keyH = 24, gap = 2;
  const int row1Y = 106, row2Y = 132, row3Y = 158, row4Y = 186;

  if (kbMode == KB_HEX) {
    const int hxW = 44, hxH = 30, hxGap = 8;
    const int hxLeft = 10;
    const int hxRow1Y = 106, hxRow2Y = 144, hxRow3Y = 182;
    const char* row1 = "123456";
    const char* row2 = "7890AB";
    const char* row3 = "CDEF";
    for (int i = 0; i < 6; i++) {
      int bx = hxLeft + i * (hxW + hxGap);
      if (x >= bx && x < bx + hxW && y >= hxRow1Y && y < hxRow1Y + hxH) { keyboardAppendChar(row1[i]); needRedraw = true; return true; }
      if (x >= bx && x < bx + hxW && y >= hxRow2Y && y < hxRow2Y + hxH) { keyboardAppendChar(row2[i]); needRedraw = true; return true; }
    }
    for (int i = 0; i < 4; i++) {
      int bx = 58 + i * (hxW + hxGap);
      if (x >= bx && x < bx + hxW && y >= hxRow3Y && y < hxRow3Y + hxH) { keyboardAppendChar(row3[i]); needRedraw = true; return true; }
    }
    if (x >= 10 && x < 54 && y >= hxRow3Y && y < hxRow3Y + hxH) {
      keyboardClearBuffer();
      needRedraw = true;
      return true;
    }
    if (x >= 266 && x < 310 && y >= hxRow3Y && y < hxRow3Y + hxH) { keyboardBackspace(); needRedraw = true; return true; }
  } else if (kbMode == KB_SYM1 || kbMode == KB_SYM2) {
    const char* const* row1 = (kbMode == KB_SYM1) ? KB_EXT_SYM1_ROW1 : KB_EXT_SYM2_ROW1;
    const char* const* row2 = (kbMode == KB_SYM1) ? KB_EXT_SYM1_ROW2 : KB_EXT_SYM2_ROW2;
    const int symW = 37, symGap = 2;
    const int symX0 = (TFT_W - (7 * symW + 6 * symGap)) / 2;
    for (int i = 0; i < 7; i++) {
      int bx = symX0 + i * (symW + symGap);
      if (x >= bx && x < bx + symW && y >= row1Y && y < row1Y + keyH) { keyboardAppendText(row1[i]); needRedraw = true; return true; }
      if (x >= bx && x < bx + symW && y >= row2Y && y < row2Y + keyH) { keyboardAppendText(row2[i]); needRedraw = true; return true; }
    }
    if (x >= 10 && x < 74 && y >= row3Y && y < row3Y + keyH) { kbMode = KB_UPPER; needRedraw = true; return true; }
    if (x >= 78 && x < 142 && y >= row3Y && y < row3Y + keyH) { kbMode = KB_LOWER; needRedraw = true; return true; }
    if (x >= 146 && x < 210 && y >= row3Y && y < row3Y + keyH) { kbMode = KB_NUM; needRedraw = true; return true; }
    if (x >= 214 && x < 310 && y >= row3Y && y < row3Y + keyH) { kbMode = (kbMode == KB_SYM1) ? KB_SYM2 : KB_SYM1; needRedraw = true; return true; }
    if (x >= 10 && x < 82 && y >= row4Y && y < row4Y + keyH) { keyboardClearBuffer(); needRedraw = true; return true; }
    if (x >= 86 && x < 234 && y >= row4Y && y < row4Y + keyH) { keyboardAppendChar(' '); needRedraw = true; return true; }
    if (x >= 238 && x < 310 && y >= row4Y && y < row4Y + keyH) { keyboardBackspace(); needRedraw = true; return true; }
  } else if (kbMode == KB_UPPER || kbMode == KB_LOWER) {
    const char* row1 = (kbMode == KB_UPPER) ? "QWERTYUIOP" : "qwertyuiop";
    const char* row2 = (kbMode == KB_UPPER) ? "ASDFGHJKL"  : "asdfghjkl";
    const char* row3 = (kbMode == KB_UPPER) ? "ZXCVBNM"    : "zxcvbnm";

    for (int i = 0; i < 10; i++) {
      int bx = 10 + i * (keyW + gap);
      if (x >= bx && x < bx + keyW && y >= row1Y && y < row1Y + keyH) { keyboardAppendChar(row1[i]); needRedraw = true; return true; }
    }
    for (int i = 0; i < 9; i++) {
      int bx = 24 + i * (keyW + gap);
      if (x >= bx && x < bx + keyW && y >= row2Y && y < row2Y + keyH) { keyboardAppendChar(row2[i]); needRedraw = true; return true; }
    }
    if (x >= 10 && x < 52 && y >= row3Y && y < row3Y + keyH) { kbMode = (kbMode == KB_UPPER) ? KB_LOWER : KB_UPPER; needRedraw = true; return true; }
    for (int i = 0; i < 7; i++) {
      int bx = 54 + i * (keyW + gap);
      if (x >= bx && x < bx + keyW && y >= row3Y && y < row3Y + keyH) { keyboardAppendChar(row3[i]); needRedraw = true; return true; }
    }
    if (x >= 54 + 7 * (keyW + gap) && x < 54 + 7 * (keyW + gap) + 72 && y >= row3Y && y < row3Y + keyH) { kbMode = KB_SYM1; needRedraw = true; return true; }
    if (x >= 10 && x < 58 && y >= row4Y && y < row4Y + keyH) { kbMode = KB_NUM; needRedraw = true; return true; }
    if (x >= 62 && x < 118 && y >= row4Y && y < row4Y + keyH) { keyboardClearBuffer(); needRedraw = true; return true; }
    if (x >= 122 && x < 248 && y >= row4Y && y < row4Y + keyH) { keyboardAppendChar(' '); needRedraw = true; return true; }
    if (x >= 252 && x < 310 && y >= row4Y && y < row4Y + keyH) { keyboardBackspace(); needRedraw = true; return true; }
  } else {
    const char* row1 = "1234567890";
    for (int i = 0; i < 10; i++) {
      int bx = 10 + i * (keyW + gap);
      if (x >= bx && x < bx + keyW && y >= row1Y && y < row1Y + keyH) { keyboardAppendChar(row1[i]); needRedraw = true; return true; }
    }
    if (kbStrictNumeric) {
      if (kbAllowDot && x >= 24 && x < 108 && y >= row2Y && y < row2Y + keyH) { keyboardAppendChar('.'); needRedraw = true; return true; }
      if (x >= 112 && x < 208 && y >= row2Y && y < row2Y + keyH) { keyboardClearBuffer(); needRedraw = true; return true; }
      if (x >= 212 && x < 296 && y >= row2Y && y < row2Y + keyH) { keyboardBackspace(); needRedraw = true; return true; }
    } else {
      if (x >= 24 && x < 64 && y >= row2Y && y < row2Y + keyH) { keyboardAppendChar('-'); needRedraw = true; return true; }
      if (x >= 68 && x < 108 && y >= row2Y && y < row2Y + keyH) { keyboardAppendChar('_'); needRedraw = true; return true; }
      if (x >= 112 && x < 152 && y >= row2Y && y < row2Y + keyH) { keyboardAppendChar('.'); needRedraw = true; return true; }
      if (x >= 156 && x < 196 && y >= row2Y && y < row2Y + keyH) { keyboardAppendChar(','); needRedraw = true; return true; }
      if (x >= 200 && x < 240 && y >= row2Y && y < row2Y + keyH) { keyboardAppendChar('@'); needRedraw = true; return true; }
      if (x >= 244 && x < 284 && y >= row2Y && y < row2Y + keyH) { keyboardAppendChar('/'); needRedraw = true; return true; }
      if (x >= 56 && x < 120 && y >= row3Y && y < row3Y + keyH) { kbMode = KB_UPPER; needRedraw = true; return true; }
      if (x >= 128 && x < 192 && y >= row3Y && y < row3Y + keyH) { kbMode = KB_LOWER; needRedraw = true; return true; }
      if (x >= 200 && x < 264 && y >= row3Y && y < row3Y + keyH) { kbMode = KB_SYM1; needRedraw = true; return true; }
      if (x >= 10 && x < 82 && y >= row4Y && y < row4Y + keyH) { keyboardClearBuffer(); needRedraw = true; return true; }
      if (x >= 86 && x < 234 && y >= row4Y && y < row4Y + keyH) { keyboardAppendChar(' '); needRedraw = true; return true; }
      if (x >= 238 && x < 310 && y >= row4Y && y < row4Y + keyH) { keyboardBackspace(); needRedraw = true; return true; }
    }
  }

  return false;
}

// ==================== UI redraw ====================
static void uiRedrawIfNeeded() {
  if (!needRedraw) return;
  needRedraw = false;

  switch (ui) {
    case UI_MAIN:               if (currentTagMode == TAGMODE_OPENSPOOL && autoPanelVisible) drawOpenSpoolAutoPanel(); else drawMainScreen(); break;
    case UI_READ:               drawReadScreen(); break;
    case UI_WRITE:              drawWriteScreen(); break;
    case UI_PICK_MAT:           drawPickMatScreen(); break;
    case UI_PICK_COLOR:         drawPickColorScreen(); break;
    case UI_PICK_SUBTYPE:       drawPickSubtypeScreen(); break;
    case UI_PICK_MFG:           drawPickMfgScreen(); break;
    case UI_COLOR_PICKER:       drawColorPickerScreen(); break;
    case UI_SETUP:              drawSetupScreen(); break;
    case UI_LANG_SELECT:        drawLangSelectScreen(); break;
    case UI_OS_TAGINFO_CONFIG:  drawOpenSpoolTagInfoConfigScreen(); break;

    case UI_MAT_MENU:           drawItemMenuScreen(LTXT(TXT_MATERIAL), LTXT(TXT_MATERIAL_LIST), LTXT(TXT_MATERIAL_EDIT), LTXT(TXT_MATERIAL_NEW), LTXT(TXT_MATERIAL_RESET)); break;
    case UI_VAR_EDIT_LIST:      drawVariantListScreen(LTXT(TXT_VARIANT_EDIT), LTXT(TXT_SELECT_ITEM), varListPage, false); break;
    case UI_VAR_EDIT_DETAIL:    drawItemDetailScreen(LTXT(TXT_VARIANT_EDIT), editVarVal, editVarName, LTXT(TXT_CHANGE_NAME), LTXT(TXT_VARIANT_EDIT), true); break;
    case UI_VAR_ADD_LIST:       drawVariantListScreen(LTXT(TXT_VARIANT_NEW), LTXT(TXT_CHOOSE_FREE), varFreePage, true); break;
    case UI_VAR_ADD_DETAIL:     drawItemDetailScreen(LTXT(TXT_VARIANT_NEW), addVarVal, addVarName, LTXT(TXT_ENTER_NAME), LTXT(TXT_VARIANT_NEW)); break;
    case UI_MAT_EDIT_LIST:      drawMaterialListScreen(LTXT(TXT_MATERIAL_EDIT), LTXT(TXT_SELECT_ITEM), matListPage, false); break;
    case UI_MAT_EDIT_DETAIL:    if (currentTagMode == TAGMODE_OPENSPOOL) drawOpenSpoolMaterialDetailScreen(LTXT(TXT_MATERIAL_EDIT), false); else drawItemDetailScreen(LTXT(TXT_MATERIAL_EDIT), editMatVal, editMatName, LTXT(TXT_CHANGE_NAME), LTXT(TXT_MATERIAL_EDIT), true); break;
    case UI_MAT_ADD_LIST:       drawMaterialListScreen(LTXT(TXT_MATERIAL_NEW), LTXT(TXT_CHOOSE_FREE), matFreePage, true); break;
    case UI_MAT_ADD_DETAIL:     if (currentTagMode == TAGMODE_OPENSPOOL) drawOpenSpoolMaterialDetailScreen(LTXT(TXT_MATERIAL_NEW), true); else drawItemDetailScreen(LTXT(TXT_MATERIAL_NEW), addMatVal, addMatName, LTXT(TXT_ENTER_NAME), LTXT(TXT_MATERIAL_NEW)); break;
    case UI_MAT_RESET_CONFIRM:  drawConfirmScreen(LTXT(TXT_MATERIAL), LTXT(TXT_MAT_RESET_Q1), LTXT(TXT_MAT_RESET_Q2), LTXT(TXT_MAT_RESET_Q3)); break;
    case UI_MAT_DELETE_CONFIRM: drawDeleteConfirmScreen(LTXT(TXT_MATERIAL), LTXT(TXT_DELETE_Q1_MAT), LTXT(TXT_DELETE_Q2_MAT)); break;
    case UI_VAR_RESET_CONFIRM:  drawConfirmScreen(LTXT(TXT_VARIANT), LTXT(TXT_VAR_RESET_Q1), LTXT(TXT_VAR_RESET_Q2), LTXT(TXT_VAR_RESET_Q3)); break;
    case UI_VAR_DELETE_CONFIRM: drawDeleteConfirmScreen(LTXT(TXT_VARIANT), LTXT(TXT_DELETE_Q1_VAR), LTXT(TXT_DELETE_Q2_VAR)); break;

    case UI_MFG_MENU:           drawItemMenuScreen(LTXT(TXT_MANUFACTURER), LTXT(TXT_MFG_LIST), LTXT(TXT_MFG_EDIT), LTXT(TXT_MFG_NEW), LTXT(TXT_MFG_RESET)); break;
    case UI_MFG_EDIT_LIST:      drawManufacturerListScreen(LTXT(TXT_MFG_EDIT), LTXT(TXT_SELECT_ITEM), mfgListPage, false); break;
    case UI_MFG_EDIT_DETAIL:    drawItemDetailScreen(LTXT(TXT_MFG_EDIT), editMfgVal, editMfgName, LTXT(TXT_CHANGE_NAME), LTXT(TXT_MFG_EDIT), true); break;
    case UI_MFG_ADD_LIST:       drawManufacturerListScreen(LTXT(TXT_MFG_NEW), LTXT(TXT_CHOOSE_FREE), mfgFreePage, true); break;
    case UI_MFG_ADD_DETAIL:     drawItemDetailScreen(LTXT(TXT_MFG_NEW), addMfgVal, addMfgName, LTXT(TXT_ENTER_NAME), LTXT(TXT_MFG_NEW)); break;
    case UI_MFG_RESET_CONFIRM:  drawConfirmScreen(LTXT(TXT_MANUFACTURER), LTXT(TXT_MFG_RESET_Q1), LTXT(TXT_MFG_RESET_Q2), LTXT(TXT_MFG_RESET_Q3)); break;
    case UI_MFG_DELETE_CONFIRM: drawDeleteConfirmScreen(LTXT(TXT_MANUFACTURER), LTXT(TXT_DELETE_Q1_MFG), LTXT(TXT_DELETE_Q2_MFG)); break;

    case UI_FACTORY_RESET_CONFIRM: drawConfirmScreen(LTXT(TXT_FACTORY_RESET_TITLE), LTXT(TXT_FACTORY_RESET_Q1), LTXT(TXT_FACTORY_RESET_Q2), LTXT(TXT_FACTORY_RESET_Q3)); break;
    case UI_SD_FORMAT_CONFIRM:     drawConfirmScreen((uiLang == LANG_DE) ? "SD-Karte" : "SD card",
                                                     (uiLang == LANG_DE) ? "Alle Dateien auf" : "Delete all files on",
                                                     (uiLang == LANG_DE) ? "der MicroSD loeschen" : "the MicroSD card",
                                                     (uiLang == LANG_DE) ? "und Ordner neu anlegen?" : "and recreate folders?"); break;

    case UI_MESSAGE_OK:         drawMessageOkScreen(); break;
    case UI_KEYBOARD:           drawKeyboardScreen(); break;
  }
}

// ==================== Touch handling ====================
static bool hit(int bx, int by, int bw, int bh, int tx, int ty) {
  return (tx >= bx && tx < bx + bw && ty >= by && ty < by + bh);
}

static void uiHandleTouch(int x, int y) {
  if (ui == UI_KEYBOARD) {
    keyboardHandleTouch(x, y);
    return;
  }

  if (ui == UI_MESSAGE_OK) {
    if (hit(110, 176, 100, 28, x, y)) {
      ui = messageOkNextState;
      needRedraw = true;
    }
    return;
  }

  if (ui == UI_MAIN) {
    if (hit(20, 50, 140, 50, x, y))  { autoDetectEnabled = !autoDetectEnabled; saveAllSetupPreferences(); readOpenSpoolDetailsVisible = false; readPopupVisible = false; readResultPending = false; readPopupMisses = 0; autoPanelVisible = false; needRedraw = true; return; }
    if (hit(160, 50, 140, 50, x, y)) { syncOpenSpoolColorFromSelection(); if (currentTagMode == TAGMODE_OPENSPOOL) openSpoolWritePage = 0; ui = UI_WRITE; needRedraw = true; return; }
    if (hit(20, 110, 140, 50, x, y)) { setupPage = 0; normalizeSetupPage(); ui = UI_SETUP; needRedraw = true; return; }
    if (hit(160, 110, 140, 50, x, y)) { currentTagMode = (currentTagMode == TAGMODE_QIDI) ? TAGMODE_OPENSPOOL : TAGMODE_QIDI; defaultTagMode = currentTagMode; saveAllSetupPreferences(); normalizeSetupPage(); reloadModeDatabases(); if (currentTagMode == TAGMODE_OPENSPOOL) { if (!osDraftsInitialized) initOpenSpoolDrafts(); loadOpenSpoolDraft(openSpoolProfileU1); } syncOpenSpoolColorFromSelection(); readOpenSpoolDetailsVisible = false; readPopupVisible = false; readResultPending = false; readPopupMisses = 0; autoPanelVisible = false; autoLastMat = autoLastCol = autoLastMfg = 0xFF; autoLastOsUidLen = 0; memset(autoLastOsUid, 0, sizeof(autoLastOsUid)); needRedraw = true; return; }
    return;
  }

  if (ui == UI_SETUP) {
    normalizeSetupPage();
    const uint8_t setupPages = getSetupPageCount();
    if (hit(8, 40, 88, 28, x, y)) { ui = UI_MAIN; setupPage = 0; needRedraw = true; return; }
    if (hit(TFT_W - 64, 40, 56, 28, x, y)) { setupPage = (setupPage + 1) % setupPages; needRedraw = true; return; }
    if (setupPage == 0) {
      bool qidiLocked = isOfficialListActiveForCurrentQidiModel();
      if (!qidiLocked) {
        if (hit(36, 78, 115, 34, x, y)) { ui = UI_MFG_MENU; needRedraw = true; return; }
        if (hit(169, 78, 115, 34, x, y)) { ui = UI_MAT_MENU; needRedraw = true; return; }
      }
      if (hit(36, 122, 115, 34, x, y)) { ui = UI_LANG_SELECT; needRedraw = true; return; }
      if (hit(169, 122, 115, 34, x, y)) { defaultTagMode = (defaultTagMode == TAGMODE_QIDI) ? TAGMODE_OPENSPOOL : TAGMODE_QIDI; currentTagMode = defaultTagMode; saveAllSetupPreferences(); normalizeSetupPage(); reloadModeDatabases(); if (currentTagMode == TAGMODE_OPENSPOOL) { if (!osDraftsInitialized) initOpenSpoolDrafts(); loadOpenSpoolDraft(openSpoolProfileU1); } syncOpenSpoolColorFromSelection(); readOpenSpoolDetailsVisible = false; readPopupVisible = false; readResultPending = false; readPopupMisses = 0; autoPanelVisible = false; autoLastMat = autoLastCol = autoLastMfg = 0xFF; autoLastOsUidLen = 0; memset(autoLastOsUid, 0, sizeof(autoLastOsUid)); needRedraw = true; return; }
      if (currentTagMode == TAGMODE_QIDI && hit(36, 166, 248, 34, x, y)) {
        qidiPrinterModel = nextQidiPrinterModel(qidiPrinterModel);
        saveAllSetupPreferences();
        reloadModeDatabases();
        needRedraw = true;
        return;
      }
    } else if (setupPage == 1) {
      if (hit(36, 92, 248, 34, x, y)) { screensaverMode = (ScreensaverMode)(((uint8_t)screensaverMode + 1) % 5); saveAllSetupPreferences(); needRedraw = true; return; }
      if (hit(36, 144, 248, 34, x, y)) { cycleBrightness(); needRedraw = true; return; }
    } else if (setupPage == 2 && currentTagMode == TAGMODE_QIDI) {
      if (!sdAvailable) return;

      struct RowDef {
        int y;
        QidiPrinterModel model;
      };
      const RowDef rows[] = {
        {72,  QIDI_MODEL_Q2},
        {110, QIDI_MODEL_PLUS4},
        {148, QIDI_MODEL_MAX4}
      };

      for (uint8_t i = 0; i < (sizeof(rows) / sizeof(rows[0])); i++) {
        if (hit(20, rows[i].y, 200, 32, x, y)) {
          showQidiCfgInfo(rows[i].model);
          return;
        }
        if (hit(228, rows[i].y, 72, 32, x, y)) {
          if (isOfficialListAvailable(rows[i].model)) {
            officialListEnabledRef(rows[i].model) = !officialListEnabledRef(rows[i].model);
            saveOfficialListFlags();
            reloadModeDatabases();
          } else {
            showQidiCfgInfo(rows[i].model);
            return;
          }
          needRedraw = true;
          return;
        }
      }

      if (hit(20, 188, 280, 24, x, y)) {
        ui = UI_SD_FORMAT_CONFIRM;
        needRedraw = true;
        return;
      }
    } else if (setupPage == 2) {
      if (hit(36, 112, 248, 34, x, y)) { osTagInfoConfigU1 = false; osTagInfoConfigPage = 0; ui = UI_OS_TAGINFO_CONFIG; needRedraw = true; return; }
      if (hit(36, 156, 248, 34, x, y)) { osTagInfoConfigU1 = true; osTagInfoConfigPage = 0; ui = UI_OS_TAGINFO_CONFIG; needRedraw = true; return; }
    } else if (setupPage == 3) {
      if (hit(21, 70, 278, 32, x, y)) {
        wifiEnabled = !wifiEnabled;
        saveWifiSettings();
        applyWifiState(true);
        needRedraw = true;
        return;
      }
      if (hit(21, 106, 278, 32, x, y)) { openKeyboardForBufferExtended(wifiSsid, sizeof(wifiSsid) - 1, UI_SETUP); return; }
      if (hit(21, 142, 278, 32, x, y)) { openKeyboardForBufferExtended(wifiPassword, sizeof(wifiPassword) - 1, UI_SETUP); return; }
      if (hit(21, 178, 278, 32, x, y)) {
        if (WiFi.status() == WL_CONNECTED) {
          char uploadUrl[32];
          buildWifiUploadUrl(uploadUrl, sizeof(uploadUrl));
          showSimpleMessage(LTXT(TXT_WIFI), String(uploadUrl), "", "", "", UI_SETUP);
        } else {
          showSimpleMessage(LTXT(TXT_WIFI), LTXT(TXT_WIFI_REQUIRED1), LTXT(TXT_WIFI_REQUIRED2), "", "", UI_SETUP);
        }
        return;
      }
    } else {
      if (hit(36, 78, 248, 34, x, y)) { displayInversionEnabled = !displayInversionEnabled; saveAllSetupPreferences(); applyDisplayInversion(); needRedraw = true; return; }
      if (hit(36, 122, 115, 34, x, y)) { calibrateTouch(); ui = UI_SETUP; needRedraw = true; return; }
      if (hit(169, 122, 115, 34, x, y)) { ui = UI_FACTORY_RESET_CONFIRM; needRedraw = true; return; }
    }
    return;
  }

  if (ui == UI_OS_TAGINFO_CONFIG) {
    if (hit(8, 40, 88, 28, x, y)) { ui = UI_SETUP; needRedraw = true; return; }
    if (osTagInfoConfigU1) {
      if (hit(TFT_W - 64, 40, 56, 28, x, y)) { osTagInfoConfigPage = (osTagInfoConfigPage + 1) % 2; needRedraw = true; return; }
      if (osTagInfoConfigPage == 0) {
        if (hit(24, 82, 272, 30, x, y)) { osInfoU1BedEnabled = !osInfoU1BedEnabled; saveAllSetupPreferences(); needRedraw = true; return; }
        if (hit(24, 118, 272, 30, x, y)) { osInfoU1AlphaEnabled = !osInfoU1AlphaEnabled; saveAllSetupPreferences(); needRedraw = true; return; }
        if (hit(24, 154, 272, 30, x, y)) { osInfoU1WeightEnabled = !osInfoU1WeightEnabled; saveAllSetupPreferences(); needRedraw = true; return; }
      } else {
        if (hit(24, 82, 272, 30, x, y)) { osInfoU1DiameterEnabled = !osInfoU1DiameterEnabled; saveAllSetupPreferences(); needRedraw = true; return; }
        if (hit(24, 118, 272, 30, x, y)) { osInfoU1AddColorsEnabled = !osInfoU1AddColorsEnabled; saveAllSetupPreferences(); needRedraw = true; return; }
        if (hit(24, 154, 272, 30, x, y)) { osReadIntervalSec = (osReadIntervalSec >= 4) ? 1 : (uint8_t)(osReadIntervalSec + 1); saveAllSetupPreferences(); needRedraw = true; return; }
      }
    } else {
      if (hit(24, 118, 272, 34, x, y)) { osInfoStdNozzleEnabled = !osInfoStdNozzleEnabled; saveAllSetupPreferences(); needRedraw = true; return; }
    }
    return;
  }

  if (ui == UI_LANG_SELECT) {
    if (hit(CP_SQ_X, 35, 80, 28, x, y)) { ui = UI_SETUP; needRedraw = true; return; }
    const int cols = 2, rows = 3, side = 12, gapX = 8, gapY = 8;
    const int top = 70, bottom = TFT_H - UI_STATUS_H - 10, availH = bottom - top;
    int btnH = (availH - (rows - 1) * gapY) / rows;
    if (btnH > 34) btnH = 34;
    if (btnH < 22) btnH = 22;
    const int totalH = rows * btnH + (rows - 1) * gapY;
    const int y0 = top + max(0, (availH - totalH) / 2);
    const int btnW = (TFT_W - 2 * side - (cols - 1) * gapX) / cols;

    int idx = 0;
    for (int r = 0; r < rows; r++) {
      for (int c = 0; c < cols; c++) {
        if (idx >= (int)LANG_COUNT) break;
        int bx = side + c * (btnW + gapX);
        int by = y0 + r * (btnH + gapY);
        if (hit(bx, by, btnW, btnH, x, y)) {
          uiLang = (UiLang)idx;
          saveAllSetupPreferences();
          needRedraw = true;
          return;
        }
        idx++;
      }
    }
    return;
  }

  if (ui == UI_READ) {
    if (hit(10, 50, 100, 40, x, y))  { ui = UI_MAIN; needRedraw = true; return; }
    if (hit(210, 50, 100, 40, x, y)) { ui = UI_MAIN; needRedraw = true; return; }
    return;
  }

  if (ui == UI_WRITE) {
    const int topY = UI_HEADER_H + 3;
    if (currentTagMode == TAGMODE_OPENSPOOL) {
      if (hit(8, topY, 80, 28, x, y)) {
        if (!osDraftsInitialized) initOpenSpoolDrafts();
        saveCurrentOpenSpoolDraft();
        if (openSpoolWritePage == 0) ui = UI_MAIN;
        else openSpoolWritePage = 0;
        needRedraw = true;
        return;
      }
    } else {
      if (hit(8, topY, 80, 28, x, y)) { ui = UI_MAIN; needRedraw = true; return; }
    }
    if (!(currentTagMode == TAGMODE_OPENSPOOL && openSpoolWritePage == 0) && hit(TFT_W - 118, topY, 110, 28, x, y)) { performWrite(); return; }

    if (currentTagMode == TAGMODE_QIDI) {
      const int btnX = 122;
      const int btnW = TFT_W - btnX - 12;
      if (hit(btnX, 78, btnW, 34, x, y)) { ui = UI_PICK_MFG; needRedraw = true; return; }
      if (hit(btnX, 122, btnW, 34, x, y)) { matPage = 0; ui = UI_PICK_MAT; needRedraw = true; return; }
      if (hit(btnX - 8, 166 - 8, btnW + 16, 38 + 16, x, y)) { ui = UI_PICK_COLOR; needRedraw = true; return; }
      return;
    }

    uint8_t osPages = getOpenSpoolWritePageCount();
    uint8_t osDisplayPages = getOpenSpoolDisplayPageCount();
    OpenSpoolWritePageKind pageKind = getOpenSpoolWritePageKind(openSpoolWritePage);
    if (osDisplayPages > 1 && openSpoolWritePage > 0) {
      if (hit(96, 35, 42, 28, x, y)) { openSpoolWritePage = (openSpoolWritePage + osPages - 1) % osPages; if (openSpoolWritePage == 0) openSpoolWritePage = osPages - 1; needRedraw = true; return; }
      if (hit(178, 35, 42, 28, x, y)) { openSpoolWritePage = (openSpoolWritePage + 1) % osPages; if (openSpoolWritePage == 0) openSpoolWritePage = 1; needRedraw = true; return; }
    }

    if (openSpoolWritePage == 0) {
      const int btnX = 16;
      const int btnW = TFT_W - 32;
      const int btnH = 38;
      const int gapY = 10;
      int y1 = topY + 28 + 14;
      int y2 = y1 + btnH + gapY;
      int y3 = y2 + btnH + gapY;
      if (!osDraftsInitialized) initOpenSpoolDrafts();
      if (hit(btnX, y1, btnW, btnH, x, y)) { saveCurrentOpenSpoolDraft(); loadOpenSpoolDraft(false); needRedraw = true; return; }
      if (hit(btnX, y2, btnW, btnH, x, y)) { saveCurrentOpenSpoolDraft(); loadOpenSpoolDraft(true); needRedraw = true; return; }
      if (hit(btnX, y3, btnW, btnH, x, y)) {
        bool restoreU1 = openSpoolProfileU1;
        resetOpenSpoolFieldsToDefault();
        openSpoolProfileU1 = restoreU1;
        saveCurrentOpenSpoolDraft();
        loadOpenSpoolDraft(restoreU1);
        needRedraw = true;
        return;
      }
      return;
    } else if (pageKind == OS_PAGE_BASE) {
      const int btnX = 122;
      const int btnW = TFT_W - btnX - 12;
      if (hit(btnX, 78, btnW, 34, x, y)) { ui = UI_PICK_MFG; needRedraw = true; return; }
      if (hit(btnX, 122, btnW, 34, x, y)) { matPage = 0; ui = UI_PICK_MAT; needRedraw = true; return; }
      if (hit(btnX - 8, 166 - 8, btnW + 16, 38 + 16, x, y)) { ui = UI_PICK_COLOR; needRedraw = true; return; }
      return;
    } else if (pageKind == OS_PAGE_STD_NOZZLE) {
      if (hit(18, 140, 140, 26, x, y)) { openKeyboardForBufferNumeric(osMinTemp, sizeof(osMinTemp)-1, UI_WRITE); return; }
      if (hit(162, 140, 140, 26, x, y)) { openKeyboardForBufferNumeric(osMaxTemp, sizeof(osMaxTemp)-1, UI_WRITE); return; }
      return;
    } else if (pageKind == OS_PAGE_U1_CORE) {
      if (hit(18, 92, 284, 26, x, y)) { pickSubtypePage = 0; ui = UI_PICK_SUBTYPE; needRedraw = true; return; }
      if (hit(18, 140, 140, 26, x, y)) { openKeyboardForBufferNumeric(osMinTemp, sizeof(osMinTemp)-1, UI_WRITE); return; }
      if (hit(162, 140, 140, 26, x, y)) { openKeyboardForBufferNumeric(osMaxTemp, sizeof(osMaxTemp)-1, UI_WRITE); return; }
      if (osInfoU1BedEnabled) {
        if (hit(18, 188, 140, 26, x, y)) { openKeyboardForBufferNumeric(osBedMinTemp, sizeof(osBedMinTemp)-1, UI_WRITE); return; }
        if (hit(162, 188, 140, 26, x, y)) { openKeyboardForBufferNumeric(osBedMaxTemp, sizeof(osBedMaxTemp)-1, UI_WRITE); return; }
      }
      return;
    } else if (pageKind == OS_PAGE_U1_ALPHA) {
      const int sliderX = 28;
      const int sliderY = 104;
      const int sliderW = 264;
      const int sliderH = 14;
      const int sliderTouchPad = 16;
      if (hit(sliderX - sliderTouchPad, sliderY - sliderTouchPad, sliderW + sliderTouchPad * 2, sliderH + sliderTouchPad * 2, x, y)) {
        int clampedX = constrain(x, sliderX, sliderX + sliderW - 1);
        uint8_t nextAlpha = (uint8_t)((((int32_t)(clampedX - sliderX) * 255L) + ((sliderW - 2) / 2)) / max(1, sliderW - 1));
        uint8_t currentAlpha = 0xFF;
        parseAlphaByte(osAlpha, currentAlpha);
        if (nextAlpha != currentAlpha) {
          setAlphaFromByte(nextAlpha);
          drawOpenSpoolAlphaPage(false);
        }
        return;
      }
      if (hit(18, 172, 284, 26, x, y)) { openKeyboardForBufferHex(osAlpha, 2, UI_WRITE); return; }
      return;
    } else if (pageKind == OS_PAGE_U1_WEIGHT) {
      int rowY = 76;
      if (osInfoU1WeightEnabled) {
        if (hit(18, rowY + 16, 284, 26, x, y)) { openKeyboardForBufferNumeric(osWeight, sizeof(osWeight)-1, UI_WRITE); return; }
        rowY += 48;
      }
      if (osInfoU1DiameterEnabled) {
        if (hit(18, rowY + 16, 284, 26, x, y)) {
          safeCopy(osDiameter, (String(osDiameter) == "2.85") ? "1.75" : "2.85", sizeof(osDiameter));
          needRedraw = true;
          return;
        }
      }
      return;
    } else if (pageKind == OS_PAGE_U1_EXTRA) {
      if (hit(18, 92, 140, 26, x, y)) { osColorEditTarget = osAddColor1; ui = UI_PICK_COLOR; needRedraw = true; return; }
      if (hit(162, 92, 140, 26, x, y)) { osColorEditTarget = osAddColor2; ui = UI_PICK_COLOR; needRedraw = true; return; }
      if (hit(18, 140, 140, 26, x, y)) { osColorEditTarget = osAddColor3; ui = UI_PICK_COLOR; needRedraw = true; return; }
      if (hit(162, 140, 140, 26, x, y)) { osColorEditTarget = osAddColor4; ui = UI_PICK_COLOR; needRedraw = true; return; }
      return;
    } else {
      return;
    }
  }

  if (ui == UI_PICK_MFG) {
    if (hit(CP_SQ_X, 35, 80, 28, x, y)) { osColorEditTarget = nullptr; ui = UI_WRITE; needRedraw = true; return; }
    int total = getAllManufacturerCount();
    int pages = max(1, (total + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE);
    if (hit(TFT_W - 120, 33, 52, 34, x, y)) { pickMfgPage--; if (pickMfgPage < 0) pickMfgPage = pages - 1; needRedraw = true; return; }
    if (hit(TFT_W - 60, 33, 52, 34, x, y))  { pickMfgPage = (pickMfgPage + 1) % pages; needRedraw = true; return; }

    const int cols = 2, rows = 4, w = 146, h = 32, gapX = 12, gapY = 6, x0 = 8, y0 = 70;
    int startIdx = pickMfgPage * ITEMS_PER_PAGE;
    int idx = startIdx;
    for (int r = 0; r < rows && idx < total; r++) {
      for (int c = 0; c < cols && idx < total; c++) {
        if (hit(x0 + c * (w + gapX), y0 + r * (h + gapY), w, h, x, y)) {
          selMfg = getAllManufacturerByIndex(idx);
          ui = UI_WRITE;
          needRedraw = true;
          return;
        }
        idx++;
      }
    }
    return;
  }

  if (ui == UI_PICK_MAT) {
    if (hit(CP_SQ_X, 35, 80, 28, x, y)) { ui = UI_WRITE; needRedraw = true; return; }
    int total = getActiveMaterialCount();
    int pages = max(1, (total + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE);
    if (hit(TFT_W - 120, 33, 52, 34, x, y)) { matPage--; if (matPage < 0) matPage = pages - 1; needRedraw = true; return; }
    if (hit(TFT_W - 60, 33, 52, 34, x, y))  { matPage = (matPage + 1) % pages; needRedraw = true; return; }

    const int cols = 2, rows = 4, w = 146, h = 32, gapX = 12, gapY = 6, x0 = 8, y0 = 70;
    int startIdx = matPage * ITEMS_PER_PAGE;
    int idx = startIdx;
    for (int r = 0; r < rows && idx < total; r++) {
      for (int c = 0; c < cols && idx < total; c++) {
        if (hit(x0 + c * (w + gapX), y0 + r * (h + gapY), w, h, x, y)) {
          selMatVal = getActiveMaterialByIndex(idx);
          if (currentTagMode == TAGMODE_OPENSPOOL) {
            applyOpenSpoolMaterialPreset();
            if (!String(osDiameter).length()) safeCopy(osDiameter, "1.75", sizeof(osDiameter));
          }
          ui = UI_WRITE;
          needRedraw = true;
          return;
        }
        idx++;
      }
    }
    return;
  }

  if (ui == UI_PICK_COLOR) {
    if (hit(CP_SQ_X, 35, 80, 28, x, y)) { ui = UI_WRITE; needRedraw = true; return; }
    if (currentTagMode == TAGMODE_OPENSPOOL && hit(232, 35, 80, 28, x, y)) {
      cpReturnState = UI_WRITE;
      const char* srcHex = osColorEditTarget ? osColorEditTarget : osColorHex;
      initColorPickerFromHex(srcHex);
      ui = UI_COLOR_PICKER;
      needRedraw = true;
      return;
    }
    const int cols = 6, rows = 4, boxW = 46, boxH = 28, gapX = 5, gapY = 7, x0 = 8, y0 = 72;
    int idx = 0;
    for (int r = 0; r < rows && idx < (int)COLORS_COUNT; r++) {
      for (int c = 0; c < cols && idx < (int)COLORS_COUNT; c++) {
        if (hit(x0 + c * (boxW + gapX), y0 + r * (boxH + gapY), boxW, boxH, x, y)) {
          selColIdx = idx;
          if (osColorEditTarget) {
            String hx = colorHexById(COLORS[idx].id, true);
            safeCopy(osColorEditTarget, hx.c_str(), 8);
            osColorEditTarget = nullptr;
          } else {
            syncOpenSpoolColorFromSelection();
          }
          ui = UI_WRITE;
          needRedraw = true;
          return;
        }
        idx++;
      }
    }
    return;
  }

  if (ui == UI_COLOR_PICKER) {
    // Back button
    if (hit(CP_SQ_X, 35, 80, 28, x, y)) {
      osColorEditTarget = nullptr;
      ui = UI_PICK_COLOR;
      needRedraw = true;
      return;
    }
    // OK button – apply picked color
    if (hit(220, 35, 80, 28, x, y)) {
      if (!cpHexExactLocked) syncCpHex();
      if (osColorEditTarget) {
        safeCopy(osColorEditTarget, cpHex, 8);
        osColorEditTarget = nullptr;
      } else {
        safeCopy(osColorHex, cpHex, sizeof(osColorHex));
        syncSelectionFromOpenSpoolColor();
      }
      ui = cpReturnState;
      needRedraw = true;
      return;
    }
    // Editable HEX field
    if (hit(CP_PRV_X, CP_PRV_Y + CP_PRV_H + 4, CP_PRV_W, 18, x, y)) {
      String cur = normalizeHexColor(cpHex, false);
      safeCopy(cpHexEdit, cur.c_str(), sizeof(cpHexEdit));
      cpHexEditActive = true;
      openKeyboardForBufferHex(cpHexEdit, sizeof(cpHexEdit) - 1, UI_COLOR_PICKER);
      needRedraw = true;
      return;
    }
    // HSV square touch – adjust Saturation & Value
    if (hit(CP_SQ_X, CP_SQ_Y, CP_SQ_W, CP_SQ_H, x, y)) {
      cpSat = (uint8_t)constrain((int)((float)(x - CP_SQ_X) / (CP_SQ_W - 1) * 255.0f), 0, 255);
      cpVal = (uint8_t)constrain(255 - (int)((float)(y - CP_SQ_Y) / (CP_SQ_H - 1) * 255.0f), 0, 255);
      syncCpHex();
      drawCpSquare();
      drawCpPreview();
      return;
    }
    // Hue bar touch – adjust Hue (extended hit area ±8px vertically)
    if (hit(CP_HUE_X, CP_HUE_Y - 8, CP_HUE_W, CP_HUE_H + 16, x, y)) {
      cpHue = (uint16_t)constrain((int)(((int32_t)(x - CP_HUE_X) * 359L) / max(1, CP_HUE_W - 1)), 0, 359);
      syncCpHex();
      drawCpSquare();
      drawCpHueBar();
      drawCpPreview();
      return;
    }
    return;
  }

  if (ui == UI_PICK_SUBTYPE) {
    if (hit(CP_SQ_X, 35, 80, 28, x, y)) { ui = UI_WRITE; needRedraw = true; return; }
    if (hit(TFT_W - 120, 35, 52, 28, x, y)) { if (pickSubtypePage > 0) pickSubtypePage--; needRedraw = true; return; }
    if (hit(TFT_W - 60, 35, 52, 28, x, y)) {
      const int itemsPerPage = 6; // cols=2, rows=3
      int total = 1 + getActiveVariantCount();
      int pages = max(1, (total + itemsPerPage - 1) / itemsPerPage);
      if (pickSubtypePage < pages - 1) pickSubtypePage++;
      needRedraw = true;
      return;
    }
    // Match draw: cols=2, rows=3, w=146, h=34, gapX=12, gapY=10, x0=8, y0=78
    const int cols = 2, rows = 3, w = 146, h = 34, gapX = 12, gapY = 10, x0 = 8, y0 = 78;
    const int itemsPerPage = cols * rows;
    int total = 1 + getActiveVariantCount();
    int idx = pickSubtypePage * itemsPerPage;
    for (int r = 0; r < rows && idx < total; r++) {
      for (int c = 0; c < cols && idx < total; c++) {
        if (hit(x0 + c*(w+gapX), y0 + r*(h+gapY), w, h, x, y)) {
          if (idx == 0) osSubtype[0] = '\0';
          else safeCopy(osSubtype, variantNameByVal(getActiveVariantByIndex(idx - 1)).c_str(), sizeof(osSubtype));
          ui = UI_WRITE;
          needRedraw = true;
          return;
        }
        idx++;
      }
    }
    return;
  }

  if (ui == UI_MAT_MENU) {
    if (hit(CP_SQ_X, 35, 80, 28, x, y)) { ui = UI_SETUP; needRedraw = true; return; }
    if (currentTagMode == TAGMODE_QIDI && isOfficialListActiveForCurrentQidiModel()) return;
    if (currentTagMode == TAGMODE_OPENSPOOL) {
      if (hit(TFT_W - 88, 35, 80, 28, x, y)) { matMenuPage = (matMenuPage + 1) % 2; needRedraw = true; return; }
      if (matMenuPage == 0) {
        if (hit(40, 68, 240, 34, x, y)) { matListPage = 0; ui = UI_MAT_EDIT_LIST; needRedraw = true; return; }
        if (hit(40, 110, 240, 34, x, y)) { matFreePage = 0; ui = UI_MAT_ADD_LIST; needRedraw = true; return; }
        if (hit(40, 152, 240, 34, x, y)) { ui = UI_MAT_RESET_CONFIRM; needRedraw = true; return; }
      } else {
        if (hit(40, 68, 240, 34, x, y)) { varListPage = 0; ui = UI_VAR_EDIT_LIST; needRedraw = true; return; }
        if (hit(40, 110, 240, 34, x, y)) { varFreePage = 0; ui = UI_VAR_ADD_LIST; needRedraw = true; return; }
        if (hit(40, 152, 240, 34, x, y)) { ui = UI_VAR_RESET_CONFIRM; needRedraw = true; return; }
      }
    } else {
      if (hit(40, 68, 240, 34, x, y)) { matListPage = 0; ui = UI_MAT_EDIT_LIST; needRedraw = true; return; }
      if (hit(40, 110, 240, 34, x, y)) { matFreePage = 0; ui = UI_MAT_ADD_LIST; needRedraw = true; return; }
      if (hit(40, 152, 240, 34, x, y)) { ui = UI_MAT_RESET_CONFIRM; needRedraw = true; return; }
    }
    return;
  }

  if (ui == UI_MAT_EDIT_LIST) {
    if (hit(CP_SQ_X, 35, 80, 28, x, y)) { ui = UI_MAT_MENU; needRedraw = true; return; }
    int total = getActiveMaterialCount();
    int pages = getPageCountForTotal(total);
    if (pages > 1 && hit(TFT_W - 88, 35, 80, 28, x, y)) { matListPage = (matListPage + 1) % pages; needRedraw = true; return; }

    const int cols = 2, rows = 4, w = 146, h = 32, gapX = 12, gapY = 6, x0 = 8, y0 = 70;
    int startIdx = matListPage * ITEMS_PER_PAGE;
    int idx = startIdx;
    for (int r = 0; r < rows && idx < total; r++) {
      for (int c = 0; c < cols && idx < total; c++) {
        if (hit(x0 + c * (w + gapX), y0 + r * (h + gapY), w, h, x, y)) {
          editMatVal = getActiveMaterialByIndex(idx);
          safeCopy(editMatName, gMaterials[editMatVal].name, sizeof(editMatName));
          if (gMaterials[editMatVal].nozzleMin > 0) snprintf(editMatMin, sizeof(editMatMin), "%u", gMaterials[editMatVal].nozzleMin); else editMatMin[0] = '\0';
          if (gMaterials[editMatVal].nozzleMax > 0) snprintf(editMatMax, sizeof(editMatMax), "%u", gMaterials[editMatVal].nozzleMax); else editMatMax[0] = '\0';
          if (gMaterials[editMatVal].bedMin > 0) snprintf(editMatBedMin, sizeof(editMatBedMin), "%u", gMaterials[editMatVal].bedMin); else editMatBedMin[0] = '\0';
          if (gMaterials[editMatVal].bedMax > 0) snprintf(editMatBedMax, sizeof(editMatBedMax), "%u", gMaterials[editMatVal].bedMax); else editMatBedMax[0] = '\0';
          ui = UI_MAT_EDIT_DETAIL;
          needRedraw = true;
          return;
        }
        idx++;
      }
    }
    return;
  }

  if (ui == UI_MAT_EDIT_DETAIL) {
    if (hit(CP_SQ_X, 35, 80, 28, x, y)) { ui = UI_MAT_EDIT_LIST; needRedraw = true; return; }
    if (hit(120, 35, 80, 28, x, y)) { ui = UI_MAT_DELETE_CONFIRM; needRedraw = true; return; }
    if (hit(TFT_W - 88, 35, 80, 28, x, y)) {
      if (currentTagMode == TAGMODE_OPENSPOOL) {
        if (!openSpoolMaterialFieldsValid(editMatName, editMatMin, editMatMax, editMatBedMin, editMatBedMax, true)) {
          showSimpleMessage(LTXT(TXT_MATERIAL_EDIT), LTXT(TXT_ALL_FIELDS_REQUIRED1), LTXT(TXT_ALL_FIELDS_REQUIRED2), "", "", UI_MAT_EDIT_DETAIL);
          return;
        }
        gMaterials[editMatVal].active = true;
        safeCopy(gMaterials[editMatVal].name, editMatName, sizeof(gMaterials[editMatVal].name));
        gMaterials[editMatVal].nozzleMin = (uint16_t)atoi(editMatMin);
        gMaterials[editMatVal].nozzleMax = (uint16_t)atoi(editMatMax);
        gMaterials[editMatVal].bedMin = (uint16_t)atoi(editMatBedMin);
        gMaterials[editMatVal].bedMax = (uint16_t)atoi(editMatBedMax);
        saveMaterialToPrefs(editMatVal);
        ensureSelectedMaterialValid();
        if (selMatVal == editMatVal) applyOpenSpoolMaterialPreset();
        showNotice(NOTICE_MATERIAL, UI_MAT_MENU);
      } else {
        if (strlen(editMatName) > 0 && strcmp(gMaterials[editMatVal].name, editMatName) != 0) {
          gMaterials[editMatVal].active = true;
          safeCopy(gMaterials[editMatVal].name, editMatName, sizeof(gMaterials[editMatVal].name));
          saveMaterialToPrefs(editMatVal);
          ensureSelectedMaterialValid();
          showNotice(NOTICE_MATERIAL, UI_MAT_MENU);
        } else {
          ui = UI_MAT_EDIT_LIST;
          needRedraw = true;
        }
      }
      return;
    }
    if (currentTagMode == TAGMODE_OPENSPOOL) {
      if (hit(120, 66, 180, 28, x, y)) { openKeyboardForBufferExtended(editMatName, ITEM_NAME_MAX, UI_MAT_EDIT_DETAIL); return; }
      if (hit(18, 132, 132, 26, x, y)) { openKeyboardForBufferNumeric(editMatMin, 3, UI_MAT_EDIT_DETAIL); return; }
      if (hit(170, 132, 132, 26, x, y)) { openKeyboardForBufferNumeric(editMatMax, 3, UI_MAT_EDIT_DETAIL); return; }
      if (hit(18, 188, 132, 22, x, y)) { openKeyboardForBufferNumeric(editMatBedMin, 3, UI_MAT_EDIT_DETAIL); return; }
      if (hit(170, 188, 132, 22, x, y)) { openKeyboardForBufferNumeric(editMatBedMax, 3, UI_MAT_EDIT_DETAIL); return; }
    } else {
      if (hit(70, 160, 180, 34, x, y)) { openKeyboardForBufferExtended(editMatName, ITEM_NAME_MAX, UI_MAT_EDIT_DETAIL); return; }
    }
    return;
  }

  if (ui == UI_MAT_ADD_LIST) {
    if (hit(CP_SQ_X, 35, 80, 28, x, y)) { ui = UI_MAT_MENU; needRedraw = true; return; }
    int total = getFreeMaterialCount();
    int pages = getPageCountForTotal(total);
    if (pages > 1 && hit(TFT_W - 88, 35, 80, 28, x, y)) { matFreePage = (matFreePage + 1) % pages; needRedraw = true; return; }

    const int cols = 2, rows = 4, w = 146, h = 32, gapX = 12, gapY = 6, x0 = 8, y0 = 70;
    int startIdx = matFreePage * ITEMS_PER_PAGE;
    int idx = startIdx;
    for (int r = 0; r < rows && idx < total; r++) {
      for (int c = 0; c < cols && idx < total; c++) {
        if (hit(x0 + c * (w + gapX), y0 + r * (h + gapY), w, h, x, y)) {
          addMatVal = getFreeMaterialByIndex(idx);
          addMatName[0] = '\0';
          addMatMin[0] = '\0';
          addMatMax[0] = '\0';
          addMatBedMin[0] = '\0';
          addMatBedMax[0] = '\0';
          ui = UI_MAT_ADD_DETAIL;
          needRedraw = true;
          return;
        }
        idx++;
      }
    }
    return;
  }

  if (ui == UI_MAT_ADD_DETAIL) {
    if (hit(CP_SQ_X, 35, 80, 28, x, y)) { ui = UI_MAT_ADD_LIST; needRedraw = true; return; }
    if (hit(TFT_W - 88, 35, 80, 28, x, y)) {
      if (currentTagMode == TAGMODE_OPENSPOOL) {
        if (!openSpoolMaterialFieldsValid(addMatName, addMatMin, addMatMax, addMatBedMin, addMatBedMax, true)) {
          showSimpleMessage(LTXT(TXT_MATERIAL_NEW), LTXT(TXT_ALL_FIELDS_REQUIRED1), LTXT(TXT_ALL_FIELDS_REQUIRED2), "", "", UI_MAT_ADD_DETAIL);
          return;
        }
        gMaterials[addMatVal].active = true;
        safeCopy(gMaterials[addMatVal].name, addMatName, sizeof(gMaterials[addMatVal].name));
        gMaterials[addMatVal].nozzleMin = (uint16_t)atoi(addMatMin);
        gMaterials[addMatVal].nozzleMax = (uint16_t)atoi(addMatMax);
        gMaterials[addMatVal].bedMin = (uint16_t)atoi(addMatBedMin);
        gMaterials[addMatVal].bedMax = (uint16_t)atoi(addMatBedMax);
        saveMaterialToPrefs(addMatVal);
        if (!gMaterials[selMatVal].active) selMatVal = addMatVal;
        showNotice(NOTICE_MATERIAL, UI_MAT_MENU);
      } else {
        if (strlen(addMatName) > 0) {
          gMaterials[addMatVal].active = true;
          safeCopy(gMaterials[addMatVal].name, addMatName, sizeof(gMaterials[addMatVal].name));
          saveMaterialToPrefs(addMatVal);
          if (!gMaterials[selMatVal].active) selMatVal = addMatVal;
          showNotice(NOTICE_MATERIAL, UI_MAT_MENU);
        } else {
          drawStatus(LTXT(TXT_NAME_REQUIRED), TFT_RED);
        }
      }
      return;
    }
    if (currentTagMode == TAGMODE_OPENSPOOL) {
      if (hit(120, 66, 180, 28, x, y)) { openKeyboardForBufferExtended(addMatName, ITEM_NAME_MAX, UI_MAT_ADD_DETAIL); return; }
      if (hit(18, 132, 132, 26, x, y)) { openKeyboardForBufferNumeric(addMatMin, 3, UI_MAT_ADD_DETAIL); return; }
      if (hit(170, 132, 132, 26, x, y)) { openKeyboardForBufferNumeric(addMatMax, 3, UI_MAT_ADD_DETAIL); return; }
      if (hit(18, 188, 132, 22, x, y)) { openKeyboardForBufferNumeric(addMatBedMin, 3, UI_MAT_ADD_DETAIL); return; }
      if (hit(170, 188, 132, 22, x, y)) { openKeyboardForBufferNumeric(addMatBedMax, 3, UI_MAT_ADD_DETAIL); return; }
    } else {
      if (hit(70, 160, 180, 34, x, y)) { openKeyboardForBufferExtended(addMatName, ITEM_NAME_MAX, UI_MAT_ADD_DETAIL); return; }
    }
    return;
  }

  if (ui == UI_VAR_EDIT_LIST) {
    int total = getActiveVariantCount();
    int pages = getPageCountForTotal(total);
    if (hit(CP_SQ_X, 35, 80, 28, x, y)) { ui = UI_MAT_MENU; needRedraw = true; return; }
    if (pages > 1 && hit(TFT_W - 88, 35, 80, 28, x, y)) { varListPage = (varListPage + 1) % pages; needRedraw = true; return; }
    const int cols = 2, rows = 4, w = 146, h = 32, gapX = 12, gapY = 6, x0 = 8, y0 = 70;
    int startIdx = varListPage * ITEMS_PER_PAGE;
    int idx = startIdx;
    for (int r = 0; r < rows && idx < total; r++) {
      for (int c = 0; c < cols && idx < total; c++) {
        int bx = x0 + c * (w + gapX), by = y0 + r * (h + gapY);
        if (hit(bx, by, w, h, x, y)) {
          editVarVal = getActiveVariantByIndex(idx);
          safeCopy(editVarName, gVariants[editVarVal].name, sizeof(editVarName));
          ui = UI_VAR_EDIT_DETAIL;
          needRedraw = true;
          return;
        }
        idx++;
      }
    }
    return;
  }

  if (ui == UI_VAR_EDIT_DETAIL) {
    if (hit(CP_SQ_X, 35, 80, 28, x, y)) { ui = UI_VAR_EDIT_LIST; needRedraw = true; return; }
    if (hit(120, 35, 80, 28, x, y)) { ui = UI_VAR_DELETE_CONFIRM; needRedraw = true; return; }
    if (hit(TFT_W - 88, 35, 80, 28, x, y)) {
      if (strlen(editVarName) > 0 && strcmp(gVariants[editVarVal].name, editVarName) != 0) {
        bool wasSelected = (strcmp(osSubtype, gVariants[editVarVal].name) == 0);
        gVariants[editVarVal].active = true;
        safeCopy(gVariants[editVarVal].name, editVarName, sizeof(gVariants[editVarVal].name));
        saveVariantToPrefs(editVarVal);
        if (wasSelected) safeCopy(osSubtype, editVarName, sizeof(osSubtype));
      }
      ui = UI_VAR_EDIT_LIST;
      needRedraw = true;
      return;
    }
      if (hit(70, 160, 180, 34, x, y)) { openKeyboardForBufferExtended(editVarName, ITEM_NAME_MAX, UI_VAR_EDIT_DETAIL); return; }
    return;
  }

  if (ui == UI_VAR_ADD_LIST) {
    int total = getFreeVariantCount();
    int pages = getPageCountForTotal(total);
    if (hit(CP_SQ_X, 35, 80, 28, x, y)) { ui = UI_MAT_MENU; needRedraw = true; return; }
    if (pages > 1 && hit(TFT_W - 88, 35, 80, 28, x, y)) { varFreePage = (varFreePage + 1) % pages; needRedraw = true; return; }
    const int cols = 2, rows = 4, w = 146, h = 32, gapX = 12, gapY = 6, x0 = 8, y0 = 70;
    int startIdx = varFreePage * ITEMS_PER_PAGE;
    int idx = startIdx;
    for (int r = 0; r < rows && idx < total; r++) {
      for (int c = 0; c < cols && idx < total; c++) {
        int bx = x0 + c * (w + gapX), by = y0 + r * (h + gapY);
        if (hit(bx, by, w, h, x, y)) {
          addVarVal = getFreeVariantByIndex(idx);
          addVarName[0] = '\0';
          ui = UI_VAR_ADD_DETAIL;
          needRedraw = true;
          return;
        }
        idx++;
      }
    }
    return;
  }

  if (ui == UI_VAR_ADD_DETAIL) {
    if (hit(CP_SQ_X, 35, 80, 28, x, y)) { ui = UI_VAR_ADD_LIST; needRedraw = true; return; }
    if (hit(TFT_W - 88, 35, 80, 28, x, y)) {
      if (strlen(addVarName) > 0) {
        gVariants[addVarVal].active = true;
        safeCopy(gVariants[addVarVal].name, addVarName, sizeof(gVariants[addVarVal].name));
        saveVariantToPrefs(addVarVal);
      }
      ui = UI_VAR_ADD_LIST;
      needRedraw = true;
      return;
    }
      if (hit(70, 160, 180, 34, x, y)) { openKeyboardForBufferExtended(addVarName, ITEM_NAME_MAX, UI_VAR_ADD_DETAIL); return; }
    return;
  }

  if (ui == UI_MAT_RESET_CONFIRM) {
    if (hit(45, 160, 90, 34, x, y)) { resetMaterialsToDefault(); ui = UI_MAT_MENU; needRedraw = true; return; }
    if (hit(185, 160, 90, 34, x, y)) { ui = UI_MAT_MENU; needRedraw = true; return; }
    return;
  }

  if (ui == UI_MAT_DELETE_CONFIRM) {
    if (hit(45, 160, 90, 34, x, y)) { ui = UI_MAT_EDIT_DETAIL; needRedraw = true; return; }
    if (hit(185, 160, 90, 34, x, y)) {
      if (editMatVal >= 1 && editMatVal <= MAX_MATERIALS) {
        bool wasSelected = (selMatVal == editMatVal);
        gMaterials[editMatVal].active = false;
        gMaterials[editMatVal].name[0] = '\0';
        gMaterials[editMatVal].nozzleMin = 0;
        gMaterials[editMatVal].nozzleMax = 0;
        gMaterials[editMatVal].bedMin = 0;
        gMaterials[editMatVal].bedMax = 0;
        saveMaterialToPrefs(editMatVal);
        if (wasSelected) { ensureSelectedMaterialValid(); if (currentTagMode == TAGMODE_OPENSPOOL) applyOpenSpoolMaterialPreset(); }
      }
      ui = UI_MAT_EDIT_LIST; needRedraw = true; return;
    }
    return;
  }

  if (ui == UI_VAR_RESET_CONFIRM) {
    if (hit(45, 160, 90, 34, x, y)) { resetVariantsToDefault(); ui = UI_MAT_MENU; needRedraw = true; return; }
    if (hit(185, 160, 90, 34, x, y)) { ui = UI_MAT_MENU; needRedraw = true; return; }
    return;
  }

  if (ui == UI_VAR_DELETE_CONFIRM) {
    if (hit(45, 160, 90, 34, x, y)) { ui = UI_VAR_EDIT_DETAIL; needRedraw = true; return; }
    if (hit(185, 160, 90, 34, x, y)) {
      if (editVarVal >= 1 && editVarVal <= MAX_VARIANTS) {
        bool wasSelected = (strcmp(osSubtype, gVariants[editVarVal].name) == 0);
        gVariants[editVarVal].active = false;
        gVariants[editVarVal].name[0] = '\0';
        gVariants[editVarVal].nozzleMin = 0;
        gVariants[editVarVal].nozzleMax = 0;
        saveVariantToPrefs(editVarVal);
        if (wasSelected) {
          osSubtype[0] = '\0';
          for (uint8_t i = 1; i <= MAX_VARIANTS; i++) {
            if (gVariants[i].active) { safeCopy(osSubtype, gVariants[i].name, sizeof(osSubtype)); break; }
          }
        }
      }
      ui = UI_VAR_EDIT_LIST; needRedraw = true; return;
    }
    return;
  }

  if (ui == UI_MFG_MENU) {
    if (hit(CP_SQ_X, 35, 80, 28, x, y)) { ui = UI_SETUP; needRedraw = true; return; }
    if (currentTagMode == TAGMODE_QIDI && isOfficialListActiveForCurrentQidiModel()) return;
    if (hit(40, 68, 240, 34, x, y)) { mfgListPage = 0; ui = UI_MFG_EDIT_LIST; needRedraw = true; return; }
    if (hit(40, 110, 240, 34, x, y)) { mfgFreePage = 0; ui = UI_MFG_ADD_LIST; needRedraw = true; return; }
    if (hit(40, 152, 240, 34, x, y)) { ui = UI_MFG_RESET_CONFIRM; needRedraw = true; return; }
    return;
  }

  if (ui == UI_MFG_EDIT_LIST) {
    if (hit(CP_SQ_X, 35, 80, 28, x, y)) { ui = UI_MFG_MENU; needRedraw = true; return; }
    int total = getEditableManufacturerCount();
    int pages = getPageCountForTotal(total);
    if (pages > 1 && hit(TFT_W - 88, 35, 80, 28, x, y)) { mfgListPage = (mfgListPage + 1) % pages; needRedraw = true; return; }

    const int cols = 2, rows = 4, w = 146, h = 32, gapX = 12, gapY = 6, x0 = 8, y0 = 70;
    int startIdx = mfgListPage * ITEMS_PER_PAGE;
    int idx = startIdx;
    for (int r = 0; r < rows && idx < total; r++) {
      for (int c = 0; c < cols && idx < total; c++) {
        if (hit(x0 + c * (w + gapX), y0 + r * (h + gapY), w, h, x, y)) {
          editMfgVal = getEditableManufacturerByIndex(idx);
          safeCopy(editMfgName, gManufacturers[editMfgVal].name, sizeof(editMfgName));
          ui = UI_MFG_EDIT_DETAIL;
          needRedraw = true;
          return;
        }
        idx++;
      }
    }
    return;
  }

  if (ui == UI_MFG_EDIT_DETAIL) {
    if (hit(CP_SQ_X, 35, 80, 28, x, y)) { ui = UI_MFG_EDIT_LIST; needRedraw = true; return; }
    if (hit(120, 35, 80, 28, x, y)) { ui = UI_MFG_DELETE_CONFIRM; needRedraw = true; return; }
    if (hit(TFT_W - 88, 35, 80, 28, x, y)) {
      if (editMfgVal <= 1) {
        drawStatus(LTXT(TXT_FIXED_ITEMS), TFT_RED);
      } else if (strlen(editMfgName) > 0 && strcmp(gManufacturers[editMfgVal].name, editMfgName) != 0) {
        gManufacturers[editMfgVal].active = true;
        safeCopy(gManufacturers[editMfgVal].name, editMfgName, sizeof(gManufacturers[editMfgVal].name));
        saveManufacturerToPrefs(editMfgVal);
        showNotice(NOTICE_MANUFACTURER, UI_MFG_MENU);
      } else {
        ui = UI_MFG_EDIT_LIST;
        needRedraw = true;
      }
      return;
    }
    if (hit(70, 160, 180, 34, x, y)) {
      if (editMfgVal <= 1) {
        drawStatus(LTXT(TXT_FIXED_ITEMS), TFT_RED);
      } else {
        openKeyboardForBufferExtended(editMfgName, ITEM_NAME_MAX, UI_MFG_EDIT_DETAIL);
      }
      return;
    }
    return;
  }

  if (ui == UI_MFG_ADD_LIST) {
    if (hit(CP_SQ_X, 35, 80, 28, x, y)) { ui = UI_MFG_MENU; needRedraw = true; return; }
    int total = getFreeManufacturerCount();
    int pages = getPageCountForTotal(total);
    if (pages > 1 && hit(TFT_W - 88, 35, 80, 28, x, y)) { mfgFreePage = (mfgFreePage + 1) % pages; needRedraw = true; return; }

    const int cols = 2, rows = 4, w = 146, h = 32, gapX = 12, gapY = 6, x0 = 8, y0 = 70;
    int startIdx = mfgFreePage * ITEMS_PER_PAGE;
    int idx = startIdx;
    for (int r = 0; r < rows && idx < total; r++) {
      for (int c = 0; c < cols && idx < total; c++) {
        if (hit(x0 + c * (w + gapX), y0 + r * (h + gapY), w, h, x, y)) {
          addMfgVal = getFreeManufacturerByIndex(idx);
          addMfgName[0] = '\0';
          ui = UI_MFG_ADD_DETAIL;
          needRedraw = true;
          return;
        }
        idx++;
      }
    }
    return;
  }

  if (ui == UI_MFG_ADD_DETAIL) {
    if (hit(CP_SQ_X, 35, 80, 28, x, y)) { ui = UI_MFG_ADD_LIST; needRedraw = true; return; }
    if (hit(TFT_W - 88, 35, 80, 28, x, y)) {
      if (strlen(addMfgName) > 0) {
        gManufacturers[addMfgVal].active = true;
        safeCopy(gManufacturers[addMfgVal].name, addMfgName, sizeof(gManufacturers[addMfgVal].name));
        saveManufacturerToPrefs(addMfgVal);
        showNotice(NOTICE_MANUFACTURER, UI_MFG_MENU);
      } else {
        drawStatus(LTXT(TXT_NAME_REQUIRED), TFT_RED);
      }
      return;
    }
      if (hit(70, 160, 180, 34, x, y)) { openKeyboardForBufferExtended(addMfgName, ITEM_NAME_MAX, UI_MFG_ADD_DETAIL); return; }
    return;
  }

  if (ui == UI_MFG_RESET_CONFIRM) {
    if (hit(45, 160, 90, 34, x, y)) { resetManufacturersToDefault(); ui = UI_MFG_MENU; needRedraw = true; return; }
    if (hit(185, 160, 90, 34, x, y)) { ui = UI_MFG_MENU; needRedraw = true; return; }
    return;
  }

  if (ui == UI_MFG_DELETE_CONFIRM) {
    if (hit(45, 160, 90, 34, x, y)) { ui = UI_MFG_EDIT_DETAIL; needRedraw = true; return; }
    if (hit(185, 160, 90, 34, x, y)) {
      if (editMfgVal <= 1) {
        drawStatus(LTXT(TXT_FIXED_ITEMS), TFT_RED);
        return;
      } else if (editMfgVal <= MAX_MANUFACTURERS) {
        bool wasSelected = (selMfg == editMfgVal);
        gManufacturers[editMfgVal].active = false;
        gManufacturers[editMfgVal].name[0] = '\0';
        saveManufacturerToPrefs(editMfgVal);
        if (wasSelected || !gManufacturers[selMfg].active) selMfg = (currentTagMode == TAGMODE_OPENSPOOL ? MFG_GENERIC : MFG_QIDI);
      }
      ui = UI_MFG_EDIT_LIST; needRedraw = true; return;
    }
    return;
  }

  if (ui == UI_FACTORY_RESET_CONFIRM) {
    if (hit(45, 160, 90, 34, x, y)) { factoryResetSettings(); ui = UI_SETUP; needRedraw = true; return; }
    if (hit(185, 160, 90, 34, x, y)) { ui = UI_SETUP; needRedraw = true; return; }
    return;
  }

  if (ui == UI_SD_FORMAT_CONFIRM) {
    if (hit(45, 160, 90, 34, x, y)) {
      bool ok = formatSdCardStorage();
      if (ok) {
        showSimpleMessage((uiLang == LANG_DE) ? "SD-Karte" : "SD card",
                          (uiLang == LANG_DE) ? "Inhalt geloescht" : "Content deleted",
                          (uiLang == LANG_DE) ? "Ordner neu erstellt" : "Folders recreated",
                          "", "", UI_SETUP);
      } else {
        showSimpleMessage((uiLang == LANG_DE) ? "SD-Karte" : "SD card",
                          (uiLang == LANG_DE) ? "Formatieren fehlgeschlagen" : "Format failed",
                          "", "", "", UI_SETUP);
      }
      needRedraw = true;
      return;
    }
    if (hit(185, 160, 90, 34, x, y)) { ui = UI_SETUP; needRedraw = true; return; }
    return;
  }
}

// ==================== UI tick ====================
static void uiTick() {
  static bool touchDown = false;
  static int pressX = 0;
  static int pressY = 0;
  static uint32_t pressStartMs = 0;
  static uint32_t lastReleaseMs = 0;

  if (screensaverActive) {
    screensaverTick();
    return;
  }

  bool tagInfoVisible =
      (ui == UI_READ && readPopupVisible) ||
      (currentTagMode == TAGMODE_OPENSPOOL && ui == UI_MAIN && autoPanelVisible);
  if (tagInfoVisible) lastUserActivityMs = millis();

  uint32_t ssTimeout = screensaverTimeoutMs();
  if (!tagInfoVisible && ssTimeout > 0 && (millis() - lastUserActivityMs) >= ssTimeout) {
    screensaverActive = true;
    applyBrightnessPercent(25);
    chooseScreensaverPosition();
    drawScreensaver();
    lastScreensaverMoveMs = millis();
    return;
  }

  int x, y;
  bool isTouched = getTouchXY(x, y);

  if (isTouched) {
    if (!touchDown) {
      touchDown = true;
      pressX = x;
      pressY = y;
      pressStartMs = millis();
    } else {
      pressX = (pressX + x) / 2;
      pressY = (pressY + y) / 2;
    }
  } else if (touchDown) {
    uint32_t now = millis();
    uint32_t pressDuration = now - pressStartMs;
    if (pressDuration >= 25 && (now - lastReleaseMs) >= 140) {
      uiHandleTouch(pressX, pressY);
      noteUserActivity();
      lastReleaseMs = now;
    }
    touchDown = false;
  }

  autoDetectTick();
  if (currentTagMode == TAGMODE_OPENSPOOL && ((ui == UI_MAIN && autoPanelVisible) || (ui == UI_READ && readOpenSpoolDetailsVisible))) {
    tickOpenSpoolReadPaging();
  }
  readAutoReturnTick();
  uiRedrawIfNeeded();
}

// ==================== Arduino setup / loop ====================
void setup() {
  Serial.begin(115200);
  delay(300);

  loadLanguage();
  loadCalibration();
  pinMode(XPT2046_CS, OUTPUT);
  digitalWrite(XPT2046_CS, HIGH);
  initSdCard();
  loadMaterials();
  loadVariants();
  loadManufacturers();
  initOpenSpoolDrafts();
  currentTagMode = defaultTagMode;
  reloadModeDatabases();
  if (currentTagMode == TAGMODE_OPENSPOOL) {
    loadOpenSpoolDraft(openSpoolProfileU1);
  }
  syncOpenSpoolColorFromSelection();

  gNfcMutex = xSemaphoreCreateMutex();

  SD.end();
  sdSPI.end();
  sdReady = false;
  activeVspiOwner = VSPI_OWNER_UNKNOWN;

  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  ts.begin(touchscreenSPI);
  ts.setRotation(TFT_ROT);
  touchSpiReady = true;
  activeVspiOwner = VSPI_OWNER_TOUCH;

  tft.init();
  initBacklight();
  applyDisplayInversion();
  tft.setRotation(TFT_ROT);
  tft.fillScreen(TFT_BLACK);
  tft.setTextFont(2);
  tft.setTextSize(1);

  TFT_W = tft.width();
  TFT_H = tft.height();
  randomSeed(micros());

  Wire.begin(PN532_SDA, PN532_SCL);

  {
    NfcLock lock(2000);
    nfc.begin();
    uint32_t version = nfc.getFirmwareVersion();
    if (!version) {
      needRedraw = true;
      uiRedrawIfNeeded();
      drawStatus(TR(STR_PN532_NOT_FOUND), TFT_RED);
    } else {
      nfc.SAMConfig();
      delay(100);
    }
  }

  lastUserActivityMs = millis();
  applyWifiState(true);
  needRedraw = true;
}

void loop() {
  uiTick();
  handleWifiTasks();
  delay(5);
}
