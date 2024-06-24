# Tetris On Arduino

This project involves the implementation of a Tetris game controlled by a joystick. The game is displayed on an 8×8 LED matrix, while game details such as score, high score, and game over messages are shown on a 20×4 LCD display. A buzzer plays the Tetris theme song. Each time a row is completed in the Tetris game, the buzzer pauses the melody to emit a confirmation sound, after which the completed row is removed from the LED matrix.

![alt text](https://ocw.cs.pub.ro/courses/_media/pm/prj2024/fgul/andra_vlad_schematic_corect_2.png?cache=&w=900&h=415&tok=9a19fa)

Parts used:

- Arduino MEGA 2560
- MAX7219 LED Matrix Module - 8×8 LED matrix
- 2004 LCD with Blue Backlight and I2C Interface
- Biaxial Joystick Module
- Passive Buzzer

Demo: https://youtu.be/XZkV4incpBM
