#include "CameraManager.h"
#include <algorithm>
#include <cmath>

CameraManager::CameraManager(flixel::FlxCamera* camGame,
                             flixel::FlxCamera* camHUD,
                             float defaultZoom)
    : camGame(camGame)
    , camHUD(camHUD)
    , camFollow(nullptr)
    , boyfriend(nullptr)
    , dad(nullptr)
    , gf(nullptr)
    , defaultCamZoom(defaultZoom)
    , cameraZoomTarget(defaultZoom)
    , cameraZoomTweenSpeed(0.0f)
    , camZooming(false)
{
}

CameraManager::~CameraManager() {
    if (camFollow) {
        delete camFollow;
        camFollow = nullptr;
    }
}

void CameraManager::initialize(Character* bf, Character* dadChar, Character* gfChar) {
    boyfriend = bf;
    dad       = dadChar;
    gf        = gfChar;

    if (boyfriend && camGame) {
        camFollow = new flixel::FlxObject(
            boyfriend->getMidpoint().x - 100,
            boyfriend->getMidpoint().y - 100,
            1, 1);
        camGame->follow(camFollow, flixel::FlxCameraFollowStyle::LOCKON, 0.04f);
        camGame->focusOn(camFollow->getMidpoint());
    }
}

void CameraManager::update(float elapsed) {
    if (!camZooming || !camGame) return;

    const float lerpFactor = 1.0f - std::pow(0.05f, elapsed);

    if (!zoomTweenActive) {
        camGame->zoom += (cameraZoomTarget - camGame->zoom) * lerpFactor;
    }

    if (camHUD) {
        camHUD->zoom += (1.0f - camHUD->zoom) * lerpFactor;
    }
}

void CameraManager::beatHit(int beat) {
    if (!camZooming || !camGame) return;

    if (cameraBopRate <= 0.0f) return;

    float phase = std::fmod(static_cast<float>(beat) - cameraBopOffset, cameraBopRate);
    if (phase < 0.0f) {
        phase += cameraBopRate;
    }

    if (phase < 0.001f || std::fabs(phase - cameraBopRate) < 0.001f) {
        camGame->zoom += cameraBopIntensity * 0.015f;
        if (camHUD) {
            camHUD->zoom += cameraBopIntensity * 0.03f;
        }
    }
}

void CameraManager::resolveFocusTarget(int charTarget, float xOff, float yOff, float& tx, float& ty) const {
    tx = xOff;
    ty = yOff;
    switch (charTarget) {
        case 0: // bf
            if (boyfriend) {
                tx += boyfriend->getMidpoint().x - 100;
                ty += boyfriend->getMidpoint().y - 100;
            }
            break;
        case 1: // papi
            if (dad) {
                tx += dad->getMidpoint().x + 150;
                ty += dad->getMidpoint().y - 100;
            }
            break;
        case 2: // bad bih
            if (gf) {
                tx += gf->getMidpoint().x;
                ty += gf->getMidpoint().y - 50;
            }
            break;
        case -1:
        default:
            break;
    }
}

void CameraManager::focusOn(int charTarget, float xOff, float yOff) {
    if (!camFollow) return;

    float tx = 0.0f;
    float ty = 0.0f;
    resolveFocusTarget(charTarget, xOff, yOff, tx, ty);

    camFollow->setPosition(tx, ty);
}

void CameraManager::focusOn(int charTarget, float xOff, float yOff, float durationSeconds,
                            flixel::tweens::EaseFunction ease, bool classic) {
    if (!camFollow) return;

    float tx = 0.0f;
    float ty = 0.0f;
    resolveFocusTarget(charTarget, xOff, yOff, tx, ty);

    flixel::tweens::cancelTweensOf(camFollow);

    if (classic || durationSeconds <= 0.0f) {
        camFollow->setPosition(tx, ty);
        return;
    }

    auto* t = new flixel::tweens::VarTween(durationSeconds);
    t->object = camFollow;
    t->ease = ease ? ease : flixel::tweens::FlxEase::linear;
    t->addProperty("x", &camFollow->x, tx);
    t->addProperty("y", &camFollow->y, ty);
    t->start();

    if (!flixel::tweens::globalManager) {
        flixel::tweens::init();
    }
    flixel::tweens::globalManager->tweens.push_back(t);
}

void CameraManager::tweenZoom(float targetZoom, float durationSeconds, bool direct,
                              flixel::tweens::EaseFunction ease) {
    if (!direct) {
        targetZoom = defaultCamZoom * targetZoom;
    }
    cameraZoomTarget = targetZoom;

    if (!camGame) return;

    flixel::tweens::cancelTweensOf(camGame);

    if (durationSeconds <= 0.0f) {
        camGame->zoom = targetZoom;
        zoomTweenActive = false;
        return;
    }

    auto* t = new flixel::tweens::VarTween(durationSeconds);
    t->object = camGame;
    t->ease   = ease ? ease : flixel::tweens::FlxEase::linear;
    t->addProperty("zoom", &camGame->zoom, targetZoom);
    t->onComplete = [this]() { zoomTweenActive = false; };
    t->start();

    if (!flixel::tweens::globalManager) {
        flixel::tweens::init();
    }
    flixel::tweens::globalManager->tweens.push_back(t);

    zoomTweenActive = true;
}
