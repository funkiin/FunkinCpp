#pragma once

#include <flixel/FlxSprite.h>
#include <string>

class HealthIcon : public flixel::FlxSprite {
public:
    HealthIcon(const std::string& character = "bf", bool isPlayer = false);
    ~HealthIcon() override;
    
    void update(float elapsed) override;
    void changeIcon(const std::string& character);
    void configure(const std::string& character, bool shouldBop = true,
                   float scale = 1.0f, bool flipX = false,
                   bool isPixel = false, float offsetX = 0.0f, float offsetY = 0.0f);
    std::string getCharacter() const { return curCharacter; }
    
    bool isPlayer;
    bool shouldBop;
    bool isPixel;
    flixel::FlxSprite* sprTracker;
    int fullIconWidth;
    
private:
    std::string curCharacter;
    float baseScale;
    float extraOffsets[2];
    float iconOffsets[2];
};
