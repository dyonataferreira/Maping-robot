#pragma once
#include "variaveis.h"

// Aloca espaço estável máximo para até 15 amostras por sensor
float historicoSensores[4][15] = {{0}};
int indiceHistorico = 0;

float calcularMediana(float array[], int tamanho) {
  float temp[15];
  for(int i=0; i<tamanho; i++) temp[i] = array[i]; 
  
  for(int i=0; i<(tamanho-1); i++) {
    for(int j=i+1; j<tamanho; j++) {
      if(temp[i] > temp[j]) {
        float t = temp[i];
        temp[i] = temp[j];
        temp[j] = t;
      }
    }
  }
  return temp[tamanho / 2]; 
}

// Esta função agora calcula o timeout com base no range que você definiu no slider
float ler_sensor_ultra(int trig, int echo, int rangeMaxCm) {
  // A cada 1 cm, o som leva ~29 microssegundos (ida e volta)
  // Então: timeout = rangeMaxCm * 58 (aproximadamente)
  long timeout = rangeMaxCm * 58; 

  digitalWrite(trig, LOW); delayMicroseconds(2);
  digitalWrite(trig, HIGH); delayMicroseconds(10); digitalWrite(trig, LOW);
  
  long duracao = pulseIn(echo, HIGH, timeout); 
  if (duracao > 0) return (duracao * 0.0343) / 2.0;
  return rangeMaxCm; // Retorna o limite caso não detecte nada
}

void TaskMapeamento(void *pvParameters) {
  float angulos[4] = {0, PI, PI/2.0, -PI/2.0};
  
  for(int i=0; i<15; i++) {
     for(int s=0; s<4; s++) historicoSensores[s][i] = 400.0;
  }
  
  for(;;) { 
    if (indiceHistorico >= qtdAmostras) indiceHistorico = 0;

    for(int i = 0; i < 4; i++) {
      float leituraCrua = ler_sensor_ultra(TRIGS[i], ECHOS[i], rangeUltrassom);
      historicoSensores[i][indiceHistorico] = leituraCrua;
      
      float distMediana = calcularMediana(historicoSensores[i], qtdAmostras);
      distanciasAtuais[i] = distMediana; 

      if (estadoAtual == ESTADO_MANUAL || estadoAtual == ESTADO_MAPPING) {
        // Usa o rangeUltrassom definido pelo slider
        if(distMediana > 15 && distMediana < rangeUltrassom) {
          int obsX = roboX + (int)(distMediana/5 * cos(roboTheta + angulos[i]));
          int obsY = roboY + (int)(distMediana/5 * sin(roboTheta + angulos[i]));
          
          if(obsX >= 0 && obsX < MAP_SIZE && obsY >= 0 && obsY < MAP_SIZE) {
            if (xSemaphoreTake(xMutexMapa, pdMS_TO_TICKS(5)) == pdTRUE) {
              if(matrizMapa[obsX][obsY] < 30) matrizMapa[obsX][obsY] += 15; 
              xSemaphoreGive(xMutexMapa);
            }
          }
        }
      }
    }
    
    indiceHistorico = (indiceHistorico + 1) % qtdAmostras;
    vTaskDelay(pdMS_TO_TICKS(10)); 
  }
}