#ifndef TIC_TAC_TOE_MIC_HPP
#define TIC_TAC_TOE_MIC_HPP

#include <stdint.h>
#include "WS2812.hpp"
#include <vector>

// Estrutura para posição
typedef struct {
    uint8_t x;
    uint8_t y;
} Position;

class TicTacToeMic {
public:
    TicTacToeMic();
    void run();

private:
    // Objeto da faixa de LED
    WS2812 ledStrip;

    // Estado do jogo
    uint8_t board[3][3];
    uint8_t currentPlayer;
    Position cursor;
    bool gameActive;

    // Controle por palmas
    std::vector<uint32_t> batidas;
    bool acima_threshold;

    // Métodos
    void initHardware();
    void drawBoard();
    void processClaps();
    void moveCursor();
    void makeMove();
    void makeAIMove();
    void checkGameState();
    void resetGame();
    void showWinAnimation(uint8_t player);
    void showDrawAnimation();
    void flashPosition(Position pos, uint32_t color);
    bool checkWin(uint8_t player);
    bool isBoardFull();
};

#endif // TIC_TAC_TOE_MIC_HPP
