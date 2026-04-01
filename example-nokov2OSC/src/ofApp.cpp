#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setupNokov() {
	// Connect to Nokov/XING server
	// Change this to your Nokov server IP address
	nokov.setup(serverAddress);
	nokov.setScale(100);
}

void ofApp::setup() {
	ofSetFrameRate(60);
	ofSetVerticalSync(true);
	ofBackground(0);

	serverAddress = "127.0.0.1";

	setupNokov();

	oscSender.setupDestinations({{"127.0.0.1", 11111}});
	ofSetCircleResolution(180);

	// set ortho cam
	cam.enableOrtho();
	cam.setScale(1.8);
	cam.setPosition(ofVec3f(-600, 0, 0));
	cam.setNearClip(1000000);
	cam.setFarClip(-1000000);
}

void ofApp::getRigidBodyInfoFromNokov() {
	// get rigidbody info
	nokov.update();
	for (int i = 0; i < nokov.getNumRigidBody(); i++) {
		const ofxNokov::RigidBody& RB = nokov.getRigidBodyAt(i);

		RigidBodyInfo rBody = RigidBodyInfo();
		rigidBodies.insert(pair<int, RigidBodyInfo>(RB.id, rBody));
	}
}

void ofApp::updateRigidBodyInformation() {
	cout << rigidBodies.size() << endl;
	for (int i = 0; i < nokov.getNumRigidBody(); i++) {
		const ofxNokov::RigidBody& RB = nokov.getRigidBodyAt(i);

		map<int, RigidBodyInfo>::iterator iter;
		iter = rigidBodies.find(RB.id);
		if (iter != rigidBodies.end()) {
			// active status
			iter->second.setActiveStatus(RB.isActive());
			// rigidbody name
			iter->second.updateName(RB.name);
			// matrix
			ofMatrix4x4 mat = RB.getMatrix();
			mat.rotate(180, 0, 1, 1);
			iter->second.updateMatrix(mat);
		}
	}
}

void ofApp::sendRigidBodyInformation() {
	for(auto &&rb : rigidBodies) {
		if(!rb.second.getActive()) {
			continue;
		}
		oscSender.send(rb.first, rb.second);
	}
}

//--------------------------------------------------------------
void ofApp::update(){
	getRigidBodyInfoFromNokov();
	updateRigidBodyInformation();
	sendRigidBodyInformation();

	if (nokov.needRequestDescription()) {
		nokov.sendRequestDescription();
	}

	// get number of RigidBody
	cout << "num of rigid body : " << nokov.getNumRigidBody() << endl;

	for (auto iter = rigidBodies.begin(); iter != rigidBodies.end(); iter++) {
		cout << iter->first << " : ";
		cout << iter->second.getActive() << endl;
	}
}

void ofApp::drawGridOneColor(float stepSize, size_t numberOfSteps, bool labels, bool x, bool y, bool z) {
	if (x) {
		ofDrawGridPlane(stepSize, numberOfSteps, labels);
	}
	if (y) {
		ofMatrix4x4 m;
		m.makeRotationMatrix(90, 0, 0, -1);
		ofPushMatrix();
		ofMultMatrix(m);
		ofDrawGridPlane(stepSize, numberOfSteps, labels);
		ofPopMatrix();
	}
	if (z) {
		ofMatrix4x4 m;
		m.makeRotationMatrix(90, 0, 1, 0);
		ofPushMatrix();
		ofMultMatrix(m);
		ofDrawGridPlane(stepSize, numberOfSteps, labels);
		ofPopMatrix();
	}

	if (labels) {
		float labelPos = stepSize * (numberOfSteps + 0.5);
		ofDrawBitmapString("x", labelPos, 0, 0);
		ofDrawBitmapString("y", 0, labelPos, 0);
		ofDrawBitmapString("z", 0, 0, labelPos);
	}
}

//--------------------------------------------------------------
void ofApp::draw(){
	ofEnableAlphaBlending();
	cam.begin();

	// draw world Axis
	ofDrawAxis(100);

	ofPushStyle();
	ofSetColor(ofColor::fromHex(0xFFFFFF), 20);
	drawGridOneColor(100, 100, true, true, true, true);
	ofPopStyle();

	// draw Boundary
	ofPushMatrix();
	ofPushStyle();
	ofSetColor(ofColor::fromHex(0xFFFF00));
	ofNoFill();
	ofSetLineWidth(3);
	ofRotateZ(0);
	ofDrawRectangle(-600, -600, 1200, 1200);
	ofPopStyle();
	ofPopMatrix();

	// draw rigidBodies
	for (auto iter = rigidBodies.begin(); iter != rigidBodies.end(); iter++) {
		if (iter->second.getActive()) {
			ofPushMatrix();
			{
				ofTranslate(iter->second.getPosition().x, iter->second.getPosition().y);
				ofSetColor(ofColor::cyan);

				ofFill();
				ofDrawCircle(0, 0, 10);

				ofSetColor(ofColor::fromHex(0xFFFF00));
				ofDrawBitmapString("[" + ofToString(iter->first) + "]" + iter->second.getName(), -30, 30);
			}
			ofPopMatrix();

			ofPushMatrix();
			{
				glMultMatrixf(iter->second.getMatrix().getPtr());
				ofPushStyle();
				{
					ofSetColor(255, 120);
					ofNoFill();
					ofDrawBox(3);

					ofDrawAxis(10);
				}
				ofPopStyle();
			}
			ofPopMatrix();
		}
	}

	cam.end();
	nokov.debugDrawInformation();
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){
	switch(key) {
		case OF_KEY_RETURN:
			setupNokov();
			break;
	}
}

//--------------------------------------------------------------
void ofApp::keyReleased(int key) {
	if (key == 'f' || key == 'F') {
		ofToggleFullscreen();
	}
}
//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y ){

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
void ofApp::mouseEntered(int x, int y){

}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){

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
