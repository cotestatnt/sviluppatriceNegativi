constexpr uint16_t INIT_EE_ADR = 1010;
constexpr uint16_t LASTSTEP_EE = 1012;
constexpr uint16_t LASTJOB_EE = 1014;

constexpr uint16_t STEP_EE = 10;
constexpr uint16_t JOB_EE = 570;
//constexpr uint16_t PidStartAdr = 1000;

void readEeprom() {  
  Step step;
  EEPROM.get(LASTSTEP_EE, lastStep);
  sprint(F("Read steps stored "));
  sprintln(lastStep );

  uint16_t eeAddress = STEP_EE;
  for(int i = 0; i < lastStep; i++){   
    EEPROM.get(eeAddress, step.stepName);
    eeAddress += NAME_LENGTH;
    EEPROM.get(eeAddress, step.durationTime);      
    eeAddress+=sizeof(uint16_t);
    EEPROM.get(eeAddress, step.speed);      
    eeAddress+=sizeof(int8_t);
    EEPROM.get(eeAddress, step.temperature);      
    eeAddress+=sizeof(uint8_t);
    EEPROM.get(eeAddress, step.reverse);      
    eeAddress+=sizeof(uint8_t);
    EEPROM.get(eeAddress, step.revTime);     
    eeAddress += sizeof(uint8_t);

    sprint(F("Step n° ")); sprint(i+1); sprint(F(": {"));
    sprint(step.stepName); sprint(F("; "));
    sprint(step.durationTime); sprint(F("; "));
    sprint(step.speed); sprint(F("; "));
    sprint(step.temperature); sprint(F("; "));
    sprint(step.reverse); sprint(F("; "));
    sprint(step.revTime);  sprintln(F("}"));
  }  

  Job job;
  EEPROM.get(LASTJOB_EE, lastJob);
  sprint(F("Read job stored "));
  sprintln(lastJob );
  eeAddress = JOB_EE;
  for(int i = 0; i < lastJob; i++){  
    EEPROM.get(eeAddress, job.jobName);
    eeAddress+= NAME_LENGTH;
    EEPROM.get(eeAddress, job.steps);
    eeAddress+=sizeof(job.steps);
    #if DEBUG
      sprint(F("Job n° ")); sprint(i+1); sprint(F(": {"));
      sprint(job.jobName); sprint(F("; {"));
      for(uint8_t i = 0; i < MAX_JOB_STEPS; i++){  
        //sprint(job.steps[i]);
        uint16_t eeAddress = STEP_EE  + job.steps[i] * 22;
        char _step[NAME_LENGTH];
        EEPROM.get(eeAddress, _step);
        sprint(_step);
        sprint(F("; "));      
      }
      sprintln(F("} }"));
    #endif
      
  } 
}

void loadStepEE(uint8_t selection) {
  uint16_t eeAddress = STEP_EE + selection * 22;
  sprint(F("\nReading EEprom from address "));  sprint(eeAddress);
  EEPROM.get(eeAddress, activeStep.stepName);
  eeAddress += NAME_LENGTH;
  EEPROM.get(eeAddress, activeStep.durationTime);
  eeAddress += sizeof(uint16_t);
  EEPROM.get(eeAddress, activeStep.speed);
  eeAddress += sizeof(int8_t);
  EEPROM.get(eeAddress, activeStep.temperature);
  eeAddress += sizeof(uint8_t);
  EEPROM.get(eeAddress, activeStep.reverse);
  eeAddress += sizeof(uint8_t);
  EEPROM.get(eeAddress, activeStep.revTime);

  sprint(F(", {"));
  sprint(activeStep.stepName); sprint(F("; "));
  sprint(activeStep.durationTime); sprint(F("; "));
  sprint(activeStep.speed); sprint(F("; "));
  sprint(activeStep.temperature); sprint(F("; "));
  sprint(activeStep.reverse); sprint(F("; "));
  sprint(activeStep.revTime); sprintln(F("}"));

  EEPROM.get(LASTSTEP_EE, lastStep);
}


void loadJobEE(uint8_t selection) {
  uint16_t eeAddress = JOB_EE + selection * 21;
  sprint(F("\nReading EEprom from address "));  sprint(eeAddress);
  EEPROM.get(eeAddress, activeJob.jobName);
  eeAddress += NAME_LENGTH;
  EEPROM.get(eeAddress, activeJob.steps);
  
  EEPROM.get(LASTJOB_EE, lastJob);
  #if DEBUG
    sprint(F(", "));
    sprint(activeJob.jobName); sprint(F(": {"));
    for(uint8_t i = 0; i < MAX_JOB_STEPS; i++){  
      uint16_t eeAddress = STEP_EE + activeJob.steps[i] * 22;
      char stepName[NAME_LENGTH];
      EEPROM.get(eeAddress, stepName);
      //sprint(activeJob.steps[i]);
      sprint(stepName);
      sprint(F("; "));
    }
    sprintln(F("}"));
  #endif
}



void writeStepEeprom(uint8_t selection) {
  uint16_t eeAddress = STEP_EE + selection * 22;  
  sprint(F("Writing EEprom address@"));  sprintln(eeAddress);
  EEPROM.put(eeAddress, activeStep.stepName);
  eeAddress += NAME_LENGTH;
  EEPROM.put(eeAddress, activeStep.durationTime);  
  eeAddress+=sizeof(uint16_t);
  EEPROM.put(eeAddress, activeStep.speed);  
  eeAddress+=sizeof(int8_t);
  EEPROM.put(eeAddress, activeStep.temperature);  
  eeAddress+=sizeof(uint8_t);
  EEPROM.put(eeAddress, activeStep.reverse);     
  eeAddress+=sizeof(uint8_t);
  EEPROM.put(eeAddress, activeStep.revTime);  
  EEPROM.put(LASTSTEP_EE, lastStep); 
  readEeprom();
}


void writeJobEeprom(uint8_t selection) {
  uint16_t eeAddress = JOB_EE + selection * sizeof(activeJob); //21;  
  sprint(F("Writing EEprom address@"));  sprintln(eeAddress);
  EEPROM.put(eeAddress, activeJob.jobName);
  eeAddress += NAME_LENGTH;
  EEPROM.put(eeAddress, activeJob.steps);  
  EEPROM.put(LASTJOB_EE, lastJob);  
  readEeprom();
}

/*
void writePID() {
  uint16_t eeAddress = PidStartAdr;
  EEPROM.put(eeAddress, Kp);
  eeAddress+=sizeof(double);
  EEPROM.put(eeAddress, Ki);
  eeAddress+=sizeof(double);
  EEPROM.put(eeAddress, Kd);
  sprint(F("PID constants saved. Kp:"));
  sprint(Kp); sprint(F(" Ki: "));
  sprint(Ki); sprint(F(" Kd: "));
  sprintln(Kd);
}
*/

//////////////////////////////////////////////////
//////////////// INIT EEPROM /////////////////////
//////////////////////////////////////////////////
void initEeprom(){
  lastStep = 0;  
  sprint(F("Clear EEPROM..."));  
  for (uint16_t i = 0; i < EEPROM.length() ; i++) {
    EEPROM.write(i, 0);
  }  
  sprintln(F(" done\nInit EEPROM"));  

  EEPROM.put(INIT_EE_ADR, 0x77);

  uint16_t eeAddress = STEP_EE;  
  char stepName[NAME_LENGTH];
  lastStep++;
  strcpy(stepName, String("").c_str());  
  EEPROM.put(eeAddress, stepName);
  eeAddress+= NAME_LENGTH;
  EEPROM.put(eeAddress, 0);
  eeAddress+=sizeof(uint16_t);
  EEPROM.put(eeAddress, 0);
  eeAddress+=sizeof(int8_t);
  EEPROM.put(eeAddress, 0);
  eeAddress+=sizeof(uint8_t);
  EEPROM.put(eeAddress, 0);
  eeAddress+=sizeof(uint8_t);
  EEPROM.put(eeAddress, 0);
  eeAddress+=sizeof(uint8_t);

  lastStep++;
  strcpy(stepName, String(F("Prelavaggio")).c_str());  
  EEPROM.put(eeAddress, stepName);
  eeAddress+= NAME_LENGTH;
  EEPROM.put(eeAddress, 30);
  eeAddress+=sizeof(uint16_t);
  EEPROM.put(eeAddress, 100);
  eeAddress+=sizeof(int8_t);
  EEPROM.put(eeAddress, 0);
  eeAddress+=sizeof(uint8_t);
  EEPROM.put(eeAddress, 0);
  eeAddress+=sizeof(uint8_t);
  EEPROM.put(eeAddress, 0);
  eeAddress+=sizeof(uint8_t);
  
  lastStep++;
  strcpy(stepName, String(F("Sviluppo")).c_str());
  EEPROM.put(eeAddress, stepName);
  eeAddress += NAME_LENGTH;
  EEPROM.put(eeAddress, 30);
  eeAddress+=sizeof(uint16_t);
  EEPROM.put(eeAddress, 80);
  eeAddress+=sizeof(int8_t);
  EEPROM.put(eeAddress, 30);
  eeAddress+=sizeof(uint8_t);
  EEPROM.put(eeAddress, 1);
  eeAddress+=sizeof(uint8_t);
  EEPROM.put(eeAddress, 10);
  eeAddress+=sizeof(uint8_t);
  
  lastStep++;
  strcpy(stepName, String(F("Sbianca")).c_str()); 
  EEPROM.put(eeAddress, stepName);
  eeAddress += NAME_LENGTH;
  EEPROM.put(eeAddress, 30);
  eeAddress+=sizeof(uint16_t);
  EEPROM.put(eeAddress, 50);
  eeAddress+=sizeof(int8_t);
  EEPROM.put(eeAddress, 0);
  eeAddress+=sizeof(uint8_t);
  EEPROM.put(eeAddress, 1);
  eeAddress+=sizeof(uint8_t);
  EEPROM.put(eeAddress, 15);
  eeAddress+=sizeof(uint8_t);

  lastStep++;
  strcpy(stepName, String(F("Fissaggio")).c_str());
  EEPROM.put(eeAddress, stepName);
  eeAddress += NAME_LENGTH;
  EEPROM.put(eeAddress, 30);
  eeAddress+=sizeof(uint16_t);
  EEPROM.put(eeAddress, 100);
  eeAddress+=sizeof(int8_t);
  EEPROM.put(eeAddress, 25);
  eeAddress+=sizeof(uint8_t);
  EEPROM.put(eeAddress, 0);
  eeAddress+=sizeof(uint8_t);
  EEPROM.put(eeAddress, 0);
  eeAddress+=sizeof(uint8_t);

  lastStep++;
  strcpy(stepName, String(F("Lavaggio")).c_str());
  EEPROM.put(eeAddress, stepName);
  eeAddress += NAME_LENGTH;
  EEPROM.put(eeAddress, 30);
  eeAddress+=sizeof(uint16_t);
  EEPROM.put(eeAddress, 127);
  eeAddress+=sizeof(int8_t);
  EEPROM.put(eeAddress, 0);
  eeAddress+=sizeof(uint8_t);
  EEPROM.put(eeAddress, 0);
  eeAddress+=sizeof(uint8_t);
  EEPROM.put(eeAddress, 0);
  eeAddress += sizeof(uint8_t);
  
  sprint(F("Total steps initialized: "));
  sprintln(lastStep);
  EEPROM.put(LASTSTEP_EE, lastStep);

  lastJob = 0;  
  eeAddress = JOB_EE;
  char jobName[NAME_LENGTH];
  lastJob++;
  strcpy(jobName, String("PORTRA").c_str());  
  EEPROM.put(eeAddress, jobName);
  eeAddress+= NAME_LENGTH;
  int8_t step1[MAX_JOB_STEPS] = {1, 2, 3, 4, 5};  
  EEPROM.put(eeAddress, step1);
  eeAddress+=sizeof(step1);

  lastJob++;
  strcpy(jobName, String("EKTAR").c_str());  
  EEPROM.put(eeAddress, jobName);
  eeAddress+= NAME_LENGTH;
  int8_t step2[MAX_JOB_STEPS] = { 1, 2, 0, 4, 5};  
  EEPROM.put(eeAddress, step2);
  eeAddress+=sizeof(step2);

  sprint(F("Total jobs initialized: "));
  sprintln(lastJob);
  EEPROM.put(LASTJOB_EE, lastJob);
  
}
