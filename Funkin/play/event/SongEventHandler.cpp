#include "SongEventHandler.h"
#include "../PlayState.h"
#include "../CameraManager.h"
#include "../song/Conductor.h"
#include "../stage/Bopper.h"
#include "../stage/Stage.h"
#include <flixel/FlxG.h>
#include <flixel/tweens/FlxEase.h>
#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>

namespace {
float parseFloat(const nlohmann::json& value, float fallback) {
    try {
        if (value.is_number()) return value.get<float>();
        if (value.is_string()) return std::stof(value.get<std::string>());
    } catch (...) {
    }
    return fallback;
}

float getFloat(const nlohmann::json& value, const std::string& key, float fallback) {
    if (value.is_object() && value.contains(key)) {
        return parseFloat(value[key], fallback);
    }
    return fallback;
}

int getInt(const nlohmann::json& value, const std::string& key, int fallback) {
    if (value.is_object() && value.contains(key)) {
        return static_cast<int>(parseFloat(value[key], static_cast<float>(fallback)));
    }
    return fallback;
}

bool getBool(const nlohmann::json& value, const std::string& key, bool fallback) {
    if (!value.is_object() || !value.contains(key)) return fallback;
    const auto& v = value[key];
    if (v.is_boolean()) return v.get<bool>();
    if (v.is_number()) return v.get<int>() != 0;
    if (v.is_string()) {
        std::string s = v.get<std::string>();
        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
        return s == "true" || s == "1" || s == "yes";
    }
    return fallback;
}

std::string getString(const nlohmann::json& value, const std::string& key, const std::string& fallback) {
    if (!value.is_object() || !value.contains(key)) return fallback;
    const auto& v = value[key];
    if (v.is_string()) return v.get<std::string>();
    if (v.is_number_float()) return std::to_string(v.get<float>());
    if (v.is_number_integer()) return std::to_string(v.get<int>());
    return fallback;
}

bool hasEaseDir(const std::string& ease) {
    return ease.size() >= 2 &&
           (ease.rfind("In") == ease.size() - 2 ||
            ease.rfind("Out") == ease.size() - 3 ||
            ease.rfind("InOut") == ease.size() - 5);
}

flixel::tweens::EaseFunction resolveEase(const std::string& ease, const std::string& easeDir) {
    std::string name = ease;
    if (name == "linear") return flixel::tweens::FlxEase::linear;
    if (!hasEaseDir(name)) {
        name += easeDir;
    }

    if (name == "quadIn") return flixel::tweens::FlxEase::quadIn;
    if (name == "quadOut") return flixel::tweens::FlxEase::quadOut;
    if (name == "quadInOut") return flixel::tweens::FlxEase::quadInOut;
    if (name == "cubeIn") return flixel::tweens::FlxEase::cubeIn;
    if (name == "cubeOut") return flixel::tweens::FlxEase::cubeOut;
    if (name == "cubeInOut") return flixel::tweens::FlxEase::cubeInOut;
    if (name == "quartIn") return flixel::tweens::FlxEase::quartIn;
    if (name == "quartOut") return flixel::tweens::FlxEase::quartOut;
    if (name == "quartInOut") return flixel::tweens::FlxEase::quartInOut;
    if (name == "quintIn") return flixel::tweens::FlxEase::quintIn;
    if (name == "quintOut") return flixel::tweens::FlxEase::quintOut;
    if (name == "quintInOut") return flixel::tweens::FlxEase::quintInOut;
    if (name == "sineIn") return flixel::tweens::FlxEase::sineIn;
    if (name == "sineOut") return flixel::tweens::FlxEase::sineOut;
    if (name == "sineInOut") return flixel::tweens::FlxEase::sineInOut;
    if (name == "expoIn") return flixel::tweens::FlxEase::expoIn;
    if (name == "expoOut") return flixel::tweens::FlxEase::expoOut;
    if (name == "expoInOut") return flixel::tweens::FlxEase::expoInOut;
    if (name == "smoothStepIn") return flixel::tweens::FlxEase::smoothStepIn;
    if (name == "smoothStepOut") return flixel::tweens::FlxEase::smoothStepOut;
    if (name == "smoothStepInOut") return flixel::tweens::FlxEase::smoothStepInOut;
    if (name == "smootherStepIn") return flixel::tweens::FlxEase::smootherStepIn;
    if (name == "smootherStepOut") return flixel::tweens::FlxEase::smootherStepOut;
    if (name == "smootherStepInOut") return flixel::tweens::FlxEase::smootherStepInOut;
    if (name == "elasticIn") return flixel::tweens::FlxEase::elasticIn;
    if (name == "elasticOut") return flixel::tweens::FlxEase::elasticOut;
    if (name == "elasticInOut") return flixel::tweens::FlxEase::elasticInOut;
    if (name == "backIn") return flixel::tweens::FlxEase::backIn;
    if (name == "backOut") return flixel::tweens::FlxEase::backOut;
    if (name == "backInOut") return flixel::tweens::FlxEase::backInOut;
    if (name == "bounceIn") return flixel::tweens::FlxEase::bounceIn;
    if (name == "bounceOut") return flixel::tweens::FlxEase::bounceOut;
    if (name == "bounceInOut") return flixel::tweens::FlxEase::bounceInOut;
    if (name == "circIn") return flixel::tweens::FlxEase::circIn;
    if (name == "circOut") return flixel::tweens::FlxEase::circOut;
    if (name == "circInOut") return flixel::tweens::FlxEase::circInOut;
    return nullptr;
}

float stepsToSeconds(float steps) {
    return Conductor::stepCrochet * steps / 1000.0f;
}
}

SongEventHandler::SongEventHandler() = default;

void SongEventHandler::loadEvents(std::vector<SongEvent>& evts) {
    events = &evts;
    std::stable_sort(evts.begin(), evts.end(),
        [](const SongEvent& a, const SongEvent& b) { return a.time < b.time; });
}

void SongEventHandler::update(float songPosition) {
    if (!events) return;
    for (auto& evt : *events) {
        if (!evt.activated && evt.time <= songPosition) {
            evt.activated = true;
            dispatch(evt);
        }
    }
}

void SongEventHandler::reset() {
    if (!events) return;
    for (auto& evt : *events) evt.activated = false;
}

void SongEventHandler::dispatch(const SongEvent& e) {
    if      (e.kind == "FocusCamera")  handleFocusCamera(e);
    else if (e.kind == "ZoomCamera")   handleZoomCamera(e);
    else if (e.kind == "PlayAnimation")handlePlayAnimation(e);
    else if (e.kind == "SetCameraBop") handleSetCameraBop(e);
    else if (e.kind == "ScrollSpeed")  handleScrollSpeed(e);
    else if (e.kind == "SetHealthIcon")handleSetHealthIcon(e);
    else if (e.kind == "SetTargetBopSpeed") handleSetTargetBopSpeed(e);
    else {
        std::cout << "[Events] Unknown event: " << e.kind << std::endl;
    }
}

void SongEventHandler::handleFocusCamera(const SongEvent& e) {
    if (!PlayState::instance) return;
    CameraManager* cam = PlayState::instance->getCameraManager();
    if (!cam) return;

    int charTarget = 0;
    float xOff = 0.0f, yOff = 0.0f;
    float durationSteps = 4.0f;
    std::string easeName = "CLASSIC";
    std::string easeDir = "InOut";

    if (e.value.is_object()) {
        charTarget = getInt(e.value, "char", charTarget);
        xOff = getFloat(e.value, "x", xOff);
        yOff = getFloat(e.value, "y", yOff);
        durationSteps = getFloat(e.value, "duration", durationSteps);
        easeName = getString(e.value, "ease", easeName);
        easeDir = getString(e.value, "easeDir", easeDir);
    } else if (e.value.is_number()) {
        charTarget = e.value.get<int>();
    }

    bool classic = easeName == "CLASSIC";
    float durationSeconds = easeName == "INSTANT" ? 0.0f : stepsToSeconds(durationSteps);
    auto ease = classic || easeName == "INSTANT" ? flixel::tweens::FlxEase::linear : resolveEase(easeName, easeDir);
    if (!ease) {
        std::cout << "[Events] Invalid FocusCamera ease: " << easeName << easeDir << std::endl;
        return;
    }

    cam->focusOn(charTarget, xOff, yOff, durationSeconds, ease, classic);
    std::cout << "[Events] FocusCamera char=" << charTarget << " ease=" << easeName << std::endl;
}

void SongEventHandler::handleZoomCamera(const SongEvent& e) {
    if (!PlayState::instance) return;
    CameraManager* cam = PlayState::instance->getCameraManager();
    if (!cam) return;

    float zoom = e.value.is_number() ? e.value.get<float>() : 1.0f;
    float durationSteps = 4.0f;
    bool direct = true;
    std::string easeName = "linear";
    std::string easeDir = "InOut";

    if (e.value.is_object()) {
        zoom = getFloat(e.value, "zoom", zoom);
        durationSteps = getFloat(e.value, "duration", durationSteps);
        direct = getString(e.value, "mode", "direct") == "direct";
        easeName = getString(e.value, "ease", easeName);
        easeDir = getString(e.value, "easeDir", easeDir);

        float widescreenScaleX = getFloat(e.value, "widescreenScaleX", 0.0f);
        float widescreenScaleY = getFloat(e.value, "widescreenScaleY", 0.0f);
        float aspect = flixel::FlxG::height > 0 ? static_cast<float>(flixel::FlxG::width) / static_cast<float>(flixel::FlxG::height) : (16.0f / 9.0f);
        float baseAspect = 16.0f / 9.0f;
        float wideX = aspect > baseAspect ? aspect / baseAspect : 1.0f;
        float wideY = aspect < baseAspect ? baseAspect / aspect : 1.0f;
        zoom += zoom * ((wideX - 1.0f) * widescreenScaleX + (wideY - 1.0f) * widescreenScaleY);
    }

    float durationSeconds = easeName == "INSTANT" ? 0.0f : stepsToSeconds(durationSteps);
    auto ease = easeName == "INSTANT" ? flixel::tweens::FlxEase::linear : resolveEase(easeName, easeDir);
    if (!ease) {
        std::cout << "[Events] Invalid ZoomCamera ease: " << easeName << easeDir << std::endl;
        return;
    }

    cam->tweenZoom(zoom, durationSeconds, direct, ease);
    std::cout << "[Events] ZoomCamera zoom=" << zoom << " dur=" << durationSeconds << "s" << std::endl;
}

void SongEventHandler::handlePlayAnimation(const SongEvent& e) {
    if (!PlayState::instance) return;
    if (!e.value.is_object()) return;

    std::string target = e.value.value("target", "bf");
    std::string anim   = e.value.value("anim", "idle");
    bool force         = e.value.value("force", false);

    Character* ch = nullptr;
    if (target == "bf" || target == "boyfriend" || target == "player")
        ch = PlayState::instance->getBoyfriend();
    else if (target == "dad" || target == "opponent")
        ch = PlayState::instance->getDad();
    else if (target == "gf" || target == "girlfriend")
        ch = PlayState::instance->getGf();

    if (ch) {
        ch->playAnim(anim, force);
        std::cout << "[Events] PlayAnimation target=" << target << " anim=" << anim << std::endl;
        return;
    }

    Stage* stage = PlayState::instance->getStage();
    flixel::FlxSprite* prop = stage ? stage->getNamedProp(target) : nullptr;
    if (auto* bopper = dynamic_cast<Bopper*>(prop)) {
        bopper->playAnimation(anim, force);
        std::cout << "[Events] PlayAnimation bopper=" << target << " anim=" << anim << std::endl;
        return;
    }

    if (prop && prop->animation) {
        prop->animation->play(anim, force);
        std::cout << "[Events] PlayAnimation prop=" << target << " anim=" << anim << std::endl;
    }
}

void SongEventHandler::handleSetCameraBop(const SongEvent& e) {
    if (!PlayState::instance) return;
    CameraManager* cam = PlayState::instance->getCameraManager();
    if (!cam) return;

    float rate      = 4.0f;
    float offset    = 0.0f;
    float intensity = 1.0f;

    if (e.value.is_object()) {
        rate      = getFloat(e.value, "rate",      4.0f);
        offset    = getFloat(e.value, "offset",    0.0f);
        intensity = getFloat(e.value, "intensity", 1.0f);
    }

    cam->setCameraBopRate(rate);
    cam->setCameraBopOffset(offset);
    cam->setCameraBopIntensity(intensity);
    std::cout << "[Events] SetCameraBop rate=" << rate << " offset=" << offset << " intensity=" << intensity << std::endl;
}

void SongEventHandler::handleScrollSpeed(const SongEvent& e) {
    if (!PlayState::instance) return;

    float scroll  = e.value.is_number() ? e.value.get<float>() : 1.0f;
    float durationSteps = 4.0f;
    bool absolute = false;
    std::string easeName = "linear";
    std::string easeDir = "InOut";
    std::string strumline = "both";

    if (e.value.is_object()) {
        scroll = getFloat(e.value, "scroll", scroll);
        durationSteps = getFloat(e.value, "duration", durationSteps);
        easeName = getString(e.value, "ease", easeName);
        easeDir = getString(e.value, "easeDir", easeDir);
        strumline = getString(e.value, "strumline", strumline);
        absolute = getBool(e.value, "absolute", false);
    } else if (e.value.is_number()) {
        absolute = true;
    }

    if (!absolute) {
        scroll *= PlayState::baseScrollSpeed;
    }

    std::vector<std::string> strumlineNames;
    if (strumline == "both") {
        strumlineNames = {"playerStrumline", "opponentStrumline"};
    } else {
        strumlineNames = {strumline + "Strumline"};
    }

    float durationSeconds = easeName == "INSTANT" ? 0.0f : stepsToSeconds(durationSteps);
    auto ease = easeName == "INSTANT" ? flixel::tweens::FlxEase::linear : resolveEase(easeName, easeDir);
    if (!ease) {
        std::cout << "[Events] Invalid ScrollSpeed ease: " << easeName << easeDir << std::endl;
        return;
    }

    PlayState::instance->tweenScrollSpeed(scroll, durationSeconds, ease, strumlineNames);
    std::cout << "[Events] ScrollSpeed speed=" << scroll << " strumline=" << strumline << std::endl;
}

void SongEventHandler::handleSetHealthIcon(const SongEvent& e) {
    if (!PlayState::instance || !e.value.is_object()) return;

    HealthBar* healthBar = PlayState::instance->getHealthBar();
    if (!healthBar) return;

    int character = getInt(e.value, "char", 0);
    std::string iconId = getString(e.value, "id", "face");
    bool shouldBop = getBool(e.value, "shouldBop", true);
    float scale = getFloat(e.value, "scale", 1.0f);
    bool flipX = getBool(e.value, "flipX", false);
    bool isPixel = getBool(e.value, "isPixel", false);
    float offsetX = getFloat(e.value, "offsetX", 0.0f);
    float offsetY = getFloat(e.value, "offsetY", 0.0f);

    healthBar->configureIcon(character, iconId, shouldBop, scale, flipX, isPixel, offsetX, offsetY);
    std::cout << "[Events] SetHealthIcon char=" << character << " id=" << iconId << std::endl;
}

void SongEventHandler::handleSetTargetBopSpeed(const SongEvent& e) {
    if (!PlayState::instance || !e.value.is_object()) return;

    std::string target = getString(e.value, "target", "boyfriend");
    float rate = getFloat(e.value, "rate", 1.0f);

    if (!PlayState::instance->setTargetBopSpeed(target, rate)) {
        std::cout << "[Events] Unknown SetTargetBopSpeed target: " << target << std::endl;
        return;
    }

    std::cout << "[Events] SetTargetBopSpeed target=" << target << " rate=" << rate << std::endl;
}
