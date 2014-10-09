#include "ofApp.h"

const int OSC_PORT = 12000;
const float ROTATE_FREQ = 0.3;
const float MIN_WEIGHT = 0.05;
const float ZERO_THRESH = 0.01;

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

void ofApp::makeEigs(float m[]) {
    // assume m[1] == m[2], for a valid covariance matrix. What if they're not?
    if(abs(m[1]/m[0]) < ZERO_THRESH && abs(m[2]/m[3]) < ZERO_THRESH) {
        // matrix is already diagonalized
        return;
    }

    float eigVal1, eigVal2;
    ofVec2f eigVec1, eigVec2;
    float left = (m[0] + m[3]) / 2;
    float right = sqrt((m[0]+m[3])*(m[0]+m[3]) - 4*(m[0]*m[3]-m[1]*m[2]))/2;
    eigVal1 = left - right;
    eigVal2 = left + right;
    if(abs(eigVal1 - eigVal2) < ZERO_THRESH) {
        eigVec1.x = eigVal1;
        eigVec1.y = 0;
        eigVec2.x = 0;
        eigVec2.y = eigVal2;
        return;
    }
    eigVec1.x = eigVal1 / sqrt(1+pow((m[0]-eigVal1)/m[1], 2));
    eigVec1.y = -(m[0]-eigVal1)/m[1] * eigVec1.x;
    eigVec2.x = eigVal2 / sqrt(1+pow((m[0]-eigVal2)/m[1], 2));
    eigVec2.y = -(m[0]-eigVal2)/m[1] * eigVec2.x;

    m[0] = eigVec1.x;
    m[1] = eigVec2.x;
    m[2] = eigVec1.y;
    m[3] = eigVec2.y;

    ofDrawBitmapString("eigVal1: " + ofToString(eigVal1), 200, -200);
    ofDrawBitmapString("eigVal2: " + ofToString(eigVal2), 200, -210);
}

ofVec2f ofApp::getPathPoint() {
    float totalWeight = 0;
    float weight[NUM_SPEAKERS];
    ofVec2f meanPos;
    // transform is a 2x2 transformation matrix in the shape of:
    //  0 1
    //  2 3
    float transform[4] = {0, 0, 0, 0};
    ofVec2f circlePoint = ofVec2f(cos(phase), sin(phase));
    ofVec2f transformed;

    // first compute the weighted mean point, used as the center of the ellipse
    for(int i = 0; i < NUM_SPEAKERS; ++i) {
        weight[i] = getSpeakerWeight(i);
        totalWeight += weight[i];
        meanPos += weight[i] * speakerPos[i];
    }
    meanPos /= totalWeight;

    // now get the spread transform, using the weighted covariance calculation
    for(int i = 0; i < NUM_SPEAKERS; ++i) {
        ofVec2f diffPos = speakerPos[i] - meanPos;
        // compute p * p' and sum up in the top-left of the transform matrix
        transform[0] += weight[i] * diffPos.x * diffPos.x;
        transform[1] += weight[i] * diffPos.x * diffPos.y;
        transform[2] += weight[i] * diffPos.x * diffPos.y;
        transform[3] += weight[i] * diffPos.y * diffPos.y;
    }
    makeEigs(transform);
    for(int i = 0; i < 4; ++i) {
        transform[i] /= sqrt(abs(transform[i]));
        //transform[i] /= totalWeight;
    }

    // apply the transform to a circle
    transformed.x = transform[0]*circlePoint.x + transform[1]*circlePoint.y;
    transformed.y = transform[2]*circlePoint.x + transform[3]*circlePoint.y;

    ofCircle(meanPos, 5);
    ofVec2f ax1 = ofVec2f(transform[0], transform[2]);
    ofVec2f ax2 = ofVec2f(transform[1], transform[3]);
    ofLine(meanPos, meanPos+ax1);
    ofLine(meanPos, meanPos+ax2);

    return transformed + meanPos;
    //return ofVec2f(0, 0);
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
