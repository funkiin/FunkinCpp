#include "Stage.h"
#include <flixel/graphics/frames/FlxAtlasFrames.h>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

Stage::Stage(const std::string& stageName) {
    curStage = stageName;
    defaultZoom = 1.05f;
    buildStage();
}

Stage::~Stage() {
    for (auto sprite : sprites) {
        if (sprite) {
            delete sprite;
        }
    }
    sprites.clear();
}

void Stage::update(float elapsed) {
    for (auto bopper : boppers) {
        if (bopper) {
            bopper->update(elapsed);
        }
    }

    for (auto sprite : sprites) {
        if (sprite && sprite->animation) {
            sprite->animation->update(elapsed);
        }
    }
}

void Stage::stepHit(int step) {
    for (auto bopper : boppers) {
        if (bopper) {
            bopper->onStepHit(step);
        }
    }
}

void Stage::beatHit(int beat) {
    for (auto bopper : boppers) {
        if (bopper) {
            bopper->onBeatHit(beat);
        }
    }
}

bool Stage::isPixelStage() const {
    return curStage == "school" || curStage == "schoolEvil";
}

std::string Stage::getStageFromSong(const std::string& songName) {
    std::string lowerSong = songName;
    std::transform(lowerSong.begin(), lowerSong.end(), lowerSong.begin(), ::tolower);
    
    if (lowerSong == "spookeez" || lowerSong == "monster" || lowerSong == "south") {
        return "spooky";
    }
    else if (lowerSong == "pico" || lowerSong == "blammed" || lowerSong == "philly") {
        return "philly";
    }
    else if (lowerSong == "milf" || lowerSong == "satin-panties" || lowerSong == "high") {
        return "limo";
    }
    else if (lowerSong == "cocoa" || lowerSong == "eggnog") {
        return "mall";
    }
    else if (lowerSong == "winter-horrorland") {
        return "mallEvil";
    }
    else if (lowerSong == "senpai" || lowerSong == "roses") {
        return "school";
    }
    else if (lowerSong == "thorns") {
        return "schoolEvil";
    }
    return "stage";
}

void Stage::buildStage() {
    if (loadFromJSON(curStage)) {
        return;
    }
    
    if (curStage == "stage") {
        buildDefaultStage();
    }
    else {
        buildDefaultStage();
    }
}

bool Stage::loadFromJSON(const std::string& stageName) {
    std::string jsonPath = ASSETS_PATH "assets/data/stages/" + stageName + ".json";
    std::ifstream file(jsonPath);
    
    if (!file.is_open()) {
        return false;
    }
    
    try {
        json stageData;
        file >> stageData;
        file.close();
        
        defaultZoom = stageData.value("defaultZoom", 1.05f);
        
        if (stageData.contains("sprites")) {
            for (const auto& spriteData : stageData["sprites"]) {
                std::string type = spriteData.value("type", "static");
                std::string imagePath = spriteData.value("imagePath", spriteData.value("assetPath", ""));
                
                if (imagePath.empty()) continue;
                
                float x = 0.0f, y = 0.0f;
                if (spriteData.contains("position") && spriteData["position"].is_array() && spriteData["position"].size() >= 2) {
                    x = spriteData["position"][0].get<float>();
                    y = spriteData["position"][1].get<float>();
                }
                
                float danceEvery = spriteData.value("danceEvery", 0.0f);
                bool isAnimated = type == "animated" || spriteData.contains("animations");
                flixel::FlxSprite* sprite = nullptr;
                Bopper* bopper = nullptr;
                
                if (isAnimated || danceEvery != 0.0f) {
                    if (isAnimated || spriteData.contains("xmlPath")) {
                        std::string xmlPath = spriteData.value("xmlPath", imagePath + ".xml");
                        bopper = createAnimatedBopper(x, y, danceEvery, imagePath, xmlPath);
                    } else {
                        bopper = createBopper(x, y, danceEvery, imagePath);
                    }
                    sprite = bopper;

                    if (sprite && sprite->animation && spriteData.contains("animations")) {
                        for (const auto& anim : spriteData["animations"]) {
                            std::string animName = anim.value("name", "");
                            std::string prefix = anim.value("prefix", "");
                            int frameRate = anim.value("frameRate", 24);
                            bool loop = anim.value("loop", false);
                            
                            if (!animName.empty() && !prefix.empty() && sprite->frames) {
                                auto animFrames = sprite->frames->getFramesByPrefix(prefix);
                                if (!animFrames.empty()) {
                                    if (anim.contains("indices") && anim["indices"].is_array()) {
                                        sprite->animation->addByIndices(animName, animFrames, anim["indices"].get<std::vector<int>>(), frameRate, loop);
                                    } else {
                                        sprite->animation->addByPrefix(animName, animFrames, frameRate, loop);
                                    }
                                }
                            }

                            if (bopper && anim.contains("offsets") && anim["offsets"].is_array() && anim["offsets"].size() >= 2) {
                                bopper->setAnimationOffsets(animName, anim["offsets"][0].get<float>(), anim["offsets"][1].get<float>());
                            }
                        }
                    }

                    if (bopper) {
                        bopper->bindAnimationCallbacks();
                        bopper->name = spriteData.value("name", "");
                        bopper->shouldBop = spriteData.value("shouldBop", true);
                        bopper->isPixel = spriteData.value("isPixel", false);
                        bopper->originalPosition.set(x, y);

                        if (spriteData.contains("shouldAlternate") && spriteData["shouldAlternate"].is_boolean()) {
                            bopper->setShouldAlternate(spriteData["shouldAlternate"].get<bool>());
                        }

                        if (spriteData.contains("globalOffsets") && spriteData["globalOffsets"].is_array() && spriteData["globalOffsets"].size() >= 2) {
                            bopper->setGlobalOffsets(spriteData["globalOffsets"][0].get<float>(), spriteData["globalOffsets"][1].get<float>());
                        }
                    }

                    std::string startAnim = spriteData.value("startingAnimation", "");
                    if (!startAnim.empty()) {
                        if (bopper) {
                            bopper->playAnimation(startAnim);
                        } else if (sprite && sprite->animation) {
                            sprite->animation->play(startAnim);
                        }
                    }
                } else {
                    sprite = createSprite(x, y, imagePath);
                }
                
                if (sprite) {
                    std::string spriteName = spriteData.value("name", "");
                    if (!spriteName.empty()) {
                        namedSprites[spriteName] = sprite;
                    }

                    if (spriteData.contains("scrollFactor") && spriteData["scrollFactor"].is_array() && spriteData["scrollFactor"].size() >= 2) {
                        float scrollX = spriteData["scrollFactor"][0].get<float>();
                        float scrollY = spriteData["scrollFactor"][1].get<float>();
                        sprite->scrollFactor.set(scrollX, scrollY);
                    } else if (spriteData.contains("scroll") && spriteData["scroll"].is_array() && spriteData["scroll"].size() >= 2) {
                        float scrollX = spriteData["scroll"][0].get<float>();
                        float scrollY = spriteData["scroll"][1].get<float>();
                        sprite->scrollFactor.set(scrollX, scrollY);
                    }
                    
                    if (spriteData.contains("scale")) {
                        if (spriteData["scale"].is_array() && spriteData["scale"].size() >= 2) {
                            float scaleX = spriteData["scale"][0].get<float>();
                            float scaleY = spriteData["scale"][1].get<float>();
                            sprite->setScale(scaleX, scaleY);
                        } else if (spriteData["scale"].is_number()) {
                            sprite->setScale(spriteData["scale"].get<float>());
                        }
                    }

                    sprite->alpha = spriteData.value("alpha", sprite->alpha);
                    sprite->angle = spriteData.value("angle", sprite->angle);
                    sprite->flipX = spriteData.value("flipX", sprite->flipX);
                    sprite->flipY = spriteData.value("flipY", sprite->flipY);

                    if (spriteData.value("updateHitbox", true)) {
                        sprite->updateHitbox();
                    }

                    if (bopper && spriteData.contains("idleSuffix") && spriteData["idleSuffix"].is_string()) {
                        bopper->setIdleSuffix(spriteData["idleSuffix"].get<std::string>());
                    } else if (bopper) {
                        bopper->applyAnimationOffsets(bopper->getCurrentAnimation());
                    }
                    
                    if (spriteData.contains("active")) {
                        sprite->active = spriteData["active"].get<bool>();
                    }
                    
                    if (spriteData.contains("visible")) {
                        sprite->visible = spriteData["visible"].get<bool>();
                    }
                }
            }
        }
        
        std::cout << "Loaded stage from JSON: " << stageName << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error parsing stage JSON for " << stageName << ": " << e.what() << std::endl;
        file.close();
        return false;
    }
}

flixel::FlxSprite* Stage::getNamedProp(const std::string& name) const {
    auto it = namedSprites.find(name);
    if (it == namedSprites.end()) {
        return nullptr;
    }
    return it->second;
}

Bopper* Stage::getNamedBopper(const std::string& name) const {
    return dynamic_cast<Bopper*>(getNamedProp(name));
}

bool Stage::setNamedPropDanceEvery(const std::string& name, float rate) {
    Bopper* bopper = getNamedBopper(name);
    if (!bopper) {
        return false;
    }
    bopper->danceEvery = rate;
    return true;
}

flixel::FlxSprite* Stage::createSprite(float x, float y, const std::string& imagePath) {
    flixel::FlxSprite* sprite = new flixel::FlxSprite(x, y);
    sprite->loadGraphic(imagePath);
    sprites.push_back(sprite);
    return sprite;
}

Bopper* Stage::createBopper(float x, float y, float danceEvery, const std::string& imagePath) {
    Bopper* sprite = new Bopper(danceEvery);
    sprite->setPosition(x, y);
    if (!imagePath.empty()) {
        sprite->loadGraphic(imagePath);
    }
    boppers.push_back(sprite);
    sprites.push_back(sprite);
    return sprite;
}

Bopper* Stage::createAnimatedBopper(float x, float y, float danceEvery, const std::string& imagePath, const std::string& xmlPath) {
    Bopper* sprite = new Bopper(danceEvery);
    sprite->setPosition(x, y);
    auto frames = flixel::graphics::frames::FlxAtlasFrames::fromSparrow(imagePath, xmlPath);
    sprite->frames = frames;
    sprite->texture = frames->texture;
    sprite->ownsTexture = false;
    sprite->animation = new flixel::animation::FlxAnimationController();
    sprite->bindAnimationCallbacks();
    boppers.push_back(sprite);
    sprites.push_back(sprite);
    return sprite;
}

void Stage::buildDefaultStage() {
    defaultZoom = 1.05f;
    
    auto bg = createSprite(-600, -200, ASSETS_PATH "assets/images/stages/stage/stageback.png");
    namedSprites["stageback"] = bg;
    bg->scrollFactor.set(0.9f, 0.9f);
    bg->active = false;
    
    auto stageFront = createSprite(-650, 600, ASSETS_PATH "assets/images/stages/stage/stagefront.png");
    namedSprites["stagefront"] = stageFront;
    stageFront->setScale(1.1f, 1.1f);
    stageFront->updateHitbox();
    stageFront->scrollFactor.set(0.9f, 0.9f);
    stageFront->active = false;
    
    auto stageCurtains = createSprite(-500, -300, ASSETS_PATH "assets/images/stages/stage/stagecurtains.png");
    namedSprites["stagecurtains"] = stageCurtains;
    stageCurtains->setScale(0.9f, 0.9f);
    stageCurtains->updateHitbox();
    stageCurtains->scrollFactor.set(1.3f, 1.3f);
    stageCurtains->active = false;
}
