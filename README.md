# Quantum Engine

Браузерный движок нового поколения, построенный на нативных технологиях.

## Обзор

Quantum Engine — лёгкий графический браузер, разработанный на C++ с использованием веб-движка WebView2. Проект предоставляет минимальную, но функциональную альтернативу тяжёлым браузерным решениям.

## Возможности

- **Вкладки** — открытие, закрытие и переключение между страницами
- **Адресная строка** — навигация по URL с автоформатированием
- **Кнопки навигации** — Назад, Вперёд, Обновить, Домой
- **Плавающая панель** — элементы управления на внешних страницах
- **Строка состояния** — отображение текущего URL

## Технологический стек

| Компонент | Технология |
|-----------|-----------|
| Язык | C++17 |
| Веб-рендеринг | WebView2 (Windows) |
| Сборка | CMake + MinGW-w64 |
| GUI | HTML/CSS/JS (внутри webview) |

## Требования

- Windows 10/11
- WebView2 Runtime (встроен в Windows 11)

## Установка

### Из релиза

1. Скачайте `QuantumEngine.exe` и все `.dll` файлы из релиза
2. Поместите их в одну папку
3. Запустите `QuantumEngine.exe`

### Из исходников

```bash
# Клонируйте репозиторий
git clone https://github.com/username/quantum-engine.git
cd quantum-engine

# Соберите
mkdir build && cd build
cmake .. -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release
cmake --build . -j 4
```

## Структура проекта

```
Quantum Engine/
├── CMakeLists.txt
├── README.md
├── Upgrade.md
├── QuantumEngine.exe
├── src/
│   ├── main.cpp
│   ├── browser.h
│   └── browser.cpp
```

## Лицензия

MIT License

## Контакты

- GitHub Issues: https://github.com/username/quantum-engine/issues
