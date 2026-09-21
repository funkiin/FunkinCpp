#pragma once

#include <flixel/FlxSprite.h>
#include "Bopper.h"
#include <string>
#include <unordered_map>
#include <vector>

class Stage {
public:
    Stage(const std::string& stageName);
    ~Stage();
    
    void addSpritesToState();
    void update(float elapsed);
    void stepHit(int step);
    void beatHit(int beat);
    
    float getDefaultZoom() const { return defaultZoom; }
    std::string getStageName() const { return curStage; }
    bool isPixelStage() const;
    
    std::vector<flixel::FlxSprite*>& getSprites() { return sprites; }
    flixel::FlxSprite* getNamedProp(const std::string& name) const;
    Bopper* getNamedBopper(const std::string& name) const;
    bool setNamedPropDanceEvery(const std::string& name, float rate);
    
    static std::string getStageFromSong(const std::string& songName);
    
private:
    std::string curStage;
    float defaultZoom;
    std::vector<flixel::FlxSprite*> sprites;
    std::vector<Bopper*> boppers;
    std::unordered_map<std::string, flixel::FlxSprite*> namedSprites;
    
    void buildStage();
    bool loadFromJSON(const std::string& stageName);
    void buildDefaultStage();
    
    flixel::FlxSprite* createSprite(float x, float y, const std::string& imagePath);
    Bopper* createBopper(float x, float y, float danceEvery, const std::string& imagePath = "");
    Bopper* createAnimatedBopper(float x, float y, float danceEvery, const std::string& imagePath, const std::string& xmlPath);
};
