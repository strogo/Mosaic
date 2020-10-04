/*==============================================================================

    Mosaic: Live Visual Patching Creative-Coding Platform

    Copyright (c) 2018 Emanuele Mazza aka n3m3da <emanuelemazza@d3cod3.org>

    Mosaic is distributed under the MIT License. This gives everyone the
    freedoms to use Mosaic in any context: commercial or non-commercial,
    public or private, open or closed source.

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the "Software"),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included
    in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
    OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.

    See https://github.com/d3cod3/Mosaic for documentation

==============================================================================*/

#include "ofApp.h"

//--------------------------------------------------------------
void ofApp::setup(){

    ///////////////////////////////////////////
    // OF Stuff
    ofSetEscapeQuitsApp(false);
    ofSetVerticalSync(true);
    ofEnableAntiAliasing();
    ofSetLogLevel(PACKAGE,OF_LOG_NOTICE);
    ofRegisterURLNotification(this);

    initDataFolderFromBundle();
    ///////////////////////////////////////////

    // TIMING
    mosaicFPS = 60;
    mosaicTiming.setFramerate(mosaicFPS);
    mosaicBPM = 120;

    // RETINA FIX
    isRetina = false;

    // LOGGER
    isInited        = false;
    isWindowResized = false;
    isLoggerON      = false;
    mosaicLoggerChannel = shared_ptr<MosaicLoggerChannel>(new MosaicLoggerChannel());
    ofSetLoggerChannel(mosaicLoggerChannel);

    ofLog(OF_LOG_NOTICE,"%s | %s <%s>",WINDOW_TITLE,DESCRIPTION,MOSAIC_WWW);
    ofLog(OF_LOG_NOTICE," an open project by Emanuele Mazza aka n3m3da");
    ofLog(OF_LOG_NOTICE,"Developers: %s",MOSAIC_DEVELOPERS);
    ofLog(OF_LOG_NOTICE,"This project deals with the idea of integrate/amplify human-machine communication, offering a real-time flowchart based visual interface for high level creative coding.\nAs live-coding scripting languages offer a high level coding environment, ofxVisualProgramming and the Mosaic Project as his parent layer container,\naim at a high level visual-programming environment, with embedded multi scripting languages availability (Processing/Java, Lua, Python, GLSL and BASH).\n");

    // Visual Programming Environment Load

    // ImGui
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.IniFilename = nullptr;

    // double font oversampling (default 3) for canvas zoom
    ImFontConfig font_config;
    font_config.OversampleH = 6;
    font_config.OversampleV = 6;

    ofFile fileToRead1(ofToDataPath(MAIN_FONT));
    string absPath1 = fileToRead1.getAbsolutePath();
    ofFile fileToRead2(ofToDataPath(LIVECODING_FONT));
    string absPath2 = fileToRead2.getAbsolutePath();
    if(ofGetScreenWidth() >= RETINA_MIN_WIDTH && ofGetScreenHeight() >= RETINA_MIN_HEIGHT){
        io.Fonts->AddFontFromFileTTF(absPath2.c_str(),30.0f,&font_config); // code editor font
        io.Fonts->AddFontFromFileTTF(absPath1.c_str(),26.0f,&font_config); // GUI font
    }else{
        io.Fonts->AddFontFromFileTTF(absPath2.c_str(),18.0f,&font_config); // code editor font
        io.Fonts->AddFontFromFileTTF(absPath1.c_str(),14.0f,&font_config); // GUI font
    }

    // merge in icons from Font Awesome
    static const ImWchar icons_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
    ImFontConfig icons_config; icons_config.MergeMode = true; icons_config.PixelSnapH = true;
    if(ofGetScreenWidth() >= RETINA_MIN_WIDTH && ofGetScreenHeight() >= RETINA_MIN_HEIGHT){
        io.Fonts->AddFontFromFileTTF( FONT_ICON_FILE_NAME_FAS, 24.0f, &icons_config, icons_ranges );
    }else{
        io.Fonts->AddFontFromFileTTF( FONT_ICON_FILE_NAME_FAS, 16.0f, &icons_config, icons_ranges );
    }

    ImFont* defaultfont = io.Fonts->Fonts[io.Fonts->Fonts.Size - 1];
    io.FontDefault = defaultfont;

    mainTheme = new MosaicTheme();
    mainMenu.setup(mainTheme,false);

    visualProgramming   = new ofxVisualProgramming();
    visualProgramming->setup( &mainMenu );
    visualProgramming->canvasViewport.set(glm::vec2(0,20*visualProgramming->scaleFactor), glm::vec2(ofGetWidth(), ofGetHeight()-(20*visualProgramming->scaleFactor)));

    patchToLoad                 = "";
    loadNewPatch                = false;
    isAutoloadedPatch           = false;

    // GUI
    mosaicLogo = new ofImage("images/logo_1024_bw.png");
    editorFullscreenButtonSource.load("images/fullscreen-icon.png");
    editorFullscreenButtonID = mainMenu.loadImage(editorFullscreenButtonSource);
    mosaicLogoID = mainMenu.loadImage(*mosaicLogo);

    showRightClickMenu      = false;
    createSearchedObject    = false;
    showConsoleWindow       = false;
    showAboutWindow         = false;
    isHoverMenu             = false;
    isHoverLogger           = false;
    isHoverCodeEditor       = false;

    // VIDEO EXPORTER ( documenting patches, tutorials, etc...)
    recordFilepath = "";
    exportVideoFlag = false;
    codecsList = {"hevc","libx264","jpeg2000","mjpeg","mpeg4"};
    selectedCodec = 4;
    recButtonLabel = "REC";

    // PATCH WINDOW ( MAIN ) VIDEO EXPORTER
    actualSubtitle          = "";
    showSubtitler           = false;

    showingClickAnimation   = false;
    mouseClickRadius        = 0.0f;
    showMouseOnRec          = false;

    captureFbo.allocate( ofGetWindowWidth(), ofGetWindowHeight(), GL_RGB );
    recorder.setup(true, false, glm::vec2(ofGetWindowWidth(), ofGetWindowHeight())); // record video only
    recorder.setOverWrite(true);

#if defined(TARGET_OSX)
    recorder.setFFmpegPath(ofToDataPath("ffmpeg/osx/ffmpeg",true));
#elif defined(TARGET_WIN32)
    recorder.setFFmpegPath(ofToDataPath("ffmpeg/win/ffmpeg.exe",true));
#endif

    // CODE EDITOR
    luaLang = TextEditor::LanguageDefinition::Lua();
    glslLang = TextEditor::LanguageDefinition::GLSL();
    pythonLang= TextEditor::LanguageDefinition::Python();
    bashLang = TextEditor::LanguageDefinition::Bash();

    initScriptLanguages();

    actualCodeEditor                = 0;
    actualEditedFilePath            = "";
    actualEditedFileName            = "";
    scriptToRemoveFromCodeEditor    = "";
    isCodeEditorON = false;
    isCodeEditorFullWindow = false;

#ifdef TARGET_LINUX
    shortcutFunc = "CTRL";
#elif defined(TARGET_OSX)
    shortcutFunc = "CMD";
#elif defined(TARGET_WIN32)
    shortcutFunc = "CTRL";
#endif

    // NET
    isInternetAvailable = false;
    isCheckingRelease = false;

    // Check for updates
    lastRelease = VERSION;

    isInternetAvailable = checkInternetReachability();

    saveNewScreenshot       = false;
    lastScreenshot          = "";
    waitForScreenshotTime   = 200;
    resetScreenshotTime     = ofGetElapsedTimeMillis();


    // AUTOLOAD PATCH ( if configured )
    checkAutoloadConfig();

}

//--------------------------------------------------------------
void ofApp::update(){

    windowTitle = visualProgramming->currentPatchFile+" - "+WINDOW_TITLE;
    ofSetWindowTitle(windowTitle);

    // Visual Programming Environment
    if(mosaicTiming.tick()){
        visualProgramming->update();
        visualProgramming->canvasViewport.set(glm::vec2(0,20*visualProgramming->scaleFactor), glm::vec2(ofGetWidth(), ofGetHeight()-(20*visualProgramming->scaleFactor)));
    }

    visualProgramming->setIsHoverMenu(isHoverMenu);
    visualProgramming->setIsHoverLogger(isHoverLogger);
    visualProgramming->setIsHoverCodeEditor(isHoverCodeEditor);

    // init start empty patch
    if(loadNewPatch){
        loadNewPatch = false;
        if(patchToLoad != ""){
            visualProgramming->preloadPatch(patchToLoad);
            mosaicBPM = visualProgramming->bpm;
        }
    }

    // autoload patch on startup
    if(autoloadPatchFile != "" && isAutoloadedPatch){
        if(ofGetElapsedTimeMillis() - autoloadStartTime > waitForAutoload){
            isAutoloadedPatch = false;
            visualProgramming->preloadPatch(autoloadPatchFile);
            mosaicBPM = visualProgramming->bpm;
        }
    }

    // load new script files in code editor
    if(visualProgramming->scriptsObjectsFilesPaths.size() > 0){
        for(map<string,string>::iterator it = visualProgramming->scriptsObjectsFilesPaths.begin(); it != visualProgramming->scriptsObjectsFilesPaths.end(); it++ ){
            ofFile tempsofp(it->second);

            map<string,TextEditor>::iterator sofpIT = codeEditors.find(tempsofp.getFileName());
            if (sofpIT == codeEditors.end()){
                // script not found in code editor, insert it
                initNewCodeEditor(tempsofp);
            }
        }
    }

    // remove deleted script files from code editor
    if(codeEditors.size() > 0){
        for(map<string,TextEditor>::iterator it = codeEditors.begin(); it != codeEditors.end(); it++ ){
            map<string,string>::iterator sofpVP = visualProgramming->scriptsObjectsFilesPaths.find(it->first);
            if (sofpVP == visualProgramming->scriptsObjectsFilesPaths.end()){
                // script not found in main map, remove it from code editor
                scriptToRemoveFromCodeEditor = it->first;
            }
        }
    }

    // listen for editing scripts external changes
    if(codeWatchers.size() > 0){
        for(map<string,PathWatcher*>::iterator it = codeWatchers.begin(); it != codeWatchers.end(); it++ ){
            while(it->second->waitingEvents()) {
                pathChanged(it->second->nextEvent());
            }
        }
    }

    if(scriptToRemoveFromCodeEditor != ""){
       removeScriptFromCodeEditor(scriptToRemoveFromCodeEditor);
    }

    if(isWindowResized){
        isWindowResized = false;
        visualProgramming->updateCanvasViewport();
    }

    if(!isInited){
        isInited = true;

        // RETINA FIX
        if(ofGetScreenWidth() >= RETINA_MIN_WIDTH && ofGetScreenHeight() >= RETINA_MIN_HEIGHT){ // RETINA SCREEN
            ofSetWindowShape(ofGetScreenWidth()-8,ofGetScreenHeight());
            isRetina = true;
        }else if(ofGetScreenWidth() >= 1920){ // DUAL HEAD, TRIPLE HEAD
            ofSetWindowShape(1920-4,ofGetScreenHeight());
        }else{ // STANDARD SCREEN
            ofSetWindowShape(ofGetScreenWidth()-4,ofGetScreenHeight());
        }

        if(isRetina){
            mainTheme->fixForRetinaScreen();
        }
        visualProgramming->setRetina(isRetina);
        fileDialog.setIsRetina(isRetina);

        // init gui positions
        initGuiPositions();
    }

    // NET
    if(isInternetAvailable && !isCheckingRelease){
        isCheckingRelease = true;
        lastReleaseResp = ofLoadURLAsync("https://raw.githubusercontent.com/d3cod3/Mosaic/master/RELEASE.md","check_release_async");
    }

    // Screenshot
    if(saveNewScreenshot){
        if(ofGetElapsedTimeMillis()-resetScreenshotTime > waitForScreenshotTime){ // avoid imgui filebrowser
            saveNewScreenshot = false;
            if(lastScreenshot != ""){
                ofFile file(lastScreenshot);
                // force .jpg file extension
                string finalPath = file.getAbsolutePath();
                if(ofToUpper(file.getExtension()) != "JPG"){
                    finalPath += ".jpg";
                }
                ofImage tempScreenshot;
                tempScreenshot.grabScreen(ofGetWindowRect().x,ofGetWindowRect().y,ofGetWindowWidth(),ofGetWindowHeight());
                tempScreenshot.getPixels().swapRgb();
                tempScreenshot.save(finalPath);
            }
        }
    }

    // Video Recording
    if(recorder.isRecording()) {
        static ofImage recordFrame;
        recordFrame.grabScreen(ofGetWindowRect().x,ofGetWindowRect().y,ofGetWindowWidth(),ofGetWindowHeight());

        captureFbo.begin();
        ofClear(0,0,0,255);
        ofSetColor(255);
        recordFrame.draw(0,0,ofGetWindowWidth(),ofGetWindowHeight());
        captureFbo.end();

        reader.readToPixels(captureFbo, capturePix,OF_IMAGE_COLOR); // ofxFastFboReader
        if(capturePix.getWidth() > 0 && capturePix.getHeight() > 0) {
            recorder.addFrame(capturePix);
        }
    }

}

//--------------------------------------------------------------
void ofApp::draw(){

    ofBackground(20);
    ofFill();
    ofSetLineWidth(1);

    // BACKGROUND GUI

    // canvas grid (TouchDesigner style)
    ofSetColor(255,255,255,6);
    ofSetLineWidth(1);
    for(int i=0;i<60;i++){
        ofDrawLine(ofGetWindowWidth()/30 * i,0,ofGetWindowWidth()/30 * i,ofGetWindowHeight());
        ofDrawLine(0,ofGetWindowWidth()/30 * i,ofGetWindowWidth(),ofGetWindowWidth()/30 * i);
    }

    // Logo
    ofSetColor(255,255,255,16);
    mosaicLogo->draw(ofGetWindowWidth()/2 - (128*visualProgramming->scaleFactor),(ofGetWindowHeight()- (240*visualProgramming->scaleFactor))/2 - (128*visualProgramming->scaleFactor),256*visualProgramming->scaleFactor,256*visualProgramming->scaleFactor);

    // Mosaic Visual Programming
    ofSetColor(255,255,255);
    if(!visualProgramming->bLoadingNewPatch){
        // draw main GUI interface
        drawImGuiInterface();

        // Draw to vp Gui
        visualProgramming->draw();

        // Manually render ImGui once ofxVP rendered to it.
        mainMenu.draw();
    }

    // DSP flag
    if(visualProgramming->dspON){
        ofSetColor(ofColor::fromHex(0xFFD00B));
        visualProgramming->font->draw("DSP ON",visualProgramming->fontSize,10*visualProgramming->scaleFactor,ofGetHeight() - (6*visualProgramming->scaleFactor));
    }else{
        ofSetColor(ofColor::fromHex(0x777777));
        visualProgramming->font->draw("DSP OFF",visualProgramming->fontSize,10*visualProgramming->scaleFactor,ofGetHeight() - (6*visualProgramming->scaleFactor));
    }

    // Last LOG on bottom bar
    string tmpMsg = mosaicLoggerChannel->GetLastLog();
    if(tmpMsg.find("[warning") != std::string::npos) {
        ofSetColor(255, 127, 0);
    }else if(tmpMsg.find("[ error") != std::string::npos || tmpMsg.find("[ fatal") != std::string::npos) {
        ofSetColor(255, 45, 45);
    }else{
        ofSetColor(220,220,220);
    }

    // FORCE CUSTOM VERBOSE
    if(tmpMsg.find("[verbose]") != std::string::npos){
        ofSetColor(60, 255, 60);
    }
    visualProgramming->font->draw(tmpMsg,visualProgramming->fontSize,100*visualProgramming->scaleFactor,ofGetHeight() - (6*visualProgramming->scaleFactor));

    // subtitler
    if(showSubtitler){
        ofSetColor(0,0,0,100);
        ofDrawRectangle(0,ofGetWindowHeight()-(166*visualProgramming->scaleFactor),ofGetWindowWidth(),147*visualProgramming->scaleFactor);
        ofSetColor(245);
        // cut subtitle at second newline
        int subLastPos = nthOccurrence(actualSubtitle,"\n",2);
        string finalSubtitle = "";
        if(subLastPos != -1){
            finalSubtitle = actualSubtitle.substr(0,subLastPos);
        }else{
            finalSubtitle = actualSubtitle;
        }
        if(isRetina){
            visualProgramming->font->drawMultiLine(finalSubtitle,96,0,ofGetHeight()-(100*visualProgramming->scaleFactor),OF_ALIGN_HORZ_CENTER,ofGetWidth());
        }else{
            visualProgramming->font->drawMultiLine(finalSubtitle,64,0,ofGetHeight()-(100*visualProgramming->scaleFactor),OF_ALIGN_HORZ_CENTER,ofGetWidth());
        }

    }

    // mouse click on recording
    if(showMouseOnRec && showingClickAnimation){ // && recorder.isRecording()
        if(mouseClickRadius < 15.0f*visualProgramming->scaleFactor){
            mouseClickRadius += 1.0f*visualProgramming->scaleFactor;
        }else{
            showingClickAnimation = false;
        }
        ofNoFill();
        ofSetLineWidth(4);
        ofSetColor(182,30,41,250);
        ofDrawCircle(lastclickPos.x,lastclickPos.y,mouseClickRadius);
    }

}

//--------------------------------------------------------------
void ofApp::drawImGuiInterface(){
    mainMenu.begin();

    {
        ImGui::BeginMainMenuBar();

        {

            isHoverMenu = ImGui::IsAnyWindowHovered() || ImGui::IsAnyItemHovered();

            openPatch           = false;
            savePatchAs         = false;
            openAutoloadPatch   = false;
            takeScreenshot      = false;
            exportVideoFlag     = false;

            if(ImGui::BeginMenu( "File")){
                if(ImGui::MenuItem( "New patch",ofToString(shortcutFunc+"+N").c_str())){
                    visualProgramming->newPatch();
                }
                ImGui::Separator();
                if(ImGui::MenuItem( "Open patch" )){
                    openPatch = true;
                }
                ImGui::Separator();
                if(ImGui::MenuItem( "Save patch As.." )){
                    savePatchAs = true;
                }
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Separator();
                ImGui::Spacing();

                if(ImGui::BeginMenu( "Autoload")){

                    if(ImGui::MenuItem( "Set autoload patch" )){
                        openAutoloadPatch = true;
                    }
                    ImGui::Separator();
                    if(ImGui::MenuItem( "Remove autoload patch" )){
                        autoloadPatchFile = "";
                        autoloadDelaySeconds = 1;
                        setAutoloadConfig();
                    }
                    ImGui::Spacing();
                    ImGui::Spacing();
                    if(ImGui::DragInt("Delay ( Seconds )",&autoloadDelaySeconds)){
                        if(autoloadDelaySeconds < 1){
                            autoloadDelaySeconds = 1;
                        }
                    }
                    ImGui::Spacing();
                    ImGui::Spacing();
                    if(autoloadPatchFile == ""){
                        ImGui::Text("no patch in autoload mode");
                    }else{
                        ImGui::Text("%s",_apf.getFileName().c_str());
                        if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s",_apf.getAbsolutePath().c_str());
                    }
                    ImGui::Spacing();
                    ImGui::Spacing();
                    if(ImGui::Button("APPLY",ImVec2(-1,26*visualProgramming->scaleFactor))){
                        setAutoloadConfig();
                    }

                    ImGui::EndMenu();
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Separator();
                ImGui::Spacing();
                if(ImGui::MenuItem( "Quit",ofToString(shortcutFunc+"+Q").c_str())){
                    quitMosaic();
                }
                ImGui::EndMenu();
            }

            if(ImGui::BeginMenu( "Objects")){
                ofxVPObjects::factory::objectCategories& objectsMatrix = ofxVPObjects::factory::getCategories();
                for(ofxVPObjects::factory::objectCategories::iterator it = objectsMatrix.begin(); it != objectsMatrix.end(); ++it ){
                    if(ImGui::BeginMenu(it->first.c_str())){
                        std::sort(it->second.begin(), it->second.end());
                        for(int j=0;j<static_cast<int>(it->second.size());j++){
                            if(it->second.at(j) != "audio device"){
                                if(ImGui::MenuItem(it->second.at(j).c_str())){
                                    visualProgramming->addObject(it->second.at(j),ofVec2f(visualProgramming->canvas.getMovingPoint().x + 200,visualProgramming->canvas.getMovingPoint().y + 200));

                                }
                            }
                        }
                        ImGui::EndMenu();
                    }
                }
                ImGui::EndMenu();
            }

            if(ImGui::BeginMenu( "Examples")){
                #if defined(TARGET_OSX)
                examplesRoot.listDir(mosaicExamplesPath.string());
                #else
                examplesRoot.listDir(ofToDataPath("../examples"));
                #endif
                examplesRoot.sort();
                for(int i=0;i<examplesRoot.getFiles().size();i++){
                    createDirectoryNode(examplesRoot.getFiles().at(i));
                }

                ImGui::EndMenu();
            }


            if(ImGui::BeginMenu( "Sound")){
                if(ImGui::Checkbox("DSP",&visualProgramming->dspON)){
                    if(visualProgramming->dspON){
                        visualProgramming->activateDSP();
                    }else{
                        visualProgramming->deactivateDSP();
                    }
                }
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Separator();
                ImGui::Spacing();
                if(ImGui::DragInt("TEMPO",&mosaicBPM,1.0f,1)){
                    visualProgramming->bpm = mosaicBPM;
                    visualProgramming->engine->sequencer.setTempo(mosaicBPM);
                    visualProgramming->setPatchVariable("bpm",mosaicBPM);
                }
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Separator();
                ImGui::Spacing();
                static int inDev = visualProgramming->audioGUIINIndex;
                if(ofxImGui::VectorCombo("Input Device", &inDev,visualProgramming->audioDevicesStringIN)){
                    visualProgramming->setAudioInDevice(inDev);
                }
                static int outDev = visualProgramming->audioGUIOUTIndex;
                if(ofxImGui::VectorCombo("Output Device", &outDev,visualProgramming->audioDevicesStringOUT)){
                    visualProgramming->setAudioOutDevice(outDev);
                }
                ImGui::EndMenu();
            }

            if(ImGui::BeginMenu( "System")){
                if(ImGui::DragInt("FPS",&mosaicFPS,1.0f,1)){
                    setMosaicFrameRate(mosaicFPS);
                }
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::Spacing();
                ImGui::Text("Desktop Recorder");

                ImGui::Spacing();
                if(ImGui::Button(ICON_FA_FILE_UPLOAD)){
                    exportVideoFlag = true;
                }
                ImGui::SameLine();
                if(recordFilepath == ""){
                    ImGui::Text("Select file...");
                }else{
                    ofFile tempFilename(recordFilepath);
                    ImGui::Text("%s",tempFilename.getFileName().c_str());
                    if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s",tempFilename.getAbsolutePath().c_str());
                }

                ImGui::Spacing();
                ImGui::PushStyleColor(ImGuiCol_Button, VHS_RED);
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, VHS_RED_OVER);
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, VHS_RED_OVER);
                char tmp[256];
                sprintf(tmp,"%s %s",ICON_FA_CIRCLE, recButtonLabel.c_str());
                if(ImGui::Button(tmp,ImVec2(-1,26*visualProgramming->scaleFactor))){
                    if(recordFilepath != ""){
                        if(!recorder.isRecording()){
                            captureFbo.allocate(ofGetWindowWidth(), ofGetWindowHeight(), GL_RGB );
                            recorder.setup(true, false, glm::vec2(ofGetWindowWidth(), ofGetWindowHeight())); // record video only
                            ofSetVerticalSync(false);
                            recorder.setOverWrite(true);
                            recorder.setBitRate(20000);
                            recorder.startCustomRecord();
                            recButtonLabel = "STOP";
                            ofLog(OF_LOG_NOTICE,"START RECORDING MOSAIC WINDOW");
                        }else if(recorder.isRecording()){
                            ofSetVerticalSync(true);
                            recorder.stop();
                            recButtonLabel = "REC";
                            ofLog(OF_LOG_NOTICE,"FINISHED RECORDING MOSAIC WINDOW");
                        }
                    }else{
                        ofLog(OF_LOG_ERROR,"SELECT FILE BEFORE RECORD VIDEO!");
                    }
                }
                ImGui::PopStyleColor(3);
                ImGui::Spacing();
                if(ImGui::BeginCombo("Codec", codecsList.at(selectedCodec).c_str() )){
                    for(int i=0; i < codecsList.size(); ++i){
                        bool is_selected = (selectedCodec == i );
                        if (ImGui::Selectable(codecsList.at(i).c_str(), is_selected)){
                            selectedCodec = i;
                            recorder.setVideoCodec(codecsList.at(selectedCodec));
                        }
                        if (is_selected) ImGui::SetItemDefaultFocus();
                    }

                    ImGui::EndCombo();
                }

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Separator();
                ImGui::Spacing();

                ImGui::Checkbox("Show mouse clicks",&showMouseOnRec);
                ImGui::Spacing();

                ImGui::Checkbox("Subtitler",&showSubtitler);
                ImGui::Spacing();
                ImGui::PushItemWidth(-1);
                ImGui::InputTextMultiline("##subtitle",&actualSubtitle,ImVec2(-1,ImGui::GetFontSize()*3));
                ImGui::PopItemWidth();

                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Separator();
                ImGui::Spacing();
                if(ImGui::Button("Screenshot", ImVec2(-1,26*visualProgramming->scaleFactor))){
                    takeScreenshot = true;
                }
                ImGui::EndMenu();
            }

            if(ImGui::BeginMenu( "View")){
                if(ImGui::Checkbox("Code Editor",&isCodeEditorON)){
                    if(visualProgramming->inspectorActive && isCodeEditorON){
                        visualProgramming->inspectorActive = false;
                    }
                    initGuiPositions();
                }
                if(ImGui::Checkbox("Logger",&isLoggerON)){
                    showConsoleWindow       = isLoggerON;
                    initGuiPositions();
                }
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Spacing();
                ImGui::Checkbox("Profiler",&visualProgramming->profilerActive);
                if(ImGui::Checkbox("Inspector",&visualProgramming->inspectorActive)){
                    if(visualProgramming->inspectorActive && isCodeEditorON){
                        isCodeEditorON = false;
                    }
                    initGuiPositions();
                }
                ImGui::EndMenu();
            }

            if(ImGui::BeginMenu( "Help")){
                if(ImGui::MenuItem("Mosaic Github")){
                    ofLaunchBrowser("https://github.com/d3cod3/Mosaic");
                }
                if(ImGui::MenuItem("Mosaic Manual")){
                    ofLaunchBrowser("https://mosaic.d3cod3.org/manual/");
                }
                if(ImGui::MenuItem("Mosaic Reference")){
                    ofLaunchBrowser("https://mosaic.d3cod3.org/#reference");
                }
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Separator();
                ImGui::Spacing();
                if(ImGui::MenuItem("OF Reference")){
                    ofLaunchBrowser("https://openframeworks.cc/documentation/");
                }
                if(ImGui::MenuItem("ofxAddons")){
                    ofLaunchBrowser("http://ofxaddons.com/categories");
                }
                ImGui::Spacing();
                ImGui::Separator();
                ImGui::Separator();
                ImGui::Spacing();
                if(ImGui::MenuItem("About Mosaic")){
                    showAboutWindow = !showAboutWindow;
                }
                ImGui::EndMenu();
            }

            // File dialogs
            if(openPatch) ImGui::OpenPopup("Open patch");
            if(savePatchAs) ImGui::OpenPopup("Save patch");
            if(openAutoloadPatch) ImGui::OpenPopup("Set autoload patch");
            if(takeScreenshot) ImGui::OpenPopup("Take screenshot");
            if(exportVideoFlag) ImGui::OpenPopup("Record video");

            // open patch
            if( fileDialog.showFileDialog("Open patch", imgui_addons::ImGuiFileBrowser::DialogMode::OPEN, ImVec2(FILE_DIALOG_WIDTH*visualProgramming->scaleFactor, FILE_DIALOG_HEIGHT*visualProgramming->scaleFactor), ".xml") ){
                ofFile file(fileDialog.selected_path);
                if (file.exists()){
                    string fileExtension = ofToUpper(file.getExtension());
                    if(fileExtension == "XML") {
                        ofxXmlSettings XML;
                        if (XML.loadFile(file.getAbsolutePath())){
                            if (XML.getValue("www","") == "https://mosaic.d3cod3.org"){
                                patchToLoad = file.getAbsolutePath();
                                loadNewPatch = true;
                            }else{
                                ofLog(OF_LOG_ERROR, "The opened file: %s, is not a Mosaic patch!",file.getAbsolutePath().c_str());
                            }
                        }
                    }
                }
            }

            // set autoload patch
            if( fileDialog.showFileDialog("Set autoload patch", imgui_addons::ImGuiFileBrowser::DialogMode::OPEN, ImVec2(FILE_DIALOG_WIDTH*visualProgramming->scaleFactor, FILE_DIALOG_HEIGHT*visualProgramming->scaleFactor), ".xml") ){
                ofFile file(fileDialog.selected_path);
                if (file.exists()){
                    string fileExtension = ofToUpper(file.getExtension());
                    if(fileExtension == "XML") {
                        ofxXmlSettings XML;
                        if (XML.loadFile(file.getAbsolutePath())){
                            if (XML.getValue("www","") == "https://mosaic.d3cod3.org"){
                                autoloadPatchFile = file.getAbsolutePath();
                                _apf.open(autoloadPatchFile);
                            }else{
                                ofLog(OF_LOG_ERROR, "The opened file: %s, is not a Mosaic patch!",file.getAbsolutePath().c_str());
                            }
                        }
                    }
                }
            }

            // save patch
            string newFileName = "mosaicPatch_"+ofGetTimestampString("%y%m%d")+".xml";
            if( fileDialog.showFileDialog("Save patch", imgui_addons::ImGuiFileBrowser::DialogMode::SAVE, ImVec2(FILE_DIALOG_WIDTH*visualProgramming->scaleFactor, FILE_DIALOG_HEIGHT*visualProgramming->scaleFactor), ".xml", newFileName) ){
                ofFile file(fileDialog.selected_path);
                visualProgramming->savePatchAs(file.getAbsolutePath());
            }

            // take patch screenshot
            string newShotName = "mosaicScreenshot_"+ofGetTimestampString("%y%m%d")+".jpg";
            if( fileDialog.showFileDialog("Take screenshot", imgui_addons::ImGuiFileBrowser::DialogMode::SAVE, ImVec2(FILE_DIALOG_WIDTH*visualProgramming->scaleFactor, FILE_DIALOG_HEIGHT*visualProgramming->scaleFactor), ".jpg", newShotName) ){
                ofFile file(fileDialog.selected_path);
                lastScreenshot = file.getAbsolutePath();
                saveNewScreenshot = true;
                resetScreenshotTime = ofGetElapsedTimeMillis();
            }

            // record video
            #if defined(TARGET_WIN32)
            string newRecordVideoName = "mosaicVideoRecorder_"+ofGetTimestampString("%y%m%d")+".avi";
            if( fileDialog.showFileDialog("Record video", imgui_addons::ImGuiFileBrowser::DialogMode::SAVE, ImVec2(FILE_DIALOG_WIDTH*visualProgramming->scaleFactor, FILE_DIALOG_HEIGHT*visualProgramming->scaleFactor), ".avi", newRecordVideoName) ){
                ofFile file(fileDialog.selected_path);
                recordFilepath = file.getAbsolutePath();
                // check extension
                if(fileDialog.ext != ".avi"){
                    recordFilepath += ".avi";
                }
                recorder.setOutputPath(recordFilepath);
                // prepare blank video file
                recorder.startCustomRecord();
                recorder.stop();
            }
            #else
            string newRecordVideoName = "mosaicVideoRecorder_"+ofGetTimestampString("%y%m%d")+".mp4";
            if( fileDialog.showFileDialog("Record video", imgui_addons::ImGuiFileBrowser::DialogMode::SAVE, ImVec2(FILE_DIALOG_WIDTH*visualProgramming->scaleFactor, FILE_DIALOG_HEIGHT*visualProgramming->scaleFactor), ".mp4", newRecordVideoName) ){
                ofFile file(fileDialog.selected_path);
                recordFilepath = file.getAbsolutePath();
                // check extension
                if(fileDialog.ext != ".mp4"){
                    recordFilepath += ".mp4";
                }
                recorder.setOutputPath(recordFilepath);
                // prepare blank video file
                recorder.startCustomRecord();
                recorder.stop();
            }
            #endif

        }

        ImGui::EndMainMenuBar();

        // About window
        if(showAboutWindow){

            ImGui::SetNextWindowPos(ImVec2((ofGetWidth()-(400*visualProgramming->scaleFactor))*.5f,(ofGetHeight()-(400*visualProgramming->scaleFactor))*.5f), ImGuiCond_Appearing );
            ImGui::SetNextWindowSize(ImVec2(400*visualProgramming->scaleFactor,400*visualProgramming->scaleFactor), ImGuiCond_Appearing );

            if( ImGui::Begin("About Mosaic", &showAboutWindow, ImGuiWindowFlags_NoCollapse ) ){

                if(mosaicLogo && mosaicLogo->isAllocated() && mosaicLogoID && mosaicLogo->getWidth()!=0 ){
                    float ratio = (150.f*visualProgramming->scaleFactor) / mosaicLogo->getWidth();
                    ImGui::Image(GetImTextureID(mosaicLogoID), ImVec2(mosaicLogo->getWidth()*ratio, mosaicLogo->getHeight()*ratio));
                }
                ImGui::Text( "%s", PACKAGE);
                ImGui::Text( "Version %s (%s)", VERSION, VERSION_GRAPHIC );
                ImGui::Spacing();
                ImGui::TextWrapped( DESCRIPTION );
                ImGui::TextWrapped( MOSAIC_WWW );
                ImGui::TextWrapped( "Developers: %s", MOSAIC_DEVELOPERS );
                ImGui::Spacing();
                ImGui::Text(" ");
                ImGui::Spacing();

                if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None)){

                    if (ImGui::BeginTabItem("Build Info")){

                        ImGui::TextWrapped("Feel free to include the following information in bug reports.");
                        bool copy_to_clipboard = ImGui::Button("Copy to clipboard");

                        ImGui::Spacing();
                        ImGui::BeginChildFrame(ImGui::GetID("Build Configuration"), ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * (18*visualProgramming->scaleFactor)), ImGuiWindowFlags_NoMove);
                        if (copy_to_clipboard){
                            ImGui::LogToClipboard();
                        }
#ifdef DEBUG
#define BUILDVARIANT "Debug"
#else
#define BUILDVARIANT "Release"
#endif
                        ImGui::Text( "%s version %s (%s) (%s build)", PACKAGE, VERSION, VERSION_GRAPHIC , BUILDVARIANT );
                        ImGui::Separator();
#ifdef _WIN32
                        ImGui::Text("define: _WIN32");
#endif
#ifdef _WIN64
                        ImGui::Text("define: _WIN64");
#endif
#ifdef __linux__
                        ImGui::Text("define: __linux__");
#endif
#ifdef __APPLE__
                        ImGui::Text("define: __APPLE__");
#endif
#ifdef _MSC_VER
                        ImGui::Text("define: _MSC_VER=%d", _MSC_VER);
#endif
#ifdef __MINGW32__
                        ImGui::Text("define: __MINGW32__");
#endif
#ifdef __MINGW64__
                        ImGui::Text("define: __MINGW64__");
#endif
#ifdef __GNUC__
                        ImGui::Text("define: __GNUC__=%d", (int)__GNUC__);
#endif
                        ImGui::Text("define: __cplusplus=%d", (int)__cplusplus);

                        ImGui::Separator();

                        ofxVPObjects::factory::objectCategories& objectsMatrix = ofxVPObjects::factory::getCategories();
                        for(ofxVPObjects::factory::objectCategories::iterator it = objectsMatrix.begin(); it != objectsMatrix.end(); ++it ){
                            if(it->second.size()<1) continue;

                            for(auto objIt=it->second.begin(); objIt!=it->second.end(); ++objIt){
                                ImGui::Text("object/%s: %s", it->first.c_str(), (*objIt).c_str() );
                            }
                        }

                        ImGui::Spacing();

                        if (copy_to_clipboard){
                            ImGui::LogFinish();
                        }
                        ImGui::EndChildFrame();

                        ImGui::EndTabItem();
                    }

                    if (ImGui::BeginTabItem("Objects")){
                        ImGui::TextWrapped("This version of Mosaic has been built with the following objects :" );
                        ImGui::Spacing();

                        ofxVPObjects::factory::objectCategories& objectsMatrix = ofxVPObjects::factory::getCategories();
                        if (objectsMatrix.size()>0){

                            ofxVPObjects::factory::objectCategories& objectsMatrix = ofxVPObjects::factory::getCategories();
                            for(ofxVPObjects::factory::objectCategories::iterator it = objectsMatrix.begin(); it != objectsMatrix.end(); ++it ){
                                if (ImGui::TreeNodeEx( it->first.c_str(), ImGuiTreeNodeFlags_DefaultOpen )){
                                    ImGui::Indent(10);
                                    for(auto objIt=it->second.begin(); objIt!=it->second.end(); ++objIt){
                                        ImGui::Text("%s", (*objIt).c_str());
                                    }
                                    ImGui::Unindent(10);
                                    ImGui::TreePop();
                                }
                            }
                        }
                        else {
                            ImGui::Text("There are no objects.");
                        }

                        ImGui::EndTabItem();
                    }

                    ImGui::EndTabBar();
                }
            }
            ImGui::End(); // end showAboutWindow
        }

        // code editor
        if(isCodeEditorON){
            if( ImGui::Begin("Code Editor", nullptr, ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoCollapse) ){

                ImGui::SetWindowPos(ImVec2(codeEditorRect.x,codeEditorRect.y), ImGuiCond_Always);
                ImGui::SetWindowSize(ImVec2(codeEditorRect.width, codeEditorRect.height), ImGuiCond_Always);

                //isHoverCodeEditor = ImGui::IsAnyWindowHovered() || ImGui::IsAnyItemHovered();
                isHoverCodeEditor = codeEditorRect.inside(ofGetMouseX(),ofGetMouseY());

                if(codeEditors.size() > 0){

                    auto cpos = codeEditors[editedFilesNames[actualCodeEditor]].GetCursorPosition();

                    if (ImGui::BeginMenuBar()){
                        if (ImGui::BeginMenu("File")){
                            if (ImGui::MenuItem("Save/Reload",ofToString(shortcutFunc+"+R").c_str())){
                                filesystem::path tempPath(editedFilesPaths[actualCodeEditor].c_str());

                                ofBuffer buff;
                                buff.set(codeEditors[editedFilesNames[actualCodeEditor]].GetText());

                                ofBufferToFile(tempPath,buff,false);
                            }
                            ImGui::EndMenu();
                        }
                        if (ImGui::BeginMenu("Edit")){
                            if (ImGui::MenuItem("Undo", ofToString(shortcutFunc+"+Z").c_str(), nullptr, codeEditors[editedFilesNames[actualCodeEditor]].CanUndo()))
                                codeEditors[editedFilesNames[actualCodeEditor]].Undo();
                            if (ImGui::MenuItem("Redo", ofToString(shortcutFunc+"+Y").c_str(), nullptr, codeEditors[editedFilesNames[actualCodeEditor]].CanRedo()))
                                codeEditors[editedFilesNames[actualCodeEditor]].Redo();

                            ImGui::Separator();

                            if (ImGui::MenuItem("Copy", ofToString(shortcutFunc+"+C").c_str(), nullptr, codeEditors[editedFilesNames[actualCodeEditor]].HasSelection()))
                                codeEditors[editedFilesNames[actualCodeEditor]].Copy();
                            if (ImGui::MenuItem("Cut", ofToString(shortcutFunc+"+X").c_str(), nullptr, codeEditors[editedFilesNames[actualCodeEditor]].HasSelection()))
                                codeEditors[editedFilesNames[actualCodeEditor]].Cut();
                            if (ImGui::MenuItem("Paste", ofToString(shortcutFunc+"+V").c_str(), nullptr, ImGui::GetClipboardText() != nullptr))
                                codeEditors[editedFilesNames[actualCodeEditor]].Paste();

                            ImGui::Separator();

                            if (ImGui::MenuItem("Select all", ofToString(shortcutFunc+"+A").c_str()))
                                codeEditors[editedFilesNames[actualCodeEditor]].SetSelection(TextEditor::Coordinates(), TextEditor::Coordinates(codeEditors[editedFilesNames[actualCodeEditor]].GetTotalLines(), 0));

                            ImGui::EndMenu();
                        }

                        ImGui::EndMenuBar();
                    }

                    ImGui::Text("%6d/%-6d %6d lines | %s | %s", cpos.mLine + 1, cpos.mColumn + 1, codeEditors[editedFilesNames[actualCodeEditor]].GetTotalLines(), codeEditors[editedFilesNames[actualCodeEditor]].GetLanguageDefinition().mName.c_str(), editedFilesPaths[actualCodeEditor].c_str());

                    //ImGuiIO& io = ImGui::GetIO();
                    //ImFontAtlas* atlas = ImGui::GetIO().Fonts;
                    ImFont* tempfont = ImGui::GetIO().Fonts->Fonts[ImGui::GetIO().Fonts->Fonts.Size - 2];

                    ImGui::Spacing();
                    ImGui::Spacing();
                    ImGui::Spacing();

                    ImGui::SliderFloat("Font scale", &tempfont->Scale, 1.0f, 2.0f);
                    ImGui::SameLine();
                    ImGui::Spacing();
                    ImGui::SameLine();
                    ImGui::Spacing();
                    ImGui::SameLine();
                    ImGui::Spacing();
                    ImGui::SameLine();
                    if(ImGui::ImageButton(GetImTextureID(editorFullscreenButtonID), ImVec2(24*visualProgramming->scaleFactor,24*visualProgramming->scaleFactor))){
                        isCodeEditorFullWindow = !isCodeEditorFullWindow;
                        initGuiPositions();
                    }

                    ImGui::Spacing();
                    ImGui::Spacing();
                    ImGui::Spacing();

                    if (ImGui::BeginTabBar("TabBar", ImGuiTabBarFlags_Reorderable | ImGuiTabBarFlags_AutoSelectNewTabs | ImGuiTabBarFlags_NoCloseWithMiddleMouseButton)){
                        for(int i=0;i<editedFilesNames.size();i++){
                            if (ImGui::BeginTabItem(editedFilesNames[i].c_str(), nullptr)){

                                if(ImGui::IsItemClicked() || ImGui::IsItemActive()){
                                    actualCodeEditor = i;
                                    actualEditedFilePath = editedFilesPaths[i];
                                    actualEditedFileName = editedFilesNames[i];
                                }

                                ImGui::PushFont(tempfont);

                                codeEditors[editedFilesNames[i]].Render("TextEditor");

                                ImGui::PopFont();

                                ImGui::EndTabItem();
                            }
                        }
                        ImGui::EndTabBar();
                    }

                }

                ImGui::End();
            }
        }

        // floating logger window
        if(showConsoleWindow){
            ImGui::SetNextWindowSize(ImVec2(loggerRect.width,loggerRect.height), ImGuiCond_Always);
            ImGui::SetNextWindowPos(ImVec2(loggerRect.x,loggerRect.y), ImGuiCond_Always);
            mosaicLoggerChannel->Draw("Logger");

            isHoverLogger = loggerRect.inside(ofGetMouseX(),ofGetMouseY());
        }else{
            isHoverLogger = false;
        }

        // right click menu
        if(showRightClickMenu){
            ImGui::SetNextWindowSize(ImVec2(200*visualProgramming->scaleFactor,280*visualProgramming->scaleFactor), ImGuiCond_Always);
            ImGui::SetNextWindowPos(ImVec2(ofGetMouseX(),ofGetMouseY()), ImGuiCond_Appearing);

            if(ImGui::Begin("Objects", &showRightClickMenu,ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse) ){

                isHoverMenu = ImGui::IsAnyWindowHovered() || ImGui::IsAnyItemHovered();

                static ImGuiTextFilter filter;
                filter.Draw("Search");
                bool bIsFiltering = filter.IsActive();

                bool bApplyFilter = false;
                if( createSearchedObject ){
                    createSearchedObject = false;
                    bApplyFilter = true;
                    bIsFiltering = true;
                }
                bool bDidSelectObject = false;

                ofxVPObjects::factory::objectCategories& objectsMatrix = ofxVPObjects::factory::getCategories();
                for(ofxVPObjects::factory::objectCategories::iterator it = objectsMatrix.begin(); it != objectsMatrix.end(); ++it ){
                    if(!bIsFiltering){
                        if(ImGui::BeginMenu(it->first.c_str())){
                            std::set<std::string> tempSecond; // for alphabetical sort
                            for(int j=0;j<static_cast<int>(it->second.size());j++){
                                tempSecond.emplace(it->second.at(j));
                            }
                            for(auto const &on : tempSecond){
                                // show items
                                if(on != "audio device"){
                                    if(ImGui::MenuItem(on.c_str())){
                                        visualProgramming->addObject(on,ofVec2f(visualProgramming->canvas.getMovingPoint().x + 200,visualProgramming->canvas.getMovingPoint().y + 200));
                                        showRightClickMenu = false;
                                    }
                                }
                            }
                            ImGui::EndMenu();
                        }
                    }
                    else {
                        bool bCatPrinted = false;
                        for(int j=0;j<static_cast<int>(it->second.size());j++){
                            // filter items
                            if (!filter.PassFilter(it->second.at(j).c_str()))
                                continue;

                            // show sub cat
                            if( !bCatPrinted ){
                                bCatPrinted = ImGui::TreeNodeEx(it->first.c_str(), ImGuiTreeNodeFlags_DefaultOpen );
                            }
                            // show items
                            ImGuiTreeNodeFlags tmpFlags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
                            if(!bDidSelectObject) tmpFlags |= ImGuiTreeNodeFlags_Selected;
                            if(it->second.at(j) != "audio device"){
                                if(ImGui::TreeNodeEx(it->second.at(j).c_str(), tmpFlags)){
                                    // choose by click or pick first one
                                    if (ImGui::IsItemClicked() || bApplyFilter){
                                        visualProgramming->addObject(it->second.at(j),ofVec2f(visualProgramming->canvas.getMovingPoint().x + (200*visualProgramming->scaleFactor),visualProgramming->canvas.getMovingPoint().y + (200*visualProgramming->scaleFactor)));
                                        showRightClickMenu = false;
                                        bApplyFilter = false;
                                        filter.Clear();
                                        break;
                                    }
                                }
                            }
                            bDidSelectObject = true;


                        }
                        if(bCatPrinted){
                            ImGui::TreePop();
                        }
                    }
                }

                // at this point, if apply filter is true, there was no match, reset search.
                if(bApplyFilter){
                    filter.Clear();
                }

                ImGui::End();
            }

        }else{
            //isHoverMenu = false;
        }

    }

    mainMenu.end();
}

//--------------------------------------------------------------
void ofApp::initGuiPositions(){

    if(visualProgramming->inspectorActive){
        loggerRect.set(0,ofGetWindowHeight()-(254*visualProgramming->scaleFactor),ofGetWindowWidth() - visualProgramming->lastInspectorWidth,234*visualProgramming->scaleFactor);
    }else{
        loggerRect.set(0,ofGetWindowHeight()-(254*visualProgramming->scaleFactor),ofGetWindowWidth(),234*visualProgramming->scaleFactor);
    }

    if(isCodeEditorFullWindow){
        if(isLoggerON){
            codeEditorRect.set(0, (26*visualProgramming->scaleFactor),ofGetWindowWidth(), ofGetWindowHeight()-(280*visualProgramming->scaleFactor));
        }else{
            codeEditorRect.set(0, (26*visualProgramming->scaleFactor),ofGetWindowWidth(), ofGetWindowHeight()-(26*visualProgramming->scaleFactor));
        }
    }else{
        if(isLoggerON){
            codeEditorRect.set((ofGetWindowWidth()/3*2) + 1, (26*visualProgramming->scaleFactor),ofGetWindowWidth()/3, ofGetWindowHeight()-(280*visualProgramming->scaleFactor));
        }else{
            codeEditorRect.set((ofGetWindowWidth()/3*2) + 1, (26*visualProgramming->scaleFactor),ofGetWindowWidth()/3, ofGetWindowHeight()-(26*visualProgramming->scaleFactor));
        }

    }
}

//--------------------------------------------------------------
void ofApp::exit() {
    visualProgramming->exit();
}

//--------------------------------------------------------------
void ofApp::keyPressed(ofKeyEventArgs &e){

    //ofLog(OF_LOG_NOTICE,"%i",e.keycode);

    if(e.hasModifier(MOD_KEY) && e.keycode == 78) {
        visualProgramming->newPatch();
    // refresh/save actual editing script
    }else if(e.hasModifier(MOD_KEY) && e.keycode == 82){
        filesystem::path tempPath(editedFilesPaths[actualCodeEditor].c_str());
        ofBuffer buff;
        buff.set(codeEditors[editedFilesNames[actualCodeEditor]].GetText());
        ofBufferToFile(tempPath,buff,false);
    }else if(e.keycode == 257){
        createSearchedObject = true;
    }

    #if defined(TARGET_LINUX) || defined(TARGET_WIN32)
    if(e.hasModifier(MOD_KEY) && e.keycode == 81){
        quitMosaic();
    }
    #endif

}

//--------------------------------------------------------------
void ofApp::keyReleased(ofKeyEventArgs &e){
    // OSX: CMD-F, WIN/LINUX: CTRL-F    (FULLSCREEN)
    if(e.hasModifier(MOD_KEY) && e.keycode == 70){
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
    if(button == 2){ // right click
        showRightClickMenu = !showRightClickMenu;
    }

    if(showMouseOnRec){
        lastclickPos.set(x,y);
        mouseClickRadius = 0.0f;
        showingClickAnimation = true;
    }

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

    if(isInited && ofGetElapsedTimeMillis() > 1000){
        isWindowResized = true;

        initGuiPositions();
    }
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){

}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){
    if( dragInfo.files.size() == 1 ){
        ofFile file (dragInfo.files[0]);
        createObjectFromFile(file,false);
    }
}

//--------------------------------------------------------------
void ofApp::urlResponse(ofHttpResponse & response) {
    if (response.status==200 && response.request.name == "check_release_async") {
        // check for updates
        lastRelease = response.data.getText();
        checkForUpdates();
    }
}

//--------------------------------------------------------------
void ofApp::pathChanged(const PathWatcher::Event &event) {
    ofFile tempfile(event.path);
    ofBuffer tempcontent = ofBufferFromFile(tempfile.getAbsolutePath());

    switch(event.change) {
        case PathWatcher::CREATED:
            //ofLogVerbose(PACKAGE) << "path created " << event.path;
            break;
        case PathWatcher::MODIFIED:
            //ofLogVerbose(PACKAGE) << "path modified " << event.path;
            codeEditors[tempfile.getFileName()].SetText(tempcontent.getText(),false);
            break;
        case PathWatcher::DELETED:
            //ofLogVerbose(PACKAGE) << "path deleted " << event.path;
            return;
        default: // NONE
            return;
    }

}

//--------------------------------------------------------------
void ofApp::quitMosaic(){
    ofExit(0);
}

//--------------------------------------------------------------
void ofApp::setMosaicFrameRate(float fps){
    mosaicTiming.setFramerate(fps);
}

//--------------------------------------------------------------
void ofApp::initDataFolderFromBundle(){

    #ifdef TARGET_OSX

    string _bundleDataPath;
    string _bundleExamplesPath;
    string _bundlePluginsPath;

    CFURLRef appUrl = CFBundleCopyBundleURL(CFBundleGetMainBundle());
    CFStringRef appPath = CFURLCopyFileSystemPath(appUrl, kCFURLPOSIXPathStyle);

    const CFIndex kCStringSize = 128;
    char temporaryCString[kCStringSize];
    bzero(temporaryCString,kCStringSize);
    CFStringGetCString(appPath, temporaryCString, kCStringSize, kCFStringEncodingUTF8);
    std::string *appPathStr = new std::string(temporaryCString);
    CFRelease(appUrl);
    CFRelease(appPath);

    CFURLRef resourceUrl = CFBundleCopyResourcesDirectoryURL(CFBundleGetMainBundle());
    CFStringRef resourcePath = CFURLCopyFileSystemPath(resourceUrl, kCFURLPOSIXPathStyle);

    bzero(temporaryCString,kCStringSize);
    CFStringGetCString(resourcePath, temporaryCString, kCStringSize, kCFStringEncodingUTF8);
    std::string *resourcePathStr = new std::string(temporaryCString);
    CFRelease(resourcePath);
    CFRelease(resourceUrl);

    _bundleDataPath = *appPathStr + "/" + *resourcePathStr + "/"; // the absolute path to the resources folder
    _bundleExamplesPath = *appPathStr + "/Contents/examples/";
    _bundlePluginsPath = *appPathStr + "/Contents/plugins/";

    const char *homeDir = getenv("HOME");

    if(!homeDir){
        struct passwd* pwd;
        pwd = getpwuid(getuid());
        if (pwd){
            homeDir = pwd->pw_dir;
        }
    }

    string _MosaicDataPath(homeDir);
    string _MosaicExamplesPath(homeDir);
    string _MosaicPluginsPath(homeDir);
    userHome = _MosaicDataPath;

    _MosaicDataPath += "/Documents/Mosaic/data";
    _MosaicExamplesPath += "/Documents/Mosaic/examples";
    _MosaicPluginsPath += "/Documents/Mosaic/plugins";

    std::filesystem::path tempPath(_MosaicDataPath.c_str());
    std::filesystem::path examplesPath(_MosaicExamplesPath.c_str());
    std::filesystem::path pluginsPath(_MosaicPluginsPath.c_str());

    mosaicPath = tempPath;
    mosaicExamplesPath = examplesPath;
    mosaicPluginsPath = pluginsPath;

    ofDirectory mosaicDir;

    // examples directory
    if(!mosaicDir.doesDirectoryExist(mosaicExamplesPath)){
        mosaicDir.createDirectory(mosaicExamplesPath,true,true);

        std::filesystem::path dataPath(_bundleExamplesPath.c_str());

        ofDirectory dataDir(dataPath);
        dataDir.copyTo(mosaicExamplesPath,true,true);
    }else{
        string relfilepath = _MosaicDataPath+"/release.txt";
        std::filesystem::path releasePath(relfilepath.c_str());
        ofFile relFile(releasePath);

        if(relFile.exists()){
            string actualRel = relFile.readToBuffer().getText();

            if(VERSION != actualRel){
                std::filesystem::path dataPath(_bundleExamplesPath.c_str());

                // remove previous release examples folder
                mosaicDir.removeDirectory(mosaicExamplesPath,true);
                mosaicDir.createDirectory(mosaicExamplesPath,true,true);

                ofDirectory dataDir(dataPath);
                dataDir.copyTo(mosaicExamplesPath,true,true);
            }
        }

    }

    // data directory
    if(!mosaicDir.doesDirectoryExist(mosaicPath)){
        mosaicDir.createDirectory(mosaicPath,true,true);

        std::filesystem::path dataPath(_bundleDataPath.c_str());

        ofDirectory dataDir(dataPath);
        dataDir.copyTo(mosaicPath,true,true);
    }else{
        string relfilepath = _MosaicDataPath+"/release.txt";
        std::filesystem::path releasePath(relfilepath.c_str());
        ofFile relFile(releasePath);

        if(relFile.exists()){
            string actualRel = relFile.readToBuffer().getText();

            if(VERSION != actualRel){
                std::filesystem::path dataPath(_bundleDataPath.c_str());

                // remove previous release data folder
                mosaicDir.removeDirectory(mosaicPath,true);
                mosaicDir.createDirectory(mosaicPath,true,true);

                // copy the new one
                ofDirectory dataDir(dataPath);
                dataDir.copyTo(mosaicPath,true,true);
            }
        }
    }

    // plugins directory
    if(!mosaicDir.doesDirectoryExist(mosaicPluginsPath)){
        mosaicDir.createDirectory(mosaicPluginsPath,true,true);

        std::filesystem::path dataPath(_bundlePluginsPath.c_str());

        ofDirectory dataDir(dataPath);
        dataDir.copyTo(mosaicPluginsPath,true,true);
    }


    ofSetDataPathRoot(mosaicPath); // tell OF to look for resources here

    examplesRoot.listDir(mosaicExamplesPath.string());

    #else

    examplesRoot.listDir(ofToDataPath("../examples"));

    #endif

    examplesRoot.sort();

}

//--------------------------------------------------------------
bool ofApp::checkInternetReachability(){

    string cmd = "";
    FILE *execFile;
    string output = "";
#ifdef TARGET_LINUX
    cmd = "ping -q -c1 www.github.com > /dev/null && echo okk || echo err";
    execFile = popen(cmd.c_str(), "r");
#elif defined(TARGET_OSX)
    cmd = "ping -q -c1 www.github.com > /dev/null && echo okk || echo err";
    execFile = popen(cmd.c_str(), "r");
#endif

    if (execFile){
        ofLog(OF_LOG_NOTICE,"CHECKING INTERNET CONNECTIVITY...");

        char buffer[128];
        if(fgets(buffer, sizeof(buffer), execFile) != nullptr){
            char *s = buffer;
            std::string tempstr(s);
            output = tempstr;
        }

        output.pop_back();

#ifdef TARGET_LINUX
        pclose(execFile);
#elif defined(TARGET_OSX)
        pclose(execFile);
#endif

        if(output == "okk"){
            ofLog(OF_LOG_NOTICE,"[verbose] INTERNET IS AVAILABLE!");
            return true;
        }else if(output == "err"){
            ofLog(OF_LOG_ERROR,"INTERNET IS NOT AVAILABLE!");
            return false;
        }
    }

#ifdef TARGET_WIN32
    ofHttpResponse tempResp = ofLoadURL("https://raw.githubusercontent.com/d3cod3/Mosaic/master/RELEASE.md");

    if(tempResp.status == 200){
        ofLog(OF_LOG_NOTICE,"[verbose] INTERNET IS AVAILABLE!");
        return true;
    }else{
        ofLog(OF_LOG_ERROR,"INTERNET IS NOT AVAILABLE!");
        return false;
    }
#else
      return false;
#endif

}

//--------------------------------------------------------------
void ofApp::checkForUpdates(){
    ofLog(OF_LOG_NOTICE,"CHECKING FOR MOSAIC UPDATES...");

    string actualVersion = VERSION;
    if(ofToInt(string(1,actualVersion.at(0))) < ofToInt(string(1,lastRelease.at(0))) || ( ofToInt(string(1,actualVersion.at(0))) == ofToInt(string(1,lastRelease.at(0))) && ofToInt(string(1,actualVersion.at(2))) < ofToInt(string(1,lastRelease.at(2))) ) || ( ofToInt(string(1,actualVersion.at(0))) == ofToInt(string(1,lastRelease.at(0))) && ofToInt(string(1,actualVersion.at(2))) == ofToInt(string(1,lastRelease.at(2))) && ofToInt(string(1,actualVersion.at(4))) < ofToInt(string(1,lastRelease.at(4))) )){
        ofLog(OF_LOG_NOTICE,"[verbose]NEW MOSAIC "+lastRelease+" UPDATE AVAILABLE!");
    }else{
        ofLog(OF_LOG_NOTICE,"NO NEW MOSAIC UPDATE AVAILABLE!");
    }

}

//--------------------------------------------------------------
void ofApp::checkAutoloadConfig(){
    string autoloadfilepath = ofToDataPath("autoload.txt",true);
    ofFile autoloadFile(autoloadfilepath);

    autoloadPatchFile       = "";
    autoloadDelaySeconds    = 1;
    waitForAutoload         = autoloadDelaySeconds*1000;
    isAutoloadedPatch       = false;

    if(autoloadFile.exists()){
        ofBuffer fileBuffer = ofBufferFromFile(autoloadfilepath);

        // extract autoload data ( fixed txt file with two lines, line one: filepath, line two: delay seconds to load )
        if(fileBuffer.size()) {
            int counter = 0;
            for (ofBuffer::Line it = fileBuffer.getLines().begin(), end = fileBuffer.getLines().end(); it != end; ++it) {
                string line = *it;

                if(counter == 0){
                    autoloadPatchFile       = line;
                }else if(counter == 1){
                    autoloadDelaySeconds    = ofToInt(line);
                    waitForAutoload         = autoloadDelaySeconds*1000;
                    isAutoloadedPatch       = true;
                }

                counter++;
            }
        }
    }

    //ofLog(OF_LOG_NOTICE,"AUTOLOAD Data: %s --> Delay %i",autoloadPatchFile.c_str(),autoloadDelaySeconds);

    autoloadStartTime = ofGetElapsedTimeMillis();
}

//--------------------------------------------------------------
void ofApp::setAutoloadConfig(){
    //std::ofstream file;
    string autoloadfilepath = ofToDataPath("autoload.txt",true);

    std::ofstream file(autoloadfilepath, std::ofstream::trunc);

    //file.open(autoloadfilepath, std::ios::out | std::ios::app);

    file << autoloadPatchFile << std::endl;
    file << ofToString(autoloadDelaySeconds) << std::endl;
    file.close();
}

//--------------------------------------------------------------
void ofApp::createDirectoryNode(ofFile file){
    if(file.isDirectory()){
        if(ImGui::BeginMenu(file.getBaseName().c_str())){
            ofDirectory temp;
            temp.listDir(file.getAbsolutePath());
            temp.sort();
            for(int i=0;i<temp.getFiles().size();i++){
                createDirectoryNode(temp.getFiles().at(i));
            }
            ImGui::EndMenu();
        }
    }else{
        if(file.getBaseName() != "LICENSE" && file.getBaseName() != "README"){
            string fileExtension = ofToUpper(file.getExtension());
            if(fileExtension == "LUA" || fileExtension == "PY" || fileExtension == "FRAG" || fileExtension == "SH" || fileExtension == "PD" || fileExtension == "XML" || fileExtension == "PNG" || fileExtension == "GIF" || fileExtension == "JPG" || fileExtension == "JPEG" || fileExtension == "TIF" || fileExtension == "TIFF" || fileExtension == "WAV" || fileExtension == "OGG" || fileExtension == "MP3" || fileExtension == "FLAC" || fileExtension == "MOV" || fileExtension == "MP4" || fileExtension == "MPEG" || fileExtension == "MPG" || fileExtension == "AVI"){
                if(fileExtension == "LUA"){
                    string tempstr = file.getEnclosingDirectory().substr(0,file.getEnclosingDirectory().find_last_of('/'));
                    if(file.getEnclosingDirectory().substr(tempstr.find_last_of('/')+1,file.getEnclosingDirectory().find_last_of('/')-tempstr.find_last_of('/')-1) == file.getFileName().substr(0,file.getFileName().find_last_of('.'))){
                        string menuName = file.getBaseName()+"."+file.getExtension();
                        if(ImGui::MenuItem(menuName.c_str())){
                            createObjectFromFile(file,true);
                        }
                    }
                }else{
                    string menuName = file.getBaseName()+"."+file.getExtension();
                    if(ImGui::MenuItem(menuName.c_str())){
                        createObjectFromFile(file,true);
                    }
                }

            }
        }

    }
}

//--------------------------------------------------------------
void ofApp::createObjectFromFile(ofFile file,bool temp){
    if (file.exists()){
        //string tempstr = file.getEnclosingDirectory().substr(0,file.getEnclosingDirectory().find_last_of('/'));
        //ofLog(OF_LOG_NOTICE,"%s : %s",file.getEnclosingDirectory().substr(tempstr.find_last_of('/')+1,file.getEnclosingDirectory().find_last_of('/')-tempstr.find_last_of('/')-1).c_str(), file.getFileName().substr(0,file.getFileName().find_last_of('.')).c_str());
        string fileExtension = ofToUpper(file.getExtension());
        if(fileExtension == "XML") {
            ofxXmlSettings XML;

            if (XML.loadFile(file.getAbsolutePath())){
                if (XML.getValue("www","") == "https://mosaic.d3cod3.org"){
                    if(temp){
                        visualProgramming->newTempPatchFromFile(file.getAbsolutePath());
                    }else{
                        visualProgramming->preloadPatch(file.getAbsolutePath());
                    }
                }else{
                    ofLog(OF_LOG_ERROR, "The opened file: %s, is not a Mosaic patch!",file.getAbsolutePath().c_str());
                }
            }
        }else if(fileExtension == "MOV" || fileExtension == "MP4" || fileExtension == "MPEG" || fileExtension == "MPG" || fileExtension == "AVI"){
            visualProgramming->addObject("video player",ofVec2f(visualProgramming->canvas.getMovingPoint().x + 20,visualProgramming->canvas.getMovingPoint().y + 20));
            if(visualProgramming->getLastAddedObject() != nullptr){
                visualProgramming->getLastAddedObject()->autoloadFile(file.getAbsolutePath());
            }
        }else if(fileExtension == "WAV" || fileExtension == "OGG" || fileExtension == "MP3" || fileExtension == "FLAC") {
            visualProgramming->addObject("soundfile player",ofVec2f(visualProgramming->canvas.getMovingPoint().x + 20,visualProgramming->canvas.getMovingPoint().y + 20));
            if(visualProgramming->getLastAddedObject() != nullptr){
                visualProgramming->getLastAddedObject()->autoloadFile(file.getAbsolutePath());
            }
        }else if(fileExtension == "PNG" || fileExtension == "GIF" || fileExtension == "JPG" || fileExtension == "JPEG" || fileExtension == "TIF" || fileExtension == "TIFF") {
            visualProgramming->addObject("image loader",ofVec2f(visualProgramming->canvas.getMovingPoint().x + 20,visualProgramming->canvas.getMovingPoint().y + 20));
            if(visualProgramming->getLastAddedObject() != nullptr){
                visualProgramming->getLastAddedObject()->autoloadFile(file.getAbsolutePath());
            }
        }else if(fileExtension == "LUA") {
            visualProgramming->addObject("lua script",ofVec2f(visualProgramming->canvas.getMovingPoint().x + 20,visualProgramming->canvas.getMovingPoint().y + 20));
            if(visualProgramming->getLastAddedObject() != nullptr){
                visualProgramming->getLastAddedObject()->autoloadFile(file.getAbsolutePath());
            }
        }else if(fileExtension == "PY") {
            visualProgramming->addObject("python script",ofVec2f(visualProgramming->canvas.getMovingPoint().x + 20,visualProgramming->canvas.getMovingPoint().y + 20));
            if(visualProgramming->getLastAddedObject() != nullptr){
                visualProgramming->getLastAddedObject()->autoloadFile(file.getAbsolutePath());
            }
        }else if(fileExtension == "FRAG" || fileExtension == "VERT") {
            visualProgramming->addObject("glsl shader",ofVec2f(visualProgramming->canvas.getMovingPoint().x + 20,visualProgramming->canvas.getMovingPoint().y + 20));
            if(visualProgramming->getLastAddedObject() != nullptr){
                visualProgramming->getLastAddedObject()->autoloadFile(file.getAbsolutePath());
            }
        }else if(fileExtension == "SH") {
            visualProgramming->addObject("bash script",ofVec2f(visualProgramming->canvas.getMovingPoint().x + 20,visualProgramming->canvas.getMovingPoint().y + 20));
            if(visualProgramming->getLastAddedObject() != nullptr){
                visualProgramming->getLastAddedObject()->autoloadFile(file.getAbsolutePath());
            }
        }else if(fileExtension == "PD") {
            visualProgramming->addObject("pd patch",ofVec2f(visualProgramming->canvas.getMovingPoint().x + 20,visualProgramming->canvas.getMovingPoint().y + 20));
            if(visualProgramming->getLastAddedObject() != nullptr){
                visualProgramming->getLastAddedObject()->autoloadFile(file.getAbsolutePath());
            }
        }
    }
}

//--------------------------------------------------------------
void ofApp::initScriptLanguages(){
    // ------------------------------------------- LUA
    static const char* const lua_mosaic_keywords[] = {
        "USING_DATA_INLET", "OUTPUT_WIDTH", "OUTPUT_HEIGHT", "_mosaic_data_inlet", "_mosaic_data_outlet", "SCRIPT_PATH", "mouseX", "mouseY" };

    static const char* const lua_mosaic_keywords_decl[] = {
        "", "The current width of the output window", "The current height of the output window",
        "_mosaic_data_inlet is the name of the lua table storing data incoming from a Mosaic patch.\n A vector<float> is automatically converted to a lua table, where the index starts from 1, NOT 0!\n So the first position of your table will be accessed like this: _mosaic_data_inlet[1]",
        "_mosaic_data_outlet is the name of the lua table storing data outgoing to a Mosaic patch.\n A lua table is automatically converted to a vector<float>. Remember that lua tables index starts from 1, NOT 0!\n So the first position of your table will be accessed like this: _mosaic_data_outlet[1]",
        "Mosaic system variable for loading external resources (files). It returns the absolute path to the script file folder container.",
        "The mouse over output window X (horizontal) coordinate", "The mouse over output window Y (vertical) coordinate"
    };

    for (int i = 0; i < sizeof(lua_mosaic_keywords) / sizeof(lua_mosaic_keywords[0]); ++i){
        TextEditor::Identifier id;
        //id.mDeclaration = lua_mosaic_keywords_decl[i];
        id.mDeclaration = "";
        luaLang.mPreprocIdentifiers.insert(std::make_pair(std::string(lua_mosaic_keywords[i]), id));
    }



    ofxXmlSettings XML;

    if (XML.loadFile("livecoding/lua_mosaic_language.xml")){
        int totalMethods = XML.getNumTags("method");

        // Load all the lua_of_mosaic methods
        for(int i=0;i<totalMethods;i++){
            if(XML.pushTag("method", i)){
                string methname = XML.getValue("name","");
                string methdesc = XML.getValue("desc","");

                TextEditor::Identifier id;
                //id.mDeclaration = fix_newlines(methdesc).c_str();
                id.mDeclaration = "";
                luaLang.mIdentifiers.insert(std::make_pair(std::string(methname.c_str()), id));

                XML.popTag();
            }
        }

    }

}

//--------------------------------------------------------------
void ofApp::initNewCodeEditor(ofFile file){
    actualEditedFilePath = file.getAbsolutePath();
    actualEditedFileName = file.getFileName();
    editedFilesPaths.push_back(actualEditedFilePath);
    editedFilesNames.push_back(actualEditedFileName);

    ofBuffer tempcontent = ofBufferFromFile(actualEditedFilePath);

    string fileExtension = ofToUpper(file.getExtension());

    TextEditor codeEditor;
    if(fileExtension == "LUA"){
        codeEditor.SetLanguageDefinition(luaLang);
    }else if(fileExtension == "PY"){
        codeEditor.SetLanguageDefinition(pythonLang);
    }else if(fileExtension == "SH"){
        codeEditor.SetLanguageDefinition(bashLang);
    }else if(fileExtension == "FRAG" || fileExtension == "VERT"){
        codeEditor.SetLanguageDefinition(glslLang);
    }

    codeEditor.SetPalette(TextEditor::GetMosaicPalette());
    codeEditor.SetShowWhitespaces(false);
    codeEditor.SetText(tempcontent.getText());

    codeEditors.insert( pair<string,TextEditor>(actualEditedFileName,codeEditor) );

    codeWatchers.insert( pair<string,PathWatcher*>(actualEditedFileName,new PathWatcher()) );
    codeWatchers[actualEditedFileName]->start();
    codeWatchers[actualEditedFileName]->removeAllPaths();
    codeWatchers[actualEditedFileName]->addPath(actualEditedFilePath);

    actualCodeEditor = codeEditors.size()-1;
}

//--------------------------------------------------------------
void ofApp::removeScriptFromCodeEditor(string filename){

    for(int i=0;i<editedFilesNames.size();i++){
        if(editedFilesNames[i] == filename){
            editedFilesPaths.erase(editedFilesPaths.begin()+i);
            editedFilesNames.erase(editedFilesNames.begin()+i);
        }
    }

    if(editedFilesPaths.size() == 0){
        actualEditedFilePath = "";
        actualEditedFileName = "";
    }else{
        actualEditedFilePath = editedFilesPaths[editedFilesPaths.size()-1];
        actualEditedFileName = editedFilesNames[editedFilesNames.size()-1];
    }

    codeEditors.erase(filename);
    codeWatchers.erase(filename);

    if(codeEditors.size() == 0){
        actualCodeEditor = 0;
    }else{
        actualCodeEditor = codeEditors.size()-1;
    }

    scriptToRemoveFromCodeEditor = "";

}
