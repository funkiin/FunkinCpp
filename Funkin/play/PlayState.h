#pragma once

#include <string>
#include <vector>
#include <map>
#include <SDL.h>
#include "../FunkinState.h"
#include "notes/NoteSprite.h"
#include "song/Song.h"
#include "song/Conductor.h"
#include "../game/GameConfig.h"
#include "PauseSubState.h"
#include "GameOverSubState.h"
#include "stage/Stage.h"
#include "character/Character.h"
#include "character/CharacterManager.h"
#include "components/HealthIcon.h"
#include "components/PopUpStuff.h"
#include "components/HealthBar.h"
#include "scoring/Scoring.h"
#include "Countdown.h"
#include "notes/Strumline.h"
#include "notes/NoteManager.h"
#include "notes/NoteHitHandler.h"
#include "notes/NoteUpdateHandler.h"
#include "input/Controls.h"
#include "song/SongLoader.h"
#include "GameplayManager.h"
#include "CameraManager.h"
#include "PauseHandler.h"
#include "PlayStateRenderer.h"
#include "../scripting/ScriptManager.h"
#include "event/SongEventHandler.h"
#include <flixel/FlxSprite.h>
#include <flixel/text/FlxText.h>
#include <flixel/sound/FlxSound.h>
#include <flixel/FlxCamera.h>
#include <flixel/tweens/FlxTween.h>

class PlayState : public FunkinState {
public:
    static PlayState* instance;
    static SwagSong SONG;
    static flixel::FlxSound* inst;
    static int deathCounter;
    static int storyDifficulty;
    static bool seenCutscene;
    static bool isStoryMode;
    static bool practiceMode;
    static std::vector<std::string> storyPlaylist;
    static int storyWeek;
    static int campaignScore;
    static std::vector<CachedNoteData> cachedNoteData;
    static std::string cachedSongName;
    static float baseScrollSpeed;
    static float playerScrollSpeed;
    static float opponentScrollSpeed;

    PlayState();
    ~PlayState() override;

    void create() override;
    void update(float elapsed) override;
    void draw() override;
    void destroy() override;

    void startSong();
    void endSong();
    void startCountdown();
    void restartSong();
    void updateCameraZoom();
    void setupHUDCamera();
    void stepHit() override;
    void beatHit();
    
    flixel::FlxObject* getCamFollow() const { 
        return cameraManager ? cameraManager->getCamFollow() : nullptr; 
    }
    
    Character* getBoyfriend() const { return boyfriend; }
    Character* getDad() const { return dad; }
    Character* getGf() const { return gf; }
    HealthBar* getHealthBar() const { return healthBar; }
    NoteHitHandler* getNoteHitHandler() const { return noteHitHandler; }
    flixel::FlxText* getScoreText() const { return scoreText; }
    flixel::FlxCamera* getCamGame() const { return camGame; }
    flixel::FlxCamera* getCamHUD() const { return camHUD; }
    CameraManager* getCameraManager() const { return cameraManager; }
    Stage* getStage() const { return stage; }
    Strumline* getPlayerStrumline() const { return playerStrumline; }
    Strumline* getOpponentStrumline() const { return opponentStrumline; }
    int getCurBeat() const { return curBeat; }
    int getCurStep() const { return curStep; }
    bool setTargetBopSpeed(const std::string& target, float rate);
    void tweenScrollSpeed(float scroll, float durationSeconds,
                          flixel::tweens::EaseFunction ease,
                          const std::vector<std::string>& strumlines);
    static float getScrollSpeedForMustPress(bool mustPress);
    
    bool persistentUpdate;
    bool persistentDraw;

private:
    flixel::FlxCamera* camGame;
    flixel::FlxCamera* camHUD;
    flixel::FlxSound* vocals;
    flixel::FlxSound* opponentVocals;
    Stage* stage;
    Character* boyfriend;
    Character* gf;
    Character* dad;
    CharacterManager* characterManager;
    Strumline* opponentStrumline;
    Strumline* playerStrumline;
    NoteManager* noteManager;
    NoteHitHandler* noteHitHandler;
    NoteUpdateHandler* noteUpdateHandler;
    GameplayManager* gameplayManager;
    CameraManager* cameraManager;
    PauseHandler* pauseHandler;
    PlayStateRenderer* renderer;
    SongEventHandler* eventHandler;
    flixel::FlxText* scoreText;
    
    std::string curSong;
    std::string vocalsPath;
    std::string instPath;
    bool startingSong;
    bool startedCountdown;
    Uint32 musicStartTicks;
    
    Countdown* countdown;
    
    int gfSpeed;
    
    HealthBar* healthBar;
    PopUpStuff* popUpStuff;
    
    static const char* NOTE_STYLES[];
    static const char* NOTE_DIRS[];
};
