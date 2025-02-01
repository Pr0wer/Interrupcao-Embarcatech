#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "ws2812b.pio.h"

// Tamanho da matriz de LEDs
#define MATRIZ_ROWS 5
#define MATRIZ_COLS 5
#define MATRIZ_SIZE MATRIZ_ROWS * MATRIZ_COLS

// Pinos
const uint led_pin_green = 11;
const uint matriz_pin = 7;
const uint btn_a = 5;
const uint btn_b = 6;

// Estrutura para armazenar um pixel com valores RGB
typedef struct Rgb
{
  uint8_t r;
  uint8_t g;
  uint8_t b;
} Rgb;

// Buffer para matriz de LEDs
Rgb matriz[MATRIZ_SIZE];

// Cor do digito na matriz (em RGB)
const uint8_t digitoRed = 5;
const uint8_t digitoGreen = 0;
const uint8_t digitoBlue = 0;
Rgb corDigito;

// Frames dos números 0 a 9 em caracteres digitais
const uint8_t nums[10][MATRIZ_ROWS][MATRIZ_COLS] = 
{
  {
    {0, 1, 1, 1, 0},
    {0, 1, 0, 1, 0},
    {0, 1, 0, 1, 0}, // Algarismo 0
    {0, 1, 0, 1, 0},
    {0, 1, 1, 1, 0},
  },
   {
    {0, 0, 1, 0, 0},
    {0, 1, 1, 0, 0},
    {0, 0, 1, 0, 0}, // Algarismo 1
    {0, 0, 1, 0, 0},
    {0, 1, 1, 1, 0}
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
    {0, 1, 1, 1, 0}
  }
};

// Buffers do digito atual e anterior da matriz de LED
static volatile uint8_t digitoAtual = 0;
static volatile uint8_t digitoAnterior = 0;

// Buffer da última vez que um botão foi apertado
static volatile uint tempoAnterior = 0;

// Variáveis para configuração do PIO
PIO np_pio;
uint sm;

// Headers de funções
void inicializarMatriz();
void gpio_irq_handler(uint gpio, uint32_t events);
void atualizarDigito(uint8_t digito);
void atualizarMatriz();
void alterarLed(int i, uint8_t red, uint8_t green, uint8_t blue);
uint obterPosicao(uint linha, uint coluna);

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

  inicializarMatriz();

  // Inicializar buffer RGB da cor emitida pelo LED da matriz
  corDigito.r = digitoRed;
  corDigito.g = digitoGreen;
  corDigito.b = digitoBlue;

  // Emitir digito inicial na matriz de LED (0)
  atualizarDigito(digitoAtual);
  atualizarMatriz();

  stdio_init_all();

  // Definir interrupção para os botões A e B
  gpio_set_irq_enabled_with_callback(btn_a, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
  gpio_set_irq_enabled_with_callback(btn_b, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

  while (true) {
    // Piscar LED RGB 5 vezes por segundo
    gpio_put(led_pin_green, 1);
    sleep_ms(100);
    gpio_put(led_pin_green, 0);
    sleep_ms(100);

    // Atualizar a matriz de LED caso o digito tenha se alterado
    if (digitoAtual != digitoAnterior)
    { 
      atualizarDigito(digitoAtual);
      atualizarMatriz();
    }
  }
}

// Inicializa a matriz de LED WS2812B. Baseado no exemplo neopixel_pio do repo BitDogLab
void inicializarMatriz()
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
    alterarLed(i, 0, 0, 0);
  }
}

void gpio_irq_handler(uint gpio, uint32_t events)
{ 
  // Tratar efeito bounce
  uint tempoAtual = to_us_since_boot(get_absolute_time());
  if (tempoAtual - tempoAnterior > 200000)
  {
      // Atualizar valores anteriores para o atual antes de alterá-los
    digitoAnterior = digitoAtual;
    tempoAnterior = tempoAtual;

    // Verifica qual dos botões foi pressionado e atualiza digito
    if (gpio == btn_a && digitoAtual < 9)
    {
      digitoAtual++;
    }
    else if (gpio == btn_b && digitoAtual > 0)
    {
      digitoAtual--;
    }
  }
}

// Adiciona um determinado digito ao buffer da matriz de LEDs
void atualizarDigito(uint8_t digito)
{
  // Obter frame do digito desejado
  const uint8_t (*frame)[5] = nums[digito];

  // Iterar por cada pixel
  for (int linha = 0; linha < MATRIZ_ROWS; linha++)
  {
    for (int coluna = 0; coluna < MATRIZ_COLS; coluna++)
    { 
      // Obter indíce correto de acordo com o modo de envio de informação do WS2812B e adicionar ao buffer
      uint index = obterPosicao(linha, coluna);

      // Liga o LED na cor definida caso o valor lógico desse pixel seja 1 (LED aceso)
      if (frame[linha][coluna])
      {
        alterarLed(index, corDigito.r, corDigito.g, corDigito.b);
      }
      else
      {
        alterarLed(index, 0, 0, 0);
      }
    }
  }
}

// Atualiza a matriz de LEDs com o conteúdo do buffer
void atualizarMatriz()
{
  for (int i = 0; i < MATRIZ_SIZE; i++)
  {
    pio_sm_put_blocking(np_pio, sm, matriz[i].g);
    pio_sm_put_blocking(np_pio, sm, matriz[i].r);
    pio_sm_put_blocking(np_pio, sm, matriz[i].b);

  }
  sleep_us(100); // Espera 100us para a próxima mudança, de acordo com o datasheet
}

// Altera a cor de um LED no buffer da matriz
void alterarLed(int i, uint8_t red, uint8_t green, uint8_t blue)
{
  matriz[i].r = red;
  matriz[i].g = green;
  matriz[i].b = blue;
}

// Converte a posição de um array bidimensional para a posição correspondente na matriz de LED
uint obterPosicao(uint linha, uint coluna)
{ 
  // Inverter linha
  uint linhaMatriz = MATRIZ_ROWS - 1 - linha;

  uint colunaMatriz;
  if (linha % 2 == 0)
  {
    // Se a linha for par, a informação é enviada da direita para a esquerda (inversão da coluna)
    colunaMatriz = MATRIZ_COLS - 1 - coluna;
  }
  else
  {
    // Senão, é enviada da esquerda para a direita (coluna é a mesma)
    colunaMatriz = coluna;
  }

  // Converter novo indíce de array bidimensional para unidimensional e retornar valor
  return MATRIZ_ROWS * linhaMatriz + colunaMatriz;
}