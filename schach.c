//
// Created by Anja Klosterhuber on 22.01.24.
//
#include <stdlib.h>
#include "schach.h"
#include <stdbool.h>
#include "list.h"
#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <limits.h>

void print_brett(void* schachbrett_pointer);

void bauerzuege(BRETT* schachbrett, int x, int y, int player, LIST* folgezustaende);
void laeuferzuege(BRETT* schachbrett, int x, int y, int player, LIST* folgezustaende);
void springerzuege(BRETT* schachbrett, int x, int y, int player, LIST* folgezustaende);
void turmzuege(BRETT* schachbrett, int x, int y, int player, LIST* folgezustaende);
void damezuege(BRETT* schachbrett, int x, int y, int player, LIST* folgezustaende);
void koenigzuege(BRETT* schachbrett, int x, int y, int player, LIST* folgezustaende);

void initialisieren(BRETT schachbrett) {
    memcpy(schachbrett, initiales_brett, sizeof(BRETT));
}

BRETT* brett_cpy(BRETT* in) {
    BRETT* out = calloc(1, sizeof(BRETT));
    memcpy(out, in, sizeof(BRETT));
    return out;
}

int brett_cmp(void* a, void* b) {
    BRETT* brett_a = (BRETT*) a;
    BRETT* brett_b = (BRETT*) b;
    for (int x = 0; x < GROESSE; x++) {
        for (int y = 0; y < GROESSE; y++) {
            int diff = (*brett_a)[y][x] - (*brett_b)[y][x];
            if (diff != 0)
                return diff;
        }
    }
    return 0;
}

bool brett_eql(void* a, void* b) {
    return (bool) brett_cmp(a, b);
}

bool checkoutofbounds(int x, int y) {
    return x >= GROESSE || y >= GROESSE || x < 0 || y < 0;
}

int schach_next_player(int player) {
    return -1 * player;
}


zuege_der_figur* figuren_regeln[6] = {
        [0] = &bauerzuege, 
        [1] = &laeuferzuege,
        [2] = &springerzuege,
        [3] = &turmzuege,
        [4] = &damezuege,
        [5] = &koenigzuege
};

LIST* schach_nachfolgezustaende(BRETT* schachbrett, int player) {
    LIST* folgezustaende = list_new();
    for (int y= 0; y < GROESSE; y++) {
        for (int x = 0; x < GROESSE; x++) {
            int aktuelle_figur = (*schachbrett)[y][x] * player;
            if (aktuelle_figur > 0) {
                (*figuren_regeln[aktuelle_figur])(schachbrett, x, y, player, folgezustaende);
            }
        }
    }
    return folgezustaende;
}

bool schach_matt(BRETT* schachbrett, int player) {
    for (int x = 0; x < GROESSE; x++) {
        for (int y = 0; y < GROESSE; y++) {
            if ((*schachbrett)[y][x] == player * KOENIG)
                return false; // wir haben den König noch!
        }
    }
    return true; // keinen eigenen König gefunden → Schach matt!
}

int schach_zustandsbewertung(BRETT* schachbrett, int player) {
    int bewertung;
    if (DEBUG) {
        printf("zbf:\n");
        print_brett(schachbrett);
    }
    if (schach_matt(schachbrett, player))
        bewertung = -player * (INT_MAX - 3); // INT_MIN wäre gefährlich, INT_MAX benutzt minimax intern
    else if (schach_matt(schachbrett, -player))
        bewertung = player * (INT_MAX - 3);
    else {
        // TODO/FIXME: im Moment ist alles was nicht matt ist gleich gut...
        bewertung = 0;
    }
    if (DEBUG)
        printf("bewertung ist %d.\n", bewertung);
    return bewertung;
}

void strahlzuege(BRETT* schachbrett, int x, int y, int dx, int dy, int player, LIST* folgezustaende) {
    assert(player == WHITE || player == BLACK);
    int MAX_STEPS = GROESSE - 1;
    int directions[4][2] = {{-1, -1}, {1, -1}, {1, 1}, {-1, 1}};
    int i = 1;
    bool geschlagen = false;
    while (i < MAX_STEPS &&
           !geschlagen &&
           !checkoutofbounds(y + dy*i, x + dx*i) &&
           ((*schachbrett)[y + dy*i][x + dx*i] * player) <= 0)
    {
        BRETT *copy = brett_cpy(schachbrett);
        if ((*schachbrett)[y + dx*i][x + dy*i] * -player > 0)
            geschlagen = true;
        (*copy)[y][x] = 0;
        (*copy)[y + dx*i][x + dy*i] = LAEUFER * player;
        list_append(folgezustaende, copy);
        i++;
    }
}

void laeuferzuege(BRETT* schachbrett, int x, int y, int player, LIST* folgezustaende) {
    assert((*schachbrett)[y][x] == LAEUFER * player && "Kein Laeufer auf der angegebenen Position.");
    strahlzuege(schachbrett, x, y, 1, 1, player, folgezustaende);  // (x+i, y+i)
    strahlzuege(schachbrett, x, y, 1, -1, player, folgezustaende); // (x+i, y-i)
    strahlzuege(schachbrett, x, y, -1, 1, player, folgezustaende); // (x-i, y+i)
    strahlzuege(schachbrett, x, y, -1, -1, player, folgezustaende); // (x-i, y-i)
}

void turmzuege(BRETT* schachbrett, int x, int y, int player, LIST* folgezustaende) {
    assert((*schachbrett)[y][x] == TURM * player && "Kein Turm auf der angegebenen Position.");
    strahlzuege(schachbrett, x, y, 1, 0, player, folgezustaende);
    strahlzuege(schachbrett, x, y, -1, 0, player, folgezustaende);
    strahlzuege(schachbrett, x, y, 0, 1, player, folgezustaende);
    strahlzuege(schachbrett, x, y, 0, -1, player, folgezustaende);
}

void damenzuege(BRETT* schachbrett, int x, int y, int player, LIST* folgezustaende) {
    assert((*schachbrett)[y][x] == DAME * player && "Keine Dame auf der angegebenen Position.");
    laeuferzuege(schachbrett,  x,  y, player, folgezustaende);
    turmzuege( schachbrett,  x,  y, player, folgezustaende);
}

void feste_Zuege(BRETT* schachbrett, int x, int y, int dx, int dy, int player, LIST* folgezustaende) {
    if (!checkoutofbounds(x + dx, y + dy) &&
        ((*schachbrett)[y + dx][x - dx] * player) <= 0)
    {
        BRETT *copy = brett_cpy(schachbrett);
        (*copy)[y][x] = 0;
        (*copy)[y + dx][x + dy] = LAEUFER * player;
        list_append(folgezustaende, copy);
    }
}

void koenigzug(BRETT* schachbrett, int x, int y, int player, LIST* folgezustaende) {
    //Läufer
    feste_Zuege(schachbrett, x, y, 1, 1, player, folgezustaende);
    feste_Zuege(schachbrett, x, y, 1, -1, player, folgezustaende);
    feste_Zuege(schachbrett, x, y, -1, 1, player, folgezustaende);
    feste_Zuege(schachbrett, x, y, -1, -1, player, folgezustaende);
    //Turm
    feste_Zuege(schachbrett, x, y, 1, 0, player, folgezustaende);
    feste_Zuege(schachbrett, x, y, -1, 0, player, folgezustaende);
    feste_Zuege(schachbrett, x, y, 0, 1, player, folgezustaende);
    feste_Zuege(schachbrett, x, y, 0, -1, player, folgezustaende);
}

void springerzug(BRETT* schachbrett, int x, int y, int player, LIST* folgezustaende) {
    feste_Zuege(schachbrett, x, y, 1, 2, player, folgezustaende);
    feste_Zuege(schachbrett, x, y, -1, 2, player, folgezustaende);
    feste_Zuege(schachbrett, x, y, 1, -2, player, folgezustaende);
    feste_Zuege(schachbrett, x, y, -1, -2, player, folgezustaende);

    feste_Zuege(schachbrett, x, y, 2, 1, player, folgezustaende);
    feste_Zuege(schachbrett, x, y, -2, 1, player, folgezustaende);
    feste_Zuege(schachbrett, x, y, 2, -1, player, folgezustaende);
    feste_Zuege(schachbrett, x, y, -2, -1, player, folgezustaende);


}

void bauerzuege(BRETT* schachbrett, int x, int y, int player, LIST* folgezustaende) {
    if((player == 1 && y == 6) || (player == -1 && y == 2)) {
        feste_Zuege(schachbrett, x, y, 0, 2, player, folgezustaende);
        feste_Zuege(schachbrett, x, y, 0, 1, player, folgezustaende);
    }
    if((schachbrett[x-1][y-1] < 0 && player == 1) || (schachbrett[x+1][y-1] < 0 && player == 1)) {
        feste_Zuege(schachbrett, x, y, -1, -1, player, folgezustaende);
        feste_Zuege(schachbrett, x, y, 1, -1, player, folgezustaende);
    }
    else if((schachbrett[x-1][y+1] > 0 && player == -1)  || (schachbrett[x+1][y+1] > 0 && player == -1)) {
        feste_Zuege(schachbrett, x, y, -1, 1, player, folgezustaende);
        feste_Zuege(schachbrett, x, y, 1, 1, player, folgezustaende);
    }

    feste_Zuege(schachbrett, x, y, 0, 1, player, folgezustaende);
}

void print_brett(void* schachbrett_pointer) {
    BRETT* schachbrett = (BRETT*) schachbrett_pointer;
    printf("\n  ");
    for (int x=0; x < GROESSE; x++){
        printf(" %c", 'A' + x);
    }
    printf("\n");
    for (int x=0; x < GROESSE + 1; x++){
        printf("__");
    }
    printf("\n");
    for (int y=0; y < GROESSE; y++) {// i= spalten=y
        printf("%d|", y);
        for (int x = 0; x < GROESSE; x++) { //k=zeilen=x
            if((*schachbrett)[y][x]>=0) {
                printf(" ");
            }
            printf("%d", (*schachbrett)[y][x]);
        }
        printf("\n");
    }
}
