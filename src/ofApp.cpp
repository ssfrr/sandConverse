#include "ofApp.h"

const int OSC_PORT = 12000;
const float ROTATE_FREQ = 1;
const float MIN_WEIGHT = 0.1;

//--------------------------------------------------------------
void ofApp::setup() {
    float speakerRadius = 200;

    for(int i = 0; i < NUM_SPEAKERS; ++i) {
        for(int j = 0; i < HISTORY_SIZE; ++i) {
            speakerSpeaking[i][j] = false;
        }
    }
    for(int i = 0; i < NUM_SPEAKERS; ++i) {
        float ang = i / (float)NUM_SPEAKERS * 2*PI;
        speakerPos[i].x = speakerRadius * cos(ang);
        speakerPos[i].y = speakerRadius * sin(ang);
        speakerSpeakingNow[i] = false;
    }
    historyIdx = 0;
    phase = 0;
    lastTime = ofGetElapsedTimef();
    cout << "Listening on port " << OSC_PORT << endl;
    receiver.setup(OSC_PORT);

    ofBackground(20, 20, 20);
}

float ofApp::getSpeakerWeight(int idx) {
    float avgWeight = 0;
    for(int i = 0; i < HISTORY_SIZE; ++i) {
        if(speakerSpeaking[idx][i]) {
            ++avgWeight;
        }
    }
    avgWeight /= HISTORY_SIZE;
    if(avgWeight < MIN_WEIGHT) {
        avgWeight = MIN_WEIGHT;
    }
    return avgWeight;
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
        if(nArgs != NUM_SPEAKERS) {
            cout << "Expected " << NUM_SPEAKERS << " args, got " << nArgs << ".\n";
            continue;
        }
        for(int i = 0; i < NUM_SPEAKERS; ++i) {
            if(msg.getArgType(i) != OFXOSC_TYPE_INT32) {
                cout << "Expected Int32 for argument " << i << ", got " << msg.getArgTypeName(i) << ".\n";
                continue;
            }
            speakerSpeakingNow[i] = msg.getArgAsInt32(i);
        }
    }
}

void ofApp::updateSpeakerSpeaking() {
    for(int i = 0; i < NUM_SPEAKERS; ++i) {
        speakerSpeaking[i][historyIdx] = speakerSpeakingNow[i];
    }
    historyIdx = (historyIdx + 1) % HISTORY_SIZE;
}

ofVec2f ofApp::getPathPoint() {
    float totalWeight = 0;
    ofVec2f meanPos = ofVec2f(0, 0);

    for(int i = 0; i < NUM_SPEAKERS; ++i) {
        float weight = getSpeakerWeight(i);
        totalWeight += weight;
        meanPos += weight * speakerPos[i];
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
    updateSpeakerSpeaking();
    phase += 2*PI*ROTATE_FREQ * (currentTime - lastTime);
    lastTime = currentTime;
}

void ofApp::DrawSpeakers() {
    ofSetColor(200, 200, 200);

    for(int i = 0; i < NUM_SPEAKERS; ++i) {
        float circSize = 30.0 * getSpeakerWeight(i);
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
