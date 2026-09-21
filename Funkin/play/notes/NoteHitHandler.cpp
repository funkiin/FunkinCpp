#include "NoteHitHandler.h"
#include "../PlayState.h"
#include "../song/Conductor.h"
#include "../../game/GameConfig.h"
#include "../../scripting/ScriptManager.h"
#include <flixel/FlxG.h>
#include <cmath>
#include <algorithm>
#include <iostream>

NoteHitHandler::NoteHitHandler(Controls* controls, NoteManager* noteManager,
                               Strumline* playerStrumline, Character* boyfriend,
                               HealthBar* healthBar, PopUpStuff* popUpStuff,
                               flixel::FlxText* scoreText, flixel::FlxSound* vocals,
                               flixel::FlxCamera* camHUD)
    : controls(controls)
    , noteManager(noteManager)
    , playerStrumline(playerStrumline)
    , boyfriend(boyfriend)
    , healthBar(healthBar)
    , popUpStuff(popUpStuff)
    , scoreText(scoreText)
    , vocals(vocals)
    , camHUD(camHUD)
    , score(0)
    , misses(0)
    , combo(0)
    , sicks(0)
    , goods(0)
    , bads(0)
    , shits(0)
    , missSound1(nullptr)
    , missSound2(nullptr)
    , missSound3(nullptr)
{
    std::random_device rd;
    rng = std::mt19937(rd());
    
    std::string soundExt = ".ogg";
    missSound1 = new flixel::FlxSound();
    missSound1->loadAsChunk(ASSETS_PATH "assets/sounds/missnote1" + soundExt, false, false);
    missSound2 = new flixel::FlxSound();
    missSound2->loadAsChunk(ASSETS_PATH "assets/sounds/missnote2" + soundExt, false, false);
    missSound3 = new flixel::FlxSound();
    missSound3->loadAsChunk(ASSETS_PATH "assets/sounds/missnote3" + soundExt, false, false);
}

NoteHitHandler::~NoteHitHandler() {
    if (missSound1) {
        delete missSound1;
        missSound1 = nullptr;
    }
    if (missSound2) {
        delete missSound2;
        missSound2 = nullptr;
    }
    if (missSound3) {
        delete missSound3;
        missSound3 = nullptr;
    }
}

void NoteHitHandler::handleInput() {
    if (!controls || !noteManager) return;
    
    auto& notes = noteManager->getActiveNotes();
    
    float closestDistances[4] = {INFINITY, INFINITY, INFINITY, INFINITY};
    std::vector<NoteSprite*> toHit;
    
    bool justHitArray[4] = {
        controls->justPressedAction(ControlAction::NOTE_LEFT),
        controls->justPressedAction(ControlAction::NOTE_DOWN),
        controls->justPressedAction(ControlAction::NOTE_UP),
        controls->justPressedAction(ControlAction::NOTE_RIGHT)
    };
    
    bool heldArray[4] = {
        controls->pressedAction(ControlAction::NOTE_LEFT),
        controls->pressedAction(ControlAction::NOTE_DOWN),
        controls->pressedAction(ControlAction::NOTE_UP),
        controls->pressedAction(ControlAction::NOTE_RIGHT)
    };
    
    if (boyfriend && (justHitArray[0] || justHitArray[1] || justHitArray[2] || justHitArray[3])) {
        boyfriend->holdTimer = 0.0f;
    }
    
    for (int i = 0; i < 4; i++) {
        if (justHitArray[i] && playerStrumline) {
            playerStrumline->playPressed(i);
        }
    }
    
    for (auto note : notes) {
        if (!note || note->kill || note->tooLate) {
            continue;
        }
        
        if (!note->mustPress) {
            continue;
        }
        
        int lane = note->noteData;
        if (lane < 0 || lane >= 4) {
            continue;
        }
        
        if (note->wasGoodHit && note->sustainLength > 0) {
            float sustainEndTime = note->strumTime + note->sustainLength;
            bool sustainStillActive = Conductor::songPosition < sustainEndTime;
            
            if (sustainStillActive) {
                if (heldArray[lane]) {
                    note->isHolding = true;
                    
                    if (boyfriend) {
                        std::string animToPlay = "";
                        switch (note->noteData) {
                            case 0: animToPlay = "singLEFT"; break;
                            case 1: animToPlay = "singDOWN"; break;
                            case 2: animToPlay = "singUP"; break;
                            case 3: animToPlay = "singRIGHT"; break;
                        }
                        if (!animToPlay.empty()) {
                            boyfriend->holdTimer = 0.0f;
                            boyfriend->playAnim(animToPlay, true);
                        }
                    }
                    
                    if (playerStrumline) {
                        playerStrumline->playNote(lane, true);
                    }
                } else if (note->isHolding && !note->holdReleased) {
                    bool hasUpcomingNote = false;
                    for (auto otherNote : notes) {
                        if (otherNote && otherNote != note && 
                            otherNote->mustPress && otherNote->noteData == lane &&
                            !otherNote->wasGoodHit && !otherNote->tooLate && !otherNote->kill) {
                            float timeDiff = otherNote->strumTime - Conductor::songPosition;
                            if (timeDiff >= -50.0f && timeDiff <= 200.0f) {
                                hasUpcomingNote = true;
                                break;
                            }
                        }
                    }
                    
                    note->isHolding = false;
                    note->holdReleased = true;
                    
                    if (!hasUpcomingNote) {
                        misses++;
                        combo = 0;
                        
                        if (healthBar) {
                            healthBar->setHealth(healthBar->getHealth() - 0.05f);
                        }
                        
                        playMissSound();
                        
                        if (boyfriend) {
                            std::string missAnim = "";
                            switch (lane) {
                                case 0: missAnim = "singLEFTmiss"; break;
                                case 1: missAnim = "singDOWNmiss"; break;
                                case 2: missAnim = "singUPmiss"; break;
                                case 3: missAnim = "singRIGHTmiss"; break;
                            }
                            if (!missAnim.empty()) {
                                boyfriend->playAnim(missAnim, true);
                            }
                        }
                        
                        updateScoreText();
                    }
                    
                    if (playerStrumline) {
                        playerStrumline->playStatic(lane);
                    }
                }
            }
            continue;
        }
        
        if (!note->canBeHit) {
            continue;
        }
        
        if (!justHitArray[lane]) {
            continue;
        }
        
        float rawHitTime = note->strumTime - Conductor::songPosition;
        float distance = std::abs(rawHitTime);
        
        float& closestLaneDistance = closestDistances[lane];
        
        if (closestLaneDistance != INFINITY && std::abs(closestLaneDistance - distance) > 5.0f) {
            continue;
        }
        
        closestLaneDistance = distance;
        
        toHit.erase(std::remove_if(toHit.begin(), toHit.end(), [lane, distance](NoteSprite* n) {
            return n->noteData == lane && std::abs(n->strumTime - Conductor::songPosition) > distance;
        }), toHit.end());
        
        toHit.push_back(note);
    }
    
    for (auto note : toHit) {
        goodNoteHit(note);
    }
    
    if (!GameConfig::getInstance()->isGhostTapping()) {
        for (int i = 0; i < 4; i++) {
            if (justHitArray[i] && closestDistances[i] == INFINITY) {
            }
        }
    }
    
    ControlAction releaseActions[4] = {
        ControlAction::NOTE_LEFT,
        ControlAction::NOTE_DOWN,
        ControlAction::NOTE_UP,
        ControlAction::NOTE_RIGHT
    };
    
    for (int i = 0; i < 4; i++) {
        if (controls->justReleasedAction(releaseActions[i]) && playerStrumline) {
            playerStrumline->playStatic(i);
        }
    }
}

void NoteHitHandler::goodNoteHit(NoteSprite* note) {
    if (!note->wasGoodHit) {
        note->wasGoodHit = true;
        note->hitTime = Conductor::songPosition;
        
        if (note->sustainLength > 0) {
            note->isHolding = true;
        }
        
        if (vocals) {
            vocals->setVolume(1.0f);
        }
        
        if (note->noteData >= 0 && note->noteData < 4) {
            if (playerStrumline) {
                playerStrumline->playNote(note->noteData, true);
            }
            
            if (boyfriend) {
                std::string animToPlay = "";
                switch (note->noteData) {
                    case 0: animToPlay = "singLEFT"; break;
                    case 1: animToPlay = "singDOWN"; break;
                    case 2: animToPlay = "singUP"; break;
                    case 3: animToPlay = "singRIGHT"; break;
                }
                if (!animToPlay.empty()) {
                    boyfriend->playAnim(animToPlay, true);
                    boyfriend->holdTimer = 0.0f;
                }
            }
        }

        float noteDiff = note->strumTime - Conductor::songPosition;
        float safeZoneOffset = (10.0f / 60.0f) * 1000.0f;
        
        Scoring::Judgement judgement = Scoring::judgeNote(noteDiff, safeZoneOffset);
        
        ScriptManager::getInstance()->callAll(ScriptCallback::ON_NOTE_HIT, {note->noteData, judgement.rating});
        
        if (judgement.rating == "shit") {
            shits++;
        } else if (judgement.rating == "bad") {
            bads++;
        } else if (judgement.rating == "good") {
            goods++;
        } else {
            sicks++;
        }
        
        if (healthBar) {
            healthBar->setHealth(healthBar->getHealth() + 0.05f);
        }
        combo++;
        
        if (!PlayState::practiceMode) {
            score += judgement.score;
        }
        
        if (popUpStuff) {
            popUpStuff->popUpScore(judgement.rating, combo, Conductor::crochet, camHUD);
        }
        
        updateScoreText();
        
        if (note->sustainLength <= 0) {
            note->kill = true;
        }
    }
}

void NoteHitHandler::updateScoreText() {
    if (!scoreText) return;
    
    std::string text = "Score: " + std::to_string(score) + 
                      " | Misses: " + std::to_string(misses) + 
                      " | Accuracy: ";
    
    float accuracy = Scoring::calculateAccuracy(sicks, goods, bads, shits, misses);
    char accStr[16];
    snprintf(accStr, sizeof(accStr), "%.2f%%", accuracy);
    text += accStr;
    
    scoreText->setText(text);
}

void NoteHitHandler::playMissSound() {
    std::uniform_int_distribution<> missNoteDist(1, 3);
    std::uniform_real_distribution<> volumeDist(0.5, 0.8);
    
    int missNoteNum = missNoteDist(rng);
    float volume = static_cast<float>(volumeDist(rng));
    
    flixel::FlxSound* selectedSound = nullptr;
    switch(missNoteNum) {
        case 1: selectedSound = missSound1; break;
        case 2: selectedSound = missSound2; break;
        case 3: selectedSound = missSound3; break;
    }
    
    if (selectedSound) {
        selectedSound->setVolume(volume);
        selectedSound->play(true);
    }
}
