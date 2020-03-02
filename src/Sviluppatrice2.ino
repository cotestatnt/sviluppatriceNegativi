#define INIT_EE false
#define DEBUG true

#if DEBUG
  #define sprintln(a)  (Serial.println(a))
  #define sprint(a)    (Serial.print(a))
#else
  #define sprintln(a) 
  #define sprint(a)  
#endif

#include <avr/pgmspace.h>
#include <EEPROM.h>
#include <Wire.h>
#include <OneWire.h>
#include <DS18B20.h>
#include <RotaryEncoder.h>
#include <hd44780.h>
#include <hd44780ioClass/hd44780_I2Cexp.h>
#include <AutoPID.h>

// Change related to relay board specs
#define OFF HIGH
#define ON  LOW

// Input and output pins
constexpr auto PWM_OUT = 6;
constexpr auto IN_A = 7;
constexpr auto IN_B = 8;
constexpr auto CLK = A2;
constexpr auto DT = A3;
constexpr auto SW = A1;
constexpr auto ONE_WIRE_BUS = 2;
constexpr auto RELAY = 12;
constexpr auto RESET = 4;

// ************************************************
// LCD Variables and constants
// ************************************************
#define HD44780_LCDOBJECT
constexpr auto WIRECLOCK = 400000L;
hd44780_I2Cexp lcd(0x27, I2Cexp_PCF8574, 0, 1, 2, 4, 5, 6, 7, 3, HIGH); // with rw support

constexpr uint8_t LCD_COLS = 20;
constexpr uint8_t LCD_ROWS = 4;
constexpr uint8_t LAST_COL = LCD_COLS - 1;

constexpr uint8_t row0 = 0, row1 = 64, row2 = 20, row3 = 84;            // LCD  memory address for each row
uint8_t currentPage = 0, oldPage = 255;

uint32_t selTime, startTime, lcdTime, actRevTime;
#define NAME_LENGTH 16
#define MAX_JOB_STEPS 5

int selStep = 0, lastStep = 7, maxStep = 20;
int selJob = 0, lastJob, maxJob = 20;
int actJobStep = 0;  
int editStep = 0;  // Index for editing job's step

bool isRevActive = false;  
bool waitTemperature = false;
bool runPID = false;
bool runJob = false;
bool doPreHeat = false;

char txBuffer[50];
char lcdBuffer[LCD_COLS];

typedef struct {
  char stepName[NAME_LENGTH] = "ABC123";
  uint16_t durationTime = 60;
  int8_t speed = 100;           // -127 0 127: CCW 0 CW with variable speed
  uint8_t temperature = 25;     // 0-255 max
  uint8_t reverse = 0;
  uint8_t revTime = 0;
} Step;
Step activeStep;

typedef struct {
  char jobName[NAME_LENGTH] = "ABC123";  
  int8_t steps[MAX_JOB_STEPS] = {0, 1, 2, 3, 4};    
} Job;
Job activeJob;


// ************************************************
// PID Variables and constants
// ************************************************
double Setpoint, ActualTemp, Output, preHeat = 30.0F; 
double Kp = 50.0F, Ki = 5.0F, Kd = 80.0F; 
constexpr auto LOW_HYST = 2.0;
constexpr auto HIGH_HYST = 0.1;

#define WindowSize  5000
unsigned long windowStartTime;
unsigned int minStartTime = 100;
volatile unsigned long onTime = 0;
AutoPID myPID(&ActualTemp, &Setpoint, &Output, 0, WindowSize, Kp, Ki, Kd);
constexpr uint16_t pidLoopInterval = 100;

// ************************************************
// DS18B20 Variables and constants
// ************************************************
OneWire oneWire(ONE_WIRE_BUS);
DS18B20 sensors(&oneWire);
//DeviceAddress ds18b20;

// ************************************************
// Rotary Encoder Variables and constants
// ************************************************
RotaryEncoder encoder(CLK, DT);
uint32_t pressTime = millis(), pTime, updateTempTime;
uint8_t pressType = 0;
constexpr auto LONG = 1000;		// Tempo "pressione tasto lunga" (Programmazione ON-OFF)
constexpr auto MEDIUM = 250;    // Tempo "pressione tasto media" (Switch tra i parametri in programmazione) 
constexpr auto SHORT = 5;      // Tempo "pressione tasto breve" (Modifica dei parametri)
long encPos = 0;


// Funzioni per gestione EEPROM
#include "eeprom_handler.h"

// Funzioni per visualizzare testo su display
#include "lcd_menu.h"

// ******************* Pin changhe Interrupt Handler *************************
ISR(PCINT1_vect) {
  encoder.tick(); // just call tick() to check the state.
}

// ************************** Timer Interrupt Handler *************************
// ******************* Called every 100ms to drive the output *****************
ISR(TIMER1_COMPA_vect) {
  if(runPID){
    myPID.run();  
    onTime = Output;  
  }
  else 
    onTime = 0;  

  if(millis() - windowStartTime > WindowSize)
     windowStartTime += WindowSize;
 
  if((onTime > minStartTime) && (onTime > (millis() - windowStartTime)))
     digitalWrite(RELAY, ON);  
  else  
     digitalWrite(RELAY, OFF);
}
 

/////////////////////////////////////////////////////////////////////////////
/////////////////////////// Ciclo di Setup //////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
void setup() {
  

#if DEBUG
  Serial.begin(115200);
#endif

  pinMode(SW, INPUT_PULLUP);
  pinMode(DT, INPUT_PULLUP);
  pinMode(CLK, INPUT_PULLUP);
  //pinMode(RESET, INPUT_PULLUP);
  pinMode(PWM_OUT, OUTPUT);
  pinMode(IN_A, OUTPUT);
  pinMode(IN_B, OUTPUT);
  
  digitalWrite(RELAY, HIGH);
  pinMode(RELAY, OUTPUT);

  PCICR |= (1 << PCIE1);                      // This enables Pin Change Interrupt 1 (Port C).
  PCMSK1 |= (1 << PCINT10) | (1 << PCINT11);  // This enables the interrupt for pin 2 and 3 of Port C.

  int status = lcd.begin(LCD_COLS, LCD_ROWS);
  if (status) {
    status = -status; // convert negative status value to positive number
    hd44780::fatalError(status); // does not return
  }
  lcd.backlight();
  lcd.clear();
  lcd.print(__DATE__);
  lcd.setCursor(0, 1);
  lcd.print(__TIME__);
  delay(1000);

  // Dallas DS18B20
  sensors.begin();  
  /*
  sprintf_P(txBuffer, PSTR("Locating devices... found %d devices\n"), sensors.getDeviceCount());
  sprintln(txBuffer);
  
  if (!sensors.getAddress(ds18b20, 0)) sprintln(F("Unable to find address for Device 0"));
  */

  sensors.setResolution(11);
  sensors.requestTemperatures();
  ActualTemp = sensors.getTempC();
  
  // Ripristino la memoria se avvio con INIT_EE == true 
  #if INIT_EE
    initEeprom();
    lcd.print(F("Clear EEprom"));
    delay(1000);
    lcd.clear();
  // Altrimenti blocco esecuzione codice
  #else    
    byte _init;
    EEPROM.get(INIT_EE_ADR, _init);
    if(_init != 0x77){
      lcd.setCursor(0, 4);
      lcd.print(msg_init_ee);
      Serial.print("READ INIT BYTE: ");
      Serial.println(_init, HEX);
      sprintln(msg_init_ee);
      while(true){;}
    }    
  #endif

  readEeprom();
  loadStepEE(0);
  loadJobEE(0);

  // Initialize the PID and related variables  
  myPID.setGains(Kp,Ki,Kd);
  myPID.setTimeStep(pidLoopInterval);     
  windowStartTime = millis();
 
   // Run Timer1 interrupt every 100 ms
  setupTimer1() ;
  lcd.clear();
}


/////////////////////////////////////////////////////////////////////////////
/////////////////////////// Ciclo Principale ////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
void loop() {
  // Aggiornamento posizione cursore LCD
  getCursor();

  // Gestione pulsante
  pressTime = millis();
  checkButton();
  
  // Gestione menu LCD e selezioni con encoder
  encHandler();
  updatePage();  

  if( doPreHeat ){
    runPID = true;
    if(ActualTemp > Setpoint)
      currentPage = P_MAIN;
  }

  if(millis() - updateTempTime > 1000){ 
    updateTempTime = millis();
    sensors.requestTemperatures();  
    /*
    // Wait until ADC conversion was done (with timeout)   
    uint32_t readTime = millis();
    while (!sensors.isConversionComplete()){
      if(millis() - readTime > 400)
        break;
    }  
    */
    ActualTemp = sensors.getTempC(); 

    if(runPID){
      snprintf_P(txBuffer, 50, 
        PSTR("Setpoint: %d.%d; Input: %d.%d; Output: %d"), 
        (int)Setpoint, (int)(Setpoint*10.0)%10,
        (int)ActualTemp, (int)(ActualTemp*10.0)%10, (int)Output);
      sprintln(txBuffer); 
    }  
     
    if (currentPage == P_MAIN) {
      lcd.setCursor(0, 3);
      snprintf_P(buffer, LCD_COLS+1, PSTR("#####  %02d.%d %cC #####"), (int)ActualTemp, (int)(ActualTemp*10)%10, (char)223 );
      lcd.print(buffer);         
    } 

    if(currentPage == P_WAIT_TEMP ){
      upadteTempRow2();
      lcd.setCursor(0, 3);
      if(ActualTemp >= Setpoint) {              
        strcpy_P(buffer, msg_next);
        lcd.print(buffer);  
        lcd.setCursor(LAST_COL, 3); 
        lcd.blink();
      }
      else {                 
        strcpy_P(buffer, msg_empty);
        lcd.print(buffer);  
        lcd.noBlink();
      }   
    }

    if( currentPage == P_WAIT_STEP){
      uint8_t row = cursor.row;
      uint8_t col = cursor.col;
      lcd.setCursor(0, 3);        
      if(ActualTemp >= Setpoint){
        strcpy_P(buffer, msg_next);
        lcd.print(buffer);  
        lcd.blink();
      }              
      upadteTempRow2();
      lcd.setCursor(col, 3);       
    }

  }

  if(waitTemperature){
    if(ActualTemp >= Setpoint){
      waitTemperature = false;
    }  
  } 

  if (!SetupMode) {
    if(runPID){
      if(ActualTemp < Setpoint && waitTemperature){
        currentPage = P_WAIT_TEMP;     
        delay(100);   
        return;
      }
    }

    if (millis() - selTime >= 10000) {      
      if(currentPage == P_SELECT_STEP || currentPage == P_SELECT_JOB){
        currentPage = P_MAIN;
        lcd.noBlink();
      }
    }
  }

 

  if(runJob){    
    static bool runFirst = false;
    static bool loadFirst = false;

    // Run this block only the first time we need to load step  
    if(actJobStep < MAX_JOB_STEPS && !loadFirst){    
      loadFirst = true;	 
      selStep = activeJob.steps[actJobStep];
      sprint(F("Current active step: "));
      sprintln(actJobStep+1);
      
      loadStepEE(selStep);   
      Setpoint = activeStep.temperature;         
      // We need to wait water heating  
      if(Setpoint > ActualTemp){
        waitTemperature = true;
        runPID = true;          
        myPID.reset();
        myPID.setBangBang(LOW_HYST, HIGH_HYST);
        currentPage = P_WAIT_TEMP;
      }
      // No need to wait temperature
      else {
        waitTemperature = false;          
        runPID = false;
        Setpoint = 0;        
        currentPage = P_RUN_STEP;          
      }                 
    }   
    
    // Water is in temperature or not necessary, run selected step
    if ( waitTemperature == false ) {  
      // Update information on LCD panel
      if (millis() - lcdTime > 1000 && (currentPage != P_WAIT_STEP)) {
        lcdTime = millis();
        runningInfo();              
      }

      // Run this block only the first time to fix the start time and set motor speed
      if(!runFirst){
        runFirst = true;
        startTime = millis(); actRevTime = millis();      
        isRevActive = activeStep.speed > 0 ? true : false; 
        setMotor(activeStep.speed, isRevActive);	        
      }

      // Reverse direction of motor needed?
      if(activeStep.reverse){
        if ((millis() - actRevTime) >= (uint32_t)activeStep.revTime * 1000) {
          actRevTime = millis();
          isRevActive = !isRevActive;
          setMotor(activeStep.speed, isRevActive);      
        }
      }

      // Check if the step is done. If yes, wait click from user
      if ((millis() - startTime) >= (uint32_t)activeStep.durationTime * 1000) {           
        loadFirst = false; 
        runFirst = false;                   
        runJob = false;
        setMotor(0, isRevActive);        

        // Last step was done, init for the next job and exit
        if(actJobStep == MAX_JOB_STEPS-1){                
          actJobStep = 0;
          currentPage = P_DONE_JOB;        
          Setpoint = 0;
          myPID.reset();
          return;
        }         
        
        if(actJobStep < MAX_JOB_STEPS ){      
          uint8_t nextStep = 0;
     
          for(uint8_t i=actJobStep; i<MAX_JOB_STEPS; i++){
            nextStep = activeJob.steps[actJobStep+1];   
            if(nextStep == 0) {
              actJobStep++;  
              sprint("Try with next step ");
              sprintln(actJobStep);
            }           
                     
            if(actJobStep >= MAX_JOB_STEPS-1 ){
              actJobStep = 0;
              currentPage = P_DONE_JOB;        
              Setpoint = 0;
              myPID.reset();
              sprintln("Last step was done");
              return;
            }                       
          }

          loadStepEE(nextStep);  
          Setpoint = activeStep.temperature;   
           
          // Update page directly 
          menu_wait_step();   
          currentPage = P_WAIT_STEP;          
          oldPage = currentPage;  

          if(Setpoint > ActualTemp) {   
            //lcd.clear();                 
            lcd.setCursor(0, 3);
            lcd.print(F("< Attendi   Avanza >"));
            lcd.blink();
          }               
          lcd.setCursor(LAST_COL, 3);               
        }        
      }
    }    
  }


}



///////////////// Azioni da eseguire alla pressione dei pulsanti  ///////////////////////
void SingleClick() {  
  uint8_t row = cursor.row;
  uint8_t col = cursor.col;
  sprintln(__PRETTY_FUNCTION__);
  // sprint(F("Click from page "));  sprintln(currentPage);
  switch (currentPage) {

    case P_MAIN:
      selTime = millis();
      currentPage = P_SELECT_JOB;
      break;
    
    case P_ADD_JOB:
      if (col != LAST_COL && col < NAME_LENGTH-1) {
        col++;
        lcd.setCursor(col, row);
        uint8_t ch = lcd.read();
        lcd.setCursor(col, row);
        if(ch == ' '){
          lcd.setCursor(col-1, row);
          ch = lcd.read();
          lcd.write(ch);  
          lcd.setCursor(col, row);         
        }          
      }   
      selStep = 1;
      break;

    case P_EDIT_JOB:                
      lcd.clear();
      menu_add_job();
      currentPage = P_ADD_JOB;           
      return;
      
    case P_SELECT_JOB: 
      // Click -> Avvio del lavoro selezionata
      loadJobEE(selJob);
      runJob = true;
      actJobStep = 0;
      break;

    case P_DONE_JOB:
      // Lavoro selezionato concluso -> ritorno a pagina principale
      if(!runJob && currentPage != P_WAIT_STEP){
        currentPage = P_MAIN;
      }
      break;

    case P_WAIT_STEP:      
      if (col == 0 && row == 3){
        waitTemperature = true;
        runPID = true;
        lcd.noBlink();
      } 
      else {
        actJobStep++;      
        lcd.noBlink();  
        runJob = true;
        lcdTime = 0; 
      }      
      return;
    
    case P_WAIT_TEMP:
      actJobStep++;      
      lcd.noBlink();  
      runJob = true;
      lcdTime = 0;
      return;

    case P_EDIT_JOB_STEPS:     
      if(editStep < MAX_JOB_STEPS){
        #if DEBUG
          uint16_t eeAddress = STEP_EE + selStep * 22;
          char stepName[NAME_LENGTH];
          EEPROM.get(eeAddress, stepName);
          sprintf_P(txBuffer, PSTR("Add step %s in position %d"), stepName, editStep);
          sprintln(txBuffer);
        #endif

        activeJob.steps[editStep] = selStep;      
        editStep++;          
        menu_edit_job_steps(editStep, true);        
      }

      if(editStep >= MAX_JOB_STEPS ){
        #if DEBUG
          sprint(activeJob.jobName); sprint(F(": {"));
          for(uint8_t i = 0; i < MAX_JOB_STEPS; i++){  
            uint16_t eeAddress = STEP_EE + activeJob.steps[i] * 22;
            char stepName[NAME_LENGTH];
            EEPROM.get(eeAddress, stepName);
            sprint(stepName);
            sprint(F("; "));
          }
          sprintln(F("}"));   
        #endif
        currentPage = P_YES_NO;  
      }
      return;      
    
    case P_SET_PREHEAT:                 
      SetupMode = false;
      Setpoint = preHeat;   
      myPID.reset();
      myPID.setBangBang(LOW_HYST, HIGH_HYST);
      runPID = true;
      waitTemperature = true;
      doPreHeat = true;
      currentPage = P_WAIT_TEMP;                 
      return;
      
    case P_SETUP_MENU1:
      currentPage = cursor.row + 1 + P_SETUP_MENU1 ;
      // Se la prossima pagina è P_ADD_STEP, incremento contatore step
      if (currentPage == P_ADD_STEP) {
        sprint(F("Step n° ")); 
        sprintln(lastStep);
        selStep = lastStep;    
        loadStepEE(0);    
        lastStep++;
      }
      break;
    
    case P_SETUP_MENU2:
      currentPage = cursor.row + 1 + P_SETUP_MENU2 ;
      // Se la prossima pagina è P_ADD_JOB, incremento contatore job
      if (currentPage == P_ADD_JOB) {      
        EEPROM.get(LASTJOB_EE, lastJob);  
        sprint(F("Job n° ")); 
        sprintln(lastJob);
        selJob = lastJob;        
        lastJob++;
        strcpy(activeJob.jobName, String("").c_str());   
      }
      if (currentPage == BACK_SETUP1) 
        currentPage = P_SETUP_MENU1;
      if (currentPage == EXIT_SETUP) {
        SetupMode = false; 
        currentPage = P_MAIN;    
      }
      break;

    case P_ADD_STEP:
      if (col != LAST_COL && col < NAME_LENGTH-1) {
        col++;
        lcd.setCursor(col, row);
        uint8_t ch = lcd.read();
        lcd.setCursor(col, row);
        if(ch == ' '){
          lcd.setCursor(col-1, row);
          ch = lcd.read();
          lcd.write(ch);  
          lcd.setCursor(col, row);         
        }          
      }          
      break;

    case P_EDT_STEP:
      lcd.clear();
      menu_add_step();
      currentPage = P_ADD_STEP;
      sprint("Step name: ");
      sprintln(activeStep.stepName);
      break;

    case P_YES_NO:        
      if (col == LAST_COL){
        if(lastSetupMenu == P_SETUP_MENU1)
          writeStepEeprom(selStep);        
        else
          writeJobEeprom(selJob);        
        save_message();              
      } 
      editStep = 0;
      currentPage = lastSetupMenu;
      break;

    case P_SELECT_STEP: 
      // Click-> Avvio della singola fase selezionata      
      for(uint8_t i=0; i<MAX_JOB_STEPS-2; i++ )
        activeJob.steps[i] = 0;

      actJobStep = MAX_JOB_STEPS-1;
      activeJob.steps[actJobStep] = selStep;      
      runJob = true;
      break;
  }
    
  if (col == LAST_COL ){
    currentPage = pActive.next;
  }  

}

void MediumClick() {  
  sprintln(__PRETTY_FUNCTION__);
  uint8_t row = cursor.row;
  uint8_t col = cursor.col;  
  static uint8_t oldCol;
  
  if( currentPage == P_YES_NO){
    if (col == LAST_COL)    
      col = 0;  
    else 
      col = LAST_COL; 
    row = 3;
    lcd.setCursor(col, row);
    delay(100);
    return;
  }

  if( currentPage == P_WAIT_STEP){
    if (col == 0)    
      col = LAST_COL;  
    else 
      col = 0; 
    row = 3;
    lcd.setCursor(col, row);
    delay(100);
    return;
  }

  if( currentPage == P_MAIN){
    selTime = millis();
    currentPage = P_SELECT_STEP;
    return;
  }
  
  if( row == 3){
    row =1;
    lcd.setCursor(oldCol, row); 
  }
  else {
    oldCol = col;
    row = 3;
    col = LAST_COL;
    lcd.setCursor(col, row); 
  }

}

void LongClick() {
  SetupMode = !SetupMode;
  sprint(F("Setup mode: "));
  sprintln(SetupMode ? "true" : "false");
  if (SetupMode) {
    currentPage = P_SETUP_MENU1;
    oldPage = P_MAIN;
    lcd.clear();
    lcd.setCursor(5, 1);
    lcd.print(F("MENU SETUP"));  
    runPID = false;
    waitTemperature = false;
    doPreHeat = false;
    editStep = 0;
    delay(1000);
  }
  else {       
    currentPage = P_MAIN;
  }
}

// Gestione della selezione per emzzo dell'encoder rotativo
void encHandler() {
  uint8_t row = cursor.row;
  uint8_t col = cursor.col;
  int pos = encoder.getPosition();
  if (pos != 0) {         
    switch (currentPage) {
      case P_SET_PREHEAT:            
        preHeat = circularSumFloat(preHeat, pos*0.1, 15.0, 45.0);  
        lcd.setCursor(0, 3);
        snprintf_P(buffer, LCD_COLS+1, PSTR("#####  %02d.%01d %cC  #####"), (int)preHeat, (int)(preHeat*10.0)%10, (char)223 );
        lcd.print(buffer);        
        break;
      case P_SETUP_MENU1:
        (pos > 0) ? row = (row + 1) % LCD_ROWS : row = (row - 1) % LCD_ROWS;
        lcd.setCursor(col, row);
        break;
      case P_SETUP_MENU2:
        (pos > 0) ? row = (row + 1) % LCD_ROWS : row = (row - 1) % LCD_ROWS;
        lcd.setCursor(col, row);
        break;
      case P_ADD_STEP:
        editStepName(pos);
        break;
      case P_ADD_STEP_TIME:
        activeStep.durationTime = circularSum(activeStep.durationTime, pos, 5, 3600);
        clearLine(); lcd.setCursor(0, 1);
        lcd.print(activeStep.durationTime);
        break;
      case P_ADD_STEP_SPEED:
        activeStep.speed = circularSum(activeStep.speed, pos, -127, 127);
        clearLine(); lcd.setCursor(0, 1);
        lcd.print(activeStep.speed);
        break;
      case P_ADD_STEP_TEMP:
        activeStep.temperature = circularSum(activeStep.temperature, pos, 0, 40);
        clearLine(); lcd.setCursor(0, 1);
        lcd.print(activeStep.temperature);
        break;
      case P_ADD_STEP_REVERSE:
        activeStep.reverse = !activeStep.reverse;
        clearLine(); lcd.setCursor(0, 1);
        lcd.print(activeStep.reverse? "SI":"NO");
        break;
      case P_ADD_STEP_REV_TIME:
        activeStep.revTime = circularSum(activeStep.revTime, pos, 0, 250);
        clearLine(); lcd.setCursor(0, 1);
        lcd.print(activeStep.revTime);
        break;
      case P_EDT_STEP:
        selStep += pos;
        selStep = constrain(selStep, 1, lastStep - 1);
        sprintf_P(txBuffer, PSTR("Select step to edit: %d."), selStep);
        sprintln(txBuffer);  
        loadStepEE(selStep);
        menu_edit_step();
        break;

      // Selezione lavoro da eseguire (non in SetupMode)
      case P_SELECT_STEP:
        selStep += pos;
        selStep = constrain(selStep, 1, lastStep - 1);        
        sprintf_P(txBuffer, PSTR("Select step to run: %d."), selStep);
        sprintln(txBuffer);
        loadStepEE(selStep);
        menu_select_step();
        break;   

      case P_SELECT_JOB:
        selJob += pos;
        selJob = constrain(selJob, 0, lastJob - 1);        
        sprintf_P(txBuffer, PSTR("Select Job to run: %d."), selJob);
        sprintln(txBuffer);
        loadJobEE(selJob);
        menu_select_job();
        break;   
      
       case P_ADD_JOB:
        editJobName(pos);
        break;

      case P_EDIT_JOB:
        selJob += pos;
        selJob = constrain(selJob, 0, lastJob - 1);
        sprintf_P(txBuffer, PSTR("Select job to edit: %d."), selJob);
        sprintln(txBuffer);  
        loadJobEE(selJob);
        menu_edit_job();
        break;

      case P_EDIT_JOB_STEPS:
        selStep += pos;
        selStep = constrain(selStep, 0, lastStep - 1);       
        loadStepEE(selStep);
        menu_edit_job_steps(editStep);
        break;          

    }        
    delay(10);
    pos = 0;
    encoder.setPosition(pos);    
  }
}

// Funzione che aggiorna le informazioni sul display LCD
void updatePage() {
  if (currentPage != oldPage) {    
    // sprintf_P(txBuffer, PSTR("Going to page %d."), currentPage);    sprintln(txBuffer);
    oldPage = currentPage;
    switch (currentPage) {
    case P_MAIN:
      menu_main();
      break;
    case P_SETUP_MENU1:
      menu_setup1();
      break;
    case P_SETUP_MENU2:
      menu_setup2();
      break;
    case P_RUN_STEP:
      menu_run_step();
      break;
    case P_WAIT_STEP:
      menu_wait_step();
      break;
    case P_SELECT_STEP:
      menu_select_step();
      break;
    case P_YES_NO:
      menu_yesno();
      break;   
    case P_ADD_STEP:
      menu_add_step();
      break;
    case P_EDT_STEP:
      menu_edit_step();
      break;
    case P_ADD_STEP_TIME:
      add_step_time();
      break;
    case P_ADD_STEP_SPEED:
      add_step_speed();
      break;
    case P_ADD_STEP_TEMP:
      add_step_temp();
      break;
    case P_ADD_STEP_REVERSE:
      add_step_reverse();
      break;
    case P_ADD_STEP_REV_TIME:
      add_step_rev_time();
      break;
    case P_WAIT_TEMP:    
      menu_wait_temp();
      break;
    case P_SET_PREHEAT:
      menu_preheat();
      break;
    case P_DONE_JOB:
      menu_done_job();  
      delay(5000);
      currentPage = P_MAIN; 
      break;
    case P_SELECT_JOB:
      menu_select_job();
      break;
    case P_ADD_JOB:
      menu_add_job();
      break;
    case P_EDIT_JOB:
      menu_edit_job();
      break;
    case P_EDIT_JOB_STEPS:      
      menu_edit_job_steps(editStep);
      break;      
    }
    
  }
}

void editStepName(int dir){
  uint8_t row = cursor.row;
  uint8_t col = cursor.col;
  uint8_t ch = lcd.read();
  lcd.setCursor(col, row);
  ch = circularSum(ch, dir, ' ', 'z');
  activeStep.stepName[col] = ch;
  activeStep.stepName[col+1] = '\0';
  lcd.write(ch);  
  lcd.setCursor(col, row);
}

void editJobName(int dir){
  uint8_t row = cursor.row;
  uint8_t col = cursor.col;
  uint8_t ch = lcd.read();
  lcd.setCursor(col, row);
  ch = circularSum(ch, dir, ' ', 'z');
  activeJob.jobName[col] = ch;
  activeJob.jobName[col+1] = '\0';
  lcd.write(ch);
  lcd.setCursor(col, row);
}

void setMotor(uint8_t speed, bool dir){
	analogWrite(PWM_OUT, map(abs(speed), 0, 127, 0, 255));
	if (dir){
		digitalWrite(IN_A, HIGH);
		digitalWrite(IN_B, LOW);
	} 
	else {
		digitalWrite(IN_A, LOW);
		digitalWrite(IN_B, HIGH);
	}	
}

void runningInfo() {    
  uint32_t workTime = (millis() - startTime) / 1000;
  lcd.noBlink();
  lcd.clear();
  lcd.print(activeStep.stepName);
  lcd.setCursor(0, 1);

  snprintf_P(buffer, LCD_COLS+1, PSTR("Tempo: %lu/%u sec."), workTime, (uint16_t)activeStep.durationTime );
  lcd.print(buffer);

  lcd.setCursor(0, 2);
  char rotation[4] = {"CCW"};
  isRevActive ? rotation[0] = ' '  : rotation[0] = 'C';
  snprintf_P(buffer, LCD_COLS+1, PSTR("V.%d %s; T.%02d.%01d %cC"), activeStep.speed, rotation, 
                                                  (int)ActualTemp, (int)(ActualTemp*10.0)%10, (char)223 );
  lcd.print(buffer);

  lcd.setCursor(0, 3);
  uint8_t perc = map(workTime, 0, activeStep.durationTime, 0, LCD_COLS);
  for (uint8_t i = 0; i < LCD_COLS; i++) {
    if (i <= perc)
      lcd.write(0xFF);
    else
      lcd.write(' '); 
  }
}

// Controlliamo se il pulsante è stato premuto brevemente o più a lungo
void checkButton() {
  if (digitalRead(SW) == LOW) {
    delay(20);
    pressType = 0;
    while (digitalRead(SW) == LOW) {
      pTime = millis() - pressTime;
      if (pTime > SHORT && pTime <= MEDIUM)
        pressType = 1;
      else if (pTime > MEDIUM && pTime <= LONG)
        pressType = 2;
      else if (pTime > LONG) {
        pressType = 3;
        break;
      }
    }
    switch (pressType) {
      case 0: break;
      case 1: SingleClick();  break;
      case 2: MediumClick();  break;
      case 3: LongClick(); delay(500); break;
    }
  }
}

void setupTimer1() {
  noInterrupts();  
  TCCR1A = 0;  TCCR1B = 0;  TCNT1 = 0;  // Clear registers
  OCR1A = 6249;                         // 10 Hz (16000000/((6249+1)*256))
  TCCR1B |= (1 << WGM12);               // CTC  
  TCCR1B |= (1 << CS12);                // Prescaler 256  
  TIMSK1 |= (1 << OCIE1A);              // Output Compare Match A Interrupt Enable
  interrupts();
}