#pragma once
#include "SongEventData.h"
#include <vector>

class SongEventHandler {
public:
    SongEventHandler();

    void loadEvents(std::vector<SongEvent>& events);
    void update(float songPosition);
    void reset();

private:
    std::vector<SongEvent>* events = nullptr;

    void dispatch(const SongEvent& e);
    void handleFocusCamera(const SongEvent& e);
    void handleZoomCamera(const SongEvent& e);
    void handlePlayAnimation(const SongEvent& e);
    void handleSetCameraBop(const SongEvent& e);
    void handleScrollSpeed(const SongEvent& e);
    void handleSetHealthIcon(const SongEvent& e);
    void handleSetTargetBopSpeed(const SongEvent& e);
};
