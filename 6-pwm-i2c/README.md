# Controle de PWM para LED e display LCD I2C

## Objetivo:
Ampliar o projeto das atividades anteriores para introduzir o uso de saídas PWM e comunicação serial (UART, I2C ou SPI), utilizando o ESP32S3 e o framework ESP-IDF, no ambiente de simulação Wokwi. Os conceitos abordados incluem: geração de sinal PWM, controle de brilho de LED, e transmissão de dados via comunicação serial.

## Material Necessário:
- ESP32S3
- 4 LEDs (para exibição do contador binário)
- 2 botões (push buttons)
- 1 LED adicional (para controle de brilho via PWM)
- 1 Display LCD com interface I2C
- Conta no Wokwi (https://wokwi.com/)

## Passos para a atividade:
###  1. Atualize o diagrama da Atividade 04/05 incluindo:
    - 1 LED;
    - 1 Display LCD

###  2. Atualize o esquemático com os novos módulos e componentes.

###  3.  Desenvolvimento do Código.
Desenvolva um programa utilizando o ESP-IDF para implementar um contador binário de 4 bits. O valor atual do contador deve ser exibido utilizando 4 LEDs. Além disso, o sistema deve utilizar 2 botões conectados a entradas digitais, 1 LED adicional com controle de brilho e um display LCD.

#### Parte A - Funcionalidade dos botões:

Botão A: a cada acionamento, deve incrementar o valor do contador conforme a unidade de incremento atual (padrão: +1).
Botão B: a cada acionamento, deve decrementar o valor do contador conforme a unidade de incremento atual (padrão: -1).
Parte A – PWM: Controle de Brilho do LED

Use o driver (PWM) do ESP-IDF.
O brilho do LED PWM deve ser proporcional ao valor do contador de 4 bits:
- 0x0 → brilho mínimo
- 0xF → brilho máximo

#### Parte B – LCD I2C: Exibir o Valor do Contador

Configure o barramento I2C e inicialize o display LCD.
Mostre na primeira linha o valor do contador em formato hexadecimal.
Mostre na primeira linha o valor do contador em formato decimal. (Nota: Parece haver uma repetição aqui, provavelmente uma linha deveria ser para o formato decimal e a outra para o hexadecimal, ou uma delas está na segunda linha do LCD)
O display deve ser atualizado sempre que o valor do contador for alterado.
Obs:

O contador deve ser circular (isto é, ao ultrapassar o valor máximo de 4 bits, ele retorna ao início com base no passo atual).
O contador é de 4 bits, portanto seu valor varia entre 0x0 (0 decimal ou b0000) e 0xF (15 decimal ou b1111).
A unidade de incremento inicial é 1 unidade.
O valor final do contador deve sempre estar dentro do intervalo de 4 bits (0x0 a 0xF) após cada incremento.

##### Obs:

O debounce deve ser tratado por software (Não usar daley).
É obrigatório o uso de interrupções para a leitura do estado dos botões.
