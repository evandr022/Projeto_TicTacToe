#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "WS2812.hpp"
#include "TicTacToeMic.hpp"
// #include "TicTacToe.cpp"

// Protótipos das funções do modo joystick
void initHardware();
void drawBoard(WS2812& ledStrip);
void makeAIMove();
void checkGameState(WS2812& ledStrip);
void processInput(WS2812& ledStrip);

// Constantes
#define LED_PIN 7
#define LED_LENGTH 25
#define BUTTON_B_PIN 10

int main() {
    stdio_init_all();

    // Inicializa botão B
    gpio_init(BUTTON_B_PIN);
    gpio_set_dir(BUTTON_B_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_B_PIN);  // botão ativo em LOW

    // Aguarda 1 segundo para estabilizar o sistema
    sleep_ms(1000);

    // Verifica se o botão B está pressionado na inicialização
    if (!gpio_get(BUTTON_B_PIN)) {
        printf(">> Modo MICROFONE selecionado!\n");
        TicTacToeMic game;
        game.run();
    } else {
        printf(">> Modo JOYSTICK selecionado!\n");

        // Modo padrão: joystick
        initHardware();
        WS2812 ledStrip(LED_PIN, LED_LENGTH, pio0, 0, WS2812::FORMAT_GRB);
        drawBoard(ledStrip);

        while (true) {
            extern bool gameActive;
            extern uint8_t currentPlayer;

            if (gameActive) {
                if (currentPlayer == 1) {
                    sleep_ms(500);
                    makeAIMove();
                    drawBoard(ledStrip);
                    checkGameState(ledStrip);
                } else {
                    processInput(ledStrip);
                }
            } else {
                processInput(ledStrip); // permite reinício
            }

            sleep_ms(10);
        }
    }

    return 0;
}
