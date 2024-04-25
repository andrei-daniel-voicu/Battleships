#define _POSIX_C_SOURCE 199309L
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SHIP 'X'
#define EMPTY ' '
#define TURN_DELAY 0.1
#define BLINK_DELAY 0.7
#define DESTROYCOOLDOWN 1
#define RANDOMIZECOOLDOWN 1
#define WHITE_PAIR 1
#define WHITE_ON_BLACK 2
#define WHITE_ON_RED 3
#define BLACK_ON_RED 4
#define RED_ON_RED 5
#define WHITE_ON_WHITE 6
#define BLACK_ON_WHITE 7

typedef enum
{
    MENU,
    CHOOSE,
    LEADERBOARD,
    GAME,
    END,
} TYPE;

typedef struct
{
    int x;
    int y;
    int hit;
    int destroyed;
    char value;
} CELL;

typedef struct
{
    WINDOW *menu;
    WINDOW *enemy;
    WINDOW *player;
    WINDOW *score;
} WINDOWS;

typedef struct
{
    int nrConfigs;
    int selectedConfig;
    char ***configs;
    char **configNames;
    TYPE windowType;
    int gameQuit;
    int foundSave;
    int resume;
    int nrScores;
    int scores[11];
    int won;
    int playerDestroyed;
    int enemyDestroyed;
    int width;
    int height;
} PROPERTIES;

typedef struct
{
    enum
    {
        NEW_GAME,
        RESUME_GAME,
        LEADERBOARD_MENU,
        QUIT,
    } selected;
    int pressed;
    int foundTitle;
    char **title;

} MAIN_MENU;

typedef struct
{
    int selected;
    int pressed;
} CHOOSE_MENU;

typedef struct
{
    CELL playerMap[10][10];
    CELL enemyMap[10][10];
    CELL lastCell;
    CELL startCell;
    float deltaTime;
    float destroyTime;
    float randomizeTime;
    float timePassedBlink;
    float timePassedTurn;
    int foundShip;
    int lastDir;
    int xSelected;
    int ySelected;
    int pressed;
    int enemyShips;
    int playerShips;
    int blink;
    int turn;
    int input;
    int randomize;
    int destroy;
    int playerCombo;
    int enemyCombo;
    int playerScore;
    int enemyScore;
} GAME_MENU;

typedef struct
{
    int foundArt;
} END_MENU;

void BubbleSort(int *array, int n)
{
    int i, j;
    for (i = 0; i < n; i++)
        for (j = 0; j < n - i - 1; j++)
        {
            if (array[j + 1] > array[j])
            {
                int aux = array[j];
                array[j] = array[j + 1];
                array[j + 1] = aux;
            }
        }
}
int CheckBounds(int i, int j)
{
    if (i < 0 || i > 9)
        return 0;
    if (j < 0 || j > 9)
        return 0;
    return 1;
}
int GetInput(WINDOW *window)
{
    return wgetch(window);
}
void Initialise(PROPERTIES *properties)
{
    FILE *file;

    initscr();
    noecho();
    cbreak();
    srand(time(0));
    start_color();
    use_default_colors();
    init_pair(WHITE_PAIR, COLOR_YELLOW, COLOR_GREEN);
    init_pair(WHITE_ON_BLACK, COLOR_WHITE, COLOR_BLACK);
    init_pair(BLACK_ON_WHITE, COLOR_BLACK, COLOR_WHITE);
    init_pair(WHITE_ON_RED, COLOR_WHITE, COLOR_RED);
    init_pair(BLACK_ON_RED, COLOR_BLACK, COLOR_RED);
    init_pair(RED_ON_RED, COLOR_RED, COLOR_RED);
    init_pair(WHITE_ON_WHITE, COLOR_WHITE, COLOR_WHITE);
    properties->nrConfigs = 0;
    properties->selectedConfig = -1;
    properties->windowType = MENU;
    properties->gameQuit = 0;
    properties->configs = 0;
    properties->resume = 0;
    properties->won = -1;
    properties->nrScores = 0;
    properties->width = getmaxx(stdscr);
    properties->height = getmaxy(stdscr);
    file = fopen("leaderboard", "ab+");
    int dummy;

    while (fread(&dummy, sizeof(int), 1, file))
    {

        properties->scores[properties->nrScores++] = dummy;
    }
    fclose(file);
}
void ResizeWindow(PROPERTIES *properties, WINDOW *window)
{
    if (properties->width != getmaxx(stdscr))
    {
        properties->width = getmaxx(stdscr);
        wclear(window);
        box(window, 0, 0);
    }
    if (properties->height != getmaxy(stdscr))
    {
        properties->height = getmaxy(stdscr);
        wclear(window);
        box(window, 0, 0);
    }
}
void InitialiseMenu(MAIN_MENU *menu, PROPERTIES *properties, WINDOWS *windows)
{
    FILE *file;

    menu->selected = 0;
    menu->pressed = 0;
    windows->menu = newwin(properties->height, properties->width, 0, 0);
    properties->resume = 0;
    curs_set(0);
    box(windows->menu, 0, 0);
    nodelay(windows->menu, TRUE);
    keypad(windows->menu, TRUE);
    if (file = fopen("save", "rb"))
    {
        properties->foundSave = 1;
        fclose(file);
    }
    else
    {
        properties->foundSave = 0;
    }
}
void DrawMenu(char **menus, MAIN_MENU menu, PROPERTIES *properties, WINDOWS *windows)
{
    int i;

    for (i = 0; i < 4; i++)
    {
        if (i == menu.selected)
            wattron(windows->menu, A_STANDOUT);
        if (i == RESUME_GAME && !properties->foundSave)
        {

            wattron(windows->menu, COLOR_PAIR(WHITE_ON_BLACK));
        }
        mvwprintw(windows->menu, i + getmaxy(stdscr) / 2 - 4 + i, getmaxx(stdscr) / 2 - strlen(menus[i]) / 2, "%s", menus[i]);
        wstandend(windows->menu);
    }
    wrefresh(windows->menu);
}
void UpdateMenu(MAIN_MENU *menu, char **menus, PROPERTIES *properties, WINDOWS *windows)
{
    int input;

    InitialiseMenu(menu, properties, windows);
    while (1)
    {
        input = GetInput(windows->menu);
        switch (input)
        {
        case 's':
        case KEY_DOWN:
            if (menu->selected == 3)
                menu->selected = 0;
            else
            {
                menu->selected++;
                if (menu->selected == RESUME_GAME && !properties->foundSave)
                {
                    menu->selected++;
                }
            }
            break;
        case 'w':
        case KEY_UP:
            if (menu->selected == 0)
                menu->selected = 3;
            else
            {
                menu->selected--;
                if (menu->selected == RESUME_GAME && !properties->foundSave)
                {
                    menu->selected--;
                }
            }
            break;
        case '\n':
            menu->pressed = 1;
            break;
        case 'q':
            wclear(windows->menu);
            wrefresh(windows->menu);
            properties->gameQuit = 1;
            return;
        }
        if (menu->pressed == 1)
        {
            switch (menu->selected)
            {
            case NEW_GAME:
                wclear(windows->menu);
                wrefresh(windows->menu);
                properties->windowType = CHOOSE;
                return;
                break;
            case RESUME_GAME:
                wclear(windows->menu);
                wrefresh(windows->menu);
                properties->windowType = GAME;
                properties->resume = 1;
                return;
                break;
            case LEADERBOARD_MENU:
                wclear(windows->menu);
                wrefresh(windows->menu);
                properties->windowType = LEADERBOARD;
                return;
                break;
            case QUIT:
                properties->gameQuit = 1;
                wclear(windows->menu);
                wrefresh(windows->menu);
                return;
                break;
            }
            menu->pressed = 0;
        }
        ResizeWindow(properties, windows->menu);
        DrawMenu(menus, *menu, properties, windows);
    }
}
void InitialiseChoose(CHOOSE_MENU *choose, PROPERTIES *properties, WINDOWS *windows)
{
    windows->menu = newwin(properties->height, properties->width, 0, 0);
    curs_set(0);
    box(windows->menu, 0, 0);
    nodelay(windows->menu, TRUE);
    keypad(windows->menu, TRUE);
    choose->selected = -1;
    choose->pressed = 0;
}
void DrawChoose(CHOOSE_MENU choose, PROPERTIES *properties, WINDOWS *windows)
{
    int i;

    if (choose.selected == -1)
        wattron(windows->menu, A_STANDOUT);
    mvwprintw(windows->menu, properties->height / 2 - 6, properties->width / 2 - strlen("random") / 2, "%s", "random");
    wstandend(windows->menu);

    for (i = 0; i < properties->nrConfigs; i++)
    {
        if (i == choose.selected)
        {
            wattron(windows->menu, A_STANDOUT);
        }
        mvwprintw(windows->menu, i + properties->height / 2 - 4 + i, properties->width / 2 - strlen(properties->configNames[i]) / 2, "%s", properties->configNames[i]);
        wstandend(windows->menu);
    }
    wrefresh(windows->menu);
}
void UpdateChoose(CHOOSE_MENU *choose, PROPERTIES *properties, WINDOWS *windows)
{
    int input;

    InitialiseChoose(choose, properties, windows);
    while (1)
    {
        input = GetInput(windows->menu);
        switch (input)
        {
        case 'w':
        case KEY_UP:
            if (choose->selected == -1)
            {
                choose->selected = properties->nrConfigs - 1;
            }
            else
            {
                choose->selected--;
            }
            break;
        case 's':
        case KEY_DOWN:
            if (choose->selected == properties->nrConfigs - 1)
            {
                choose->selected = -1;
            }
            else
            {
                choose->selected++;
            }
            break;
        case '\n':
            choose->pressed = 1;
            break;
        case 'q':
            properties->windowType = MENU;
            wclear(windows->menu);
            wrefresh(windows->menu);
            return;
        }
        if (choose->pressed == 1)
        {
            properties->selectedConfig = choose->selected;
            properties->windowType = GAME;
            wclear(windows->menu);
            wrefresh(windows->menu);
            return;
        }
        ResizeWindow(properties, windows->menu);
        DrawChoose(*choose, properties, windows);
    }
}
int CheckNeighbors(CELL map[][10], int i, int j)
{
    int l;
    int dirX[] = {-1, 1, 0, 0, -1, -1, 1, 1};
    int dirY[] = {0, 0, -1, 1, -1, 1, -1, 1};
    for (l = 0; l < 8; l++)
    {
        if (i + dirY[l] < 0 || i + dirY[l] > 9)
        {
            continue;
        }
        if (j + dirX[l] < 0 || j + dirX[l] > 9)
        {
            continue;
        }
        if (map[i + dirY[l]][j + dirX[l]].value == SHIP)
        {
            return 0;
        }
    }
    return 1;
}
int CheckDirection(CELL map[][10], int i, int j, int depth, int dirX, int dirY)
{
    int l;
    for (l = 0; l < depth; l++)
    {
        if (i < 0 || j < 0)
            return 0;
        if (i > 9 || j > 9)
            return 0;
        if (map[i][j].value == SHIP)
            return 0;
        if (!CheckNeighbors(map, i, j))
            return 0;
        i += dirY;
        j += dirX;
    }
    return 1;
}
CELL GetRandomPosition()
{
    CELL cell;
    cell.x = rand() % 10;
    cell.y = rand() % 10;
    return cell;
}
CELL GetEmptyCell(CELL map[][10])
{
    CELL cell;
    do
    {
        cell = GetRandomPosition();
    } while (map[cell.y][cell.x].value == SHIP && map[cell.y][cell.x].hit == 0);
    return cell;
}
void GenerateMap(CELL map[][10])
{
    int nrShips, i, j, nrCells, nrDirs;
    CELL cell;
    int dirX[] = {-1, 0, 0, 1};
    int dirY[] = {0, -1, 1, 0};
    for (i = 0; i < 10; i++)
    {
        for (j = 0; j < 10; j++)
        {
            map[i][j].value = EMPTY;
        }
    }
    for (nrCells = 4; nrCells; nrCells--)
    {
        for (nrShips = 4 - nrCells + 1; nrShips != 0; nrShips--)
        {
            int available = 0;
            do
            {
                cell = GetEmptyCell(map);
                for (nrDirs = 0; nrDirs < 4; nrDirs++)
                {
                    if (CheckDirection(map, cell.y, cell.x, nrCells, dirX[nrDirs], dirY[nrDirs]))
                    {
                        for (i = 0; i < nrCells; i++)
                        {
                            map[cell.y][cell.x].value = SHIP;
                            cell.x += dirX[nrDirs];
                            cell.y += dirY[nrDirs];
                        }
                        available = 1;
                        break;
                    }
                }
            } while (!available);
        }
    }
}
void InitialiseGame(GAME_MENU *game, PROPERTIES *properties, WINDOWS *windows)
{
    int i, j;

    game->pressed = 0;
    game->blink = 0;
    game->turn = 0;
    game->xSelected = 4;
    game->ySelected = 4;
    game->timePassedBlink = 0;
    game->deltaTime = 0;
    game->playerShips = 20;
    game->enemyShips = 20;
    game->input = 0;
    game->timePassedTurn = 0;
    game->foundShip = 0;
    game->playerCombo = 0;
    game->enemyCombo = 0;
    game->playerScore = 0;
    game->enemyScore = 0;
    game->randomize = 0;
    game->destroy = 0;
    game->destroyTime = 0;
    game->randomizeTime = 0;
    game->lastDir = -1;
    if (!properties->resume)
    {
        GenerateMap(game->enemyMap);
        if (properties->selectedConfig == -1)
        {
            GenerateMap(game->playerMap);
        }
        for (i = 0; i < 10; i++)
            for (j = 0; j < 10; j++)
            {
                game->playerMap[i][j].hit = 0;
                game->playerMap[i][j].x = j;
                game->playerMap[i][j].y = i;
                game->enemyMap[i][j].x = j;
                game->enemyMap[i][j].y = i;
                game->enemyMap[i][j].hit = 0;
                game->playerMap[i][j].destroyed = 0;
                game->enemyMap[i][j].destroyed = 0;
                if (properties->selectedConfig != -1)
                {
                    game->playerMap[i][j].value = properties->configs[properties->selectedConfig][i][j];
                }
            }
    }
    else if (properties->resume && properties->foundSave)
    {
        FILE *save = fopen("save", "rb");
        fread(game, sizeof(GAME_MENU), 1, save);
        fclose(save);
        properties->resume = 0;
    }
    curs_set(0);
    windows->player = newwin(21, 21, properties->height / 2 - 10, properties->width / 3 - 10);
    box(windows->player, 0, 0);
    nodelay(windows->player, TRUE);
    windows->enemy = newwin(21, 21, properties->height / 2 - 10, properties->width / 3 + 32);
    box(windows->enemy, 0, 0);
    nodelay(windows->enemy, TRUE);
    keypad(windows->player, TRUE);
    windows->score = newwin(21, 21, properties->height / 2 - 10, properties->width / 3 + 11);
}
void DrawGame(GAME_MENU game, WINDOWS *windows)
{
    int i, j;
    for (i = 2; i < 20; i += 2)
    {
        mvwhline(windows->enemy, i, 1, ACS_HLINE, 19);
        mvwhline(windows->player, i, 1, ACS_HLINE, 19);
    }
    for (i = 2; i < 20; i += 2)
    {
        mvwvline(windows->enemy, 1, i, ACS_VLINE, 19);
        mvwvline(windows->player, 1, i, ACS_VLINE, 19);
    }
    for (i = 1; i < 20; i += 2)
    {
        for (j = 1; j < 20; j += 2)
        {
            if (i / 2 == game.ySelected && j / 2 == game.xSelected)
            {
                if (game.enemyMap[i / 2][j / 2].hit == 1)
                {
                    if (game.enemyMap[i / 2][j / 2].value == SHIP)
                    {
                        if (game.blink == 0)
                        {
                            wattron(windows->enemy, COLOR_PAIR(WHITE_ON_RED));
                        }
                        else
                        {
                            wattron(windows->enemy, COLOR_PAIR(BLACK_ON_RED));
                        }
                    }
                    else
                    {
                        if (game.blink == 0)
                        {
                            wattron(windows->enemy, COLOR_PAIR(RED_ON_RED));
                        }
                        else
                        {
                            wattron(windows->enemy, COLOR_PAIR(WHITE_ON_WHITE));
                        }
                    }
                }
                else
                {
                    if (game.blink == 0)
                    {
                        wattron(windows->enemy, COLOR_PAIR(BLACK_ON_WHITE));
                    }
                    else
                    {
                        wattron(windows->enemy, COLOR_PAIR(WHITE_ON_BLACK));
                    }
                }
            }
            else
            {
                if (game.enemyMap[i / 2][j / 2].hit == 1)
                {

                    wattron(windows->enemy, COLOR_PAIR(BLACK_ON_RED));
                }
            }
            if (game.enemyMap[i / 2][j / 2].hit == 1)
            {
                mvwprintw(windows->enemy, i, j, "%c", game.enemyMap[i / 2][j / 2].value);
            }
            else
            {
                mvwprintw(windows->enemy, i, j, "%c", EMPTY);
            }
            wstandend(windows->enemy);
            if (game.playerMap[i / 2][j / 2].hit == 1)
            {
                wattron(windows->player, COLOR_PAIR(WHITE_ON_RED));
            }
            mvwprintw(windows->player, i, j, "%c", game.playerMap[i / 2][j / 2].value);
            wstandend(windows->player);
        }
    }
    if (game.turn)
    {
        mvwprintw(windows->score, 2, 4, "ENEMY'S TURN");
        mvwprintw(windows->score, 3, 6, "COMBO: %d", game.enemyCombo);
    }
    else
    {
        mvwprintw(windows->score, 2, 6, "YOUR TURN");
        mvwprintw(windows->score, 3, 6, "COMBO: %d", game.playerCombo);
    }
    mvwprintw(windows->score, 18, 2, "PLAYER SCORE: %d", game.playerScore);
    mvwprintw(windows->score, 19, 2, "ENEMY SCORE: %d", game.enemyScore);
    if (game.randomize)
    {
        wattron(windows->score, COLOR_PAIR(WHITE_ON_RED));
    }
    else
    {
        wattroff(windows->score, COLOR_PAIR(WHITE_ON_RED));
    }
    mvwprintw(windows->score, 9, 0, "PRESS R FOR RANDOMIZE");
    if (game.destroy)
    {
        wattron(windows->score, COLOR_PAIR(WHITE_ON_RED));
    }
    else
    {
        wattroff(windows->score, COLOR_PAIR(WHITE_ON_RED));
    }
    mvwprintw(windows->score, 10, 0, "PRESS D FOR DESTROY");
    wattroff(windows->score, COLOR_PAIR(WHITE_ON_RED));
    wrefresh(windows->player);
    wrefresh(windows->enemy);
    wrefresh(windows->score);
}
void HitNearby(CELL map[][10], CELL start)
{
    int l;
    int dirX[] = {-1, 1, 0, 0};
    int dirY[] = {0, 0, -1, 1};
    for (l = 0; l < 4; l++)
    {
        if (start.y + dirY[l] < 0 || start.y + dirY[l] > 9)
        {
            continue;
        }
        if (start.x + dirX[l] < 0 || start.x + dirX[l] > 9)
        {
            continue;
        }
        map[start.y + dirY[l]][start.x + dirX[l]].hit = 1;
    }
}
void UnhitNearby(CELL map[][10], CELL start)
{
    int l;
    int dirX[] = {-1, 1, 0, 0, -1, -1, 1, 1};
    int dirY[] = {0, 0, -1, 1, -1, 1, 1, -1};
    for (l = 0; l < 8; l++)
    {
        if (start.y + dirY[l] < 0 || start.y + dirY[l] > 9)
        {
            continue;
        }
        if (start.x + dirX[l] < 0 || start.x + dirX[l] > 9)
        {
            continue;
        }
        map[start.y + dirY[l]][start.x + dirX[l]].hit = 0;
    }
}
CELL *GetRandomCell(CELL map[][10])
{
    CELL *cell;
    int i = rand() % 10;
    int j = rand() % 10;
    cell = &map[i][j];
    return cell;
}
CELL *GetUnhitCell(CELL map[][10])
{
    CELL *cell;
    do
    {
        cell = GetRandomCell(map);
    } while (cell->hit == 1);
    return cell;
}
int CheckIntegrity(CELL map[][10], int xStart, int yStart, CELL **ships, int *nr, int type)
{
    int dirX[] = {-1, 0, 0, 1};
    int dirY[] = {0, -1, 1, 0};
    int dir;
    int nrShips = 1;
    int nrHit = 0;
    int i;
    int j;

    *ships = malloc(sizeof(CELL));
    (*ships)[nrShips - 1] = map[yStart][xStart];
    if (type == 0)
    {
        if (map[yStart][xStart].hit == 1)
        {
            nrHit++;
        }
    }
    else
    {
        nrHit = 1;
    }
    for (dir = 0; dir < 4; dir++)
    {
        i = yStart + dirY[dir];
        j = xStart + dirX[dir];
        if (CheckBounds(i, j) == 0)
        {
            continue;
        }
        while (map[i][j].value == SHIP)
        {
            nrShips++;
            *ships = realloc(*ships, nrShips * sizeof(CELL));
            (*ships)[nrShips - 1] = map[i][j];
            if (map[i][j].hit == 1)
            {
                nrHit++;
            }
            i += dirY[dir];
            j += dirX[dir];
            if (nrShips == 4)
                break;
            if (i < 0 || i > 9)
                break;
            if (j < 0 || j > 9)
                break;
        }
    }
    *nr = nrShips;
    if (nrShips == nrHit)
        return 1;
    return 0;
}
void DestroyCorners(CELL map[][10], int x, int y)
{
    int dirX[] = {-1, 1, 1, -1};
    int dirY[] = {-1, 1, -1, 1};
    int dir;
    int i;
    int j;
    for (dir = 0; dir < 4; dir++)
    {
        i = y + dirY[dir];
        j = x + dirX[dir];
        if (CheckBounds(i, j) == 0)
        {
            continue;
        }
        map[i][j].hit = 1;
    }
}
int DestroyShips(CELL map[][10], int xStart, int yStart, int type)
{
    if (map[yStart][xStart].destroyed == 0)
    {
        CELL *ships = NULL;
        int nrShips = 0;
        if (CheckIntegrity(map, xStart, yStart, &ships, &nrShips, type))
        {
            int i;
            for (i = 0; i < nrShips; i++)
            {
                ships[i].destroyed = 1;
                HitNearby(map, ships[i]);
            }
            free(ships);
            ships = NULL;
            nrShips = 0;
            return 1;
        }
        free(ships);
        ships = NULL;
        nrShips = 0;
        return 0;
    }
    return 0;
}
void PlayerLogic(GAME_MENU *game, WINDOWS *windows)
{
    switch (game->input)
    {
    case KEY_UP:
        game->ySelected--;
        if (game->ySelected == -1)
        {
            game->ySelected = 9;
        }
        game->timePassedBlink = 0;
        game->blink = 0;
        break;
    case KEY_DOWN:
        game->ySelected++;
        if (game->ySelected == 10)
        {
            game->ySelected = 0;
        }
        game->timePassedBlink = 0;
        game->blink = 0;
        break;
    case KEY_LEFT:
        game->xSelected--;
        if (game->xSelected == -1)
        {
            game->xSelected = 9;
        }
        game->timePassedBlink = 0;
        game->blink = 0;
        break;
    case KEY_RIGHT:
        game->xSelected++;
        if (game->xSelected == 10)
        {
            game->xSelected = 0;
        }
        game->timePassedBlink = 0;
        game->blink = 0;
        break;
    case '\n':
        game->pressed = 1;
        break;
    case 'd':
        game->destroy = 1;
        break;
    case 'r':
        game->randomize = 1;
        break;
    }
    if (game->pressed)
    {
        if (game->enemyMap[game->ySelected][game->xSelected].hit == 0)
        {
            if (game->enemyMap[game->ySelected][game->xSelected].value == SHIP)
            {
                game->playerCombo++;
                game->playerScore += game->playerCombo;
                game->enemyShips--;
                DestroyCorners(game->enemyMap, game->xSelected, game->ySelected);
                DestroyShips(game->enemyMap, game->xSelected, game->ySelected, 1);
            }
            else
            {
                game->playerCombo = 0;
                game->turn = !(game->turn);
                game->blink = 0;
            }
            game->enemyMap[game->ySelected][game->xSelected].hit = 1;
        }
        game->pressed = 0;
    }
    if (game->timePassedBlink >= BLINK_DELAY)
    {
        wclear(windows->score);
        game->blink = !(game->blink);
        game->timePassedBlink = 0;
    }
    game->timePassedBlink += game->deltaTime;
}
CELL *GetNeighborShip(CELL map[][10], CELL start, int *lastDir)
{
    int dirX[] = {-1, 0, 0, 1};
    int dirY[] = {0, -1, 1, 0};
    int dir;
    CELL *cell;
    int i;
    int j;
    if (*lastDir != -1)
    {
        i = start.y + dirY[*lastDir];
        j = start.x + dirX[*lastDir];
        if (CheckBounds(i, j))
        {
            if (map[i][j].hit == 0)
            {
                cell = &map[i][j];
                return cell;
            }
        }
        else
        {
            *lastDir = -1;
            return NULL;
        }
    }
    else
    {
        for (dir = 0; dir < 4; dir++)
        {
            i = start.y + dirY[dir];
            j = start.x + dirX[dir];
            if (CheckBounds(i, j))
            {
                if (map[i][j].hit == 0)
                {
                    cell = &map[i][j];
                    *lastDir = dir;
                    return cell;
                }
            }
            else
            {
                continue;
            }
        }
    }
    *lastDir = -1;
    return NULL;
}
void AILogic(GAME_MENU *game, WINDOWS *windows)
{
    if (game->timePassedTurn >= TURN_DELAY)
    {
        CELL *currentCell;
        if (game->foundShip)
        {
            currentCell = GetNeighborShip(game->playerMap, game->lastCell, &game->lastDir);
            if (currentCell == NULL)
            {
                game->lastCell = game->startCell;
                currentCell = GetNeighborShip(game->playerMap, game->lastCell, &game->lastDir);
            }
        }
        else
        {
            currentCell = GetUnhitCell(game->playerMap);
        }
        if (currentCell->value == SHIP)
        {
            game->enemyCombo++;
            game->enemyScore += game->enemyCombo;
            if (game->foundShip == 0)
            {
                game->startCell = *currentCell;
            }
            game->playerShips--;
            game->lastCell = *currentCell;
            DestroyCorners(game->playerMap, currentCell->x, currentCell->y);
            game->foundShip = !DestroyShips(game->playerMap, currentCell->x, currentCell->y, 1);
        }
        else
        {
            game->enemyCombo = 0;
            wclear(windows->score);
            game->turn = !(game->turn);
            game->lastDir = -1;
        }
        currentCell->hit = 1;
        game->timePassedTurn = 0;
    }
    game->timePassedTurn += game->deltaTime;
}
int VerifyEnd(GAME_MENU *game, PROPERTIES *properties, WINDOWS *windows)
{
    if (game->enemyShips == 0 || game->playerShips == 0)
    {
        FILE *file;
        int dummy, i;

        properties->windowType = END;
        wclear(windows->player);
        wrefresh(windows->player);
        wclear(windows->enemy);
        wrefresh(windows->enemy);
        wclear(windows->score);
        wrefresh(windows->score);
        if (game->enemyShips == game->playerShips)
        {
            properties->won = -1;
        }
        else if (game->enemyShips > game->playerShips)
        {
            properties->won = 0;
        }
        else
        {
            properties->won = 1;
        }
        properties->enemyDestroyed = 20 - game->playerShips;
        properties->playerDestroyed = 20 - game->enemyShips;
        file = fopen("leaderboard", "wb+");
        if (properties->nrScores < 10)
        {
            properties->scores[properties->nrScores++] = game->playerScore;
            BubbleSort(properties->scores, properties->nrScores);
            for (i = 0; i < properties->nrScores; i++)
            {
                fwrite(&properties->scores[i], sizeof(int), 1, file);
            }
        }
        else
        {
            properties->scores[10] = game->playerScore;
            BubbleSort(properties->scores, properties->nrScores + 1);
            for (i = 0; i < properties->nrScores; i++)
            {
                fwrite(&properties->scores[i], sizeof(int), 1, file);
            }
        }
        fclose(file);
        return 1;
    }
    return 0;
}
int DestroyInAdvance(GAME_MENU *game, WINDOWS *windows, PROPERTIES *properties)
{
    int i;
    for (i = 0; i < 10; i++)
    {
        CELL *cell = GetUnhitCell(game->playerMap);
        cell->hit = 1;
        if (cell->value == SHIP)
        {
            game->playerShips--;
            game->enemyScore++;
            DestroyCorners(game->playerMap, cell->x, cell->y);
            DestroyShips(game->playerMap, cell->x, cell->y, 1);
        }
        cell = GetUnhitCell(game->enemyMap);
        cell->hit = 1;
        if (cell->value == SHIP)
        {
            game->enemyShips--;
            game->playerScore++;
            DestroyCorners(game->enemyMap, cell->x, cell->y);
            DestroyShips(game->enemyMap, cell->x, cell->y, 1);
        }
        DrawGame(*game, windows);
        if (VerifyEnd(game, properties, windows))
        {
            return 1;
        }
    }
    return 0;
}
void Randomize(GAME_MENU *game, WINDOWS *windows)
{
    game->foundShip = 0;
    game->lastDir = -1;
    int i, j;
    CELL **undestroyedShips;
    int nrRandomShips = 0;
    int *nrCellsShip;
    for (i = 0; i < 10; i++)
    {
        for (j = 0; j < 10; j++)
        {
            if (game->playerMap[i][j].value == SHIP && game->playerMap[i][j].destroyed == 0)
            {
                int l;
                CELL *ships = NULL;
                int nrShips = 0;
                CheckIntegrity(game->playerMap, j, i, &ships, &nrShips, 0);
                for (l = 0; l < nrShips; l++)
                {
                    game->playerMap[ships[l].y][ships[l].x].value = EMPTY;
                    game->playerMap[ships[l].y][ships[l].x].hit = 0;
                }
                nrRandomShips++;
                if (nrRandomShips == 1)
                {
                    nrCellsShip = malloc(sizeof(int));
                    undestroyedShips = malloc(sizeof(CELL *));
                }
                else
                {
                    nrCellsShip = realloc(nrCellsShip, nrRandomShips * sizeof(int));
                    undestroyedShips = realloc(undestroyedShips, nrRandomShips * sizeof(CELL *));
                }
                nrCellsShip[nrRandomShips - 1] = nrShips;
                undestroyedShips[nrRandomShips - 1] = malloc(nrShips * sizeof(CELL));
                for (l = 0; l < nrShips; l++)
                {
                    undestroyedShips[nrRandomShips - 1][l] = ships[l];
                }
                free(ships);
                //printf("%d\n", nrShips);
                // wrefresh(windows->player);
            }
            else if (game->playerMap[i][j].value == EMPTY)
            {
                game->playerMap[i][j].hit = 0;
            }
        }
    }
    for (i = 0; i < 10; i++)
        for (j = 0; j < 10; j++)
            if (game->playerMap[i][j].value == SHIP)
            {
                DestroyShips(game->playerMap, j, i, 0);
                DestroyCorners(game->playerMap, j, i);
            }
    int nrDirs;
    int dirX[] = {-1, 0, 0, 1};
    int dirY[] = {0, -1, 1, 0};
    for (i = 0; i < nrRandomShips; i++)
    {
        int available = 0;
        do
        {
            CELL cell = GetEmptyCell(game->playerMap);
            for (nrDirs = 0; nrDirs < 4; nrDirs++)
            {
                if (CheckDirection(game->playerMap, cell.y, cell.x, nrCellsShip[i], dirX[nrDirs], dirY[nrDirs]))
                {
                    for (j = 0; j < nrCellsShip[i]; j++)
                    {
                        game->playerMap[cell.y][cell.x] = undestroyedShips[i][j];
                        if (game->playerMap[cell.y][cell.x].hit == 1)
                        {
                            DestroyCorners(game->playerMap, cell.x, cell.y);
                        }

                        cell.x += dirX[nrDirs];
                        cell.y += dirY[nrDirs];
                    }
                    available = 1;
                    break;
                }
            }
        } while (!available);
    }
    free(nrCellsShip);
    for (i = 0; i < nrRandomShips; i++)
    {
        free(undestroyedShips[i]);
    }
    free(undestroyedShips);
    for (i = 0; i < 10; i++)
    {
        for (j = 0; j < 10; j++)
        {
            if (game->playerMap[i][j].value == SHIP)
            {
                game->playerMap[i][j].x = j;
                game->playerMap[i][j].y = i;
            }
        }
    }
    for (i = 0; i < 10; i++)
    {
        for (j = 0; j < 10; j++)
        {
            if (game->playerMap[i][j].value == SHIP)
                if (game->playerMap[i][j].hit == 1)
                    DestroyShips(game->playerMap, j, i, 1);
        }
    }
}
void UpdateGame(GAME_MENU *game, PROPERTIES *properties, WINDOWS *windows)
{
    struct timespec start, stop;

    InitialiseGame(game, properties, windows);
    while (1)
    {
        clock_gettime(CLOCK_REALTIME, &start);
        game->input = GetInput(windows->player);
        if (VerifyEnd(game, properties, windows))
        {
            return;
        }
        if (game->input == 'q')
        {
            FILE *save = fopen("save", "wb+");
            fwrite(game, sizeof(GAME_MENU), 1, save);
            fclose(save);
            wclear(windows->player);
            wclear(windows->enemy);
            wrefresh(windows->player);
            wrefresh(windows->enemy);
            wclear(windows->score);
            wrefresh(windows->score);
            properties->windowType = MENU;
            return;
        }
        if (game->turn == 0)
        {
            if (game->destroy)
            {
                if (game->destroyTime == 0)
                {
                    if (DestroyInAdvance(game, windows, properties))
                    {
                        return;
                    }
                }
                game->destroyTime += game->deltaTime;
                if (game->deltaTime == 0 && game->destroyTime == 0)
                {
                    game->destroyTime = 0.000001f;
                }
            }
            if (game->destroyTime >= DESTROYCOOLDOWN)
            {
                game->destroyTime = 0;
                game->destroy = 0;
            }
            if (game->randomize)
            {
                if (game->randomizeTime == 0)
                {
                    Randomize(game, windows);
                }
                game->randomizeTime += game->deltaTime;
                if (game->deltaTime == 0 && game->randomizeTime == 0)
                {
                    game->randomizeTime = 0.000001f;
                }
            }
            if (game->randomizeTime >= RANDOMIZECOOLDOWN)
            {
                game->randomizeTime = 0;
                game->randomize = 0;
            }
            PlayerLogic(game, windows);
        }
        else
        {
            AILogic(game, windows);
        }
        if (properties->width != getmaxx(stdscr))
        {
            wclear(windows->player);
            wclear(windows->enemy);
            wclear(windows->score);
            wrefresh(windows->player);
            wrefresh(windows->enemy);
            wrefresh(windows->score);
            delwin(windows->player);
            delwin(windows->enemy);
            delwin(windows->score);
            properties->width = getmaxx(stdscr);
            windows->player = newwin(21, 21, properties->height / 2 - 10, properties->width / 3 - 10);
            box(windows->player, 0, 0);
            nodelay(windows->player, TRUE);
            windows->enemy = newwin(21, 21, properties->height / 2 - 10, properties->width / 3 + 32);
            box(windows->enemy, 0, 0);
            nodelay(windows->enemy, TRUE);
            keypad(windows->player, TRUE);
            windows->score = newwin(21, 21, properties->height / 2 - 10, properties->width / 3 + 11);
        }
        if (properties->height != getmaxy(stdscr))
        {
            wclear(windows->player);
            wclear(windows->enemy);
            wclear(windows->score);
            wrefresh(windows->player);
            wrefresh(windows->enemy);
            wrefresh(windows->score);
            delwin(windows->player);
            delwin(windows->enemy);
            delwin(windows->score);
            properties->height = getmaxy(stdscr);
            windows->player = newwin(21, 21, properties->height / 2 - 10, properties->width / 3 - 10);
            box(windows->player, 0, 0);
            nodelay(windows->player, TRUE);
            windows->enemy = newwin(21, 21, properties->height / 2 - 10, properties->width / 3 + 32);
            box(windows->enemy, 0, 0);
            nodelay(windows->enemy, TRUE);
            keypad(windows->player, TRUE);
            windows->score = newwin(21, 21, properties->height / 2 - 10, properties->width / 3 + 11);
        }
        DrawGame(*game, windows);
        clock_gettime(CLOCK_REALTIME, &stop);
        game->deltaTime = (float)(stop.tv_nsec - start.tv_nsec) / CLOCKS_PER_SEC / 1000;

        if (game->deltaTime < 0)
        {
            game->deltaTime = 0;
        }
    }
}
void InitialiseEnd(PROPERTIES *properties, WINDOWS *windows)
{
    windows->menu = newwin(properties->height, properties->width, 0, 0);
    box(windows->menu, 0, 0);
    keypad(windows->menu, TRUE);
    nodelay(windows->menu, TRUE);
}
void DrawEnd(PROPERTIES *properties, WINDOWS *windows)
{
    if (properties->won == -1)
    {
        mvwprintw(windows->menu, properties->height / 2, properties->width / 2 - 1, "%s", "TIE!");
        mvwprintw(windows->menu, properties->height / 2 + 3, properties->width / 2 - 12, "Player destroyed: %d ships", properties->playerDestroyed);
        mvwprintw(windows->menu, properties->height / 2 + 4, properties->width / 2 - 12, "Computer destroyed: %d ships", properties->enemyDestroyed);
    }
    else if (properties->won == 0)
    {
        mvwprintw(windows->menu, properties->height / 2, properties->width / 2 - 3, "%s", "PC WON!");
        mvwprintw(windows->menu, properties->height / 2 + 3, properties->width / 2 - 12, "Player destroyed: %d ships", properties->playerDestroyed);
        mvwprintw(windows->menu, properties->height / 2 + 4, properties->width / 2 - 12, "Computer destroyed: %d ships", properties->enemyDestroyed);
    }
    else
    {
        mvwprintw(windows->menu, properties->height / 2, properties->width / 2 - 12, "%s", "YOU WON! CONGRATULATIONS!");
        mvwprintw(windows->menu, properties->height / 2 + 3, properties->width / 2 - 12, "Player destroyed: %d ships", properties->playerDestroyed);
        mvwprintw(windows->menu, properties->height / 2 + 4, properties->width / 2 - 12, "Computer destroyed: %d ships", properties->enemyDestroyed);
    }
    wrefresh(windows->menu);
}
void UpdateEnd(END_MENU *end, PROPERTIES *properties, WINDOWS *windows)
{
    int input;

    InitialiseEnd(properties, windows);
    while (1)
    {
        ResizeWindow(properties, windows->menu);
        DrawEnd(properties, windows);
        input = GetInput(windows->menu);
        if (input == 'q' || input == '\n')
        {
            properties->windowType = MENU;
            wclear(windows->menu);
            wrefresh(windows->menu);
            return;
        }
    }
}
void InitialiseLeaderboard(WINDOWS *windows, PROPERTIES *properties)
{
    windows->menu = newwin(properties->height, properties->width, 0, 0);
    box(windows->menu, 0, 0);
    keypad(windows->menu, TRUE);
    nodelay(windows->menu, TRUE);
}
void DrawLeaderboard(PROPERTIES *properties, WINDOWS *windows)
{
    int i;

    for (i = 0; i < properties->nrScores; i++)
    {
        mvwprintw(windows->menu, i + 2, properties->width / 2, "%d. %d", i + 1, properties->scores[i]);
    }
}
void UpdateLeaderboard(PROPERTIES *properties, WINDOWS *windows)
{
    int input;

    InitialiseLeaderboard(windows, properties);
    while (1)
    {
        ResizeWindow(properties, windows->menu);
        DrawLeaderboard(properties, windows);
        input = GetInput(windows->menu);
        if (input == 'q' || input == '\n')
        {
            properties->windowType = MENU;
            wclear(windows->menu);
            wrefresh(windows->menu);
            return;
        }
    }
}
void DeallocateMemory(PROPERTIES *properties)
{
    int i, j;
    for (i = 0; i < properties->nrConfigs; i++)
    {

        for (j = 0; j < 10; j++)
        {
            free(properties->configs[i][j]);
        }
        free(properties->configs[i]);
    }
    free(properties->configs);
    endwin();
}
int main(int argc, char *argv[])
{
    PROPERTIES properties;
    MAIN_MENU *menu;
    GAME_MENU *game;
    CHOOSE_MENU *choose;
    END_MENU *end;
    FILE *config;
    WINDOWS windows;
    char *menus[] = {"NEW GAME", "RESUME GAME", "LEADERBOARD", "QUIT"};
    int i, j, k;
    int error = 0;

    if (argc == 1)
        return 1;
    Initialise(&properties);
    for (i = 1; i < argc; i++)
    {
        if (!(config = fopen(argv[i], "r")))
        {
            printf("[Eroare]: Fisierul %s nu poate fi deschis\n", argv[i]);
            error = 1;
        }
        else
        {
            properties.nrConfigs++;
            fclose(config);
        }
    }
    if (error)
    {
        endwin();
        return 1;
    }
    properties.configs = malloc(properties.nrConfigs * sizeof(char **));
    properties.configNames = argv + 1;
    for (i = 0; i < properties.nrConfigs; i++)
    {
        config = fopen(argv[i + 1], "r");
        properties.configs[i] = malloc(10 * sizeof(char *));
        char dummy[23];
        for (j = 0; j < 10; j++)
        {
            properties.configs[i][j] = malloc(10 * sizeof(char));
            int l = 0;
            fgets(dummy, 23, config);
            for (k = 0; dummy[k]; k++)
            {
                if (dummy[k] == ' ' || dummy[k] == 'X')
                {
                    properties.configs[i][j][l++] = dummy[k];
                }
            }
        }
        fclose(config);
    }
    while (!(properties.gameQuit))
    {
        switch (properties.windowType)
        {
        case MENU:
            menu = malloc(sizeof(MAIN_MENU));
            UpdateMenu(menu, menus, &properties, &windows);
            delwin(windows.menu);
            free(menu);
            break;
        case CHOOSE:
            choose = malloc(sizeof(CHOOSE_MENU));
            UpdateChoose(choose, &properties, &windows);
            delwin(windows.menu);
            free(choose);
            break;
        case GAME:
            game = malloc(sizeof(GAME_MENU));
            memset(game, 0, sizeof(GAME_MENU));
            UpdateGame(game, &properties, &windows);
            delwin(windows.enemy);
            delwin(windows.player);
            delwin(windows.score);
            free(game);
            break;
        case END:
            end = malloc(sizeof(END_MENU));
            UpdateEnd(end, &properties, &windows);
            delwin(windows.menu);
            free(end);
            break;
        case LEADERBOARD:
            UpdateLeaderboard(&properties, &windows);
            delwin(windows.menu);
            break;
        }
    }
    DeallocateMemory(&properties);
    return 0;
}