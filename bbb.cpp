#include <stdio.h>
#include <math.h>
#include "pico/stdlib.h"
#include "WS2812.hpp"
#include "hardware/adc.h"

#define LED_PIN 7
#define LED_LENGTH 25
#define JOYSTICK_X_PIN 26
#define JOYSTICK_Y_PIN 27
#define JOYSTICK_BUTTON_PIN 22

// Mapeamento da matriz de LEDs
const int gridIndices[5][5] = {
    { 0,  1,  2,  3,  4},
    { 5,  6,  7,  8,  9},
    {10, 11, 12, 13, 14},
    {15, 16, 17, 18, 19},
    {20, 21, 22, 23, 24}
};

// Cores
const uint32_t COLOR_GRID = WS2812::RGB(5, 5, 0);    // Grade branca fraca
const uint32_t COLOR_CURSOR = WS2812::RGB(0, 20, 0); // Cursor verde
const uint32_t COLOR_PLAYER1 = WS2812::RGB(20, 0, 0);// Jogador 1 vermelho
const uint32_t COLOR_PLAYER2 = WS2812::RGB(0, 0, 20);// Jogador 2 azul

// Estado do jogo
int board[3][3] = {0}; // Tabuleiro lógico 3x3
int currentPlayer = 1;  // 1 ou 2
int cursorX = 1;        // Posição do cursor (0-2)
int cursorY = 1;        // Posição do cursor (0-2)

// Função para mapear posição lógica para física
int getPhysicalPos(int logicalX, int logicalY) {
    // Converte coordenadas lógicas (0-2) para físicas (0,2,4)
    int physicalX = logicalX * 2;
    int physicalY = logicalY * 2;
    return gridIndices[physicalY][physicalX];
}

// Função para desenhar o tabuleiro
void drawBoard(WS2812& ledStrip) {
    ledStrip.fill(WS2812::RGB(0, 0, 0)); // Limpa tudo
    
    // Desenha grade
    for (int x = 0; x < 5; x++) {
        ledStrip.setPixelColor(gridIndices[1][x], COLOR_GRID);
        ledStrip.setPixelColor(gridIndices[3][x], COLOR_GRID);
    }
    for (int y = 0; y < 5; y++) {
        ledStrip.setPixelColor(gridIndices[y][1], COLOR_GRID);
        ledStrip.setPixelColor(gridIndices[y][3], COLOR_GRID);
    }
    
    // Desenha jogadas
    for (int y = 0; y < 3; y++) {
        for (int x = 0; x < 3; x++) {
            if (board[y][x] == 1) {
                ledStrip.setPixelColor(getPhysicalPos(x, y), COLOR_PLAYER1);
            } else if (board[y][x] == 2) {
                ledStrip.setPixelColor(getPhysicalPos(x, y), COLOR_PLAYER2);
            }
        }
    }
    
    // Desenha cursor
    if (board[cursorY][cursorX] == 0) { // Só mostra cursor em posições vazias
        ledStrip.setPixelColor(getPhysicalPos(cursorX, cursorY), COLOR_CURSOR);
    }
    
    ledStrip.show();
}

// Função para ler joystick
void readJoystick(int& dx, int& dy, bool& buttonPressed) {
    // Lê eixo X (ADC0 - GP26)
    adc_select_input(0); // Seleciona ADC0
    int xValue = adc_read();
    
    // Lê eixo Y (ADC1 - GP27)
    adc_select_input(1); // Seleciona ADC1
    int yValue = adc_read();
    // Converte para direção (-1, 0, 1)
    dx = 0;
    dy = 0;
    
    if (xValue < 1000) dx = -1;       // Movimento para esquerda
    else if (xValue > 3000) dx = 1;   // Movimento para direita
    
    if (yValue < 1000) dy = -1;       // Movimento para cima
    else if (yValue > 3000) dy = 1;   // Movimento para baixo
    
    // Lê botão (ativo baixo)
    buttonPressed = !gpio_get(JOYSTICK_BUTTON_PIN);
}

// Função para verificar vitória
bool checkWin() {
    // Verifica linhas e colunas
    for (int i = 0; i < 3; i++) {
        if (board[i][0] != 0 && board[i][0] == board[i][1] && board[i][1] == board[i][2]) return true;
        if (board[0][i] != 0 && board[0][i] == board[1][i] && board[1][i] == board[2][i]) return true;
    }
    // Verifica diagonais
    if (board[0][0] != 0 && board[0][0] == board[1][1] && board[1][1] == board[2][2]) return true;
    if (board[0][2] != 0 && board[0][2] == board[1][1] && board[1][1] == board[2][0]) return true;
    
    return false;
}

int main() {
    stdio_init_all();
    
    // Inicializa LEDs
    WS2812 ledStrip(LED_PIN, LED_LENGTH, pio0, 0, WS2812::FORMAT_GRB);
    
    // Inicializa ADC para joystick
    adc_init();
    adc_gpio_init(JOYSTICK_X_PIN);
    adc_gpio_init(JOYSTICK_Y_PIN);
    adc_select_input(0); // Canal 0 (GP26)
    
    // Inicializa botão do joystick
    gpio_init(JOYSTICK_BUTTON_PIN);
    gpio_set_dir(JOYSTICK_BUTTON_PIN, GPIO_IN);
    gpio_pull_up(JOYSTICK_BUTTON_PIN);
    
    // Variáveis para controle do joystick
    int dx, dy;
    bool buttonPressed = false;
    bool lastButtonState = false;
    uint32_t lastMoveTime = 0;
    
    // Desenha tabuleiro inicial
    drawBoard(ledStrip);
    
    while (true) {
        // Lê joystick
        readJoystick(dx, dy, buttonPressed);
        
        // Processa movimento do cursor
        if (to_ms_since_boot(get_absolute_time()) - lastMoveTime > 200) { // Debounce
            if (dx != 0 || dy != 0) {
                cursorX = (cursorX + dx + 3) % 3; // Mantém dentro do range 0-2
                cursorY = (cursorY - dy + 3) % 3; // Inverte Y (joystick pode estar invertido)
                lastMoveTime = to_ms_since_boot(get_absolute_time());
                drawBoard(ledStrip);
            }
        }
        
        // Processa botão (confirmação)
        if (buttonPressed && !lastButtonState) {
            if (board[cursorY][cursorX] == 0) { // Só marca se estiver vazio
                board[cursorY][cursorX] = currentPlayer;
                
                if (checkWin()) {
                    // Pisca as luzes para indicar vitória
                    for (int i = 0; i < 3; i++) {
                        ledStrip.fill(currentPlayer == 1 ? COLOR_PLAYER1 : COLOR_PLAYER2);
                        ledStrip.show();
                        sleep_ms(200);
                        drawBoard(ledStrip);
                        sleep_ms(200);
                    }
                    // Reinicia o jogo
                    for (int y = 0; y < 3; y++)
                        for (int x = 0; x < 3; x++)
                            board[y][x] = 0;
                    currentPlayer = 1;
                } else {
                    // Alterna jogador
                    currentPlayer = currentPlayer == 1 ? 2 : 1;
                }
                
                drawBoard(ledStrip);
            }
        }
        lastButtonState = buttonPressed;
        
        sleep_ms(10); // Pequena pausa para evitar leituras muito rápidas
    }
    
    return 0;
}