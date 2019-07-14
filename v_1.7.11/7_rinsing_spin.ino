//******************** Полоскание + отжим *******************************


// Полоскание
void rinsing(unsigned long rinsing_time) {
  unsigned long time_Start_rinsing = millis();
  unsigned long time_Stop_rinsing = time_Start_rinsing + rinsing_time * 1000;
  unsigned long sumTimeM = 0;

  addLog("Подпрограмма ПОЛОСКАНИЯ на " + String((time_Stop_rinsing - time_Start_rinsing) / 1000) + " сек");

  addLog("Start ПОЛОСКАНИЕ");
  while (millis() < time_Stop_rinsing) {
    MOTOR_direction = !MOTOR_direction;
    Motor(MOTOR_direction, rinsing_speed, MOTOR_POWER_Time_rinsing);
    delay2(rinsing_pause);
    sumTimeM = floor((millis() - time_Start_rinsing) / 1000 / 60); // Прошло времени в минутах
    setTime(TimeLeft - sumTimeM);
  }
  addLog("End ПОЛОСКАНИЕ");
}

void rinsing_cycle(int mode) {
  // Набор воды для полоскания либо через клапан основной стирки (2), либо через оба клапана (3)
  if (mode == 1) mode = 2;
  water_pour(mode, WATER_LEVEL2);
  TimeLeft = TimeLeft - 1;
  setTime(TimeLeft);
  rinsing(PROGRAM_RINSING_TIME);
  drain_water();
}

// Отжим
// mode = 1 - Промежуточный отжим с удалением пены после стирки и после полоскании
// mode = 2 - Промежуточный отжим при полоскании
// mode = 3 - Окончательный отжим
void spin(int mode) {
  bool fl_end = false;

  // Удаление пены
  if (mode == 1 && fl_del_p) {
    addLog("Удаление пены");
    pump_off ();
    water_valte2_on (TIME_FOAM_REMOVAL_WATER);
    delay2(TIME_FOAM_REMOVAL_WATER * 1000);
    drain_water ();
    fl_del_p = false;
  }

  if (fl_0 != 1) { // Влючен отжим

    // Пытаемся DEFAULT_SPIN_COUNT (количество попыток выхода на режим отжима) раз отжать
    for (int i = 0; i < DEFAULT_SPIN_COUNT; i = i + 1) {

      // Включим контроль вибрации
      fl_Vibro_Control = true;
      // Выключим признак срабатывания
      fl_Vibro = false;

      pump_on (20);
      SetMinimalPOWER();

      // Запускаем барабаном со скоростью MOTOR_SPEED_STRAIGHTEN_MAX1 (примерно 25-30 об/мин)
      // это скорость, при которой дисбаланс точно не возникает
      addLog("Запускаем барабаном со скоростью MOTOR_SPEED_STRAIGHTEN_MAX1");
      MOTOR_direction = !MOTOR_direction;
      digitalWrite(PIN_MOTOR_LR, MOTOR_direction); // Установим направление вращения
      delay2(1000); // Пауза, что бы реле успело сработать
      pwmSet = MOTOR_SPEED_STRAIGHTEN_MAX1;
      pwmSetMinMax();
      MotorStart();
      // Ждем пока обороты "устаканятся"
      delay2(MOTOR_WAIT_STRAIGHTEN_TIMER2);
      
      addLog("Плавно увеличиваем скорость до MOTOR_SPEED_STRAIGHTEN_UNBALANCE");
      for (int i = MOTOR_SPEED_STRAIGHTEN_MAX1; i <= MOTOR_SPEED_STRAIGHTEN_UNBALANCE; i = i + 1) {
        if (DEBUG_LEVEL >= 2) {
          addLog("MOTOR_direction: " + String(MOTOR_direction) + " pwmSet: " + String(pwmSet) + " pwmSpeed:" + String(pwmSpeed) + " (" + String(pwmSpeedSec) + ") pwmOut:" + String(pwmOut) + " Dimmer:" + String(Dimmer));
        }
        if (pwmSpeed > i) {
          if (pwmSpeed < MOTOR_SPEED_STRAIGHTEN_UNBALANCE_MAX) {
            if (pwmSet < pwmSpeed) {
              pwmSet = pwmSpeed;
              pwmSetMinMax();
            }
          } else {
            if (pwmSpeed > 30) {
              pwmSet = pwmSpeed;
              pwmSetMinMax();
              break;
            } else {
              pwmSet = MOTOR_SPEED_STRAIGHTEN_UNBALANCE_MAX;
              pwmSetMinMax();
            }
          }
        } else {
          if (pwmSet < i) {
            pwmSet = i;
            pwmSetMinMax();
          }
        }
        
        if (pwmSet == MOTOR_SPEED_STRAIGHTEN_UNBALANCE_MAX && pwmSpeed < MOTOR_SPEED_STRAIGHTEN_UNBALANCE_MAX && pwmOut < 130) {
          MOTOR_POWER_MINIMAL_ORIGINAL = pwmOut;
          addLog("MPMO:" + String(MOTOR_POWER_MINIMAL_ORIGINAL));
        }

        if(fl_Vibro) {
          MotorStop();
          break;
        }
        if (pwmSpeed == 0) {
          i = MOTOR_SPEED_STRAIGHTEN_MAX1;
        }
        
        if (pwmSet == MOTOR_SPEED_STRAIGHTEN_UNBALANCE_MAX) {
          delay2(3000);
        } else {
          delay2(3000); // Небольшая скорость, просто пауза, на такой скорости вибрацию не контролируем, т.к. белье еще не закрепилось на барабане
        }

        if(fl_Vibro) {
          MotorStop();
          break;
        }
      }
      
      if (!fl_Vibro) {
        delay2(5000);
        // Измеряем вибрацию 5 раз
        for (int i = 1; i <= 5; i = i + 1) {
          delay3(3000); // Скорость 50-80 об/мн, пауза и проверка на вибрацию
          if (fl_Vibro) {
            break;
          }
        }
      }
      MOTOR_POWER_MINIMAL_ORIGINAL = 100;
      if (fl_Vibro) {
        
        MotorStop();
        MotorWaitStop();
        fl_Vibro = false;
          
        addLog("ВИБРАЦИЯ: цикл укладки белья");
        pump_on (20);

        delay2(3000);
        SetMinimalPOWER();

        // расправление белье, цикл из 2 раз со случайно выбираемой скоростью между MOTOR_SPEED_STRAIGHTEN_MIN  и MOTOR_SPEED_STRAIGHTEN_MAX1
        for (int j = 1; j <= 2; j = j + 1) {
          MOTOR_direction = !MOTOR_direction;
          digitalWrite(PIN_MOTOR_OUT, LOW); //Принудительно отключим мотор
          digitalWrite(PIN_MOTOR_LR, MOTOR_direction); // Установим направление вращения
          delay2(1000); // Пауза, что бы реле успело сработать
          pwmSet = random(MOTOR_SPEED_STRAIGHTEN_MIN1, MOTOR_SPEED_STRAIGHTEN_MAX1);
          pwmSetMinMax();
          MotorStart();
          delay2(MOTOR_POWER_Time_STRAIGHTEN * 1000);
          MotorStop();
          MotorWaitStop();
          delay2(random(1000, 3000)); // Случайная задержка от 1 до 3 сек
        }
        continue; // Начнем отжим с начала
      }

      pump_on (240);

      if (!fl_Vibro) {
        addLog("Выключим контроль вибрации");
        // Далее не контролируем дисбаланс, выключим контроль вибрации
        fl_Vibro_Control = false;
        
        if (fl_400 == 1) {
          addLog("выход на режим отжима MOTOR_SPEED_STRAIGHTEN_MAX6");
          pwmSet = MOTOR_SPEED_STRAIGHTEN_MAX6;
          pwmSetMinMax();
          delay2(RINSING_TIME_MAX_F);
          addLog("выход на режим отжима MOTOR_SPEED_STRAIGHTEN_MAX5");
          pwmSet = MOTOR_SPEED_STRAIGHTEN_MAX5;
          pwmSetMinMax();
          delay2(RINSING_TIME_MIN);
          addLog("выход на режим отжима MOTOR_SPEED_STRAIGHTEN_MAX6");
          pwmSet = MOTOR_SPEED_STRAIGHTEN_MAX6;
          pwmSetMinMax();
          delay2(RINSING_TIME_MID);
          addLog("выход на режим отжима MOTOR_SPEED_STRAIGHTEN_MAX6");
          pwmSet = MOTOR_SPEED_STRAIGHTEN_MAX6;
          pwmSetMinMax();
        }
        if (fl_500 == 1) {
          addLog("выход на режим отжима MOTOR_SPEED_STRAIGHTEN_MAX7");
          pwmSet = MOTOR_SPEED_STRAIGHTEN_MAX7;
          pwmSetMinMax();
          delay2(RINSING_TIME_MAX_F);
          addLog("выход на режим отжима MOTOR_SPEED_STRAIGHTEN_MAX5");
          pwmSet = MOTOR_SPEED_STRAIGHTEN_MAX5;
          pwmSetMinMax();
          delay2(RINSING_TIME_MIN);
          addLog("выход на режим отжима MOTOR_SPEED_STRAIGHTEN_MAX6");
          pwmSet = MOTOR_SPEED_STRAIGHTEN_MAX6;
          pwmSetMinMax();
          delay2(RINSING_TIME_MID);
          addLog("выход на режим отжима MOTOR_SPEED_STRAIGHTEN_MAX6");
          pwmSet = MOTOR_SPEED_STRAIGHTEN_MAX6;
          pwmSetMinMax();
          delay2(RINSING_TIME_MID);
          addLog("выход на режим отжима MOTOR_SPEED_STRAIGHTEN_MAX7");
          pwmSet = MOTOR_SPEED_STRAIGHTEN_MAX7;
          pwmSetMinMax();
        }
        if (fl_600 == 1) {
          addLog("выход на режим отжима MOTOR_SPEED_STRAIGHTEN_MAX8");
          pwmSet = MOTOR_SPEED_STRAIGHTEN_MAX8;
          pwmSetMinMax();
          delay2(RINSING_TIME_MAX_F);
          addLog("выход на режим отжима MOTOR_SPEED_STRAIGHTEN_MAX5");
          pwmSet = MOTOR_SPEED_STRAIGHTEN_MAX5;
          pwmSetMinMax();
          delay2(RINSING_TIME_MIN);
          addLog("выход на режим отжима MOTOR_SPEED_STRAIGHTEN_MAX6");
          pwmSet = MOTOR_SPEED_STRAIGHTEN_MAX6;
          pwmSetMinMax();
          delay2(RINSING_TIME_MID);
          addLog("выход на режим отжима MOTOR_SPEED_STRAIGHTEN_MAX6");
          pwmSet = MOTOR_SPEED_STRAIGHTEN_MAX6;
          pwmSetMinMax();
          delay2(RINSING_TIME_MID);
          addLog("выход на режим отжима MOTOR_SPEED_STRAIGHTEN_MAX7");
          pwmSet = MOTOR_SPEED_STRAIGHTEN_MAX7;
          pwmSetMinMax();
          delay2(RINSING_TIME_MID);
          addLog("выход на режим отжима MOTOR_SPEED_STRAIGHTEN_MAX8");
          pwmSet = MOTOR_SPEED_STRAIGHTEN_MAX8;
          pwmSetMinMax();
        }
        if (fl_800 == 1) {
          addLog("выход на режим отжима MOTOR_SPEED_STRAIGHTEN_MAX8");
          pwmSet = MOTOR_SPEED_STRAIGHTEN_MAX8;
          pwmSetMinMax();
          delay2(RINSING_TIME_MAX_F);
          addLog("выход на режим отжима MOTOR_SPEED_STRAIGHTEN_MAX5");
          pwmSet = MOTOR_SPEED_STRAIGHTEN_MAX5;
          pwmSetMinMax();
          delay2(RINSING_TIME_MIN);
          addLog("выход на режим отжима MOTOR_SPEED_STRAIGHTEN_MAX6");
          pwmSet = MOTOR_SPEED_STRAIGHTEN_MAX6;
          pwmSetMinMax();
          delay2(RINSING_TIME_MID);
          addLog("выход на режим отжима MOTOR_SPEED_STRAIGHTEN_MAX6");
          pwmSet = MOTOR_SPEED_STRAIGHTEN_MAX6;
          pwmSetMinMax();
          delay2(RINSING_TIME_MID);
          addLog("выход на режим отжима MOTOR_SPEED_STRAIGHTEN_MAX7");
          pwmSet = MOTOR_SPEED_STRAIGHTEN_MAX7;
          pwmSetMinMax();
          delay2(RINSING_TIME_MID);
          addLog("выход на режим отжима MOTOR_SPEED_STRAIGHTEN_MAX8");
          pwmSet = MOTOR_SPEED_STRAIGHTEN_MAX8;
          pwmSetMinMax();
          delay2(RINSING_TIME_MID);
          addLog("выход на режим отжима MOTOR_SPEED_STRAIGHTEN_MAX9");
          pwmSet = MOTOR_SPEED_STRAIGHTEN_MAX9;
          pwmSetMinMax();
        }

        fl_end = true;

        if (mode == 1) { // Отжим после стирки
          delay2(RINSING_TIME_AFTER_WASHING);

        } else if (mode == 2) { // Отжим после полоскания
          delay2(RINSING_TIME_AFTER_RINSING);
          
        } else if (mode == 3) { // Отжим окончательный
          delay2(RINSING_TIME_END);
        }
      }

      if (fl_end) {
        break;
      }
    }
    MotorStop();
    MotorWaitStop();

    delay2(PROGRAM_PAUSE);

    pump_off ();

    // Если не получилось за DEFAULT_SPIN_COUNT попыток отжать, показываем ошибку UE
    if (!fl_end) {
      error_ue ();
    }

    if (mode == 1) { // Отжим после стирки
      addLog("Удаление пены после отжима, после стирки");
      pump_off ();
      water_valte2_on (TIME_FOAM_REMOVAL_WATER);
      delay2(TIME_FOAM_REMOVAL_WATER * 1000);
      drain_water ();
    }
    
  }
  fl_Vibro_Control = false;
}

// Полоскание + отжим
void rinsing_spin() {

  int mode = 2;
  for (int i = 1; i < DEFAULT_RINSING_COUNT; i = i + 1) {
    rinsing_cycle(mode);
    spin(2);
    TimeLeft = TimeLeft - 6;
    setTime(TimeLeft);
  }

  if (fl_PoloskanirPlus == 0) {
    // если не включен режим "Полоскание +" включим одновременно два клапна, что бы смыть кондиционер для белья из среднего отсека
    addLog("Окончательное полоскание с кондиционером для белья из среднего отсека и отжим");
    mode = 3;
    rinsing_cycle(mode);
    spin(mode);
  } else {
    // Если включено "Полоскание+", сначала полоскаем в теплой воде, затем последнее полоскание с кондиционером
    addLog("включено 'Полоскание+', полоскаем в теплой воде");
    water_pour(mode, WATER_LEVEL2);
    water_heating (PROGRAM_RINSING_WARM_TEMP);
    TimeLeft = TimeLeft - 10;
    setTime(TimeLeft);
    rinsing(PROGRAM_RINSING_WARM_TIME);
    drain_water();
    
    spin(2);

    TimeLeft = 7;
    setTime(TimeLeft);

    addLog("Окончательное полоскание и отжим");
    mode = 3;
    rinsing_cycle(mode);
    spin(mode);
  }

  setTime(1);

  if (fl_Easy == 1) {
    addLog("Режим Легкая глажка");
    unsigned long time_Stop_Easy = millis() + 60000;
    while (millis() < time_Stop_Easy) {
      MOTOR_direction = !MOTOR_direction;
      Motor(MOTOR_direction, washing_speed1, MOTOR_POWER_Time_Esy_ON);
      delay2(washing_pause0);
    }
  }
}
