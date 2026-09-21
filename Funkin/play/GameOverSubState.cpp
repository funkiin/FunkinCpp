#include "GameOverSubState.h"
#include "PlayState.h"
#include "song/Conductor.h"
#include "song/Song.h"
#include "../game/GameConfig.h"
#include "../ui/NewFreeplayState.h"
#include "../ui/StoryMenuState.h"
#include "input/Controls.h"
#include <flixel/FlxG.h>
#include <flixel/FlxGame.h>
#include <flixel/FlxCamera.h>
#include <flixel/util/FlxTimer.h>
#include <iostream>

GameOverSubState::GameOverSubState(float x, float y, flixel::FlxCamera* camera)
    : blackScreen(nullptr)
    , bf(nullptr)
    , camFollow(nullptr)
    , gameOverMusic(nullptr)
    , gameCamera(camera)
    , isEnding(false)
    , bfX(x)
    , bfY(y)
{
}

GameOverSubState::~GameOverSubState() {
    destroy();
}

void GameOverSubState::create() {
    Conductor::songPosition = 0.0f;
    
    blackScreen = new flixel::FlxSprite(0, 0);
    blackScreen->makeGraphic(flixel::FlxG::width, flixel::FlxG::height, {0, 0, 0, 255});
    blackScreen->scrollFactor.x = 0.0f;
    blackScreen->scrollFactor.y = 0.0f;
    blackScreen->camera = gameCamera;
    
    bf = new Character(bfX, bfY, "bf", true);
    if (bf) {
        bf->playAnim("firstDeath", true);
        bf->camera = gameCamera;
    }
    
    if (bf) {
        camFollow = new flixel::FlxObject(bf->x + bf->width / 2.0f, bf->y + bf->height / 2.0f, 1, 1);
    } else {
        camFollow = new flixel::FlxObject(bfX, bfY, 1, 1);
    }
    
    flixel::FlxG::sound.playAsChunk(ASSETS_PATH "assets/sounds/fnf_loss_sfx.ogg");    
    Conductor::changeBPM(100.0f);
    
    if (gameCamera) {
        gameCamera->scroll.x = 0;
        gameCamera->scroll.y = 0;
        gameCamera->target = nullptr;
    }
}

void GameOverSubState::update(float elapsed) {
    FlxSubState::update(elapsed);
    
    if (blackScreen) blackScreen->update(elapsed);
    if (bf) bf->update(elapsed);
    if (camFollow) camFollow->update(elapsed);
    if (gameCamera) gameCamera->update(elapsed);
    
    Controls* controls = GameConfig::getInstance()->controls;
    
    if (controls->justPressedAction(ControlAction::ACCEPT)) {
        endBullshit();
    }
    
    if (controls->justPressedAction(ControlAction::BACK) && !isEnding) {
        isEnding = true;
        
        if (gameOverMusic) {
            gameOverMusic->stop();
        }
        
        if (flixel::FlxG::camera) {
            flixel::FlxG::camera->fade(flixel::util::FlxColor::BLACK, 0.5f, false, []() {
                if (PlayState::isStoryMode) {
                    flixel::FlxG::game->switchState(new StoryMenuState());
                } else {
                    flixel::FlxG::game->switchState(new NewFreeplayState(false, flixel::FlxPoint(0.0f, 0.0f)));
                }
            });
        } else {
            if (PlayState::isStoryMode) {
                flixel::FlxG::game->switchState(new StoryMenuState());
            } else {
                flixel::FlxG::game->switchState(new NewFreeplayState(false, flixel::FlxPoint(0.0f, 0.0f)));
            }
        }
    }
    
    if (bf) {
        if (bf->getCurrentAnimName() == "firstDeath" && bf->getCurrentAnimFrame() == 12) {
            if (gameCamera && camFollow) {
                gameCamera->follow(camFollow, flixel::FlxCameraFollowStyle::LOCKON, 0.01f);
            }
        }
        
        if (bf->getCurrentAnimName() == "firstDeath" && bf->isCurrentAnimFinished()) {
            if (!gameOverMusic) {
                gameOverMusic = new flixel::FlxSound();
                if (gameOverMusic->loadStream(ASSETS_PATH "assets/music/gameOver.ogg", true)) {
                    gameOverMusic->play();
                }
            }
            bf->playAnim("deathLoop", true);
        }
    }
    
    if (gameOverMusic && gameOverMusic->playing) {
        Conductor::songPosition = gameOverMusic->get_time() * 1000.0f;
    }
}

void GameOverSubState::draw() {
    if (blackScreen) blackScreen->draw();
    if (bf) bf->draw();
    
    FlxSubState::draw();
}

void GameOverSubState::beatHit() {}

void GameOverSubState::endBullshit() {
    if (!isEnding) {
        isEnding = true;
        
        if (bf) {
            bf->playAnim("deathConfirm", true);
        }
        
        if (gameOverMusic) {
            gameOverMusic->stop();
        }
        
        flixel::FlxG::sound.playAsChunk(ASSETS_PATH "assets/music/gameOverEnd.ogg");
        
        flixel::util::FlxTimer* timer = new flixel::util::FlxTimer();
        timer->start(0.7f, [this](flixel::util::FlxTimer* t) {
            if (flixel::FlxG::camera) {
                flixel::FlxG::camera->fade(flixel::util::FlxColor::BLACK, 2.0f, false, []() {
                    PlayState::deathCounter++;
                    flixel::FlxG::game->switchState(new PlayState());
                });
            } else {
                PlayState::deathCounter++;
                flixel::FlxG::game->switchState(new PlayState());
            }
            delete t;
        }, 1);
    }
}

void GameOverSubState::destroy() {
    if (blackScreen) {
        delete blackScreen;
        blackScreen = nullptr;
    }
    if (bf) {
        delete bf;
        bf = nullptr;
    }
    if (camFollow) {
        delete camFollow;
        camFollow = nullptr;
    }
    if (gameOverMusic) {
        delete gameOverMusic;
        gameOverMusic = nullptr;
    }
}
