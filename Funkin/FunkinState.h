#pragma once

#include <flixel/FlxState.h>
#include <flixel/sound/FlxSound.h>
#include <flixel/util/FlxColor.h>
#include <flixel/math/FlxPoint.h>
#include <flixel/addons/transition/TransitionData.h>
#include <flixel/addons/transition/FlxTransitionEffect.h>
#include <functional>
#include "play/song/Conductor.h"

class FunkinState : public flixel::FlxState {
public:
    FunkinState();
    virtual ~FunkinState();

    void create() override;
    void update(float elapsed) override;
    void draw() override;
    void destroy() override;

    void updateBeat();
    void updateCurStep();
    virtual void stepHit();
    virtual void beatHit();
    
    void startTransitionIn(float duration = 0.5f, flixel::util::FlxColor color = flixel::util::FlxColor::BLACK, 
                          flixel::FlxPoint direction = flixel::FlxPoint(0, -1));
    void startTransitionOut(float duration = 0.5f, flixel::util::FlxColor color = flixel::util::FlxColor::BLACK,
                           flixel::FlxPoint direction = flixel::FlxPoint(0, 1),
                           std::function<void()> onComplete = nullptr);

    static const std::string soundExt;
    int curStep = 0;
    int curBeat = 0;
    float lastBeat = 0.0f;
    float lastStep = 0.0f;
    
protected:
    flixel::addons::transition::FlxTransitionEffect* _transition = nullptr;
    bool _transitionActive = false;
};
