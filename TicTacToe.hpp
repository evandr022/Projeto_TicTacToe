#ifndef TIC_TAC_TOE_HPP
#define TIC_TAC_TOE_HPP

#include <stdint.h>
#include "WS2812.hpp"

// Estrutura para representar uma posição no tabuleiro
typedef struct {
    uint8_t x;
    uint8_t y;
} Position;

// Funções do jogo
void initHardware();
void drawBoard(WS2812& ledStrip);
void processInput(WS2812& ledStrip);
void makeAIMove();
void checkGameState(WS2812& ledStrip);
void resetGame(WS2812& ledStrip);
void showWinAnimation(WS2812& ledStrip, uint8_t player);
void showDrawAnimation(WS2812& ledStrip);
void flashPosition(WS2812& ledStrip, Position pos, uint32_t color);
int evaluateBoard();
bool checkWin(uint8_t player);
bool isBoardFull();

#endif // TIC_TAC_TOE_HPP
