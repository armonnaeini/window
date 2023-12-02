#pragma once

#include "ofxiOS.h"
#include <ARKit/ARKit.h>
#include "ofxARKit.h"
class ofApp : public ofxiOSApp {
    
public:
    
    ofApp (ARSession * session);
    ofApp();
    ~ofApp ();
    
    void setup();
    void update();
    void draw();
    void exit();
    
    void touchDown(ofTouchEventArgs &touch);
    void deviceOrientationChanged(int newOrientation);
    
    vector < matrix_float4x4 > mats;
    vector<ARAnchor*> anchors;
    ofCamera camera;
    ofTrueTypeFont font;
    ofImage img;

    // ====== AR STUFF ======== //
    ARSession * session;
    UIDevice * device;
    ARRef processor;
    
    ofShader meshShader;
    
    std::vector<glm::mat4> trailData;
    
    // Add a new member variable to control the maximum number of trails
    int maxTrailCount = 90;
    ofFbo bodyFbo;
    bool fboAllocated = false;
    
    //create seperate mesh + shader for default camera
    ofMesh cameraMesh;
    ofShader cameraMeshShader;
    
};


