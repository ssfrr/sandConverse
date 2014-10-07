#pragma once

#include "ofMain.h"
#include "ofxOsc.h"

#define NUM_SPEAKERS 5

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

        ofxOscReceiver receiver;
        int speakers[NUM_SPEAKERS];
        float speakerDominance[NUM_SPEAKERS];
};
