#define P_NULL                255
#define P_MAIN                0
#define P_YES_NO              1
#define P_SETUP_MENU1         2

#define P_SET_PREHEAT         3
#define P_ADD_STEP            4
  #define P_ADD_STEP_TIME       41
  #define P_ADD_STEP_SPEED      42
  #define P_ADD_STEP_TEMP       43
  #define P_ADD_STEP_REVERSE    44
  #define P_ADD_STEP_REV_TIME   45
#define P_EDT_STEP            5
#define P_SETUP_MENU2         6
  #define BACK_SETUP1           7
#define P_ADD_JOB             8
#define P_EDIT_JOB            9   
  #define P_EDIT_JOB_STEPS      91
#define EXIT_SETUP            10

#define P_SELECT_STEP         20
#define P_SELECT_JOB          21
#define P_RUN_STEP            22
#define P_WAIT_STEP           23
#define P_WAIT_TEMP           24
#define P_DONE_JOB            25

const char msg_empty[] PROGMEM            = "                    ";
const char msg_null[] PROGMEM             = "####################";
const char msg_vendor[] PROGMEM           = "   Sviluppatrice    ";
const char msg_version[] PROGMEM          = "       v2.0         ";
const char msg_confirm[] PROGMEM          = "Conferma?           ";
const char msg_yes_no[] PROGMEM           = "< No            Si >";
const char msg_previuos[] PROGMEM         = "< Indietro          ";
const char msg_next[] PROGMEM             = "            Avanti >";
const char msg_edit[] PROGMEM             = "          Modifica >";
const char msg_preheat[] PROGMEM          = "> Preriscaldo bagno ";
const char msg_add_step[] PROGMEM         = "> Aggiungi Fase     ";
const char msg_edit_step[] PROGMEM        = "> Modifica Fase     ";
const char msg_add_job[] PROGMEM          = "> Aggiungi Lavoro   ";
const char msg_edit_job[] PROGMEM         = "> Modifica Lavoro   ";

const char msg_select_job[] PROGMEM       = "Selezione Lavoro:   ";
const char msg_job_name[] PROGMEM         = "Nome Lavoro:        ";
const char msg_job_edit_name[] PROGMEM    = "Modifica Lavoro:    ";

const char msg_setup1[] PROGMEM           = "> Menu Setup 1      ";
const char msg_setup2[] PROGMEM           = "> Menu Setup 2      ";
const char msg_exit[] PROGMEM             = "> Uscita menu Setup ";

const char msg_select_step[] PROGMEM      = "Selezione Fase:     ";
const char msg_step_name[] PROGMEM        = "Nome Fase:          ";
const char msg_step_edit_name[] PROGMEM   = "Modifica nome Fase: ";
const char msg_step_time[] PROGMEM        = "Durata (secondi):   ";
const char msg_step_speed[] PROGMEM       = "Velocita' motore:   ";
const char msg_step_temp[] PROGMEM        = "Temperatura bagno:  ";
const char msg_step_reverse[] PROGMEM     = "Inverti rotazione:  ";
const char msg_step_rev_time[] PROGMEM    = "Tempo inversione:   ";

const char msg_wait_preheat1[] PROGMEM    = "  Preriscaldo bagno ";
const char msg_wait_preheat2[] PROGMEM    = " Imposta temperatura";
const char msg_init_ee[] PROGMEM          = "Init EEPROM required";

typedef struct {
  uint8_t col = 0;
  uint8_t row = 0;
} Cursor;

Cursor cursor;

typedef struct{
    uint8_t number;
    uint8_t prev;
    uint8_t next;  
} Pages;

Pages pMain = {P_MAIN, P_MAIN, P_MAIN};
Pages pYesNo = {P_YES_NO, P_MAIN, P_SETUP_MENU1};
Pages pMenuSetup1 = {P_SETUP_MENU1, P_YES_NO, P_SETUP_MENU2};
Pages pMenuSetup2 = {P_SETUP_MENU2, P_SETUP_MENU1, P_ADD_STEP};
Pages pAddStep = {P_ADD_STEP, P_SETUP_MENU1, P_ADD_STEP_TIME};
Pages pAddStepTime = {P_ADD_STEP_TIME, P_SETUP_MENU1, P_ADD_STEP_SPEED};
Pages pAddStepSpeed = {P_ADD_STEP_SPEED, P_ADD_STEP_TIME, P_ADD_STEP_TEMP};
Pages pAddStepTemp = {P_ADD_STEP_TEMP, P_ADD_STEP_SPEED, P_ADD_STEP_REVERSE};
Pages pAddStepReverse = {P_ADD_STEP_REVERSE, P_ADD_STEP_TEMP, P_ADD_STEP_REV_TIME};
Pages pAddStepRevTime = {P_ADD_STEP_REV_TIME, P_ADD_STEP_REVERSE, P_YES_NO};
Pages pEditStep = {P_EDT_STEP, P_SETUP_MENU1, P_ADD_STEP_TIME};
Pages pSelectStep = { P_SELECT_STEP, P_MAIN, P_RUN_STEP };
Pages pRunStep = { P_RUN_STEP, P_SELECT_STEP, P_MAIN };
Pages pWaitTemp = { P_WAIT_TEMP, P_SELECT_STEP, P_MAIN };
Pages pPreHeat = { P_SET_PREHEAT, P_WAIT_TEMP, P_WAIT_TEMP };

Pages pSelectJob = { P_SELECT_JOB, P_MAIN, P_RUN_STEP };
Pages pAddJob = {P_ADD_JOB, P_SETUP_MENU2, P_EDIT_JOB_STEPS};
Pages pEditJob = {P_EDIT_JOB, P_SETUP_MENU2, P_EDIT_JOB_STEPS};
Pages pEditJobSteps = {P_EDIT_JOB_STEPS, P_EDIT_JOB, P_EDIT_JOB_STEPS};

Pages pActive = pMain;

char buffer[LCD_COLS * 4];
bool SetupMode = false; 
int lastSetupMenu;

void clearLine() {
  uint8_t row = cursor.row;
  //uint8_t col = cursor.col;
  lcd.setCursor(0, row);
  for (uint8_t i = 0; i < LCD_COLS; i++)
    lcd.write(' ');
  lcd.setCursor(0, row);
}

void menu_main() {  
  sprintln(__PRETTY_FUNCTION__);
  lcd.clear();
  lcd.noBlink();
  strcpy_P(buffer, msg_vendor);
  strcat_P(buffer, msg_empty);
  strcat_P(buffer, msg_version);
  strcat_P(buffer, msg_empty);
  lcd.print(buffer);
  lcd.setCursor(0, 3);
  snprintf_P(buffer, LCD_COLS+1, PSTR("##### %02d.%02d %cC #####"), (int)ActualTemp, (int)(ActualTemp*10)%10, (char)223 );
  lcd.print(buffer);
  pActive = pMain;
}

void menu_setup1(){
  sprintln(__PRETTY_FUNCTION__);
  lcd.clear();  
  strcpy_P(buffer, msg_preheat);
  strcat_P(buffer, msg_edit_step);
  strcat_P(buffer, msg_add_step);
  strcat_P(buffer, msg_setup2);
  lcd.print(buffer);  
  lcd.setCursor(0, 1);
  lcd.blink();   
  pActive = pMenuSetup1;
  lastSetupMenu = P_SETUP_MENU1;
}

void menu_setup2(){
  sprintln(__PRETTY_FUNCTION__);

  lcd.clear();  
  strcpy_P(buffer, msg_setup1);
  strcat_P(buffer, msg_edit_job);
  strcat_P(buffer, msg_add_job);
  strcat_P(buffer, msg_exit);
  lcd.print(buffer);
  
  lcd.setCursor(0, 1);
  lcd.blink();   
  pActive = pMenuSetup2;
  lastSetupMenu = P_SETUP_MENU2;
}

void menu_done_job(){
  sprintln(__PRETTY_FUNCTION__);
  lcd.clear(); 
  lcd.setCursor(7, 1);
  lcd.print(F("LAVORO"));
  lcd.setCursor(6, 2);
  lcd.print(F("ESEGUITO"));  
}

void menu_add_job(){
  sprintln(__PRETTY_FUNCTION__);
  lcd.clear(); 
  strcpy_P(buffer, msg_job_name);
  strcat_P(buffer, msg_empty);
  strcat_P(buffer, msg_empty);
  strcat_P(buffer, msg_next);
  lcd.print(buffer);
  lcd.setCursor(0, 1);
  lcd.print(activeJob.jobName);
  lcd.setCursor(0, 1);
  lcd.blink();   
  pActive = pAddJob;
}

void menu_edit_job(){
  sprintln(__PRETTY_FUNCTION__);
  lcd.clear();  
  strcpy_P(buffer, msg_job_edit_name);
  strcat_P(buffer, msg_empty);
  strcat_P(buffer, msg_empty);
  strcat_P(buffer, msg_edit);
  lcd.print(buffer);
  lcd.setCursor(0, 1);
  lcd.print(activeJob.jobName);
  lcd.blink();    
  pActive = pEditJob;
}

void menu_edit_job_steps(int jobStepIndex, bool fromEE = false) {
  sprintln(__PRETTY_FUNCTION__);
  lcd.clear(); 
  strcpy_P(buffer, msg_select_step);
  strcat_P(buffer, msg_empty);
  strcat_P(buffer, msg_empty);
  strcat_P(buffer, msg_next);
  lcd.print(buffer);
  lcd.setCursor(15, 0);
  lcd.print(F(" "));
  lcd.print(jobStepIndex+1);
  lcd.setCursor(0, 1);
  
  if(fromEE){
    uint16_t eeAddress = STEP_EE  + activeJob.steps[jobStepIndex] * 22;
    char _stepName[NAME_LENGTH];
    EEPROM.get(eeAddress, _stepName);
    lcd.print(_stepName);
    selStep = activeJob.steps[jobStepIndex];
  }
  else
    lcd.print(activeStep.stepName);
  lcd.setCursor(LAST_COL, 3);
  lcd.blink();
  pActive = pEditJobSteps;
}

void menu_select_job() {
  sprintln(__PRETTY_FUNCTION__);
  lcd.clear(); 
  strcpy_P(buffer, msg_select_job);
  strcat_P(buffer, msg_empty);
  strcat_P(buffer, msg_empty);
  strcat_P(buffer, msg_empty);
  lcd.print(buffer);
  lcd.setCursor(0, 1);
  lcd.print(activeJob.jobName);
  lcd.blink();
  pActive = pSelectJob;
}

void menu_select_step() {
  sprintln(__PRETTY_FUNCTION__);
  lcd.clear(); 
  strcpy_P(buffer, msg_select_step);
  strcat_P(buffer, msg_empty);
  strcat_P(buffer, msg_empty);
  strcat_P(buffer, msg_empty);
  lcd.print(buffer);
  lcd.setCursor(0, 1);
  lcd.print(activeStep.stepName);
  lcd.blink();
  pActive = pSelectStep;
}

void menu_add_step(){
  sprintln(__PRETTY_FUNCTION__);
  lcd.clear(); 
  strcpy_P(buffer, msg_step_name);
  strcat_P(buffer, msg_empty);
  strcat_P(buffer, msg_empty);
  strcat_P(buffer, msg_next);
  lcd.print(buffer);
  lcd.setCursor(0, 1);
  lcd.print(activeStep.stepName);
  lcd.setCursor(0, 1);
  lcd.blink();   
  pActive = pAddStep;
}

void menu_edit_step(){
  sprintln(__PRETTY_FUNCTION__);
  lcd.clear();  
  strcpy_P(buffer, msg_step_edit_name);
  strcat_P(buffer, msg_empty);
  strcat_P(buffer, msg_empty);
  strcat_P(buffer, msg_edit);
  lcd.print(buffer);
  lcd.setCursor(0, 1);
  lcd.print(activeStep.stepName);
  lcd.blink();    
  pActive = pEditStep;
}

void add_step_time(){
  sprintln(__PRETTY_FUNCTION__);
  lcd.clear(); 
  strcpy_P(buffer, msg_step_time);
  strcat_P(buffer, msg_empty);
  strcat_P(buffer, msg_empty);
  strcat_P(buffer, msg_next);
  lcd.print(buffer);  
  lcd.setCursor(0, 1);
  lcd.print(activeStep.durationTime);
  lcd.blink(); 
  pActive = pAddStepTime;
}

void add_step_speed(){
  sprintln(__PRETTY_FUNCTION__);
  lcd.clear();
  strcpy_P(buffer, msg_step_speed);
  strcat_P(buffer, msg_empty);
  strcat_P(buffer, msg_empty);
  strcat_P(buffer, msg_next);
  lcd.print(buffer);
  lcd.setCursor(0, 1);
  lcd.print(activeStep.speed);
  lcd.blink(); 
  pActive = pAddStepSpeed;
}

void add_step_temp(){
  sprintln(__PRETTY_FUNCTION__);
  lcd.clear(); 
  strcpy_P(buffer, msg_step_temp);
  strcat_P(buffer, msg_empty);
  strcat_P(buffer, msg_empty);
  strcat_P(buffer, msg_next);
  lcd.print(buffer);
  lcd.setCursor(0, 1);
  lcd.print(activeStep.temperature);
  lcd.blink(); 
  pActive = pAddStepTemp;
}


void add_step_reverse(){
  sprintln(__PRETTY_FUNCTION__);
  lcd.clear();  
  strcpy_P(buffer, msg_step_reverse);
  strcat_P(buffer, msg_empty);
  strcat_P(buffer, msg_empty);
  strcat_P(buffer, msg_next);
  lcd.print(buffer);
  lcd.setCursor(0, 1);
  lcd.print(activeStep.reverse ? "SI" : "NO");
  lcd.blink(); 
  pActive = pAddStepReverse;
}

void add_step_rev_time(){
  sprintln(__PRETTY_FUNCTION__);
  lcd.clear(); 
  strcpy_P(buffer, msg_step_rev_time);
  strcat_P(buffer, msg_empty);
  strcat_P(buffer, msg_empty);
  strcat_P(buffer, msg_next);
  lcd.print(buffer);
  lcd.setCursor(0, 1);
  lcd.print(activeStep.revTime);
  lcd.blink(); 
  pActive = pAddStepRevTime;
}


void menu_preheat(){
  sprintln(__PRETTY_FUNCTION__);
  lcd.clear();
  lcd.noBlink(); 
  strcpy_P(buffer, msg_wait_preheat2);
  strcat_P(buffer, msg_empty); 
  strcat_P(buffer, msg_wait_preheat1);     
  strcat_P(buffer, msg_empty);  
  lcd.print(buffer);
  lcd.setCursor(0, 3);
  snprintf_P(buffer, LCD_COLS+1, PSTR("#####  %02d.%d %cC  #####"), (int)preHeat, (int)(preHeat*10)%10, (char)223 );
  lcd.print(buffer);
  pActive = pPreHeat;
}


void upadteTempRow2() {
  lcd.setCursor(0, 2);
  snprintf_P(buffer, LCD_COLS+1, PSTR("Set %02d.%d, Temp. %02d.%d"), 
              (int)Setpoint, (int)(Setpoint*10)%10,
              (int)ActualTemp, (int)(ActualTemp*10)%10
            );
  lcd.print(buffer);
}

void menu_wait_temp(){
  sprintln(__PRETTY_FUNCTION__);
  lcd.clear();
  lcd.noBlink(); 
  lcd.print(F("Attesa riscaldamento"));
  lcd.setCursor(0, 1);
  lcd.print(activeStep.stepName); 
  
  upadteTempRow2();
  pActive = pWaitTemp;
}

void menu_run_step(){
  sprintln(__PRETTY_FUNCTION__);
  lcd.clear();
  lcd.noBlink(); 
  lcd.print(activeStep.stepName);
  pActive = pRunStep;
}

void menu_wait_step() {
  sprintln(__PRETTY_FUNCTION__);
  lcd.noBlink();  
  lcd.setCursor(0, 0);    
  lcd.print(F("Sostituzione TANK")); 
  lcd.setCursor(0, 1);     
  lcd.print(F(">>> "));
  lcd.print(activeStep.stepName);
  for(uint8_t i=0; i< LCD_COLS-strlen(activeStep.stepName)-4; i++)
    lcd.print(" ");
}

void menu_yesno(){
  sprintln(__PRETTY_FUNCTION__);
  lcd.clear(); 
  strcpy_P(buffer, msg_confirm);
  strcat_P(buffer, msg_empty);
  strcat_P(buffer, msg_empty);
  strcat_P(buffer, msg_yes_no);  
  lcd.print(buffer);
  lcd.setCursor(LAST_COL, 3);  
  lcd.blink();    
  pActive = pYesNo;
}


void save_message(){
  sprintln(F("Save to EEprom"));
  lcd.clear();
  lcd.noBlink();
  lcd.setCursor(5, 1);
  lcd.print(F("SALVATAGGIO"));
  lcd.setCursor(6, 2);
  lcd.print(F("IN CORSO"));
  delay(1000); 
  lcd.clear();
}

int circularSum(int value, int sum, int min, int max) {
  value += sum;
  if (value > max)
    value = min;
  if (value < min)
    value = max;
  return value;
}

double circularSumFloat(double value, double sum, double min, double max) {
  value += sum;
  if (value > max)
    value = min;
  if (value < min)
    value = max;
  return value;
}

void getCursor() {
  uint8_t adr = lcd.status();
  if (adr >= row0 && adr <= row0 + LAST_COL) {
    cursor.row = 0;
    cursor.col = adr - row0;
  }
  if (adr >= row1 && adr <= row1 + LAST_COL) {
    cursor.row = 1;
    cursor.col = adr - row1;
  }
  if (adr >= row2 && adr <= row2 + LAST_COL) {
    cursor.row = 2;
    cursor.col = adr - row2;
  }
  if (adr >= row3 && adr <= row3 + LAST_COL) {
    cursor.row = 3;
    cursor.col = adr - row3;
  }

}

