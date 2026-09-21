#pragma once

#include <flixel/FlxCamera.h>
#include <flixel/FlxObject.h>
#include <flixel/tweens/FlxTween.h>
#include "character/Character.h"

class CameraManager {
public:
    CameraManager(flixel::FlxCamera* camGame, flixel::FlxCamera* camHUD, float defaultZoom);
    ~CameraManager();

    void initialize(Character* boyfriend, Character* dad, Character* gf);
    void update(float elapsed);
    void beatHit(int beat);
    void focusOn(int charTarget, float xOff = 0.0f, float yOff = 0.0f);
    void focusOn(int charTarget, float xOff, float yOff, float durationSeconds,
                 flixel::tweens::EaseFunction ease, bool classic = false);
    void tweenZoom(float targetZoom, float durationSeconds = 0.0f, bool direct = true,
                   flixel::tweens::EaseFunction ease = flixel::tweens::FlxEase::cubeOut);
    void setCameraBopRate(float rate)      { cameraBopRate = rate; }
    void setCameraBopOffset(float offset)  { cameraBopOffset = offset; }
    void setCameraBopIntensity(float i)    { cameraBopIntensity = i; }
    flixel::FlxObject* getCamFollow() const { return camFollow; }
    float getDefaultZoom() const { return defaultCamZoom; }
    bool isCamZooming() const { return camZooming; }
    void setCamZooming(bool zooming) { camZooming = zooming; }

private:
    flixel::FlxCamera*  camGame;
    flixel::FlxCamera*  camHUD;
    flixel::FlxObject*  camFollow;

    Character* boyfriend;
    Character* dad;
    Character* gf;

    float defaultCamZoom;
    float cameraZoomTarget;
    float cameraZoomTweenSpeed;
    bool  camZooming;
    bool  zoomTweenActive = false;

    float cameraBopRate      = 4.0f;
    float cameraBopOffset    = 0.0f;
    float cameraBopIntensity = 1.0f;

    void resolveFocusTarget(int charTarget, float xOff, float yOff, float& tx, float& ty) const;
};
