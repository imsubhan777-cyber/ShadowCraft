#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <cmath>
#include <string>

struct Block { int x, y, z, type; };
static std::vector<Block> world;
static const int WORLD = 18;
static int selectedBlock = 1;
static bool creative = false;
static Vector3 player = {8.0f, 3.0f, 8.0f};
static float yaw = 0.0f, pitch = 0.0f;
static float verticalVelocity = 0.0f;
static bool grounded = false;

Color blockColor(int t) {
    switch (t) {
        case 1: return GREEN;       // grass
        case 2: return BROWN;       // dirt
        case 3: return GRAY;        // stone
        case 4: return {120, 82, 45, 255}; // wood
        case 5: return SKYBLUE;     // glass-like
        default: return WHITE;
    }
}
int findBlock(int x, int y, int z) {
    for (int i = (int)world.size()-1; i >= 0; --i)
        if (world[i].x==x && world[i].y==y && world[i].z==z) return i;
    return -1;
}
void addBlock(int x,int y,int z,int t) {
    if (findBlock(x,y,z)<0 && y>=0 && y<24) world.push_back({x,y,z,t});
}
void makeWorld() {
    world.clear();
    for (int x=0;x<WORLD;x++) for (int z=0;z<WORLD;z++) {
        float h = 1.0f + 0.7f*sinf(x*0.55f)*cosf(z*0.42f);
        int height = (int)floorf(h+1.0f);
        for (int y=0;y<height;y++) addBlock(x,y,z, y==height-1 ? 1 : (y>height-3 ? 2 : 3));
    }
    // A few simple trees
    for (int x=3;x<WORLD;x+=6) for (int z=3;z<WORLD;z+=7) {
        int ground=2;
        for(int y=ground;y<ground+3;y++) addBlock(x,y,z,4);
        for(int dx=-2;dx<=2;dx++) for(int dz=-2;dz<=2;dz++)
            for(int dy=2;dy<=4;dy++)
                if (abs(dx)+abs(dz)+(dy==4?1:0)<5) addBlock(x+dx,ground+dy,z+dz,1);
    }
}
RayCollision hitTest(Camera3D camera) {
    Ray ray = GetMouseRay({GetScreenWidth()*0.5f, GetScreenHeight()*0.5f}, camera);
    RayCollision best = {0};
    best.distance = 9999.0f;
    for (const auto &b : world) {
        BoundingBox box = {{(float)b.x,(float)b.y,(float)b.z},{b.x+1.0f,b.y+1.0f,b.z+1.0f}};
        RayCollision hit = GetRayCollisionBox(ray, box);
        if (hit.hit && hit.distance < best.distance) best=hit;
    }
    return best;
}
void drawWorld() {
    for (const auto &b : world) {
        Vector3 p={(float)b.x+0.5f,(float)b.y+0.5f,(float)b.z+0.5f};
        DrawCube(p,1,1,1,blockColor(b.type));
        DrawCubeWires(p,1,1,1,{35,35,35,255});
    }
    DrawGrid(30,1.0f);
}
int main() {
    SetConfigFlags(FLAG_MSAA_4X_HINT);
    InitWindow(960,540,"ShadowCraft - C++ Voxel Prototype");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);
    makeWorld();

    Camera3D camera={0};
    camera.position=player;
    camera.target={player.x,player.y,player.z-1};
    camera.up={0,1,0};
    camera.fovy=70;
    camera.projection=CAMERA_PERSPECTIVE;

    Vector2 lastLook={0,0};
    bool wasLooking=false;
    while (!WindowShouldClose()) {
        float dt=GetFrameTime();
        // Desktop keyboard controls; on Android use the on-screen zones below.
        Vector3 forward={sinf(yaw),0,-cosf(yaw)};
        Vector3 right={cosf(yaw),0,sinf(yaw)};
        Vector3 move={0,0,0};
        if (IsKeyDown(KEY_W)) move=Vector3Add(move,forward);
        if (IsKeyDown(KEY_S)) move=Vector3Subtract(move,forward);
        if (IsKeyDown(KEY_D)) move=Vector3Add(move,right);
        if (IsKeyDown(KEY_A)) move=Vector3Subtract(move,right);

        // Touch: left lower quarter is movement joystick; right half is look.
        int touches=GetTouchPointCount();
        for (int i=0;i<touches;i++) {
            Vector2 p=GetTouchPosition(i);
            if (p.x < GetScreenWidth()*0.42f && p.y > GetScreenHeight()*0.48f) {
                Vector2 center={GetScreenWidth()*0.20f,GetScreenHeight()*0.78f};
                Vector2 d=Vector2Subtract(p,center);
                float len=Vector2Length(d);
                if(len>12) {
                    d=Vector2Scale(Vector2Normalize(d),fminf(len/65.0f,1.0f));
                    move=Vector3Add(move,Vector3Add(Vector3Scale(forward,-d.y),Vector3Scale(right,d.x)));
                }
            } else if (p.x > GetScreenWidth()*0.38f && p.y < GetScreenHeight()*0.82f) {
                if (!wasLooking) { lastLook=p; wasLooking=true; }
                else {
                    Vector2 delta=Vector2Subtract(p,lastLook);
                    yaw -= delta.x*0.0045f;
                    pitch -= delta.y*0.0045f;
                    pitch=Clamp(pitch,-1.35f,1.35f);
                    lastLook=p;
                }
            }
        }
        if(touches==0) wasLooking=false;

        if(Vector3Length(move)>0) {
            move=Vector3Normalize(move);
            player=Vector3Add(player,Vector3Scale(move,creative?7.0f*dt:4.5f*dt));
        }
        if(IsKeyPressed(KEY_SPACE) && grounded) { verticalVelocity=7.0f; grounded=false; }
        if(!creative) {
            verticalVelocity-=18.0f*dt;
            player.y+=verticalVelocity*dt;
            if(player.y<2.2f) { player.y=2.2f; verticalVelocity=0; grounded=true; }
        } else {
            if(IsKeyDown(KEY_SPACE)) player.y+=5*dt;
            if(IsKeyDown(KEY_LEFT_SHIFT)) player.y-=5*dt;
        }
        player.x=Clamp(player.x,0.2f,WORLD-0.2f);
        player.z=Clamp(player.z,0.2f,WORLD-0.2f);

        camera.position=player;
        Vector3 look={sinf(yaw)*cosf(pitch),sinf(pitch),-cosf(yaw)*cosf(pitch)};
        camera.target=Vector3Add(player,look);

        RayCollision target=hitTest(camera);
        // Keyboard actions for testing; Android buttons below.
        if(IsKeyPressed(KEY_F) && target.hit) {
            Vector3 q=Vector3Subtract(target.point,Vector3Scale(target.normal,0.02f));
            int idx=findBlock((int)floorf(q.x),(int)floorf(q.y),(int)floorf(q.z));
            if(idx>=0) world.erase(world.begin()+idx);
        }
        if(IsKeyPressed(KEY_E) && target.hit) {
            Vector3 q=Vector3Add(target.point,Vector3Scale(target.normal,0.55f));
            addBlock((int)floorf(q.x),(int)floorf(q.y),(int)floorf(q.z),selectedBlock);
        }
        if(IsKeyPressed(KEY_M)) creative=!creative;
        if(IsKeyPressed(KEY_ONE)) selectedBlock=1;
        if(IsKeyPressed(KEY_TWO)) selectedBlock=2;
        if(IsKeyPressed(KEY_THREE)) selectedBlock=3;
        if(IsKeyPressed(KEY_FOUR)) selectedBlock=4;

        BeginDrawing();
        ClearBackground({135,190,235,255});
        BeginMode3D(camera);
        drawWorld();
        if(target.hit) {
            Vector3 q=Vector3Subtract(target.point,Vector3Scale(target.normal,0.01f));
            DrawCubeWires({floorf(q.x)+0.5f,floorf(q.y)+0.5f,floorf(q.z)+0.5f},1.03f,1.03f,1.03f,YELLOW);
        }
        EndMode3D();

        int w=GetScreenWidth(), h=GetScreenHeight();
        DrawText(creative?"CREATIVE":"SURVIVAL",18,16,22,WHITE);
        DrawText("SHADOWCRAFT",18,43,16,DARKBLUE);
        DrawText("WASD move | SPACE jump/fly | F mine | E place | M mode",18,h-28,14,WHITE);
        DrawCircleLines((int)(w*0.5f),(int)(h*0.5f),9,WHITE);
        // Touch action buttons
        Rectangle mine={w-150.0f,h-115.0f,62,62};
        Rectangle place={w-78.0f,h-185.0f,62,62};
        Rectangle mode={w-78.0f,h-105.0f,62,52};
        DrawCircle((int)(w*0.20f),(int)(h*0.78f),54,{255,255,255,45});
        DrawCircleLines((int)(w*0.20f),(int)(h*0.78f),54,WHITE);
        DrawRectangleRounded(mine,0.25f,8,{40,40,40,170});
        DrawRectangleRounded(place,0.25f,8,{40,100,40,190});
        DrawRectangleRounded(mode,0.25f,8,{30,60,120,190});
        DrawText("MINE",(int)mine.x+8,(int)mine.y+23,14,WHITE);
        DrawText("PLACE",(int)place.x+7,(int)place.y+23,13,WHITE);
        DrawText("MODE",(int)mode.x+8,(int)mode.y+18,13,WHITE);
        DrawText(TextFormat("Block: %d",selectedBlock),w/2-38,h-54,16,WHITE);
        DrawText("Tap block buttons: 1 Grass  2 Dirt  3 Stone  4 Wood",w/2-190,16,14,DARKBLUE);
        // Tap detection for action buttons and block selector.
        if(IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            Vector2 p=GetMousePosition();
            if(CheckCollisionPointRec(p,mine) && target.hit) {
                Vector3 q=Vector3Subtract(target.point,Vector3Scale(target.normal,0.02f));
                int idx=findBlock((int)floorf(q.x),(int)floorf(q.y),(int)floorf(q.z));
                if(idx>=0) world.erase(world.begin()+idx);
            } else if(CheckCollisionPointRec(p,place) && target.hit) {
                Vector3 q=Vector3Add(target.point,Vector3Scale(target.normal,0.55f));
                addBlock((int)floorf(q.x),(int)floorf(q.y),(int)floorf(q.z),selectedBlock);
            } else if(CheckCollisionPointRec(p,mode)) creative=!creative;
        }
        EndDrawing();
    }
    CloseWindow();
    return 0;
}
