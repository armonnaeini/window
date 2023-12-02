// do shaders here
#ifndef Camera_h
#define Camera_h

#include "MetalCam.h"
#include "ofMain.h"
#include "ofxiOS.h"
#include "ARUtils.h"

#define STRINGIFY(A) #A

namespace ofxARKit {
    namespace core {
        
        typedef std::shared_ptr<class Camera> CameraRef;

        class Camera : public MetalCamRenderer{
            ofShader shader;
            ofMesh mesh;
            
            ofShader camShader;
            ofMesh camMesh;
            
            ofFbo trailFbo;
            ofFbo occlusionFbo;
            ofFbo cameraFbo;
            
            //MetalCamView * _view;
            //ARSession * session;
            CGRect viewport;
            
            UIInterfaceOrientation orientation;
            ofxARKit::common::ARCameraMatrices cameraMatrices;
            float near,far;
            
        public:
            //! The current tracking state of the camera
            ARTrackingState trackingState;
            //! The reason for when a tracking state might be limited.
            ARTrackingStateReason trackingStateReason;
            //! Flag for turning debug mode on/off
            bool debugMode;
            Camera(ARSession * session);
            static CameraRef create(ARSession * session){
                return CameraRef(new Camera(session));
            }
            
            CVOpenGLESTextureRef getTexture();
                        
            
        /******** MATTE STUFF ***********/
        #if defined( __IPHONE_13_0 )

            // returns a default matte shader.
            ofShader getDefaultMatteShader(){
                if(!this->session.configuration.frameSemantics == ARFrameSemanticPersonSegmentationWithDepth){
                    return ofShader();
                }
                
                ofShader shd;
                shd.setupShaderFromSource(GL_VERTEX_SHADER, vertexMatte);
                shd.setupShaderFromSource(GL_FRAGMENT_SHADER, fragmentMatte);

                shd.linkProgram();
                return shd;
            }
            
            /*! THIS IS WHERE YOU ARE WORKING!!! USE THIS TO INITIALIZE NEW SHADERS + MESH FOR CAMERA  !*/
            ofShader getNormalShader() {
                ofShader shd2;
                shd2.setupShaderFromSource(GL_VERTEX_SHADER, vertex);
                shd2.setupShaderFromSource(GL_FRAGMENT_SHADER, fragment);
            }

            CVOpenGLESTextureRef getTextureMatteAlpha();
            CVOpenGLESTextureRef getTextureMatteDepth();
            CVOpenGLESTextureRef getTextureDepth();
            ofMatrix3x3 getAffineTransform();

             //! returns vertex shader source suitable for doing things with person occlusion
            std::string getDefaultMatteVertexShader(){ return vertexMatte; }

                //! a helper to allow you to more easily integrate your own shader source with person occlusion.
                //! @param source the main portion of the shader you want to create
                //! @return chunk the final composed shader source.
            std::string composeMatteCompatibleFragmentShader(std::string source){
                std::string chunk = STRINGIFY(
                                                  
                    precision highp float;
                    varying vec2 vUv;
                    varying vec2 vUvCam;
                                             
                    uniform sampler2D tex;
                    uniform sampler2D texAlphaBody;
                    uniform sampler2D texDepthBody;
                    uniform sampler2D texDepth;
                                             
                    uniform mat4 u_CameraProjectionMat;
                                             
                    uniform float u_time;
                
                    // get the depth, texture and texture depth info from camera feed
                    void main(){
                    
                        vec4 sceneColor = texture2D(tex, vUv);
                        float sceneDepth = texture2D(texDepth, vUvCam).r;
                       
                        float alpha = texture2D( texAlphaBody, vUvCam).r;
                        float dilatedLinearDepth = texture2D(texDepthBody, vUvCam).r;

                        float dilatedDepth = clamp((u_CameraProjectionMat[2][2] * - dilatedLinearDepth + u_CameraProjectionMat[3][2]) / (u_CameraProjectionMat[2][3] * -dilatedLinearDepth + u_CameraProjectionMat[3][3]), 0.0, 1.0);
                                                        
                        float showOccluder = step(dilatedDepth, sceneDepth); // forwardZ case
                );
                        chunk += source;
                            chunk += "}";
                        return chunk;
                    }

              //! Renders the camera image along with a segmented person image.
            //! intended to be used for debugging purposes.
            void drawDebugPersonSegmentation();
            bool debugMatteShaderBuilt;
        #endif
            
            ARTrackingStateReason getTrackingState();
            
            //! returns the projection matrix
            glm::mat4 getProjectionMatrix();

            //! Returns the view matrix
            glm::mat4 getViewMatrix();

            //! Returns the transform matrix.
            glm::mat4 getTransformMatrix();
                             
            //! store trail positions + textures
            std::vector<std::pair<glm::mat4, ofTexture>> trailData;
            int maxTrailCount = 30;
            
            //! Returns the current set of camera matrices for the given device orientation.
            ofxARKit::common::ARCameraMatrices getMatricesForOrientation(UIInterfaceOrientation orientation,float near, float far);
            
            //! Returns the current set of camera matrices
            ofxARKit::common::ARCameraMatrices getCameraMatrices();

            //! Update the interface orientation of the device.
            void updateInterfaceOrientation(int newOrientation);

            //! Sets the current camera matrices as the current global camera projection and view matrices.
            void setARCameraMatrices();

            //! Logs the current tracking state of the camera.
            void logTrackingState();

            //! Updates the camera view
            //! @deprecated ?
            void update();

            //! Render the camera image
            void draw();
                        
            private:
               
                std::string vertex = STRINGIFY(
                    attribute vec2 position;
                    varying vec2 vUv;
                    const vec2 scale = vec2(0.5,0.5);
                    void main(){
                        vec2 uV = position.xy * scale + scale;
                        vUv = vec2(uV.s, 1.0 - uV.t);
                        gl_Position = vec4(position,0.0,1.0);
                });
                            
                std::string fragment = STRINGIFY(
                    precision highp float;
                    varying vec2 vUv;
                                                             
                    uniform sampler2D tex;
                    void main(){
                        gl_FragColor = texture2D(tex, vUv);
                    }
                );
                                              
                /** SHADERS GO HERE  **/
                #if defined( __IPHONE_13_0 )

                /**VERTEX SHADER **/
                std::string vertexMatte = STRINGIFY(
                    attribute vec2 position;

                    uniform vec4 cAffineCamABCD;
                    uniform vec2 cAffineCamTxTy;
               
                    varying vec2 vUv;
                    varying vec2 vUvCam;

                    // https://developer.apple.com/documentation/coregraphics/cgaffinetransform
                    vec2 affineTransform(vec2 uv, vec4 coeff, vec2 offset){
                    return vec2(uv.s * coeff.x + uv.t * coeff.z + offset.x,
                        uv.s * coeff.y + uv.t * coeff.w + offset.y);
                    }
                                                        
                    const vec2 scale = vec2(0.5,0.5);
                                                        
                    void main(){
                        vec2 uV = position.xy * scale + scale;
                        vUv = vec2(uV.s, 1.0 - uV.t);
                        vUvCam = affineTransform(vUv, cAffineCamABCD, cAffineCamTxTy);
                        gl_Position = vec4(position,0.0,1.0);
                });
                
                                              
                /**FRAGMENT SHADER**/
                std::string fragmentMatte = STRINGIFY(
                    
                    precision highp float;
                    varying vec2 vUv;
                    varying vec2 vUvCam;
         
                    uniform sampler2D tex;
                    uniform sampler2D texAlphaBody;
                    uniform sampler2D texDepthBody;
                    uniform sampler2D texDepth;
                                                            
                    uniform mat4 u_CameraProjectionMat;
                 
                uniform float u_time;
                const float fluid_speed = 2.25;
                                                      
                    void main(){
                        vec4 sceneColor = texture2D(tex, vUv);
                        float sceneDepth = texture2D(texDepth, vUvCam).r;


                        float alpha = texture2D( texAlphaBody, vUvCam).r;
                        float dilatedLinearDepth = texture2D(texDepthBody, vUvCam).r;

                        float dilatedDepth = clamp((u_CameraProjectionMat[2][2] * - dilatedLinearDepth + u_CameraProjectionMat[3][2]) / (u_CameraProjectionMat[2][3] * -dilatedLinearDepth + u_CameraProjectionMat[3][3]), 0.0, 1.0);

                
                        float showOccluder = step(dilatedDepth, sceneDepth); // forwardZ case

                        // camera Color is a sine of the actual color * time
                        vec4 cameraColor = vec4(sceneColor.r + abs(sin(u_time)), sceneColor.g + abs(cos(u_time)), sceneColor.b, sceneColor.a) * dilatedDepth;

                        
                        // liquid glow
                        vec2 p = (sceneColor.xy);
                        
                        for (int i = 1; i < 12; i++){
                            vec2 newp = p + u_time*.01;
                            newp.x+=2.1/float(i)*sin(float(i)*p.y+u_time/fluid_speed+0.3*float(i)) + .04;
                            newp.y+=1.4/float(i)*sin(float(i)*p.x+u_time/fluid_speed+0.3*float(i+10)) - 0.05;
                            p=newp;
                        }
                        
                        vec4 sceneColor2 = vec4(.7*sin(3.0*p.x) + .7, .7*sin(3.0*p.y)+ .7, .7*sin(p.x+p.y)+.7, sceneColor.a);
                        vec4 sceneColor3 = vec4(sceneColor2.r, sceneColor2.g, sceneColor2.b, sceneColor.a) * dilatedDepth;
                        
                        
                        // DEFAULT //
                        vec4 occluderResult = mix(sceneColor, cameraColor, alpha);
                        vec4 occluderResult2 = mix(sceneColor, sceneColor2, alpha);
                        
                        vec4 mattingResult = mix(sceneColor, occluderResult, showOccluder);
                        vec4 mattingResult2 = mix(sceneColor2, occluderResult2, showOccluder);

        
                        // OUTPUT //
                        gl_FragColor = occluderResult2;
    
                    }
                );
                                                      
            
                #endif
                    
    };
}}

#endif /* Camera_h */
