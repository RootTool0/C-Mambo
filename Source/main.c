#include <stdio.h>
#include <conio.h>
#include <stdint.h>
#include <time.h>
#include <windows.h>
#include <stdbool.h>

#define KEY_UP    72
#define KEY_DOWN  80
#define KEY_LEFT  75
#define KEY_RIGHT 77
#define KEY_ENTER 13
#define KEY_ESC   27

typedef enum
{
    CT_Main,
    CT_Game
} EConsoleType;

enum EMenuOption
{
    MO_StartGame = 0,
    MO_Size = 1,
    MO_FillChance = 2,
    MO_EdgeChance = 3,
    MO_Count
};

HANDLE GConsoleHandler;
EConsoleType GConsoleType = CT_Game;

int8_t GMenuSelected = 0;

int8_t GGameSize = 6;
int8_t GGameFillChance = 35;
int8_t GGameEdgeChance = 15;
int16_t GGameX = 0;
int16_t GGameY = 0;
bool GGamePlaying = false;

#define GAME_MAX_SIZE 16

uint8_t GGameAnswerGrid[GAME_MAX_SIZE * GAME_MAX_SIZE];
uint8_t GGameUserGrid[GAME_MAX_SIZE * GAME_MAX_SIZE];
uint8_t GGameEdgeH[GAME_MAX_SIZE * GAME_MAX_SIZE] = {0};
uint8_t GGameEdgeV[GAME_MAX_SIZE * GAME_MAX_SIZE] = {0};

// Grid:
// x00 (0b0000) - Empty
// x01 (0b0001) - "0"
// x02 (0b0010) - "X"
// x03 (0b0011) - NOT USED
// x04 (0b01xx) - Blocked
// x08 (0b1xxx) - Has Error

// Edge:
// 0x00 - Empty
// 0x01 - "="
// 0x02 - "x'

void GameSetValue(uint8_t* InGrid, const int16_t InX, const int16_t InY, const uint8_t InValue) { InGrid[InY * GGameSize + InX] = InValue & 0x03; }
uint8_t GameGetValue(const uint8_t* InGrid, const int16_t InX, const int16_t InY) { return InGrid[InY * GGameSize + InX] & 0x03; }

void GameSetLocked(uint8_t* InGrid, const int16_t InX, const int16_t InY, const bool bLocked) { if(bLocked) InGrid[InY * GGameSize + InX] |= 0x04; else InGrid[InY * GGameSize + InX] &= ~0x04; }
bool GameGetLocked(const uint8_t* InGrid, int16_t InX, int16_t InY) { return (InGrid[InY * GGameSize + InX] & 0x04) != 0 ? true : false; }

void GameSetError(uint8_t* InGrid, const int16_t InX, const int16_t InY, const bool bError) { if(bError) InGrid[InY * GGameSize + InX] |= 0x08; else InGrid[InY * GGameSize + InX] &= ~0x08; }
bool GameGetError(const uint8_t* InGrid, int16_t InX, int16_t InY) { return (InGrid[InY * GGameSize + InX] & 0x08) != 0 ? true : false; }

inline uint8_t GameGetEdgeH(const int16_t InX, const int16_t InY) { return GGameEdgeH[InY * GGameSize + InX]; }
inline void GameSetEdgeH(const int16_t InX, const int16_t InY, uint8_t InVal) { GGameEdgeH[InY * GGameSize + InX] = InVal; }

inline uint8_t GameGetEdgeV(const int16_t InX, const int16_t InY) { return GGameEdgeV[InY * GGameSize + InX]; }
inline void GameSetEdgeV(const int16_t InX, const int16_t InY, uint8_t InVal) { GGameEdgeV[InY * GGameSize + InX] = InVal; }

bool GameAnswerCheckIsValid(const int16_t InX, const int16_t InY, const uint8_t InValue)
{
    if(InX >= 2 && GameGetValue(GGameAnswerGrid, InX - 1, InY) == InValue && GameGetValue(GGameAnswerGrid, InX - 2, InY) == InValue) return false;
    if(InY >= 2 && GameGetValue(GGameAnswerGrid, InX, InY - 1) == InValue && GameGetValue(GGameAnswerGrid, InX, InY - 2) == InValue) return false;
    
    uint8_t SumX = 0;
    for(int16_t X = 0; X < GGameSize; ++X)
        if(GameGetValue(GGameAnswerGrid, X, InY) == InValue)
            ++SumX;
    if(SumX > GGameSize / 2) return false;
    
    uint8_t SumY = 0;
    for(int16_t Y = 0; Y < GGameSize; ++Y)
        if(GameGetValue(GGameAnswerGrid, InX, Y) == InValue)
            ++SumY;
    if(SumY > GGameSize / 2) return false;
    
    return true;
}

// 0 - Nothing
// 1 - Error
// 2 - Full
uint8_t GameUserCheckIsValid(const int16_t InX, const int16_t InY)
{
    const uint8_t Value = GameGetValue(GGameUserGrid, InX, InY);
    
    if(Value == 0) return 0;
    
    {
        if(InX >= 2                && GameGetValue(GGameUserGrid, InX - 1, InY) == Value && GameGetValue(GGameUserGrid, InX - 2, InY) == Value                                                      ) return 1;
        if(InX >= 1                && InX < GGameSize - 1                            && GameGetValue(GGameUserGrid, InX - 1, InY) == Value && GameGetValue(GGameUserGrid, InX + 1, InY) == Value) return 1;
        if(InX < GGameSize - 2 && GameGetValue(GGameUserGrid, InX + 1, InY) == Value && GameGetValue(GGameUserGrid, InX + 2, InY) == Value                                                      ) return 1;
    
        if(InY >= 2                && GameGetValue(GGameUserGrid, InX, InY - 1) == Value && GameGetValue(GGameUserGrid, InX, InY - 2) == Value                                                      ) return 1;
        if(InY >= 1                && InY < GGameSize - 1                            && GameGetValue(GGameUserGrid, InX, InY - 1) == Value && GameGetValue(GGameUserGrid, InX, InY + 1) == Value) return 1;
        if(InY < GGameSize - 2 && GameGetValue(GGameUserGrid, InX, InY + 1) == Value && GameGetValue(GGameUserGrid, InX, InY + 2) == Value                                                      ) return 1;
    }
    
    {
        if(InX > 0)
        {
            const uint8_t LeftEdge = GameGetEdgeH(InX - 1, InY);
            const uint8_t LeftVal = GameGetValue(GGameUserGrid, InX - 1, InY);
            if(LeftEdge != 0 && LeftVal != 0)
            {
                if(LeftEdge == 1 && Value != LeftVal) return 1;
                if(LeftEdge == 2 && Value == LeftVal) return 1;
            }
        }
        if(InX < GGameSize - 1)
        {
            const uint8_t RightEdge = GameGetEdgeH(InX, InY);
            const uint8_t RightVal = GameGetValue(GGameUserGrid, InX + 1, InY);
            if(RightEdge != 0 && RightVal != 0)
            {
                if(RightEdge == 1 && Value != RightVal) return 1;
                if(RightEdge == 2 && Value == RightVal) return 1;
            }
        }
        
        if(InY > 0)
        {
            const uint8_t TopEdge = GameGetEdgeV(InX, InY - 1);
            const uint8_t TopVal = GameGetValue(GGameUserGrid, InX, InY - 1);
            if(TopEdge != 0 && TopVal != 0)
            {
                if(TopEdge == 1 && Value != TopVal) return 1;
                if(TopEdge == 2 && Value == TopVal) return 1;
            }
        }
        if(InY < GGameSize - 1)
        {
            const uint8_t BottomEdge = GameGetEdgeV(InX, InY);
            const uint8_t BottomVal = GameGetValue(GGameUserGrid, InX, InY + 1);
            if(BottomVal != 0)
            {
                if(BottomEdge == 1 && Value != BottomVal) return 1;
                if(BottomEdge == 2 && Value == BottomVal) return 1;
            }
        }
    }
    
    bool bHasFullX = true;
    bool bHasFullY = true;
    {
        int8_t BalanceX = 0;
        for(int16_t X = 0; X < GGameSize; ++X)
        {
            uint8_t ValueX = GameGetValue(GGameUserGrid, X, InY);
            
            if(ValueX == 0) { bHasFullX = false; break; }
            if(ValueX == 1) ++BalanceX;
            if(ValueX == 2) --BalanceX;
        }
        if(bHasFullX && BalanceX != 0) return 1;
        
        int8_t BalanceY = 0;
        for(int16_t Y = 0; Y < GGameSize; ++Y)
        {
            uint8_t ValueX = GameGetValue(GGameUserGrid, InX, Y);
            
            if(ValueX == 0) { bHasFullY = false; break; }
            if(ValueX == 1) ++BalanceY;
            if(ValueX == 2) --BalanceY;
        }
        if(bHasFullY && BalanceY != 0) return 1;
    }
    
    return bHasFullX || bHasFullY ? 2 : 0;
}

bool GameBacktrackGenerate(const int16_t InX, const int16_t InY)
{
    if(InY == GGameSize) return true;
    
    if(InX == GGameSize) return GameBacktrackGenerate(0, InY + 1);
    
    bool bRandom = rand() % 2 == 0;
    uint8_t Choices[2] = { bRandom ? 1 : 2, bRandom ? 2 : 1};
    
    for(int8_t i = 0; i < 2; ++i)
    {
        GameSetValue(GGameAnswerGrid, InX, InY, Choices[i]);
        
        if(GameAnswerCheckIsValid(InX, InY, Choices[i]) && GameBacktrackGenerate(InX + 1, InY))
            return true;
        
        GameSetValue(GGameAnswerGrid, InX, InY, -1);
    }
    
    return false;
}

void GameStart(void)
{
    for(int16_t i = 0; i < GGameSize * GGameSize; ++i)
    {
        GGameAnswerGrid[i] = 0;
        GGameUserGrid[i] = 0;
        GGameEdgeH[i] = 0;
        GGameEdgeV[i] = 0;
    }
    
    unsigned int Seed = time(NULL); 
    Seed ^= (unsigned int)(uintptr_t)&Seed; 
    srand(Seed);
    
    GameBacktrackGenerate(0, 0);
    
    for(int16_t Y = 0; Y < GGameSize; ++Y)
    {
        for(int16_t X = 0; X < GGameSize; ++X)
        {
            if(rand() % 100 < GGameFillChance)
            {
                GameSetValue(GGameUserGrid, X, Y, GameGetValue(GGameAnswerGrid, X, Y));
                GameSetLocked(GGameUserGrid, X, Y, true);
            }
            
            if(X < GGameSize - 1 && rand() % 100 < GGameEdgeChance)
                GameSetEdgeH(X, Y, (GameGetValue(GGameAnswerGrid, X, Y) == GameGetValue(GGameAnswerGrid, X + 1, Y)) ? 1 : 2);
            
            if (Y < GGameSize - 1 && rand() % 100 < GGameEdgeChance) 
                GameSetEdgeV(X, Y, (GameGetValue(GGameAnswerGrid, X, Y) == GameGetValue(GGameAnswerGrid, X, Y + 1)) ? 1 : 2);
        }
    }
}

void ConsoleSetConsoleType(EConsoleType InConsoleType)
{
    system("cls");
    GConsoleType = InConsoleType;

    switch(GConsoleType)
    {
    case CT_Main:
        GMenuSelected = 0;
        break;
    case CT_Game:
        GGameX = 0;
        GGameY = 0;
        GameStart();
        break;
    }
}

void ConsoleHandleKey(const int InKey)
{
    switch(GConsoleType)
    {
    case CT_Main:
        {
            
            switch(InKey)
            {
            default: break;
                
            case KEY_UP:
                --GMenuSelected;
                if(GMenuSelected < 0) GMenuSelected = MO_Count - 1;
                break;
            case KEY_DOWN:
                ++GMenuSelected;
                if(GMenuSelected > MO_Count - 1) GMenuSelected = 0;
                break;
                
            case KEY_LEFT:
                {
                    switch(GMenuSelected)
                    {
                    default: break;
                        
                    case MO_Size:
                        GGameSize -= 2;
                        if(GGameSize < 2) GGameSize = 2;
                        break;
                    case MO_FillChance:
                        if(--GGameFillChance < 0) GGameFillChance = 0;
                        break;
                    case MO_EdgeChance:
                        if(--GGameEdgeChance < 0) GGameEdgeChance = 0;
                        break;
                    }
                }
                break;
            case KEY_RIGHT:
                {
                    switch(GMenuSelected)
                    {
                    default: break;
                        
                    case MO_Size:
                        GGameSize += 2;
                        if(GGameSize > GAME_MAX_SIZE) GGameSize = GAME_MAX_SIZE;
                        break;
                    case MO_FillChance:
                        if(++GGameFillChance > 100) GGameFillChance = 100;
                        break;
                    case MO_EdgeChance:
                        if(++GGameEdgeChance > 100) GGameEdgeChance = 100;
                        break;
                    }
                }
                break;
                
            case KEY_ENTER:
                if(GMenuSelected == MO_StartGame)
                    ConsoleSetConsoleType(CT_Game);
                break;
            case KEY_ESC:
				exit(0);
                break;
            }
            
        }
        break;
    case CT_Game:
        {
            if(GGamePlaying)
                ConsoleSetConsoleType(CT_Main);
            
            switch(InKey)
            {
            default: break;
                
            case KEY_UP:
                --GGameY;
                if(GGameY < 0) GGameY = GGameSize - 1;
                break;
            case KEY_DOWN:
                ++GGameY;
                if(GGameY > GGameSize - 1) GGameY = 0;
                break;
                
            case KEY_LEFT:
                --GGameX;
                if(GGameX < 0) GGameX = GGameSize - 1;
                break;
            case KEY_RIGHT:
                ++GGameX;
                if(GGameX > GGameSize - 1) GGameX = 0;
                break;
                
            case KEY_ENTER:
                {
                    if(GameGetLocked(GGameUserGrid, GGameX, GGameY)) break;
                    
                    uint8_t Value = GameGetValue(GGameUserGrid, GGameX, GGameY) + 1;
                    if(Value > 2) Value = 0;
                    GameSetValue(GGameUserGrid, GGameX, GGameY, Value);
                }
                break;
            case KEY_ESC:
                ConsoleSetConsoleType(CT_Main);
                break;
            }
        }
        break;
    }
}

void ConsoleRenderScreen(void)
{
    switch(GConsoleType)
    {
    case CT_Main:
        printf("  %s %s %s\n", GMenuSelected == MO_StartGame ? "[" : " ", "Start Game"  , GMenuSelected == MO_StartGame ? "]" : " ");
        printf("\n");
        printf("    Size:        %s%d\033[0m   \n", GMenuSelected == MO_Size       ? "\033[7m" : "", GGameSize      );
        printf("    Fill Chance: %s%d\033[0m %%\n", GMenuSelected == MO_FillChance ? "\033[7m" : "", GGameFillChance);
        printf("    Edge Chance: %s%d\033[0m %%\n", GMenuSelected == MO_EdgeChance ? "\033[7m" : "", GGameEdgeChance);
        break;
    case CT_Game:
        GGamePlaying = true;
        for(int16_t Y = 0; Y < GGameSize; ++Y)
        {
            for(int16_t X = 0; X < GGameSize; ++X)
            {
                if(GameGetValue(GGameUserGrid, X, Y) != GameGetValue(GGameAnswerGrid, X, Y))
                    GGamePlaying = false;
                
                printf("\033[1;");
                
                if(GGameX == X && GGameY == Y)
                    printf("4;");
                
                uint8_t CheckResult = GameUserCheckIsValid(X, Y);
                if(CheckResult == 1)
                    printf("48;5;52;");
                else if(CheckResult == 2)
                    printf("48;5;22;");
                else if(GameGetLocked(GGameUserGrid, X, Y))
                    printf("48;5;239;");
                
                switch(GameGetValue(GGameUserGrid, X, Y))
                {
                default: printf("37m "); break;
                case 1:  printf("36m0"); break;
                case 2:  printf("92mX"); break;
                }
                
                printf("\033[0m");
                
                if(X == GGameSize - 1) continue;
                
                printf(" \033[1;33m");
                
                switch(GameGetEdgeH(X, Y))
                {
                default: printf(" "); break;
                case 1:  printf("="); break;
                case 2:  printf("x"); break;
                }
                
                printf("\033[0m ");
            }
            printf("\n");
            
            if(Y == GGameSize - 1) continue;
            
            for(int16_t X = 0; X < GGameSize; ++X)
            {
                printf("\033[1;33m");
                
                switch(GameGetEdgeV(X, Y))
                {
                default: printf(" "); break;
                case 1:  printf(":"); break;
                case 2:  printf("x"); break;
                }
                
                printf("\033[0m   ");
            }
            printf("\n");
        }
        break;
    }
}

int main(void)
{
    GConsoleHandler = GetStdHandle(STD_OUTPUT_HANDLE);
    
    CONSOLE_CURSOR_INFO CursorInfo;
    GetConsoleCursorInfo(GConsoleHandler, &CursorInfo);
    CursorInfo.bVisible = FALSE; 
    SetConsoleCursorInfo(GConsoleHandler, &CursorInfo);
    
    DWORD ConsoleMode;
    GetConsoleMode(GConsoleHandler, &ConsoleMode);
    SetConsoleMode(GConsoleHandler, ConsoleMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    
    ConsoleSetConsoleType(CT_Main);
    
    while(true)
    {
        SetConsoleCursorPosition(GConsoleHandler, (COORD){0, 0});
        
        printf("====== C-Mambo ======\n\n");
        
        ConsoleRenderScreen();
        
        int Key = _getch();
        if(Key == 0 || Key == 224) Key = _getch();
        
        ConsoleHandleKey(Key);
    }
}
