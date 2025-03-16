#include <Windows.h>
#include <Windowsx.h>
#include <vector>
#include <fstream> // Для std::ofstream и std::ifstream
#include <string>
#include <algorithm> // Для std::all_of
#include <ctime>
#include <random>
#include <cstdio> // Для fopen, fread, fwrite, fclose
#include <sstream> // Для std::istringstream
#pragma comment(lib, "winmm.lib")
#include <mmsystem.h> // Для PlaySound


// Прототипы функций
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
void SaveState(HWND hwnd, int method);
void LoadState(HWND hwnd, int cmdN, int method);
COLORREF GetRandomColor();
void CreateWinAPIMenu(HWND hwnd);
void ResetGridState(HWND hwnd);
void ResetColors(HWND hwnd);
void UpdateWindowTitle(HWND hwnd);
bool isOver(HWND hwnd);
void PlaySystemSound(LPCWSTR soundAlias);
void DefaultLoad();

// Глобальные переменные
const TCHAR szWinClass[] = L"MainWinAPIClass";
const int defaultN = 3;  // Размер сетки в ячейках по умолчанию
int n = defaultN;
const int defaultWidth = 320; // Размеры окна по умолчанию
const int defaultHeight = 240;
float cellWidth = defaultWidth / n; // Размеры клеток
float cellHeight = defaultHeight / n;
std::vector<std::vector<char>> gridState; // Состояние сетки: 'O' для кружков, 'X' для крестиков, '_' для пустых
const wchar_t* stateFileName = L"grid_state.txt"; // Файл для сохранения и загрузки размеров и цветов окна и сетки и состояния сетки
COLORREF backgroundColor = RGB(0, 0, 255); // Цвет фона (синий фон по умолчанию)
COLORREF gridColor = RGB(255, 0, 0); // Цвет линий (красный цвет по умолчанию)
char currentPlayer = 'X'; // Текущий игрок ('X' по умолчанию)
int method = 3;
#define OnClickResetGrid 1
#define OnClickExit 2
#define OnClickResetColors 3
#define OnClickChangeGridSize 4
#define OnClickAboutMenu 5

// Создание генератора случайных чисел
std::mt19937 rng(std::random_device{}());
std::uniform_int_distribution<> dist(0, 255); // Распределение для генерации целых чисел от 0 до 255

/// <summary>
/// Точка входа в приложение
/// </summary>
/// <param name="hInst"> Дескриптор приложения </param>
/// <param name="hPrevInstance"></param>
/// <param name="pCmdLine"> Аргументы командной строки, переданные при запуске программы </param>
/// <param name="nCmdShow"> Начальное состояние окна </param>
/// <returns></returns>
int WINAPI wWinMain(HINSTANCE hInst, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    // Создание и регистрация класса окна
    WNDCLASS SoftwareWindowsClass = { 0 };
    SoftwareWindowsClass.hIcon = LoadIcon(NULL, IDI_HAND);
    SoftwareWindowsClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    SoftwareWindowsClass.hInstance = hInst;
    SoftwareWindowsClass.lpszClassName = szWinClass;
    SoftwareWindowsClass.hbrBackground = CreateSolidBrush(backgroundColor);
    SoftwareWindowsClass.lpfnWndProc = WindowProc;

    if (!RegisterClassW(&SoftwareWindowsClass)) {
        return -1;
    }

    // Создание окна
    HWND hwnd = CreateWindowW(
        L"MainWinAPIClass",
        L"Крестики-нолики",
        WS_OVERLAPPEDWINDOW | WS_VISIBLE,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        defaultWidth,
        defaultHeight,
        NULL,
        NULL,
        hInst,
        NULL
    );

    // Обработка аргументов командной строки для изменения размера сетки и работы с файлами
    int argc;
    LPWSTR* argv = CommandLineToArgvW(GetCommandLineW(), &argc); // Разделение командной строки на аргументы
    if (!argv) {
        MessageBox(NULL, L"Ошибка обработки командной строки!", L"Ошибка", MB_ICONERROR);
        return -1;
    }

    int cmdN = 0; // Обработка аргументов
    int method = 3; // По умолчанию используем fstream
    for (int i = 1; i < argc; ++i) {
        if (wcscmp(argv[i], L"-m") == 0) {
            method = 1; // Отображение файлов на память
        }
        else if (wcscmp(argv[i], L"-f") == 0) {
            method = 2; // Файловые переменные
        }
        else if (wcscmp(argv[i], L"-s") == 0) {
            method = 3; // Потоки ввода-вывода
        }
        else if (wcscmp(argv[i], L"-w") == 0) {
            method = 4; // WinAPI / NativeAPI
        }
        else {
            // Попытка преобразовать аргумент в число (размер сетки)
            int value = _wtoi(argv[i]);
            if (value > 0 && value <= 9) {
                n = value;
                cmdN = n;
            }
        }
    }
    LocalFree(argv); // Освобождение памяти

    // Инициализация состояния сетки
    LoadState(hwnd, cmdN, method);

    RECT rect;
    GetClientRect(hwnd, &rect);
    cellWidth = (static_cast<float>(rect.right) - rect.left) / n;
    cellHeight = (static_cast<float>(rect.bottom) - rect.top) / n;

    // Создание меню
    CreateWinAPIMenu(hwnd);
    UpdateWindowTitle(hwnd);

    // Обработка сообщений окна
    MSG msg = { 0 };
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    HMENU hMenu = GetMenu(hwnd);
    if (hMenu) DestroyMenu(hMenu);
    DestroyWindow(hwnd);
    UnregisterClass(szWinClass, hInst);

    return 0;
}

/// <summary>
/// Обработчик сообщений окна
/// </summary>
/// <param name="hwnd"> Дескриптор окна </param>
/// <param name="uMsg"> Код сообщения </param>
/// <param name="wParam"> Дополнительная информация, связанная с сообщением </param>
/// <param name="lParam"> Дополнительная информация, связанная с сообщением </param>
/// <returns></returns>
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
    case WM_SIZE:
    {
        // Изменение размера окна
        RECT rect;
        GetClientRect(hwnd, &rect);
        cellWidth = (static_cast<float>(rect.right) - rect.left) / n;
        cellHeight = (static_cast<float>(rect.bottom) - rect.top) / n;
        InvalidateRect(hwnd, NULL, TRUE);
        return 0;
    }
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        // Рисование сетки
        HPEN hPen = CreatePen(PS_SOLID, 2, gridColor);
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

        for (int i = 1; i < n; ++i) {
            MoveToEx(hdc, i * cellWidth, 0, NULL);
            LineTo(hdc, i * cellWidth, n * cellHeight);

            MoveToEx(hdc, 0, i * cellHeight, NULL);
            LineTo(hdc, n * cellWidth, i * cellHeight);
        }

        // Рисование крестиков и ноликов
        for (int row = 0; row < n; ++row) {
            for (int col = 0; col < n; ++col) {
                int x = col * cellWidth;
                int y = row * cellHeight;

                if (gridState[row][col] == 'O') {
                    HPEN circlePen = CreatePen(PS_SOLID, 2, RGB(255, 255, 255));
                    HBRUSH hBrush = (HBRUSH)GetStockObject(NULL_BRUSH); // Прозрачная заливка

                    SelectObject(hdc, circlePen);
                    SelectObject(hdc, hBrush);
                    Ellipse(hdc, x + 0.9f * cellWidth, y + 0.9f * cellHeight, 
                        x + 0.1f * cellWidth, y + 0.1f * cellHeight);

                    DeleteObject(circlePen);
                    DeleteObject(hBrush);
                }
                else if (gridState[row][col] == 'X') {
                    HPEN crossPen = CreatePen(PS_SOLID, 2, RGB(255, 255, 255));
                    SelectObject(hdc, crossPen);

                    MoveToEx(hdc, x + 0.9f * cellWidth, y + 0.9f * cellHeight, NULL);
                    LineTo(hdc, x + 0.1f * cellWidth, y + 0.1f * cellHeight);
                    MoveToEx(hdc, x + 0.1f * cellWidth, y + 0.9f * cellHeight, NULL);
                    LineTo(hdc, x + 0.9f * cellWidth, y + 0.1f * cellHeight);

                    DeleteObject(crossPen);
                }
            }
        }

        SelectObject(hdc, hOldPen);
        DeleteObject(hPen);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
    {
        // Сохранение положения крестиков и ноликов
        int xPos = GET_X_LPARAM(lParam);
        int yPos = GET_Y_LPARAM(lParam);

        int col = xPos / cellWidth;
        int row = yPos / cellHeight;

        if (row < n && col < n && gridState[row][col] == '_') {
            if ((uMsg == WM_LBUTTONDOWN && currentPlayer == 'O') || (uMsg == WM_RBUTTONDOWN && currentPlayer == 'X')) {
                gridState[row][col] = currentPlayer;
                currentPlayer = (currentPlayer == 'O') ? 'X' : 'O';
            }
            InvalidateRect(hwnd, NULL, TRUE);
            if (isOver(hwnd)) {
                break;
            }
            UpdateWindowTitle(hwnd);
        }
        return 0;
    }
    case WM_KEYDOWN:
    {
        if ((wParam == 'Q' && (GetKeyState(VK_CONTROL) & 0x8000)) || wParam == VK_ESCAPE) {
            PostMessage(hwnd, WM_CLOSE, 0, 0); // Закрытие приложения
        }
        else if ((GetKeyState(VK_SHIFT) & 0x8000) && wParam == 'C') {
            ShellExecute(NULL, L"open", L"notepad.exe", NULL, NULL, SW_SHOWNORMAL); // Открытие блокнота
        }
        else if (wParam == VK_RETURN) {
            // Смена цвета фона на случайный
            backgroundColor = GetRandomColor();
            HBRUSH hOldBrush = (HBRUSH)GetClassLongPtr(hwnd, GCLP_HBRBACKGROUND);
            HBRUSH hNewBrush = CreateSolidBrush(backgroundColor);
            SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)hNewBrush);
            DeleteObject(hOldBrush);
            InvalidateRect(hwnd, NULL, TRUE);
        }
        else if ((wParam == 'R' && (GetKeyState(VK_CONTROL) & 0x8000))) {
            ResetGridState(hwnd); // Перезапуск партии
        }
        else if (wParam >= '1' && wParam <= '9') {
            int newSize = wParam - '0';
            n = newSize;
            RECT rect;
            GetClientRect(hwnd, &rect);
            cellWidth = (static_cast<float>(rect.right) - rect.left) / n;
            cellHeight = (static_cast<float>(rect.bottom) - rect.top) / n;
            ResetGridState(hwnd);
        }
        return 0;
    }
    case WM_MOUSEWHEEL:
    {
        // Плавное изменение цвета
        int delta = GET_WHEEL_DELTA_WPARAM(wParam);
        int r = GetRValue(gridColor);
        int g = GetGValue(gridColor);
        int b = GetBValue(gridColor);

        r = (r + delta / 6 + 256) % 256;
        g = (g + delta / 12 + 256) % 256;
        b = (b + delta / 4 + 256) % 256;

        gridColor = RGB(r, g, b);
        InvalidateRect(hwnd, NULL, TRUE);
        return 0;
    }
    case WM_COMMAND:
    {
        // Обработка сообещний меню
        switch (wParam) {
        case OnClickResetGrid:
        {
            ResetGridState(hwnd);
            break;
        } 
        case OnClickResetColors:
        {
            ResetColors(hwnd);
            break;
        }
        case OnClickExit:
        {
            PostQuitMessage(0);
            break;
        }
        case OnClickAboutMenu:
        {
            MessageBox(hwnd, L"Левая кнопка мыши - рисовать нолики\n"
                L"Правая кнопка мыши - рисовать крестики\n"
                L"CTRL+R - новая игра\n"
                L"1-9 - изменить количество клеток сетки\n"
                L"ENTER - изменить цвет фона на случайный\n"
                L"Прокрутка колёсика мыши - плавное изменение цвета сетки\n"
                L"SHIFT+C - открыть блокнот\n"
                L"ESC или CTRL+Q - выход", L"Справка", MB_ICONINFORMATION);
            break;
        }
        default:
            break;
        }
        return 0;
    }
    case WM_DESTROY:
    {
        SaveState(hwnd, method);

        HBRUSH hBrush = (HBRUSH)GetClassLongPtr(hwnd, GCLP_HBRBACKGROUND);
        if (hBrush) {
            DeleteObject(hBrush);
        }

        PostQuitMessage(0);
        return 0;
    }
    default:
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

/// <summary>
/// Функция для сохранения состояния сетки в файл
/// </summary>
/// <param name="hwnd"> Дескриптор окна </param>
/// <param name="method"> Способ работы с файлами, переданный через консоль </param>
void SaveState(HWND hwnd, int method) {
    switch (method) {
    case 1: // Отображение файлов на память
    {
        HANDLE hFile = CreateFile(
            stateFileName, 
            GENERIC_READ | GENERIC_WRITE, 
            0, 
            NULL, 
            CREATE_ALWAYS, 
            FILE_ATTRIBUTE_NORMAL, 
            NULL
        );
        if (hFile == INVALID_HANDLE_VALUE) return;

        HANDLE hMapFile = CreateFileMapping(
            hFile, 
            NULL, 
            PAGE_READWRITE, 
            0, 
            1024, 
            NULL
        );
        if (!hMapFile) {
            CloseHandle(hFile);
            return;
        }

        LPVOID pMap = MapViewOfFile(
            hMapFile, 
            FILE_MAP_ALL_ACCESS, 
            0, 
            0, 
            1024
        );
        if (!pMap) {
            CloseHandle(hMapFile);
            CloseHandle(hFile);
            return;
        }

        std::string data = std::to_string(n) + "\n";
        RECT rect;
        GetWindowRect(hwnd, &rect);
        data += std::to_string(rect.right - rect.left) + " " + std::to_string(rect.bottom - rect.top) + "\n";
        data += std::to_string(GetRValue(backgroundColor)) + " " + std::to_string(GetGValue(backgroundColor)) + " " + std::to_string(GetBValue(backgroundColor)) + "\n";
        data += std::to_string(GetRValue(gridColor)) + " " + std::to_string(GetGValue(gridColor)) + " " + std::to_string(GetBValue(gridColor)) + "\n";
        data += std::string(1, currentPlayer) + "\n";
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                data += gridState[i][j];
            }
            data += "\n";
        }

        CopyMemory(pMap, data.c_str(), data.size());
        UnmapViewOfFile(pMap);
        CloseHandle(hMapFile);
        CloseHandle(hFile);
        break;
    }
    case 2: // Файловые переменные
    {
        FILE* file = nullptr;
        errno_t err = _wfopen_s(&file, stateFileName, L"w");
        if (err != 0 || file == nullptr) {
            MessageBox(hwnd, L"Ошибка открытия файла для записи!", L"Ошибка", MB_ICONERROR);
            return;
        }

        fprintf(file, "%d\n", n);
        RECT rect;
        GetWindowRect(hwnd, &rect);
        fprintf(file, "%d %d\n", rect.right - rect.left, rect.bottom - rect.top);
        fprintf(file, "%d %d %d\n", GetRValue(backgroundColor), GetGValue(backgroundColor), GetBValue(backgroundColor));
        fprintf(file, "%d %d %d\n", GetRValue(gridColor), GetGValue(gridColor), GetBValue(gridColor));
        fprintf(file, "%c\n", currentPlayer);
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                fprintf(file, "%c", gridState[i][j]);
            }
            fprintf(file, "\n");
        }

        fclose(file);
        break;
    }
    case 3: // Потоки ввода-вывода
    {
        std::ofstream outFile(stateFileName);
        if (!outFile.is_open()) return;

        outFile << n << std::endl;
        RECT rect;
        GetWindowRect(hwnd, &rect);
        outFile << rect.right - rect.left << " " << rect.bottom - rect.top << std::endl;
        outFile << (int)GetRValue(backgroundColor) << " " << (int)GetGValue(backgroundColor) << " " << (int)GetBValue(backgroundColor) << std::endl;
        outFile << (int)GetRValue(gridColor) << " " << (int)GetGValue(gridColor) << " " << (int)GetBValue(gridColor) << std::endl;
        outFile << currentPlayer << std::endl;
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                outFile << gridState[i][j];
            }
            outFile << std::endl;
        }

        outFile.close();
        break;
    }
    case 4: // WinAPI / NativeAPI
    {
        HANDLE hFile = CreateFile(
            stateFileName, 
            GENERIC_WRITE, 
            0, 
            NULL, 
            CREATE_ALWAYS, 
            FILE_ATTRIBUTE_NORMAL, 
            NULL
        );
        if (hFile == INVALID_HANDLE_VALUE) return;

        std::string data = std::to_string(n) + "\n";
        RECT rect;
        GetWindowRect(hwnd, &rect);
        data += std::to_string(rect.right - rect.left) + " " + std::to_string(rect.bottom - rect.top) + "\n";
        data += std::to_string(GetRValue(backgroundColor)) + " " + std::to_string(GetGValue(backgroundColor)) + " " + std::to_string(GetBValue(backgroundColor)) + "\n";
        data += std::to_string(GetRValue(gridColor)) + " " + std::to_string(GetGValue(gridColor)) + " " + std::to_string(GetBValue(gridColor)) + "\n";
        data += std::string(1, currentPlayer) + "\n";
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                data += gridState[i][j];
            }
            data += "\n";
        }

        DWORD bytesWritten;
        WriteFile(hFile, data.c_str(), data.size(), &bytesWritten, NULL);
        CloseHandle(hFile);
        break;
    }
    }
}

/// <summary>
/// Функция для загрузки состояния сетки из файла
/// </summary>
/// <param name="hwnd"> Дескриптор окна </param>
/// <param name="cmdN"> Параметр n, переданный через консоль </param>
/// <param name="method"> Способ работы с файлами, переданный через консоль </param>
void LoadState(HWND hwnd, int cmdN, int method) {
    switch (method) {
    case 1: // Отображение файлов на память
    {
        HANDLE hFile = CreateFile(stateFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE) {
            DefaultLoad();
            break;
        }

        HANDLE hMapFile = CreateFileMapping(hFile, NULL, PAGE_READONLY, 0, 0, NULL);
        if (!hMapFile) {
            CloseHandle(hFile);
            DefaultLoad();
            break;
        }

        LPVOID pMap = MapViewOfFile(hMapFile, FILE_MAP_READ, 0, 0, 0);
        if (!pMap) {
            CloseHandle(hMapFile);
            CloseHandle(hFile);
            DefaultLoad();
            break;
        }

        // Чтение параметров
        std::string data(static_cast<char*>(pMap));
        std::istringstream iss(data);
        int savedN;
        iss >> savedN;
        n = savedN;
        iss.ignore();

        int width, height;
        iss >> width >> height;
        SetWindowPos(hwnd, NULL, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);
        iss.ignore();

        int r, g, b;
        iss >> r >> g >> b;
        backgroundColor = RGB(r, g, b);
        HBRUSH hOldBrush = (HBRUSH)GetClassLongPtr(hwnd, GCLP_HBRBACKGROUND);
        HBRUSH hNewBrush = CreateSolidBrush(backgroundColor);
        SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)hNewBrush);
        DeleteObject(hOldBrush);
        iss >> r >> g >> b;
        gridColor = RGB(r, g, b);
        iss.ignore();

        iss >> currentPlayer;
        iss.ignore();

        if (!cmdN || (cmdN == n)) {
            gridState.resize(n, std::vector<char>(n, '_'));
            for (int i = 0; i < n; ++i) {
                for (int j = 0; j < n; ++j) {
                    iss.get(gridState[i][j]);
                }
                iss.ignore();
            }
            n = savedN;
        }
        else {
            n = cmdN;
            currentPlayer = 'X';
            gridState.clear();
            gridState.resize(n, std::vector<char>(n, '_'));
        }

        UnmapViewOfFile(pMap);
        CloseHandle(hMapFile);
        CloseHandle(hFile);
        break;
    }
    case 2: // Файловые переменные
    {
        FILE* file = nullptr;
        errno_t err = _wfopen_s(&file, stateFileName, L"r"); // Попытка открытия файла
        if (err != 0 || file == nullptr) {
            MessageBox(hwnd, L"Ошибка открытия файла для чтения!", L"Ошибка", MB_ICONERROR);
            DefaultLoad();
            break;
        }

        // Чтение параметров
        int savedN;
        fscanf_s(file, "%d", &savedN);
        n = savedN;
        fgetc(file);

        int width, height;
        fscanf_s(file, "%d %d", &width, &height);
        SetWindowPos(hwnd, NULL, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);
        fgetc(file);

        int r, g, b;
        fscanf_s(file, "%d %d %d", &r, &g, &b);
        backgroundColor = RGB(r, g, b);
        HBRUSH hOldBrush = (HBRUSH)GetClassLongPtr(hwnd, GCLP_HBRBACKGROUND);
        HBRUSH hNewBrush = CreateSolidBrush(backgroundColor);
        SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)hNewBrush);
        DeleteObject(hOldBrush);
        fscanf_s(file, "%d %d %d", &r, &g, &b);
        gridColor = RGB(r, g, b);
        fgetc(file);

        fscanf_s(file, "%c", &currentPlayer);
        fgetc(file);

        if (!cmdN || (cmdN == n)) {
            gridState.resize(n, std::vector<char>(n, '_'));
            for (int i = 0; i < n; ++i) {
                for (int j = 0; j < n; ++j) {
                    fscanf_s(file, "%c", &gridState[i][j]);
                }
                fgetc(file);
            }
            n = savedN;
        }
        else {
            n = cmdN;
            currentPlayer = 'X';
            gridState.clear();
            gridState.resize(n, std::vector<char>(n, '_'));
        }

        fclose(file);
        break;
    }
    case 3: // Потоки ввода-вывода
    {
        std::ifstream inFile(stateFileName);
        // Попытка открытия файла
        if (!inFile.is_open()) {
            DefaultLoad();
            break;
        }

        // Чтение параметров
        int savedN;
        inFile >> savedN;
        n = savedN;
        inFile.ignore();

        int width, height;
        inFile >> width >> height;
        SetWindowPos(hwnd, NULL, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);
        inFile.ignore();

        int r, g, b;
        inFile >> r >> g >> b;
        backgroundColor = RGB(r, g, b);
        HBRUSH hOldBrush = (HBRUSH)GetClassLongPtr(hwnd, GCLP_HBRBACKGROUND);
        HBRUSH hNewBrush = CreateSolidBrush(backgroundColor);
        SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)hNewBrush);
        DeleteObject(hOldBrush);
        inFile >> r >> g >> b;
        gridColor = RGB(r, g, b);
        inFile.ignore();

        inFile >> currentPlayer;
        inFile.ignore();

        if (!cmdN || (cmdN == n)) {
            gridState.resize(n, std::vector<char>(n, '_'));
            for (int i = 0; i < n; ++i) {
                for (int j = 0; j < n; ++j) {
                    inFile.get(gridState[i][j]);
                }
                inFile.ignore();
            }
            n = savedN;
        }
        else {
            n = cmdN;
            currentPlayer = 'X';
            gridState.clear();
            gridState.resize(n, std::vector<char>(n, '_'));
        }

        inFile.close();
        break;
    }
    case 4: // WinAPI / NativeAPI
    {
        HANDLE hFile = CreateFile(stateFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        if (hFile == INVALID_HANDLE_VALUE) {
            DefaultLoad();
            break;
        }

        DWORD fileSize = GetFileSize(hFile, NULL); // Попытка подсчёта размера файла
        if (fileSize == INVALID_FILE_SIZE) {
            CloseHandle(hFile);
            DefaultLoad();
            break;
        }

        std::vector<char> buffer(fileSize + 1);
        DWORD bytesRead;
        // Попытка чтения файла
        if (!ReadFile(hFile, buffer.data(), fileSize, &bytesRead, NULL)) {
            CloseHandle(hFile);
            DefaultLoad();
            break;
        }
        buffer[fileSize] = '\0';

        // Чтение параметров
        std::istringstream iss(buffer.data());
        int savedN;
        iss >> savedN;
        n = savedN;
        iss.ignore();

        int width, height;
        iss >> width >> height;
        SetWindowPos(hwnd, NULL, 0, 0, width, height, SWP_NOMOVE | SWP_NOZORDER);
        iss.ignore();

        int r, g, b;
        iss >> r >> g >> b;
        backgroundColor = RGB(r, g, b);
        HBRUSH hOldBrush = (HBRUSH)GetClassLongPtr(hwnd, GCLP_HBRBACKGROUND);
        HBRUSH hNewBrush = CreateSolidBrush(backgroundColor);
        SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)hNewBrush);
        DeleteObject(hOldBrush);
        iss >> r >> g >> b;
        gridColor = RGB(r, g, b);
        iss.ignore();

        iss >> currentPlayer;
        iss.ignore();

        if (!cmdN || (cmdN == n)) {
            gridState.resize(n, std::vector<char>(n, '_'));
            for (int i = 0; i < n; ++i) {
                for (int j = 0; j < n; ++j) {
                    iss.get(gridState[i][j]);
                }
                iss.ignore();
            }
            n = savedN;
        }
        else {
            n = cmdN;
            currentPlayer = 'X';
            gridState.clear();
            gridState.resize(n, std::vector<char>(n, '_'));
        }

        CloseHandle(hFile);
        break;
    }
    default:
    {
        DefaultLoad();
        break;
    }
    }
}

/// <summary>
/// Запуск приложения по умолчанию при сбое методов чтения файла
/// </summary>
void DefaultLoad() {
    gridState.clear();
    gridState.resize(n, std::vector<char>(n, '_'));
}

/// <summary>
/// Функция для генерации случайного цвета
/// </summary>
/// <returns> Цвет в формате RGB(r, g, b) </returns>
COLORREF GetRandomColor() {
    return RGB(dist(rng), dist(rng), dist(rng));
}

/// <summary>
/// Создание меню
/// </summary>
/// <param name="hwnd"> Дескриптор окна </param>
void CreateWinAPIMenu(HWND hwnd) {
    HMENU rootMenu = CreateMenu();
    HMENU subMenu = CreateMenu();
    AppendMenu(rootMenu, MF_POPUP, (int)subMenu, L"Меню");
    AppendMenu(rootMenu, MF_STRING, OnClickAboutMenu, L"Справка");
    AppendMenu(subMenu, MF_STRING, OnClickResetGrid, L"Новая игра");
    AppendMenu(subMenu, MF_STRING, OnClickResetColors, L"Сброс цветов");
    AppendMenu(subMenu, MF_STRING, OnClickExit, L"Выход");

    SetMenu(hwnd, rootMenu);
}

/// <summary>
/// Сброс состояния сетки
/// </summary>
/// <param name="hwnd"> Дескриптор окна </param>
void ResetGridState(HWND hwnd) {
    gridState.clear(); // Очищаем текущее состояние сетки
    gridState.resize(n, std::vector<char>(n, '_')); // Создаем новую сетку
    currentPlayer = 'X';
    UpdateWindowTitle(hwnd);
    InvalidateRect(hwnd, NULL, TRUE);
}

/// <summary>
/// Сброс цветов
/// </summary>
/// <param name="hwnd"> Дескриптор окна </param>
void ResetColors(HWND hwnd) {
    backgroundColor = RGB(0, 0, 255);
    gridColor = RGB(255, 0, 0);
    SetClassLongPtr(hwnd, GCLP_HBRBACKGROUND, (LONG_PTR)CreateSolidBrush(backgroundColor));
    InvalidateRect(hwnd, NULL, TRUE);
}

/// <summary>
/// Обновление названия окна
/// </summary>
/// <param name="hwnd"> Дескриптор окна </param>
void UpdateWindowTitle(HWND hwnd) {
    wchar_t title[50];
    wsprintf(title, L"Крестики-нолики - Ход игрока: %c", currentPlayer);
    SetWindowText(hwnd, title);
}

/// <summary>
/// Проверка на завершение игры
/// </summary>
/// <param name="hwnd"> Дескриптор окна </param>
/// <returns> Завершена игра или нет </returns>
bool isOver(HWND hwnd) {
    char winner = '_';
    // Проверка горизонталей и вертикалей
    for (int i = 0; i < n; ++i) {
        if (winner == '_' && gridState[i][0] != '_' && std::all_of(gridState[i].begin(), gridState[i].end(), [i](char c) { return c == gridState[i][0]; }))
            winner = gridState[i][0];
        if (winner == '_' && gridState[0][i] != '_' && std::all_of(gridState.begin(), gridState.end(), [i](std::vector<char>& row) { return row[i] == gridState[0][i]; }))
            winner = gridState[0][i];
    }

    // Проверка диагоналей
    if (winner == '_') {
        bool left_diag = true, right_diag = true;
        for (int i = 0; i < n; ++i) {
            if (gridState[i][i] != gridState[0][0]) left_diag = false;
            if (gridState[i][n - 1 - i] != gridState[0][n - 1]) right_diag = false;
        }

        if (gridState[0][0] != '_' && left_diag) winner = gridState[0][0];
        if (gridState[0][n - 1] != '_' && right_diag) winner = gridState[0][n - 1];
    }

    // Завершение игры
    if (winner == '_') {
        bool fullFilled = true;
        for (int i = 0; i < n; ++i) {
            if (fullFilled)
                for (int j = 0; j < n; ++j) {
                    if (gridState[i][j] == '_') {
                        fullFilled = false;
                        break;
                    }
                }
        }

        if (fullFilled) {
            PlaySystemSound(L"SYSTEMDEFAULT");
            MessageBox(hwnd, L"Ничья!", L"Ничья", MB_OK);
            ResetGridState(hwnd);
            return true;
        }
    }
    else {
        PlaySystemSound(L"SYSTEMDEFAULT");
        MessageBox(hwnd, (std::wstring(L"Игрок ") + static_cast<wchar_t>(winner) + L" победил!").c_str(), L"Победа", MB_OK);
        ResetGridState(hwnd);
        return true;
    }
}

/// <summary>
/// Воспроизводит системный звук.
/// </summary>
/// <param name="soundAlias"> Системный звук </param>
void PlaySystemSound(LPCWSTR soundAlias) {
    PlaySound(soundAlias, NULL, SND_ALIAS | SND_ASYNC);
}
