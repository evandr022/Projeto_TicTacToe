#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "WS2812.hpp"
#include "pico/time.h"

// Configurações do hardware
#define LED_PIN 7
#define LED_LENGTH 25
#define JOYSTICK_X_PIN 26  // ADC0
#define JOYSTICK_Y_PIN 27  // ADC1
#define JOYSTICK_BUTTON_PIN 22
#define DEBOUNCE_DELAY_MS 200
#define BOTTON_RESET_PIN 5
// #define BUZZER_PIN 6 // Sinalizar que ganhou, perdeu ou empatou com efeitos sonoros

// Estrutura para representar uma posição no tabuleiro
typedef struct {
    uint8_t x;
    uint8_t y;
} Position;

// Mapeamento da matriz de LEDs
const Position ledMap[3][3] = {
    {{0,0}, {2,0}, {4,0}},
    {{0,2}, {2,2}, {4,2}},
    {{0,4}, {2,4}, {4,4}}
};

// Cores melhoradas
const uint32_t COLOR_GRID = WS2812::RGB(2, 2, 0);
const uint32_t COLOR_CURSOR = WS2812::RGB(0, 0, 30);
const uint32_t COLOR_PLAYER1 = WS2812::RGB(30, 0, 0);
const uint32_t COLOR_PLAYER2 = WS2812::RGB(0, 0, 30);
const uint32_t COLOR_WIN = WS2812::RGB(0, 30, 0);
const uint32_t COLOR_DRAW = WS2812::RGB(20, 20, 0);

// Estado do jogo
uint8_t board[3][3] = {0}; // 0 = vazio, 1 = IA, 2 = Humano
uint8_t currentPlayer = 1;  // IA começa
Position cursor = {1, 1};   // Posição do cursor
bool gameActive = true;

// Protótipos de funções
void initHardware();
void drawBoard(WS2812& ledStrip);
void updateCursor(WS2812& ledStrip, int dx, int dy);
void processInput(WS2812& ledStrip);
void makeAIMove();
void checkGameState(WS2812& ledStrip);
void resetGame(WS2812& ledStrip);
void showWinAnimation(WS2812& ledStrip, uint8_t player);
void showDrawAnimation(WS2812& ledStrip);
int evaluateBoard();
int minimax(int depth, bool isMaximizing, int alpha, int beta);
Position findBestMove();
bool checkWin(uint8_t player);
bool isBoardFull();
void flashPosition(WS2812& ledStrip, Position pos, uint32_t color);

// Inicialização do hardware
void initHardware() {
    stdio_init_all();
    
    // Inicializa ADC para o joystick
    adc_init();
    adc_gpio_init(JOYSTICK_X_PIN);
    adc_gpio_init(JOYSTICK_Y_PIN);
    
    // Inicializa botão do joystick
    gpio_init(JOYSTICK_BUTTON_PIN);
    gpio_set_dir(JOYSTICK_BUTTON_PIN, GPIO_IN);
    gpio_pull_up(JOYSTICK_BUTTON_PIN);

    // Inicializa botão de reset
    gpio_init(BOTTON_RESET_PIN);
    gpio_set_dir(BOTTON_RESET_PIN, GPIO_IN);
    gpio_pull_up(BOTTON_RESET_PIN);
    
    // Inicializa gerador de números aleatórios
    srand(to_ms_since_boot(get_absolute_time()));
}

// Função principal
int main() {
    // Inicializa hardware
    initHardware();
    
    // Inicializa fita de LED
    WS2812 ledStrip(LED_PIN, LED_LENGTH, pio0, 0, WS2812::FORMAT_GRB);
    
    // Desenha tabuleiro inicial
    drawBoard(ledStrip);
    
    // Loop principal do jogo
    while (true) {
        if (gameActive) {
            if (currentPlayer == 1) { // Vez da IA
                sleep_ms(500); // Atraso para parecer mais humano
                makeAIMove();
                drawBoard(ledStrip);
                checkGameState(ledStrip);
            } else { // Vez do jogador humano
                processInput(ledStrip);
            }
        } else {
            // Jogo inativo (entre partidas)
            processInput(ledStrip);
        }
        
        sleep_ms(10); // Pequena pausa para reduzir carga da CPU
    }
    
    return 0;
}

// Mapeamento da matriz de LEDs
const int gridIndices[5][5] = {
    { 0,  1,  2,  3,  4},
    { 5,  6,  7,  8,  9},
    {10, 11, 12, 13, 14},
    {15, 16, 17, 18, 19},
    {20, 21, 22, 23, 24}
};

// Desenha o tabuleiro completo
void drawBoard(WS2812& ledStrip) {
    ledStrip.fill(WS2812::RGB(0, 0, 0)); // Limpa tudo
    
    // Desenha grade
    for (uint8_t x = 1; x < 5; x += 2) {
        for (uint8_t y = 0; y < 5; y++) {
            ledStrip.setPixelColor(gridIndices[y][x], COLOR_GRID);
        }
    }
    for (uint8_t y = 1; y < 5; y += 2) {
        for (uint8_t x = 0; x < 5; x++) {
            ledStrip.setPixelColor(gridIndices[y][x], COLOR_GRID);
        }
    }
    
    // Desenha jogadas
    for (uint8_t y = 0; y < 3; y++) {
        for (uint8_t x = 0; x < 3; x++) {
            if (board[y][x] == 1) {
                ledStrip.setPixelColor(gridIndices[ledMap[y][x].y][ledMap[y][x].x], COLOR_PLAYER1);
            } else if (board[y][x] == 2) {
                ledStrip.setPixelColor(gridIndices[ledMap[y][x].y][ledMap[y][x].x], COLOR_PLAYER2);
            }
        }
    }
    
    // Desenha cursor (se for vez do humano e jogo ativo)
    if (gameActive && currentPlayer == 2 && board[cursor.y][cursor.x] == 0) {
        ledStrip.setPixelColor(gridIndices[ledMap[cursor.y][cursor.x].y][ledMap[cursor.y][cursor.x].x], COLOR_CURSOR);
    }
    
    ledStrip.show();
}

// Processa entrada do jogador
void processInput(WS2812& ledStrip) {
    static uint32_t lastMoveTime = 0;
    static bool lastButtonState = false;
    static bool lastResetState = false;
    
    // Lê joystick
    int dx = 0, dy = 0;
    bool buttonPressed = !gpio_get(JOYSTICK_BUTTON_PIN);

    // Lê botão de reset
    bool resetPressed = !gpio_get(BOTTON_RESET_PIN);

    // Reinicia o jogo se o botão de reset for pressionado
    if (resetPressed && !lastResetState) {
        resetGame(ledStrip);
    }
    lastResetState = resetPressed;
    
    // Lê eixos apenas se o jogo estiver ativo
    if (gameActive) {
        adc_select_input(0);
        int xValue = adc_read();
        adc_select_input(1);
        int yValue = adc_read();
        
        dx = (yValue < 1000) ? 1 : (yValue > 3000) ? -1 : 0;
        dy = (xValue < 1000) ? 1 : (xValue > 3000) ? -1 : 0;
    }
    
    // Atualiza cursor
    uint32_t currentTime = to_ms_since_boot(get_absolute_time());
    if (currentTime - lastMoveTime > DEBOUNCE_DELAY_MS) {
        if (dx != 0 || dy != 0) {
            cursor.x = (cursor.x + dx + 3) % 3;
            cursor.y = (cursor.y - dy + 3) % 3;
            drawBoard(ledStrip);
            lastMoveTime = currentTime;
        }
    }
    
    // Processa botão
    if (buttonPressed && !lastButtonState) {
        if (!gameActive) {
            // Reinicia o jogo se pressionado quando inativo
            resetGame(ledStrip);
        } else if (currentPlayer == 2 && board[cursor.y][cursor.x] == 0) {
            // Faz jogada humana
            board[cursor.y][cursor.x] = 2;
            flashPosition(ledStrip, cursor, COLOR_PLAYER2);
            checkGameState(ledStrip);
        }
    }
    lastButtonState = buttonPressed;
}

// Implementação melhorada da IA
void makeAIMove() {
    // Verifica se pode ganhar na próxima jogada
    for (uint8_t y = 0; y < 3; y++) {
        for (uint8_t x = 0; x < 3; x++) {
            if (board[y][x] == 0) {
                board[y][x] = 1;
                if (checkWin(1)) {
                    return; // Jogada vencedora encontrada
                }
                board[y][x] = 0;
            }
        }
    }
    
    // Verifica se precisa bloquear o jogador
    for (uint8_t y = 0; y < 3; y++) {
        for (uint8_t x = 0; x < 3; x++) {
            if (board[y][x] == 0) {
                board[y][x] = 2;
                if (checkWin(2)) {
                    board[y][x] = 1; // Bloqueia jogada vencedora
                    return;
                }
                board[y][x] = 0;
            }
        }
    }
    
    // Estratégias de abertura
    /*bool isFirstMove = true;
    for (uint8_t y = 0; y < 3; y++) {
        for (uint8_t x = 0; x < 3; x++) {
            if (board[y][x] != 0) {
                isFirstMove = false;
                break;
            }
        }
    }
    
    if (isFirstMove) {
        // Joga em um canto aleatório no primeiro movimento
        uint8_t corners[4][2] = {{0,0}, {0,2}, {2,0}, {2,2}};
        uint8_t choice = rand() % 4;
        board[corners[choice][1]][corners[choice][0]] = 1;
        return;
    }
    
    // Se o centro estiver livre, prioriza
    if (board[1][1] == 0) {
        board[1][1] = 1;
        return;
    }
    
    // Usa Minimax para outras situações
    Position bestMove = findBestMove();
    board[bestMove.y][bestMove.x] = 1;*/

    // Primeiro conta quantas posições estão vazias
    uint8_t emptySpots = 0;
    for (uint8_t y = 0; y < 3; y++) {
        for (uint8_t x = 0; x < 3; x++) {
            if (board[y][x] == 0) emptySpots++;
        }
    }

    // Escolhe um índice aleatório
    uint8_t randomChoice = rand() % emptySpots;
    uint8_t count = 0;

    // Encontra a posição correspondente ao índice escolhido
    for (uint8_t y = 0; y < 3; y++) {
        for (uint8_t x = 0; x < 3; x++) {
            if (board[y][x] == 0) {
                if (count == randomChoice) {
                    board[y][x] = 1;
                    return;
                }
                count++;
            }
        }
    }
}

// Implementação do Minimax com poda alfa-beta
Position findBestMove() {
    int bestScore = -1000;
    Position bestMove = {0, 0};
    
    for (uint8_t y = 0; y < 3; y++) {
        for (uint8_t x = 0; x < 3; x++) {
            if (board[y][x] == 0) {
                board[y][x] = 1;
                int score = minimax(0, false, -1000, 1000);
                board[y][x] = 0;
                
                if (score > bestScore) {
                    bestScore = score;
                    bestMove = (Position){x, y};
                }
            }
        }
    }
    
    return bestMove;
}

int minimax(int depth, bool isMaximizing, int alpha, int beta) {
    int score = evaluateBoard();
    
    if (score == 10) return score - depth;
    if (score == -10) return score + depth;
    if (isBoardFull()) return 0;
    
    if (isMaximizing) {
        int maxEval = -1000;
        for (uint8_t y = 0; y < 3; y++) {
            for (uint8_t x = 0; x < 3; x++) {
                if (board[y][x] == 0) {
                    board[y][x] = 1;
                    int eval = minimax(depth + 1, false, alpha, beta);
                    board[y][x] = 0;
                    maxEval = (eval > maxEval) ? eval : maxEval;
                    alpha = (alpha > maxEval) ? alpha : maxEval;
                    if (beta <= alpha) break;
                }
            }
        }
        return maxEval;
    } else {
        int minEval = 1000;
        for (uint8_t y = 0; y < 3; y++) {
            for (uint8_t x = 0; x < 3; x++) {
                if (board[y][x] == 0) {
                    board[y][x] = 2;
                    int eval = minimax(depth + 1, true, alpha, beta);
                    board[y][x] = 0;
                    minEval = (eval < minEval) ? eval : minEval;
                    beta = (beta < minEval) ? beta : minEval;
                    if (beta <= alpha) break;
                }
            }
        }
        return minEval;
    }
}

// Verifica o estado atual do jogo
void checkGameState(WS2812& ledStrip) {
    if (checkWin(currentPlayer)) {
        showWinAnimation(ledStrip, currentPlayer);
        gameActive = false;
    } else if (isBoardFull()) {
        showDrawAnimation(ledStrip);
        gameActive = false;
    } else {
        currentPlayer = (currentPlayer == 1) ? 2 : 1;
    }
}

// Reinicia o jogo
void resetGame(WS2812& ledStrip) {
    for (uint8_t y = 0; y < 3; y++) {
        for (uint8_t x = 0; x < 3; x++) {
            board[y][x] = 0;
        }
    }
    
    cursor = (Position){1, 1};
    currentPlayer = 1;
    gameActive = true;
    
    // Faz a primeira jogada da IA
    // makeAIMove();
    drawBoard(ledStrip);
}

// Animação de vitória
void showWinAnimation(WS2812& ledStrip, uint8_t player) {
    uint32_t color = (player == 1) ? COLOR_PLAYER1 : COLOR_PLAYER2;
    
    for (uint8_t i = 0; i < 5; i++) {
        ledStrip.fill((i % 2 == 0) ? color : WS2812::RGB(0, 0, 0));
        ledStrip.show();
        sleep_ms(200);
    }
    
    drawBoard(ledStrip);
}

// Animação de empate
void showDrawAnimation(WS2812& ledStrip) {
    for (uint8_t i = 0; i < 3; i++) {
        ledStrip.fill((i % 2 == 0) ? COLOR_DRAW : WS2812::RGB(0, 0, 0));
        ledStrip.show();
        sleep_ms(300);
    }
    
    drawBoard(ledStrip);
}

// Pisca uma posição específica
void flashPosition(WS2812& ledStrip, Position pos, uint32_t color) {
    for (uint8_t i = 0; i < 3; i++) {
        ledStrip.setPixelColor(gridIndices[ledMap[pos.y][pos.x].y][ledMap[pos.y][pos.x].x], 
                              (i % 2 == 0) ? color : WS2812::RGB(0, 0, 0));
        ledStrip.show();
        sleep_ms(100);
    }
}

// Funções auxiliares do jogo
int evaluateBoard() {
    if (checkWin(1)) return 10;
    if (checkWin(2)) return -10;
    return 0;
}

bool checkWin(uint8_t player) {
    // Verifica linhas e colunas
    for (uint8_t i = 0; i < 3; i++) {
        if (board[i][0] == player && board[i][1] == player && board[i][2] == player) return true;
        if (board[0][i] == player && board[1][i] == player && board[2][i] == player) return true;
    }
    
    // Verifica diagonais
    if (board[0][0] == player && board[1][1] == player && board[2][2] == player) return true;
    if (board[0][2] == player && board[1][1] == player && board[2][0] == player) return true;
    
    return false;
}

bool isBoardFull() {
    for (uint8_t y = 0; y < 3; y++) {
        for (uint8_t x = 0; x < 3; x++) {
            if (board[y][x] == 0) return false;
        }
    }
    return true;
}