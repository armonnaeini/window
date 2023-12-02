#include "ofApp.h"

using namespace ofxARKit::common;

//--------------------------------------------------------------
ofApp :: ofApp (ARSession * session){
    
     ARFaceTrackingConfiguration *configuration = [ARFaceTrackingConfiguration new];
     [session runWithConfiguration:configuration];
    
    this->session = session;
    cout << "creating ofApp" << endl;
}

ofApp::ofApp(){}

//--------------------------------------------------------------
ofApp :: ~ofApp () {
    cout << "destroying ofApp" << endl;
}

//--------------------------------------------------------------
void ofApp::setup() {
    ofClear(0,0,0,0);    
    int fontSize = 8;
    if (ofxiOSGetOFWindow()->isRetinaSupportedOnDevice())
        fontSize *= 2;
    font.load("fonts/mono0755.ttf", fontSize);
    processor = ARProcessor::create(session);
    processor->setup();
    
}

//--------------------------------------------------------------
void ofApp::update(){
    /*allocate FBO*/
    if (!fboAllocated) {
        bodyFbo.allocate(ofGetWidth(), ofGetHeight(), GL_RGBA);
        fboAllocated = true;
    }
    processor->update();
}

//--------------------------------------------------------------
void ofApp::draw() {
    ofClear(0,0,0,0);

    processor->setARCameraMatrices();
   
    /* DRAW DEFAULT CAMERA VIEW */
     bodyFbo.begin();
         ofClear(0,0,0,0);
         ofDisableDepthTest();
         processor->drawCameraDebugPersonSegmentation();
         ofEnableDepthTest();
     bodyFbo.end();
     
    /*image anchoring right here*/
    if (session.currentFrame) {
        if (session.currentFrame.camera) {
            camera.begin();
            processor->setARCameraMatrices();
            
            //here we iterate through all the anchors that we placed in our
            //touchDown function
            for (int i = 0; i < session.currentFrame.anchors.count; i++){
                ARAnchor * anchor = session.currentFrame.anchors[i];
                
                ofPushMatrix();
                ofMatrix4x4 mat = convert<matrix_float4x4, ofMatrix4x4>(anchor.transform);
                ofMultMatrix(mat);
                
                ofRotate(-90,0,0,1);
                ofSetColor(255);
                ofScale(-1, 1, 1);
                
                bodyFbo.draw(-0.25 / 2, -0.25 / 2, .25, .5);

                ofPopMatrix();
            }
            camera.end();
        }
    }
    ofEnableDepthTest();
    ofDisableDepthTest();
}

//--------------------------------------------------------------
void ofApp::exit() {
}

//--------------------------------------------------------------
void ofApp::touchDown(ofTouchEventArgs &touch){
    
    if (session.currentFrame){
        ARFrame *currentFrame = [session currentFrame];

        matrix_float4x4 translation = matrix_identity_float4x4;
        translation.columns[3].z = -0.2;
        matrix_float4x4 transform = matrix_multiply(currentFrame.camera.transform, translation);

        // Add a new anchor to the session
        ARAnchor *anchor = [[ARAnchor alloc] initWithTransform:transform];

        [session addAnchor:anchor];
    }
}


//--------------------------------------------------------------
void ofApp::deviceOrientationChanged(int newOrientation){
    processor->deviceOrientationChanged(newOrientation);
}



