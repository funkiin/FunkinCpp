#pragma once

#include <SDLAnimate/SDL2/SDLAnimate.h>
#include <flixel/FlxSprite.h>
#include <memory>
#include <string>
#include <map>
#include <vector>
#include <nlohmann/json.hpp>

class Character : public flixel::FlxSprite {
public:
    Character(float x, float y, const std::string& character = "bf", bool isPlayer = false);
    ~Character();
    
    void update(float elapsed) override;
    void draw() override;
    void dance();
    void playAnim(const std::string& animName, bool force = false, bool reversed = false, int frame = 0);
    void addOffset(const std::string& name, float x = 0, float y = 0);
    std::string getCurrentAnimName() const;
    int getCurrentAnimFrame() const;
    bool isCurrentAnimFinished() const;
    bool hasAnimation(const std::string& animName) const;
    
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
    bool usesAnimateAtlas = false;
    std::string baseAssetPath;
    float baseOffsetX = 0.0f;
    float baseOffsetY = 0.0f;
    std::unique_ptr<SDLAnimate::FlxAnimate> animateSprite;
    
    void loadCharacter();
    bool loadFromJSON(const std::string& character);
    bool loadAnimateCharacter(const nlohmann::json& charData, const std::string& assetPath);
    bool loadSparrowCharacter(const nlohmann::json& charData, const std::string& assetPath);
    void syncAnimateSprite();
    void setupBF();
    void setupGF();
    void setupDad();
};
