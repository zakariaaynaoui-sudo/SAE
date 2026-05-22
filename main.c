#include "mcc_generated_files/mcc.h"
#include "mcc_generated_files/pin_manager.h"
#include "mcc_generated_files/adcc.h"
#include "ILI9341.h"
#include "GFX_Library.h"
#include <math.h>

#define ILI9341_ORANGE 0xFD20

#define PI 3.14159f

// Pages : 0=menu  1=jeu  2=score
int page     = 0;
int menu_sel = 0;

// LEDs
#define RL_ON()  do { LATEbits.LATE0 = 1; } while(0)
#define RL_OFF() do { LATEbits.LATE0 = 0; } while(0)
#define GL_ON()  do { LATEbits.LATE1 = 1; } while(0)
#define GL_OFF() do { LATEbits.LATE1 = 0; } while(0)
#define BL_ON()  do { LATEbits.LATE2 = 1; } while(0)
#define BL_OFF() do { LATEbits.LATE2 = 0; } while(0)

int blink_cnt = 0;
int blink_idx = 0;

// 8 directions unitaires (x100) : 0=haut, sens horaire
const int ANG_DX[8] = {   0,  71, 100,  71,   0, -71, -100, -71 };
const int ANG_DY[8] = { -100, -71,   0,  71, 100,  71,    0, -71 };

// Palette vaisseau : orange
uint16_t pal[3] = { ILI9341_ORANGE, 0xF81F, ILI9341_CYAN };

// ----------------------------------------------------------------
// Convertit les axes joystick en heading 0-7 via atan2.
// atan2f(jx, -jy) donne l'angle depuis le Nord, sens horaire.
// On divise le cercle en 8 secteurs de 45 deg chacun.
static int joy_to_heading(int jx, int jy)
{
    float angle = atan2f((float)jx, -(float)jy);
    // angle : -PI..PI, 0 = haut (nord), positif = sens horaire
    if(angle < 0.0f) angle += 2.0f * PI;
    // angle : 0..2PI
    // +PI/8 pour centrer les secteurs, puis /( PI/4) pour avoir 0..8
    int h = (int)((angle + PI / 8.0f) / (PI / 4.0f)) % 8;
    return h;
}

// ----------------------------------------------------------------
void cycle_leds(void)
{
    blink_cnt++;
    if(blink_cnt >= 5)
    {
        blink_cnt = 0;
        RL_OFF(); GL_OFF(); BL_OFF();
        if(blink_idx == 0) RL_ON();
        if(blink_idx == 1) BL_ON();
        if(blink_idx == 2) GL_ON();
        if(++blink_idx > 2) blink_idx = 0;
    }
}

// ----------------------------------------------------------------
// Vaisseau fleche decoratif (menu)
void draw_ship(int cx, int cy, uint16_t col)
{
    display_drawLine(cx,   cy-14, cx-8, cy+8, col);
    display_drawLine(cx,   cy-14, cx+8, cy+8, col);
    display_drawLine(cx-5, cy+2,  cx+5, cy+2, col);
    display_drawLine(cx-8, cy+8,  cx+8, cy+8, col);
}

// ----------------------------------------------------------------
// Triangle oriente selon heading 0-7 (en jeu)
void draw_ship_angle(int cx, int cy, int ang, uint16_t col)
{
    int fx = cx + ANG_DX[ang]          * 15 / 100;
    int fy = cy + ANG_DY[ang]          * 15 / 100;
    int lx = cx + ANG_DX[(ang+3) % 8] * 10 / 100;
    int ly = cy + ANG_DY[(ang+3) % 8] * 10 / 100;
    int rx = cx + ANG_DX[(ang+5) % 8] * 10 / 100;
    int ry = cy + ANG_DY[(ang+5) % 8] * 10 / 100;
    display_drawLine(fx, fy, lx, ly, col);
    display_drawLine(fx, fy, rx, ry, col);
    display_drawLine(lx, ly, rx, ry, col);
}

// ----------------------------------------------------------------
void draw_stars(void)
{
    display_fillCircle(20,   18, 2, ILI9341_WHITE);
    display_fillCircle(285,  12, 2, ILI9341_WHITE);
    display_fillCircle(55,   50, 1, ILI9341_WHITE);
    display_fillCircle(255,  58, 1, 0xF81F);
    display_fillCircle(155,  32, 1, ILI9341_WHITE);
    display_fillCircle(18,   72, 1, 0xF81F);
    display_fillCircle(300,  48, 1, ILI9341_CYAN);
    display_fillCircle(95,   22, 1, ILI9341_WHITE);
    display_fillCircle(235,  38, 1, 0xF81F);
    display_fillCircle(308,  88, 1, ILI9341_WHITE);
    display_fillCircle(8,  105,  1, ILI9341_WHITE);
    display_fillCircle(195,  22, 1, ILI9341_CYAN);
    display_fillCircle(130, 140, 1, ILI9341_WHITE);
    display_fillCircle(270, 160, 1, 0xF81F);
    display_fillCircle(45,  175, 1, ILI9341_CYAN);
    display_fillCircle(180,  90, 1, ILI9341_WHITE);
    display_fillCircle(310, 130, 1, ILI9341_WHITE);
    display_fillCircle(75,  200, 1, 0xF81F);
    display_fillCircle(340,  55, 1, ILI9341_WHITE);
}

// ----------------------------------------------------------------
void show_menu(void)
{
    fillScreen(ILI9341_BLACK);
    draw_stars();

    display_drawRect(3, 3, 314, 234, 0xF81F);
    display_drawRect(6, 6, 308, 228, ILI9341_CYAN);
    fillRect(7, 7, 306, 58, 0x000C);
    display_drawLine(7, 65, 313, 65, 0xF81F);

    display_setCursor(38, 18); display_setTextSize(3);
    display_setTextColor2(ILI9341_YELLOW);
    display_print('A'); display_print('S'); display_print('T');
    display_print('E'); display_print('R'); display_print('O');
    display_print('I'); display_print('D');

    draw_ship(297, 38, ILI9341_CYAN);

    if(menu_sel == 0) fillRect(20, 74, 280, 34, 0x0420);
    display_setCursor(50, 84); display_setTextSize(2);
    display_setTextColor2(menu_sel == 0 ? ILI9341_YELLOW : ILI9341_GREEN);
    display_print('J'); display_print('O'); display_print('U');
    display_print('E'); display_print('R');

    if(menu_sel == 1) fillRect(20, 112, 280, 34, 0x0420);
    display_setCursor(50, 122); display_setTextSize(2);
    display_setTextColor2(menu_sel == 1 ? ILI9341_YELLOW : ILI9341_CYAN);
    display_print('S'); display_print('C'); display_print('O');
    display_print('R'); display_print('E');

    if(menu_sel == 2) fillRect(20, 150, 280, 34, 0x2000);
    display_setCursor(50, 160); display_setTextSize(2);
    display_setTextColor2(menu_sel == 2 ? ILI9341_YELLOW : ILI9341_RED);
    display_print('Q'); display_print('U'); display_print('I');
    display_print('T'); display_print('T'); display_print('E');
    display_print('R');

    display_setCursor(28, 84 + menu_sel * 38);
    display_setTextSize(2); display_setTextColor2(0xF81F);
    display_print('>');

    display_drawLine(7, 218, 313, 218, 0xF81F);
    fillRect(7, 219, 306, 15, 0x000C);
    display_setCursor(18, 224); display_setTextSize(1);
    display_setTextColor2(ILI9341_LIGHTGREY);
    display_print('A'); display_print('/'); display_print('C');
    display_print('='); display_print('N'); display_print('A');
    display_print('V'); display_print(' ');
    display_print('B'); display_print('='); display_print('O');
    display_print('K'); display_print(' ');
    display_print('D'); display_print('='); display_print('R');
    display_print('E'); display_print('T');
}

// ----------------------------------------------------------------
void show_score(void)
{
    fillScreen(ILI9341_BLACK);
    draw_stars();

    display_drawRect(3, 3, 314, 234, 0xF81F);
    display_drawRect(6, 6, 308, 228, ILI9341_CYAN);
    fillRect(7, 7, 306, 18, 0x000C);
    display_drawLine(7, 25, 313, 25, 0xF81F);

    display_setCursor(108, 10); display_setTextSize(1);
    display_setTextColor2(ILI9341_YELLOW);
    display_print('H'); display_print('A'); display_print('L');
    display_print('L'); display_print(' '); display_print('O');
    display_print('F'); display_print(' '); display_print('F');
    display_print('A'); display_print('M'); display_print('E');

    fillRect(20, 50, 280, 38, 0x8400);
    display_drawRect(20, 50, 280, 38, ILI9341_YELLOW);
    display_fillCircle(40, 69, 13, ILI9341_YELLOW);
    display_setCursor(34, 63); display_setTextSize(2);
    display_setTextColor2(ILI9341_BLACK); display_print('1');
    display_setCursor(65, 63); display_setTextColor2(ILI9341_YELLOW);
    display_print('0'); display_print('0'); display_print('0');
    display_print('0'); display_print('0');
    display_setCursor(185, 66); display_setTextSize(1);
    display_setTextColor2(ILI9341_LIGHTGREY);
    display_print('P'); display_print('T'); display_print('S');

    fillRect(20, 100, 280, 38, 0x2104);
    display_drawRect(20, 100, 280, 38, ILI9341_LIGHTGREY);
    display_fillCircle(40, 119, 13, ILI9341_LIGHTGREY);
    display_setCursor(34, 113); display_setTextSize(2);
    display_setTextColor2(ILI9341_BLACK); display_print('2');
    display_setCursor(65, 113); display_setTextColor2(ILI9341_LIGHTGREY);
    display_print('0'); display_print('0'); display_print('0');
    display_print('0'); display_print('0');
    display_setCursor(185, 116); display_setTextSize(1);
    display_setTextColor2(ILI9341_LIGHTGREY);
    display_print('P'); display_print('T'); display_print('S');

    fillRect(20, 150, 280, 38, 0x4200);
    display_drawRect(20, 150, 280, 38, ILI9341_ORANGE);
    display_fillCircle(40, 169, 13, ILI9341_ORANGE);
    display_setCursor(34, 163); display_setTextSize(2);
    display_setTextColor2(ILI9341_BLACK); display_print('3');
    display_setCursor(65, 163); display_setTextColor2(ILI9341_ORANGE);
    display_print('0'); display_print('0'); display_print('0');
    display_print('0'); display_print('0');
    display_setCursor(185, 166); display_setTextSize(1);
    display_setTextColor2(ILI9341_LIGHTGREY);
    display_print('P'); display_print('T'); display_print('S');

    display_drawLine(7, 218, 313, 218, 0xF81F);
    fillRect(7, 219, 306, 15, 0x000C);
    display_setCursor(18, 224); display_setTextSize(1);
    display_setTextColor2(ILI9341_LIGHTGREY);
    display_print('D'); display_print('='); display_print('R');
    display_print('E'); display_print('T'); display_print('O');
    display_print('U'); display_print('R');
}

// ----------------------------------------------------------------
void draw_hud(int pts, int vie)
{
    int k;
    fillRect(0, 0, 320, 18, ILI9341_BLACK);
    display_setCursor(5, 5); display_setTextSize(1);
    display_setTextColor2(ILI9341_YELLOW);
    display_print('P'); display_print('T'); display_print('S');
    display_print(':'); display_print(' ');
    int t = pts;
    if(t >= 10000) display_print('0' + (t/10000)%10);
    if(t >= 1000)  display_print('0' + (t/1000)%10);
    if(t >= 100)   display_print('0' + (t/100)%10);
    if(t >= 10)    display_print('0' + (t/10)%10);
    display_print('0' + t%10);
    for(k = 0; k < vie; k++)
        display_fillCircle(265 + k*18, 8, 5, 0xF81F);
}

// ----------------------------------------------------------------
void show_fin(int pts)
{
    fillScreen(ILI9341_BLACK);
    draw_stars();

    fillRect(38, 53, 244, 144, 0x000A);
    display_drawRect(38, 53, 244, 144, 0xF81F);
    display_drawRect(40, 55, 240, 140, 0x8008);

    display_setCursor(55, 66); display_setTextSize(3);
    display_setTextColor2(ILI9341_RED);
    display_print('G'); display_print('A'); display_print('M');
    display_print('E');
    display_setCursor(72, 100);
    display_print('O'); display_print('V'); display_print('E');
    display_print('R');

    display_drawLine(55, 133, 265, 133, 0xF81F);

    display_setCursor(60, 143); display_setTextSize(2);
    display_setTextColor2(ILI9341_YELLOW);
    display_print('P'); display_print('T'); display_print('S');
    display_print(':'); display_print(' ');
    int t = pts;
    if(t >= 10000) display_print('0' + (t/10000)%10);
    if(t >= 1000)  display_print('0' + (t/1000)%10);
    if(t >= 100)   display_print('0' + (t/100)%10);
    if(t >= 10)    display_print('0' + (t/10)%10);
    display_print('0' + t%10);

    display_setCursor(58, 178); display_setTextSize(1);
    display_setTextColor2(ILI9341_LIGHTGREY);
    display_print('A'); display_print('p'); display_print('p');
    display_print('u'); display_print('i'); display_print('e');
    display_print(' '); display_print('B'); display_print(' ');
    display_print('p'); display_print('o'); display_print('u');
    display_print('r'); display_print(' '); display_print('r');
    display_print('e'); display_print('j'); display_print('o');
    display_print('u'); display_print('e'); display_print('r');

    RL_ON(); __delay_ms(250); RL_OFF(); __delay_ms(250);
    RL_ON(); __delay_ms(250); RL_OFF(); __delay_ms(250);
    RL_ON(); __delay_ms(250); RL_OFF();

    while(IO_RB4_GetValue() == 1) {}
    __delay_ms(200);
}

// ----------------------------------------------------------------
void run_game(void)
{
    int ship_x  = 160;
    int ship_y  = 120;
    int heading = 0;

    int blt_x = 0, blt_y = 0, blt_vx = 0, blt_vy = 0, blt_on = 0;
    int pts = 0, vie = 3, fin = 0;
    int joy_x, joy_y, ax, ay, k;

    int rx[6], ry[6], rr[6], rdx[6], rdy[6], ron[6];

    rx[0]=45;  ry[0]=45;  rr[0]=15; rdx[0]= 1; rdy[0]= 1; ron[0]=1;
    rx[1]=275; ry[1]=45;  rr[1]=15; rdx[1]=-1; rdy[1]= 1; ron[1]=1;
    rx[2]=160; ry[2]=195; rr[2]=15; rdx[2]= 1; rdy[2]=-1; ron[2]=1;
    rx[3]=0;   ry[3]=0;   rr[3]=8;  rdx[3]=0;  rdy[3]=0;  ron[3]=0;
    rx[4]=0;   ry[4]=0;   rr[4]=8;  rdx[4]=0;  rdy[4]=0;  ron[4]=0;
    rx[5]=0;   ry[5]=0;   rr[5]=8;  rdx[5]=0;  rdy[5]=0;  ron[5]=0;

    fillScreen(ILI9341_BLACK);
    draw_stars();
    draw_hud(pts, vie);

    for(k = 0; k < 6; k++)
        if(ron[k])
            display_drawCircle(rx[k], ry[k], rr[k], ILI9341_LIGHTGREY);

    draw_ship_angle(ship_x, ship_y, heading, pal[0]);

    while(fin == 0)
    {
        cycle_leds();

        // --- JOYSTICK : atan2f pour direction directe ---
        joy_x = ADCC_GetSingleConversion(channel_X) - 512;
        joy_y = ADCC_GetSingleConversion(channel_Y) - 512;
        ax = joy_x > 0 ? joy_x : -joy_x;
        ay = joy_y > 0 ? joy_y : -joy_y;

        if(ax > 200 || ay > 200)
        {
            int nh = joy_to_heading(joy_x, joy_y);
            if(nh != heading)
            {
                draw_ship_angle(ship_x, ship_y, heading, ILI9341_BLACK);
                heading = nh;
                draw_ship_angle(ship_x, ship_y, heading, pal[0]);
            }
        }

        // --- TIR : bouton B ---
        // La balle part de la pointe du triangle (meme calcul que fx/fy)
        if(IO_RB4_GetValue() == 0 && blt_on == 0)
        {
            blt_x  = ship_x + ANG_DX[heading] * 15 / 100;
            blt_y  = ship_y + ANG_DY[heading] * 15 / 100;
            blt_vx = ANG_DX[heading];
            blt_vy = ANG_DY[heading];
            blt_on = 1;
        }

        // --- DEPLACEMENT BALLE ---
        if(blt_on)
        {
            display_fillCircle(blt_x, blt_y, 2, ILI9341_BLACK);
            blt_x += blt_vx * 14 / 100;
            blt_y += blt_vy * 14 / 100;
            // Mur HUD (y<20) et bords ecran
            if(blt_x < 0 || blt_x > 319 || blt_y < 20 || blt_y > 214)
                blt_on = 0;
            else
                display_fillCircle(blt_x, blt_y, 2, ILI9341_YELLOW);
        }

        // --- ASTEROIDES ---
        for(k = 0; k < 6; k++)
        {
            int ex, ey;

            display_drawCircle(rx[k], ry[k], rr[k], ILI9341_BLACK);

            rx[k] += rdx[k];
            ry[k] += rdy[k];

            // Wrap horizontal
            if(rx[k] < 0)   rx[k] = 319;
            if(rx[k] > 319) rx[k] = 0;

            // Mur invisible HUD : le bord du cercle ne depasse jamais y=20
            if(ry[k] - rr[k] < 20)  ry[k] = 214 - rr[k];
            if(ry[k] + rr[k] > 214) ry[k] =  20 + rr[k];

            // Collision balle / asteroide
            if(blt_on && ron[k])
            {
                ex = blt_x - rx[k]; if(ex < 0) ex = -ex;
                ey = blt_y - ry[k]; if(ey < 0) ey = -ey;
                if(ex < rr[k]+3 && ey < rr[k]+3)
                {
                    display_fillCircle(blt_x, blt_y, 2, ILI9341_BLACK);
                    blt_on = 0;
                    ron[k] = 0;
                    pts += 20;
                    draw_hud(pts, vie);
                    GL_ON(); __delay_ms(50); GL_OFF();
                }
            }

            if(ron[k])
            {
                display_drawCircle(rx[k], ry[k], rr[k], ILI9341_LIGHTGREY);

                // Collision asteroide / vaisseau
                ex = ship_x - rx[k]; if(ex < 0) ex = -ex;
                ey = ship_y - ry[k]; if(ey < 0) ey = -ey;
                if(ex < rr[k] + 8 && ey < rr[k] + 8)
                {
                    ron[k] = 0;
                    display_drawCircle(rx[k], ry[k], rr[k], ILI9341_BLACK);
                    vie--;
                    draw_hud(pts, vie);
                    RL_ON(); __delay_ms(400); RL_OFF();
                    if(vie <= 0) fin = 1;
                }
            }
        }

        // --- RETOUR MENU : bouton D ---
        if(IO_RB3_GetValue() == 0)
        {
            __delay_ms(200);
            page = 0; menu_sel = 0;
            show_menu();
            return;
        }

        __delay_ms(15);
    }

    show_fin(pts);
    page = 0; menu_sel = 0;
    show_menu();
}

// ================================================================
void main(void)
{
    SYSTEM_Initialize();

    TRISEbits.TRISE0 = 0;
    TRISEbits.TRISE1 = 0;
    TRISEbits.TRISE2 = 0;
    RL_OFF(); GL_OFF(); BL_OFF();

    SPI1_Open(SPI1_DEFAULT);
    tft_begin();
    setRotation(3);
    fillScreen(ILI9341_BLACK);

    show_menu();

    while(1)
    {
        cycle_leds();

        // Bouton A : menu haut
        if(IO_RB6_GetValue() == 0)
        {
            if(page == 0) { if(--menu_sel < 0) menu_sel = 2; show_menu(); }
            __delay_ms(200);
        }

        // Bouton C : menu bas
        if(IO_RB7_GetValue() == 0)
        {
            if(page == 0) { if(++menu_sel > 2) menu_sel = 0; show_menu(); }
            __delay_ms(200);
        }

        // Bouton B : valider
        if(IO_RB4_GetValue() == 0)
        {
            if(page == 0)
            {
                if(menu_sel == 0) { page = 1; run_game(); }
                if(menu_sel == 1) { page = 2; show_score(); }
                if(menu_sel == 2) { fillScreen(ILI9341_BLACK); }
            }
            __delay_ms(200);
        }

        // Bouton D : retour menu
        if(IO_RB3_GetValue() == 0)
        {
            page = 0; menu_sel = 0;
            show_menu();
            __delay_ms(200);
        }

        __delay_ms(100);
    }
}
