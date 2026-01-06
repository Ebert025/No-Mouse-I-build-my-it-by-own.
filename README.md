### WARNING / AVISO
EN:

## I'm incredibly lazy, in fact, I wrote the entire code without AI (believe it or not, although almost everyone does it nowadays) or anything, but I'm terrible at explaining things and I don't want to spend hours writing documentation, so I used GitHub's Copilot to do it for me.

PT-BR:

## Eu sou preguiçoso pra caralho, de fato, fiz o código inteiro sem IA(acredite em mim ou não, apesar que quase todo mundo faz hoje em dia) nem nada, mas sou péssimo em explicar as coisas e não quero ficar passando horas escrevendo docs, então usei o Copilot do github pra fazer isso por mim.


### _(A seguir, palavras da IA / Next, words from AI.)_



# No-Mouse — Control your mouse with the Numpad / Controle o mouse com o Numpad

EN — A small personal utility that turns your numeric keypad into a mouse driver. Use the Numpad to move the pointer, click, and scroll. This is a personal project; feel free to modify the code to add new keys, change bindings, or adapt it to your needs.

PT-BR — Pequena utilidade pessoal que transforma o teclado numérico (Numpad) em um driver de mouse. Use o Numpad para mover o cursor, clicar e rolar. É um projeto pessoal — fique à vontade para alterar o código, adicionar novas teclas/binds ou adaptar como quiser.

---

## Features / Funcionalidades
- Control pointer movement using Numpad keys (relative movement).
- Left and right mouse clicks from Numpad.
- Scrolling (vertical/horizontal) while holding the "Special" key.
- Adjustable sensitivity (pixels per step), delay (microseconds), and wheel step size.
- Option to "consume" Numpad input (grab keys via X11) so numeric characters are not sent to other apps while the utility runs.
- Exits with Numpad Enter or Ctrl+C.

---

## Requirements / Requisitos
- Linux (X11)
- Root privileges to create a virtual input device (/dev/uinput)
- Libraries / packages:
  - libinput (development headers)
  - libudev (development headers)
  - libX11 (development headers)
  - uinput support in the kernel (module `uinput` / device `/dev/uinput`)

Example package names (Debian/Ubuntu-like):
- libinput-dev, libudev-dev, libx11-dev, build-essential, and ensure uinput is available (modprobe uinput)

---

## Build / Compilar
EN:
Compile with gcc (example):
```
gcc main.c -o app -linput -lX11 -ludev
```

PT-BR:
Compilar com gcc (exemplo):
```
gcc main.c -o app -linput -lX11 -ludev
```

Notes / Observações:
- The program requires root to open /dev/uinput. Run with sudo.
- You may add debugging flags or address sanitizer during development, for example:
```
gcc main.c -o app -g -fsanitize=address -linput -lX11 -ludev
```

---

## Run / Executar
EN:
Run as root:
```
sudo ./app
```

PT-BR:
Execute como root:
```
sudo ./app
```

On start the program prints controls and current defaults and asks if you want to change them.

---

## Default controls / Controles padrão (Numpad-only)
EN:
- Numpad 8: Move Up
- Numpad 2: Move Down
- Numpad 4: Move Left
- Numpad 6: Move Right
- Numpad 7: Left Click
- Numpad 9: Right Click
- Numpad 5: Special (hold to enable scroll mode / config shortcuts)
- Numpad Enter: Exit
- Numpad +: Increase wheel step
- Numpad -: Decrease wheel step
- When Special is held:
  - Numpad 8: Scroll Up
  - Numpad 2: Scroll Down
  - Numpad 4: Scroll Left
  - Numpad 6: Scroll Right
  - Numpad 1: Increase delay (microseconds)
  - Numpad 3: Decrease delay (microseconds)
  - NumLock + Special: Toggle Numpad consume (enable/disable grabbing keys so other apps don't receive them)

Also:
- Numpad 1 (when not in Special): Increase sensitivity (pixels per step)
- Numpad 3 (when not in Special): Decrease sensitivity (if > 1)

PT-BR:
- Numpad 8: Cima
- Numpad 2: Baixo
- Numpad 4: Esquerda
- Numpad 6: Direita
- Numpad 7: Click esquerdo
- Numpad 9: Click direito
- Numpad 5: Especial (segure para ativar modo scroll / atalhos de configuração)
- Numpad Enter: Sair
- Numpad +: Aumentar passo da roda
- Numpad -: Diminuir passo da roda
- Com Especial pressionado:
  - Numpad 8: Scroll para cima
  - Numpad 2: Scroll para baixo
  - Numpad 4: Scroll para esquerda
  - Numpad 6: Scroll para direita
  - Numpad 1: Aumentar delay (microsegundos)
  - Numpad 3: Diminuir delay (microsegundos)
  - NumLock + Especial: Alterna consumo do Numpad (pegar as teclas via X11 para que outros apps não as recebam)

Além disso:
- Numpad 1 (sem Especial): Aumentar sensibilidade (pixels por passo)
- Numpad 3 (sem Especial): Diminuir sensibilidade (se > 1)

---

## Default configuration values / Valores padrões
- Sensitivity (pixels per move): 1
- Delay between steps: 5000 microseconds (5 ms)
- Wheel step: 4
- Numpad consume (grab keys): enabled by default

You can change these on startup when prompted or modify the values in the code (config array).

---

## How to customize / Como personalizar
- Edit `main.c` to change key bindings, add new behaviors, or adjust defaults.
- Key handling is implemented in function `keyboardinput()` — change KEY_KP_* constants and actions there.
- Default values are in the `configs` array in `main()`. You can also expose more runtime options if desired.

---

## Safety & Notes / Avisos
- The app requires root and creates a virtual input device. Use at your own risk.
- Grabbing Numpad keys may interfere with other programs that expect numeric input.
- Tested on X11 environments. Behavior on Wayland is not supported by this code.
- Make sure /dev/uinput exists and you have permissions (or run as root). To load kernel module:
```
sudo modprobe uinput
```

---

## License / Licença
This project is released under the MIT License. See [LICENSE](LICENSE).

EN — Personal project. Contributions, fixes and improvements are welcome — modify the code as you need.

PT-BR — Projeto pessoal. Contribuições, correções e melhorias são bem-vindas — modifique o código conforme precisar.
