#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "WS2812.hpp"
#include "TicTacToeMic.hpp"

// Protótipos das funções do modo joystick
void initHardware();
void drawBoard(WS2812 &ledStrip);
void makeAIMove();
void checkGameState(WS2812 &ledStrip);
void processInput(WS2812 &ledStrip);

// Constantes
#define LED_PIN 7
#define LED_LENGTH 25
#define BUTTON_B_PIN 5 // Botão B físico do BitDogLab

int main()
{
    stdio_init_all();

    // Inicializa botão B
    gpio_init(BUTTON_B_PIN);
    gpio_set_dir(BUTTON_B_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_B_PIN); // botão ativo em LOW

    sleep_ms(100); // estabiliza leitura após reset

    // Lê o estado do botão B no momento do boot
    bool pressionado = !gpio_get(BUTTON_B_PIN); // LOW = pressionado

    if (!pressionado)
    {
        // Modo padrão = MIC
        printf(">> Botão B NÃO pressionado no reset: iniciando modo MICROFONE\n");
        TicTacToeMic game;
        game.run();
    }
    else
    {
        // Só entra no modo joystick se o botão B estiver pressionado
        printf(">> Botão B pressionado no reset: iniciando modo JOYSTICK\n");

        initHardware();
        WS2812 ledStrip(LED_PIN, LED_LENGTH, pio0, 0, WS2812::FORMAT_GRB);
        drawBoard(ledStrip);

        while (true)
        {
            extern bool gameActive;
            extern uint8_t currentPlayer;

            if (gameActive)
            {
                if (currentPlayer == 1)
                {
                    sleep_ms(500);
                    makeAIMove();
                    drawBoard(ledStrip);
                    checkGameState(ledStrip);
                }
                else
                {
                    processInput(ledStrip);
                }
            }
            else
            {
                processInput(ledStrip); // permite reinício
            }

            sleep_ms(10);
        }
    }
}
