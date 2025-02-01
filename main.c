#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812b.pio.h"

// Tamanho da matriz de LEDs
#define MATRIZ_ROWS 5
#define MATRIZ_COLS 5
#define MATRIZ_SIZE MATRIZ_ROWS * MATRIZ_COLS

// Estrutura para armazenar um pixel com valores RGB
typedef struct Rgb
{
  uint r;
  uint g;
  uint b;
} Rgb;

// Buffer para matriz de LEDs
Rgb matriz[MATRIZ_SIZE];

// Frames dos números 0 a 9 em caracteres digitais
const uint nums[10][MATRIZ_ROWS][MATRIZ_COLS] = 
{
  {
    {0, 1, 1, 1, 0},
    {0, 1, 0, 1, 0},
    {0, 1, 0, 1, 0},
    {0, 1, 0, 1, 0},
    {0, 1, 1, 1, 0},
  },
   {
    {0, 0, 1, 0, 0},
    {0, 1, 1, 0, 0},
    {0, 0, 1, 0, 0}, // Algarismo 1
    {0, 0, 1, 0, 0},
    {0, 0, 1, 0, 0}
  },
  {
    {0, 1, 1, 1, 0},
    {0, 0, 0, 1, 0},
    {0, 1, 1, 1, 0}, // Algarismo 2
    {0, 1, 0, 0, 0},
    {0, 1, 1, 1, 0}
  },
  {
    {0, 1, 1, 1, 0},
    {0, 0, 0, 1, 0},
    {0, 1, 1, 1, 0}, // Algarismo 3
    {0, 0, 0, 1, 0},
    {0, 1, 1, 1, 0}
  },
  {
    {0, 1, 0, 1, 0},
    {0, 1, 0, 1, 0},
    {0, 1, 1, 1, 0}, // Algarismo 4
    {0, 0, 0, 1, 0},
    {0, 0, 0, 1, 0}
  },
  {
    {0, 1, 1, 1, 0},
    {0, 1, 0, 0, 0},
    {0, 1, 1, 1, 0}, // Algarismo 5
    {0, 0, 0, 1, 0},
    {0, 1, 1, 1, 0}
  },
  {
    {0, 1, 1, 1, 0},
    {0, 1, 0, 0, 0},
    {0, 1, 1, 1, 0}, // Algarismo 6
    {0, 1, 0, 1, 0},
    {0, 1, 1, 1, 0}
  },
  {
    {0, 1, 1, 1, 0},
    {0, 0, 0, 1, 0},
    {0, 0, 0, 1, 0}, // Algarismo 7
    {0, 0, 0, 1, 0},
    {0, 0, 0, 1, 0}
  },
  {
    {0, 1, 1, 1, 0},
    {0, 1, 0, 1, 0},
    {0, 1, 1, 1, 0}, // Algarismo 8
    {0, 1, 0, 1, 0},
    {0, 1, 1, 1, 0}
  },
  {
    {0, 1, 1, 1, 0},
    {0, 1, 0, 1, 0},
    {0, 1, 1, 1, 0}, // Algarismo 9
    {0, 0, 0, 1, 0},
    {0, 0, 0, 1, 0}
  }
};


// Pinos
const uint led_pin_green = 11;
const uint matriz_pin = 7;
const uint btn_a = 5;
const uint btn_b = 6;

// Variáveis para configuração do PIO
PIO np_pio;
uint sm;

// Headers de funções
void inicializar_matriz();

int main() 
{
  // Inicialização das variáveis e pinos
  gpio_init(led_pin_green);
  gpio_set_dir(led_pin_green, GPIO_OUT);
  gpio_put(led_pin_green, 0);

  gpio_init(btn_a);
  gpio_set_dir(btn_a, GPIO_IN);
  gpio_pull_up(btn_a);

  gpio_init(btn_b);
  gpio_set_dir(btn_b, GPIO_IN);
  gpio_pull_up(btn_b);

  inicializar_matriz();

  stdio_init_all();

  while (true) {
    sleep_ms(1000);
  }
}

void inicializar_matriz()
{
  // Cria PIO
  uint offset = pio_add_program(pio0, &ws2812b_program);
  np_pio = pio0;

  // Pede uma máquina de estado do PIO
  sm = pio_claim_unused_sm(np_pio, false);
  if (sm < 0) 
  {
    np_pio = pio1;
    sm = pio_claim_unused_sm(np_pio, true);
  }

  // Roda programa na máquina de estado
  ws2812b_program_init(np_pio, sm, offset, matriz_pin, 800000.f);

  // Limpa buffer da matriz
  for (uint i = 0; i < MATRIZ_SIZE; i++) 
  {
    matriz[i].r = 0;
    matriz[i].g = 0;
    matriz[i].b = 0;
  }
}

