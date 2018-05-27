// tlacitka 
#define TL_P 8    // S2 - plus
#define TL_M 9    // S1 - minus
#define TL_A A5   // S3 - potvrzeni

//senzor
#define pinDHT 7

// signalizacni led - otevreno(sviti)/zavreno(nesviti)
#define LED_OKNO 13

// Rotační enkodér KY-040
// proměnné pro nastavení propojovacích pinů
int pinCLK = 6;
int pinDT  = 5;
int pinSW  = 10;

#define FILE_NAME "data.csv"



#define typDHT11 DHT11



#define MAIN_MENU 1
#define NAST_MENU 2

#define TXT_KROK_ZPET "Zpet..."

#define IDD_MENU_NAST 1
#define TXT_MENU_NAST "1. Mezni hodnoty"
#define IDD_MENU_KONN 2
#define TXT_MENU_KONN TXT_KROK_ZPET
/*
#define IDD_MENU_TEST 3
#define TXT_MENU_TEST "3. Testovani"
*/

#define IDD_NAST_1_1_L1 1
#define TXT_NAST_1_1_L1 "1.1 L1-zavrit"

#define IDD_NAST_1_2_H1 2
#define TXT_NAST_1_2_H1 "1.2 H1-otevrit"

#define IDD_NAST_1_3_H2 3
#define TXT_NAST_1_3_H2 "1.3 H2-zavrit"

#define IDD_NAST_1_4_Z5 4
#define TXT_NAST_1_4_Z5 TXT_KROK_ZPET


#define REZIM_MAN_ZKR "man"
#define REZIM_AUT_ZKR "aut"
#define REZIM_SIM_ZKR "sim"
#define REZIM_NAS_ZKR "nas"

#define REZIM_MAN_AKT 1
#define REZIM_AUT_AKT 2
#define REZIM_SIM_AKT 3
#define REZIM_NAS_AKT 4

#define STAV_OTEVREN "Otevreno"
#define STAV_ZAVRENO "Zavreno "



// lcd, i2c, address 0x20
#include <LiquidTWI2.h>
LiquidTWI2 lcd(0x20);


// cidlo vlhkosti teploty
#include "DHT.h"

// karta
/*
#include <SPI.h>
#include <SD.h>
 

//rtc
#include <Wire.h>
#include <TimeLib.h>
#include <DS1307RTC.h>
*/



// připojení potřebné knihovny
//#include <ESP8266WiFi.h>




// nastavení čísla pinu s připojeným DHT senzorem
DHT mojeDHT(pinDHT, typDHT11);


byte iAktRezim = REZIM_AUT_AKT;
boolean lStavOkna = false;


int iAktuMenu = MAIN_MENU;
int iAktRMenu = 1;

String cMenuMain [2] {
    TXT_MENU_NAST,
    TXT_MENU_KONN /*,
    TXT_MENU_TEST */
  };

String cMenuNast [4] {
    TXT_NAST_1_1_L1,
    TXT_NAST_1_2_H1,
    TXT_NAST_1_3_H2,
    TXT_NAST_1_4_Z5
  }; 

byte pozicMainMenu = sizeof(cMenuMain) / sizeof(String);
byte pozicNastMenu = sizeof(cMenuNast) / sizeof(String);

byte iMaxRMenu = pozicMainMenu;

boolean lPohotMonitor = true;
boolean lNastavHodnL1 = false;
boolean lNastavHodnH1 = false;
boolean lNastavHodnH2 = false;
boolean lNastavHodnSe = false;


// init hodnoty
byte iHodnotaL1 = 40;
byte iHodnotaH1 = 60;
byte iHodnotaH2 = 72;
byte iHodnotaSV = 45 /*= 52*/ ;   // hodnota vlhkosti
byte iHodnotaST = 22 /*= 23*/ ;   // hodnota teploty

#define OKNO_ZAV 0
#define OKNO_OTE 1

byte oknoZav[] = 
    {
        B11111,
        B10101,
        B10101,
        B11111,
        B10101,
        B10101,
        B11111,
        B00000
    };

byte oknoOte[] = 
    {
        B11111,
        B10001,
        B10001,
        B10001,
        B10001,
        B10001,
        B11111,
        B00000
    };


unsigned long previousMillis = 0;
const long interval = 30000;           // interval mereni 30sek


// proměnné pro uložení pozice a stavů pro určení směru
// a stavu tlačítka
int poziceEnkod = 0;
int stavPred;
int stavCLK;
int stavSW;



boolean lJeWifiOK = false;



void setup() {

    pinMode(TL_P, INPUT);
    pinMode(TL_M, INPUT);
    pinMode(TL_A, INPUT);

    pinMode(LED_OKNO, OUTPUT);

    // nastavení propojovacích pinů jako vstupních
    pinMode(pinCLK, INPUT);
    pinMode(pinDT, INPUT);
    // nastavení propojovacího pinu pro tlačítko
    // jako vstupní s pull up odporem
    pinMode(pinSW, INPUT);
    // načtení aktuálního stavu pinu CLK pro porovnávání
    stavPred = digitalRead(pinCLK);



    Serial.begin(9600);
    Serial1.begin(115200);


//while (!Serial);

   // ZobrazHlavniMenu();
   

    // inicializace LCD
    initLcd();
    
    // init wifi
    initWifi();

    // inicializace DHT
    initDht();

    // inicializace karty
//    initSdc();

    // precte hodnotu senzoru DHT
    ctiTepVlh();



   ZobrazPracoPanel();
   VyhodnotitLimity();
   SignalizaceNaLed();
}




void loop() {
  
    // put your main code here, to run repeatedly:
while(Serial.available()){
        Serial1.println(Serial.read());    
    }
    while(Serial1.available()){
        Serial.println(Serial1.read());    
    }
    boolean lJeDoPlu = false;
    boolean lJeDoMin = false;

    boolean lJeZmena = false;
    unsigned long currentMillis = millis();




    if ((iAktRezim == REZIM_AUT_AKT) && currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis;

        Serial.println("Cteni dat senzoru");
        ctiTepVlh();
        lJeZmena = true;
    }


    // dig encoder
    // načtení stavu pinu CLK
    stavCLK = digitalRead(pinCLK);

    // pokud je stav CLK odlišný od předchozího měření,
    // víme, že osa byla otočena
    if (stavCLK != stavPred) {
        // pokud stav pinu DT neodpovídá stavu pinu CLK,
        // byl pin CLK změněn jako první a rotace byla
        // po směru hodin, tedy vpravo
        if (digitalRead(pinDT) != stavCLK) {
            // vytištění zprávy o směru rotace a přičtení
            // hodnoty 1 u počítadla pozice enkodéru
            lJeDoPlu = true;
            lJeDoMin = false;
        }
        // v opačném případě, tedy pin DT byl změněn
        // jako první, se jedná o rotaci
        // proti směru hodin, tedy vlevo
        else {
            // vytištění zprávy o směru rotace a odečtení
            // hodnoty 1 u počítadla pozice enkodéru
            lJeDoMin = true;
            lJeDoPlu  = false;
        }
    }
    // uložení posledního stavu pinu CLK
    // jako reference pro další porovnávání
    stavPred = stavCLK;








    // tlacitko PLUS
    if (digitalRead(TL_P)==LOW || lJeDoPlu){

        // rolovani pohotovostnim - rezimem 
        if (lPohotMonitor) {
            if (iAktRezim == REZIM_NAS_AKT) {
                iAktRezim = REZIM_MAN_AKT;
            } else iAktRezim++;
            lJeZmena = true;
        } else

        // nastaveni H1
        if (lNastavHodnH1) {
            if (iHodnotaH1 + 1 < iHodnotaH2) {
                iHodnotaH1++;
                lJeZmena = true;
            }
        } else

        // nastaveni H2
        if (lNastavHodnH2) {
            if (iHodnotaH2 + 1 < 100) {
                iHodnotaH2++;
                lJeZmena = true;
            }
        } else

        // nastaveni L1
        if (lNastavHodnL1) {
            if (iHodnotaL1 + 1 < iHodnotaH1) {
                iHodnotaL1++;
                lJeZmena = true;
            }
        } else

        // nastaveni Se
        if (lNastavHodnSe) {
            if (iHodnotaSV + 1 < 100) {
                iHodnotaSV++;
                lJeZmena = true;
            }
        } else


        // menu
        if (iAktRMenu < iMaxRMenu) {
            iAktRMenu++;
            lJeZmena = true;
        }
    } else



    // tlacitko MINUS
    if (digitalRead(TL_M)==LOW || lJeDoMin){
    
        // rolovani pohotovostnim 
        if (lPohotMonitor) {
            if (iAktRezim == REZIM_MAN_AKT) {
                iAktRezim = REZIM_NAS_AKT;
            } else iAktRezim--;
            lJeZmena = true;
        } else


        // nastaveni H1
        if (lNastavHodnH1) {
            if (iHodnotaH1 - 1 > iHodnotaL1) {
                iHodnotaH1--;
                lJeZmena = true;
            }
        } else
    
        // nastaveni H2
        if (lNastavHodnH2) {
            if (iHodnotaH2 - 1 > iHodnotaH1) {
                iHodnotaH2--;
                lJeZmena = true;
            }
        } else
    
        // nastaveni L1
        if (lNastavHodnL1) {
            if (iHodnotaL1 - 1 > 0) {
                iHodnotaL1--;
                lJeZmena = true;
            }
        } else
    
        // nastaveni Se
        if (lNastavHodnSe) {
            if (iHodnotaSV - 1 > 0) {
                iHodnotaSV--;
                lJeZmena = true;
            }
        } else


        // menu
        if (iAktRMenu > 1) {
            iAktRMenu--;
            lJeZmena = true;
        }
    }


    // načtení stavu pinu SW - tlačítko
    stavSW = digitalRead(pinSW);

    // potvrzovaci tlacitko
    if (analogRead(TL_A)>200 || stavSW == 0) {
    
        delay(500);
        // aSerial.println(String("-- volba: ") + iAktRMenu + String(" v menu: ") + iAktuMenu);

        // pohotovost
        if (lPohotMonitor) {
            if (iAktRezim == REZIM_MAN_AKT) {
                lStavOkna = !lStavOkna;
                SignalizaceNaLed();
//                zapisujDataNaKartu();
                zapisujNaWeb();

            } else
            if (iAktRezim == REZIM_SIM_AKT) {
                lNastavHodnSe = true;
                lPohotMonitor = false;

            } else
            if (iAktRezim == REZIM_NAS_AKT) {
                lPohotMonitor = false;
            }

            //Serial.println(lStavOkna);

            lJeZmena = true; 
        } else

        // bylo nastavovani SE v simulatoru
        if (lNastavHodnSe) {
            lNastavHodnSe = false;
            lPohotMonitor = true;
            lJeZmena = true;
        } else


        //prechod do menu nastaveni
        if (iAktuMenu == MAIN_MENU && iAktRMenu == IDD_MENU_NAST) {
            iMaxRMenu = pozicNastMenu;            
            iAktuMenu = NAST_MENU;
            lJeZmena = true;
        } else
        

        //krok zpet z menu nastaveni
        if (iAktuMenu == MAIN_MENU && iAktRMenu == IDD_MENU_KONN) {
            iAktRMenu = IDD_MENU_NAST;
            lPohotMonitor = true;
            lJeZmena = true;
        } else


        //volba nastaveni H1 z menu nastaveni
        if (iAktuMenu == NAST_MENU && iAktRMenu == IDD_NAST_1_2_H1) {
            lNastavHodnH1 = !lNastavHodnH1;
            if (lNastavHodnH1 == true) {
                Serial.println(String("Nastav H1 (") + (iHodnotaL1 + 1) + String("-") + (iHodnotaH2 - 1) + String(")") );


                hodNaLcdText(1, 1, String("Nastav H1  ") + (iHodnotaL1 + 1) + String("-") + (iHodnotaH2 - 1) + String(")"));
                vycistiRadek(2);
                hodNaLcdHodn(2, 1, iHodnotaH1);
                blikejPozici(2, 2);
                lJeZmena = false;

            } else
            {
                Serial.println(String("H1 nastavena."));
                lJeZmena = true;
            }
        } else


        //volba nastaveni H2 z menu nastaveni
        if (iAktuMenu == NAST_MENU && iAktRMenu == IDD_NAST_1_3_H2) {
            lNastavHodnH2 = !lNastavHodnH2;
            if (lNastavHodnH2 == true) {
                Serial.println(String("Nastav H2 (>") + iHodnotaH1 + String(")"));

                hodNaLcdText(1, 1, String("Nastav H2    >") + iHodnotaH1 + String(")"));
                vycistiRadek(2);
                hodNaLcdHodn(2, 1, iHodnotaH2);
                blikejPozici(2, 2);
                lJeZmena = false;
            } else
            {
                Serial.println(String("H2 nastavena."));
                lJeZmena = true;
            }
        } else


        //volba nastaveni L1 z menu nastaveni
        if (iAktuMenu == NAST_MENU && iAktRMenu == IDD_NAST_1_1_L1) {
            lNastavHodnL1 = !lNastavHodnL1;
            if (lNastavHodnL1 == true) {
                Serial.println(String("Nastav L1 (<") + iHodnotaH1 + String(")"));

                hodNaLcdText(1, 1, String("Nastav L1    <") + iHodnotaH1);
                vycistiRadek(2);
                hodNaLcdHodn(2, 1, iHodnotaL1);
                blikejPozici(2, 2);
                lJeZmena = false;
            } else
            {
                Serial.println(String("L1 nastavena."));
                lJeZmena = true;
            }
        } else


        //volba zpet z menu nastaveni
        if (iAktuMenu == NAST_MENU && iAktRMenu == IDD_NAST_1_4_Z5) {
            iMaxRMenu = pozicMainMenu;            
            iAktuMenu = MAIN_MENU;
            iAktRMenu = 1;
            lJeZmena = true;
        } 

    }




    if (lJeZmena==true){

        // pohotovost rezim
        if (lPohotMonitor) {

            if (iAktRezim == REZIM_AUT_AKT) {
                VyhodnotitLimity();
                SignalizaceNaLed();
//                zapisujDataNaKartu();
                zapisujNaWeb();
            }


            ZobrazPracoPanel();
        } else

        // tisk nastavovane hodnoty SE
        if (lNastavHodnSe) {

            Serial.println(String("Hodnota Se: ") + iHodnotaSV);
            VyhodnotitLimity();
            SignalizaceNaLed();
//            zapisujDataNaKartu();
            zapisujNaWeb();

            hodNaLcdHodn(2, 6, iHodnotaSV);
            hodNaLcdPkto(2, 16);
            //hodNaLcdText(2, 9, OtevrZavre());
            blikejPozici(2, 7);

        } else


        // tisk nastavovane hodnoty H1
        if (lNastavHodnH1) {
            Serial.println(String("Hodnota H1: ") + iHodnotaH1);
            hodNaLcdHodn(2, 1, iHodnotaH1);
            blikejPozici(2, 2);
        } else

 
        // tisk nastavovane hodnoty H2
        if (lNastavHodnH2) {
            Serial.println(String("Hodnota H2: ") + iHodnotaH2);
            hodNaLcdHodn(2, 1, iHodnotaH2);
            blikejPozici(2, 2);
        } else

 
        // tisk nastavovane hodnoty L1
        if (lNastavHodnL1) {
            Serial.println(String("Hodnota L1: ") + iHodnotaL1);
            hodNaLcdHodn(2, 1, iHodnotaL1);
            blikejPozici(2, 2);
        } else


        // tisk hlavniho menu
        if (iAktuMenu == MAIN_MENU) {
            ZobrazHlavniMenu();    
        } else


        // tisk nastavovaciho menu
        if (iAktuMenu == NAST_MENU) {
            ZobrazNastavMenu();
        }

        delay(200);
    }
 
}



void ZobrazHlavniMenu(){

    String cRadek1 = cMenuMain[iAktRMenu-1];
    String cRadek2;

    Serial.println("------- menu --------");  
    Serial.println(cRadek1);

    
    vymazatDisplej();
    lcd.setCursor(0, 0);
    lcd.print(cRadek1);


    if (iAktRMenu < pozicMainMenu) {
        cRadek2 = cMenuMain[iAktRMenu];
        Serial.println(cRadek2);  

        lcd.setCursor(0, 1);
        lcd.print(cRadek2);

    }
   

    blikejPozici(1, 1);
}


void ZobrazNastavMenu(){
 
    String HodnPar1;
    String HodnPar2;

    if (iAktRMenu == 1) {
        HodnPar1 = String(" (") + String(iHodnotaL1) + String(")");
        HodnPar2 = String(" (") + String(iHodnotaH1) + String(")");
    } else 
    if (iAktRMenu == 2) {
        HodnPar1 = String(" (") + String(iHodnotaH1) + String(")");
        HodnPar2 = String(" (") + String(iHodnotaH2) + String(")");
    } else
    if (iAktRMenu == 3) {
        HodnPar1 = String(" (") + String(iHodnotaH2) + String(")");
        HodnPar2 = "";
    }

    String cRadek1 = cMenuNast[iAktRMenu-1];
    String cRadek2;



    Serial.println("----- nastaveni -----");
    Serial.println(cRadek1 + HodnPar1 );

    vymazatDisplej();

    lcd.setCursor(0, 0);
    lcd.print(cRadek1);



    if (iAktRMenu < pozicNastMenu) {
        cRadek2 = cMenuNast[iAktRMenu];
        Serial.println(cRadek2 + HodnPar2);  

        lcd.setCursor(0, 1);
        lcd.print(cRadek2);

    }
   
    Serial.println("------- konec -------");

    blikejPozici(1, 1);

}

String OtevrZavre(){
    if (lStavOkna) return STAV_OTEVREN;
    else return STAV_ZAVRENO;
}

String vratRezim(){
    String rRezim = "R";

    if (iAktRezim == REZIM_MAN_AKT) rRezim += REZIM_MAN_ZKR;
    else
    if (iAktRezim == REZIM_AUT_AKT) rRezim += REZIM_AUT_ZKR;
    else
    if (iAktRezim == REZIM_SIM_AKT) rRezim += REZIM_SIM_ZKR;
    else
    if (iAktRezim == REZIM_NAS_AKT) rRezim += REZIM_NAS_ZKR;
    else rRezim += "?";

    return rRezim;
}


void ZobrazPracoPanel(){

    String cRezim = vratRezim();

    String cLine1;
    String cLine2;

    cLine1 = "L:" + String(iHodnotaL1) + " H1:" + String(iHodnotaH1) + " H2:" + String(iHodnotaH2);




    //cLine2 = cOznaA + String(iHodnotaSV) + "% " + "R:"  + " " + cOznaR + "M " + cOznaS + "S:" + OtevrZavre();


    cLine2 = cRezim + "    " + OtevrZavre();


    // monitor
    Serial.println(cLine1);
    Serial.println(cLine2);


    lcd.setCursor(0, 0);
    lcd.print(cLine1);

    hodNaLcdText(2, 1, "                ");
    hodNaLcdText(2, 1, cRezim);

    if (iAktRezim == REZIM_AUT_AKT || iAktRezim == REZIM_SIM_AKT) {
        hodNaLcdHodn(2, 6, iHodnotaSV);
        hodNaLcdText(2, 8, "%");

        if (iAktRezim == REZIM_AUT_AKT ) {
            hodNaLcdHodn(2, 10, iHodnotaST);
            hodNaLcdText(2, 12, String(char(223)) + "C");
        }
    }  

    hodNaLcdPkto(2, 16);
    //hodNaLcdText(2, 9, OtevrZavre());

}

void initDht(){
    mojeDHT.begin();

    vymazatDisplej();
    hodNaLcdText(1, 1, "Inicializace DHT");
    delay(1000);
}

/*
void initSdc(){

    vymazatDisplej();
    hodNaLcdText(1, 1, "Inicializace SD");
 
    if (!SD.begin(4)) {
        hodNaLcdText(2, 1, "chyba SD karty");
    } else {
        hodNaLcdText(2, 1, "SD karta OK");
    }
    delay(2000);

}
*/

void initLcd(){

    // LCD setup
    lcd.setMCPType(LTI_TYPE_MCP23008);

    lcd.begin(16, 2);
    lcd.createChar(OKNO_ZAV, oknoZav);
    lcd.createChar(OKNO_OTE, oknoOte);
    
    lcd.setBacklight(HIGH);

    vymazatDisplej();
    hodNaLcdText(1, 1, "Inicializace LCD");
    hodNaLcdText(2, 1, "LCD OK");

    delay(2000);
}

void vymazatDisplej(){
    lcd.clear();
}


void hodNaLcdHodn(byte bRow, byte bCol, byte bVal ){
    
    String sVal = String(bVal);

    if (sVal.length() == 1) sVal = " " + sVal;

    lcd.setCursor(bCol - 1, bRow - 1);
    lcd.print(sVal);
}

void hodNaLcdText(byte bRow, byte bCol, String sVal ){
    lcd.setCursor(bCol - 1, bRow - 1);
    lcd.print(sVal);
}

void hodNaLcdPkto(byte bRow, byte bCol){
    lcd.setCursor(bCol - 1, bRow - 1);
    if (lStavOkna) lcd.write(OKNO_OTE);
    else lcd.write(OKNO_ZAV);
}

void SignalizaceNaLed(){
    if (lStavOkna) digitalWrite(LED_OKNO, HIGH);
    else digitalWrite(LED_OKNO, LOW);
}

void VyhodnotitLimity(){
    if (iHodnotaSV >= iHodnotaH1 && iHodnotaSV < iHodnotaH2) {
        lStavOkna = true;
    } else {
    
      if (iHodnotaSV >= iHodnotaH2) lStavOkna = false;
    
        else {
          if (iHodnotaSV <= iHodnotaL1) lStavOkna = false; 
          else {
          lStavOkna = false;
          }
        }
    }
}

void blikejPozici(byte bRow, byte bCol){
    lcd.setCursor(bCol - 1, bRow - 1);
    lcd.blink();
}

void vycistiRadek(byte bRow){
    hodNaLcdText(bRow, 1, "                ");
}


void ctiTepVlh(){

  float tep = mojeDHT.readTemperature();
  float vlh = mojeDHT.readHumidity();
  // kontrola, jestli jsou načtené hodnoty čísla pomocí funkce isnan
  if (isnan(tep) || isnan(vlh)) {
    // při chybném čtení vypiš hlášku
    Serial.println("Chyba pri cteni z DHT senzoru!");
  } else {
    // pokud jsou hodnoty v pořádku,
    // vypiš je po sériové lince
    Serial.print("Teplota: "); 
    Serial.print(tep);
    Serial.print(" Vlhkost: "); 
    Serial.print(vlh);
    Serial.println(" %");
  
    iHodnotaSV = int(vlh);
    iHodnotaST = int(tep);
  }


}

void initWifi(){


    vymazatDisplej();
    hodNaLcdText(1, 1, "INIT WIFI...");


    if (PrikazWifi("AT", 2000, "OK", "1. WIFI modul") == false) {
      hodNaLcdText(2, 1, "NENI MODUL");
      delay(3000);
      return;
    }

    hodNaLcdText(2, 1, "modul:OK,cekej..");
    
    if (PrikazWifi("AT+CWJAP=\"wifiname\",\"wifipass\"", 13000, "WIFI|CONN", "2. Connect to WIFI" ) == false) {
      
      hodNaLcdText(2, 1, "NENI WIFI SPOJENI");
      delay(3000);
      return;
    }

    hodNaLcdText(2, 1, "spojeni:OK,...");
    lJeWifiOK = true;
    delay(3000);

}


void zapisujNaWeb(){

    if (lJeWifiOK) {
      if (PrikazWifi("AT+CIPSTART=\"TCP\",\"api.thingspeak.com\",80", 5000, "OK", "3. Connect via TCP") == false) return;
      if (PrikazWifi("AT+CIPSEND=62", 3000, "OK", "4. AT+CIPSEND") == false) return;
      if (PrikazWifi("GET https://api.thingspeak.com/update.json?api_key=APIKEY&field1=" + String(iHodnotaSV)+"&field2="+String(iHodnotaST)+"&field3="+String(lStavOkna), 5000, "CLOSED", "5. SEND DATA") == false) return;
      Serial.println("zapsal jsem na web");
    } else {
      Serial.println("web neni pristupny");
    }
    
    

}


boolean PrikazWifi(String cmd, long cekej, char cOdpo[], String chyba){

    String odpo;

    Serial.println("[P]: " + cmd + " [T]: " + cekej + "ms [R]: " + cOdpo);

    Serial1.println(cmd);
    delay(cekej);

    while(Serial1.available()){

        char znak = Serial1.read();

Serial.print(".");
        Serial.print(znak);

        if (isAlphaNumeric(znak)) {
           odpo += String(znak);
           } else {
            odpo += "|";
            }
    }

    Serial.println("Odpoved: " + odpo);

    if (odpo.indexOf(cOdpo) + 1 == 0) {
        Serial.println(chyba + " :Chyba");
        Serial.println("");
        return false;
    } else {
        Serial.println(chyba + " :OK");
        Serial.println("");
        return true;
    }

    //Serial.println(odpo.indexOf(Hledam) + 1 , DEC );

}


/*
void zapisujDataNaKartu(){

    File myFile; 

    char cas[20];
    char sto;
    String dataString = ""; 
    tmElements_t tm;

    if (RTC.read(tm)) {

        sprintf(cas, "%02d.%02d.%04d %02d:%02d:%02d", tm.Day, tm.Month, tmYearToCalendar(tm.Year), tm.Hour, tm.Minute, tm.Second);
        Serial.println(cas);

        if (lStavOkna) sto = '1';
        else sto = '0';

        dataString = cas; 
        dataString = dataString + ";" + vratRezim();
        dataString = dataString + ";" + String(iHodnotaL1);
        dataString = dataString + ";" + String(iHodnotaH1);
        dataString = dataString + ";" + String(iHodnotaH2);
        dataString = dataString + ";" + String(iHodnotaSV);
        dataString = dataString + ";" + String(iHodnotaST);
        dataString = dataString + ";" + sto;


        myFile = SD.open(FILE_NAME, FILE_WRITE); 
      
        if (myFile) {
            myFile.println(dataString);
            myFile.close();
        }
    }
}
*/

;

