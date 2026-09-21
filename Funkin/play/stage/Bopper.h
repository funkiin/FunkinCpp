#pragma once

#include <flixel/FlxSprite.h>
#include <flixel/math/FlxPoint.h>
#include <map>
#include <optional>
#include <string>
#include <vector>

class Bopper : public flixel::FlxSprite {
public:
    explicit Bopper(float danceEvery = 0.0f);
    ~Bopper() override = default;

    void update(float elapsed) override;

    void bindAnimationCallbacks();
    void resetPosition();
    void onStepHit(int step);
    void onBeatHit(int beat);
    void dance(bool forceRestart = false);
    void playAnimation(const std::string& animName, bool restart = false,
                       bool ignoreOther = false, bool reversed = false);
    void forceAnimationForDuration(const std::string& animName, float durationSeconds);

    bool hasAnimation(const std::string& animName) const;
    std::string getCurrentAnimation() const;
    std::string correctAnimationName(const std::string& animName,
                                     const std::string& fallback = "") const;

    void setAnimationOffsets(const std::string& animName, float xOffset, float yOffset);
    void applyAnimationOffsets(const std::string& animName);
    void setIdleSuffix(const std::string& suffix);
    void setGlobalOffsets(float xOffset, float yOffset);
    void setShouldAlternate(std::optional<bool> value);

    std::string name;
    float danceEvery = 0.0f;
    std::optional<bool> shouldAlternate;
    std::map<std::string, std::vector<float>> animationOffsets;
    std::string idleSuffix;
    bool isPixel = false;
    bool shouldBop = true;
    std::vector<float> globalOffsets = {0.0f, 0.0f};
    std::vector<float> animOffsets = {0.0f, 0.0f};
    flixel::FlxPoint originalPosition = flixel::FlxPoint(0.0f, 0.0f);
    bool canPlayOtherAnims = true;
    std::vector<std::string> ignoreExclusionPref;

private:
    bool hasDanced = false;
    float forceAnimationTimer = 0.0f;

    void updateShouldAlternate();
    void onAnimationFinished(const std::string& animName);
};
