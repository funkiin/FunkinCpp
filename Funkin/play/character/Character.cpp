#include "Character.h"
#include <SDLAnimate/SDL2/SDLAnimateAssets.h>
#include <flixel/FlxG.h>
#include <flixel/graphics/frames/FlxAtlasFrames.h>
#include "../song/Conductor.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <filesystem>

using json = nlohmann::json;
namespace fs = std::filesystem;

namespace {

bool fileExists(const std::string& path) {
    std::error_code ec;
    return fs::exists(path, ec);
}

std::string readTextFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string normalizeAssetPath(const std::string& assetPath) {
    if (assetPath.empty()) {
        return "";
    }

    const std::string assetRoot = ASSETS_PATH;
    if (assetPath.front() == '/' || (!assetRoot.empty() && assetPath.rfind(assetRoot, 0) == 0)) {
        return assetPath;
    }

    size_t colon = assetPath.find(':');
    if (colon != std::string::npos) {
        std::string library = assetPath.substr(0, colon);
        std::string local = assetPath.substr(colon + 1);
        return std::string(ASSETS_PATH) + "assets/" + library + "/images/" + local;
    }

    if (assetPath.rfind("assets/", 0) == 0) {
        return std::string(ASSETS_PATH) + assetPath;
    }

    return std::string(ASSETS_PATH) + "assets/images/" + assetPath;
}

std::string findCharacterDataPath(const std::string& character) {
    std::vector<std::string> candidates = {
        std::string(ASSETS_PATH) + "assets/preload/data/characters/" + character + ".json",
        std::string(ASSETS_PATH) + "assets/data/characters/" + character + ".json"
    };

    for (const std::string& path : candidates) {
        if (fileExists(path)) {
            return path;
        }
    }
    return "";
}

bool jsonBool(const json& value, const std::string& modernKey, const std::string& legacyKey, bool fallback) {
    if (value.contains(modernKey)) return value[modernKey].get<bool>();
    if (value.contains(legacyKey)) return value[legacyKey].get<bool>();
    return fallback;
}

std::vector<int> jsonIndices(const json& value) {
    if (value.contains("frameIndices")) return value["frameIndices"].get<std::vector<int>>();
    if (value.contains("indices")) return value["indices"].get<std::vector<int>>();
    return {};
}

} // namespace

Character::Character(float x, float y, const std::string& character, bool isPlayer)
    : FlxSprite(x, y)
    , curCharacter(character)
    , isPlayer(isPlayer)
    , debugMode(false)
    , stunned(false)
    , holdTimer(0.0f)
    , danceEvery(1.0f)
    , danced(false)
    , healthColorR(255)
    , healthColorG(255)
    , healthColorB(255)
{
    loadCharacter();
    dance();
    
    if (isPlayer) {
        flipX = !flipX;
    }
}

Character::~Character() {
    animOffsets.clear();
}

void Character::loadCharacter() {
    if (loadFromJSON(curCharacter)) {
        return;
    }
    
    if (curCharacter == "bf") {
        setupBF();
    } else if (curCharacter == "gf") {
        setupGF();
    } else if (curCharacter == "dad") {
        setupDad();
    } else {
        std::cerr << "Unknown character: " << curCharacter << ", defaulting to dad" << std::endl;
        curCharacter = "dad";
        setupDad();
    }
}

bool Character::loadFromJSON(const std::string& character) {
    std::string jsonPath = findCharacterDataPath(character);
    if (jsonPath.empty()) {
        return false;
    }

    std::ifstream file(jsonPath);
    
    if (!file.is_open()) {
        return false;
    }
    
    try {
        json charData;
        file >> charData;
        file.close();
        
        std::string assetPath = normalizeAssetPath(charData.value("assetPath", "assets/images/chars/BOYFRIEND"));
        std::string renderType = charData.value("renderType", "");
        bool wantsAnimate = renderType.find("animateatlas") != std::string::npos ||
                            fileExists(assetPath + "/Animation.json");

        bool loaded = wantsAnimate
            ? loadAnimateCharacter(charData, assetPath)
            : loadSparrowCharacter(charData, assetPath);

        if (!loaded) {
            std::cerr << "Failed to load character assets for: " << character << std::endl;
            return false;
        }

        if (charData.contains("flipX")) {
            flipX = charData["flipX"].get<bool>();
        }

        if (charData.contains("offsets") && charData["offsets"].is_array() && charData["offsets"].size() >= 2) {
            baseOffsetX = charData["offsets"][0].get<float>();
            baseOffsetY = charData["offsets"][1].get<float>();
        }
        
        std::string startAnim = charData.value("startingAnimation", usesAnimateAtlas && curCharacter == "gf" ? "danceRight" : "idle");
        playAnim(startAnim);
        
        std::cout << "Loaded character from JSON: " << character << std::endl;
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "Error parsing character JSON for " << character << ": " << e.what() << std::endl;
        file.close();
        return false;
    }
}

bool Character::loadAnimateCharacter(const json& charData, const std::string& assetPath) {
    SDLAnimate::FlxAnimateAssets::setRenderer(flixel::FlxG::renderer);
    SDLAnimate::FlxAnimateAssets::setAssetRoot(ASSETS_PATH);

    animateSprite = std::make_unique<SDLAnimate::FlxAnimate>(x, y);
    animateSprite->setRenderer(flixel::FlxG::renderer);
    if (!animateSprite->loadAnimate(assetPath)) {
        animateSprite.reset();
        return false;
    }

    usesAnimateAtlas = true;
    baseAssetPath = assetPath;
    animation = nullptr;
    texture = nullptr;
    frames = nullptr;
    frameWidth = animateSprite->frameWidth;
    frameHeight = animateSprite->frameHeight;
    width = animateSprite->width;
    height = animateSprite->height;
    centerOrigin();

    if (charData.contains("animations")) {
        for (const auto& animData : charData["animations"]) {
            std::string animName = animData.value("name", "");
            std::string prefix = animData.value("prefix", "");
            if (animName.empty() || prefix.empty()) {
                continue;
            }

            std::string animAssetPath = normalizeAssetPath(animData.value("assetPath", charData.value("assetPath", "")));
            if (!animAssetPath.empty() && animAssetPath != baseAssetPath && fileExists(animAssetPath + "/Animation.json")) {
                auto extra = SDLAnimate::FlxAnimateFrames::fromAnimate(animAssetPath);
                if (extra && animateSprite->library) {
                    animateSprite->library->mergeFrom(*extra);
                }
            }

            float frameRate = animData.value("frameRate", 0.0f);
            bool loop = jsonBool(animData, "looped", "loop", false);
            std::vector<int> indices = jsonIndices(animData);
            std::string animType = animData.value("animType", "");

            if (animType == "symbol") {
                if (!indices.empty()) {
                    animateSprite->anim.addBySymbolIndices(animName, prefix, indices, frameRate, loop);
                } else {
                    animateSprite->anim.addBySymbol(animName, prefix, frameRate, loop);
                }
            } else {
                if (!indices.empty()) {
                    animateSprite->anim.addByFrameLabelIndices(animName, prefix, indices, frameRate, loop);
                } else {
                    animateSprite->anim.addByFrameLabel(animName, prefix, frameRate, loop);
                }
            }

            if (animData.contains("offsets") && animData["offsets"].is_array() && animData["offsets"].size() >= 2) {
                addOffset(animName, animData["offsets"][0].get<float>(), animData["offsets"][1].get<float>());
            }
        }
    }

    if (charData.contains("scale")) {
        float scaleVal = charData["scale"].get<float>();
        scale.set(scaleVal, scaleVal);
    }

    if (charData.contains("healthColor") && charData["healthColor"].is_array() && charData["healthColor"].size() >= 3) {
        healthColorR = charData["healthColor"][0].get<int>();
        healthColorG = charData["healthColor"][1].get<int>();
        healthColorB = charData["healthColor"][2].get<int>();
    }

    return true;
}

bool Character::loadSparrowCharacter(const json& charData, const std::string& assetPath) {
    std::string xmlPath = assetPath + ".xml";
    std::string pngPath = assetPath + ".png";

    std::string xmlText = readTextFile(xmlPath);
    if (xmlText.empty()) {
        std::cerr << "Failed to load character XML: " << xmlPath << std::endl;
        return false;
    }

    auto tex = flixel::graphics::frames::FlxAtlasFrames::fromSparrow(pngPath, xmlText);
    if (!tex) {
        return false;
    }

    frames = tex;

    if (frames && !frames->frames.empty()) {
        const auto& firstFrame = frames->frames[0];
        frameWidth = firstFrame.sourceSize.w;
        frameHeight = firstFrame.sourceSize.h;
        width = static_cast<float>(frameWidth);
        height = static_cast<float>(frameHeight);
    }

    texture = tex->texture;
    ownsTexture = false;
    animation = new flixel::animation::FlxAnimationController();
    usesAnimateAtlas = false;

    if (charData.contains("animations")) {
        for (const auto& animData : charData["animations"]) {
            std::string animName = animData.value("name", "");
            std::string prefix = animData.value("prefix", "");
            int frameRate = animData.value("frameRate", 24);
            bool loop = jsonBool(animData, "looped", "loop", false);

            if (!frames || animName.empty() || prefix.empty()) {
                continue;
            }

            auto animFrames = frames->getFramesByPrefix(prefix);
            if (!animFrames.empty()) {
                std::vector<int> indices = jsonIndices(animData);
                if (!indices.empty()) {
                    animation->addByIndices(animName, animFrames, indices, frameRate, loop);
                } else {
                    animation->addByPrefix(animName, animFrames, frameRate, loop);
                }
            }

            if (animData.contains("offsets") && animData["offsets"].is_array() && animData["offsets"].size() >= 2) {
                addOffset(animName, animData["offsets"][0].get<float>(), animData["offsets"][1].get<float>());
            }
        }
    }
        
    if (charData.contains("scale")) {
        float scaleVal = charData["scale"].get<float>();
        scale.set(scaleVal, scaleVal);
    }
        
    if (charData.contains("healthColor") && charData["healthColor"].is_array() && charData["healthColor"].size() >= 3) {
        healthColorR = charData["healthColor"][0].get<int>();
        healthColorG = charData["healthColor"][1].get<int>();
        healthColorB = charData["healthColor"][2].get<int>();
    }

    return true;
}

void Character::setupBF() {
    std::ifstream file(ASSETS_PATH "assets/images/chars/BOYFRIEND.xml");
    if (!file.is_open()) {
        std::cerr << "Failed to load BOYFRIEND.xml" << std::endl;
        return;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string xmlText = buffer.str();
    file.close();
    
    auto tex = flixel::graphics::frames::FlxAtlasFrames::fromSparrow(
        ASSETS_PATH "assets/images/chars/BOYFRIEND.png",
        xmlText
    );
    
    frames = tex;
    
    if (frames && !frames->frames.empty()) {
        const auto& firstFrame = frames->frames[0];
        frameWidth = firstFrame.sourceSize.w;
        frameHeight = firstFrame.sourceSize.h;
        width = static_cast<float>(frameWidth);
        height = static_cast<float>(frameHeight);
    }
    texture = tex->texture;
    ownsTexture = false;
    animation = new flixel::animation::FlxAnimationController();
    
    auto idleFrames = frames->getFramesByPrefix("BF idle dance");
    auto singUPFrames = frames->getFramesByPrefix("BF NOTE UP");
    auto singLEFTFrames = frames->getFramesByPrefix("BF NOTE LEFT");
    auto singRIGHTFrames = frames->getFramesByPrefix("BF NOTE RIGHT");
    auto singDOWNFrames = frames->getFramesByPrefix("BF NOTE DOWN");
    auto singUPmissFrames = frames->getFramesByPrefix("BF NOTE UP MISS");
    auto singLEFTmissFrames = frames->getFramesByPrefix("BF NOTE LEFT MISS");
    auto singRIGHTmissFrames = frames->getFramesByPrefix("BF NOTE RIGHT MISS");
    auto singDOWNmissFrames = frames->getFramesByPrefix("BF NOTE DOWN MISS");
    auto heyFrames = frames->getFramesByPrefix("BF HEY!!");
    auto firstDeathFrames = frames->getFramesByPrefix("BF dies");
    auto deathLoopFrames = frames->getFramesByPrefix("BF Dead Loop");
    auto deathConfirmFrames = frames->getFramesByPrefix("BF Dead confirm");
    auto scaredFrames = frames->getFramesByPrefix("BF idle shaking");
    
    if (!idleFrames.empty()) animation->addByPrefix("idle", idleFrames, 24, false);
    if (!singUPFrames.empty()) animation->addByPrefix("singUP", singUPFrames, 24, false);
    if (!singLEFTFrames.empty()) animation->addByPrefix("singLEFT", singLEFTFrames, 24, false);
    if (!singRIGHTFrames.empty()) animation->addByPrefix("singRIGHT", singRIGHTFrames, 24, false);
    if (!singDOWNFrames.empty()) animation->addByPrefix("singDOWN", singDOWNFrames, 24, false);
    if (!singUPmissFrames.empty()) animation->addByPrefix("singUPmiss", singUPmissFrames, 24, false);
    if (!singLEFTmissFrames.empty()) animation->addByPrefix("singLEFTmiss", singLEFTmissFrames, 24, false);
    if (!singRIGHTmissFrames.empty()) animation->addByPrefix("singRIGHTmiss", singRIGHTmissFrames, 24, false);
    if (!singDOWNmissFrames.empty()) animation->addByPrefix("singDOWNmiss", singDOWNmissFrames, 24, false);
    if (!heyFrames.empty()) animation->addByPrefix("hey", heyFrames, 24, false);
    if (!firstDeathFrames.empty()) animation->addByPrefix("firstDeath", firstDeathFrames, 24, false);
    if (!deathLoopFrames.empty()) animation->addByPrefix("deathLoop", deathLoopFrames, 24, true);
    if (!deathConfirmFrames.empty()) animation->addByPrefix("deathConfirm", deathConfirmFrames, 24, false);
    if (!scaredFrames.empty()) animation->addByPrefix("scared", scaredFrames, 24, false);
    
    addOffset("idle", -5, 0);
    addOffset("singUP", -29, 27);
    addOffset("singRIGHT", -38, -7);
    addOffset("singLEFT", 12, -6);
    addOffset("singDOWN", -10, -50);
    addOffset("singUPmiss", -29, 27);
    addOffset("singRIGHTmiss", -30, 21);
    addOffset("singLEFTmiss", 12, 24);
    addOffset("singDOWNmiss", -11, -19);
    addOffset("hey", 7, 4);
    addOffset("firstDeath", 37, 11);
    addOffset("deathLoop", 37, 5);
    addOffset("deathConfirm", 37, 69);
    addOffset("scared", -4, 0);
    
    playAnim("idle");
    flipX = true;
}

void Character::setupGF() {
    std::ifstream file(ASSETS_PATH "assets/images/chars/GF_assets.xml");
    if (!file.is_open()) {
        std::cerr << "Failed to load GF_assets.xml" << std::endl;
        return;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string xmlText = buffer.str();
    file.close();
    
    auto tex = flixel::graphics::frames::FlxAtlasFrames::fromSparrow(
        ASSETS_PATH "assets/images/chars/GF_assets.png",
        xmlText
    );
    
    frames = tex;
    
    if (frames && !frames->frames.empty()) {
        const auto& firstFrame = frames->frames[0];
        frameWidth = firstFrame.sourceSize.w;
        frameHeight = firstFrame.sourceSize.h;
        width = static_cast<float>(frameWidth);
        height = static_cast<float>(frameHeight);
    }
    texture = tex->texture;
    ownsTexture = false;
    animation = new flixel::animation::FlxAnimationController();
    
    auto cheerFrames = frames->getFramesByPrefix("GF Cheer");
    auto singLEFTFrames = frames->getFramesByPrefix("GF left note");
    auto singRIGHTFrames = frames->getFramesByPrefix("GF Right Note");
    auto singUPFrames = frames->getFramesByPrefix("GF Up Note");
    auto singDOWNFrames = frames->getFramesByPrefix("GF Down Note");
    
    if (!cheerFrames.empty()) animation->addByPrefix("cheer", cheerFrames, 24, false);
    if (!singLEFTFrames.empty()) animation->addByPrefix("singLEFT", singLEFTFrames, 24, false);
    if (!singRIGHTFrames.empty()) animation->addByPrefix("singRIGHT", singRIGHTFrames, 24, false);
    if (!singUPFrames.empty()) animation->addByPrefix("singUP", singUPFrames, 24, false);
    if (!singDOWNFrames.empty()) animation->addByPrefix("singDOWN", singDOWNFrames, 24, false);
    
    std::vector<int> danceLeftIndices = {30, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14};
    std::vector<int> danceRightIndices = {15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29};
    
    auto danceFrames = frames->getFramesByPrefix("GF Dancing Beat");
    if (!danceFrames.empty()) {
        animation->addByIndices("danceLeft", danceFrames, danceLeftIndices, 24, false);
        animation->addByIndices("danceRight", danceFrames, danceRightIndices, 24, false);
    }
    
    addOffset("cheer", 0, 0);
    addOffset("danceLeft", 0, -9);
    addOffset("danceRight", 0, -9);
    addOffset("singUP", 0, 4);
    addOffset("singRIGHT", 0, -20);
    addOffset("singLEFT", 0, -19);
    addOffset("singDOWN", 0, -20);
    
    danced = true;
    playAnim("danceRight");
}

void Character::setupDad() {
    std::ifstream file(ASSETS_PATH "assets/images/chars/DADDY_DEAREST.xml");
    if (!file.is_open()) {
        std::cerr << "Failed to load DADDY_DEAREST.xml" << std::endl;
        return;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string xmlText = buffer.str();
    file.close();
    
    auto tex = flixel::graphics::frames::FlxAtlasFrames::fromSparrow(
        ASSETS_PATH "assets/images/chars/DADDY_DEAREST.png",
        xmlText
    );
    
    frames = tex;
    
    if (frames && !frames->frames.empty()) {
        const auto& firstFrame = frames->frames[0];
        frameWidth = firstFrame.sourceSize.w;
        frameHeight = firstFrame.sourceSize.h;
        width = static_cast<float>(frameWidth);
        height = static_cast<float>(frameHeight);
    }
    texture = tex->texture;
    ownsTexture = false;
    animation = new flixel::animation::FlxAnimationController();
    
    auto idleFrames = frames->getFramesByPrefix("Dad idle dance");
    auto singUPFrames = frames->getFramesByPrefix("Dad Sing Note UP");
    auto singRIGHTFrames = frames->getFramesByPrefix("Dad Sing Note RIGHT");
    auto singDOWNFrames = frames->getFramesByPrefix("Dad Sing Note DOWN");
    auto singLEFTFrames = frames->getFramesByPrefix("Dad Sing Note LEFT");
    
    if (!idleFrames.empty()) animation->addByPrefix("idle", idleFrames, 24, true);
    if (!singUPFrames.empty()) animation->addByPrefix("singUP", singUPFrames, 24, false);
    if (!singRIGHTFrames.empty()) animation->addByPrefix("singRIGHT", singRIGHTFrames, 24, false);
    if (!singDOWNFrames.empty()) animation->addByPrefix("singDOWN", singDOWNFrames, 24, false);
    if (!singLEFTFrames.empty()) animation->addByPrefix("singLEFT", singLEFTFrames, 24, false);
    
    addOffset("idle", 0, 0);
    addOffset("singUP", -6, 50);
    addOffset("singRIGHT", 0, 27);
    addOffset("singLEFT", -10, 10);
    addOffset("singDOWN", 0, -30);
    
    playAnim("idle");
}

void Character::update(float elapsed) {
    flixel::FlxSprite::update(elapsed);
    if (usesAnimateAtlas && animateSprite) {
        syncAnimateSprite();
        animateSprite->update(elapsed);
    }
    
    if (animation) {
        animation->update(elapsed);
        
        if (frames && !frames->frames.empty()) {
            int frameIdx = animation->getCurrentFrame();
            if (frameIdx >= 0 && frameIdx < static_cast<int>(frames->frames.size())) {
                const auto& frame = frames->frames[frameIdx];
                frameWidth = frame.sourceSize.w;
                frameHeight = frame.sourceSize.h;
                width = static_cast<float>(frameWidth);
                height = static_cast<float>(frameHeight);
            }
        }
    }
    
    if (curCharacter.rfind("bf", 0) == 0) {
        holdTimer += elapsed;
        
        if (holdTimer >= Conductor::stepCrochet * 4 * 0.001f) {
            std::string currentAnim = getCurrentAnimName();
            bool shouldReturnToIdle = (currentAnim.rfind("sing", 0) == 0) || 
                                     (currentAnim == "hey") || 
                                     (currentAnim == "scared");
            
            if (shouldReturnToIdle) {
                dance();
            }
        }
    } else {
        if (getCurrentAnimName().rfind("sing", 0) == 0) {
            holdTimer += elapsed;
            
            float dadVar = 4.0f;
            if (curCharacter == "dad") {
                dadVar = 6.1f;
            }
            
            if (holdTimer >= Conductor::stepCrochet * dadVar * 0.001f) {
                dance();
                holdTimer = 0.0f;
            }
        }
    }
}

void Character::draw() {
    if (usesAnimateAtlas && animateSprite) {
        syncAnimateSprite();
        animateSprite->draw();
        return;
    }
    flixel::FlxSprite::draw();
}

void Character::syncAnimateSprite() {
    if (!animateSprite) {
        return;
    }

    animateSprite->setRenderer(flixel::FlxG::renderer);
    animateSprite->x = x;
    animateSprite->y = y;
    animateSprite->angle = angle;
    animateSprite->alpha = alpha;
    animateSprite->offsetX = offsetX;
    animateSprite->offsetY = offsetY;
    animateSprite->originX = originX;
    animateSprite->originY = originY;
    animateSprite->visible = visible;
    animateSprite->active = active;
    animateSprite->flipX = flipX;
    animateSprite->flipY = flipY;
    animateSprite->scale = {scale.x, scale.y};
    animateSprite->scrollFactor = {scrollFactor.x, scrollFactor.y};
    if (camera) {
        animateSprite->setCamera(camera->scroll.x, camera->scroll.y, camera->zoom);
    } else {
        animateSprite->setCamera(0.0f, 0.0f, 1.0f);
    }
}

void Character::dance() {
    if (!debugMode) {
        if (curCharacter == "gf") {
            danced = !danced;
            if (danced) {
                playAnim("danceRight", true);
            } else {
                playAnim("danceLeft", true);
            }
        } else {
            playAnim("idle", true);
        }
    }
}

void Character::playAnim(const std::string& animName, bool force, bool reversed, int frame) {
    if (usesAnimateAtlas) {
        if (!force) {
            std::string currentAnim = getCurrentAnimName();
            bool isSpecialAnim = (currentAnim == "hey" || currentAnim == "scared");
            
            if (isSpecialAnim && holdTimer < Conductor::stepCrochet * 4 * 0.001f) {
                return;
            }
        }

        if (animateSprite) {
            animateSprite->anim.play(animName, force, reversed, frame);
        }
        
        if (animOffsets.find(animName) != animOffsets.end()) {
            auto& offset = animOffsets[animName];
            offsetX = baseOffsetX + offset[0];
            offsetY = baseOffsetY + offset[1];
        } else {
            offsetX = baseOffsetX;
            offsetY = baseOffsetY;
        }
        
        holdTimer = 0.0f;
        
        if (curCharacter == "gf") {
            if (animName == "singLEFT") {
                danced = true;
            } else if (animName == "singRIGHT") {
                danced = false;
            }
            
            if (animName == "singUP" || animName == "singDOWN") {
                danced = !danced;
            }
        }
        return;
    }

    if (animation) {
        if (!force) {
            std::string currentAnim = animation->current;
            bool isSpecialAnim = (currentAnim == "hey" || currentAnim == "scared");
            
            if (isSpecialAnim && holdTimer < Conductor::stepCrochet * 4 * 0.001f) {
                return;
            }
        }
        
        animation->play(animName, force, reversed, frame);
        
        if (animOffsets.find(animName) != animOffsets.end()) {
            auto& offset = animOffsets[animName];
            offsetX = baseOffsetX + offset[0];
            offsetY = baseOffsetY + offset[1];
        } else {
            offsetX = baseOffsetX;
            offsetY = baseOffsetY;
        }
        
        holdTimer = 0.0f;
        
        if (curCharacter == "gf") {
            if (animName == "singLEFT") {
                danced = true;
            } else if (animName == "singRIGHT") {
                danced = false;
            }
            
            if (animName == "singUP" || animName == "singDOWN") {
                danced = !danced;
            }
        }
    }
}

void Character::addOffset(const std::string& name, float x, float y) {
    animOffsets[name] = {x, y};
}

std::string Character::getCurrentAnimName() const {
    if (usesAnimateAtlas) {
        const SDLAnimate::Animation* current = animateSprite ? animateSprite->anim.current() : nullptr;
        return current ? current->name : "";
    }
    return animation ? animation->current : "";
}

int Character::getCurrentAnimFrame() const {
    if (usesAnimateAtlas) {
        return animateSprite ? animateSprite->anim.framePosition() : 0;
    }
    return animation ? animation->currentFrame : 0;
}

bool Character::isCurrentAnimFinished() const {
    if (usesAnimateAtlas) {
        return animateSprite ? animateSprite->anim.finished() : true;
    }
    return animation ? animation->finished : true;
}

bool Character::hasAnimation(const std::string& animName) const {
    if (usesAnimateAtlas) {
        return animateSprite && animateSprite->anim.has(animName);
    }
    return animation && animation->animations.find(animName) != animation->animations.end();
}
