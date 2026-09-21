#include "NoteSprite.h"
#include "../PlayState.h"
#include "../song/Conductor.h"
#include "../../game/GameConfig.h"
#include <flixel/FlxG.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>

const float NoteSprite::STRUM_X = 42.0f;
const float NoteSprite::swagWidth = 160.0f * 0.7f;
bool NoteSprite::assetsLoaded = false;
flixel::graphics::frames::FlxAtlasFrames* NoteSprite::noteFrames = nullptr;

void NoteSprite::loadAssets() {
    if (!assetsLoaded) {
        std::ifstream file(ASSETS_PATH "assets/images/play/notetypes/default/NOTE_assets.xml");
        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string xmlText = buffer.str();
            file.close();
            
            noteFrames = flixel::graphics::frames::FlxAtlasFrames::fromSparrow(
                ASSETS_PATH "assets/images/play/notetypes/default/NOTE_assets.png", xmlText);
            assetsLoaded = true;
            std::cout << "Note assets loaded successfully" << std::endl;
        } else {
            std::cerr << "Failed to load NOTE_assets.xml" << std::endl;
        }
    }
}

void NoteSprite::unloadAssets() {
    if (assetsLoaded) {
        if (noteFrames) {
            delete noteFrames;
            noteFrames = nullptr;
        }
        assetsLoaded = false;
    }
}

NoteSprite::NoteSprite(float strumTime, int noteData) 
    : FlxSprite(), strumTime(strumTime), noteData(noteData)
{
    if (!assetsLoaded) {
        loadAssets();
    }
    
    x += STRUM_X + 50.0f;
    y -= 2000.0f;
    
    if (noteFrames) {
        texture = noteFrames->texture;
        ownsTexture = false;
        frames = noteFrames;
        animation = new flixel::animation::FlxAnimationController();
        
        if (!noteFrames->frames.empty()) {
            const auto& firstFrame = noteFrames->frames[0];
            sourceRect = firstFrame.rect;
            frameWidth = firstFrame.sourceSize.w;
            frameHeight = firstFrame.sourceSize.h;
            width = static_cast<float>(frameWidth);
            height = static_cast<float>(frameHeight);
        }
        
        cacheFrameIndices();
        
        std::string framePrefix;
        switch (noteData) {
            case LEFT_NOTE:  framePrefix = "purple"; break;
            case DOWN_NOTE:  framePrefix = "blue"; break;
            case UP_NOTE:    framePrefix = "green"; break;
            case RIGHT_NOTE: framePrefix = "red"; break;
            default:         framePrefix = "purple"; break;
        }
        
        auto scrollFrames = frames->getFramesByPrefix(framePrefix);
        if (!scrollFrames.empty()) {
            animation->addByPrefix("scroll", scrollFrames, 24, false);
            animation->play("scroll");
        }
    }
    
    x += swagWidth * noteData;
    setScale(0.7f, 0.7f);
    updateHitbox();
    visible = true;
}

void NoteSprite::cacheFrameIndices() {
    if (!noteFrames) return;
    
    std::string headPrefix, bodyPrefix, tailPrefix;
    switch (noteData) {
        case LEFT_NOTE:
            headPrefix = "purple";
            bodyPrefix = "purple hold piece";
            tailPrefix = "pruple end hold";
            break;
        case DOWN_NOTE:
            headPrefix = "blue";
            bodyPrefix = "blue hold piece";
            tailPrefix = "blue hold end";
            break;
        case UP_NOTE:
            headPrefix = "green";
            bodyPrefix = "green hold piece";
            tailPrefix = "green hold end";
            break;
        case RIGHT_NOTE:
            headPrefix = "red";
            bodyPrefix = "red hold piece";
            tailPrefix = "red hold end";
            break;
        default:
            headPrefix = "purple";
            bodyPrefix = "purple hold piece";
            tailPrefix = "pruple end hold";
            break;
    }
    
    auto headFrames = noteFrames->getFramesByPrefix(headPrefix);
    auto bodyFrames = noteFrames->getFramesByPrefix(bodyPrefix);
    auto tailFrames = noteFrames->getFramesByPrefix(tailPrefix);
    
    if (!headFrames.empty()) headFrameIndex = headFrames[0];
    if (!bodyFrames.empty()) bodyFrameIndex = bodyFrames[0];
    if (!tailFrames.empty()) tailFrameIndex = tailFrames[0];
}

float NoteSprite::getTargetY() {
    int windowHeight = 720;
    if (GameConfig::getInstance()->isDownscroll()) {
        return windowHeight - 150.0f;
    }
    return 50.0f;
}

float NoteSprite::getHoldClipAmount() const {
    if (sustainLength <= 0) return 0.0f;
    if (!wasGoodHit) return 0.0f;
    
    float elapsedSinceHit = Conductor::songPosition - hitTime;
    
    return std::max(0.0f, std::min(elapsedSinceHit, sustainLength));
}

void NoteSprite::drawFrame(int frameIndex, float drawX, float drawY, float drawScaleX, float drawScaleY) {
    if (frameIndex < 0 || frameIndex >= static_cast<int>(noteFrames->frames.size())) return;
    
    const auto& frame = noteFrames->frames[frameIndex];
    SDL_Rect srcRect = frame.rect;
    
    float camScrollX = 0.0f;
    float camScrollY = 0.0f;
    float camZoom = 1.0f;
    if (camera) {
        camScrollX = camera->scroll.x * scrollFactor.x;
        camScrollY = camera->scroll.y * scrollFactor.y;
        camZoom = camera->zoom;
    }
    
    float finalX = (drawX - camScrollX) * camZoom;
    float finalY = (drawY - camScrollY) * camZoom;
    float finalW = srcRect.w * drawScaleX * camZoom;
    float finalH = srcRect.h * drawScaleY * camZoom;
    
    SDL_FRect destRectF = {finalX, finalY, finalW, finalH};
    
    SDL_SetTextureAlphaMod(texture, static_cast<Uint8>(alpha * 255.0f));
    
    SDL_RendererFlip flip = GameConfig::getInstance()->isDownscroll() ? SDL_FLIP_VERTICAL : SDL_FLIP_NONE;
    
    SDL_RenderCopyExF(flixel::FlxG::renderer, texture, &srcRect, &destRectF, 0.0f, nullptr, flip);
}

void NoteSprite::draw() {
    if (!texture || !visible || alpha <= 0.0f) return;
    
    bool isDownscroll = GameConfig::getInstance()->isDownscroll();
    float baseScale = 0.7f;
    
    if (sustainLength <= 0 || !wasGoodHit) {
        FlxSprite::draw();
    }
    
    if (sustainLength <= 0 || bodyFrameIndex < 0 || tailFrameIndex < 0) return;
    
    float scrollSpeed = PlayState::getScrollSpeedForMustPress(mustPress);
    float fullHoldLength = sustainLength * 0.45f * scrollSpeed;
    
    const auto& bodyFrame = noteFrames->frames[bodyFrameIndex];
    const auto& tailFrame = noteFrames->frames[tailFrameIndex];
    
    float bodyHeight = bodyFrame.rect.h * baseScale;
    float tailHeight = tailFrame.rect.h * baseScale;
    
    float headCenterX = x + (width * baseScale) / 2.0f;
    float bodyX = headCenterX - (bodyFrame.rect.w * baseScale) / 2.0f - 9.0f;
    
    float clipTimeAmount = getHoldClipAmount();    
    float clipVisualAmount = clipTimeAmount * 0.45f * scrollSpeed;
    float remainingLength = std::max(0.0f, fullHoldLength - clipVisualAmount);
    if (remainingLength <= 0 || holdReleased) return;
    
    float originalAlpha = alpha;
    
    if (wasGoodHit && !isHolding && !holdReleased) {
        alpha = 0.3f;
    } else {
        alpha = 0.6f;
    }
    
    float targetY = getTargetY();
    float holdStartY;
    
    if (wasGoodHit && isHolding) {
        if (isDownscroll) {
            holdStartY = targetY - bodyHeight;
        } else {
            holdStartY = targetY + (height * baseScale);
        }
    } else {
        if (isDownscroll) {
            holdStartY = y - bodyHeight;
        } else {
            holdStartY = y + (height * baseScale);
        }
    }
    
    float bodyLengthToDraw = std::max(0.0f, remainingLength - tailHeight);
    float currentY = holdStartY;
    
    if (isDownscroll) {
        currentY = holdStartY;
        float drawnLength = 0.0f;
        
        while (drawnLength < bodyLengthToDraw) {
            float segmentHeight = std::min(bodyHeight, bodyLengthToDraw - drawnLength);
            float segmentScale = segmentHeight / (bodyFrame.rect.h * baseScale);
            
            drawFrame(bodyFrameIndex, bodyX, currentY, baseScale, baseScale * segmentScale);
            
            currentY -= segmentHeight;
            drawnLength += segmentHeight;
        }
        
        float tailX = headCenterX - (tailFrame.rect.w * baseScale) / 2.0f - 9.0f;
        drawFrame(tailFrameIndex, tailX, currentY, baseScale, baseScale);
    } else {
        currentY = holdStartY;
        float drawnLength = 0.0f;
        
        while (drawnLength < bodyLengthToDraw) {
            float segmentHeight = std::min(bodyHeight, bodyLengthToDraw - drawnLength);
            float segmentScale = segmentHeight / (bodyFrame.rect.h * baseScale);
            
            drawFrame(bodyFrameIndex, bodyX, currentY, baseScale, baseScale * segmentScale);
            
            currentY += segmentHeight;
            drawnLength += segmentHeight;
        }
        
        float tailX = headCenterX - (tailFrame.rect.w * baseScale) / 2.0f - 9.0f;
        drawFrame(tailFrameIndex, tailX, currentY, baseScale, baseScale);
    }
    
    alpha = originalAlpha;
}

void NoteSprite::update(float elapsed) {
    FlxSprite::update(elapsed);

    if (animation) {
        animation->update(elapsed);
    }

    float songPos = Conductor::songPosition;
    float scrollSpeed = PlayState::getScrollSpeedForMustPress(mustPress);
    
    float targetY = getTargetY();
    
    float timeDiff = strumTime - songPos;
    float distance = timeDiff * 0.45f * scrollSpeed;
    
    float newY = targetY + (GameConfig::getInstance()->isDownscroll() ? -distance : distance);
    
    setPosition(x, newY);
    visible = true;
    
    flipY = false;

    if (mustPress) {
        if (strumTime > songPos - Conductor::safeZoneOffset &&
            strumTime < songPos + (Conductor::safeZoneOffset * 0.5f)) {
            canBeHit = true;
        } else {
            canBeHit = false;
        }

        if (strumTime < songPos - Conductor::safeZoneOffset && !wasGoodHit) {
            tooLate = true;
        }
    } else {
        canBeHit = false;
        if (strumTime <= songPos) {
            wasGoodHit = true;
        }
    }
}

NoteSprite::~NoteSprite() {}

void NoteSprite::reset(float newStrumTime, int newNoteData, float newSustainLength, bool newMustPress) {
    strumTime = newStrumTime;
    sustainLength = newSustainLength;
    mustPress = newMustPress;
    
    canBeHit = false;
    wasGoodHit = false;
    tooLate = false;
    noteScore = 1.0f;
    kill = false;
    hitTime = 0.0f;
    isHolding = false;
    holdReleased = false;
    handledMiss = false;
    holdClipTime = 0.0f;
    visible = true;
    alpha = 1.0f;
    
    bool needsAnimUpdate = (noteData != newNoteData);
    noteData = newNoteData;
    
    if (needsAnimUpdate) {
        cacheFrameIndices();
        
        std::string framePrefix;
        switch (noteData) {
            case LEFT_NOTE:  framePrefix = "purple"; break;
            case DOWN_NOTE:  framePrefix = "blue"; break;
            case UP_NOTE:    framePrefix = "green"; break;
            case RIGHT_NOTE: framePrefix = "red"; break;
            default:         framePrefix = "purple"; break;
        }
        
        if (noteFrames && animation) {
            auto scrollFrames = noteFrames->getFramesByPrefix(framePrefix);
            if (!scrollFrames.empty()) {
                animation->addByPrefix("scroll", scrollFrames, 24, false);
                animation->play("scroll");
            }
        }
    } else if (animation) {
        animation->play("scroll");
    }
    
    x = STRUM_X + 50.0f + swagWidth * noteData;
    if (mustPress) {
        x += static_cast<float>(flixel::FlxG::width) / 2.0f;
    }
    y = -2000.0f;
}
