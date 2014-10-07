#include "ofApp.h"

#define OSC_PORT 12000
// HISTORY_GAIN of 1.0 will ignore input, 0.0 will immediately react
#define HISTORY_GAIN 0.99

//--------------------------------------------------------------
void ofApp::setup(){
    for(int i = 0; i < NUM_SPEAKERS; ++i) {
        speakers[i] = false;
        speakerDominance[i] = 0.0;
    }
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
        if(nArgs != NUM_SPEAKERS) {
            cout << "Expected " << NUM_SPEAKERS << " args, got " << nArgs << ".\n";
            continue;
        }
        for(int i = 0; i < NUM_SPEAKERS; ++i) {
            if(msg.getArgType(i) != OFXOSC_TYPE_INT32) {
                cout << "Expected Int32 for argument " << i << ", got " << msg.getArgTypeName(i) << ".\n";
                continue;
            }
            speakers[i] = msg.getArgAsInt32(i);
        }
    }
}

//--------------------------------------------------------------
void ofApp::update(){
    handleOscMsgs();
    for(int i = 0; i < NUM_SPEAKERS; ++i) {
        speakerDominance[i] = speakerDominance[i] * HISTORY_GAIN + speakers[i] * (1.0 - HISTORY_GAIN);
    }
}

//--------------------------------------------------------------
void ofApp::draw(){
    float rad = 200;
    float offsetX = ofGetWidth() / 2.0;
    float offsetY = ofGetHeight() / 2.0;
    ofSetColor(200, 200, 200);
    ofCircle(offsetX, offsetY, 10);

    for(int i = 0; i < NUM_SPEAKERS; ++i) {
        float circSize = 30.0 * speakerDominance[i];
        float ang = i / (float)NUM_SPEAKERS * 2*PI;
        ofCircle(rad * cos(ang)+offsetX, offsetY - rad*sin(ang), circSize);
    }
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
