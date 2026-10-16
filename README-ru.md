# xme0724 ADC linux console
## Терминал для Xilinx SoC xme072420

## Назначение
Терминальное приложение для тестирования базовой
функциональности SoC xme0724-20. \

На плате дополнительно установлено АЦП ADS125, 
которое также может управляться приложением.

Исходный код реализован для запуска как bare-metal-приложение,
так и как ARM-приложение. \
В первом случае код используется в Application в среде 
Vivado/Vitis (Xilinx), \
во втором случае сборка осуществяется
с использованием кросс-компиляции для получения ARM-приложения.

## bare-metal сборка (PS вариант)
- Запускаем Xilinx SDK(Vitas) 
- Добавляем проект Application (New Application Project), выбираем пустой проект (Empty)
- Добавляем исходники: Import -> File System -> Browse -> Select files -> Finish
- Для компиляции - либо соберется само, если включено "Make автоматически", \
  либо выбираем Make "вручную" из меню.
- Запускается как "Run As - 1" (Ctrl+F11) или "Debug As - 1" (F11).

Для работы приложения необходимо предварительно загрузить FPGA-прошивку - файл *.bit \
(см. меню Xilinx - Program FPGA) \

## сборка ARM-приложения
- Необходимо наличие установленногоь CMAKE и либо make.exe.
- Необходимо наличие установленного Toolchain для сборки ARM приложений. \
Например этот: [Arm GNU Toolchain Downloads](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads)

Компилятор может быть любой, например **arm-none-gnueabihf-gcc**.

Сборку выполняем либо из командной строки либо из любой среды, с которой умеем работать (например CLion от JetBrains, FAR тоже подойдет)\
Ну а далее кто как умеет...

# CMAKE сборка
Описание сборки в файле CMakeLists.txt

Основное внимание на CMAKE-переменные:
- set(CMAKE_SYSTEM_NAME Generic)
- set(CMAKE_C_COMPILER "arm-none-linux-gnueabihf-gcc.exe")

Данные переменные могут быть напрямую использованы в CMakeLists.txt, \
либо внесены в настройки среды разработки.

Подготовка сборки (пути к утилитам прописываем в окружении ОС, либо явно указываем в команде):
```
$ cd src
$ cmake -G "MinGW Makefiles" -DCMAKE_SYSTEM_NAME=Generic -DCMAKE_C_COMPILER=arm-none-linux-gnueabihf-gcc.exe -B debug
```
или
```
$ mkdir build
$ cd build
$ cmake -G "MinGW Makefiles" -DCMAKE_SYSTEM_NAME=Generic -DCMAKE_C_COMPILER=arm-none-linux-gnueabihf-gcc.exe
```

Сборка: `$ make -C build`

Очистка: `$ make -C build clear`

Запуск: копируем бинарник **adc-term** на xme0724 и делаем так `$ ./adc-term`

# MAKE сборка
[*Файл Makefile скоро будет опубликован.*]

Кратко: \
Делаем простейший Makefile с исходниками *.c *.h, \
добавляем переменные
```
- CC=<путь к>arm-none-linux-ueabihf.exe
- CFLAGS=-static
```
Запускаем: `$ make`



 