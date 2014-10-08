#pragma once

#include "ofMain.h"
#include "ofxOsc.h"

// The number of actual speakers. We'll index them 1-N, and the 0th speaker is
// a virtual speaker in the center
#define NUM_REAL_SPEAKERS 5
#define NUM_SPEAKERS (NUM_REAL_SPEAKERS + 1)

class ofApp : public ofBaseApp{
    public:
        void setup();
        void update();
        void draw();

        void keyPressed(int key);
        void keyReleased(int key);
        void mouseMoved(int x, int y);
        void mouseDragged(int x, int y, int button);
        void mousePressed(int x, int y, int button);
        void mouseReleased(int x, int y, int button);
        void windowResized(int w, int h);
        void dragEvent(ofDragInfo dragInfo);
        void gotMessage(ofMessage msg);

    private:

        void handleOscMsgs();
        void updateWeights();
        void DrawSpeakers();
        ofVec2f getPathPoint();

        ofxOscReceiver receiver;
        ofVec2f speakerPos[NUM_SPEAKERS];
        bool speakerSpeaking[NUM_SPEAKERS];
        float speakerWeight[NUM_SPEAKERS];
        float phase;
        float lastTime;
};
