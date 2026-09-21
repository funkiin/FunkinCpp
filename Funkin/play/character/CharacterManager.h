#pragma once

#include "Character.h"
#include "../song/Song.h"

class CharacterManager {
public:
    CharacterManager(Character* gf, Character* dad, Character* boyfriend, int gfSpeed);
    
    void update(float elapsed);
    void beatHit(int curBeat, int curStep, const SwagSong& song);
    bool setDanceEvery(const std::string& target, float rate);
    
private:
    Character* gf;
    Character* dad;
    Character* boyfriend;
    int gfSpeed;
};
