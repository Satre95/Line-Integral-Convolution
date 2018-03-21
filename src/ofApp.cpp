#include "ofApp.h"
#include "math_helper.hpp"

using namespace std::chrono;

//--------------------------------------------------------------
void ofApp::setup() {
	ofSetVerticalSync(true);
	ofEnableDepthTest();
	mCamera.setTarget(ofVec3f(10.f, 1.f, 0.f));
	mCamera.setDistance(80.f);
	mCamera.setNearClip(0.01f);
	mCamera.setFarClip(1000.f);

	lastFrameTime = high_resolution_clock::now();
}

//--------------------------------------------------------------
void ofApp::update() {
	auto now = high_resolution_clock::now();
	auto diff = now - lastFrameTime;
	double delta_t = duration_cast<microseconds>(diff).count() * 10e-6f;

	auto frameNum = ofGetFrameNum();
	mFluidRenderer.Update(delta_t, frameNum);
}

//--------------------------------------------------------------
void ofApp::draw() {
	ofBackground(0);
    mCamera.begin();
	{
		mFluidRenderer.Draw();
	}
    mCamera.end();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key) {
    mFluidRenderer.KeyPressed(key);
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key) {
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y) {
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button) {
}

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button) {
}

//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button) {
}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y) {
}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y) {
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h) {
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg) {
}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo) {
}
