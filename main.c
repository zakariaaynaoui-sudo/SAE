/* ============================================================================
 * TP5 - IHM non bloquante (FSM) + taches telerupteurs
 * Cible : PIC 16F18877
 * ============================================================================
 * Ce programme illustre deux concepts :
 *   1. Une IHM (interface serie via PuTTY) refactorisee d'un algorithme
 *      bloquant (TP4) vers une machine a etats finis (FSM) non bloquante.
 *   2. L'execution sequentielle de plusieurs FSM dans la boucle principale
 *      donne l'illusion d'un fonctionnement multitache.
 *
 * Trois taches s'executent "en parallele" :
 *   - IHM()       : operation arithmetique entre 2 nombres via PuTTY
 *   - telerupt1() : commande de la LED D2 par le bouton S1
 *   - telerupt2() : commande de la LED D3 par le bouton S2
 * ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include "mcc_generated_files/mcc.h"

/* ---------------------------------------------------------------------------
 * Alias des E/S (a adapter si MCC genere d'autres noms)
 *   - Boutons :  S1 = RB4, S2 = RB5  (actifs bas avec pull-up)
 *   - LEDs    :  D2 = RA4, D3 = RA5
 * ------------------------------------------------------------------------- */
#define BP_S1        (PORTBbits.RB4)
#define BP_S2        (PORTBbits.RB5)
#define LED_D2       (LATAbits.LATA4)
#define LED_D3       (LATAbits.LATA5)

#define BP_ENFONCE   0
#define BP_RELACHE   1
#define LED_ALLUMEE  1
#define LED_ETEINTE  0


/* ===========================================================================
 *  1) TACHE IHM : operation arithmetique entre 2 nombres (FSM 5 etats)
 * ===========================================================================
 *
 *                       +---------------+
 *                       |  ETAT_INIT    |   <-- etat d'amorcage
 *                       +---------------+
 *                              |
 *                              | (transition inconditionnelle)
 *                              | / effacer ecran, afficher titre,
 *                              |   demander 1er nombre
 *                              v
 *                       +---------------+
 *                       |  ATTENTE_NB1  |
 *                       +---------------+
 *                              |
 *                              | EUSART_Read_Str(nombre) == true
 *                              | / nb1 = atol(nombre);
 *                              |   demander operateur
 *                              v
 *                       +---------------+
 *                       |  ATTENTE_OP   |<--+ (car invalide : on re-demande)
 *                       +---------------+   |
 *                              |            |
 *                              | car E {'+','-','*','/'}
 *                              | / op = car; demander 2eme nombre
 *                              v
 *                       +---------------+
 *                       |  ATTENTE_NB2  |
 *                       +---------------+
 *                              |
 *                              | EUSART_Read_Str(nombre) == true
 *                              | / nb2 = atol(nombre); calculer res;
 *                              |   afficher "nb1 op nb2 = res";
 *                              |   demander appui sur barre espace
 *                              v
 *                       +-----------------+
 *                       | ATTENTE_ESPACE  |
 *                       +-----------------+
 *                              |
 *                              | car recu == ' '
 *                              v
 *                       retour a ETAT_INIT
 * ---------------------------------------------------------------------------
 * REGLE D'OR (cf. enonce TP) : les printf des messages d'invitation sont
 * places dans la TRANSITION qui amene a l'etat d'attente, et JAMAIS dans
 * l'etat lui-meme, sinon le message serait reaffiche en permanence.
 * ---------------------------------------------------------------------------
 */

typedef enum {
    ETAT_INIT,
    ATTENTE_NB1,
    ATTENTE_OP,
    ATTENTE_NB2,
    ATTENTE_ESPACE
} etat_IHM_t;

void IHM(void)
{
    static etat_IHM_t etat = ETAT_INIT;
    static char       nombre[64];
    static int32_t    nb1, nb2, res;
    static char       op;
    char              c;

    switch (etat)
    {
        case ETAT_INIT:
            // ACTION de la transition vers ATTENTE_NB1
            printf("\x1B[2J"); printf("\x1B[H");                 // efface ecran
            printf(" ****** Operation entre 2 nombres ********\n\r");
            printf("Donner un premier nombre:\n");
            etat = ATTENTE_NB1;
            break;

        case ATTENTE_NB1:
            // EUSART_Read_Str() est deja non bloquante : elle renvoie false
            // tant que la chaine n'est pas complete, true quand Enter est tape.
            if (EUSART_Read_Str(nombre) == true)
            {
                nb1 = atol(nombre);
                // ACTION de la transition vers ATTENTE_OP
                printf("Donner l'operateur:\n");
                etat = ATTENTE_OP;
            }
            break;

        case ATTENTE_OP:
            // Lecture d'UN caractere seulement s'il est dispo (non bloquant)
            if (EUSART_is_rx_ready())
            {
                c = EUSART_Read();
                if (c == '+' || c == '-' || c == '*' || c == '/')
                {
                    op = c;
                    putch(13);
                    // ACTION de la transition vers ATTENTE_NB2
                    printf("Donner un deuxieme nombre:\n");
                    etat = ATTENTE_NB2;
                }
                else
                {
                    // caractere invalide : on redemande sans changer d'etat
                    printf("Donner l'operateur:\n");
                }
            }
            break;

        case ATTENTE_NB2:
            if (EUSART_Read_Str(nombre) == true)
            {
                nb2 = atol(nombre);

                // Calcul du resultat
                switch (op)
                {
                    case '+': res = nb1 + nb2; break;
                    case '-': res = nb1 - nb2; break;
                    case '*': res = nb1 * nb2; break;
                    case '/':
                        if (nb2 != 0) res = nb1 / nb2;
                        else printf("Operation impossible !!! (division par zero)\n");
                        break;
                }
                printf("%ld %c %ld = %ld\n", nb1, op, nb2, res);

                // ACTION de la transition vers ATTENTE_ESPACE
                putch(13);
                printf("Appuyer sur barre espace pour une nouvelle operation\n");
                etat = ATTENTE_ESPACE;
            }
            break;

        case ATTENTE_ESPACE:
            if (EUSART_is_rx_ready())
            {
                c = EUSART_Read();
                if (c == ' ')
                {
                    etat = ETAT_INIT;       // on reboucle pour une nouvelle operation
                }
            }
            break;
    }
}


/* ===========================================================================
 *  2) TACHES TELERUPTEUR : commande LED par bouton poussoir (FSM 5 etats)
 * ===========================================================================
 *                           Reset uC
 *                              |
 *                              | / Eteindre LED
 *                              v
 *                      +---------------+
 *                      |  Etat initial |
 *                      +---------------+
 *                              | BP relache  (securite : pas de BP tenu au boot)
 *                              v
 *               +-----------------------------+   BP relache
 *               | Attente appui BP pr eclair  |<------+
 *               +-----------------------------+-------+ (self-loop implicite)
 *                              | BP enfonce
 *                              | / Allumer LED
 *                              v
 *               +-----------------------------+
 *               | Attente relache BP apr ecl  |
 *               +-----------------------------+
 *                              | BP relache
 *                              v
 *               +-----------------------------+  BP relache
 *               | Attente appui BP pr eteind  |<------+
 *               +-----------------------------+-------+
 *                              | BP enfonce
 *                              | / Eteindre LED
 *                              v
 *               +-----------------------------+
 *               | Attente relache BP apr ext  |
 *               +-----------------------------+
 *                              | BP relache
 *                              |
 *                              +---> retour a "Attente appui BP pr eclairer"
 * ---------------------------------------------------------------------------
 * Les etats "Attente relachement" sont indispensables : sans eux, tant que
 * l'utilisateur garde le doigt sur le bouton, la LED clignoterait sans cesse.
 * ---------------------------------------------------------------------------
 */

typedef enum {
    TR_INIT,
    TR_ATTENTE_APPUI_ECLAIRER,
    TR_ATTENTE_RELACHE_APRES_ECLAIRAGE,
    TR_ATTENTE_APPUI_ETEINDRE,
    TR_ATTENTE_RELACHE_APRES_EXTINCTION
} etat_telerupt_t;

void telerupt1(void)
{
    static etat_telerupt_t etat = TR_INIT;

    switch (etat)
    {
        case TR_INIT:
            LED_D2 = LED_ETEINTE;                       // action : eteindre LED
            if (BP_S1 == BP_RELACHE)
                etat = TR_ATTENTE_APPUI_ECLAIRER;
            break;

        case TR_ATTENTE_APPUI_ECLAIRER:
            if (BP_S1 == BP_ENFONCE)
            {
                LED_D2 = LED_ALLUMEE;                   // action de transition
                etat = TR_ATTENTE_RELACHE_APRES_ECLAIRAGE;
            }
            break;

        case TR_ATTENTE_RELACHE_APRES_ECLAIRAGE:
            if (BP_S1 == BP_RELACHE)
                etat = TR_ATTENTE_APPUI_ETEINDRE;
            break;

        case TR_ATTENTE_APPUI_ETEINDRE:
            if (BP_S1 == BP_ENFONCE)
            {
                LED_D2 = LED_ETEINTE;                   // action de transition
                etat = TR_ATTENTE_RELACHE_APRES_EXTINCTION;
            }
            break;

        case TR_ATTENTE_RELACHE_APRES_EXTINCTION:
            if (BP_S1 == BP_RELACHE)
                etat = TR_ATTENTE_APPUI_ECLAIRER;       // boucle
            break;
    }
}

void telerupt2(void)
{
    // Copie de telerupt1() adaptee : LED D3 et bouton S2.
    static etat_telerupt_t etat = TR_INIT;

    switch (etat)
    {
        case TR_INIT:
            LED_D3 = LED_ETEINTE;
            if (BP_S2 == BP_RELACHE)
                etat = TR_ATTENTE_APPUI_ECLAIRER;
            break;

        case TR_ATTENTE_APPUI_ECLAIRER:
            if (BP_S2 == BP_ENFONCE)
            {
                LED_D3 = LED_ALLUMEE;
                etat = TR_ATTENTE_RELACHE_APRES_ECLAIRAGE;
            }
            break;

        case TR_ATTENTE_RELACHE_APRES_ECLAIRAGE:
            if (BP_S2 == BP_RELACHE)
                etat = TR_ATTENTE_APPUI_ETEINDRE;
            break;

        case TR_ATTENTE_APPUI_ETEINDRE:
            if (BP_S2 == BP_ENFONCE)
            {
                LED_D3 = LED_ETEINTE;
                etat = TR_ATTENTE_RELACHE_APRES_EXTINCTION;
            }
            break;

        case TR_ATTENTE_RELACHE_APRES_EXTINCTION:
            if (BP_S2 == BP_RELACHE)
                etat = TR_ATTENTE_APPUI_ECLAIRER;
            break;
    }
}


/* ===========================================================================
 *  3) BOUCLE PRINCIPALE
 * ===========================================================================
 * Les trois fonctions sont appelees sequentiellement a chaque tour de boucle.
 * Comme chacune est non bloquante (elle execute uniquement le code de l'etat
 * courant puis rend la main), le microcontroleur traite "en parallele" les
 * trois taches. L'IHM continue de tourner pendant que l'utilisateur appuie
 * sur S1/S2 pour piloter les LEDs.
 * ---------------------------------------------------------------------------
 */
void main(void)
{
    SYSTEM_Initialize();

    while (1)
    {
        IHM();
        telerupt1();
        telerupt2();
    }
}
