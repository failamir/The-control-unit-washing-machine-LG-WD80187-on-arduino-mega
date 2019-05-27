void delay2(unsigned long mS) {
  unsigned long millis_d = millis();
  while ((millis() - millis_d) <= mS )  {
  }
}

void delay3(unsigned long mS) {
  unsigned long millis_d = millis();
  double CountMidVibro = 0;
  double SumMidVibro = 0;
  double pwmOutMin = pwmOut;
  double pwmOutMax = pwmOut;
  double pwmSpeedMin = pwmSpeed;
  double pwmSpeedMax = pwmSpeed;
  double pwmSpeedSecMin = pwmSpeedSec;
  double pwmSpeedSecMax = pwmSpeedSec;
  
  while ((millis() - millis_d) <= mS )  {
    getLevelVibro();
    SumMidVibro = SumMidVibro + AcMax;
    CountMidVibro++;
    
    if (pwmOut < pwmOutMin) pwmOutMin = pwmOut;
    if (pwmOut > pwmOutMax) pwmOutMax = pwmOut;
    if (pwmSpeed > 0) {
      if (pwmSpeed < pwmSpeedMin) pwmSpeedMin = pwmSpeed;
      if (pwmSpeed > pwmSpeedMax) pwmSpeedMax = pwmSpeed;
      if (pwmSpeedSec < pwmSpeedSecMin) pwmSpeedSecMin = pwmSpeedSec;
      if (pwmSpeedSec > pwmSpeedSecMax) pwmSpeedSecMax = pwmSpeedSec;
    }
  }
  if (CountMidVibro > 0) {
    AcMax = SumMidVibro / CountMidVibro;
  } else {
    AcMax = 20000; // Если, что то пошло не так, значение больше чем нужно
  }
  addLog("AcMax: " + String(AcMax));
  addLog(String(pwmOut) + "," + String(pwmOutMin) + ", " + String(pwmOutMax) + "," + String(pwmOutMax - pwmOutMin));
  addLog(String(pwmSpeed) + "," + String(pwmSpeedMin) + "," + String(pwmSpeedMax) + "," + String(pwmSpeedMax - pwmSpeedMin)  + " === " + String(pwmSpeedSec) + ", " + String(pwmSpeedSecMin) + "," + String(pwmSpeedSecMax) + "," + String(pwmSpeedSecMax - pwmSpeedSecMin));

  if (pwmSpeed > 0 && pwmSpeedMax - pwmSpeedMin > 10) {
    addLog("ДИСБАЛАНС ПО ТАХО!");
    fl_Vibro = true;
  }

  if (AcMax > 17400) {
    addLog("ДИСБАЛАНС ПО AcMax!");
    fl_Vibro = true;
  }
}


void addLog(String txt) {
  Serial.println(String(millis()) + ": " + txt);
}

void TimePrint(int sec) {
  if (sec / 60 / 60 < 10) {
    Serial.print ("0");
  }
  Serial.print (sec / 60 / 60);
  Serial.print (":");
  if (sec / 60 % 60 < 10) {
    Serial.print ("0");
  }
  Serial.print ((sec / 60) % 60);
  Serial.print (":");
  if (sec % 60 < 10) {
    Serial.print ("0");
  }
  Serial.println (sec % 60);
}

void Service() {
  if (Serial.available() > 0) {
    Serial_val = Serial.read();

    if (Serial_val == 'h') {
      Serial.println ("Сервисный режим, команды:");
      Serial.println ("с - калибровка мотора");
      Serial.println ("q - вкл/выкл клапана набора воды, предварительной стирки");
      Serial.println ("w - вкл/выкл клапана набора воды, основной стирки");
      Serial.println ("W - вкл включение режима набора воды в режиме основной стирки WATER_LEVEL1 (полный автомат)");
      Serial.println ("t - вкл/выкл ТЭНА");
      Serial.println ("T - вкл режима подогрева воды, до 30 градусов (полный автомат)");
      Serial.println ("p - вкл/выкл насоса");
      Serial.println ("P - вкл режима откачки воды, с контролем уровня воды (полный автомат)");
      Serial.println ("o - отжим в режиме mode = 2 (Промежуточный отжим при полоскании)");
      Serial.println ("O - полоскание и отжим, с учетом всех настроек режимов (полный автомат)");
      Serial.println ("m - вкл/выкл ручное управление мотором");
      Serial.println ("r - вкл/выкл режима реверса мотора");
      Serial.println ("s - вкл режима стирки на 1800 сек (полный автомат)");
      Serial.println ("e - вкл режима полоскания на 360 сек (полный автомат)");
      Serial.println ("z - показать уровень");
      Serial.println ("x - стоп машина");
      Serial.println ("y - Конец машина");
      Serial.println ("I - ТЕСТ");
    }

    if (Serial_val == 'I') {
       pwmSet = 5;
       pwmSetMinMax();
       MotorStart();
        for (int j = 150; j <= 255; j = j + 1) {
          pwmOut = j;
          setPower(pwmOut);
          addLog("MOTOR_direction: " + String(MOTOR_direction) + " pwmSet: " + String(pwmSet) + " pwmSpeed:" + String(pwmSpeed) + " pwmOut:" + String(pwmOut) + " Dimmer:" + String(Dimmer));
          delay2(1000);
        }
      MotorStop();
      MotorWaitStop();
    }

    
    if (Serial_val == 'y') {
      EndMashine();
    }

    if (Serial_val == 'x') {
      StopMashine();
    }

    if (Serial_val == 'z') {
      Serial.println ("Уровень: " + String(current_water_level) + " Hz");
    }

    if (Serial_val == 'o') {
      spin(2);
    }
    if (Serial_val == 'i') {
      spin(3);
    }

    if (Serial_val == 'O') {
      rinsing_spin();
    }

    if (Serial_val == 'm') {
      WORK = true;
      PAUSE = false;
      digitalWrite(PIN_AC, HIGH);         // Включим 220V
      digitalWrite(PIN_BLOCK_DOOR, HIGH); // Заблокируем дверь

      debug_motor_manual = !debug_motor_manual;
      if (!debug_motor_manual) {
        setPower(0);
      }
    }

    if (Serial_val == 'q') {
      if (debug_water_valte1) {
        debug_water_valte1 = false;
        Serial.println("water 1 off");
        water_valte1_off ();
      } else {
        debug_water_valte1 = true;
        Serial.println("water 1 on");
        water_valte1_on (60);
      }
    }

    if (debug_water_valte2) {
      Serial.println("current_water_level: " + String(current_water_level) + "   TEN_current_temp: " + String(TEN_current_temp) + " current_water_level: " + String(current_water_level) + "   TEN_current_temp: " + String(analogRead(A0)));
    }

    if (Serial_val == 'w') {
      if (debug_water_valte2) {
        debug_water_valte2 = false;
        Serial.println("water 2 off");
        water_valte2_off ();

      } else {
        debug_water_valte2 = true;
        Serial.println("water 2 on");
        water_valte2_on (60);
      }
    }

    if (Serial_val == 'W') {
      water_pour(2, WATER_LEVEL1);
    }

    if (Serial_val == 'p') {
      if (debug_flag_pump) {
        debug_flag_pump = false;
        pump_off ();
      } else {
        debug_flag_pump = true;
        pump_on ();
      }
    }

    if (Serial_val == 'P') {
      drain_water ();
    }

    if (Serial_val == 't') {
      if (debug_ten) {
        debug_ten = false;
        ten_off ();
        Serial.println("ten off");
      } else {
        debug_ten = true;
        ten_on ();
        Serial.println("ten on");
      }
    }

    if (Serial_val == 'r') {
      if (debug_motor_reverce) {
        debug_motor_reverce = false;
        MOTOR_direction = false;
        digitalWrite(PIN_MOTOR_LR, MOTOR_direction); // Изменим направление вращения барабана
        delay2(1000); // Пауза, что бы реле успело сработать

        Serial.println("reverce off");
      } else {
        debug_motor_reverce = true;
        MOTOR_direction = true;
        digitalWrite(PIN_MOTOR_LR, MOTOR_direction); // Изменим направление вращения барабана

        delay2(1000); // Пауза, что бы реле успело сработать
        Serial.println("reverce on");
      }
    }


    if (Serial_val == 'T') {
      water_heating(30, 21);
      Serial.println("подогрев авто");
    }

    if (Serial_val == 'e') {
      Serial.println("полоскание");
      rinsing(240);
    }
    if (Serial_val == 's') {
      Serial.println("стирка");
      washing(1800, 30, washing_speed2, washing_pause1);
    }
    if (Serial_val == 'c') {
      Serial.println("калибровка мотора");
      SetMinimalPOWER();
      Motor_Calibration();

      unsigned long n_sum = 0;
      unsigned long sum = 0;
      int MOTOR_POWER_MINIMAL_qqq = 0;
      int sizeArray_POWER = sizeof(MOTOR_POWER_Array) / sizeof(int);
      for (int i = 0; i < sizeArray_POWER; i = i + 1) {
        if (MOTOR_POWER_Array[i] != 0) {
          sum = sum + MOTOR_POWER_Array[i];
          n_sum++;
          int qqq = MOTOR_POWER_Array[i];
          Serial.println("MOTOR_POWER_Array[" + String(i) + "]: " + String(qqq));
        }
      }
      if (n_sum != 0) {
        MOTOR_POWER_MINIMAL_qqq = round(sum / n_sum);
      }
      Serial.println("set sum: " + String(sum) + "set n_sum: " + String(n_sum));

      Serial.println("set MOTOR_POWER_MINIMAL_qqq: " + String(MOTOR_POWER_MINIMAL_qqq));
      Serial.println("set MOTOR_POWER_MINIMAL: " + String(MOTOR_POWER_MINIMAL));

    }


  }

  if (debug_motor_manual) {
    pwmSet1  =  analogRead(PIN_SET); // считываем показания потенциометра регулировки скорости
    pwmSet = map(pwmSet1, 0, 1023, 0, 255);
    //pwmOut = pwmSet;

    //if (pwmSet == 0) {
    //  pwmOut = 0;
    //}

    //Dimmer = 255 - pwmOut;

  }

  if (pwmOut > 11) {
    fl_Vibro_Control = true;
    getLevelVibro ();

    Serial.println("AcMid: " + String(AcMid) + " AcMax: " + String(AcMax) + " pwmSet1: " + String(pwmSet1) + " pwmSet: " + String(pwmSet) + " pwmSpeed:" + String(pwmSpeed) + " pwmOut:" + String(pwmOut) + " Dimmer:" + String(Dimmer) + " current_water_level: " + String(current_water_level) + "   TEN_current_temp: " + String(TEN_current_temp));
  }

}
