#include "PlayState.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <random>
#include "song/Song.h"
#include "song/Conductor.h"
#include "scoring/Highscore.h"
#include <map>
#include <flixel/FlxG.h>
#include <flixel/FlxGame.h>
#include <flixel/math/FlxMath.h>
#include <flixel/tweens/FlxTween.h>
#include "../ui/StoryMenuState.h"
#include "../ui/NewFreeplayState.h"

PlayState* PlayState::instance = nullptr;
SwagSong PlayState::SONG;
flixel::FlxSound* PlayState::inst = nullptr;
int PlayState::deathCounter = 0;
int PlayState::storyDifficulty = 1;
bool PlayState::seenCutscene = false;
bool PlayState::isStoryMode = false;
bool PlayState::practiceMode = false;
std::vector<std::string> PlayState::storyPlaylist;
int PlayState::storyWeek = 0;
int PlayState::campaignScore = 0;
std::vector<CachedNoteData> PlayState::cachedNoteData;
std::string PlayState::cachedSongName;
float PlayState::baseScrollSpeed = 1.0f;
float PlayState::playerScrollSpeed = 1.0f;
float PlayState::opponentScrollSpeed = 1.0f;

const char* PlayState::NOTE_STYLES[] = {"purple", "blue", "green", "red"};
const char* PlayState::NOTE_DIRS[] = {"LEFT", "DOWN", "UP", "RIGHT"};

PlayState::PlayState() {
    instance = this;
    inst = nullptr;
    vocals = nullptr;
    opponentVocals = nullptr;
    camGame = nullptr;
    camHUD = nullptr;
    stage = nullptr;
    boyfriend = nullptr;
    gf = nullptr;
    dad = nullptr;
    characterManager = nullptr;
    opponentStrumline = nullptr;
    playerStrumline = nullptr;
    noteManager = nullptr;
    noteHitHandler = nullptr;
    noteUpdateHandler = nullptr;
    gameplayManager = nullptr;
    cameraManager = nullptr;
    pauseHandler = nullptr;
    renderer = nullptr;
    eventHandler = nullptr;
    healthBar = nullptr;
    persistentUpdate = true;
    persistentDraw = true;
    scoreText = new flixel::FlxText(0, 0, 0, "");
    scoreText->setFont(ASSETS_PATH "assets/fonts/vcr.ttf");
    scoreText->setSize(16);
    scoreText->setBorderStyle(flixel::FlxTextBorderStyle::OUTLINE_FAST, {0, 0, 0, 255}, 1.0f);
    scoreText->scrollFactor.x = 0.0f;
    scoreText->scrollFactor.y = 0.0f;
    
    int windowWidth = flixel::FlxG::width;
    int windowHeight = flixel::FlxG::height;
    scoreText->setText("Score: 0 | Misses: 0 | Accuracy: 100.00%");
    
    startingSong = false;
    startedCountdown = false;
    musicStartTicks = 0;
    gfSpeed = 1;

    countdown = new Countdown();
    popUpStuff = new PopUpStuff();
}

PlayState::~PlayState() {
    ScriptManager::getInstance()->clear();
    
    if (vocals != nullptr) {
        delete vocals;
        vocals = nullptr;
    }
    if (opponentVocals != nullptr) {
        delete opponentVocals;
        opponentVocals = nullptr;
    }
    if (inst != nullptr) {
        delete inst;
        inst = nullptr;
    }
    
    if (cameraManager != nullptr) {
        delete cameraManager;
        cameraManager = nullptr;
    }
    if (pauseHandler != nullptr) {
        delete pauseHandler;
        pauseHandler = nullptr;
    }
    if (renderer != nullptr) {
        delete renderer;
        renderer = nullptr;
    }
    if (eventHandler != nullptr) {
        delete eventHandler;
        eventHandler = nullptr;
    }
    
    flixel::FlxG::camera = nullptr;
    
    if (camGame != nullptr) {
        delete camGame;
        camGame = nullptr;
    }
    if (camHUD != nullptr) {
        delete camHUD;
        camHUD = nullptr;
    }
    
    if (stage != nullptr) {
        delete stage;
        stage = nullptr;
    }
    
    if (characterManager != nullptr) {
        delete characterManager;
        characterManager = nullptr;
    }
    if (boyfriend != nullptr) {
        delete boyfriend;
        boyfriend = nullptr;
    }
    if (gf != nullptr) {
        delete gf;
        gf = nullptr;
    }
    if (dad != nullptr) {
        delete dad;
        dad = nullptr;
    }
    
    if (opponentStrumline != nullptr) {
        delete opponentStrumline;
        opponentStrumline = nullptr;
    }
    if (playerStrumline != nullptr) {
        delete playerStrumline;
        playerStrumline = nullptr;
    }
    if (noteManager != nullptr) {
        delete noteManager;
        noteManager = nullptr;
    }
    if (noteHitHandler != nullptr) {
        delete noteHitHandler;
        noteHitHandler = nullptr;
    }
    if (noteUpdateHandler != nullptr) {
        delete noteUpdateHandler;
        noteUpdateHandler = nullptr;
    }
    if (gameplayManager != nullptr) {
        delete gameplayManager;
        gameplayManager = nullptr;
    }
    if (healthBar != nullptr) {
        delete healthBar;
        healthBar = nullptr;
    }
    
    delete scoreText;
    
    if (countdown != nullptr) {
        delete countdown;
        countdown = nullptr;
    }
    if (popUpStuff != nullptr) {
        delete popUpStuff;
        popUpStuff = nullptr;
    }
}

void PlayState::create() {
    Conductor::songPosition = 0;
    curStep = 0;
    curBeat = 0;
    startingSong = true;
    startedCountdown = false;
    
    NoteSprite::loadAssets();
    
    camGame = new flixel::FlxCamera(0.0f, 0.0f, 0, 0, 0.0f);
    camHUD = new flixel::FlxCamera(0.0f, 0.0f, 0, 0, 1.0f);
    
    flixel::FlxG::camera = camGame;
    
    startTransitionIn(0.5f, flixel::util::FlxColor::BLACK, flixel::FlxPoint(0, -1));
    
    setupHUDCamera();
    
    if (SONG.song.empty()) {
        std::cout << "You somehow loaded into PlayState without a song. Congrats I guess?" << std::endl;
        return;
    } else {
        std::string baseSongName = SONG.song;
        std::transform(baseSongName.begin(), baseSongName.end(), baseSongName.begin(), ::tolower);
        curSong = baseSongName;
        if (storyDifficulty == 0) {
            curSong += "-easy";
        } else if (storyDifficulty == 2) {
            curSong += "-hard";
        }
        SongLoader::loadSongAudio(curSong, inst, vocals, opponentVocals, SONG);
    }
    
    if (!SONG.validScore) {
        std::cout << "Error: Failed to load song data!" << std::endl;
        return;
    }
    
    Conductor::mapBPMChanges(SONG);
    Conductor::changeBPM(SONG.bpm);
    baseScrollSpeed = SONG.speed;
    playerScrollSpeed = baseScrollSpeed;
    opponentScrollSpeed = baseScrollSpeed;
    
    std::string stageName = SONG.stage;
    if (stageName.empty()) {
        stageName = Stage::getStageFromSong(SONG.song);
        SONG.stage = stageName;
    }
    
    stage = new Stage(stageName);
    if (stage && camGame) {
        for (auto sprite : stage->getSprites()) {
            if (sprite) {
                sprite->camera = camGame;
            }
        }
        float stageZoom = stage->getDefaultZoom();
        camGame->zoom = stageZoom;
        camGame->initialZoom = stageZoom;
        camGame->updateFlashOffset();
        camGame->updateScrollRect();
    }
    
    gf = new Character(400, 130, SONG.gfVersion, false);
    gf->scrollFactor.set(0.95f, 0.95f);
    if (camGame) {
        gf->camera = camGame;
    }
    
    dad = new Character(100, 100, SONG.player2, false);
    if (camGame) {
        dad->camera = camGame;
    }
    
    boyfriend = new Character(770, 450, SONG.player1, true);
    if (camGame) {
        boyfriend->camera = camGame;
    }
    
    ScriptManager::getInstance()->clear();
    ScriptManager::getInstance()->loadScriptsFromDirectory(ASSETS_PATH "assets/scripts");
    
    std::string songName = SONG.song;
    std::transform(songName.begin(), songName.end(), songName.begin(), ::tolower);
    ScriptManager::getInstance()->loadSongScripts(songName);
    
    std::vector<std::string> characterNames = {SONG.player1, SONG.player2, SONG.gfVersion};
    ScriptManager::getInstance()->loadCharacterScripts(characterNames);
    
    std::string stageScriptName = SONG.stage;
    if (!stageScriptName.empty()) {
        ScriptManager::getInstance()->loadStageScripts(stageScriptName);
    }
    
    ScriptManager::getInstance()->callAll(ScriptCallback::ON_CREATE);
    
    float stageZoom = stage ? stage->getDefaultZoom() : 1.05f;
    cameraManager = new CameraManager(camGame, camHUD, stageZoom);
    cameraManager->initialize(boyfriend, dad, gf);
    characterManager = new CharacterManager(gf, dad, boyfriend, gfSpeed);
    
    int windowWidth = flixel::FlxG::width;
    int windowHeight = flixel::FlxG::height;
    
    float healthBarY = GameConfig::getInstance()->isDownscroll() ? (windowHeight * 0.11f) : (windowHeight * 0.89f);
    float healthBarX = (windowWidth - 601) / 2.0f;
    
    healthBar = new HealthBar(healthBarX, healthBarY, camHUD);
    healthBar->setIcons(SONG.player1, SONG.player2);
    healthBar->setColors(boyfriend->healthColorR, boyfriend->healthColorG, boyfriend->healthColorB,
                        dad->healthColorR, dad->healthColorG, dad->healthColorB);
    healthBar->setHealth(1.0f);
    
    float scoreTextY = GameConfig::getInstance()->isDownscroll() ? 10.0f : static_cast<float>(windowHeight - 50);
    scoreText->setPosition(static_cast<float>(windowWidth / 2 - 100), scoreTextY);
    
    noteManager = new NoteManager(camHUD);
    
    std::string currentSongName = SONG.song;
    std::transform(currentSongName.begin(), currentSongName.end(), currentSongName.begin(), ::tolower);
    
    if (!cachedNoteData.empty() && cachedSongName == currentSongName) {
        noteManager->setCachedNotes(cachedNoteData);
        noteManager->regenerateFromCache();
    } else {
        noteManager->generateNotes(SONG);
        cachedNoteData = noteManager->getCachedNotes();
        cachedSongName = currentSongName;
    }
    
    float strumYPos = GameConfig::getInstance()->isDownscroll() ? (windowHeight - 150.0f) : 50.0f;
    float opponentX = 42.0f + 50.0f;
    float playerX = opponentX + (windowWidth / 2.0f);
    
    opponentStrumline = new Strumline(opponentX, strumYPos, 0, camHUD);
    playerStrumline = new Strumline(playerX, strumYPos, 1, camHUD);
    
    opponentStrumline->fadeInArrows();
    playerStrumline->fadeInArrows();
    
    noteHitHandler = new NoteHitHandler(GameConfig::getInstance()->controls, noteManager, playerStrumline, boyfriend, healthBar, popUpStuff, scoreText, vocals, camHUD);
    gameplayManager = new GameplayManager(noteManager, opponentStrumline, dad, gf, healthBar, vocals, SONG);
    noteUpdateHandler = new NoteUpdateHandler(noteManager, noteHitHandler, gameplayManager, gf);
    pauseHandler = new PauseHandler();
    renderer = new PlayStateRenderer();

    eventHandler = new SongEventHandler();
    eventHandler->loadEvents(SONG.events);

    startCountdown();
}

void PlayState::update(float elapsed) {
    FunkinState::update(elapsed);
    
    ScriptManager::getInstance()->callAll(ScriptCallback::ON_UPDATE, {elapsed});
    ScriptManager::getInstance()->updateScriptObjects(elapsed);
    
    if (pauseHandler) {
        pauseHandler->update(elapsed, inst, vocals, opponentVocals,
                             Conductor::songPosition, musicStartTicks, subState,
                             [this](flixel::FlxSubState* s) { this->openSubState(s); },
                             [this]() { this->closeSubState(); });
    }

    if (!subState) {
        if (camGame) {
            camGame->update(elapsed);
        }
        if (camHUD) {
            camHUD->update(elapsed);
        }
        if (stage) {
            stage->update(elapsed);
        }
        
        if (inst) {
            inst->update(elapsed);
        }
        if (vocals) {
            vocals->update(elapsed);
        }
        if (opponentVocals) {
            opponentVocals->update(elapsed);
        }
        
        if (popUpStuff) {
            popUpStuff->update(elapsed, Conductor::crochet);
        }
        
        if (noteHitHandler) {
            noteHitHandler->handleInput();
        }
        if (gameplayManager) {
            gameplayManager->handleOpponentNotes(elapsed);
        }
        
        if (characterManager) {
            characterManager->update(elapsed);
        }
        
        if (opponentStrumline) {
            opponentStrumline->update(elapsed);
        }
        if (playerStrumline) {
            playerStrumline->update(elapsed);
        }
        
        if (healthBar) {
            healthBar->update(elapsed);
            
            if (healthBar->getHealth() <= 0.0f && !practiceMode && !subState && boyfriend) {
                boyfriend->stunned = true;
                persistentUpdate = false;
                persistentDraw = false;
                
                if (inst) {
                    inst->stop();
                }
                if (vocals) {
                    vocals->stop();
                }
                if (opponentVocals) {
                    opponentVocals->stop();
                }

                openSubState(new GameOverSubState(boyfriend->x, boyfriend->y, camGame));
            }
        }

        if (!startingSong && musicStartTicks > 0) {
            Conductor::songPosition = static_cast<float>(SDL_GetTicks() - musicStartTicks);
        } else {
            Conductor::songPosition += elapsed * 1000.0f;
        }

        if (noteUpdateHandler) {
            noteUpdateHandler->updateNotes(elapsed, boyfriend, vocals);
        }

        if (startedCountdown && startingSong && countdown) {
            countdown->update(elapsed);
            
            flixel::FlxSprite* countdownSprite = countdown->getCurrentSprite();
            if (countdownSprite && camHUD) {
                                countdownSprite->camera = camHUD;
            }
            
            if (Conductor::songPosition >= 0 && countdown->isFinished()) {
                startSong();
            }
        }

        if (cameraManager) {
            cameraManager->update(elapsed);
        }

        if (eventHandler) {
            eventHandler->update(Conductor::songPosition);
        }
    }
}

void PlayState::startSong() {
    startingSong = false;
    musicStartTicks = SDL_GetTicks();
    
    ScriptManager::getInstance()->callAll(ScriptCallback::ON_SONG_START);
    
    if (cameraManager) {
        cameraManager->setCamZooming(true);
    }
    
    if (inst) {
        inst->onComplete = [this]() {
            this->endSong();
        };
        inst->play();
    }

    if (vocals) {
        vocals->play();
    }
    if (opponentVocals) {
        opponentVocals->play();
    }
}

void PlayState::endSong() {
    seenCutscene = false;
    deathCounter = 0;
    
    ScriptManager::getInstance()->callAll(ScriptCallback::ON_SONG_END);
    
    if (inst) {
        inst->setVolume(0.0f);
    }
    if (vocals) {
        vocals->setVolume(0.0f);
    }
    if (opponentVocals) {
        opponentVocals->setVolume(0.0f);
    }
    
    int songScore = noteHitHandler ? noteHitHandler->getScore() : 0;
    float accuracy = 0.0f;
    if (noteHitHandler) {
        int totalHits = noteHitHandler->getSicks() + noteHitHandler->getGoods() + 
                        noteHitHandler->getBads() + noteHitHandler->getShits();
        int totalNotes = totalHits + noteHitHandler->getMisses();
        if (totalNotes > 0) {
            accuracy = static_cast<float>(totalHits) / static_cast<float>(totalNotes);
        }
    }
    
    if (camGame) {
        camGame->fade(flixel::util::FlxColor::BLACK, 0.6f);
    }
    
    startTransitionOut(0.6f, flixel::util::FlxColor::BLACK, flixel::FlxPoint(0, 1), [this, songScore, accuracy]() {
        if (isStoryMode) {
            campaignScore += songScore;
            
            std::string songName = SONG.song;
            std::transform(songName.begin(), songName.end(), songName.begin(), ::tolower);
            Highscore::saveScore(songName, songScore, storyDifficulty, accuracy);
            
            if (!storyPlaylist.empty()) {
                storyPlaylist.erase(storyPlaylist.begin());
            }
            
            if (storyPlaylist.empty()) {
                Highscore::saveWeekScore(storyWeek, campaignScore, storyDifficulty, accuracy);
                
                if (flixel::FlxG::sound.music) {
                    flixel::FlxG::sound.playMusic(ASSETS_PATH "assets/music/freakyMenu.ogg");
                }
                
                flixel::FlxG::game->switchState(new StoryMenuState());
            } else {
                std::string difficulty = "";
                if (storyDifficulty == 0) {
                    difficulty = "-easy";
                } else if (storyDifficulty == 2) {
                    difficulty = "-hard";
                }
                
                std::string nextSong = storyPlaylist[0];
                std::transform(nextSong.begin(), nextSong.end(), nextSong.begin(), ::tolower);
                
                if (inst) {
                    inst->stop();
                }
                if (vocals) {
                    vocals->stop();
                }
                if (opponentVocals) {
                    opponentVocals->stop();
                }

                SONG = Song::loadFromJson(nextSong + difficulty, nextSong);
                flixel::FlxG::game->switchState(new PlayState());
            }
        } else {
            std::string songName = SONG.song;
            std::transform(songName.begin(), songName.end(), songName.begin(), ::tolower);
            Highscore::saveScore(songName, songScore, storyDifficulty, accuracy);
            
            if (flixel::FlxG::sound.music) {
                flixel::FlxG::sound.playMusic(ASSETS_PATH "assets/music/freakyMenu.ogg");
            }
            flixel::FlxG::game->switchState(new NewFreeplayState(true, flixel::FlxPoint(0.0f, 0.0f)));
        }
    });
}

void PlayState::startCountdown() {
    startedCountdown = true;
    
    ScriptManager::getInstance()->callAll(ScriptCallback::ON_COUNTDOWN_STARTED);
    
    if (countdown) {
        countdown->start(Conductor::crochet);
    }

    Conductor::songPosition = -(Conductor::crochet * 5);
}

void PlayState::restartSong() {
    if (inst) {
        inst->stop();
    }
    if (vocals) {
         vocals->stop();
    }
    if (opponentVocals) {
        opponentVocals->stop();
    }
    
    Conductor::songPosition = 0;
    curStep = 0;
    curBeat = 0;
    playerScrollSpeed = baseScrollSpeed;
    opponentScrollSpeed = baseScrollSpeed;
    startingSong = true;
    startedCountdown = false;
    musicStartTicks = 0;
    
    if (healthBar) {
        healthBar->setHealth(1.0f);
    }
    
    if (boyfriend) {
        boyfriend->stunned = false;
    }
    if (dad) {
        dad->stunned = false;
    }
    if (gf) {
        gf->stunned = false;
    }
    
    if (noteHitHandler) {
        noteHitHandler->setScore(0);
        noteHitHandler->setMisses(0);
        noteHitHandler->setCombo(0);
        noteHitHandler->setSicks(0);
        noteHitHandler->setGoods(0);
        noteHitHandler->setBads(0);
        noteHitHandler->setShits(0);
        noteHitHandler->updateScore();
    }
    
    if (noteManager) {
        noteManager->animateNotesDownward();
        noteManager->clearWithoutPooling();
        if (!cachedNoteData.empty() && cachedSongName == curSong) {
            noteManager->regenerateFromCache();
        } else {
            noteManager->generateNotes(SONG);
            cachedNoteData = noteManager->getCachedNotes();
            cachedSongName = curSong;
        }
    }
    
    if (cameraManager) {
        cameraManager->setCamZooming(false);
    }
    
    if (popUpStuff) {
        popUpStuff->clear();
    }
    
    persistentUpdate = true;
    persistentDraw = true;
    
    startCountdown();
}

void PlayState::draw() {
    if (renderer) {
        renderer->draw(stage, gf, dad, boyfriend, opponentStrumline, playerStrumline,
                      noteManager, healthBar, scoreText, popUpStuff, countdown, subState);
    }
    ScriptManager::getInstance()->drawScriptObjects();
    
    FunkinState::draw();
}

void PlayState::destroy() {
    FunkinState::destroy();
}

void PlayState::updateCameraZoom() {}

void PlayState::setupHUDCamera() {
    if (camHUD && scoreText) {
        scoreText->camera = camHUD;
    }
}

void PlayState::stepHit() {
    ScriptManager::getInstance()->callAll(ScriptCallback::ON_STEP_HIT, {curStep});

    if (stage) {
        stage->stepHit(curStep);
    }

    if (curStep % 4 == 0) {
        beatHit();
    }
}

bool PlayState::setTargetBopSpeed(const std::string& target, float rate) {
    if (characterManager && characterManager->setDanceEvery(target, rate)) {
        return true;
    }
    return stage && stage->setNamedPropDanceEvery(target, rate);
}

void PlayState::tweenScrollSpeed(float scroll, float durationSeconds,
                                 flixel::tweens::EaseFunction ease,
                                 const std::vector<std::string>& strumlines) {
    auto applySpeed = [&](float* target) {
        if (!target) return;
        flixel::tweens::cancelTweensOf(target);
        if (durationSeconds <= 0.0f) {
            *target = scroll;
            return;
        }

        auto* tween = new flixel::tweens::VarTween(durationSeconds);
        tween->object = target;
        tween->ease = ease ? ease : flixel::tweens::FlxEase::linear;
        tween->addProperty("scrollSpeed", target, scroll);
        tween->start();

        if (!flixel::tweens::globalManager) {
            flixel::tweens::init();
        }
        flixel::tweens::globalManager->tweens.push_back(tween);
    };

    for (const auto& strumline : strumlines) {
        if (strumline == "playerStrumline") {
            applySpeed(&playerScrollSpeed);
        } else if (strumline == "opponentStrumline") {
            applySpeed(&opponentScrollSpeed);
        }
    }
}

float PlayState::getScrollSpeedForMustPress(bool mustPress) {
    return mustPress ? playerScrollSpeed : opponentScrollSpeed;
}

void PlayState::beatHit() {
    ScriptManager::getInstance()->callAll(ScriptCallback::ON_BEAT_HIT, {curBeat});

    if (cameraManager) {
        cameraManager->beatHit(curBeat);
    }

    if (characterManager) {
        characterManager->beatHit(curBeat, curStep, SONG);
    }

    if (stage) {
        stage->beatHit(curBeat);
    }
}
