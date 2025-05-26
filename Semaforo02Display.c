#include "pico/stdlib.h"       // Biblioteca padrão do Raspberry Pi Pico
#include "hardware/i2c.h"      // Suporte para comunicação I2C
#include "hardware/gpio.h"     // Controle de pinos GPIO
#include "hardware/irq.h"      // Gerenciamento de interrupções
#include "lib/ssd1306.h"       // Biblioteca para display OLED SSD1306
#include "FreeRTOS.h"          // Sistema operacional em tempo real FreeRTOS
#include "task.h"              // Gerenciamento de tarefas do FreeRTOS
#include "semphr.h"            // Semáforos e mutexes do FreeRTOS
#include "pico/bootrom.h"      // Funções do bootrom do Pico
#include "stdio.h"             // Funções de entrada/saída padrão
#include "hardware/pwm.h"      // Modulação por largura de pulso (PWM) para buzzer
#include "hardware/clocks.h"   // Controle de clocks do sistema

// Definições de pinos e constantes
#define I2C_PORT i2c1          // Porta I2C usada (i2c1)
#define I2C_SDA 14             // Pino SDA para comunicação I2C
#define I2C_SCL 15             // Pino SCL para comunicação I2C
#define ENDERECO 0x3C          // Endereço I2C do display OLED

#define BOTAO_ENTRADA 5        // Pino do botão para entrada
#define BOTAO_SAIDA 6          // Pino do botão para saída
#define BOTAO_RESET 22         // Pino do botão para reset

#define LED_R 13               // Pino do LED vermelho
#define LED_G 11               // Pino do LED verde
#define LED_B 12               // Pino do LED azul
#define BUZZER 21              // Pino do buzzer

#define MAX_USUARIOS 8         // Máximo de usuários permitidos na sala

// Variáveis globais
ssd1306_t ssd;                        // Estrutura para o display OLED
SemaphoreHandle_t xSemContagem;        // Semáforo de contagem para limitar usuários
SemaphoreHandle_t xSemReset;           // Semáforo binário para reset via interrupção
SemaphoreHandle_t xMutexDisplay;       // Mutex para acesso seguro ao display
uint16_t usuariosAtivos = 0;           // Contador de usuários ativos na sala

// Função para tocar o buzzer com PWM
void play_buzzer(uint pin, uint frequency, uint duration_ms) {
    // Configura o pino para função PWM
    gpio_set_function(pin, GPIO_FUNC_PWM);
    // Obtém o número do slice PWM associado ao pino
    uint slice_num = pwm_gpio_to_slice_num(pin);
    // Carrega a configuração padrão do PWM
    pwm_config config = pwm_get_default_config();
    // Ajusta o divisor de clock para atingir a frequência desejada
    pwm_config_set_clkdiv(&config, clock_get_hz(clk_sys) / (frequency * 4096));
    // Inicializa o PWM com a configuração
    pwm_init(slice_num, &config, true);
    // Define o nível do PWM (~50% duty cycle)
    pwm_set_gpio_level(pin, 2048);
    // Aguarda a duração especificada
    vTaskDelay(pdMS_TO_TICKS(duration_ms));
    // Desliga o buzzer (nível 0)
    pwm_set_gpio_level(pin, 0);
    // Desativa o slice PWM
    pwm_set_enabled(slice_num, false);
}

// Função para atualizar o display OLED
void atualizarDisplay(const char *msg)
{
    // Tenta obter o mutex para acesso seguro ao display
    if (xSemaphoreTake(xMutexDisplay, portMAX_DELAY)) {
        // Limpa o display (preenche com preto)
        ssd1306_fill(&ssd, 0);
        // Exibe "LAB DE DIGITAIS" no topo (coluna 0, linha 0)
        ssd1306_draw_string(&ssd, "LAB DE DIGITAIS", 0, 0);
        // Desenha linha horizontal abaixo do título (de x=0 a x=127, y=15)
        ssd1306_hline(&ssd, 0, 127, 15, 1);
        // Exibe "Alunos" (coluna 0, linha 20)
        ssd1306_draw_string(&ssd, "Alunos", 0, 20);
        // Desenha linha vertical para separar "Alunos" do contador (x=50, y=20 a y=30)
        ssd1306_vline(&ssd, 50, 20, 30, 1);
        // Exibe o número de usuários (coluna 60, linha 20)
        char buffer[32];
        sprintf(buffer, "%d", usuariosAtivos);
        ssd1306_draw_string(&ssd, buffer, 60, 20);
        // Desenha linha horizontal abaixo da seção de alunos (de x=0 a x=127, y=35)
        ssd1306_hline(&ssd, 0, 127, 35, 1);
        // Exibe a mensagem recebida (coluna 0, linha 40)
        ssd1306_draw_string(&ssd, msg, 0, 40);
        // Envia os dados para o display
        ssd1306_send_data(&ssd);
        // Libera o mutex
        xSemaphoreGive(xMutexDisplay);
    }
}

// Função para atualizar o estado do LED RGB com base no número de usuários
void atualizarRGB()
{
    if (usuariosAtivos == 0) {
        // Sem usuários: LED azul aceso
        gpio_put(LED_R, 0);
        gpio_put(LED_G, 0);
        gpio_put(LED_B, 1);
    } else if (usuariosAtivos < MAX_USUARIOS - 1) {
        // Menos de 7 usuários: LED verde aceso
        gpio_put(LED_R, 0);
        gpio_put(LED_G, 1);
        gpio_put(LED_B, 0);
    } else if (usuariosAtivos == MAX_USUARIOS - 1) {
        // 7 usuários (quase cheio): LED amarelo (vermelho + verde)
        gpio_put(LED_R, 1);
        gpio_put(LED_G, 1);
        gpio_put(LED_B, 0);
    } else {
        // 8 ou mais usuários: LED vermelho aceso
        gpio_put(LED_R, 1);
        gpio_put(LED_G, 0);
        gpio_put(LED_B, 0);
    }
}

// Tarefa para gerenciar entradas de usuários
void vTaskEntrada(void *params)
{
    while (1) {
        // Verifica se o botão de entrada está pressionado (ativo em nível baixo)
        if (gpio_get(BOTAO_ENTRADA) == 0) {
            // Tenta obter o semáforo de contagem (disponível se < 8 usuários)
            if (xSemaphoreTake(xSemContagem, 0) == pdTRUE) {
                // Incrementa o contador de usuários
                usuariosAtivos++;
                // Atualiza o display com mensagem de entrada autorizada
                atualizarDisplay("Entrada autori-zada");
                // Atualiza o LED RGB
                atualizarRGB();
            } else {
                // Semáforo não disponível (sala cheia): exibe mensagem de negação
                atualizarDisplay("Acesso negado");
                // Toca o buzzer com tom de 2000 Hz por 200 ms
                play_buzzer(BUZZER, 2000, 200);
            }
            // Debounce: aguarda 300 ms para evitar múltiplas leituras
            vTaskDelay(pdMS_TO_TICKS(300));
        }
        // Pequeno atraso para reduzir uso da CPU
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// Tarefa para gerenciar saídas de usuários
void vTaskSaida(void *params)
{
    while (1) {
        // Verifica se o botão de saída está pressionado e há usuários na sala
        if (gpio_get(BOTAO_SAIDA) == 0 && usuariosAtivos > 0) {
            // Decrementa o contador de usuários
            usuariosAtivos--;
            // Libera uma unidade do semáforo de contagem
            xSemaphoreGive(xSemContagem);
            // Atualiza o display com mensagem de saída autorizada
            atualizarDisplay("Saida autoriza-da");
            // Atualiza o LED RGB
            atualizarRGB();
            // Debounce: aguarda 300 ms
            vTaskDelay(pdMS_TO_TICKS(300));
        }
        // Pequeno atraso para reduzir uso da CPU
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// Tarefa para gerenciar o reset do sistema
void vTaskReset(void *params)
{
    while (1) {
        // Aguarda o semáforo de reset (ativado por interrupção)
        if (xSemaphoreTake(xSemReset, portMAX_DELAY) == pdTRUE) {
            // Zera o contador de usuários e libera o semáforo de contagem
            while (usuariosAtivos > 0) {
                xSemaphoreGive(xSemContagem);
                usuariosAtivos--;
            }
            // Atualiza o display com mensagem de reset
            atualizarDisplay("Reset realizado");
            // Atualiza o LED RGB
            atualizarRGB();
            // Toca o buzzer com tom de 2000 Hz por 200 ms
            play_buzzer(BUZZER, 2000, 200);
            // Aguarda 300 ms antes de processar outro reset
            vTaskDelay(pdMS_TO_TICKS(300));
        }
    }
}

// Função de callback para interrupção do botão de reset
void gpio_callback(uint gpio, uint32_t events)
{
    if (gpio == BOTAO_RESET) {
        // Sinaliza o semáforo de reset a partir da interrupção
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        xSemaphoreGiveFromISR(xSemReset, &xHigherPriorityTaskWoken);
        // Força troca de contexto se necessário
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}

// Função para inicializar o hardware
void init_hardware()
{
    // Inicializa a interface de entrada/saída padrão (UART/USB)
    stdio_init_all();

    // Configura a interface I2C com 400 kHz
    i2c_init(I2C_PORT, 400 * 1000);
    // Define os pinos SDA e SCL como função I2C
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    // Ativa resistores de pull-up internos
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    // Inicializa o display OLED com resolução padrão, endereço I2C e porta
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, ENDERECO, I2C_PORT);
    // Configura parâmetros adicionais do display
    ssd1306_config(&ssd);
    // Envia dados iniciais para o display
    ssd1306_send_data(&ssd);

    // Configura o botão de entrada como entrada com pull-up
    gpio_init(BOTAO_ENTRADA);
    gpio_set_dir(BOTAO_ENTRADA, GPIO_IN);
    gpio_pull_up(BOTAO_ENTRADA);

    // Configura o botão de saída como entrada com pull-up
    gpio_init(BOTAO_SAIDA);
    gpio_set_dir(BOTAO_SAIDA, GPIO_IN);
    gpio_pull_up(BOTAO_SAIDA);

    // Configura o botão de reset como entrada com pull-up e interrupção
    gpio_init(BOTAO_RESET);
    gpio_set_dir(BOTAO_RESET, GPIO_IN);
    gpio_pull_up(BOTAO_RESET);
    // Habilita interrupção na borda de descida
    gpio_set_irq_enabled_with_callback(BOTAO_RESET, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);

    // Configura os pinos dos LEDs como saídas
    gpio_init(LED_R);
    gpio_init(LED_G);
    gpio_init(LED_B);
    gpio_set_dir(LED_R, GPIO_OUT);
    gpio_set_dir(LED_G, GPIO_OUT);
    gpio_set_dir(LED_B, GPIO_OUT);

    // Configura o pino do buzzer como saída
    gpio_init(BUZZER);
    gpio_set_dir(BUZZER, GPIO_OUT);
}

// Função principal
int main()
{
    // Inicializa o hardware
    init_hardware();

    // Cria semáforo de contagem com máximo de 8 usuários
    xSemContagem = xSemaphoreCreateCounting(MAX_USUARIOS, MAX_USUARIOS);
    // Cria semáforo binário para reset
    xSemReset = xSemaphoreCreateBinary();
    // Cria mutex para acesso ao display
    xMutexDisplay = xSemaphoreCreateMutex();

    // Cria tarefas do FreeRTOS
    xTaskCreate(vTaskEntrada, "Entrada", 256, NULL, 1, NULL); // Tarefa para entrada
    xTaskCreate(vTaskSaida, "Saida", 256, NULL, 1, NULL);     // Tarefa para saída
    xTaskCreate(vTaskReset, "Reset", 256, NULL, 2, NULL);     // Tarefa para reset (maior prioridade)

    // Inicia o escalonador do FreeRTOS
    vTaskStartScheduler();
}
