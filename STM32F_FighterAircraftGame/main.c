#if 1
#include "device_driver.h" 
#include <stdlib.h>       
#include <math.h>         
#include <stdio.h>        

#define LCDW (320)
#define LCDH (240)
#define BACK_COLOR   (5) 
#define PLAYER_COLOR (3) 
#define BULLET_COLOR (4) 
#define ENEMY_COLOR  (0) 
#define ENEMY_BULLET_COLOR (1) 
#define BOSS_COLOR   (2) 
#define BOSS_BULLET_COLOR (1) 

typedef enum { INIT, WAIT_START, STAGE_START, PLAYING, BOSS_FIGHT, GAME_OVER, WIN_SCREEN } GameState;

#define JOG_STATE_NONE  (0)
#define JOG_STATE_LEFT  (1)
#define JOG_STATE_RIGHT (2)

#define JOG_MASK_UP     (1 << 0) 
#define JOG_MASK_DOWN   (1 << 1) 
#define JOG_MASK_LEFT   (1 << 2) 
#define JOG_MASK_RIGHT  (1 << 3) 
//#define JOG_MASK_BTN2   (1 << 4) 
//#define JOG_MASK_BTN1   (1 << 5) 

#define BGM_BASE (200) 

enum key{ REST= -1, C4=0, Cs4, D4, Ds4, E4, F4, Fs4, G4, Gs4, A4, As4, B4, C5, Cs5, D5, Ds5, E5, F5, Fs5, G5, Gs5, A5, As5, B5 };
const unsigned short tone_value[] = {
    262, 277, 294, 311, 330, 349, 370, 392, 415, 440, 466, 494, // C4 ~ B4
    523, 554, 587, 622, 659, 698, 740, 784, 831, 880, 932, 988  // C5 ~ B5
};

enum note{ N16=BGM_BASE/4, N8=BGM_BASE/2, N4=BGM_BASE, N2=BGM_BASE*2, N1=BGM_BASE*4 };

const int song1[][2] = {
    {B4, N8}, {Ds5, N8}, {Fs5, N8}, {As5, N4},  // 시 레# 파# 라#
    {B4, N8}, {D5, N8}, {F5, N8}, {Gs5, N4},   // 시 레 파 솔#
    {As4, N8}, {Cs5, N8}, {F5, N8}, {Gs5, N4}, // 라# 도# 파 솔#
    {A4, N8}, {C5, N8}, {Ds5, N8}, {Fs5, N4},  // 라 도 레# 파#
    {Gs4, N8}, {B4, N8}, {Ds5, N8}, {Fs5, N4},  // 솔# 시 레# 파#
    {B4, N8}, {D5, N8}, {F5, N8}, {Gs5, N4},   // 시 레 파 솔#
    {Cs5, N8}, {F5, N8}, {Fs5, N8}, {As5, N4}, // 도# 파 파# 라#
    {Cs5, N8}, {Ds5, N8}, {G5, N8}, {As5, N4}  // 도# 레# 솔 라#
};
const int song1_length = sizeof(song1) / sizeof(song1[0]);

typedef struct { int x, y; int w, h; int life; } Player;
typedef struct { int x, y; int dx, dy; int active; } Bullet;
typedef struct { int x, y; int alive; } Enemy;
typedef struct { int x, y; int dx, dy; int active; } EnemyBullet;
typedef struct { int x, y; int w, h; int dx; int hp; int alive; } Boss;

#define MAX_BULLETS (10)
#define MAX_ENEMIES (5) 
#define MAX_ENEMY_BULLETS (10)
#define PLAYER_SPEED (5)
#define BOSS_HP (30)
#define STAGE_DISPLAY_DURATION_MS (1500)
#define ENEMY_SPEED (1)

Player player;
Bullet bullets[MAX_BULLETS];
Enemy enemies[MAX_ENEMIES];
EnemyBullet enemy_bullets[MAX_ENEMY_BULLETS];
Boss boss;
int score = 0;

GameState game_state = INIT;
int current_stage = 1;
int stage_display_timer_ms = 0;

int bgm_song_index = 0;
int bgm_note_duration_ticks = 0;
int bgm_is_playing = 0; // 0: 정지, 1: 재생 중, 2: 쉼표 대기

extern volatile int Jog_key_in;
extern volatile int Jog_key;
extern volatile int TIM4_expired;
//extern volatile int TIM2_Expired; 

#define GAME_TICK_MS (10) 
unsigned short color[] = { RED, YELLOW, GREEN, BLUE, WHITE, BLACK };

void System_Init(void);
void Game_Init(void);
void Wait_Or_End_Screen(const char* msg, GameState next_state_on_key);
void Draw_Player(void);
void Draw_Bullet(Bullet* b);
void Draw_Enemy(Enemy* e);
void Draw_Enemy_Bullet(EnemyBullet* eb);
void Draw_Boss(void);
void Display_Stage_Screen(int stage);
void Fire_Bullet(void);
void Enemy_Fire_Spread(int start_x, int start_y, int target_x, int target_y, int bullet_speed, int bullet_count);
void Enemy_Try_Fire(int enemy_index);
void Boss_Try_Fire(void);
void Move_Player(void);
void Move_Bullets(void);
void Move_Enemies(void);
void Move_Enemy_Bullets(void);
void Move_Boss(void);
void Check_Collision(void);
void Check_Player_Collision(void);
void Spawn_Enemies(void);
int Read_Jog_State(void);


int Read_Jog_State(void) {
    int key_mask = Jog_Get_Pressed();
    if (key_mask & JOG_MASK_LEFT) return JOG_STATE_LEFT;
    else if (key_mask & JOG_MASK_RIGHT) return JOG_STATE_RIGHT;
    else return JOG_STATE_NONE;
}

void System_Init(void) {
    Clock_Init();
    LED_Init();
    Jog_Poll_Init();
    Uart1_Init(115200);
    TIM3_Out_Init();
    
    SCB->VTOR = 0x08003000;
	SCB->SHCSR = 0;
}

void Spawn_Enemies(void) {
     if (current_stage == 5) return;
     int i; 
     for (i = 0; i < MAX_ENEMIES; i++) {
         if (!enemies[i].alive) {
             enemies[i].x = rand() % (LCDW - 20);
             enemies[i].y = 0;
             enemies[i].alive = 1;
             break;
         }
     }
}

void Game_Init(void) {
    player.x = LCDW / 2 - 15; player.y = LCDH - 30;
    player.w = 30; player.h = 10; player.life = 5;
    score = 0;
    current_stage = 1;
    int i; 
    for (i = 0; i < MAX_BULLETS; i++) bullets[i].active = 0;
    for (i = 0; i < MAX_ENEMIES; i++) enemies[i].alive = 0;
    for (i = 0; i < MAX_ENEMY_BULLETS; i++) enemy_bullets[i].active = 0;
    boss.alive = 0;

    game_state = STAGE_START;
    stage_display_timer_ms = STAGE_DISPLAY_DURATION_MS;
    Display_Stage_Screen(current_stage);

    bgm_song_index = 0;
    bgm_note_duration_ticks = 0;
    bgm_is_playing = 0;
}

void Wait_Or_End_Screen(const char* msg, GameState next_state_on_key) {
    Lcd_Clr_Screen();
    if (game_state == WIN_SCREEN) {
        Lcd_Printf(50, 80, GREEN, BLACK, 2, 2, "Congratulations!");
        Lcd_Printf(80, 130, WHITE, BLACK, 2, 2, "YOU WIN!");
        Lcd_Printf(40, 180, YELLOW, BLACK, 1, 1, msg);
    } else if (game_state == GAME_OVER) {
        Lcd_Printf(50, 100, RED, BLACK, 3, 3, "GAME OVER");
        Lcd_Printf(40, 180, YELLOW, BLACK, 1, 1, msg);
    } else {
        Lcd_Printf(50, 80, GREEN, BLACK, 2, 2, "Core Invaders 32");
        Lcd_Printf(40, 180, WHITE, BLACK, 1, 1, msg);
    }
    Jog_key_in = 0;
    while (!Jog_key_in){}
    TIM2_Delay(50);
    Jog_key_in = 0;
    game_state = next_state_on_key;
}

void Draw_Player(void) { 
    Lcd_Draw_Box(player.x, player.y, player.w, player.h, color[PLAYER_COLOR]); }
void Draw_Bullet(Bullet* b) { 
    if (b->active) Lcd_Draw_Box(b->x, b->y, 5, 10, color[BULLET_COLOR]); }
void Draw_Enemy(Enemy* e) { 
    if (e->alive) Lcd_Draw_Box(e->x, e->y, 20, 20, color[ENEMY_COLOR]); }
void Draw_Enemy_Bullet(EnemyBullet* eb) {
    if (eb->active) {
        int draw_x = eb->x; int draw_y = eb->y;
        int draw_w = 3; int draw_h = 6;
        if (draw_y < 0) { draw_h += draw_y; draw_y = 0; }
        if (draw_y + draw_h > LCDH) { draw_h = LCDH - draw_y; }
        if (draw_h > 0 && draw_w > 0) { Lcd_Draw_Box(draw_x, draw_y, draw_w, draw_h, color[ENEMY_BULLET_COLOR]); }
    }
}
void Draw_Boss(void) {
    if (boss.alive) { 
        Lcd_Draw_Box(boss.x, boss.y, boss.w, boss.h, color[BOSS_COLOR]); 
        int bar_width = boss.w; int current_hp_width = (bar_width * boss.hp) / BOSS_HP; 
        if (current_hp_width < 0) current_hp_width = 0; 
        Lcd_Draw_Box(boss.x, boss.y - 6, bar_width, 4, RED); 
        Lcd_Draw_Box(boss.x, boss.y - 6, current_hp_width, 4, GREEN); }
}
void Display_Stage_Screen(int stage) {
    Lcd_Clr_Screen(); char stage_text[20];
    if (stage == 5) sprintf(stage_text, "BOSS STAGE"); else sprintf(stage_text, "STAGE %d", stage);
    Lcd_Printf(80, 100, WHITE, BLACK, 3, 3, stage_text);
}

void Fire_Bullet(void) {
    int i; int num_to_fire = 1 + (score / 30); 
    int fired_count = 0; int base_speed_y = 10; 
    float angle_factor; 
    if (num_to_fire > MAX_BULLETS) num_to_fire = MAX_BULLETS; 
    for (i = 0; i < MAX_BULLETS && fired_count < num_to_fire; i++) { 
        if (!bullets[i].active) { 
            bullets[i].x = player.x + player.w / 2 - 2; 
            bullets[i].y = player.y - 10; 
            bullets[i].active = 1; 
            if (num_to_fire == 1) {
                 bullets[i].dx = 0; bullets[i].dy = -base_speed_y; 
                } 
            else { 
                angle_factor = (float)(fired_count * 2) / (num_to_fire - 1) - 1.0f; 
                bullets[i].dx = (int)(angle_factor * (base_speed_y / 3.0f)); 
                bullets[i].dy = -base_speed_y; 
            } 
            fired_count++; 
        } 
    }
}

void Enemy_Fire_Spread(int start_x, int start_y, int target_x, int target_y, int bullet_speed, int bullet_count) {
    int j; 
    int found = 0;
    int slots[3] = {-1, -1, -1}; 
    float angle_center, angle_offset, angle;
    int dx, dy;
    const int bullet_width = 3; 

    if(bullet_count > 3) bullet_count = 3; 

    
    for (j = 0; j < MAX_ENEMY_BULLETS && found < bullet_count; j++) {
        if (!enemy_bullets[j].active) {
            slots[found++] = j;
        }
    }

    if (found < bullet_count) return; 

    int target_dx = target_x - start_x;
    int target_dy = target_y - start_y;
    float distance = sqrtf((float)target_dx * target_dx + (float)target_dy * target_dy);
    if (distance == 0) distance = 1; 

    angle_center = atan2f((float)target_dy, (float)target_dx);
    angle_offset = 12.0f * (M_PI / 180.0f); 

    for (j = 0; j < bullet_count; j++) {
        int slot = slots[j];
        if (bullet_count == 1) {
            angle = angle_center;
        } else if (bullet_count % 2 == 1) { 
            angle = angle_center + (j - bullet_count / 2) * angle_offset;
        } else { 
             angle = angle_center + (j - bullet_count / 2 + 0.5f) * angle_offset;
        }

        dx = (int)roundf(cosf(angle) * bullet_speed);
        dy = (int)roundf(sinf(angle) * bullet_speed);

        if (dx == 0 && dy == 0) dy = bullet_speed; 
        if (dy <= 0) dy = 1;

        enemy_bullets[slot].x = start_x - bullet_width / 2; 
        enemy_bullets[slot].y = start_y + 1; 
        enemy_bullets[slot].dx = dx;
        enemy_bullets[slot].dy = dy;
        enemy_bullets[slot].active = 1;
    }
}

void Enemy_Try_Fire(int enemy_index) {
    int start_x_center = enemies[enemy_index].x + 10; 
    int start_y_base = enemies[enemy_index].y + 20;   
    int target_x = player.x + player.w / 2;
    int target_y = player.y + player.h / 2;

    Enemy_Fire_Spread(start_x_center, start_y_base, target_x, target_y, 2, 1);
}

void Boss_Try_Fire(void) {
    int start_x = boss.x + boss.w / 2;
    int start_y = boss.y + boss.h;
    int target_x = player.x + player.w / 2;
    int target_y = player.y + player.h / 2;

    Enemy_Fire_Spread(start_x, start_y, target_x, target_y, 3, 3);
}


void Move_Player(void) {
    int key_mask = Jog_Get_Pressed();
    int prev_x = player.x; int prev_y = player.y; int moved = 0;
    if ((key_mask & JOG_MASK_UP) && player.y > 0) { 
        player.y -= PLAYER_SPEED; if (player.y < 0) player.y = 0; moved = 1; }
    else if ((key_mask & JOG_MASK_DOWN) && player.y + player.h < LCDH) { 
        player.y += PLAYER_SPEED; 
        if (player.y + player.h > LCDH) player.y = LCDH - player.h; moved = 1; }
    if ((key_mask & JOG_MASK_LEFT) && player.x > 0) { 
        player.x -= PLAYER_SPEED; 
        if (player.x < 0) player.x = 0; moved = 1; }
    else if ((key_mask & JOG_MASK_RIGHT) && player.x + player.w < LCDW) { 
        player.x += PLAYER_SPEED; 
        if (player.x + player.w > LCDW) player.x = LCDW - player.w; moved = 1; }
    if (moved) { 
        Lcd_Draw_Box(prev_x, prev_y, player.w, player.h, color[BACK_COLOR]); 
        Draw_Player(); }
}

void Move_Bullets(void) {
    int i; for (i = 0; i < MAX_BULLETS; i++) { 
        if (bullets[i].active) { 
            Lcd_Draw_Box(bullets[i].x, bullets[i].y, 5, 10, color[BACK_COLOR]); 
            bullets[i].x += bullets[i].dx; 
            bullets[i].y += bullets[i].dy; 
            if (bullets[i].y < 0 || bullets[i].y > LCDH || bullets[i].x < 0 || bullets[i].x > LCDW - 5) bullets[i].active = 0; 
            else Draw_Bullet(&bullets[i]); 
        } 
    }
}
void Move_Enemy_Bullets(void) {
    int i; int prev_x, prev_y, next_x, next_y; const int bullet_width = 3; const int bullet_height = 6;
    for (i = 0; i < MAX_ENEMY_BULLETS; i++) { 
        if (enemy_bullets[i].active) { 
            prev_x = enemy_bullets[i].x; prev_y = enemy_bullets[i].y; 
            next_x = prev_x + enemy_bullets[i].dx; next_y = prev_y + enemy_bullets[i].dy; 
            if ((next_y + bullet_height) >= LCDH || next_y < 0 || next_x < 0 || next_x >= LCDW - bullet_width) { 
                enemy_bullets[i].active = 0; 
                Lcd_Draw_Box(prev_x, prev_y, bullet_width, bullet_height, color[BACK_COLOR]); 
            } 
            else { 
                Lcd_Draw_Box(prev_x, prev_y, bullet_width, bullet_height, color[BACK_COLOR]); 
                enemy_bullets[i].x = next_x; enemy_bullets[i].y = next_y; 
                Draw_Enemy_Bullet(&enemy_bullets[i]); 
            } 
        } 
    }
}
void Move_Boss(void) {
    if (boss.alive) { 
        Lcd_Draw_Box(boss.x, boss.y, boss.w, boss.h, color[BACK_COLOR]); 
        Lcd_Draw_Box(boss.x, boss.y - 6, boss.w, 4, color[BACK_COLOR]); 
        boss.x += boss.dx; 
        if (boss.x <= 0 || boss.x + boss.w >= LCDW) { 
            boss.dx = -boss.dx; 
            if (boss.x < 0) boss.x = 0; 
            if (boss.x + boss.w > LCDW) boss.x = LCDW - boss.w; 
        } 
        Draw_Boss(); 
    }
}
void Move_Enemies(void) {
    int i; 
    for (i = 0; i < MAX_ENEMIES; i++) { 
        if (enemies[i].alive) { 
            Lcd_Draw_Box(enemies[i].x, enemies[i].y, 20, 20, color[BACK_COLOR]); 
            enemies[i].y += ENEMY_SPEED; 
            if (enemies[i].y > LCDH - 20) { 
                enemies[i].alive = 0; player.life--; 
            } 
            else { Draw_Enemy(&enemies[i]); } 
        } 
    }
}

void Check_Collision(void) {
int i, j, k; 
int next_stage; 
int stage_cleared = 0; 
for (i = 0; i < MAX_BULLETS; i++) { 
    if (bullets[i].active) { 
        if (game_state == PLAYING) { 
            for (j = 0; j < MAX_ENEMIES; j++) { 
                if (enemies[j].alive) { 
                    if (bullets[i].x < enemies[j].x + 20 && bullets[i].x + 5 > enemies[j].x && bullets[i].y < enemies[j].y + 20 && bullets[i].y + 10 > enemies[j].y) { 
                        Lcd_Draw_Box(bullets[i].x, bullets[i].y, 5, 10, color[BACK_COLOR]); 
                        bullets[i].active = 0; 
                        Lcd_Draw_Box(enemies[j].x, enemies[j].y, 20, 20, color[BACK_COLOR]); 
                        enemies[j].alive = 0; score += 10; next_stage = (score / 30) + 1; 
                        if (next_stage > 5) next_stage = 5; 
                        if (next_stage > current_stage) { current_stage = next_stage; game_state = STAGE_START; 
                        stage_display_timer_ms = STAGE_DISPLAY_DURATION_MS; 
                        for(k=0; k<MAX_ENEMIES; k++) { if(enemies[k].alive) 
                        Lcd_Draw_Box(enemies[k].x, enemies[k].y, 20, 20, color[BACK_COLOR]); 
                        enemies[k].alive = 0; } 
                    for(k=0; k<MAX_ENEMY_BULLETS; k++) { 
                        if(enemy_bullets[k].active) 
                        Lcd_Draw_Box(enemy_bullets[k].x, enemy_bullets[k].y, 3, 6, color[BACK_COLOR]); 
                        enemy_bullets[k].active = 0; } 
                        Display_Stage_Screen(current_stage); 
                    } goto next_bullet_collision_check_stage; 
                } 
            } 
        } 
    } 
    else if (game_state == BOSS_FIGHT && boss.alive) { 
        if (bullets[i].x < boss.x + boss.w && bullets[i].x + 5 > boss.x && bullets[i].y < boss.y + boss.h && bullets[i].y + 10 > boss.y) { 
            Lcd_Draw_Box(bullets[i].x, bullets[i].y, 5, 10, color[BACK_COLOR]); 
            bullets[i].active = 0; boss.hp--; 
            if (boss.hp <= 0) { 
                boss.alive = 0; 
                Lcd_Draw_Box(boss.x, boss.y, boss.w, boss.h, color[BACK_COLOR]);
                Lcd_Draw_Box(boss.x, boss.y - 6, boss.w, 4, color[BACK_COLOR]); 
                game_state = WIN_SCREEN; stage_cleared = 1; 
            } goto next_bullet_collision_check_stage; } } 
        } next_bullet_collision_check_stage:; 
    }
}

void Check_Player_Collision(void) {
    int i; 
    for (i = 0; i < MAX_ENEMY_BULLETS; i++) { 
        if (enemy_bullets[i].active) { 
            if (enemy_bullets[i].x < player.x + player.w && enemy_bullets[i].x + 3 > player.x && enemy_bullets[i].y < player.y + player.h && enemy_bullets[i].y + 6 > player.y) {
                Lcd_Draw_Box(enemy_bullets[i].x, enemy_bullets[i].y, 3, 6, color[BACK_COLOR]); 
                enemy_bullets[i].active = 0; player.life--; 
            } 
        } 
    }
}

void Main(void) {
    System_Init();
    Lcd_Init(3);
    Jog_ISR_Enable(1);
    TIM4_Repeat_Interrupt_Enable(1, GAME_TICK_MS);

    game_state = INIT;

    bgm_song_index = 0;
    bgm_note_duration_ticks = 0;
    bgm_is_playing = 0;

    for (;;) {
        if (game_state == INIT || game_state == WAIT_START) {
            TIM3_Out_Stop(); 
            bgm_is_playing = 0;
            Wait_Or_End_Screen("Press key to start!", STAGE_START);
            if(game_state == STAGE_START) Game_Init(); 
        }

        while (game_state == STAGE_START || game_state == PLAYING || game_state == BOSS_FIGHT) {
            if (game_state == STAGE_START) {
                if (stage_display_timer_ms <= 0) {
                    Lcd_Clr_Screen(); Draw_Player();
                    if (current_stage == 5) { game_state = BOSS_FIGHT; boss.w=80; boss.h=40; boss.x=LCDW/2-boss.w/2; 
                        boss.y=30; boss.hp=BOSS_HP; boss.dx=2; boss.alive=1; Draw_Boss(); }
                    else game_state = PLAYING;
                }
            }

            if (Jog_key_in && (game_state == PLAYING || game_state == BOSS_FIGHT)) {
                if (Jog_key == 4) { Fire_Bullet();  } 
                else if (Jog_key == 5) 
                { 
                    if (current_stage < 5) 
                    { 
                        current_stage++; game_state = STAGE_START; stage_display_timer_ms = STAGE_DISPLAY_DURATION_MS; int clear_idx; 
                        for(clear_idx=0; clear_idx<MAX_ENEMIES; clear_idx++){ 
                            if(enemies[clear_idx].alive) 
                            Lcd_Draw_Box(enemies[clear_idx].x, enemies[clear_idx].y, 20, 20, color[BACK_COLOR]); enemies[clear_idx].alive = 0; 
                        } 
                        for(clear_idx=0; clear_idx<MAX_ENEMY_BULLETS; clear_idx++){ 
                            if(enemy_bullets[clear_idx].active) 
                            Lcd_Draw_Box(enemy_bullets[clear_idx].x, enemy_bullets[clear_idx].y, 3, 6, color[BACK_COLOR]); enemy_bullets[clear_idx].active = 0; 
                        } 
                        for(clear_idx=0; clear_idx<MAX_BULLETS; clear_idx++){ 
                            if(bullets[clear_idx].active) 
                            Lcd_Draw_Box(bullets[clear_idx].x, bullets[clear_idx].y, 5, 10, color[BACK_COLOR]); bullets[clear_idx].active = 0; 
                        } 
                        if (boss.alive) { 
                            Lcd_Draw_Box(boss.x, boss.y, boss.w, boss.h, color[BACK_COLOR]); 
                            Lcd_Draw_Box(boss.x, boss.y - 6, boss.w, 4, color[BACK_COLOR]); boss.alive = 0; 
                        } 
                        TIM3_Out_Stop(); bgm_is_playing = 0; Display_Stage_Screen(current_stage); 
                    } 
                }
                Jog_key_in = 0;
            }

            if (TIM4_expired) {
                if (bgm_is_playing) { 
                    bgm_note_duration_ticks--;
                    if (bgm_note_duration_ticks <= 0) { 
                        TIM3_Out_Stop(); 
                        bgm_is_playing = 0; 
                        bgm_song_index++;   
                        if (bgm_song_index >= song1_length) {
                            bgm_song_index = 0; 
                        }
                    }
                }

                if (!bgm_is_playing && (game_state == PLAYING || game_state == BOSS_FIGHT)) {
                    int current_tone_enum = song1[bgm_song_index][0];
                    int current_duration_ms = song1[bgm_song_index][1];

                    if (current_tone_enum == REST) { 
                        TIM3_Out_Stop(); 
                        bgm_note_duration_ticks = current_duration_ms / GAME_TICK_MS;
                        if (bgm_note_duration_ticks <= 0) bgm_note_duration_ticks = 1; 
                        bgm_is_playing = 2; 
                    } else if (current_duration_ms > 0) { 
                        if (current_tone_enum >= 0 && current_tone_enum < sizeof(tone_value)/sizeof(tone_value[0])) { 
                             TIM3_Out_Freq_Generation(tone_value[current_tone_enum]); 
                             bgm_note_duration_ticks = current_duration_ms / GAME_TICK_MS;
                             if (bgm_note_duration_ticks <= 0) bgm_note_duration_ticks = 1;
                             bgm_is_playing = 1; 
                        } else {
                             bgm_song_index++;
                             if (bgm_song_index >= song1_length) bgm_song_index = 0;
                        }
                    } else {
                        bgm_song_index++;
                        if (bgm_song_index >= song1_length) bgm_song_index = 0;
                    }

                     if(bgm_is_playing != 0 && bgm_note_duration_ticks <= 0){
                          TIM3_Out_Stop();
                          bgm_is_playing = 0;
                          bgm_song_index++;
                          if (bgm_song_index >= song1_length) bgm_song_index = 0;
                     }
                }

                if (game_state == STAGE_START && stage_display_timer_ms > 0) {
                    stage_display_timer_ms -= GAME_TICK_MS;
                    if (stage_display_timer_ms < 0) stage_display_timer_ms = 0;
                }

                if (game_state == PLAYING || game_state == BOSS_FIGHT) {
                    Move_Player(); Move_Bullets(); Move_Enemies(); Move_Enemy_Bullets();
                    if (game_state == BOSS_FIGHT) Move_Boss();
                    Check_Collision();
                    if (game_state != WIN_SCREEN) Check_Player_Collision();

                    if (game_state == PLAYING) { int i; if (rand() % (100 / GAME_TICK_MS * 2) == 0) Spawn_Enemies(); 
                        for(i = 0; i < MAX_ENEMIES; i++) { if (enemies[i].alive && (rand() % 250 == 0)) Enemy_Try_Fire(i); } }
                    else if (game_state == BOSS_FIGHT && boss.alive) { if (rand() % 40 == 0) Boss_Try_Fire(); } 
                }

                if(game_state == PLAYING || game_state == BOSS_FIGHT){ 
                    Lcd_Printf(0, 0, color[PLAYER_COLOR], color[BACK_COLOR], 1, 1, "SCORE:%d LIFE:%d STAGE:%d", score, player.life, current_stage); }

                TIM4_expired = 0; 

                
                if (player.life <= 0 && (game_state == PLAYING || game_state == BOSS_FIGHT)) { game_state = GAME_OVER; break; }
                if (game_state == WIN_SCREEN) { break; }

            } 
        }

        if (game_state == GAME_OVER) {
            TIM3_Out_Stop(); 
            bgm_is_playing = 0;
            Wait_Or_End_Screen("Press key to restart", WAIT_START);
        } else if (game_state == WIN_SCREEN) {
            TIM3_Out_Stop(); 
            bgm_is_playing = 0;
            Wait_Or_End_Screen("Press key to restart", WAIT_START);
        }
    }
} 
#endif