#pragma once
#include "variaveis.h"

const int MPU = 0x68; 

float AccX, AccY, AccZ;
float GyroX, GyroY, GyroZ;
float AccErrorX, AccErrorY, GyroErrorX, GyroErrorY, GyroErrorZ;
float accAngleX, accAngleY, gyroAngleX, gyroAngleY, gyroAngleZ;
float roll, pitch, yaw;
float elapsedTime;
unsigned long currentTimeMPU, previousTimeMPU;

const int NUM_AMOSTRAS = 10;
float leiturasZ[NUM_AMOSTRAS];
int indiceZ = 0;
float totalZ = 0;
float yawFiltrado = 0;

void mpu_begin() {
  Wire.begin();
  Wire.setClock(400000); 

  Wire.beginTransmission(MPU);
  Wire.write(0x6B); 
  Wire.write(0x00); 
  Wire.endTransmission(true);

  Wire.beginTransmission(MPU);
  Wire.write(0x1C); Wire.write(0); Wire.endTransmission(true);
  Wire.beginTransmission(MPU);
  Wire.write(0x1B); Wire.write(0); Wire.endTransmission(true);
  delay(20);
}

void resetarAnguloMpu() {
  gyroAngleX = 0; gyroAngleY = 0; gyroAngleZ = 0;
  roll = 0; pitch = 0; yaw = 0;
  yawFiltrado = 0;
  totalZ = 0;
  indiceZ = 0;
  for (int i = 0; i < NUM_AMOSTRAS; i++) leiturasZ[i] = 0;
  roboTheta = -PI / 2.0;
}

void readData() {
  Wire.beginTransmission(MPU);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 14, true);

  int16_t ax = (Wire.read() << 8) | Wire.read();
  int16_t ay = (Wire.read() << 8) | Wire.read();
  int16_t az = (Wire.read() << 8) | Wire.read();
  Wire.read(); Wire.read(); 
  int16_t gx = (Wire.read() << 8) | Wire.read();
  int16_t gy = (Wire.read() << 8) | Wire.read();
  int16_t gz = (Wire.read() << 8) | Wire.read();

  AccX = ax / 16384.0f; AccY = ay / 16384.0f; AccZ = az / 16384.0f;
  GyroX = gx / 131.0f; GyroY = gy / 131.0f; GyroZ = gz / 131.0f;
}

void mpu_calibrate(int leituras) {
  for (int i = 0; i < leituras; i++) {
    readData();
    AccErrorX += (atan(AccY / sqrt(pow(AccX, 2) + pow(AccZ, 2))) * 180 / PI);
    AccErrorY += (atan(-1 * AccX / sqrt(pow(AccY, 2) + pow(AccZ, 2))) * 180 / PI);
    GyroErrorX += GyroX; GyroErrorY += GyroY; GyroErrorZ += GyroZ;
    delay(3);
  }
  AccErrorX /= leituras; AccErrorY /= leituras;
  GyroErrorX /= leituras; GyroErrorY /= leituras; GyroErrorZ /= leituras;
}

float aplicarMediaMovelZ(float novaLeitura) {
  totalZ = totalZ - leiturasZ[indiceZ];
  leiturasZ[indiceZ] = novaLeitura;
  totalZ = totalZ + leiturasZ[indiceZ];
  indiceZ = (indiceZ + 1) % NUM_AMOSTRAS;
  return totalZ / NUM_AMOSTRAS;
}

void mpu_loop() {
  previousTimeMPU = currentTimeMPU;
  currentTimeMPU = micros();
  elapsedTime = (currentTimeMPU - previousTimeMPU) / 1000000.0f;

  readData();

  accAngleX = (atan(AccY / sqrt(pow(AccX, 2) + pow(AccZ, 2))) * 180 / PI) - AccErrorX;
  accAngleY = (atan(-1 * AccX / sqrt(pow(AccY, 2) + pow(AccZ, 2))) * 180 / PI) - AccErrorY;

  GyroX -= GyroErrorX; 
  GyroY -= GyroErrorY; 
  GyroZ -= GyroErrorZ;
  
  if (abs(GyroZ) < 1.5) GyroZ = 0; 

  static float lastGyroZ = 0;
  GyroZ = (GyroZ * 0.7) + (lastGyroZ * 0.3);
  lastGyroZ = GyroZ;

  gyroAngleX += GyroX * elapsedTime;
  gyroAngleY += GyroY * elapsedTime;
  gyroAngleZ += GyroZ * elapsedTime;

  roll = 0.96 * gyroAngleX + 0.04 * accAngleX;
  pitch = 0.96 * gyroAngleY + 0.04 * accAngleY;
  yaw = gyroAngleZ;
  
  yawFiltrado = aplicarMediaMovelZ(yaw);
  roboTheta = (yawFiltrado * PI / 180.0) - (PI / 2.0);
}
