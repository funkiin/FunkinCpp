#pragma once

#include <flixel/FlxSprite.h>
#include <string>
#include <map>
#include <vector>

class Character : public flixel::FlxSprite {
public:
    Character(float x, float y, const std::string& character = "bf", bool isPlayer = false);
    ~Character();
    
    void update(float elapsed) override;
    void dance();
    void playAnim(const std::string& animName, bool force = false, bool reversed = false, int frame = 0);
    void addOffset(const std::string& name, float x = 0, float y = 0);
    
    std::string curCharacter;
    bool isPlayer;
    bool debugMode;
    bool stunned;
    float holdTimer;
    float danceEvery;
    
    int healthColorR;
    int healthColorG;
    int healthColorB;
    
private:
    std::map<std::string, std::vector<float>> animOffsets;
    bool danced;
    
    void loadCharacter();
    bool loadFromJSON(const std::string& character);
    void setupBF();
    void setupGF();
    void setupDad();
};
