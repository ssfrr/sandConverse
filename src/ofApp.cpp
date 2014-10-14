#include "ofApp.h"

const int OSC_PORT = 12000;
const float ROTATE_FREQ = 0.2;
const float MIN_WEIGHT = 0.02;
const float ZERO_THRESH = 0.01;

//--------------------------------------------------------------
void ofApp::setup() {
    float speakerRadius = 200;

    ofEnableSmoothing();
    lastEllipseAngle = 0;
    lastBallPos = ofVec2f(10000, 10000);
    lastBallPhase = 0;
    ballFbo.allocate(ofGetWidth(), ofGetHeight(), GL_RGBA);
    ballFbo.begin();
    ofClear(0, 0, 0, 0);
    ballFbo.end();
    for(int i = 0; i < NUM_SPEAKERS; ++i) {
        for(int j = 0; i < HISTORY_SIZE; ++i) {
            speakerSpeaking[i][j] = false;
        }
    }
    for(int i = 0; i < NUM_SPEAKERS; ++i) {
        float ang = i / (float)NUM_SPEAKERS * 2*PI + PI/4;
        speakerPos[i].x = speakerRadius * cos(ang);
        speakerPos[i].y = speakerRadius * sin(ang);
        speakerSpeakingNow[i] = false;
    }
    historyIdx = 0;
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
    avgWeight = avgWeight * avgWeight * (1 - MIN_WEIGHT) + MIN_WEIGHT;
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

void ofApp::getEllipse(float m[], float *a, float *b, float *angle) {
    // assume m[1] == m[2], for a valid covariance matrix. What if they're not?
    if(abs(m[1]/m[0]) < ZERO_THRESH && abs(m[2]/m[3]) < ZERO_THRESH) {
        // matrix is already diagonalized
        *angle = 0;
        *a = sqrt(abs(m[0]));
        *b = sqrt(abs(m[3]));
        return;
    }

    float right = sqrt((m[0]+m[3])*(m[0]+m[3]) - 4*(m[0]*m[3]-m[1]*m[2]))/2;
    float left = (m[0] + m[3]) / 2;
    if(right < ZERO_THRESH) {
        // ellipse is a circle
        *a = sqrt(left);
        *b = *a;
        angle = 0;
        return;
    }
    float eigVal1 = left - right;
    float eigVal2 = left + right;
    ofVec2f eigVec1, eigVec2;
    eigVec1.x = eigVal1 / sqrt(1+pow((m[0]-eigVal1)/m[1], 2));
    eigVec1.y = -(m[0]-eigVal1)/m[1] * eigVec1.x;
    eigVec2.x = eigVal2 / sqrt(1+pow((m[0]-eigVal2)/m[1], 2));
    eigVec2.y = -(m[0]-eigVal2)/m[1] * eigVec2.x;

    *a = sqrt(abs(eigVal1));
    *b = sqrt(abs(eigVal2));
    *angle = atan2(eigVec1.y, eigVec1.x);
}

void ofApp::makeEigs(float m[]) {
    // assume m[1] == m[2], for a valid covariance matrix. What if they're not?
    if(abs(m[1]/m[0]) < ZERO_THRESH && abs(m[2]/m[3]) < ZERO_THRESH) {
        // matrix is already diagonalized, just
        m[0] = sqrt(m[0]);
        m[1] = 0;
        m[2] = 0;
        m[3] = sqrt(m[3]);
        return;
    }

    float eigVal1, eigVal2;
    ofVec2f eigVec1, eigVec2;
    float left = (m[0] + m[3]) / 2;
    float right = sqrt((m[0]+m[3])*(m[0]+m[3]) - 4*(m[0]*m[3]-m[1]*m[2]))/2;
    eigVal1 = left - right;
    eigVal2 = left + right;
    if(abs(eigVal1 - eigVal2) < ZERO_THRESH) {
        m[0] = sqrt(eigVal1);
        m[1] = 0;
        m[2] = 0;
        m[3] = m[0];
        return;
    }
    eigVec1.x = eigVal1 / sqrt(1+pow((m[0]-eigVal1)/m[1], 2));
    eigVec1.y = -(m[0]-eigVal1)/m[1] * eigVec1.x;
    eigVec2.x = eigVal2 / sqrt(1+pow((m[0]-eigVal2)/m[1], 2));
    eigVec2.y = -(m[0]-eigVal2)/m[1] * eigVec2.x;

    m[0] = eigVec1.x / sqrt(eigVal1);
    m[1] = eigVec2.x / sqrt(eigVal2);
    m[2] = eigVec1.y / sqrt(eigVal1);
    m[3] = eigVec2.y / sqrt(eigVal2);

}

void ofApp::updateBallPos() {
    float currentTime = ofGetElapsedTimef();
    float totalWeight = 0;
    float weight[NUM_SPEAKERS];
    lastBallPos = ballPos;

    meanPos = ofVec2f(0, 0);
    // first compute the weighted mean point, used as the center of the ellipse
    for(int i = 0; i < NUM_SPEAKERS; ++i) {
        weight[i] = getSpeakerWeight(i);
        totalWeight += weight[i];
        meanPos += weight[i] * speakerPos[i];
    }
    meanPos /= totalWeight;

    for(int i = 0; i < 4; ++i) {
        transform[i] = 0;
    }
    // now get the spread transform, using the weighted covariance calculation
    for(int i = 0; i < NUM_SPEAKERS; ++i) {
        ofVec2f diffPos = speakerPos[i] - meanPos;
        // compute p * p' and sum up in the top-left of the transform matrix
        transform[0] += weight[i] * diffPos.x * diffPos.x;
        transform[1] += weight[i] * diffPos.x * diffPos.y;
        transform[2] += weight[i] * diffPos.x * diffPos.y;
        transform[3] += weight[i] * diffPos.y * diffPos.y;
    }
    getEllipse(transform, &ellipseA, &ellipseB, &ellipseAngle);
    float nextPhase = lastBallPhase - (ellipseAngle - lastEllipseAngle) + 2*PI*ROTATE_FREQ * (currentTime - lastTime);
    while(nextPhase > PI) {
        nextPhase -= 2*PI;
    }
    while(nextPhase < -PI) {
        nextPhase += 2*PI;
    }

    ballPos.x = ellipseA*cos(-ellipseAngle)*cos(nextPhase) + ellipseB * sin(-ellipseAngle) * sin(nextPhase);
    ballPos.y = -ellipseA*sin(-ellipseAngle)*cos(nextPhase) + ellipseB * cos(-ellipseAngle) * sin(nextPhase);

    ballPos += meanPos;
    lastBallPhase = nextPhase;
    lastEllipseAngle = ellipseAngle;

    lastTime = currentTime;
}

//--------------------------------------------------------------
void ofApp::update() {
    handleOscMsgs();
    updateSpeakerSpeaking();
    updateBallPos();
}

void ofApp::DrawSpeakers() {
    ofSetColor(200, 200, 200, 100);

    for(int i = 0; i < NUM_SPEAKERS; ++i) {
        float circSize = 30.0 * getSpeakerWeight(i);
        ofCircle(speakerPos[i], circSize);
    }
}

//--------------------------------------------------------------
void ofApp::draw(){
    ballFbo.begin();
    ofPushMatrix();
    ofTranslate(ofGetWidth() / 2, ofGetHeight() / 2);
    ofScale(1, -1);
    if((ballPos - lastBallPos).lengthSquared() < 500) {
        ofPushStyle();
        ofSetColor(20, 20, 20, 20);
        for(int w = 10; w >= 2; w--) {
            ofSetLineWidth(w);
            ofLine(lastBallPos, ballPos);
        }
        ofSetColor(255, 255, 255, 255);
        ofSetLineWidth(2);
        ofLine(lastBallPos, ballPos);
        ofCircle(lastBallPos, 1);
        //ofSetColor(237, 176, 135, 255);
        ofPopStyle();
    }
    ballFbo.end();
    ofPopMatrix();

    ballFbo.draw(0, 0);

    ofPushMatrix();
    ofTranslate(ofGetWidth() / 2, ofGetHeight() / 2);
    ofScale(1, -1);

    /*
    DrawSpeakers();

    ofCircle(meanPos, 5);
    ofVec2f ax1 = ellipseA * ofVec2f(cos(ellipseAngle), sin(ellipseAngle));
    ofVec2f ax2 = ellipseB * ofVec2f(cos(ellipseAngle-PI/2), sin(ellipseAngle-PI/2));
    ofLine(meanPos, meanPos+ax1);
    ofLine(meanPos, meanPos+ax2);

    ofDrawBitmapString("meanPos: " + ofToString(meanPos), 300, -200);
    ofDrawBitmapString("a: " + ofToString(ellipseA), 300, -210);
    ofDrawBitmapString("b: " + ofToString(ellipseB), 300, -220);
    ofDrawBitmapString("angle: " + ofToString(ellipseAngle), 300, -230);
    */

    // draw the ball
    ofSetColor(255, 255, 255, 255);
    ofCircle(ballPos, 4);

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
