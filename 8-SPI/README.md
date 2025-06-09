# QAD com SDCard


## Objetivo:
A partir do projeto da Atividade 7, implemente o uso do SDCard. Os conceitos abordados incluem: utilização da interface de comunicação SPI.

## Material Necessário:
- ESP32S3
- 4 LEDs (para exibição do contador binário)
- 2 botões (push buttons)
- 1 Buzzer (Controle por PWM)
- 1 Display LCD com interface I2C
- 1 NTC (Sensor de temperatura analógico)
- Micro SD Card
- Conta no Wokwi (https://wokwi.com/)

## Passos para a atividade:
###  1. Diagrama de bloco atualizado.

###  2. Esquemático atualizado com os novos componentes.

###  3.  Desenvolvimento do Código.
Desenvolva um programa utilizando o ESP-IDF para salvar a leitura da temperatura adquirida por meio do NTC em um SDCard.

#### Parte A - Funcionalidade dos botões:

Botão A: a cada acionamento, deve incrementar a temperatura de alarme (padrão: +5).
Botão B: a cada acionamento, deve decrementar a temperatura de alarme (padrão: -5).

#### Parte B – PWM: Gerar o alarme sonoro

- Use o driver (PWM) do ESP-IDF.
- O alerta sonoro deve ser acionado quando a temperatura registrada pelo NTC for maior que a temperatura de alarme.
- O alerta sonoro só deve ser desligado quando a temperatura do NTC estiver abaixo da temperatura de alarme.

#### Parte C – LCD I2C: Exibir a temperatura registrada no NTC e a temperatura de alarme

- Configure o barramento I2C e inicialize o display LCD.
- Mostre na primeira linha o valor da temperatura no NTC.
- Mostre na segunda linha o valor da temperatura de alarme atual.
- O display deve ser atualizado sempre que os valores das temperaturas forem alteradas.

#### Parte D – LEDs: Sinalizar a aproximação da temperatura do NTC a temperatura de alarme

- Ligar 1 LED quando a temperatura do NTC estiver a 20 °C da temperatura de alarme.
- Ligar 2 LED quando a temperatura do NTC estiver a 15 °C da temperatura de alarme.
- Ligar 3 LED quando a temperatura do NTC estiver a 10 °C da temperatura de alarme.
- Ligar 4 LED quando a temperatura do NTC estiver a 2 °C da temperatura de alarme.
- Piscar os 4 LEDs quando a temperatura do NTC for maior ou igual a temperatura de alarme.
- Os LEDs devem continuar piscando enquanto a temperatura do NTC for maior que a temperatura de alarme.

#### Parte E - SDCard:

- Salvar todas as leituras realizadas pelo ADC da temperatura do NTC no SDCard.

##### Obs:

O debounce deve ser tratado por software (Não usar daley).
É obrigatório o uso de interrupções para a leitura do estado dos botões.