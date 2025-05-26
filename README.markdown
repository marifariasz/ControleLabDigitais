# Sistema de Controle de Acesso com FreeRTOS 🚪✨

Bem-vindo ao **Sistema de Controle de Acesso** do *LAB DE DIGITAIS*! 🎉 Este projeto, desenvolvido para o Raspberry Pi Pico com FreeRTOS, gerencia o acesso de até 8 usuários em uma sala, usando botões para entrada e saída, um display OLED para feedback visual, um LED RGB para indicar o status e um buzzer para alertas sonoros. Tudo isso com semáforos para controle de concorrência e um mutex para acesso seguro ao display! 🖥️🔒


## 🌟 Funcionalidades
- **Controle de Usuários**: Limita a sala a 8 usuários simultâneos com um semáforo de contagem. 📊
- **Display OLED**: Exibe:
  - "LAB DE DIGITAIS" no topo. 🏷️
  - "Alunos" e o número de usuários na sala, separados por uma linha vertical. 📈
  - Mensagens de status ("Entrada autorizada", "Saída autorizada", "Acesso negado" ou "Reset realizado") na parte inferior. 💬
- **Linhas Visuais**: Linhas horizontais e verticais no display OLED organizam as informações. 📏
- **Feedback por LED RGB**:
  - 🔵 **Azul**: 0 usuários (sala vazia).
  - 🟢 **Verde**: 1 a 6 usuários.
  - 🟡 **Amarelo**: 7 usuários (quase cheio).
  - 🔴 **Vermelho**: 8 ou mais usuários (sala cheia).
- **Buzzer**: Toca um som de 2000 Hz por 200 ms em caso de acesso negado (beep simples) ou reset (beep duplo). 🎵
- **Reset**: Botão de reset zera a contagem de usuários via interrupção. 🔄
- **Thread Safety**: Mutex garante acesso seguro ao display OLED. 🔐

## 🛠️ Requisitos de Hardware
- **Raspberry Pi Pico**: Microcontrolador principal. 🖥️
- **Display OLED SSD1306**: Resolução 128x64, conectado via I2C (SDA: GPIO 14, SCL: GPIO 15, endereço: 0x3C). 📟
- **Botões**:
  - Entrada: GPIO 5 (pull-up, ativo em nível baixo). 🚪
  - Saída: GPIO 6 (pull-up, ativo em nível baixo). 🚶
  - Reset: GPIO 22 (pull-up, ativo em nível baixo, com interrupção). 🔄
- **LED RGB**:
  - Vermelho: GPIO 13. 🔴
  - Verde: GPIO 11. 🟢
  - Azul: GPIO 12. 🔵
- **Buzzer**: GPIO 21, controlado por PWM para gerar tons. 🎶
- **Resistores**: Pull-ups externos podem ser necessários, dependendo do circuito. ⚡

## 📚 Dependências de Software
- **Pico SDK**: Para controle de hardware do Pico (I2C, GPIO, PWM). 🛠️
- **FreeRTOS**: Gerenciamento de tarefas, semáforos e mutexes. ⚙️
- **Biblioteca SSD1306**: Controle do display OLED (funções como `ssd1306_draw_string`, `ssd1306_hline`, `ssd1306_vline`). 🖼️
- **Bibliotecas C**: Para formatação de strings (`stdio.h`). 📝

## 🔌 Configuração de Pinos
| Componente       | Pino GPIO | Função                     |
|------------------|-----------|----------------------------|
| I2C SDA          | 14        | Linha de dados I2C         |
| I2C SCL          | 15        | Linha de clock I2C         |
| Botão Entrada    | 5         | Entrada (pull-up)          |
| Botão Saída      | 6         | Entrada (pull-up)          |
| Botão Reset      | 22        | Entrada (pull-up, interrupção) |
| LED Vermelho     | 13        | Saída                      |
| LED Verde        | 11        | Saída                      |
| LED Azul         | 12        | Saída                      |
| Buzzer           | 21        | Saída PWM                  |

## ⚙️ Como Configurar
1. **Montagem do Hardware**:
   - Conecte o display OLED SSD1306 ao I2C1 (SDA: GPIO 14, SCL: GPIO 15). 🖥️
   - Ligue os botões de entrada (GPIO 5), saída (GPIO 6) e reset (GPIO 22) com resistores pull-up (ou use os internos do Pico). 🚪🔄
   - Conecte o LED RGB aos GPIOs 13 (vermelho), 11 (verde) e 12 (azul). 💡
   - Conecte o buzzer ao GPIO 21 para saída PWM. 🎶
   - Verifique as conexões e alimente o circuito. ⚡

2. **Configuração do Software**:
   - Instale o **Pico SDK** e configure o ambiente de desenvolvimento. 📚
   - Adicione a biblioteca **FreeRTOS** ao projeto. ⚙️
   - Inclua a biblioteca **SSD1306** para controle do display. 🖼️
   - Compile e grave o código no Raspberry Pi Pico usando uma ferramenta como CMake e um programador (ex.: Picoprobe ou modo USB). 🚀

3. **Estrutura do Código**:
   - O arquivo principal (`main.c`) contém:
     - Inicialização do hardware (`init_hardware`). 🛠️
     - Tarefas FreeRTOS para entrada (`vTaskEntrada`), saída (`vTaskSaida`) e reset (`vTaskReset`). ⚙️
     - Função para atualizar o display (`atualizarDisplay`) com layout organizado. 🖥️
     - Controle do LED RGB (`atualizarRGB`) e buzzer (`play_buzzer`). 💡🎶
     - Sem ව

## 🎮 Como Usar
- **Entrada**: Pressione o botão conectado ao GPIO 5 para registrar a entrada de um usuário. 🚪
  - O display mostra "Entrada autorizada" e o número de usuários é atualizado.
  - O LED RGB muda de cor conforme o número de usuários.
  - Se a sala estiver cheia (8 usuários), o display mostra "Acesso negado" e o buzzer toca.
- **Saída**: Pressione o botão conectado ao GPIO 6 para registrar a saída de um usuário. 🚶
  - O display mostra "Saída autorizada" e atualiza o contador.
- **Reset**: Pressione o botão conectado ao GPIO 22 para zerar o contador de usuários. 🔄
  - O display mostra "Reset realizado" e o buzzer toca.
- **Feedback Visual**:
  - O display OLED exibe o nome do laboratório, o número de alunos e mensagens de status. 📟
  - O LED RGB indica o status da sala (azul, verde, amarelo ou vermelho). 💡
- **Feedback Sonoro**: O buzzer emite um som de 2000 Hz por 200 ms em caso de acesso negado (beep simples) ou reset (beep duplo). 🎵