#pragma once
#include <Arduino.h>
#include <RoboCore_Vespa.h>
#include <Wire.h>

// --- MÁQUINA DE ESTADOS FINITOS (FSM) ---
enum EstadoRobo {
  ESTADO_IDLE,         
  ESTADO_MANUAL,       
  ESTADO_MAPPING,      
  ESTADO_EMERGENCY     
};

extern EstadoRobo estadoAtual;
void mudarEstado(EstadoRobo novoEstado);

// --- MONITOR DE SEQUÊNCIA (LOGGER) ---
void logEvento(String id, String acao);

// --- CONFIGURAÇÃO DA MATRIZ ---
#define MAP_SIZE 100
extern int matrizMapa[MAP_SIZE][MAP_SIZE];

extern const int TRIGS[4]; 
extern const int ECHOS[4]; 
extern float distanciasAtuais[4]; 

// --- ODOMETRIA E POSIÇÃO ---
extern float roboTheta; 
extern float roboX_cm; 
extern float roboY_cm;
extern int roboX; 
extern int roboY;

// Variáveis de calibração dinâmica vindas da Web
extern float fatorOdometria;
extern int rangeUltrassom; // Valor padrão em cm
extern int qtdAmostras;
extern int raioBorracha;

extern int velE_global;
extern int velD_global;
extern uint32_t ultimoTempoOdom;

// --- OBJETOS GLOBAIS ---
extern SemaphoreHandle_t xMutexMapa;
extern VespaMotors motores;
extern VespaBattery vbat;