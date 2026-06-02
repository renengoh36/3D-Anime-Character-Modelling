#define NOMINMAX
#include <Windows.h>
#include <Windowsx.h>
#ifndef GET_X_LPARAM
#  define GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#  define GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
#endif
#include <gl/GL.h>
#include <gl/GLU.h>
#include <algorithm>
#include <array>
#include <cmath>

#pragma comment (lib, "OpenGL32.lib")
#pragma comment (lib, "Glu32.lib")

// meshes
#include "onlyhat.h"
#include "onlybody.h"
#include "onlycloak.h"
#include "onlyclothes.h"
#include "onlyhair.h"
#include "onlykneecoverleft.h"
#include "onlykneecoverright.h"
#include "onlyleftarm.h"
#include "onlyleftfist.h"
#include "onlylefthandcover.h"
#include "onlylowerlegleft.h"
#include "onlylowerlegright.h"
#include "onlylowwaist.h"
#include "onlypants.h"
#include "onlyrightarm.h"
#include "onlyrightfist.h"
#include "onlyrighthandcover.h"
#include "onlyshoeleft.h"
#include "onlyshoeright.h"
#include "onlyweapon.h"
#include "upperleftleg.h"
#include "uprightleg.h"

#define WINDOW_TITLE "OpenGL Window"

// ---------- state ----------
static float gRotateX = 10.f, gRotateY = 0.0f, gZoom = -15.f, gViewHeight = 0.0f;
static bool  gDragging = false, gLighting = true, gShowGround = true;
static bool  gUseOrtho = false;
static POINT gLastMouse{};
static bool  gPlay = true;
static float gSpeed = 1.0f;

enum AnimMode { WALK = 0, IDLE = 1, ATTACK = 2, SPIN = 3 };
static AnimMode gMode = IDLE;
static bool  gAttackActive = false;

// ---------- sky colors (switch with 'F') ----------
static int currentSky = 0;
static const float skyColors[][3] = {
    {0.10f, 0.10f, 0.18f},  // night blue
    {0.53f, 0.81f, 0.92f},  // day sky
    {0.00f, 0.00f, 0.00f},  // midnight
    {0.90f, 0.50f, 0.20f}   // sunset
};

// ---------- textures ----------
static GLuint texHat[3] = { 0,0,0 };
static GLuint texShirt[3] = { 0,0,0 };
static GLuint texCloak[3] = { 0,0,0 };
static GLuint texBoots = 0;
static GLuint texGround = 0;
static int currentShirt = 0, currentHat = 0, currentCloak = 0;

static HBITMAP loadBMP(const char* path) {
    return (HBITMAP)LoadImage(GetModuleHandle(NULL), path, IMAGE_BITMAP, 0, 0,
        LR_CREATEDIBSECTION | LR_LOADFROMFILE);
}
static GLuint makeGLTexture(HBITMAP hbmp) {
    if (!hbmp) return 0;
    BITMAP bmp; GetObject(hbmp, sizeof(BITMAP), &bmp);
    GLuint id = 0; glGenTextures(1, &id); glBindTexture(GL_TEXTURE_2D, id);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, bmp.bmWidth, bmp.bmHeight,
        GL_BGR_EXT, GL_UNSIGNED_BYTE, bmp.bmBits);
    return id;
}
static GLuint loadTextureBMP(const char* path) {
    HBITMAP h = loadBMP(path); GLuint id = makeGLTexture(h); if (h) DeleteObject(h); return id;
}
static void bindIf(GLuint tex) { if (tex) { glEnable(GL_TEXTURE_2D); glBindTexture(GL_TEXTURE_2D, tex); } else glDisable(GL_TEXTURE_2D); }
static void unbindTex() { glDisable(GL_TEXTURE_2D); }

static void loadAllTexturesOnce() {
    static bool done = false; if (done) return; done = true;
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);

    glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

    texHat[0] = loadTextureBMP("textures/hat_texture.bmp");
    texHat[1] = loadTextureBMP("textures/hat_texture2.bmp");
    texHat[2] = loadTextureBMP("textures/hat_texture3.bmp");
    texShirt[0] = loadTextureBMP("textures/shirt_texture.bmp");
    texShirt[1] = loadTextureBMP("textures/shirt_texture2.bmp");
    texShirt[2] = loadTextureBMP("textures/shirt_texture3.bmp");
    texCloak[0] = loadTextureBMP("textures/cloak_texture1.bmp");
    texCloak[1] = loadTextureBMP("textures/cloak_texture2.bmp");
    texCloak[2] = loadTextureBMP("textures/cloak_texture3.bmp");
    texBoots = loadTextureBMP("textures/boots_texture.bmp");
    texGround = loadTextureBMP("textures/ground_texture1.bmp");
}

// ---------- parts ----------
struct Part { const char* name; const float (*V)[3]; const float (*N)[3]; const int (*F)[6]; int numFaces; };
enum PartId {
    BODY, UPPERLEG_L, UPPERLEG_R, LOWERLEG_L, LOWERLEG_R, KNEECOVER_L, KNEECOVER_R,
    PANTS, LOWWAIST, CLOTHES, CLOAK, ARM_L, ARM_R, HANDCOVER_L, HANDCOVER_R,
    FIST_L, FIST_R, SHOE_L, SHOE_R, HAT, HAIR, WEAPON, PART_COUNT
};
static Part parts[PART_COUNT] = {
    {"onlybody",          vertices_onlybody,          normals_onlybody,          faces_onlybody,          numFaces_onlybody},
    {"upperleftleg",      vertices_upperleftleg,      normals_upperleftleg,      faces_upperleftleg,      numFaces_upperleftleg},
    {"uprightleg",        vertices_uprightleg,        normals_uprightleg,        faces_uprightleg,        numFaces_uprightleg},
    {"onlylowerlegleft",  vertices_onlylowerlegleft,  normals_onlylowerlegleft,  faces_onlylowerlegleft,  numFaces_onlylowerlegleft},
    {"onlylowerlegright", vertices_onlylowerlegright, normals_onlylowerlegright, faces_onlylowerlegright, numFaces_onlylowerlegright},
    {"onlykneecoverleft", vertices_onlykneecoverleft, normals_onlykneecoverleft, faces_onlykneecoverleft, numFaces_onlykneecoverleft},
    {"onlykneecoverright",vertices_onlykneecoverright,normals_onlykneecoverright,faces_onlykneecoverright,numFaces_onlykneecoverright},
    {"onlypants",         vertices_onlypants,         normals_onlypants,         faces_onlypants,         numFaces_onlypants},
    {"onlylowwaist",      vertices_onlylowwaist,      normals_onlylowwaist,      faces_onlylowwaist,      numFaces_onlylowwaist},
    {"onlyclothes",       vertices_onlyclothes,       normals_onlyclothes,       faces_onlyclothes,       numFaces_onlyclothes},
    {"onlycloak",         vertices_onlycloak,         normals_onlycloak,         faces_onlycloak,         numFaces_onlycloak},
    {"onlyleftarm",       vertices_onlyleftarm,       normals_onlyleftarm,       faces_onlyleftarm,       numFaces_onlyleftarm},
    {"onlyrightarm",      vertices_onlyrightarm,      normals_onlyrightarm,      faces_onlyrightarm,      numFaces_onlyrightarm},
    {"onlylefthandcover", vertices_onlylefthandcover, normals_onlylefthandcover, faces_onlylefthandcover, numFaces_onlylefthandcover},
    {"onlyrighthandcover",vertices_onlyrighthandcover,normals_onlyrighthandcover,faces_onlyrighthandcover,numFaces_onlyrighthandcover},
    {"onlyleftfist",      vertices_onlyleftfist,      normals_onlyleftfist,      faces_onlyleftfist,      numFaces_onlyleftfist},
    {"onlyrightfist",     vertices_onlyrightfist,     normals_onlyrightfist,     faces_onlyrightfist,     numFaces_onlyrightfist},
    {"onlyshoeleft",      vertices_onlyshoeleft,      normals_onlyshoeleft,      faces_onlyshoeleft,      numFaces_onlyshoeleft},
    {"onlyshoeright",     vertices_onlyshoeright,     normals_onlyshoeright,     faces_onlyshoeright,     numFaces_onlyshoeright},
    {"onlyhat",           vertices_onlyhat,           normals_onlyhat,           faces_onlyhat,           numFaces_onlyhat},
    {"onlyhair",          vertices_onlyhair,          normals_onlyhair,          faces_onlyhair,          numFaces_onlyhair},
    {"onlyweapon",        vertices_onlyweapon,        normals_onlyweapon,        faces_onlyweapon,        numFaces_onlyweapon}
};

// ---------- bounds/pivots ----------
struct Vec3 { float x, y, z; };
struct BBox { Vec3 minv, maxv, center; float maxDim; };
static Vec3 gUpperLegOffsetL{ 0,0,0 }, gUpperLegOffsetR{ 0,0,0 };
static Vec3 gLowerLegOffsetL{ 0,0,0 }, gLowerLegOffsetR{ 0,0,0 };
static Vec3 gKneeCoverOffsetR{ 0,0,0 }, gKneeCoverOffsetL{ 0,0,0 };
static GLUquadric* gKneeQuad = nullptr; static float gKneeRadius = 0.20f;
static Vec3  gKneeJointOffsetR{ 0,0,0 }, gKneeJointOffsetL{ 0,0,0 };

static BBox computeBBox(const Part& p) {
    BBox b{}; int v0 = p.F[0][0];
    b.minv = b.maxv = { p.V[v0][0],p.V[v0][1],p.V[v0][2] };
    for (int i = 0; i < p.numFaces; ++i) {
        int v1 = p.F[i][0], v2 = p.F[i][2], v3 = p.F[i][4];
        const float* a = p.V[v1]; const float* c = p.V[v2]; const float* d = p.V[v3];
        const float pts[3][3] = { {a[0],a[1],a[2]},{c[0],c[1],c[2]},{d[0],d[1],d[2]} };
        for (int k = 0; k < 3; ++k) {
            b.minv.x = std::min(b.minv.x, pts[k][0]); b.maxv.x = std::max(b.maxv.x, pts[k][0]);
            b.minv.y = std::min(b.minv.y, pts[k][1]); b.maxv.y = std::max(b.maxv.y, pts[k][1]);
            b.minv.z = std::min(b.minv.z, pts[k][2]); b.maxv.z = std::max(b.maxv.z, pts[k][2]);
        }
    }
    b.center = { (b.minv.x + b.maxv.x) / 2.f,(b.minv.y + b.maxv.y) / 2.f,(b.minv.z + b.maxv.z) / 2.f };
    b.maxDim = std::max(b.maxv.x - b.minv.x, std::max(b.maxv.y - b.minv.y, b.maxv.z - b.minv.z));
    return b;
}
static BBox gPartBox[PART_COUNT], gAllBox; static float gScale = 1.f;
const BBox& wb = gPartBox[LOWWAIST];
const BBox& kbL = gPartBox[KNEECOVER_L];
const BBox& kbR = gPartBox[KNEECOVER_R];
static Vec3 pivotHipL, pivotHipR, pivotKneeL, pivotKneeR, pivotAnkleL, pivotAnkleR,
pivotShoulderL, pivotShoulderR, pivotWristL, pivotWristR, pivotHat;

static BBox computeGlobalBBox() {
    BBox b = gPartBox[BODY];
    for (int i = 0; i < PART_COUNT; ++i) {
        b.minv.x = std::min(b.minv.x, gPartBox[i].minv.x);
        b.minv.y = std::min(b.minv.y, gPartBox[i].minv.y);
        b.minv.z = std::min(b.minv.z, gPartBox[i].minv.z);
        b.maxv.x = std::max(b.maxv.x, gPartBox[i].maxv.x);
        b.maxv.y = std::max(b.maxv.y, gPartBox[i].maxv.y);
        b.maxv.z = std::max(b.maxv.z, gPartBox[i].maxv.z);
    }
    b.center = { (b.minv.x + b.maxv.x) / 2.f,(b.minv.y + b.maxv.y) / 2.f,(b.minv.z + b.maxv.z) / 2.f };
    b.maxDim = std::max(
        b.maxv.x - b.minv.x,
        std::max(b.maxv.y - b.minv.y, b.maxv.z - b.minv.z)
    );
    return b;
}

// ---------- tweak ranges ----------
static float gArmROffsetDeg = 0, gArmLOffsetDeg = 0, gFistRRollDeg = 0, gFistLRollDeg = 0;
static float gLegROffsetDeg = 0, gLegLOffsetDeg = 0, gKneeROffsetDeg = 0, gKneeLOffsetDeg = 0, gHatTiltDeg = 0;

// ---------- time ----------
static double nowSec() { static LARGE_INTEGER f{}; static bool inited = false; if (!inited) { QueryPerformanceFrequency(&f); inited = true; } LARGE_INTEGER t; QueryPerformanceCounter(&t); return double(t.QuadPart) / double(f.QuadPart); }

// ---------- projection & camera ----------
static void setupProjection(int w, int h) {
    if (h == 0) h = 1; glViewport(0, 0, w, h); glMatrixMode(GL_PROJECTION); glLoadIdentity();
    if (gUseOrtho) { const double s = 5.0; double a = (double)w / (double)h; glOrtho(-s * a, s * a, -s, s, -100, 100); }
    else { gluPerspective(45.0, (double)w / (double)h, 0.1, 100.0); }
    glMatrixMode(GL_MODELVIEW);
}
static void applyCameraAndCenter() {
    glTranslatef(0, 0, gZoom); glRotatef(gRotateX, 1, 0, 0); glRotatef(gRotateY, 0, 1, 0);
    glTranslatef(-gAllBox.center.x, -gAllBox.center.y, -gAllBox.center.z); glScalef(gScale, gScale, gScale);
}

// ---------- lighting ----------
static void applyLightingPreset() {
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    glFrontFace(GL_CW);
    if (!gLighting) { glDisable(GL_LIGHTING); return; }
    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    GLfloat amb[] = { 0.2f,0.2f,0.2f,1 }, diff[] = { 0.8f,0.8f,0.8f,1 }, pos[] = { 1,1,1,0 };
    glLightfv(GL_LIGHT0, GL_AMBIENT, amb);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diff);
    glLightfv(GL_LIGHT0, GL_POSITION, pos);
    glEnable(GL_LIGHT0);
    GLfloat spec[] = { 1,1,1,1 }; GLfloat shin[] = { 48 };
    glMaterialfv(GL_FRONT, GL_SPECULAR, spec);
    glMaterialfv(GL_FRONT, GL_SHININESS, shin);

}

// ---------- draw helpers ----------
static void drawPart(const Part& p) {
    glBegin(GL_TRIANGLES);
    for (int i = 0; i < p.numFaces; ++i) {
        int v1 = p.F[i][0], n1 = p.F[i][1];
        int v2 = p.F[i][2], n2 = p.F[i][3];
        int v3 = p.F[i][4], n3 = p.F[i][5];
        if (n1 >= 0) glNormal3fv(p.N[n1]); glVertex3fv(p.V[v1]);
        if (n2 >= 0) glNormal3fv(p.N[n2]); glVertex3fv(p.V[v2]);
        if (n3 >= 0) glNormal3fv(p.N[n3]); glVertex3fv(p.V[v3]);
    }
    glEnd();
}
static void drawTexturedGround(float half = 30.f, float tile = 10.f) {
    bindIf(texGround);
    glColor3f(1, 1, 1);
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glTexCoord2f(0, 0);       glVertex3f(-half, gAllBox.minv.y, -half);
    glTexCoord2f(tile, 0);    glVertex3f(half, gAllBox.minv.y, -half);
    glTexCoord2f(tile, tile); glVertex3f(half, gAllBox.minv.y, half);
    glTexCoord2f(0, tile);    glVertex3f(-half, gAllBox.minv.y, half);
    glEnd();
    unbindTex();
}
static void enableTexGenCylindricalY(float scaleS, float scaleT) {
    GLfloat sPlane[4] = { 1.0f / scaleS, 0.0f, 0.0f, 0.0f };
    GLfloat tPlane[4] = { 0.0f, 1.0f / scaleT, 0.0f, 0.0f };
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
    glTexGenfv(GL_S, GL_OBJECT_PLANE, sPlane);
    glTexGenfv(GL_T, GL_OBJECT_PLANE, tPlane);
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);
}
static void disableTexGen() { glDisable(GL_TEXTURE_GEN_S); glDisable(GL_TEXTURE_GEN_T); }

// ---------- anim curves ----------
static void walkParams(double ph, float& ll, float& lr, float& kl, float& kr, float& al, float& ar, float& wl, float& wr, float& bob) {
    ll = 25.f * sinf((float)ph); lr = -ll;
    kl = 15.f * std::max(0.f, sinf((float)ph));
    kr = 15.f * std::max(0.f, sinf((float)ph + 3.1416f));
    al = -20.f * sinf((float)ph); ar = -al;
    wl = 10.f * sinf((float)ph * 1.5f); wr = -wl;
    bob = 0.1f * sinf((float)ph * 2.f);
}
static void idleParams(double, float& ll, float& lr, float& kl, float& kr, float& al, float& ar, float& wl, float& wr, float& bob) {
    ll = lr = kl = kr = al = ar = wl = wr = bob = 0.0f;
}
static void attackParams(double t, float& ar, float& wr, float& twist, float& swing) {
    if (t < 0.3) { float u = (float)(t / 0.3); ar = -40 * u; wr = -20 * u; twist = -10 * u; swing = -60 * u; }
    else if (t < 0.8) { float u = (float)((t - 0.3) / 0.5); ar = -40 + 100 * u; wr = -20 + 60 * u; twist = -10 + 25 * u; swing = -60 + 220 * u; }
    else { float u = (float)((t - 0.8) / 0.2); ar = 60 * (1 - u); wr = 40 * (1 - u); twist = 15 * (1 - u); swing = 160 * (1 - u); }
}

// ---------- leg offset setters ----------
void adjustUpperLegPositions(float lx, float ly, float lz, float rx, float ry, float rz) { gUpperLegOffsetL = { lx,ly,lz }; gUpperLegOffsetR = { rx,ry,rz }; }
void adjustLowerLegPositions(float lx, float ly, float lz, float rx, float ry, float rz) { gLowerLegOffsetL = { lx,ly,lz }; gLowerLegOffsetR = { rx,ry,rz }; }

// ---------- draw character ----------
static void drawCharacter() {
    static double tPrev = nowSec(), walkPh = 0, idlePh = 0, atkStart = 0;
    double t = nowSec(), dt = t - tPrev; tPrev = t;

    if (gPlay) { walkPh += dt * gSpeed * 1.5; idlePh += dt * gSpeed * 0.8; }

    float legL = 0, legR = 0, kneeL = 0, kneeR = 0, armL = 0, armR = 0, wristL = 0, wristR = 0, bob = 0, twist = 0, swing = 0;
    if (gMode == WALK) {
        walkParams(walkPh, legL, legR, kneeL, kneeR, armL, armR, wristL, wristR, bob);
    }
    else if (gMode == IDLE) {
        idleParams(idlePh, legL, legR, kneeL, kneeR, armL, armR, wristL, wristR, bob);
    }
    else if (gMode == ATTACK) {
        if (!gAttackActive) { gAttackActive = true; atkStart = t; }
        double p = std::min(3.5, (t - atkStart) * gSpeed);
        attackParams(p, armR, wristR, twist, swing);
        float a, b, c, d, iBob; idleParams(idlePh, legL, legR, kneeL, kneeR, a, b, c, d, iBob);
        bob = iBob; armL = wristL = 0;
        if (p >= 1.0) { gMode = IDLE; gAttackActive = false; }
    }
    else if (gMode == SPIN) {
        if (!gAttackActive) { gAttackActive = true; atkStart = t; }

        double p = std::min(10.0, (t - atkStart) * gSpeed);
        float u = (float)(p / 1.2f);
        float s = u * u * (3.0f - 2.0f * u);

        twist = 350.0f * s;
        swing = 160.0f * sinf(u * 3.1415926f);
        armL = 0.0f; wristL = 0.0f;
        armR = -30.0f + 80.0f * s;
        wristR = 50.0f * s;
        legL = -10.0f * s; legR = 10.0f * s;
        kneeL = kneeR = 8.0f * s;
        bob = -0.06f * sinf(u * 3.1415926f);

        if (p >= 1.2) { gMode = IDLE; gAttackActive = false; }
    }
    legR += gLegROffsetDeg; legL += gLegLOffsetDeg;

    // Belt & adornment parameters 
    const float ww = wb.maxv.x - wb.minv.x;
    const float dzW = wb.maxv.z - wb.minv.z;
    const float dxK = kbL.maxv.x - kbL.minv.x;
    const float dzK = kbL.maxv.z - kbL.minv.z;

    // Buckle
    const float buckleR = 0.18f * ww;
    const float buckleH = 0.06f * dzW + 0.02f;
    const float frontZ = wb.maxv.z + 0.02f;
    const float eps = 0.002f;

    // Knee spikes
    const float spikeR = 0.09f * dxK;
    const float spikeLen = 0.25f * dzK;

    // Buckle decal (diamond/square)
    const float squareS = 0.55f * buckleR;

    // Side buttons
    const float buttonR = 0.35f * buckleR;
    const float buttonH = 0.50f * buckleH;
    const float gap = 0.02f * ww;
    const float offsetX = buckleR + buttonR + gap;
    const float zEps = 0.0015f;

    glLoadIdentity();
    glTranslatef(0, gViewHeight, gZoom);
    glRotatef(gRotateX, 1, 0, 0);
    glRotatef(gRotateY, 0, 1, 0);
    glTranslatef(-gPartBox[BODY].center.x, -gPartBox[BODY].center.y, -gPartBox[BODY].center.z);
    glScalef(gScale, gScale, gScale);

    if (gShowGround) drawTexturedGround();

    glPushMatrix();
    glTranslatef(0, bob, 0);
    glRotatef(twist, 0, 1, 0);

    glColor3f(0.878f, 0.690f, 0.518f); drawPart(parts[BODY]);

    // LEFT LEG
    glPushMatrix();
    glTranslatef(gUpperLegOffsetL.x, gUpperLegOffsetL.y, gUpperLegOffsetL.z);
    glTranslatef(pivotHipL.x, pivotHipL.y, pivotHipL.z);
    glRotatef(legL, 1, 0, 0);
    glTranslatef(-pivotHipL.x, -pivotHipL.y, -pivotHipL.z);
    glColor3f(0, 0, 0); drawPart(parts[UPPERLEG_L]); // black
    glTranslatef(pivotKneeL.x, pivotKneeL.y, pivotKneeL.z);
    glRotatef(kneeL + gKneeLOffsetDeg, 1, 0, 0);
    glTranslatef(gLowerLegOffsetL.x, gLowerLegOffsetL.y, gLowerLegOffsetL.z);
    glTranslatef(-pivotKneeL.x, -pivotKneeL.y, -pivotKneeL.z);

    // lower leg uses boots texture (TexGen)
    bindIf(texBoots);
    enableTexGenCylindricalY(2.0f, 2.5f);
    glColor3f(1, 1, 1);
    drawPart(parts[LOWERLEG_L]);
    disableTexGen();
    unbindTex();

    // knee cover and knee sphere
    glPushMatrix();
    glTranslatef(pivotKneeL.x, pivotKneeL.y, pivotKneeL.z);
    glTranslatef(gKneeCoverOffsetL.x, gKneeCoverOffsetL.y, gKneeCoverOffsetL.z);
    glTranslatef(-pivotKneeL.x, -pivotKneeL.y, -pivotKneeL.z);
    glColor3f(0.35f, 0.20f, 0.08f);
    drawPart(parts[KNEECOVER_L]);
    glPopMatrix();

    // Knee spike
    glPushMatrix();
    glTranslatef(pivotKneeL.x, pivotKneeL.y, pivotKneeL.z);
    glTranslatef(gKneeCoverOffsetL.x, gKneeCoverOffsetL.y, gKneeCoverOffsetL.z);
    glTranslatef(-pivotKneeL.x, -pivotKneeL.y, -pivotKneeL.z);
    glTranslatef(kbL.center.x, kbL.center.y, kbL.maxv.z);
    glDisable(GL_TEXTURE_2D);
    glColor3f(0.80f, 0.80f, 0.85f); // steel-ish
    gluCylinder(gKneeQuad, spikeR, 0.0, spikeLen, 16, 1);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(pivotKneeL.x, pivotKneeL.y, pivotKneeL.z);
    glTranslatef(+0.415f, 1.46f, -0.00f);
    glTranslatef(gKneeJointOffsetL.x, gKneeJointOffsetL.y, gKneeJointOffsetL.z);
    glTranslatef(-pivotKneeL.x, -pivotKneeL.y, -pivotKneeL.z);
    bindIf(texBoots);
    enableTexGenCylindricalY(2.0f, 2.0f);
    glColor3f(1, 1, 1);
    gluSphere(gKneeQuad, gKneeRadius, 20, 20);
    disableTexGen();
    unbindTex();
    glPopMatrix();

    glTranslatef(pivotAnkleL.x, pivotAnkleL.y, pivotAnkleL.z);
    glRotatef(-kneeL * 0.5f, 1, 0, 0);
    glTranslatef(-pivotAnkleL.x, -pivotAnkleL.y, -pivotAnkleL.z);
    bindIf(texBoots); glColor3f(1, 1, 1); drawPart(parts[SHOE_L]); unbindTex();
    glPopMatrix();

    // RIGHT LEG
    glPushMatrix();
    glTranslatef(gUpperLegOffsetR.x, gUpperLegOffsetR.y, gUpperLegOffsetR.z);
    glTranslatef(pivotHipR.x, pivotHipR.y, pivotHipR.z);
    glRotatef(legR, 1, 0, 0);
    glTranslatef(-pivotHipR.x, -pivotHipR.y, -pivotHipR.z);
    glColor3f(0, 0, 0); drawPart(parts[UPPERLEG_R]); // black
    glTranslatef(pivotKneeR.x, pivotKneeR.y, pivotKneeR.z);
    glRotatef(kneeR + gKneeROffsetDeg, 1, 0, 0);
    glTranslatef(gLowerLegOffsetR.x, gLowerLegOffsetR.y, gLowerLegOffsetR.z);
    glTranslatef(-pivotKneeR.x, -pivotKneeR.y, -pivotKneeR.z);

    bindIf(texBoots);
    enableTexGenCylindricalY(2.0f, 2.5f);
    glColor3f(1, 1, 1);
    drawPart(parts[LOWERLEG_R]);
    disableTexGen();
    unbindTex();

    glPushMatrix();
    glTranslatef(pivotKneeR.x, pivotKneeR.y, pivotKneeR.z);
    glTranslatef(gKneeCoverOffsetR.x, gKneeCoverOffsetR.y, gKneeCoverOffsetR.z);
    glTranslatef(-pivotKneeR.x, -pivotKneeR.y, -pivotKneeR.z);
    glColor3f(0.35f, 0.20f, 0.08f);
    drawPart(parts[KNEECOVER_R]);
    glPopMatrix();

    // Knee Spike 
    glPushMatrix();
    glTranslatef(pivotKneeR.x, pivotKneeR.y, pivotKneeR.z);
    glTranslatef(gKneeCoverOffsetR.x, gKneeCoverOffsetR.y, gKneeCoverOffsetR.z);
    glTranslatef(-pivotKneeR.x, -pivotKneeR.y, -pivotKneeR.z);

    glTranslatef(kbR.center.x, kbR.center.y, kbR.maxv.z);

    glDisable(GL_TEXTURE_2D);
    glColor3f(0.80f, 0.80f, 0.85f);
    gluCylinder(gKneeQuad, spikeR, 0.0, spikeLen, 16, 1);
    glPopMatrix();

    glPushMatrix();
    glTranslatef(pivotKneeR.x, pivotKneeR.y, pivotKneeR.z);
    glTranslatef(-0.45f, 1.45f, -0.02f);
    glTranslatef(gKneeJointOffsetR.x, gKneeJointOffsetR.y, gKneeJointOffsetR.z);
    glTranslatef(-pivotKneeR.x, -pivotKneeR.y, -pivotKneeR.z);
    bindIf(texBoots);
    enableTexGenCylindricalY(2.0f, 2.0f);
    glColor3f(1, 1, 1);
    gluSphere(gKneeQuad, gKneeRadius, 20, 20);
    disableTexGen();
    unbindTex();
    glPopMatrix();

    glTranslatef(pivotAnkleR.x, pivotAnkleR.y, pivotAnkleR.z);
    glRotatef(-(kneeR + gKneeROffsetDeg) * 0.5f, 1, 0, 0);
    glTranslatef(-pivotAnkleR.x, -pivotAnkleR.y, -pivotAnkleR.z);
    bindIf(texBoots); glColor3f(1, 1, 1); drawPart(parts[SHOE_R]); unbindTex();
    glPopMatrix();

    // Pants BLACK
    glColor3f(0, 0, 0); drawPart(parts[PANTS]);

    // Belt/low waist color
    glColor3f(0.231f, 0.165f, 0.102f); drawPart(parts[LOWWAIST]);

    //Belt buckle
    glPushMatrix();
    glColor3f(0.85f, 0.70f, 0.25f);
    glTranslatef(wb.center.x, wb.center.y, frontZ);
    gluCylinder(gKneeQuad, buckleR, buckleR, buckleH, 24, 1);
    glTranslatef(0.0f, 0.0f, buckleH);
    gluDisk(gKneeQuad, 0.0, buckleR, 24, 1);
    glPopMatrix();

    glDisable(GL_TEXTURE_2D);
    glColor3f(0.85f, 0.70f, 0.25f);

    for (int s = -1; s <= 1; s += 2) {
        glPushMatrix();
        glTranslatef(wb.center.x + s * offsetX, wb.center.y, frontZ + zEps);
        gluCylinder(gKneeQuad, buttonR, buttonR, buttonH, 20, 1);
        glTranslatef(0, 0, buttonH);
        gluDisk(gKneeQuad, 0.0, buttonR, 20, 1);
        glPopMatrix();
    }

    //Belt buckle design 
    glPushMatrix();
    glDisable(GL_TEXTURE_2D);
    glColor3f(0.8f, 0.0f, 0.0f);
    glTranslatef(wb.center.x, wb.center.y, frontZ + buckleH + eps);
    glRotatef(45.f, 0.f, 0.f, 1.f);
    glBegin(GL_QUADS);
    glNormal3f(0.f, 0.f, 1.f);
    glVertex3f(-squareS, -squareS, 0.f);
    glVertex3f(squareS, -squareS, 0.f);
    glVertex3f(squareS, squareS, 0.f);
    glVertex3f(-squareS, squareS, 0.f);
    glEnd();
    glPopMatrix();

    // Shirt texture (TexGen)
    bindIf(texShirt[currentShirt]);
    enableTexGenCylindricalY(3.0f, 3.0f);
    glColor3f(1, 1, 1);
    drawPart(parts[CLOTHES]);
    disableTexGen();
    unbindTex();

    // Cloak texture (TexGen)
    bindIf(texCloak[currentCloak]);
    enableTexGenCylindricalY(4.0f, 5.0f);
    glColor3f(1, 1, 1);
    drawPart(parts[CLOAK]);
    disableTexGen();
    unbindTex();

    // Arms & hands
    glPushMatrix();
    glTranslatef(pivotShoulderL.x, pivotShoulderL.y, pivotShoulderL.z);
    glRotatef(armL + gArmLOffsetDeg, 1, 0, 0);
    glTranslatef(-pivotShoulderL.x, -pivotShoulderL.y, -pivotShoulderL.z);
    glColor3f(0.878f, 0.690f, 0.518f); drawPart(parts[ARM_L]);
    glTranslatef(pivotWristL.x, pivotWristL.y, pivotWristL.z); glRotatef(wristL, 1, 0, 0);
    glPushMatrix(); glTranslatef(-pivotWristL.x, -pivotWristL.y, -pivotWristL.z);
    glColor3f(0.55f, 0.27f, 0.07f); drawPart(parts[HANDCOVER_L]);
    glPopMatrix();
    glPushMatrix(); glRotatef(gFistLRollDeg, 1, 0, 0); glTranslatef(-pivotWristL.x, -pivotWristL.y, -pivotWristL.z);
    glColor3f(0.90f, 0.70f, 0.70f); drawPart(parts[FIST_L]);
    glPopMatrix();
    glPopMatrix();

    glPushMatrix();
    glTranslatef(pivotShoulderR.x, pivotShoulderR.y, pivotShoulderR.z);
    glRotatef(armR + gArmROffsetDeg, 1, 0, 0);
    glTranslatef(-pivotShoulderR.x, -pivotShoulderR.y, -pivotShoulderR.z);
    glColor3f(0.878f, 0.690f, 0.518f); drawPart(parts[ARM_R]);
    glTranslatef(pivotWristR.x, pivotWristR.y, pivotWristR.z); glRotatef(wristR, 1, 0, 0);

    if (gMode == ATTACK) {
        glEnable(GL_LIGHT1);
        GLfloat pos[4] = { 0,0,0,1 }, dir[3] = { 0,-0.3f,-1.0f }, d[4] = { 0.9f,0.9f,0.8f,1 }, a[4] = { 0.05f,0.05f,0.05f,1 };
        glLightfv(GL_LIGHT1, GL_POSITION, pos);
        glLightfv(GL_LIGHT1, GL_SPOT_DIRECTION, dir);
        glLightf(GL_LIGHT1, GL_SPOT_CUTOFF, 22.0f);
        glLightf(GL_LIGHT1, GL_SPOT_EXPONENT, 15.0f);
        glLightfv(GL_LIGHT1, GL_DIFFUSE, d);
        glLightfv(GL_LIGHT1, GL_AMBIENT, a);
    }
    else glDisable(GL_LIGHT1);

    glPushMatrix(); glRotatef(gFistRRollDeg, 1, 0, 0); glTranslatef(-pivotWristR.x, -pivotWristR.y, -pivotWristR.z);
    glColor3f(0.90f, 0.70f, 0.70f); drawPart(parts[FIST_R]);
    glPopMatrix();
    glPushMatrix(); glTranslatef(-pivotWristR.x, -pivotWristR.y, -pivotWristR.z);
    glColor3f(0.55f, 0.27f, 0.07f); drawPart(parts[HANDCOVER_R]);
    glPopMatrix();
    glPushMatrix(); glRotatef(gFistRRollDeg, 1, 0, 0); glRotatef(swing, 1, 0, 0); glTranslatef(-pivotWristR.x, -pivotWristR.y, -pivotWristR.z);
    glColor3f(0.184f, 0.184f, 0.184f); drawPart(parts[WEAPON]);
    glPopMatrix();

    glPopMatrix();

    //Shoulder pads (spheres) 
    glPushMatrix();
    glColor3f(0.878f, 0.690f, 0.518f);
    glTranslatef(pivotShoulderL.x, pivotShoulderL.y, pivotShoulderL.z);

    glTranslatef(-0.07f, -0.1f, -0.08f);
    gluSphere(gKneeQuad, 0.18f, 16, 16);
    glPopMatrix();

    glPushMatrix();
    glColor3f(0.878f, 0.690f, 0.518f);
    glTranslatef(pivotShoulderR.x, pivotShoulderR.y, pivotShoulderR.z);
    glTranslatef(0.20f, -0.12f, -0.03f);
    gluSphere(gKneeQuad, 0.195f, 16, 16);
    glPopMatrix();

    // Hat (TexGen) + hair
    glPushMatrix();
    glTranslatef(pivotHat.x, pivotHat.y, pivotHat.z);
    glRotatef(gHatTiltDeg, 1, 0, 0);
    glTranslatef(-pivotHat.x, -pivotHat.y, -pivotHat.z);
    bindIf(texHat[currentHat]);
    enableTexGenCylindricalY(5.0f, 2.0f);
    glColor3f(1, 1, 1);
    drawPart(parts[HAT]);
    disableTexGen();
    unbindTex();
    glPopMatrix();

    glColor3f(0.35f, 0.35f, 0.40f); drawPart(parts[HAIR]);

    glPopMatrix();
}

// ---------- input ----------
LRESULT WINAPI WindowProcedure(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_DESTROY: PostQuitMessage(0); break;
    case WM_SIZE: { RECT rc; GetClientRect(hWnd, &rc); setupProjection(rc.right - rc.left, rc.bottom - rc.top); } break;
    case WM_LBUTTONDOWN: gDragging = true; gLastMouse.x = GET_X_LPARAM(lParam); gLastMouse.y = GET_Y_LPARAM(lParam); SetCapture(hWnd); break;
    case WM_LBUTTONUP:   gDragging = false; ReleaseCapture(); break;
    case WM_MOUSEMOVE:
        if (gDragging) {
            int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);
            gRotateY += (x - gLastMouse.x) * 0.4f; gRotateX += (y - gLastMouse.y) * 0.4f; gLastMouse.x = x; gLastMouse.y = y;
        }
        break;
    case WM_MOUSEWHEEL: { short d = GET_WHEEL_DELTA_WPARAM(wParam); gZoom += (d > 0 ? -0.6f : 0.6f); } break;

    case WM_KEYDOWN:
        if (wParam == VK_ESCAPE) PostQuitMessage(0);
        switch (wParam) {
        case VK_LEFT:  gRotateY -= 5.f; break;  case VK_RIGHT: gRotateY += 5.f; break;
        case VK_UP:    gRotateX -= 5.f; break;  case VK_DOWN:  gRotateX += 5.f; break;
        case 'Z': gViewHeight += 0.3f; break;
        case 'X': gViewHeight -= 0.3f; break;
        case 'P': gPlay = !gPlay; break;
        case VK_OEM_PLUS: case '=': gSpeed = std::min(3.0f, gSpeed + 0.1f); break;
        case VK_OEM_MINUS: case '-': gSpeed = std::max(0.1f, gSpeed - 0.1f); break;
        case '1': gMode = WALK; break;          case '2': gMode = IDLE; break;
        case '3': if (gMode != ATTACK || !gAttackActive) { gMode = ATTACK; gAttackActive = false; } break;
        case '4': if (gMode != SPIN || !gAttackActive) { gMode = SPIN; gAttackActive = false; } break;
        case 'T': gArmROffsetDeg = std::min(gArmROffsetDeg + 5.0f, 30.0f); break;
        case 'G': gArmROffsetDeg = std::max(gArmROffsetDeg - 5.0f, -30.0f); break;
        case 'Y': gArmLOffsetDeg = std::min(gArmLOffsetDeg + 5.0f, 30.0f); break;
        case 'H': gArmLOffsetDeg = std::max(gArmLOffsetDeg - 5.0f, -30.0f); break;
        case 'I': gFistRRollDeg = std::min(gFistRRollDeg + 2.0f, 15.0f); break;
        case 'K': gFistRRollDeg = std::max(gFistRRollDeg - 2.0f, -15.0f); break;
        case 'U': gFistLRollDeg = std::min(gFistLRollDeg + 2.0f, 15.0f); break;
        case 'J': gFistLRollDeg = std::max(gFistLRollDeg - 2.0f, -15.0f); break;
        case 'V': gLegROffsetDeg = std::min(gLegROffsetDeg + 2.0f, 30.0f); break;
        case 'B': gLegROffsetDeg = std::max(gLegROffsetDeg - 2.0f, -20.0f); break;
        case 'N': gLegLOffsetDeg = std::min(gLegLOffsetDeg + 2.0f, 30.0f); break;
        case 'M': gLegLOffsetDeg = std::max(gLegLOffsetDeg - 2.0f, -20.0f); break;
        case VK_OEM_COMMA:  gKneeROffsetDeg = std::min(gKneeROffsetDeg + 2.0f, 12.0f); break;
        case VK_OEM_PERIOD: gKneeROffsetDeg = std::max(gKneeROffsetDeg - 2.0f, 0.0f); break;
        case VK_OEM_4:      gKneeLOffsetDeg = std::min(gKneeLOffsetDeg + 2.0f, 15.0f); break;
        case VK_OEM_6:      gKneeLOffsetDeg = std::max(gKneeLOffsetDeg - 2.0f, 0.0f); break;
        case 'Q': gHatTiltDeg = std::min(gHatTiltDeg + 3.0f, 30.0f); break;
        case 'E': gHatTiltDeg = std::max(gHatTiltDeg - 3.0f, -16.0f); break;
        case 'O': gUseOrtho = !gUseOrtho; { RECT rc; GetClientRect(hWnd, &rc); setupProjection(rc.right - rc.left, rc.bottom - rc.top); } break;
        case 'R': gShowGround = !gShowGround; break;
        case 'L': gLighting = !gLighting; applyLightingPreset(); break;

            // texture switching
        case VK_F1: currentShirt = 0; break;
        case VK_F2: currentShirt = 1; break;
        case VK_F3: currentShirt = 2; break;
        case VK_F4: currentHat = 0; break;
        case VK_F5: currentHat = 1; break;
        case VK_F6: currentHat = 2; break;
        case VK_F7: currentCloak = 0; break;
        case VK_F8: currentCloak = 1; break;
        case VK_F9: currentCloak = 2; break;

            // sky switching
        case 'F': currentSky = (currentSky + 1) % (int)(sizeof(skyColors) / sizeof(skyColors[0])); break;
        }
        break;
    default: break;
    }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

// ---------- GL setup ----------
bool initPixelFormat(HDC hdc) {
    PIXELFORMATDESCRIPTOR pfd; ZeroMemory(&pfd, sizeof(pfd));
    pfd.cAlphaBits = 8; pfd.cColorBits = 32; pfd.cDepthBits = 24; pfd.cStencilBits = 0;
    pfd.dwFlags = PFD_DOUBLEBUFFER | PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW;
    pfd.iLayerType = PFD_MAIN_PLANE; pfd.iPixelType = PFD_TYPE_RGBA; pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR); pfd.nVersion = 1;
    int n = ChoosePixelFormat(hdc, &pfd); return SetPixelFormat(hdc, n, &pfd) ? true : false;
}

// ---------- frame ----------
void display() {
    const float* sc = skyColors[currentSky];
    glClearColor(sc[0], sc[1], sc[2], 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    drawCharacter();
}

// ---------- entry ----------
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int nCmdShow)
{
    for (int i = 0; i < PART_COUNT; ++i) gPartBox[i] = computeBBox(parts[i]);
    gAllBox = computeGlobalBBox(); gScale = (gAllBox.maxDim > 0) ? (8.f / gAllBox.maxDim) : 1.f;

    pivotHipL = { gPartBox[UPPERLEG_L].center.x,gPartBox[UPPERLEG_L].maxv.y,gPartBox[UPPERLEG_L].center.z };
    pivotHipR = { gPartBox[UPPERLEG_R].center.x,gPartBox[UPPERLEG_R].maxv.y,gPartBox[UPPERLEG_R].center.z };
    pivotKneeL = { gPartBox[LOWERLEG_L].center.x,gPartBox[LOWERLEG_L].maxv.y,gPartBox[LOWERLEG_L].center.z };
    pivotKneeR = { gPartBox[LOWERLEG_R].center.x,gPartBox[LOWERLEG_R].maxv.y,gPartBox[LOWERLEG_R].center.z };
    pivotAnkleL = { gPartBox[LOWERLEG_L].center.x,gPartBox[LOWERLEG_L].minv.y,gPartBox[LOWERLEG_L].center.z };
    pivotAnkleR = { gPartBox[LOWERLEG_R].center.x,gPartBox[LOWERLEG_R].minv.y,gPartBox[LOWERLEG_R].center.z };
    pivotHat = { gPartBox[HAT].center.x,gPartBox[HAT].minv.y,gPartBox[HAT].center.z };

    adjustUpperLegPositions(-0.87f, 0.0f, 0.0f, 1.90f, 0.3f, 1.10f);
    adjustLowerLegPositions(0.90f, 0.05f, 0.00f, -1.90f, -0.30f, -1.05f);

    const BBox& kbR = gPartBox[KNEECOVER_R]; const BBox& kbL = gPartBox[KNEECOVER_L];
    gKneeCoverOffsetR.x = pivotKneeR.x - kbR.center.x + 0.05f;
    gKneeCoverOffsetR.y = pivotKneeR.y - kbR.center.y + 0.10f * (kbR.maxv.y - kbR.minv.y);
    gKneeCoverOffsetR.z = pivotKneeR.z - kbR.center.z + 0.08f;
    gKneeCoverOffsetL.x = pivotKneeL.x - kbL.center.x - 0.05f;
    gKneeCoverOffsetL.y = pivotKneeL.y - kbL.center.y + 0.10f * (kbL.maxv.y - kbL.minv.y);
    gKneeCoverOffsetL.z = pivotKneeL.z - kbL.center.z + 0.08f;

    Vec3 bC = gPartBox[BODY].center; float mix = 0.01f, inward = 0.01f * gAllBox.maxDim;
    Vec3 aCR = gPartBox[ARM_R].center; Vec3 pR = { aCR.x * (1 - mix) + bC.x * mix,gPartBox[ARM_R].maxv.y,aCR.z * (1 - mix) + bC.z * mix };
    Vec3 dirR = { bC.x - aCR.x,0,bC.z - aCR.z }; float lenR = std::sqrt(dirR.x * dirR.x + dirR.z * dirR.z);
    if (lenR > 1e-6f) { dirR.x /= lenR; dirR.z /= lenR; pR.x += dirR.x * inward; pR.z += dirR.z * inward; } pivotShoulderR = pR;
    Vec3 aCL = gPartBox[ARM_L].center; Vec3 pL = { aCL.x * (1 - mix) + bC.x * mix,gPartBox[ARM_L].maxv.y,aCL.z * (1 - mix) + bC.z * mix };
    Vec3 dirL = { bC.x - aCL.x,0,bC.z - aCL.z }; float lenL = std::sqrt(dirL.x * dirL.x + dirL.z * dirL.z);
    if (lenL > 1e-6f) { dirL.x /= lenL; dirL.z /= lenL; pL.x += dirL.x * inward; pL.z += dirL.z * inward; } pivotShoulderL = pL;
    pivotWristL = { gPartBox[ARM_L].center.x,gPartBox[ARM_L].minv.y,gPartBox[ARM_L].center.z };
    pivotWristR = { gPartBox[ARM_R].center.x,gPartBox[ARM_R].minv.y,gPartBox[ARM_R].center.z };

    WNDCLASSEX wc{}; wc.cbSize = sizeof(WNDCLASSEX); wc.hInstance = GetModuleHandle(NULL);
    wc.lpfnWndProc = WindowProcedure; wc.lpszClassName = WINDOW_TITLE; wc.style = CS_HREDRAW | CS_VREDRAW;
    if (!RegisterClassEx(&wc)) return false;
    HWND hWnd = CreateWindow(WINDOW_TITLE, WINDOW_TITLE, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 1280, 900, NULL, NULL, wc.hInstance, NULL);

    HDC hdc = GetDC(hWnd);
    if (!initPixelFormat(hdc)) return false;
    HGLRC hglrc = wglCreateContext(hdc);
    if (!wglMakeCurrent(hdc, hglrc)) return false;

    RECT rc; GetClientRect(hWnd, &rc); setupProjection(rc.right - rc.left, rc.bottom - rc.top);
    applyLightingPreset();

    gKneeQuad = gluNewQuadric(); gluQuadricNormals(gKneeQuad, GLU_SMOOTH); gluQuadricDrawStyle(gKneeQuad, GLU_FILL);

    ShowWindow(hWnd, nCmdShow);
    loadAllTexturesOnce();

    // first display instructions
    MessageBoxA(
        hWnd,
        "Mouse:\r\n"
        " • Drag = rotate\r\n"
        " • Wheel / Z / X = view height\r\n"
        "\r\n"
        "Modes:\r\n"
        " • 1 = Walk\r\n"
        " • 2 = Idle\r\n"
        " • 3 = Attack (spotlight)\r\n"
        " • 4 = Spin \r\n"
        " • P = Play/Pause animation\r\n"
        "\r\n"
        "Camera & View:\r\n"
        " • F = Cycle sky color\r\n"
        " • +/- = Speed\r\n"
        " • O = Ortho/Persp\r\n"
        " • L = Toggle lighting\r\n"
        " • R = Toggle ground\r\n"
        "\r\n"
        "Body Controls:\r\n"
        " • Arms Right T/G, Left Y/H\r\n"
        " • Fists Right I/K, Left U/J\r\n"
        " • Legs Right V/B, Left N/M\r\n"
        " • Knees Right ',' '.' | Left '[' ']'\r\n"
        " • Hat tilt Q/E\r\n"
        "\r\n"
        "Textures:\r\n"
        " • Shirt F1/F2/F3\r\n"
        " • Hat F4/F5/F6\r\n"
        " • Cloak F7/F8/F9\r\n",
        "Controls",
        MB_OK | MB_ICONINFORMATION);

    MSG msg{}; while (true) {
        if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) { if (msg.message == WM_QUIT) break; TranslateMessage(&msg); DispatchMessage(&msg); }
        const float* sc = skyColors[currentSky];
        glClearColor(sc[0], sc[1], sc[2], 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        drawCharacter();
        SwapBuffers(hdc);
    }
    UnregisterClass(WINDOW_TITLE, wc.hInstance);
    return true;
}
