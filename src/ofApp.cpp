#include "ofApp.h"

const int OSC_PORT = 12000;
// HISTORY_GAIN of 1.0 will ignore input, 0.0 will immediately react
const float HISTORY_GAIN = 0.99;
const float ROTATE_FREQ = 1;
const float CENTER_WEIGHT = 0.1;

//--------------------------------------------------------------
void ofApp::setup() {
    float speakerRadius = 200;

    speakerPos[0] = ofVec2f(0, 0);
    speakerWeight[0] = CENTER_WEIGHT;
    for(int i = 1; i < NUM_SPEAKERS; ++i) {
        float ang = (i-1) / (float)(NUM_SPEAKERS-1) * 2*PI;
        speakerSpeaking[i] = false;
        speakerWeight[i] = 0.0;
        speakerPos[i].x = speakerRadius * cos(ang);
        speakerPos[i].y = speakerRadius * sin(ang);
    }
    phase = 0;
    lastTime = ofGetElapsedTimef();
    cout << "Listening on port " << OSC_PORT << endl;
    receiver.setup(OSC_PORT);

    ofBackground(20, 20, 20);
}

void ofApp::handleOscMsgs() {
    while(receiver.hasWaitingMessages()) {
        ofxOscMessage msg;
        receiver.getNextMessage(&msg);
        string addr = msg.getAddress();
        if(addr != "/speaking") {
            cout << "Unrecognized address \"" << addr << "\". use \"/speaking\".\n";
            continue;
        }
        int nArgs = msg.getNumArgs();
        if(nArgs != NUM_REAL_SPEAKERS) {
            cout << "Expected " << NUM_REAL_SPEAKERS << " args, got " << nArgs << ".\n";
            continue;
        }
        for(int i = 0; i < NUM_REAL_SPEAKERS; ++i) {
            if(msg.getArgType(i) != OFXOSC_TYPE_INT32) {
                cout << "Expected Int32 for argument " << i << ", got " << msg.getArgTypeName(i) << ".\n";
                continue;
            }
            speakerSpeaking[i+1] = msg.getArgAsInt32(i);
        }
    }
}

void ofApp::updateWeights() {
    for(int i = 1; i < NUM_SPEAKERS; ++i) {
        speakerWeight[i] = speakerWeight[i] * HISTORY_GAIN + speakerSpeaking[i] * (1.0 - HISTORY_GAIN);
    }
}

ofVec2f ofApp::getPathPoint() {
    float totalWeight = 0;
    ofVec2f meanPos = ofVec2f(0, 0);

    for(int i = 0; i < NUM_SPEAKERS; ++i) {
        totalWeight += speakerWeight[i];
        meanPos += speakerWeight[i] * speakerPos[i];
    }
    meanPos /= totalWeight;

    meanPos.x += 30 * cos(phase);
    meanPos.y += 30 * sin(phase);

    return meanPos;
}

//--------------------------------------------------------------
void ofApp::update() {
    float currentTime = ofGetElapsedTimef();
    handleOscMsgs();
    updateWeights();
    phase += 2*PI*ROTATE_FREQ * (currentTime - lastTime);
    lastTime = currentTime;
}

void ofApp::DrawSpeakers() {
    ofSetColor(200, 200, 200);

    for(int i = 0; i < NUM_SPEAKERS; ++i) {
        float circSize = 30.0 * speakerWeight[i];
        ofCircle(speakerPos[i], circSize);
    }
}

//--------------------------------------------------------------
void ofApp::draw(){
    ofPushMatrix();
    ofTranslate(ofGetWidth() / 2, ofGetHeight() / 2);
    ofScale(1, -1);
    DrawSpeakers();
    ofVec2f pathPoint = getPathPoint();
    ofCircle(pathPoint, 10);
    ofPopMatrix();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

}

//--------------------------------------------------------------
void ofApp::keyReleased(int key){

}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){

}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){

}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){ 

}
