#include "ofApp.h"
#include "math_helper.hpp"

//--------------------------------------------------------------
void ofApp::setup() {
	mCamera.setPosition(0, 100.f, 0.f);
	ofEnableDepthTest();
}

//--------------------------------------------------------------
void ofApp::update() {
	auto delta_t = ofGetElapsedTimef() - ofGetLastFrameTime();
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