#include <assert.h>
#include <math.h>
#include <raylib.h>
#include <raymath.h>
#include <stdio.h>
#include <stdlib.h>

const float GRAVITY = 0.3;

const size_t PLAYER_VEL_TRANSITION_FRAMES = 30;
typedef struct Player {
    Vector2 pos;
    Vector2 vel;
    Vector2 targetVel;

    size_t maxVel;
    size_t radius;
    int velTransitionTime;
} Player;

// Message queue to display on screen
#define MAX_MESSAGES_LEN 100
static const char *messages[MAX_MESSAGES_LEN] = {0};

static size_t messagesIndex = 0;
static size_t messagesLen = 0;

bool isInQueue(size_t index)
{
    return index >= messagesIndex && index < messagesIndex + messagesLen;
}

void messagesDebug(bool printAllMemory)
{
    printf("messages = {\n");
    printf("\t.index = %li,\n", messagesIndex);
    printf("\t.len = %li,\n", messagesLen);
    printf("\t.data = {\n");
    size_t stopIndex = printAllMemory ? MAX_MESSAGES_LEN : messagesLen;
    for (size_t i = 0; i < stopIndex; i += 1) {
        size_t messages_i = (i + messagesIndex) % MAX_MESSAGES_LEN;
        printf("\t\t");
        if (isInQueue(messages_i)) {
            printf("\"%s\",\n", messages[messages_i]);
        } else {
            printf("*\"%s\",\n", messages[messages_i]);
        }
    }
    printf("\t}\n");
    printf("}\n");
}

void messagesPut(const char *m)
{
    if (messagesLen > MAX_MESSAGES_LEN) {
        fprintf(stderr, "MESSAGE QUEUE DROPPING MESSAGES...\n");
        messagesLen -= 1;
        messagesIndex = (messagesIndex + 1) % MAX_MESSAGES_LEN;
        // remove first message
    }

    messages[(messagesIndex + messagesLen) % MAX_MESSAGES_LEN] = m;
    messagesLen += 1;
    messagesDebug(false);
}

bool messagesIsEmpty() { return messagesLen == 0; }

const char *messagesGet()
{
    if (messagesLen == 0) {
        return NULL;
    }

    size_t outIndex = messagesIndex;
    assert(isInQueue(outIndex));

    messagesIndex = (messagesIndex + 1) % MAX_MESSAGES_LEN;
    messagesLen -= 1;
    const char *out = messages[outIndex];
    messages[outIndex] = NULL;
    return out;
}

void messagesDestroy()
{
    while (!messagesIsEmpty()) {
        free((void *)messagesGet());
    }
}

void playerUpdate(Player *p)
{
    p->pos.x += p->vel.x;
    p->pos.y += p->vel.y;

    if (Vector2Length(p->vel) == 0) {
        return;
    }

    if (p->velTransitionTime > 0) { // need to speed up
        // linear
        //
        // vel {mag dir} -> max {mag dir}
        // ratio = currentFrame / transistionLength
        // vel + (ratio * dFrame{max / transistionLength, dir})
        //
        // [-------<----------------------]
        //         └─ p->velRamp          └─ PLAYER_VEL_TRANSITION_FRAMES
        Vector2 newVel = Vector2Lerp(p->vel, p->targetVel,
                                     (float)PLAYER_VEL_TRANSITION_FRAMES /
                                         p->velTransitionTime);
        p->vel = newVel;
    }

    if (p->velTransitionTime == -1) {
        p->vel = Vector2Scale(p->vel, 0.9);
    }
}

void messagesNew(const char *fmt, ...)
{
    va_list argsp;
    va_start(argsp, fmt);

    const size_t MAX_STR_LEN = 255;
    char *buff = MemAlloc(MAX_STR_LEN * sizeof(char));
    vsnprintf(buff, MAX_STR_LEN, fmt, argsp);
    puts(buff);
    va_end(argsp);

    messagesPut(buff);
}

void playerStop(Player *p) { p->velTransitionTime = -1; }
void playerMove(Player *p, Vector2 direction)
{
    static int moveNum = 0;

    if (p->maxVel / Vector2Length(direction) >= 1.0) {
        direction = Vector2Scale(Vector2Normalize(direction), p->maxVel);
    }

    if (p->velTransitionTime == -1) {
        p->velTransitionTime = PLAYER_VEL_TRANSITION_FRAMES;
    }

    moveNum += 1;
    // works
    if (Vector2Length(p->vel) < 1e-1) {
        p->vel = direction;
    }

    p->targetVel = direction;

    messagesNew("vel: {%.2f, %.2f}", p->vel.x, p->vel.y);
}

// Global varibales
static Player player;
static Camera2D playerCam;

#define MAX_BUILDINGS 100
static Rectangle buildings[MAX_BUILDINGS] = {0};
static Color buildColors[MAX_BUILDINGS] = {0};

Vector2 directionVector;

void setup()
{
    int spacing = 0;

    for (int i = 0; i < MAX_BUILDINGS; i++) {
        buildings[i].width = (float)GetRandomValue(50, 200);
        buildings[i].height = (float)GetRandomValue(100, 800);
        buildings[i].y = GetScreenHeight() - 130.0f - buildings[i].height;
        buildings[i].x = -6000.0f + spacing;

        spacing += (int)buildings[i].width;

        buildColors[i] =
            (Color){GetRandomValue(200, 240), GetRandomValue(200, 240),
                    GetRandomValue(200, 250), 255};
    }

    playerCam = (Camera2D){
        .offset = {(float)GetScreenWidth() / 2, (float)GetScreenHeight() / 2},
        .rotation = 0,
        .zoom = 1,
    };
    player = (Player){.radius = 10, .maxVel = 10};

    directionVector = (Vector2){0};
}

void update()
{

    playerCam.target = (Vector2){player.pos.x + 20, player.pos.y + 20};
    // playerVel.y += GRAVITY;
    playerUpdate(&player);

    if (IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
        Vector2 m = GetScreenToWorld2D(GetMousePosition(), playerCam);

        messagesNew("mouse clicked at {%.2f, %.2f}", m.x, m.y);

        directionVector = Vector2Subtract(m, player.pos);
        playerMove(&player, directionVector);
    }

    switch (GetKeyPressed()) {
    case KEY_R:
        setup();
        while (!messagesIsEmpty()) {
            free((void *)messagesGet());
        }
        break;
    case KEY_C:
        while (!messagesIsEmpty()) {
            free((void *)messagesGet());
        }
        break;
    case KEY_BACKSPACE:
        free((void *)messagesGet());
        break;
    case KEY_UP:
        player.pos.y -= 10;
        break;
    case KEY_LEFT:
        player.pos.x -= 10;
        break;
    case KEY_DOWN:
        player.pos.y += 10;
        break;
    case KEY_RIGHT:
        player.pos.x += 10;
        break;
    case KEY_W:
        playerMove(&player, (Vector2){0, -1});
        break;
    case KEY_A:
        playerMove(&player, (Vector2){-1, 0});
        break;
    case KEY_S:
        playerMove(&player, (Vector2){0, 1});
        break;
    case KEY_D:
        playerMove(&player, (Vector2){1, 0});
        break;
    }

    if (!IsKeyDown(KEY_W) && !IsKeyDown(KEY_A) && !IsKeyDown(KEY_S) &&
        !IsKeyDown(KEY_D)) {
        playerStop(&player);
    }
}

void draw()
{
    const size_t FONT_SIZE = 20;

    ClearBackground(WHITE);
    BeginMode2D(playerCam);
    DrawRectangle(-6000, 320, 13000, 8000, DARKGRAY);

    for (int i = 0; i < MAX_BUILDINGS; i++) {
        DrawRectangleRec(buildings[i], buildColors[i]);
    }

    DrawText(TextFormat("{%.2f, %.2f} %i", player.vel.x, player.vel.y,
                        player.velTransitionTime),
             playerCam.target.x, playerCam.target.y, FONT_SIZE, BLACK);

    DrawCircleV(player.pos, player.radius, BLACK);

    DrawLineV(player.pos,
              Vector2Add(player.pos, Vector2Scale(Vector2Normalize(player.vel),
                                                  player.maxVel * 2)),
              RED);

    DrawLineV(player.pos, Vector2Add(player.pos, directionVector), PURPLE);
    DrawLineV(player.pos,
              Vector2Add(player.pos, Vector2Rotate(directionVector, 90)),
              ORANGE);

    EndMode2D();

    static const float MESSAGE_LIFE = 2; // seconds
    // remove messages every MESSAGE_LIFE seconds
    static const char *message = NULL;
    static double messageBirth = 0;
    if (GetTime() - messageBirth > MESSAGE_LIFE) {
        if (message != NULL) {
            MemFree((void *)message);
        }

        message = messagesGet();
        if (message != NULL) {
            messageBirth = GetTime();
        }
    }

    // draw all messages in queue above player
    for (size_t i = 0; i < messagesLen; i += 1) {
        size_t messages_i = (i + messagesIndex) % MAX_MESSAGES_LEN;

        const char *nextMessage = messages[messages_i];
        if (nextMessage != NULL) {
            int paddingLeft = 4;
            int paddingRight = 4;
            int paddingTop = 4;
            int paddingBottom = 4;

            int textY =
                GetScreenHeight() -
                ((FONT_SIZE + paddingBottom + paddingTop + 4) * (i + 1)) - 10;
            int textX = 10;

            size_t textWidth = MeasureText(nextMessage, FONT_SIZE);

            DrawRectangle(textX - paddingLeft, textY - paddingTop,
                          textWidth + (paddingLeft + paddingRight),
                          FONT_SIZE + (paddingTop + paddingBottom),
                          (Color){25, 25, 25, 39});
            DrawText(nextMessage, textX, textY, FONT_SIZE, BLACK);
        }
    }

    DrawText(TextFormat("%.2f", GetTime()), 10, 10, FONT_SIZE, GREEN);
}

int init()
{
    InitWindow(600, 400, "leep");

    SetTargetFPS(60);
    return 1;
}

void deInit() { CloseWindow(); }

int main()
{
    if (!init()) {
        fprintf(stderr, "Error during initalization\n");
    }

    setup();

    while (!WindowShouldClose()) {
        BeginDrawing();
        update();
        draw();
        EndDrawing();
    }

    deInit();
}
