#include "TitleState.h"
#include "MainMenuState.h"
#include "../play/song/Conductor.h"
#include "../play/scoring/Highscore.h"
#include "../game/GameConfig.h"
#include "../play/input/Controls.h"
#include <flixel/FlxG.h>
#include <flixel/FlxGame.h>
#include <flixel/FlxCamera.h>
#include <flixel/graphics/frames/FlxAtlasFrames.h>
#include <flixel/util/FlxTimer.h>
#include <flixel/util/FlxColor.h>
#include <SDL.h>
#include <iostream>
#include <fstream>
#include <sstream>

TitleState::TitleState(bool skipIntro) 
    : logo(nullptr)
    , gf(nullptr)
    , enter(nullptr)
    , confirmSound(nullptr)
    , whiteAlpha(0.0f)
    , skippedIntro(skipIntro)
    , transitioning(false)
    , musicStartTicks(0)
    , musicStarted(false)
{
}

TitleState::~TitleState() {
    destroy();
}

void TitleState::create() {
    if (!flixel::FlxG::camera) {
        flixel::FlxG::camera = new flixel::FlxCamera(0.0f, 0.0f, 0, 0, 1.0f);
    }
    
    Conductor::changeBPM(102.0f);
    Highscore::load();
    
    if (!flixel::FlxG::sound.music || !Mix_PlayingMusic()) {
        flixel::FlxG::sound.playMusic(ASSETS_PATH "assets/music/freakyMenu.ogg", 1.0f, true);
    }
    
    confirmSound = new flixel::FlxSound();
    confirmSound->loadAsChunk(ASSETS_PATH "assets/sounds/confirmMenu.ogg", false, false);
    
    gf = new flixel::FlxSprite(flixel::FlxG::width * 0.4f, flixel::FlxG::height * 0.07f);
    {
        std::ifstream file(ASSETS_PATH "assets/images/menu/title/gfDanceTitle.xml");
        std::string xmlText((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        
        auto gfFrames = flixel::graphics::frames::FlxAtlasFrames::fromSparrow(
            ASSETS_PATH "assets/images/menu/title/gfDanceTitle.png",
            xmlText
        );
        gf->frames = gfFrames;
        gf->texture = gfFrames->texture;
        gf->ownsTexture = false;
        gf->animation = new flixel::animation::FlxAnimationController();
        
        auto gfIndices = gfFrames->getFramesByPrefix("gfDance");
        if (!gfIndices.empty()) {
            gf->animation->addByPrefix("gfDance", gfIndices, 24, true);
            gf->animation->play("gfDance");
        }
    }
    gf->alpha = 0.0f;
    
    logo = new flixel::FlxSprite(-150, -100);
    {
        std::ifstream file(ASSETS_PATH "assets/images/menu/title/logoBumpin.xml");
        std::string xmlText((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        
        auto logoFrames = flixel::graphics::frames::FlxAtlasFrames::fromSparrow(
            ASSETS_PATH "assets/images/menu/title/logoBumpin.png",
            xmlText
        );
        logo->frames = logoFrames;
        logo->texture = logoFrames->texture;
        logo->ownsTexture = false;
        logo->animation = new flixel::animation::FlxAnimationController();
        
        auto logoIndices = logoFrames->getFramesByPrefix("logo bumpin");
        if (!logoIndices.empty()) {
            logo->animation->addByPrefix("logo bumpin", logoIndices, 24, true);
            logo->animation->play("logo bumpin");
        }
    }
    logo->alpha = 0.0f;
    
    enter = new flixel::FlxSprite(100, flixel::FlxG::height * 0.8f);
    {
        std::ifstream file(ASSETS_PATH "assets/images/menu/title/titleEnter.xml");
        std::string xmlText((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        
        auto enterFrames = flixel::graphics::frames::FlxAtlasFrames::fromSparrow(
            ASSETS_PATH "assets/images/menu/title/titleEnter.png",
            xmlText
        );
        enter->frames = enterFrames;
        enter->texture = enterFrames->texture;
        enter->ownsTexture = false;
        enter->animation = new flixel::animation::FlxAnimationController();
        
        auto pressedIndices = enterFrames->getFramesByPrefix("ENTER PRESSED");
        auto beginIndices = enterFrames->getFramesByPrefix("Press Enter to Begin");
        if (!pressedIndices.empty()) {
            enter->animation->addByPrefix("ENTER PRESSED", pressedIndices, 24, true);
        }
        if (!beginIndices.empty()) {
            enter->animation->addByPrefix("Press Enter to Begin", beginIndices, 24, true);
            enter->animation->play("Press Enter to Begin");
        }
    }
    enter->alpha = 0.0f;
    
    if (skippedIntro) {
        gf->alpha = 1.0f;
        logo->alpha = 1.0f;
        enter->alpha = 1.0f;
    }
}

void TitleState::update(float elapsed) {
    if (Mix_PlayingMusic()) {
        if (!musicStarted) {
            musicStartTicks = SDL_GetTicks();
            musicStarted = true;
        }
        Conductor::songPosition = static_cast<float>(SDL_GetTicks() - musicStartTicks);
    } else {
        musicStarted = false;
    }
    
    FunkinState::update(elapsed);
    
    if (gf) {
        gf->update(elapsed);
        if (gf->animation) gf->animation->update(elapsed);
    }
    if (logo) {
        logo->update(elapsed);
        if (logo->animation) logo->animation->update(elapsed);
    }
    if (enter) {
        enter->update(elapsed);
        if (enter->animation) enter->animation->update(elapsed);
    }
    
    for (auto* alpha : alphabets) {
        if (alpha) alpha->update(elapsed);
    }
    
    if (whiteAlpha > 0.0f) {
        whiteAlpha -= elapsed * 2.0f;
        if (whiteAlpha < 0.0f) whiteAlpha = 0.0f;
    }
    
    Controls* controls = GameConfig::getInstance()->controls;
    
    if (controls->justPressedAction(ControlAction::ACCEPT) && !skippedIntro) {
        skipIntro();
    }
    else if (controls->justPressedAction(ControlAction::ACCEPT) && skippedIntro && !transitioning) {
        if (enter) {
            enter->animation->play("ENTER PRESSED", true);
        }
        if (confirmSound) {
            confirmSound->play();
        }
        transitioning = true;
        
        auto timer = new flixel::util::FlxTimer();
        timer->start(1.0f, [this](flixel::util::FlxTimer* t) {
            startTransitionOut(0.5f, flixel::util::FlxColor::BLACK, flixel::FlxPoint(0, 1), []() {
                flixel::FlxG::game->switchState(new MainMenuState());
            });
            delete t;
        });
    }
}

void TitleState::draw() {
    SDL_Renderer* renderer = flixel::FlxG::renderer;
    
    if (!skippedIntro) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
    }
    
    if (gf && gf->visible) gf->draw();
    if (logo && logo->visible) logo->draw();
    if (enter && enter->visible) enter->draw();
    
    for (auto* alpha : alphabets) {
        if (alpha) alpha->draw();
    }
    
    if (whiteAlpha > 0.0f) {
        SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, static_cast<Uint8>(whiteAlpha * 255));
        SDL_Rect rect = {0, 0, flixel::FlxG::width, flixel::FlxG::height};
        SDL_RenderFillRect(renderer, &rect);
    }
    
    FunkinState::draw();
}

void TitleState::destroy() {
    for (auto* alpha : alphabets) {
        if (alpha) {
            delete alpha;
        }
    }
    alphabets.clear();
    
    if (gf) {
        delete gf;
        gf = nullptr;
    }
    if (logo) {
        delete logo;
        logo = nullptr;
    }
    if (enter) {
        delete enter;
        enter = nullptr;
    }
    if (confirmSound) {
        delete confirmSound;
        confirmSound = nullptr;
    }
    
    FunkinState::destroy();
}

void TitleState::createCoolText(const std::vector<std::string>& lines) {
    int lineHeight = 60;
    int startY = 200;
    
    for (size_t i = 0; i < lines.size(); ++i) {
        Alphabet* alpha = new Alphabet(lines[i], 0, startY + static_cast<int>(i) * lineHeight);
        alpha->screenCenter();
        alphabets.push_back(alpha);
    }
}

void TitleState::createMoreCoolText(const std::string& line) {
    Alphabet* alpha = new Alphabet(line, 0, static_cast<int>(alphabets.size() * 60 + 200));
    alpha->screenCenter();
    alphabets.push_back(alpha);
}

void TitleState::removeText() {
    for (auto* alpha : alphabets) {
        if (alpha) {
            delete alpha;
        }
    }
    alphabets.clear();
}

void TitleState::skipIntro() {
    skippedIntro = true;
    removeText();
    whiteAlpha = 1.0f;
    
    if (gf) gf->alpha = 1.0f;
    if (logo) logo->alpha = 1.0f;
    if (enter) enter->alpha = 1.0f;
}

void TitleState::beatHit() {
    if (skippedIntro) return;
    
    switch (curBeat) {
    case 1:
        createCoolText({"NINJAMUFFIN99", "PHANTOMARCADE", "KAWAISPRITE", "EVILSK8R"});
        break;
    case 3:
        createMoreCoolText("PRESENT");
        break;
    case 4:
        removeText();
        break;
    case 5:
        createCoolText({"NOT IN ASSOCIATION", "WITH"});
        break;
    case 7:
        createMoreCoolText("NEWGROUNDS");
        break;
    case 8:
        removeText();
        break;
    case 9:
        createCoolText({"PORT BY"});
        break;
    case 11:
        createCoolText({"DOTFLA"});
        break;
    case 12:
        removeText();
        break;
    case 13:
        createMoreCoolText("FRIDAY");
        break;
    case 14:
        createMoreCoolText("NIGHT");
        break;
    case 15:
        createMoreCoolText("FUNKIN");
        break;
    case 16:
        skipIntro();
        break;
    }
}
