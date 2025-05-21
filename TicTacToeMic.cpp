// Conteúdo corrigido de tic_tac_toe_mic.cpp
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "WS2812.hpp"
#include "pico/time.h"
#include "TicTacToeMic.hpp"
#include <vector>
#include <algorithm>

#define LED_PIN 7
#define LED_LENGTH 25
#define DEBOUNCE_DELAY_MS 200
#define BOTTON_RESET_PIN 5
#define ADC_VREF 3.3
#define ADC_RANGE (1 << 12)
#define ADC_CONVERT (ADC_VREF / (ADC_RANGE - 1))
#define THRESHOLD_VOLTS 1.5f
#define ANALISE_JANELA_MS 1000
#define BATIDA_TIMEOUT_MS 20
#define MIC_PIN 28

const Position ledMap[3][3] = {
    {{0,0}, {2,0}, {4,0}},
    {{0,2}, {2,2}, {4,2}},
    {{0,4}, {2,4}, {4,4}}
};

const uint32_t COLOR_GRID = WS2812::RGB(2, 2, 0);
const uint32_t COLOR_CURSOR = WS2812::RGB(0, 0, 30);
const uint32_t COLOR_PLAYER1 = WS2812::RGB(30, 0, 0);
const uint32_t COLOR_PLAYER2 = WS2812::RGB(0, 0, 30);
const uint32_t COLOR_WIN = WS2812::RGB(0, 30, 0);
const uint32_t COLOR_DRAW = WS2812::RGB(20, 20, 0);

const int gridIndices[5][5] = {
    { 0,  1,  2,  3,  4},
    { 5,  6,  7,  8,  9},
    {10, 11, 12, 13, 14},
    {15, 16, 17, 18, 19},
    {20, 21, 22, 23, 24}
};

TicTacToeMic::TicTacToeMic()
    : ledStrip(LED_PIN, LED_LENGTH, pio0, 0, WS2812::FORMAT_GRB),
      currentPlayer(1), gameActive(true), cursor({1, 1}), acima_threshold(false)
{
    // Zera o tabuleiro
    for (uint8_t y = 0; y < 3; y++)
        for (uint8_t x = 0; x < 3; x++)
            board[y][x] = 0;

    initHardware();
}


void TicTacToeMic::run() {
    drawBoard();
    while (true) {
        if (gameActive) {
            if (currentPlayer == 1) {
                sleep_ms(500);
                makeAIMove();
                drawBoard();
                checkGameState();
            } else {
                processClaps();
            }
        } else {
            processClaps();
        }
        sleep_ms(10);
    }
}

void TicTacToeMic::initHardware() {
    stdio_init_all();
    adc_init();
    adc_gpio_init(MIC_PIN);
    adc_select_input(2);
    gpio_init(BOTTON_RESET_PIN);
    gpio_set_dir(BOTTON_RESET_PIN, GPIO_IN);
    gpio_pull_up(BOTTON_RESET_PIN);
    srand(to_ms_since_boot(get_absolute_time()));
}

void TicTacToeMic::drawBoard() {
    ledStrip.fill(WS2812::RGB(0, 0, 0));
    for (uint8_t x = 1; x < 5; x += 2)
        for (uint8_t y = 0; y < 5; y++)
            ledStrip.setPixelColor(gridIndices[y][x], COLOR_GRID);
    for (uint8_t y = 1; y < 5; y += 2)
        for (uint8_t x = 0; x < 5; x++)
            ledStrip.setPixelColor(gridIndices[y][x], COLOR_GRID);
    for (uint8_t y = 0; y < 3; y++)
        for (uint8_t x = 0; x < 3; x++) {
            if (board[y][x] == 1)
                ledStrip.setPixelColor(gridIndices[ledMap[y][x].y][ledMap[y][x].x], COLOR_PLAYER1);
            else if (board[y][x] == 2)
                ledStrip.setPixelColor(gridIndices[ledMap[y][x].y][ledMap[y][x].x], COLOR_PLAYER2);
        }
    if (gameActive && currentPlayer == 2 && board[cursor.y][cursor.x] == 0)
        ledStrip.setPixelColor(gridIndices[ledMap[cursor.y][cursor.x].y][ledMap[cursor.y][cursor.x].x], COLOR_CURSOR);
    ledStrip.show();
}

void TicTacToeMic::processClaps() {
    uint16_t adc_raw = adc_read();
    float volts = (adc_raw * ADC_CONVERT);
    if (!acima_threshold && volts > THRESHOLD_VOLTS) {
        acima_threshold = true;
        uint32_t t = to_ms_since_boot(get_absolute_time());
        batidas.push_back(t);
        printf("Batida detectada (%.2f V) em %u ms\n", volts, t);
    } else if (volts < THRESHOLD_VOLTS * 0.6f) {
        acima_threshold = false;
    }
    uint32_t agora = to_ms_since_boot(get_absolute_time());
    batidas.erase(std::remove_if(batidas.begin(), batidas.end(),
        [agora](uint32_t t) { return agora - t > ANALISE_JANELA_MS; }), batidas.end());
    if (!batidas.empty()) {
        uint32_t tempo_desde_ultima = agora - batidas.back();
        if (tempo_desde_ultima > BATIDA_TIMEOUT_MS) {
            if (batidas.size() == 1)
                moveCursor();
            else
                makeMove();
            batidas.clear();
        }
    }
    if (!gpio_get(BOTTON_RESET_PIN)) {
        resetGame();
    }
    printf("Volts: %.2f\n", volts);
}

void TicTacToeMic::moveCursor() {
    if (!gameActive || currentPlayer != 2) return;

    for (int i = 0; i < 9; i++) {
        // Avança para próxima posição
        cursor.x = (cursor.x + 1) % 3;
        if (cursor.x == 0)
            cursor.y = (cursor.y + 1) % 3;

        // Se a casa estiver vazia, para o loop
        if (board[cursor.y][cursor.x] == 0) break;
    }

    drawBoard();
}

void TicTacToeMic::makeMove() {
    if (!gameActive || currentPlayer != 2 || board[cursor.y][cursor.x] != 0) return;
    board[cursor.y][cursor.x] = 2;
    flashPosition(cursor, COLOR_PLAYER2);
    checkGameState();
}

void TicTacToeMic::makeAIMove() {
    for (uint8_t y = 0; y < 3; y++)
        for (uint8_t x = 0; x < 3; x++)
            if (board[y][x] == 0) {
                board[y][x] = 1;
                if (checkWin(1)) return;
                board[y][x] = 0;
            }
    for (uint8_t y = 0; y < 3; y++)
        for (uint8_t x = 0; x < 3; x++)
            if (board[y][x] == 0) {
                board[y][x] = 2;
                if (checkWin(2)) {
                    board[y][x] = 1;
                    return;
                }
                board[y][x] = 0;
            }
    uint8_t emptySpots = 0;
    for (uint8_t y = 0; y < 3; y++)
        for (uint8_t x = 0; x < 3; x++)
            if (board[y][x] == 0) emptySpots++;
    uint8_t randomChoice = rand() % emptySpots;
    uint8_t count = 0;
    for (uint8_t y = 0; y < 3; y++)
        for (uint8_t x = 0; x < 3; x++)
            if (board[y][x] == 0) {
                if (count == randomChoice) {
                    board[y][x] = 1;
                    return;
                }
                count++;
            }
}

void TicTacToeMic::checkGameState() {
    if (checkWin(currentPlayer)) {
        showWinAnimation(currentPlayer);
        gameActive = false;
    } else if (isBoardFull()) {
        showDrawAnimation();
        gameActive = false;
    } else {
        currentPlayer = (currentPlayer == 1) ? 2 : 1;
    }
}

void TicTacToeMic::resetGame() {
    for (uint8_t y = 0; y < 3; y++)
        for (uint8_t x = 0; x < 3; x++)
            board[y][x] = 0;
    cursor = {1, 1};
    currentPlayer = 1;
    gameActive = true;
    drawBoard();
}

void TicTacToeMic::showWinAnimation(uint8_t player) {
    uint32_t color = (player == 1) ? COLOR_PLAYER1 : COLOR_PLAYER2;
    for (uint8_t i = 0; i < 5; i++) {
        ledStrip.fill((i % 2 == 0) ? color : WS2812::RGB(0, 0, 0));
        ledStrip.show();
        sleep_ms(200);
    }
    drawBoard();
}

void TicTacToeMic::showDrawAnimation() {
    for (uint8_t i = 0; i < 3; i++) {
        ledStrip.fill((i % 2 == 0) ? COLOR_DRAW : WS2812::RGB(0, 0, 0));
        ledStrip.show();
        sleep_ms(300);
    }
    drawBoard();
}

void TicTacToeMic::flashPosition(Position pos, uint32_t color) {
    for (uint8_t i = 0; i < 3; i++) {
        ledStrip.setPixelColor(gridIndices[ledMap[pos.y][pos.x].y][ledMap[pos.y][pos.x].x], 
                              (i % 2 == 0) ? color : WS2812::RGB(0, 0, 0));
        ledStrip.show();
        sleep_ms(100);
    }
}

bool TicTacToeMic::checkWin(uint8_t player) {
    for (uint8_t i = 0; i < 3; i++) {
        if (board[i][0] == player && board[i][1] == player && board[i][2] == player) return true;
        if (board[0][i] == player && board[1][i] == player && board[2][i] == player) return true;
    }
    if (board[0][0] == player && board[1][1] == player && board[2][2] == player) return true;
    if (board[0][2] == player && board[1][1] == player && board[2][0] == player) return true;
    return false;
}

bool TicTacToeMic::isBoardFull() {
    for (uint8_t y = 0; y < 3; y++)
        for (uint8_t x = 0; x < 3; x++)
            if (board[y][x] == 0) return false;
    return true;
}
