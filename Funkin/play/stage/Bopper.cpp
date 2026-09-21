#include "Bopper.h"
#include <algorithm>
#include <cmath>
#include <iostream>

Bopper::Bopper(float danceEvery)
    : FlxSprite()
    , danceEvery(danceEvery)
{
}

void Bopper::bindAnimationCallbacks() {
    if (!animation) return;
    animation->finishCallback = [this](const std::string& animName) {
        onAnimationFinished(animName);
    };
}

void Bopper::update(float elapsed) {
    FlxSprite::update(elapsed);

    if (forceAnimationTimer > 0.0f) {
        forceAnimationTimer -= elapsed;
        if (forceAnimationTimer <= 0.0f) {
            forceAnimationTimer = 0.0f;
            canPlayOtherAnims = true;
        }
    }
}

void Bopper::resetPosition() {
    x = originalPosition.x;
    y = originalPosition.y;
}

void Bopper::onStepHit(int step) {
    if (danceEvery <= 0.0f) return;

    float interval = danceEvery * 4.0f;
    if (interval <= 0.0f) return;

    float phase = std::fmod(static_cast<float>(step), interval);
    if (phase < 0.0f) phase += interval;

    if (phase < 0.001f || std::fabs(phase - interval) < 0.001f) {
        dance(shouldBop);
    }
}

void Bopper::onBeatHit(int beat) {
}

void Bopper::updateShouldAlternate() {
    shouldAlternate = hasAnimation("danceLeft");
}

void Bopper::dance(bool forceRestart) {
    if (!animation) return;

    if (!shouldAlternate.has_value()) {
        updateShouldAlternate();
    }

    if (shouldAlternate.value_or(false)) {
        if (hasDanced) {
            playAnimation("danceRight" + idleSuffix, forceRestart);
        } else {
            playAnimation("danceLeft" + idleSuffix, forceRestart);
        }
        hasDanced = !hasDanced;
    } else {
        playAnimation("idle" + idleSuffix, forceRestart);
    }
}

bool Bopper::hasAnimation(const std::string& animName) const {
    return animation && animation->animations.find(animName) != animation->animations.end();
}

std::string Bopper::getCurrentAnimation() const {
    return animation ? animation->current : "";
}

std::string Bopper::correctAnimationName(const std::string& animName,
                                         const std::string& fallback) const {
    if (hasAnimation(animName)) {
        return animName;
    }

    size_t suffix = animName.find_last_of('-');
    if (suffix != std::string::npos) {
        std::string strippedName = animName.substr(0, suffix);
        std::cout << "[Bopper] Animation \"" << animName
                  << "\" missing, stripping suffix to \"" << strippedName << "\"" << std::endl;
        return correctAnimationName(strippedName, fallback);
    }

    if (!fallback.empty() && fallback != animName) {
        std::cout << "[Bopper] Animation \"" << animName
                  << "\" missing, falling back to \"" << fallback << "\"" << std::endl;
        return correctAnimationName(fallback);
    }

    std::cerr << "[Bopper] Missing animation: " << animName << std::endl;
    return "";
}

void Bopper::playAnimation(const std::string& animName, bool restart,
                           bool ignoreOther, bool reversed) {
    if (!animation) return;

    if (!canPlayOtherAnims) {
        std::string currentAnim = getCurrentAnimation();
        if (!(currentAnim == animName && restart)) {
            bool allowedByPrefix = false;
            for (const auto& prefix : ignoreExclusionPref) {
                if (animName.rfind(prefix, 0) == 0) {
                    allowedByPrefix = true;
                    break;
                }
            }

            if (!allowedByPrefix) {
                return;
            }
        }
    }

    std::string correctedName = correctAnimationName(animName);
    if (correctedName.empty()) return;

    int startFrame = 0;
    if (reversed) {
        auto it = animation->animations.find(correctedName);
        if (it != animation->animations.end() && !it->second.frames.empty()) {
            startFrame = static_cast<int>(it->second.frames.size()) - 1;
        }
    }

    animation->play(correctedName, restart, reversed, startFrame);

    if (ignoreOther) {
        canPlayOtherAnims = false;
    }

    applyAnimationOffsets(correctedName);
}

void Bopper::forceAnimationForDuration(const std::string& animName, float durationSeconds) {
    if (!animation) return;

    std::string correctedName = correctAnimationName(animName);
    if (correctedName.empty()) return;

    animation->play(correctedName, false);
    applyAnimationOffsets(correctedName);

    canPlayOtherAnims = false;
    forceAnimationTimer = durationSeconds;
}

void Bopper::setAnimationOffsets(const std::string& animName, float xOffset, float yOffset) {
    animationOffsets[animName] = {xOffset, yOffset};
}

void Bopper::applyAnimationOffsets(const std::string& animName) {
    auto it = animationOffsets.find(animName);
    if (it != animationOffsets.end() && it->second.size() >= 2) {
        animOffsets[0] = it->second[0];
        animOffsets[1] = it->second[1];
    } else {
        animOffsets[0] = 0.0f;
        animOffsets[1] = 0.0f;
    }

    offsetX = (animOffsets[0] - globalOffsets[0]) * scale.x;
    offsetY = (animOffsets[1] - globalOffsets[1]) * scale.y;
}

void Bopper::setIdleSuffix(const std::string& suffix) {
    idleSuffix = suffix;
    dance();
}

void Bopper::setGlobalOffsets(float xOffset, float yOffset) {
    globalOffsets[0] = xOffset;
    globalOffsets[1] = yOffset;
    applyAnimationOffsets(getCurrentAnimation());
}

void Bopper::setShouldAlternate(std::optional<bool> value) {
    shouldAlternate = value;
}

void Bopper::onAnimationFinished(const std::string& animName) {
    if (!canPlayOtherAnims) {
        canPlayOtherAnims = true;
    }
}
