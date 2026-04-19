/*
  BoxRFID BLE Bridge + Touch UI - ESP32-2432S028R (DIYMalls / CYD) + PN532 I2C
  ===========================================================================
  Version: V2.1

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
#include <TFT_eSPI.h>

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include <Preferences.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include <XPT2046_Touchscreen.h>

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

SPIClass touchscreenSPI = SPIClass(VSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);

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
static const char* PREF_LANG = "lang";
static const char* PREF_DISP_INV = "dinv";

// Material storage
static const char* PREF_NS_MAT = "matdb";

// Manufacturer storage
static const char* PREF_NS_MFG = "mfgdb";

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
static bool gDisplayInversion = false;

// ==================== Display ====================
static const uint8_t TFT_ROT = 1;
static int TFT_W = 320;
static int TFT_H = 240;

static const int UI_HEADER_H = 32;
static const int UI_STATUS_H = 24;

static const char* APP_VERSION = "V2.1";

// ==================== RFID data format ====================
static const uint8_t DATA_BLOCK = 4;

// ==================== Manufacturer IDs ====================
enum ManufacturerId : uint8_t {
  MFG_GENERIC = 0,
  MFG_QIDI    = 1
};

// ==================== Databases ====================
static const uint8_t MAX_MATERIALS = 50;
static const uint8_t MAX_MANUFACTURERS = 24;
static const uint8_t ITEM_NAME_MAX = 32;

struct RuntimeItem {
  bool active;
  char name[ITEM_NAME_MAX + 1];
};

static RuntimeItem gMaterials[MAX_MATERIALS + 1];          // 1..50
static RuntimeItem gManufacturers[MAX_MANUFACTURERS + 1];  // 0..24

struct DefaultMaterialItem {
  const char* name;
  uint8_t val;
};

static const DefaultMaterialItem DEFAULT_MATERIALS[] = {
  {"PLA", 1},
  {"PETG", 41},
  {"ABS", 11},
  {"ASA", 18},
  {"ABS-GF", 12},
  {"ABS-Metal", 13},
  {"ABS-Odorless", 14},
  {"ASA-AERO", 19},
  {"PA12-CF", 25},
  {"PAHT-CF", 30},
  {"PAHT-GF", 31},
  {"PC/ABS-FR", 34},
  {"PET-CF", 37},
  {"PET-GF", 38},
  {"PETG Basic", 39},
  {"PETG Translucent", 45},
  {"PETG-Though", 40},
  {"PLA Basic", 7},
  {"PLA Matte", 2},
  {"PLA Matte Basic", 8},
  {"PLA Metal", 3},
  {"PLA Silk", 4},
  {"PLA-CF", 5},
  {"PLA-Wood", 6},
  {"PPS-CF", 44},
  {"PVA", 47},
  {"Support For PAHT", 32},
  {"Support For PET/PA", 33},
  {"TPU", 50},
  {"TPU-AERO", 49},
  {"UltraPA", 24},
  {"UltraPA-CF25", 26},
};
static const uint16_t DEFAULT_MATERIALS_COUNT = sizeof(DEFAULT_MATERIALS) / sizeof(DEFAULT_MATERIALS[0]);

struct DefaultManufacturerItem {
  const char* name;
  uint8_t val;
};

static const DefaultManufacturerItem DEFAULT_MANUFACTURERS[] = {
  {"Generic", 0},
  {"QIDI", 1},
};
static const uint16_t DEFAULT_MANUFACTURERS_COUNT = sizeof(DEFAULT_MANUFACTURERS) / sizeof(DEFAULT_MANUFACTURERS[0]);

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
  STR_BLE_CONNECTED,
  STR_BLE_READY,
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
  STR_DISPLAY_INVERSION,
  STR_DISPLAY_INV_ON,
  STR_DISPLAY_INV_OFF,
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
    "BoxRFID - Hauptmenue","Tag Lesen","Tag Schreiben","Auto: AN","Auto: AUS","BLE: Verbunden","BLE: Bereit","Bereit zum Lesen",
    "Konfiguration","Material auswaehlen","Farbe auswaehlen","Warte auf Tag...","NFC beschaeftigt!","Kein Tag gefunden","Auth fehlgeschlagen!",
    "Lesen fehlgeschlagen!","Lesen: Tag erkannt","Schreiben fehlgeschlagen!","Schreiben erfolgreich!","Auto: Tag erkannt","Zurueck",
    "Tag Informationen","Hersteller","Material","Farbe","FEHLER: PN532 nicht gefunden!","Lesen","Schreiben","Setup","Sprache","Display Inversion","AN","AUS","Sprache waehlen",
    "Kalibrierung","Kalibrieren","Druecke jeden Punkt mind. 1 Sekunde!","Kalibrierung gespeichert","Kalibrierung abgebrochen",
    "Werkseinstellung","Werkseinstellungen geladen","Weiss","Schwarz","Grau","Hellgruen","Mint","Blau","Pink","Gelb","Gruen","Hellblau",
    "Dunkelblau","Lavendel","Lime","Royalblau","Himmelblau","Violett","Rosa","Rot","Beige","Silber","Braun","Khaki","Orange","Bronze"
  },
  {
    "BoxRFID - Main Menu","Read Tag","Write Tag","Auto: ON","Auto: OFF","BLE: Connected","BLE: Ready","Ready to read",
    "Configure","Select material","Select color","Waiting for tag...","NFC busy!","No tag found","Auth failed!",
    "Read failed!","Read: tag detected","Write failed!","Write successful!","Auto: tag detected","Back",
    "Tag information","Manufacturer","Material","Color","ERROR: PN532 not found!","Read","Write","Setup","Language","Display inversion","ON","OFF","Select language",
    "Calibration","Calibrate","Press each point for at least 1 second!","Calibration saved","Calibration aborted",
    "Factory default","Factory defaults restored","White","Black","Gray","Light Green","Mint","Blue","Pink","Yellow","Green","Light Blue",
    "Dark Blue","Lavender","Lime","Royal Blue","Sky Blue","Violet","Rose","Red","Beige","Silver","Brown","Khaki","Orange","Bronze"
  },
  {
    "BoxRFID - Menu","Leer Tag","Escribir Tag","Auto: ON","Auto: OFF","BLE: Conectado","BLE: Listo","Listo para leer",
    "Configurar","Elegir material","Elegir color","Esperando tag...","NFC ocupado!","No se encontro tag","Fallo de auth!",
    "Fallo de lectura!","Leido: tag detectado","Fallo de escritura!","Escritura OK!","Auto: tag detectado","Atras",
    "Info del tag","Fabricante","Material","Color","ERROR: PN532 no encontrado!","Leer","Escribir","Setup","Idioma","Inversion de pantalla","ON","OFF","Elegir idioma",
    "Calibracion","Calibrar","Manten cada punto al menos 1 segundo!","Calibracion guardada","Calibracion cancelada",
    "Ajustes fabrica","Ajustes restaurados","Blanco","Negro","Gris","Verde claro","Menta","Azul","Rosa","Amarillo","Verde","Azul claro",
    "Azul oscuro","Lavanda","Lima","Azul rey","Azul cielo","Violeta","Rosado","Rojo","Beige","Plata","Marron","Caqui","Naranja","Bronce"
  },
  {
    "BoxRFID - Menu","Ler Tag","Escrever Tag","Auto: ON","Auto: OFF","BLE: Conectado","BLE: Pronto","Pronto para ler",
    "Configurar","Escolher material","Escolher cor","Aguardando tag...","NFC ocupado!","Nenhum tag","Falha de auth!",
    "Falha na leitura!","Leitura: tag detectado","Falha na escrita!","Escrita OK!","Auto: tag detectado","Voltar",
    "Info do tag","Fabricante","Material","Cor","ERRO: PN532 nao encontrado!","Ler","Escrever","Setup","Idioma","Inversao de ecra","ON","OFF","Selecionar idioma",
    "Calibracao","Calibrar","Pressione cada ponto por 1 segundo!","Calibracao salva","Calibracao cancelada",
    "Padrao fabrica","Padrao restaurado","Branco","Preto","Cinza","Verde claro","Menta","Azul","Rosa","Amarelo","Verde","Azul claro",
    "Azul escuro","Lavanda","Lima","Azul royal","Azul ceu","Violeta","Rose","Vermelho","Bege","Prata","Marrom","Caqui","Laranja","Bronze"
  },
  {
    "BoxRFID - Menu","Lire Tag","Ecrire Tag","Auto: ON","Auto: OFF","BLE: Connecte","BLE: Pret","Pret a lire",
    "Configurer","Choisir mat.","Choisir couleur","Attente du tag...","NFC occupe!","Aucun tag","Auth echouee!",
    "Lecture echouee!","Lu: tag detecte","Ecriture echouee!","Ecriture OK!","Auto: tag detecte","Retour",
    "Info du tag","Fabricant","Materiau","Couleur","ERREUR: PN532 introuvable!","Lire","Ecrire","Setup","Langue","Inversion affichage","ON","OFF","Choisir langue",
    "Calibration","Calibrer","Appuyez 1 seconde sur chaque point!","Calibration enregistree","Calibration annulee",
    "Param. usine","Parametres restaures","Blanc","Noir","Gris","Vert clair","Menthe","Bleu","Rose","Jaune","Vert","Bleu clair",
    "Bleu fonce","Lavande","Citron vert","Bleu royal","Bleu ciel","Violet","Rose","Rouge","Beige","Argent","Marron","Kaki","Orange","Bronze"
  },
  {
    "BoxRFID - Menu principale","Leggi tag","Scrivi tag","Auto: ON","Auto: OFF","BLE: Connesso","BLE: Pronto","Pronto per leggere",
    "Configura","Seleziona materiale","Seleziona colore","In attesa del tag...","NFC occupato!","Nessun tag trovato","Auth fallita!",
    "Lettura fallita!","Lettura: tag rilevato","Scrittura fallita!","Scrittura riuscita!","Auto: tag rilevato","Indietro",
    "Informazioni tag","Produttore","Materiale","Colore","ERRORE: PN532 non trovato!","Leggi","Scrivi","Setup","Lingua","Inversione display","ON","OFF","Seleziona lingua",
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

static const char* const TXT_MATERIAL_LIST[LANG_COUNT]     = {"Materialliste","Material list","Lista materiales","Lista materiais","Liste materiaux","Lista materiali"};
static const char* const TXT_MATERIAL_EDIT[LANG_COUNT]     = {"Material aendern","Edit material","Editar material","Editar material","Modifier materiau","Modifica materiale"};
static const char* const TXT_MATERIAL_NEW[LANG_COUNT]      = {"Neues Material","New material","Nuevo material","Novo material","Nouveau materiau","Nuovo materiale"};
static const char* const TXT_MATERIAL_RESET[LANG_COUNT]    = {"Auslieferungszustand","Factory default","Restaurar fabrica","Restaurar fabrica","Etat usine","Stato di fabbrica"};

static const char* const TXT_MFG_LIST[LANG_COUNT]          = {"Herstellerliste","Manufacturer list","Lista fabricantes","Lista fabricantes","Liste fabricants","Lista produttori"};
static const char* const TXT_MFG_EDIT[LANG_COUNT]          = {"Hersteller aendern","Edit manufacturer","Editar fabricante","Editar fabricante","Modifier fabricant","Modifica produttore"};
static const char* const TXT_MFG_NEW[LANG_COUNT]           = {"Neuer Hersteller","New manufacturer","Nuevo fabricante","Novo fabricante","Nouveau fabricant","Nuovo produttore"};
static const char* const TXT_MFG_RESET[LANG_COUNT]         = {"Auslieferungszustand","Factory default","Restaurar fabrica","Restaurar fabrica","Etat usine","Stato di fabbrica"};

static const char* const TXT_SELECT_ITEM[LANG_COUNT]       = {"Auswaehlen","Select","Seleccionar","Selecionar","Choisir","Seleziona"};
static const char* const TXT_NUMBER[LANG_COUNT]            = {"Nummer:","Number:","Numero:","Numero:","Numero:","Numero:"};
static const char* const TXT_NAME[LANG_COUNT]              = {"Name:","Name:","Nombre:","Nome:","Nom:","Nome:"};
static const char* const TXT_SAVE[LANG_COUNT]              = {"Speichern","Save","Guardar","Salvar","Enregistrer","Salva"};
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
static const char* const TXT_CANCEL[LANG_COUNT]            = {"Abbruch","Cancel","Cancelar","Cancelar","Annuler","Annulla"};
static const char* const TXT_SPACE[LANG_COUNT]             = {"Leerz.","Space","Espacio","Espaco","Espace","Spazio"};
static const char* const TXT_CLEAR[LANG_COUNT]             = {"Loesch.","Clear","Borrar","Limpar","Effacer","Pulisci"};
static const char* const TXT_BKSP[LANG_COUNT]              = {"Bksp","Bksp","Bksp","Bksp","Bksp","Bksp"};
static const char* const TXT_NAME_REQUIRED[LANG_COUNT]     = {"Name eingeben","Enter name","Introducir nombre","Inserir nome","Entrer nom","Inserisci nome"};
static const char* const TXT_FIXED_ITEMS[LANG_COUNT]       = {"QIDI/Generic gesperrt","QIDI/Generic locked","QIDI/Generic bloqueado","QIDI/Generic bloqueado","QIDI/Generic verrouille","QIDI/Generic bloccato"};

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
  {  1, TFT_WHITE,       STR_COLOR_WHITE },
  {  2, TFT_BLACK,       STR_COLOR_BLACK },
  {  3, TFT_LIGHTGREY,   STR_COLOR_GRAY },
  {  4, TFT_GREENYELLOW, STR_COLOR_LIGHT_GREEN },
  {  5, 0x97F3,          STR_COLOR_MINT },
  {  6, TFT_BLUE,        STR_COLOR_BLUE },
  {  7, TFT_PINK,        STR_COLOR_PINK },
  {  8, TFT_YELLOW,      STR_COLOR_YELLOW },
  {  9, TFT_GREEN,       STR_COLOR_GREEN },
  { 10, TFT_CYAN,        STR_COLOR_LIGHT_BLUE },
  { 11, 0x0011,          STR_COLOR_DARK_BLUE },
  { 12, 0xB5BF,          STR_COLOR_LAVENDER },
  { 13, 0x87E0,          STR_COLOR_LIME },
  { 14, 0x435C,          STR_COLOR_ROYAL_BLUE },
  { 15, 0x867F,          STR_COLOR_SKY_BLUE },
  { 16, 0x801F,          STR_COLOR_VIOLET },
  { 17, 0xFBB6,          STR_COLOR_ROSE },
  { 18, TFT_RED,         STR_COLOR_RED },
  { 19, 0xF7BB,          STR_COLOR_BEIGE },
  { 20, 0xC618,          STR_COLOR_SILVER },
  { 21, 0x79E0,          STR_COLOR_BROWN },
  { 22, 0xB5A0,          STR_COLOR_KHAKI },
  { 23, TFT_ORANGE,      STR_COLOR_ORANGE },
  { 24, 0xA145,          STR_COLOR_BRONZE }
};
static const uint16_t COLORS_COUNT = sizeof(COLORS) / sizeof(COLORS[0]);

// ==================== BLE ====================
static const char* BLE_DEVICE_NAME = "BoxRFID-ESP32";
static BLEUUID SERVICE_UUID("6E400001-B5A3-F393-E0A9-E50E24DCCA9E");
static BLEUUID RX_UUID("6E400002-B5A3-F393-E0A9-E50E24DCCA9E");
static BLEUUID TX_UUID("6E400003-B5A3-F393-E0A9-E50E24DCCA9E");

BLEServer* gServer = nullptr;
BLECharacteristic* gTx = nullptr;
BLECharacteristic* gRx = nullptr;

static bool gClientConnected = false;
static String gRxBuffer;
static SemaphoreHandle_t gNfcMutex = nullptr;

// ==================== UI state ====================
enum UIState {
  UI_MAIN,
  UI_READ,
  UI_WRITE,
  UI_PICK_MAT,
  UI_PICK_COLOR,
  UI_PICK_MFG,
  UI_SETUP,
  UI_LANG_SELECT,

  UI_MAT_MENU,
  UI_MAT_EDIT_LIST,
  UI_MAT_EDIT_DETAIL,
  UI_MAT_ADD_LIST,
  UI_MAT_ADD_DETAIL,
  UI_MAT_RESET_CONFIRM,

  UI_MFG_MENU,
  UI_MFG_EDIT_LIST,
  UI_MFG_EDIT_DETAIL,
  UI_MFG_ADD_LIST,
  UI_MFG_ADD_DETAIL,
  UI_MFG_RESET_CONFIRM,

  UI_FACTORY_RESET_CONFIRM,

  UI_MESSAGE_OK,
  UI_KEYBOARD
};

enum KeyboardMode : uint8_t { KB_UPPER=0, KB_LOWER=1, KB_NUM=2 };
enum NoticeKind : uint8_t { NOTICE_MATERIAL=0, NOTICE_MANUFACTURER=1 };

static UIState ui = UI_MAIN;
static UIState uiBeforeKeyboard = UI_MAIN;

static uint8_t selMatVal = 1;
static int matPage = 0;
static int pickMfgPage = 0;
static const int ITEMS_PER_PAGE = 8;

static int selColIdx = 0;
static uint8_t selMfg = MFG_QIDI;

static bool autoDetectEnabled = true;
static uint32_t lastAutoCheck = 0;
static const uint32_t AUTO_CHECK_INTERVAL = 150;

static bool readResultPending = false;
static bool readPopupVisible = false;
static uint32_t readLastSeen = 0;

static bool autoPanelVisible = false;
static uint32_t autoLastSeen = 0;
static uint8_t autoLastMat = 0xFF;
static uint8_t autoLastCol = 0xFF;
static uint8_t autoLastMfg = 0xFF;

static bool needRedraw = true;

// material UI
static int matListPage = 0;
static int matFreePage = 0;
static uint8_t editMatVal = 1;
static char editMatName[ITEM_NAME_MAX + 1] = {0};
static uint8_t addMatVal = 1;
static char addMatName[ITEM_NAME_MAX + 1] = {0};

// manufacturer UI
static int mfgListPage = 0;
static int mfgFreePage = 0;
static uint8_t editMfgVal = 2;
static char editMfgName[ITEM_NAME_MAX + 1] = {0};
static uint8_t addMfgVal = 2;
static char addMfgName[ITEM_NAME_MAX + 1] = {0};

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

// ==================== Prototypes ====================
static void drawStatus(const char* msg, uint16_t color);
static bool waitForTagUID(uint8_t* uid, uint8_t& uidLen, uint32_t timeoutMs);
static bool authBlockWithDefaultKeyA(uint8_t* uid, uint8_t uidLen, uint8_t block);
static bool readBlock(uint8_t block, uint8_t* data);
static bool writeBlock(uint8_t block, const uint8_t* data);
static void drawTagInfoPopup(uint8_t matID, uint8_t colID, uint8_t mfgID);
static void uiRedrawIfNeeded();
static void sendTxLine(const String& s);
static String bytesToHex(const uint8_t* data, size_t len);
static bool hexToBytes16(String hex, uint8_t out[16]);

static void loadMaterials();
static void resetMaterialsToDefault();
static void saveMaterialToPrefs(uint8_t val);
static void ensureSelectedMaterialValid();
static String materialNameByVal(uint8_t val);

static void loadManufacturers();
static void resetManufacturersToDefault();
static void saveManufacturerToPrefs(uint8_t val);
static String manufacturerNameByVal(uint8_t val);

static String trimName18(const String& s);
static void showNotice(NoticeKind kind, UIState nextState);
static void openKeyboardForBuffer(char* target, uint8_t maxLen, UIState returnState);
static void drawKeyboardScreen();
static bool keyboardHandleTouch(int x, int y);

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

static String bytesToHex(const uint8_t* data, size_t len) {
  const char* hex = "0123456789ABCDEF";
  String s;
  s.reserve(len * 2);
  for (size_t i = 0; i < len; i++) {
    s += hex[(data[i] >> 4) & 0x0F];
    s += hex[data[i] & 0x0F];
  }
  return s;
}

static int hexNibble(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return -1;
}

static bool hexToBytes16(String hex, uint8_t out[16]) {
  hex.trim();
  if (hex.length() != 32) return false;
  for (int i = 0; i < 16; i++) {
    int hi = hexNibble(hex[i * 2]);
    int lo = hexNibble(hex[i * 2 + 1]);
    if (hi < 0 || lo < 0) return false;
    out[i] = (uint8_t)((hi << 4) | lo);
  }
  return true;
}

static void sendTxLine(const String& s) {
  Serial.print("TX> ");
  Serial.println(s);
  if (gTx && gClientConnected) {
    String payload = s + "\n";
    gTx->setValue((uint8_t*)payload.c_str(), payload.length());
    gTx->notify();
  }
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
  int c = 0;
  for (uint8_t i = 2; i <= MAX_MANUFACTURERS; i++) if (gManufacturers[i].active) c++;
  return c;
}

static int getFreeManufacturerCount() {
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
  prefs.begin(PREF_NS_MAT, false);
  char key[5];
  snprintf(key, sizeof(key), "m%02u", val);
  if (gMaterials[val].active && strlen(gMaterials[val].name) > 0) prefs.putString(key, gMaterials[val].name);
  else prefs.remove(key);
  prefs.end();
}

static void resetMaterialsToDefault() {
  for (uint8_t i = 1; i <= MAX_MATERIALS; i++) {
    gMaterials[i].active = false;
    gMaterials[i].name[0] = '\0';
  }
  for (uint16_t i = 0; i < DEFAULT_MATERIALS_COUNT; i++) {
    uint8_t v = DEFAULT_MATERIALS[i].val;
    if (v >= 1 && v <= MAX_MATERIALS) {
      gMaterials[v].active = true;
      safeCopy(gMaterials[v].name, DEFAULT_MATERIALS[i].name, sizeof(gMaterials[v].name));
    }
  }
  prefs.begin(PREF_NS_MAT, false);
  prefs.clear();
  prefs.end();
  for (uint8_t i = 1; i <= MAX_MATERIALS; i++) saveMaterialToPrefs(i);
  ensureSelectedMaterialValid();
}

static void loadMaterials() {
  for (uint8_t i = 1; i <= MAX_MATERIALS; i++) {
    gMaterials[i].active = false;
    gMaterials[i].name[0] = '\0';
  }
  prefs.begin(PREF_NS_MAT, true);
  bool anyStored = false;
  for (uint8_t i = 1; i <= MAX_MATERIALS; i++) {
    char key[5];
    snprintf(key, sizeof(key), "m%02u", i);
    String s = prefs.getString(key, "");
    if (s.length() > 0) {
      anyStored = true;
      gMaterials[i].active = true;
      safeCopy(gMaterials[i].name, s.c_str(), sizeof(gMaterials[i].name));
    }
  }
  prefs.end();
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
  prefs.begin(PREF_NS_MFG, false);
  char key[5];
  snprintf(key, sizeof(key), "m%02u", val);
  if (gManufacturers[val].active && strlen(gManufacturers[val].name) > 0) prefs.putString(key, gManufacturers[val].name);
  else prefs.remove(key);
  prefs.end();
}

static void resetManufacturersToDefault() {
  for (uint8_t i = 0; i <= MAX_MANUFACTURERS; i++) {
    gManufacturers[i].active = false;
    gManufacturers[i].name[0] = '\0';
  }
  for (uint16_t i = 0; i < DEFAULT_MANUFACTURERS_COUNT; i++) {
    uint8_t v = DEFAULT_MANUFACTURERS[i].val;
    if (v <= MAX_MANUFACTURERS) {
      gManufacturers[v].active = true;
      safeCopy(gManufacturers[v].name, DEFAULT_MANUFACTURERS[i].name, sizeof(gManufacturers[v].name));
    }
  }
  prefs.begin(PREF_NS_MFG, false);
  prefs.clear();
  prefs.end();
  for (uint8_t i = 0; i <= MAX_MANUFACTURERS; i++) saveManufacturerToPrefs(i);
  if (!gManufacturers[selMfg].active) selMfg = MFG_QIDI;
}

static void loadManufacturers() {
  for (uint8_t i = 0; i <= MAX_MANUFACTURERS; i++) {
    gManufacturers[i].active = false;
    gManufacturers[i].name[0] = '\0';
  }
  prefs.begin(PREF_NS_MFG, true);
  bool anyStored = false;
  for (uint8_t i = 0; i <= MAX_MANUFACTURERS; i++) {
    char key[5];
    snprintf(key, sizeof(key), "m%02u", i);
    String s = prefs.getString(key, "");
    if (s.length() > 0) {
      anyStored = true;
      gManufacturers[i].active = true;
      safeCopy(gManufacturers[i].name, s.c_str(), sizeof(gManufacturers[i].name));
    }
  }
  prefs.end();
  if (!anyStored) resetManufacturersToDefault();

  if (!gManufacturers[MFG_GENERIC].active) {
    gManufacturers[MFG_GENERIC].active = true;
    safeCopy(gManufacturers[MFG_GENERIC].name, "Generic", sizeof(gManufacturers[MFG_GENERIC].name));
  }
  if (!gManufacturers[MFG_QIDI].active) {
    gManufacturers[MFG_QIDI].active = true;
    safeCopy(gManufacturers[MFG_QIDI].name, "QIDI", sizeof(gManufacturers[MFG_QIDI].name));
  }
  if (!gManufacturers[selMfg].active) selMfg = MFG_QIDI;
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

static void saveLanguage(UiLang lang) {
  prefs.begin(PREF_NS_UI, false);
  prefs.putUChar(PREF_LANG, (uint8_t)lang);
  prefs.end();
}

static void clearLanguagePrefs() {
  prefs.begin(PREF_NS_UI, false);
  prefs.clear();
  prefs.end();
  uiLang = LANG_EN;
}

static void loadLanguage() {
  prefs.begin(PREF_NS_UI, true);
  uint8_t v = prefs.getUChar(PREF_LANG, (uint8_t)LANG_EN);
  prefs.end();
  if (v >= LANG_COUNT) v = (uint8_t)LANG_EN;
  uiLang = (UiLang)v;
}

static void saveDisplayInversion(bool enabled) {
  prefs.begin(PREF_NS_UI, false);
  prefs.putBool(PREF_DISP_INV, enabled);
  prefs.end();
}

static void clearDisplayInversionPrefs() {
  prefs.begin(PREF_NS_UI, false);
  prefs.remove(PREF_DISP_INV);
  prefs.end();
  gDisplayInversion = false;
}

static void loadDisplayInversion() {
  prefs.begin(PREF_NS_UI, true);
  gDisplayInversion = prefs.getBool(PREF_DISP_INV, false);
  prefs.end();
}

static void applyDisplayInversion() {
  tft.invertDisplay(gDisplayInversion);
}

static void factoryResetSettings() {
  clearCalibrationPrefs();
  clearLanguagePrefs();
  clearDisplayInversionPrefs();
  applyDisplayInversion();
  resetMaterialsToDefault();
  resetManufacturersToDefault();
  drawStatus(TR(STR_FACTORY_RESET_DONE), TFT_GREEN);
  needRedraw = true;
}

// ==================== Touch functions ====================
static bool getTouchRaw(int& rx, int& ry, int& rz) {
  if (!(ts.tirqTouched() && ts.touched())) return false;
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
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
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

// ==================== Notice ====================
static void showNotice(NoticeKind kind, UIState nextState) {
  messageTitle = LTXT(TXT_NOTICE);
  messageLine1 = (kind == NOTICE_MATERIAL) ? LTXT(TXT_MAT_NOTICE1) : LTXT(TXT_MFG_NOTICE1);
  messageLine2 = LTXT(TXT_NOTICE2);
  messageLine3 = LTXT(TXT_NOTICE3);
  messageLine4 = LTXT(TXT_NOTICE4);
  messageOkNextState = nextState;
  ui = UI_MESSAGE_OK;
  needRedraw = true;
}

// ==================== Tag info popup ====================
static void drawTagInfoPopup(uint8_t matID, uint8_t colID, uint8_t mfgID) {
  int cIdx = findColorById(colID);

  int x = 18;
  int y = 48;
  int w = TFT_W - 36;
  int h = 124;

  tft.fillRoundRect(x, y, w, h, 8, TFT_NAVY);
  tft.drawRoundRect(x, y, w, h, 8, TFT_WHITE);

  tft.setTextColor(TFT_WHITE, TFT_NAVY);
  tft.drawString(TR(STR_TAG_INFO_TITLE), x + 8, y + 8, 2);

  tft.setTextColor(TFT_YELLOW, TFT_NAVY);
  tft.drawString(String(TR(STR_LABEL_MANUFACTURER)) + ":", x + 8, y + 34, 2);
  tft.setTextColor(TFT_WHITE, TFT_NAVY);
  tft.drawString(manufacturerNameByVal(mfgID), x + 118, y + 34, 2);

  tft.setTextColor(TFT_YELLOW, TFT_NAVY);
  tft.drawString(String(TR(STR_LABEL_MATERIAL)) + ":", x + 8, y + 58, 2);
  tft.setTextColor(TFT_WHITE, TFT_NAVY);
  tft.drawString(materialNameByVal(matID), x + 118, y + 58, 2);

  tft.setTextColor(TFT_YELLOW, TFT_NAVY);
  tft.drawString(String(TR(STR_LABEL_COLOR)) + ":", x + 8, y + 82, 2);

  int colorBoxX = x + 118;
  int colorBoxY = y + 80;
  int colorBoxW = 90;
  int colorBoxH = 22;
  uint16_t fill = (cIdx >= 0) ? COLORS[cIdx].rgb565 : TFT_DARKGREY;
  uint16_t textCol = colorTextForBg(fill);

  tft.fillRoundRect(colorBoxX, colorBoxY, colorBoxW, colorBoxH, 4, fill);
  tft.drawRoundRect(colorBoxX, colorBoxY, colorBoxW, colorBoxH, 4, TFT_WHITE);
  tft.setTextDatum(MC_DATUM);
  tft.setTextColor(textCol, fill);
  tft.drawString((cIdx >= 0) ? TR(COLORS[cIdx].labelId) : "Unknown", colorBoxX + colorBoxW / 2, colorBoxY + colorBoxH / 2, 2);
  tft.setTextDatum(TL_DATUM);
}

// ==================== Tag operations ====================
static void performRead() {
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
  drawStatus(TR(STR_READ_TAG_DETECTED), TFT_GREEN);

  readPopupVisible = true;
  readLastSeen = millis();
  readResultPending = true;
}

static void performWrite() {
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

static void autoDetectTick() {
  if (!autoDetectEnabled) {
    if (autoPanelVisible) {
      clearAutoTagPanel();
      autoPanelVisible = false;
      autoLastMat = autoLastCol = autoLastMfg = 0xFF;
      String status = gClientConnected ? TR(STR_BLE_CONNECTED) : TR(STR_BLE_READY);
      drawStatus(status.c_str(), gClientConnected ? TFT_GREEN : TFT_YELLOW);
    }
    return;
  }

  if (ui != UI_MAIN) return;

  if (autoPanelVisible && (millis() - autoLastSeen > 250)) {
    clearAutoTagPanel();
    autoPanelVisible = false;
    autoLastMat = autoLastCol = autoLastMfg = 0xFF;
    return;
  }

  if (millis() - lastAutoCheck < AUTO_CHECK_INTERVAL) return;
  lastAutoCheck = millis();

  NfcLock lock(150);
  if (!lock.locked) return;

  uint8_t uid[10] = {0};
  uint8_t uidLen = 0;

  if (!waitForTagUID(uid, uidLen, 80)) return;
  if (!authBlockWithDefaultKeyA(uid, uidLen, DATA_BLOCK)) return;

  uint8_t data[16] = {0};
  if (!readBlock(DATA_BLOCK, data)) return;

  uint8_t matID = data[0];
  uint8_t colID = data[1];
  uint8_t mfgID = data[2];

  autoLastSeen = millis();

  if (!autoPanelVisible || matID != autoLastMat || colID != autoLastCol || mfgID != autoLastMfg) {
    drawAutoTagPanel(matID, colID, mfgID);
    autoPanelVisible = true;
    autoLastMat = matID;
    autoLastCol = colID;
    autoLastMfg = mfgID;
    drawStatus(TR(STR_AUTO_TAG_DETECTED), TFT_GREEN);
  }
}

static void readAutoReturnTick() {
  if (ui != UI_READ) return;
  if (!readResultPending) return;
  if (!readPopupVisible) return;

  if (millis() - lastAutoCheck >= AUTO_CHECK_INTERVAL) {
    lastAutoCheck = millis();
    NfcLock lock(80);
    if (lock.locked) {
      uint8_t uid[10] = {0};
      uint8_t uidLen = 0;
      if (waitForTagUID(uid, uidLen, 30)) readLastSeen = millis();
    }
  }

  if (millis() - readLastSeen > 250) {
    readPopupVisible = false;
    readResultPending = false;
    needRedraw = true;
  }
}

// ==================== UI drawing ====================
static void drawMainScreen() {
  tft.fillScreen(TFT_BLACK);
  drawHeader(TR(STR_MAIN_TITLE));

  fillButton(20, 50, 140, 50, TFT_DARKGREEN, TFT_WHITE, TR(STR_READ_TAG), TFT_WHITE, 2);
  fillButton(160, 50, 140, 50, TFT_MAROON, TFT_WHITE, TR(STR_WRITE_TAG), TFT_WHITE, 2);
  fillButton(20, 110, 140, 50, TFT_DARKCYAN, TFT_WHITE, TR(STR_SETUP), TFT_WHITE, 2);
  fillButton(160, 110, 140, 50, autoDetectEnabled ? TFT_DARKGREEN : TFT_DARKGREY, TFT_WHITE,
             autoDetectEnabled ? TR(STR_AUTO_ON) : TR(STR_AUTO_OFF), TFT_WHITE, 2);

  drawStatusBarFrame();
  String status = gClientConnected ? TR(STR_BLE_CONNECTED) : TR(STR_BLE_READY);
  drawStatus(status.c_str(), gClientConnected ? TFT_GREEN : TFT_YELLOW);
}

static void drawReadScreen() {
  tft.fillScreen(TFT_BLACK);
  drawHeader(TR(STR_ACTION_READ));
  fillButton(10, 50, 100, 40, TFT_DARKGREY, TFT_WHITE, TR(STR_BACK), TFT_WHITE, 2);
  fillButton(210, 50, 100, 40, TFT_DARKGREEN, TFT_WHITE, TR(STR_READ_TAG), TFT_WHITE, 2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString(TR(STR_READY_READ), TFT_W / 2, 120, 2);
  drawStatusBarFrame();
  drawStatus(TR(STR_READY_READ), TFT_WHITE);
}

static void drawWriteScreen() {
  ensureSelectedMaterialValid();

  tft.fillScreen(TFT_BLACK);
  drawHeader(TR(STR_ACTION_WRITE));

  const int topY = UI_HEADER_H + 3;
  const int topH = 28;

  fillButton(8, topY, 80, topH, TFT_DARKGREY, TFT_WHITE, TR(STR_BACK), TFT_WHITE, 2);
  fillButton(TFT_W - 8 - 110, topY, 110, topH, TFT_DARKGREEN, TFT_WHITE, TR(STR_WRITE_TAG), TFT_WHITE, 2);

  const int labelX = 12;
  const int btnX = 122;
  const int btnW = TFT_W - btnX - 12;
  const int row1Y = 78;
  const int row2Y = 122;
  const int row3Y = 166;

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString(String(TR(STR_LABEL_MANUFACTURER)) + ":", labelX, row1Y + 9, 2);
  tft.drawString(String(TR(STR_LABEL_MATERIAL)) + ":", labelX, row2Y + 9, 2);
  tft.drawString(String(TR(STR_LABEL_COLOR)) + ":", labelX, row3Y + 11, 2);

  fillButton(btnX, row1Y, btnW, 34, TFT_NAVY, TFT_WHITE, trimName18(manufacturerNameByVal(selMfg)), TFT_WHITE, 2);
  fillButton(btnX, row2Y, btnW, 34, TFT_DARKCYAN, TFT_WHITE, trimName18(materialNameByVal(selMatVal)), TFT_WHITE, 2);

  uint16_t cfill = COLORS[selColIdx].rgb565;
  uint16_t ctxt  = colorTextForBg(cfill);
  fillButton(btnX, row3Y, btnW, 38, cfill, TFT_WHITE, TR(COLORS[selColIdx].labelId), ctxt, 2);

  drawStatusBarFrame();
  drawStatus(TR(STR_CONFIGURE), TFT_WHITE);
}

static String displayInversionButtonText() {
  return String(TR(STR_DISPLAY_INVERSION)) + ": " + (gDisplayInversion ? TR(STR_DISPLAY_INV_ON) : TR(STR_DISPLAY_INV_OFF));
}

static void drawSetupScreen() {
  tft.fillScreen(TFT_BLACK);
  drawHeader(TR(STR_SETUP));

  fillButton(8, 40, 88, 28, TFT_DARKGREY, TFT_WHITE, TR(STR_BACK), TFT_WHITE, 2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(TR_DATUM);
  tft.drawString(String(LTXT(TXT_APP_PREFIX)) + APP_VERSION, TFT_W - 8, 48, 2);
  tft.setTextDatum(TL_DATUM);

  const int leftX = 20;
  const int rightX = 165;
  const int btnW = 135;
  const int btnH = 30;
  const int row1Y = 74;
  const int row2Y = 110;
  const int row3Y = 146;
  const int row4Y = 182;
  const int fullX = 20;
  const int fullW = 280;

  fillButton(leftX,  row1Y, btnW, btnH, TFT_NAVY,     TFT_WHITE, LTXT(TXT_MANUFACTURER), TFT_WHITE, 2);
  fillButton(rightX, row1Y, btnW, btnH, TFT_DARKCYAN, TFT_WHITE, LTXT(TXT_MATERIAL),     TFT_WHITE, 2);

  fillButton(leftX,  row2Y, btnW, btnH, TFT_DARKGREY, TFT_WHITE, TR(STR_LANGUAGE),      TFT_WHITE, 2);
  fillButton(rightX, row2Y, btnW, btnH, TFT_DARKGREY, TFT_WHITE, LTXT(TXT_CALIBRATION), TFT_WHITE, 2);

  fillButton(fullX, row3Y, fullW, btnH, gDisplayInversion ? TFT_DARKGREEN : TFT_DARKGREY, TFT_WHITE, displayInversionButtonText(), TFT_WHITE, 2);
  fillButton(fullX, row4Y, fullW, btnH, TFT_MAROON, TFT_WHITE, TR(STR_FACTORY_DEFAULTS), TFT_WHITE, 2);

  drawStatusBarFrame();
  drawStatus(TR(STR_SETUP), TFT_WHITE);
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
  fillButton(40, 68, 240, 34, TFT_DARKCYAN, TFT_WHITE, editTitle, TFT_WHITE, 2);
  fillButton(40, 110, 240, 34, TFT_NAVY, TFT_WHITE, addTitle, TFT_WHITE, 2);
  fillButton(40, 152, 240, 34, TFT_MAROON, TFT_WHITE, resetTitle, TFT_WHITE, 2);
  drawStatusBarFrame();
  drawStatus(listTitle, TFT_WHITE);
}

static void drawMaterialListScreen(const char* title, const char* statusText, int page, bool freeMode) {
  tft.fillScreen(TFT_BLACK);
  drawHeader(title);
  fillButton(8, 35, 80, 28, TFT_DARKGREY, TFT_WHITE, TR(STR_BACK), TFT_WHITE, 2);
  fillButton(TFT_W - 120, 33, 52, 34, TFT_DARKGREY, TFT_WHITE, "<", TFT_WHITE, 2);
  fillButton(TFT_W - 60, 33, 52, 34, TFT_DARKGREY, TFT_WHITE, ">", TFT_WHITE, 2);

  int total = freeMode ? getFreeMaterialCount() : getActiveMaterialCount();
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

static void drawManufacturerListScreen(const char* title, const char* statusText, int page, bool freeMode) {
  tft.fillScreen(TFT_BLACK);
  drawHeader(title);
  fillButton(8, 35, 80, 28, TFT_DARKGREY, TFT_WHITE, TR(STR_BACK), TFT_WHITE, 2);
  fillButton(TFT_W - 120, 33, 52, 34, TFT_DARKGREY, TFT_WHITE, "<", TFT_WHITE, 2);
  fillButton(TFT_W - 60, 33, 52, 34, TFT_DARKGREY, TFT_WHITE, ">", TFT_WHITE, 2);

  int total = freeMode ? getFreeManufacturerCount() : getEditableManufacturerCount();
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

static void drawItemDetailScreen(const char* title, uint8_t value, const char* name, const char* buttonText, const char* statusText) {
  tft.fillScreen(TFT_BLACK);
  drawHeader(title);
  fillButton(8, 35, 80, 28, TFT_DARKGREY, TFT_WHITE, TR(STR_BACK), TFT_WHITE, 2);
  fillButton(TFT_W - 88, 35, 80, 28, TFT_DARKGREEN, TFT_WHITE, LTXT(TXT_SAVE), TFT_WHITE, 2);

  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString(LTXT(TXT_NUMBER), 16, 78, 2);
  tft.drawString(LTXT(TXT_NAME), 16, 118, 2);

  fillButton(120, 72, 180, 30, TFT_DARKGREY, TFT_WHITE, String(value), TFT_WHITE, 2);
  fillButton(120, 112, 180, 30, TFT_NAVY, TFT_WHITE, trimName18(String(name)), TFT_WHITE, 2);
  fillButton(70, 160, 180, 34, TFT_DARKCYAN, TFT_WHITE, buttonText, TFT_WHITE, 2);

  drawStatusBarFrame();
  drawStatus(statusText, TFT_WHITE);
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
static void openKeyboardForBuffer(char* target, uint8_t maxLen, UIState returnState) {
  kbTargetBuffer = target;
  kbTargetMaxLen = maxLen;
  uiBeforeKeyboard = returnState;
  kbMode = KB_UPPER;
  ui = UI_KEYBOARD;
  needRedraw = true;
}

static void keyboardAppendChar(char ch) {
  if (!kbTargetBuffer) return;
  size_t len = strlen(kbTargetBuffer);
  if (len >= kbTargetMaxLen) return;
  kbTargetBuffer[len] = ch;
  kbTargetBuffer[len + 1] = '\0';
}

static void keyboardBackspace() {
  if (!kbTargetBuffer) return;
  size_t len = strlen(kbTargetBuffer);
  if (len == 0) return;
  kbTargetBuffer[len - 1] = '\0';
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

  if (kbMode == KB_UPPER || kbMode == KB_LOWER) {
    const char* row1 = (kbMode == KB_UPPER) ? "QWERTYUIOP" : "qwertyuiop";
    const char* row2 = (kbMode == KB_UPPER) ? "ASDFGHJKL"  : "asdfghjkl";
    const char* row3 = (kbMode == KB_UPPER) ? "ZXCVBNM"    : "zxcvbnm";

    for (int i = 0; i < 10; i++) drawKeyboardKey(10 + i * (keyW + gap), row1Y, keyW, keyH, String(row1[i]));
    for (int i = 0; i < 9; i++)  drawKeyboardKey(24 + i * (keyW + gap), row2Y, keyW, keyH, String(row2[i]));

    drawKeyboardKey(10, row3Y, 42, keyH, (kbMode == KB_UPPER) ? "ABC" : "abc", TFT_NAVY);
    for (int i = 0; i < 7; i++)  drawKeyboardKey(54 + i * (keyW + gap), row3Y, keyW, keyH, String(row3[i]));
    drawKeyboardKey(54 + 7 * (keyW + gap), row3Y, 72, keyH, "_", TFT_DARKCYAN);

    drawKeyboardKey(10, row4Y, 48, keyH, "123", TFT_NAVY);
    drawKeyboardKey(62, row4Y, 56, keyH, LTXT(TXT_SPACE), TFT_DARKCYAN);
    drawKeyboardKey(122, row4Y, 66, keyH, LTXT(TXT_CLEAR), TFT_MAROON);
    drawKeyboardKey(192, row4Y, 58, keyH, LTXT(TXT_BKSP), TFT_MAROON);
    drawKeyboardKey(254, row4Y, 56, keyH, "-", TFT_DARKCYAN);
  } else {
    const char* row1 = "1234567890";
    for (int i = 0; i < 10; i++) drawKeyboardKey(10 + i * (keyW + gap), row1Y, keyW, keyH, String(row1[i]));
    drawKeyboardKey(24, row2Y, 40, keyH, "-", TFT_DARKCYAN);
    drawKeyboardKey(68, row2Y, 40, keyH, "_", TFT_DARKCYAN);
    drawKeyboardKey(112, row2Y, 96, keyH, LTXT(TXT_SPACE), TFT_DARKCYAN);
    drawKeyboardKey(212, row2Y, 84, keyH, LTXT(TXT_BKSP), TFT_MAROON);
    drawKeyboardKey(10, row3Y, 64, keyH, "ABC", TFT_NAVY);
    drawKeyboardKey(78, row3Y, 64, keyH, "abc", TFT_NAVY);
    drawKeyboardKey(146, row3Y, 64, keyH, "123", TFT_DARKGREEN);
    drawKeyboardKey(214, row3Y, 96, keyH, LTXT(TXT_CLEAR), TFT_MAROON);
    drawKeyboardKey(90, row4Y, 140, keyH, LTXT(TXT_SPACE), TFT_DARKCYAN);
  }

  drawStatusBarFrame();
  if (kbMode == KB_UPPER) drawStatus("ABC", TFT_WHITE);
  else if (kbMode == KB_LOWER) drawStatus("abc", TFT_WHITE);
  else drawStatus("123", TFT_WHITE);
}

static bool keyboardHandleTouch(int x, int y) {
  if (x >= 8 && x < 88 && y >= 35 && y < 63) {
    ui = uiBeforeKeyboard;
    needRedraw = true;
    return true;
  }
  if (x >= TFT_W - 88 && x < TFT_W - 8 && y >= 35 && y < 63) {
    ui = uiBeforeKeyboard;
    needRedraw = true;
    return true;
  }

  const int keyW = 28, keyH = 24, gap = 2;
  const int row1Y = 106, row2Y = 132, row3Y = 158, row4Y = 186;

  if (kbMode == KB_UPPER || kbMode == KB_LOWER) {
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
    if (x >= 54 + 7 * (keyW + gap) && x < 54 + 7 * (keyW + gap) + 72 && y >= row3Y && y < row3Y + keyH) { keyboardAppendChar('_'); needRedraw = true; return true; }
    if (x >= 10 && x < 58 && y >= row4Y && y < row4Y + keyH) { kbMode = KB_NUM; needRedraw = true; return true; }
    if (x >= 62 && x < 118 && y >= row4Y && y < row4Y + keyH) { keyboardAppendChar(' '); needRedraw = true; return true; }
    if (x >= 122 && x < 188 && y >= row4Y && y < row4Y + keyH) { if (kbTargetBuffer) kbTargetBuffer[0] = '\0'; needRedraw = true; return true; }
    if (x >= 192 && x < 250 && y >= row4Y && y < row4Y + keyH) { keyboardBackspace(); needRedraw = true; return true; }
    if (x >= 254 && x < 310 && y >= row4Y && y < row4Y + keyH) { keyboardAppendChar('-'); needRedraw = true; return true; }
  } else {
    const char* row1 = "1234567890";
    for (int i = 0; i < 10; i++) {
      int bx = 10 + i * (keyW + gap);
      if (x >= bx && x < bx + keyW && y >= row1Y && y < row1Y + keyH) { keyboardAppendChar(row1[i]); needRedraw = true; return true; }
    }
    if (x >= 24 && x < 64 && y >= row2Y && y < row2Y + keyH) { keyboardAppendChar('-'); needRedraw = true; return true; }
    if (x >= 68 && x < 108 && y >= row2Y && y < row2Y + keyH) { keyboardAppendChar('_'); needRedraw = true; return true; }
    if (x >= 112 && x < 208 && y >= row2Y && y < row2Y + keyH) { keyboardAppendChar(' '); needRedraw = true; return true; }
    if (x >= 212 && x < 296 && y >= row2Y && y < row2Y + keyH) { keyboardBackspace(); needRedraw = true; return true; }
    if (x >= 10 && x < 74 && y >= row3Y && y < row3Y + keyH) { kbMode = KB_UPPER; needRedraw = true; return true; }
    if (x >= 78 && x < 142 && y >= row3Y && y < row3Y + keyH) { kbMode = KB_LOWER; needRedraw = true; return true; }
    if (x >= 146 && x < 210 && y >= row3Y && y < row3Y + keyH) { kbMode = KB_NUM; needRedraw = true; return true; }
    if (x >= 214 && x < 310 && y >= row3Y && y < row3Y + keyH) { if (kbTargetBuffer) kbTargetBuffer[0] = '\0'; needRedraw = true; return true; }
    if (x >= 90 && x < 230 && y >= row4Y && y < row4Y + keyH) { keyboardAppendChar(' '); needRedraw = true; return true; }
  }

  return false;
}

// ==================== UI redraw ====================
static void uiRedrawIfNeeded() {
  if (!needRedraw) return;
  needRedraw = false;

  switch (ui) {
    case UI_MAIN:               drawMainScreen(); break;
    case UI_READ:               drawReadScreen(); break;
    case UI_WRITE:              drawWriteScreen(); break;
    case UI_PICK_MAT:           drawPickMatScreen(); break;
    case UI_PICK_COLOR:         drawPickColorScreen(); break;
    case UI_PICK_MFG:           drawPickMfgScreen(); break;
    case UI_SETUP:              drawSetupScreen(); break;
    case UI_LANG_SELECT:        drawLangSelectScreen(); break;

    case UI_MAT_MENU:           drawItemMenuScreen(LTXT(TXT_MATERIAL), LTXT(TXT_MATERIAL_LIST), LTXT(TXT_MATERIAL_EDIT), LTXT(TXT_MATERIAL_NEW), LTXT(TXT_MATERIAL_RESET)); break;
    case UI_MAT_EDIT_LIST:      drawMaterialListScreen(LTXT(TXT_MATERIAL_EDIT), LTXT(TXT_SELECT_ITEM), matListPage, false); break;
    case UI_MAT_EDIT_DETAIL:    drawItemDetailScreen(LTXT(TXT_MATERIAL_EDIT), editMatVal, editMatName, LTXT(TXT_CHANGE_NAME), LTXT(TXT_MATERIAL_EDIT)); break;
    case UI_MAT_ADD_LIST:       drawMaterialListScreen(LTXT(TXT_MATERIAL_NEW), LTXT(TXT_CHOOSE_FREE), matFreePage, true); break;
    case UI_MAT_ADD_DETAIL:     drawItemDetailScreen(LTXT(TXT_MATERIAL_NEW), addMatVal, addMatName, LTXT(TXT_ENTER_NAME), LTXT(TXT_MATERIAL_NEW)); break;
    case UI_MAT_RESET_CONFIRM:  drawConfirmScreen(LTXT(TXT_MATERIAL), LTXT(TXT_MAT_RESET_Q1), LTXT(TXT_MAT_RESET_Q2), LTXT(TXT_MAT_RESET_Q3)); break;

    case UI_MFG_MENU:           drawItemMenuScreen(LTXT(TXT_MANUFACTURER), LTXT(TXT_MFG_LIST), LTXT(TXT_MFG_EDIT), LTXT(TXT_MFG_NEW), LTXT(TXT_MFG_RESET)); break;
    case UI_MFG_EDIT_LIST:      drawManufacturerListScreen(LTXT(TXT_MFG_EDIT), LTXT(TXT_SELECT_ITEM), mfgListPage, false); break;
    case UI_MFG_EDIT_DETAIL:    drawItemDetailScreen(LTXT(TXT_MFG_EDIT), editMfgVal, editMfgName, LTXT(TXT_CHANGE_NAME), LTXT(TXT_MFG_EDIT)); break;
    case UI_MFG_ADD_LIST:       drawManufacturerListScreen(LTXT(TXT_MFG_NEW), LTXT(TXT_CHOOSE_FREE), mfgFreePage, true); break;
    case UI_MFG_ADD_DETAIL:     drawItemDetailScreen(LTXT(TXT_MFG_NEW), addMfgVal, addMfgName, LTXT(TXT_ENTER_NAME), LTXT(TXT_MFG_NEW)); break;
    case UI_MFG_RESET_CONFIRM:  drawConfirmScreen(LTXT(TXT_MANUFACTURER), LTXT(TXT_MFG_RESET_Q1), LTXT(TXT_MFG_RESET_Q2), LTXT(TXT_MFG_RESET_Q3)); break;

    case UI_FACTORY_RESET_CONFIRM: drawConfirmScreen(LTXT(TXT_FACTORY_RESET_TITLE), LTXT(TXT_FACTORY_RESET_Q1), LTXT(TXT_FACTORY_RESET_Q2), LTXT(TXT_FACTORY_RESET_Q3)); break;

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
    if (hit(20, 50, 140, 50, x, y))  { ui = UI_READ;  needRedraw = true; return; }
    if (hit(160, 50, 140, 50, x, y)) { ui = UI_WRITE; needRedraw = true; return; }
    if (hit(20, 110, 140, 50, x, y)) { ui = UI_SETUP; needRedraw = true; return; }
    if (hit(160, 110, 140, 50, x, y)) { autoDetectEnabled = !autoDetectEnabled; needRedraw = true; return; }
    return;
  }

  if (ui == UI_SETUP) {
    if (hit(8, 40, 88, 28, x, y)) { ui = UI_MAIN; needRedraw = true; return; }
    if (hit(20, 74, 135, 30, x, y)) { ui = UI_MFG_MENU; needRedraw = true; return; }
    if (hit(165, 74, 135, 30, x, y)) { ui = UI_MAT_MENU; needRedraw = true; return; }
    if (hit(20, 110, 135, 30, x, y)) { ui = UI_LANG_SELECT; needRedraw = true; return; }
    if (hit(165, 110, 135, 30, x, y)) { calibrateTouch(); ui = UI_SETUP; needRedraw = true; return; }
    if (hit(20, 146, 280, 30, x, y)) {
      gDisplayInversion = !gDisplayInversion;
      saveDisplayInversion(gDisplayInversion);
      applyDisplayInversion();
      needRedraw = true;
      return;
    }
    if (hit(20, 182, 280, 30, x, y)) { ui = UI_FACTORY_RESET_CONFIRM; needRedraw = true; return; }
    return;
  }

  if (ui == UI_LANG_SELECT) {
    if (hit(8, 35, 80, 28, x, y)) { ui = UI_SETUP; needRedraw = true; return; }
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
          saveLanguage(uiLang);
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
    if (hit(210, 50, 100, 40, x, y)) { performRead(); return; }
    return;
  }

  if (ui == UI_WRITE) {
    const int topY = UI_HEADER_H + 3;
    if (hit(8, topY, 80, 28, x, y)) { ui = UI_MAIN; needRedraw = true; return; }
    if (hit(TFT_W - 118, topY, 110, 28, x, y)) { performWrite(); return; }

    const int btnX = 122;
    const int btnW = TFT_W - btnX - 12;
    if (hit(btnX, 78, btnW, 34, x, y)) { ui = UI_PICK_MFG; needRedraw = true; return; }
    if (hit(btnX, 122, btnW, 34, x, y)) { matPage = 0; ui = UI_PICK_MAT; needRedraw = true; return; }
    if (hit(btnX - 8, 166 - 8, btnW + 16, 38 + 16, x, y)) { ui = UI_PICK_COLOR; needRedraw = true; return; }
    return;
  }

  if (ui == UI_PICK_MFG) {
    if (hit(8, 35, 80, 28, x, y)) { ui = UI_WRITE; needRedraw = true; return; }
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
    if (hit(8, 35, 80, 28, x, y)) { ui = UI_WRITE; needRedraw = true; return; }
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
    if (hit(8, 35, 80, 28, x, y)) { ui = UI_WRITE; needRedraw = true; return; }
    const int cols = 6, rows = 4, boxW = 46, boxH = 28, gapX = 5, gapY = 7, x0 = 8, y0 = 72;
    int idx = 0;
    for (int r = 0; r < rows && idx < (int)COLORS_COUNT; r++) {
      for (int c = 0; c < cols && idx < (int)COLORS_COUNT; c++) {
        if (hit(x0 + c * (boxW + gapX), y0 + r * (boxH + gapY), boxW, boxH, x, y)) {
          selColIdx = idx;
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
    if (hit(8, 35, 80, 28, x, y)) { ui = UI_SETUP; needRedraw = true; return; }
    if (hit(40, 68, 240, 34, x, y)) { matListPage = 0; ui = UI_MAT_EDIT_LIST; needRedraw = true; return; }
    if (hit(40, 110, 240, 34, x, y)) { matFreePage = 0; ui = UI_MAT_ADD_LIST; needRedraw = true; return; }
    if (hit(40, 152, 240, 34, x, y)) { ui = UI_MAT_RESET_CONFIRM; needRedraw = true; return; }
    return;
  }

  if (ui == UI_MAT_EDIT_LIST) {
    if (hit(8, 35, 80, 28, x, y)) { ui = UI_MAT_MENU; needRedraw = true; return; }
    int total = getActiveMaterialCount();
    int pages = max(1, (total + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE);
    if (hit(TFT_W - 120, 33, 52, 34, x, y)) { matListPage--; if (matListPage < 0) matListPage = pages - 1; needRedraw = true; return; }
    if (hit(TFT_W - 60, 33, 52, 34, x, y))  { matListPage = (matListPage + 1) % pages; needRedraw = true; return; }

    const int cols = 2, rows = 4, w = 146, h = 32, gapX = 12, gapY = 6, x0 = 8, y0 = 70;
    int startIdx = matListPage * ITEMS_PER_PAGE;
    int idx = startIdx;
    for (int r = 0; r < rows && idx < total; r++) {
      for (int c = 0; c < cols && idx < total; c++) {
        if (hit(x0 + c * (w + gapX), y0 + r * (h + gapY), w, h, x, y)) {
          editMatVal = getActiveMaterialByIndex(idx);
          safeCopy(editMatName, gMaterials[editMatVal].name, sizeof(editMatName));
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
    if (hit(8, 35, 80, 28, x, y)) { ui = UI_MAT_EDIT_LIST; needRedraw = true; return; }
    if (hit(TFT_W - 88, 35, 80, 28, x, y)) {
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
      return;
    }
    if (hit(70, 160, 180, 34, x, y)) { openKeyboardForBuffer(editMatName, ITEM_NAME_MAX, UI_MAT_EDIT_DETAIL); return; }
    return;
  }

  if (ui == UI_MAT_ADD_LIST) {
    if (hit(8, 35, 80, 28, x, y)) { ui = UI_MAT_MENU; needRedraw = true; return; }
    int total = getFreeMaterialCount();
    int pages = max(1, (total + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE);
    if (hit(TFT_W - 120, 33, 52, 34, x, y)) { matFreePage--; if (matFreePage < 0) matFreePage = pages - 1; needRedraw = true; return; }
    if (hit(TFT_W - 60, 33, 52, 34, x, y))  { matFreePage = (matFreePage + 1) % pages; needRedraw = true; return; }

    const int cols = 2, rows = 4, w = 146, h = 32, gapX = 12, gapY = 6, x0 = 8, y0 = 70;
    int startIdx = matFreePage * ITEMS_PER_PAGE;
    int idx = startIdx;
    for (int r = 0; r < rows && idx < total; r++) {
      for (int c = 0; c < cols && idx < total; c++) {
        if (hit(x0 + c * (w + gapX), y0 + r * (h + gapY), w, h, x, y)) {
          addMatVal = getFreeMaterialByIndex(idx);
          addMatName[0] = '\0';
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
    if (hit(8, 35, 80, 28, x, y)) { ui = UI_MAT_ADD_LIST; needRedraw = true; return; }
    if (hit(TFT_W - 88, 35, 80, 28, x, y)) {
      if (strlen(addMatName) > 0) {
        gMaterials[addMatVal].active = true;
        safeCopy(gMaterials[addMatVal].name, addMatName, sizeof(gMaterials[addMatVal].name));
        saveMaterialToPrefs(addMatVal);
        if (!gMaterials[selMatVal].active) selMatVal = addMatVal;
        showNotice(NOTICE_MATERIAL, UI_MAT_MENU);
      } else {
        drawStatus(LTXT(TXT_NAME_REQUIRED), TFT_RED);
      }
      return;
    }
    if (hit(70, 160, 180, 34, x, y)) { openKeyboardForBuffer(addMatName, ITEM_NAME_MAX, UI_MAT_ADD_DETAIL); return; }
    return;
  }

  if (ui == UI_MAT_RESET_CONFIRM) {
    if (hit(45, 160, 90, 34, x, y)) { resetMaterialsToDefault(); ui = UI_MAT_MENU; needRedraw = true; return; }
    if (hit(185, 160, 90, 34, x, y)) { ui = UI_MAT_MENU; needRedraw = true; return; }
    return;
  }

  if (ui == UI_MFG_MENU) {
    if (hit(8, 35, 80, 28, x, y)) { ui = UI_SETUP; needRedraw = true; return; }
    if (hit(40, 68, 240, 34, x, y)) { mfgListPage = 0; ui = UI_MFG_EDIT_LIST; needRedraw = true; return; }
    if (hit(40, 110, 240, 34, x, y)) { mfgFreePage = 0; ui = UI_MFG_ADD_LIST; needRedraw = true; return; }
    if (hit(40, 152, 240, 34, x, y)) { ui = UI_MFG_RESET_CONFIRM; needRedraw = true; return; }
    return;
  }

  if (ui == UI_MFG_EDIT_LIST) {
    if (hit(8, 35, 80, 28, x, y)) { ui = UI_MFG_MENU; needRedraw = true; return; }
    int total = getEditableManufacturerCount();
    int pages = max(1, (total + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE);
    if (hit(TFT_W - 120, 33, 52, 34, x, y)) { mfgListPage--; if (mfgListPage < 0) mfgListPage = pages - 1; needRedraw = true; return; }
    if (hit(TFT_W - 60, 33, 52, 34, x, y))  { mfgListPage = (mfgListPage + 1) % pages; needRedraw = true; return; }

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
    if (hit(8, 35, 80, 28, x, y)) { ui = UI_MFG_EDIT_LIST; needRedraw = true; return; }
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
        openKeyboardForBuffer(editMfgName, ITEM_NAME_MAX, UI_MFG_EDIT_DETAIL);
      }
      return;
    }
    return;
  }

  if (ui == UI_MFG_ADD_LIST) {
    if (hit(8, 35, 80, 28, x, y)) { ui = UI_MFG_MENU; needRedraw = true; return; }
    int total = getFreeManufacturerCount();
    int pages = max(1, (total + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE);
    if (hit(TFT_W - 120, 33, 52, 34, x, y)) { mfgFreePage--; if (mfgFreePage < 0) mfgFreePage = pages - 1; needRedraw = true; return; }
    if (hit(TFT_W - 60, 33, 52, 34, x, y))  { mfgFreePage = (mfgFreePage + 1) % pages; needRedraw = true; return; }

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
    if (hit(8, 35, 80, 28, x, y)) { ui = UI_MFG_ADD_LIST; needRedraw = true; return; }
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
    if (hit(70, 160, 180, 34, x, y)) { openKeyboardForBuffer(addMfgName, ITEM_NAME_MAX, UI_MFG_ADD_DETAIL); return; }
    return;
  }

  if (ui == UI_MFG_RESET_CONFIRM) {
    if (hit(45, 160, 90, 34, x, y)) { resetManufacturersToDefault(); ui = UI_MFG_MENU; needRedraw = true; return; }
    if (hit(185, 160, 90, 34, x, y)) { ui = UI_MFG_MENU; needRedraw = true; return; }
    return;
  }

  if (ui == UI_FACTORY_RESET_CONFIRM) {
    if (hit(45, 160, 90, 34, x, y)) { factoryResetSettings(); ui = UI_SETUP; needRedraw = true; return; }
    if (hit(185, 160, 90, 34, x, y)) { ui = UI_SETUP; needRedraw = true; return; }
    return;
  }
}

// ==================== BLE command handler ====================
static void handleCommandLine(String line) {
  line.trim();
  if (line.length() == 0) return;

  int sp1 = line.indexOf(' ');
  String cmd = (sp1 < 0) ? line : line.substring(0, sp1);
  cmd.toUpperCase();

  if (cmd == "PING") { sendTxLine("OK PONG"); return; }

  if (cmd == "HELP") {
    sendTxLine("OK CMDS: PING | PRESENT | UID | READ <block> | WRITE <block> <32HEX> | HELP");
    return;
  }

  if (cmd == "PRESENT") {
    NfcLock lock(300);
    if (!lock.locked) { sendTxLine("OK NO_TAG"); return; }
    uint8_t uid[10] = {0}; uint8_t uidLen = 0;
    if (waitForTagUID(uid, uidLen, 120)) sendTxLine("OK TAG " + bytesToHex(uid, uidLen));
    else sendTxLine("OK NO_TAG");
    return;
  }

  if (cmd == "UID") {
    NfcLock lock(2000);
    if (!lock.locked) { sendTxLine("ERR BUSY"); return; }
    uint8_t uid[10] = {0}; uint8_t uidLen = 0;
    sendTxLine("OK PLACE_TAG");
    if (!waitForTagUID(uid, uidLen, 5000)) { sendTxLine("ERR NO_TAG"); return; }
    sendTxLine("OK UID " + bytesToHex(uid, uidLen));
    return;
  }

  if (cmd == "READ") {
    if (sp1 < 0) { sendTxLine("ERR ARG READ <block>"); return; }
    String rest = line.substring(sp1 + 1); rest.trim();
    int block = rest.toInt();
    if (block < 0 || block > 255) { sendTxLine("ERR BAD_BLOCK"); return; }

    NfcLock lock(2500);
    if (!lock.locked) { sendTxLine("ERR BUSY"); return; }

    uint8_t uid[10] = {0}; uint8_t uidLen = 0;
    sendTxLine("OK PLACE_TAG");
    if (!waitForTagUID(uid, uidLen, 5000)) { sendTxLine("ERR NO_TAG"); return; }
    if (!authBlockWithDefaultKeyA(uid, uidLen, (uint8_t)block)) { sendTxLine("ERR AUTH_FAIL"); return; }

    uint8_t data[16] = {0};
    if (!nfc.mifareclassic_ReadDataBlock((uint8_t)block, data)) { sendTxLine("ERR READ_FAIL"); return; }
    sendTxLine("OK READ " + String(block) + " " + bytesToHex(data, 16));
    return;
  }

  if (cmd == "WRITE") {
    if (sp1 < 0) { sendTxLine("ERR ARG WRITE <block> <32HEX>"); return; }
    String rest = line.substring(sp1 + 1); rest.trim();

    int sp2 = rest.indexOf(' ');
    if (sp2 < 0) { sendTxLine("ERR ARG WRITE <block> <32HEX>"); return; }

    String blockStr = rest.substring(0, sp2);
    String hexStr = rest.substring(sp2 + 1);
    blockStr.trim(); hexStr.trim();

    int block = blockStr.toInt();
    if (block < 0 || block > 255) { sendTxLine("ERR BAD_BLOCK"); return; }

    uint8_t payload[16] = {0};
    if (!hexToBytes16(hexStr, payload)) { sendTxLine("ERR BAD_HEX (need 32 hex chars)"); return; }

    NfcLock lock(2500);
    if (!lock.locked) { sendTxLine("ERR BUSY"); return; }

    uint8_t uid[10] = {0}; uint8_t uidLen = 0;
    sendTxLine("OK PLACE_TAG");
    if (!waitForTagUID(uid, uidLen, 5000)) { sendTxLine("ERR NO_TAG"); return; }
    if (!authBlockWithDefaultKeyA(uid, uidLen, (uint8_t)block)) { sendTxLine("ERR AUTH_FAIL"); return; }
    if (!nfc.mifareclassic_WriteDataBlock((uint8_t)block, payload)) { sendTxLine("ERR WRITE_FAIL"); return; }

    sendTxLine("OK WRITE " + String(block));
    return;
  }

  sendTxLine("ERR UNKNOWN_CMD");
}

// ==================== BLE callbacks ====================
class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer*) override {
    gClientConnected = true;
    sendTxLine("OK CONNECTED");
  }
  void onDisconnect(BLEServer*) override {
    gClientConnected = false;
    BLEDevice::startAdvertising();
  }
};

class RxCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* c) override {
    String v = c->getValue().c_str();
    if (v.length() == 0) return;
    gRxBuffer += v;

    int nl;
    while ((nl = gRxBuffer.indexOf('\n')) >= 0) {
      String line = gRxBuffer.substring(0, nl);
      gRxBuffer.remove(0, nl + 1);
      handleCommandLine(line);
    }

    if (gRxBuffer.length() > 2048) {
      gRxBuffer = "";
      sendTxLine("ERR RX_OVERFLOW");
    }
  }
};

// ==================== UI tick ====================
static void uiTick() {
  static bool touchDown = false;
  static int pressX = 0;
  static int pressY = 0;
  static uint32_t pressStartMs = 0;
  static uint32_t lastReleaseMs = 0;

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
      lastReleaseMs = now;
    }
    touchDown = false;
  }

  autoDetectTick();
  readAutoReturnTick();
  uiRedrawIfNeeded();
}

// ==================== Arduino setup / loop ====================
void setup() {
  Serial.begin(115200);
  delay(300);

  loadLanguage();
  loadDisplayInversion();
  loadCalibration();
  loadMaterials();
  loadManufacturers();

  gNfcMutex = xSemaphoreCreateMutex();

  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  ts.begin(touchscreenSPI);
  ts.setRotation(TFT_ROT);

  tft.init();
  applyDisplayInversion();
  tft.setRotation(TFT_ROT);
  tft.fillScreen(TFT_BLACK);
  tft.setTextFont(2);
  tft.setTextSize(1);

  TFT_W = tft.width();
  TFT_H = tft.height();

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
    }
  }

  BLEDevice::init(BLE_DEVICE_NAME);
  gServer = BLEDevice::createServer();
  gServer->setCallbacks(new ServerCallbacks());

  BLEService* service = gServer->createService(SERVICE_UUID);

  gTx = service->createCharacteristic(TX_UUID, BLECharacteristic::PROPERTY_NOTIFY);
  gTx->addDescriptor(new BLE2902());

  gRx = service->createCharacteristic(
    RX_UUID,
    BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
  );
  gRx->setCallbacks(new RxCallbacks());

  service->start();

  BLEAdvertising* adv = BLEDevice::getAdvertising();
  adv->addServiceUUID(SERVICE_UUID);
  adv->setScanResponse(true);
  adv->setMinPreferred(0x06);
  adv->setMinPreferred(0x12);
  BLEDevice::startAdvertising();

  sendTxLine("OK READY");
  needRedraw = true;
}

void loop() {
  uiTick();
  delay(15);
}