#include "CharacterManager.h"
#include <algorithm>
#include <cmath>

CharacterManager::CharacterManager(Character* gf, Character* dad, Character* boyfriend, int gfSpeed)
    : gf(gf)
    , dad(dad)
    , boyfriend(boyfriend)
    , gfSpeed(gfSpeed)
{
}

void CharacterManager::update(float elapsed) {
    if (gf) {
        gf->update(elapsed);
    }
    if (dad) {
        dad->update(elapsed);
    }
    if (boyfriend) {
        boyfriend->update(elapsed);
    }
}

void CharacterManager::beatHit(int curBeat, int curStep, const SwagSong& song) {
    auto shouldDance = [curBeat](Character* character, float fallbackRate) {
        float rate = character ? character->danceEvery : fallbackRate;
        if (rate <= 0.0f) return false;
        return std::fmod(static_cast<float>(curBeat), rate) < 0.001f;
    };

    auto canIdleDance = [](Character* character) {
        if (!character) return false;

        std::string currentAnim = character->getCurrentAnimName();
        bool isSinging = currentAnim.rfind("sing", 0) == 0;
        bool isSpecialAnim = currentAnim == "hey" || currentAnim == "scared" ||
                             currentAnim == "firstDeath" || currentAnim == "deathLoop" ||
                             currentAnim == "deathConfirm";

        return !isSinging && !isSpecialAnim;
    };

    if (gf && shouldDance(gf, static_cast<float>(gfSpeed))) {
        gf->dance();
    }
    
    if (boyfriend && shouldDance(boyfriend, 1.0f) && canIdleDance(boyfriend)) {
        boyfriend->dance();
    }
    
    if (dad && shouldDance(dad, 1.0f)) {
        int curSection = curStep / 16;
        if (curSection >= 0 && curSection < song.notes.size()) {
            if (song.notes[curSection].mustHitSection && canIdleDance(dad)) {
                dad->dance();
            }
        }
    }
}

bool CharacterManager::setDanceEvery(const std::string& target, float rate) {
    std::string targetName = target;
    std::transform(targetName.begin(), targetName.end(), targetName.begin(), ::tolower);

    Character* character = nullptr;
    if (targetName == "boyfriend" || targetName == "bf" || targetName == "player") {
        character = boyfriend;
    } else if (targetName == "dad" || targetName == "opponent") {
        character = dad;
    } else if (targetName == "girlfriend" || targetName == "gf") {
        character = gf;
    }

    if (!character) {
        return false;
    }

    character->danceEvery = rate;
    return true;
}
