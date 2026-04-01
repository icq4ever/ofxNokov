#include "ofxNokov.h"

ofxNokov::ofxNokov()
    : client(nullptr)
    , scaleFactor(1.0f)
    , connected(false)
    , need_to_update_descs(true)
    , frame_number(0)
    , latency(0)
    , _frame_number(0)
    , _latency(0)
    , _last_packet_time(0)
{
    transform.makeIdentityMatrix();
}

ofxNokov::~ofxNokov()
{
    dispose();
}

void ofxNokov::setup(string serverAddress)
{
    dispose();

    client = new NokovSDKClientBase();

    // set callbacks before Initialize
    client->SetDataCallback(dataCallback, this);
    client->SetMessageCallback(messageCallback);

    char addr[256];
    strncpy(addr, serverAddress.c_str(), sizeof(addr) - 1);
    addr[sizeof(addr) - 1] = '\0';

    int ret = client->Initialize(addr);
    if (ret == 0) {
        connected = true;
        ofLogNotice("ofxNokov") << "Connected to server: " << serverAddress;

        // request data descriptions to get rigid body names
        sendRequestDescription();
    } else {
        connected = false;
        ofLogError("ofxNokov") << "Failed to connect to server: " << serverAddress << " (error: " << ret << ")";
    }
}

void ofxNokov::dispose()
{
    if (client) {
        client->Uninitialize();
        delete client;
        client = nullptr;
    }
    connected = false;

    markers.clear();
    rigidbodies.clear();
    rigidbodies_arr.clear();
    skeletons.clear();
    skeletons_arr.clear();
    rigidbody_descs.clear();
    skeleton_descs.clear();
    markerset_descs.clear();
    name_to_stream_id.clear();
    stream_id_to_name.clear();
}

void ofxNokov::update()
{
    if (!client) {
        ofLogError("ofxNokov") << "call setup() first";
        return;
    }

    std::lock_guard<std::mutex> lock(dataMutex);

    frame_number = _frame_number;
    latency = _latency;

    if (isConnected()) {
        markers = _markers;

        rigidbodies = _rigidbodies;
        rigidbodies_arr = _rigidbodies_arr;

        skeletons = _skeletons;
        skeletons_arr = _skeletons_arr;

        rigidbody_descs = _rigidbody_descs;
        skeleton_descs = _skeleton_descs;
        markerset_descs = _markerset_descs;

        name_to_stream_id = _name_to_stream_id;
        stream_id_to_name.clear();
        for (auto& p : name_to_stream_id) {
            stream_id_to_name[p.second] = p.first;
        }
    } else {
        markers.clear();
        rigidbodies.clear();
        rigidbodies_arr.clear();
        skeletons.clear();
        skeletons_arr.clear();
    }
}

bool ofxNokov::isConnected() const
{
    if (!client) return false;
    if (!connected) return false;
    // consider disconnected if no data for 1 second
    float elapsed = ofGetElapsedTimef() - _last_packet_time;
    return (_last_packet_time > 0) ? (elapsed < 1.0f) : connected.load();
}

void ofxNokov::setScale(float v)
{
    scaleFactor = v;
    transform = ofMatrix4x4::newScaleMatrix(v, v, v);
}

ofVec3f ofxNokov::getScale() const
{
    return transform.getScale();
}

void ofxNokov::setTransform(const ofMatrix4x4& m)
{
    transform = m;
}

const ofMatrix4x4& ofxNokov::getTransform() const
{
    return transform;
}

void ofxNokov::sendRequestDescription()
{
    if (!client) return;

    sDataDescriptions* pDataDescriptions = nullptr;
    int ret = client->GetDataDescriptions(&pDataDescriptions);
    if (ret == 0 && pDataDescriptions) {
        processDescriptions(pDataDescriptions);
        client->FreeDataDescriptions(pDataDescriptions);
        need_to_update_descs = false;
    } else {
        ofLogWarning("ofxNokov") << "Failed to get data descriptions";
    }
}

bool ofxNokov::needRequestDescription() const
{
    return need_to_update_descs;
}

// ---- Static Callbacks ----

void XINGYING_CALLCONV ofxNokov::dataCallback(sFrameOfMocapData* pFrameOfData, void* pUserData)
{
    ofxNokov* self = static_cast<ofxNokov*>(pUserData);
    if (self && pFrameOfData) {
        self->processFrame(pFrameOfData);
    }
}

void XINGYING_CALLCONV ofxNokov::messageCallback(int msgType, char* msg)
{
    if (msg) {
        switch (msgType) {
            case Verbosity_Error:
                ofLogError("ofxNokov") << msg;
                break;
            case Verbosity_Warning:
                ofLogWarning("ofxNokov") << msg;
                break;
            case Verbosity_Info:
                ofLogNotice("ofxNokov") << msg;
                break;
            default:
                ofLogVerbose("ofxNokov") << msg;
                break;
        }
    }
}

// ---- Data Processing ----

void ofxNokov::processFrame(sFrameOfMocapData* data)
{
    std::lock_guard<std::mutex> lock(dataMutex);

    _frame_number = data->iFrame;
    _latency = data->fLatency;
    _last_packet_time = ofGetElapsedTimef();

    ofQuaternion rotTransform = transform.getRotate();

    // -- Other Markers (unlabeled) --
    _markers.clear();
    if (data->OtherMarkers) {
        for (int i = 0; i < data->nOtherMarkers; i++) {
            ofVec3f p(data->OtherMarkers[i][0],
                      data->OtherMarkers[i][1],
                      data->OtherMarkers[i][2]);
            p = transform.preMult(p);
            _markers.push_back(p);
        }
    }

    // -- Labeled Markers --
    for (int i = 0; i < data->nLabeledMarkers; i++) {
        sMarker& m = data->LabeledMarkers[i];
        ofVec3f p(m.x, m.y, m.z);
        p = transform.preMult(p);
        _markers.push_back(p);
    }

    // -- Rigid Bodies --
    _rigidbodies_arr.clear();
    _rigidbodies_arr.resize(data->nRigidBodies);

    for (int i = 0; i < data->nRigidBodies; i++) {
        sRigidBodyData& rbData = data->RigidBodies[i];
        RigidBody& RB = _rigidbodies_arr[i];

        RB.id = rbData.ID;
        RB.raw_position = ofVec3f(rbData.x, rbData.y, rbData.z);

        // build matrix from position + quaternion
        ofVec3f pos(rbData.x, rbData.y, rbData.z);
        pos = transform.preMult(pos);

        ofQuaternion q(rbData.qx, rbData.qy, rbData.qz, rbData.qw);

        ofMatrix4x4 mat;
        mat.setTranslation(pos);
        mat.setRotate(q * rotTransform);
        RB.matrix = mat;

        // mean marker error
        RB.mean_marker_error = rbData.MeanError;

        // active status: tracking valid when params bit 0x01 is set,
        // fallback to mean_marker_error > 0
        if (rbData.params != 0) {
            RB._active = (rbData.params & 0x01) != 0;
        } else {
            RB._active = rbData.MeanError > 0;
        }

        // markers associated with this rigid body
        RB.markers.clear();
        if (rbData.Markers) {
            for (int j = 0; j < rbData.nMarkers; j++) {
                ofVec3f mp(rbData.Markers[j][0],
                           rbData.Markers[j][1],
                           rbData.Markers[j][2]);
                mp = transform.preMult(mp);
                RB.markers.push_back(mp);
            }
        }

        // assign name from description map
        auto nameIt = _name_to_stream_id.begin();
        for (; nameIt != _name_to_stream_id.end(); ++nameIt) {
            if (nameIt->second == rbData.ID) {
                RB.name = nameIt->first;
                break;
            }
        }
        if (nameIt == _name_to_stream_id.end()) {
            RB.name = "RigidBody_" + ofToString(rbData.ID);
            need_to_update_descs = true;
        }

        // update map
        _rigidbodies[RB.id] = RB;
    }

    // -- Skeletons --
    _skeletons_arr.clear();
    _skeletons_arr.resize(data->nSkeletons);

    for (int i = 0; i < data->nSkeletons; i++) {
        sSkeletonData& skelData = data->Skeletons[i];
        Skeleton& S = _skeletons_arr[i];
        S.id = skelData.skeletonID;
        S.joints.resize(skelData.nRigidBodies);

        for (int j = 0; j < skelData.nRigidBodies; j++) {
            sRigidBodyData& jData = skelData.RigidBodyData[j];
            RigidBody& joint = S.joints[j];

            joint.id = jData.ID;
            joint.raw_position = ofVec3f(jData.x, jData.y, jData.z);

            ofVec3f pos(jData.x, jData.y, jData.z);
            pos = transform.preMult(pos);

            ofQuaternion q(jData.qx, jData.qy, jData.qz, jData.qw);

            ofMatrix4x4 mat;
            mat.setTranslation(pos);
            mat.setRotate(q * rotTransform);
            joint.matrix = mat;

            joint.mean_marker_error = jData.MeanError;
            joint._active = jData.MeanError > 0;
        }

        _skeletons[S.id] = S;
    }
}

void ofxNokov::processDescriptions(sDataDescriptions* pData)
{
    std::lock_guard<std::mutex> lock(dataMutex);

    _rigidbody_descs.clear();
    _skeleton_descs.clear();
    _markerset_descs.clear();
    _name_to_stream_id.clear();

    for (int i = 0; i < pData->nDataDescriptions; i++) {
        sDataDescription& desc = pData->arrDataDescriptions[i];

        if (desc.type == Descriptor_MarkerSet) {
            sMarkerSetDescription* ms = desc.Data.MarkerSetDescription;
            MarkerSetDescription msd;
            msd.name = ms->szName;
            for (int j = 0; j < ms->nMarkers; j++) {
                if (ms->szMarkerNames && ms->szMarkerNames[j]) {
                    msd.marker_names.push_back(ms->szMarkerNames[j]);
                }
            }
            _markerset_descs.push_back(msd);

        } else if (desc.type == Descriptor_RigidBody) {
            sRigidBodyDescription* rb = desc.Data.RigidBodyDescription;
            RigidBodyDescription rbd;
            rbd.name = rb->szName;
            rbd.id = rb->ID;
            rbd.parent_id = rb->parentID;
            rbd.offset = ofVec3f(rb->offsetx, rb->offsety, rb->offsetz);
            _rigidbody_descs.push_back(rbd);

            _name_to_stream_id[rbd.name] = rbd.id;

        } else if (desc.type == Descriptor_Skeleton) {
            sSkeletonDescription* sk = desc.Data.SkeletonDescription;
            SkeletonDescription skd;
            skd.name = sk->szName;
            skd.id = sk->skeletonID;
            skd.joints.resize(sk->nRigidBodies);
            for (int j = 0; j < sk->nRigidBodies; j++) {
                sRigidBodyDescription& jDesc = sk->RigidBodies[j];
                skd.joints[j].name = jDesc.szName;
                skd.joints[j].id = jDesc.ID;
                skd.joints[j].parent_id = jDesc.parentID;
                skd.joints[j].offset = ofVec3f(jDesc.offsetx, jDesc.offsety, jDesc.offsetz);
            }
            _skeleton_descs.push_back(skd);
        }
    }

    ofLogNotice("ofxNokov") << "Descriptions loaded: "
        << _markerset_descs.size() << " MarkerSets, "
        << _rigidbody_descs.size() << " RigidBodies, "
        << _skeleton_descs.size() << " Skeletons";
}

// ---- Debug Drawing ----

void ofxNokov::debugDrawMarkers()
{
    ofPushStyle();
    ofFill();

    // draw all markers
    ofSetColor(255, 30);
    for (size_t i = 0; i < getNumMarker(); i++) {
        ofDrawBox(getMarker(i), 3);
    }

    ofNoFill();

    // draw rigidbodies
    for (size_t i = 0; i < getNumRigidBody(); i++) {
        const RigidBody& RB = getRigidBodyAt(i);

        if (RB.isActive())
            ofSetColor(0, 255, 0);
        else
            ofSetColor(255, 0, 0);

        ofPushMatrix();
        glMultMatrixf(RB.getMatrix().getPtr());
        ofDrawAxis(30);
        ofPopMatrix();

        glBegin(GL_LINE_LOOP);
        for (size_t n = 0; n < RB.markers.size(); n++) {
            glVertex3fv(RB.markers[n].getPtr());
        }
        glEnd();

        for (size_t n = 0; n < RB.markers.size(); n++) {
            ofDrawBox(RB.markers[n], 5);
        }
    }

    // draw skeletons
    for (size_t j = 0; j < getNumSkeleton(); j++) {
        const Skeleton& S = getSkeletonAt(j);
        ofSetColor(255, 0, 255);

        for (size_t i = 0; i < S.joints.size(); i++) {
            const RigidBody& RB = S.joints[i];
            ofPushMatrix();
            glMultMatrixf(RB.getMatrix().getPtr());
            ofDrawBox(5);
            ofPopMatrix();
        }
    }

    ofPopStyle();
}

void ofxNokov::debugDrawInformation()
{
    ofPushStyle();

    string str;
    str += "frames: " + ofToString(getFrameNumber()) + "\n";
    str += string("connected: ") + (isConnected() ? "YES" : "NO") + "\n";
    str += "num marker: " + ofToString(getNumMarker()) + "\n";
    str += "num rigidbody: " + ofToString(getNumRigidBody()) + "\n";
    str += "num skeleton: " + ofToString(getNumSkeleton()) + "\n\n";

    if (getNumRigidBody() > 0) {
        str += "Active Rigidbody: \n";
        for (size_t i = 0; i < getNumRigidBody(); ++i) {
            auto&& rb = getRigidBodyAt(i);
            if (rb.isActive()) {
                str += rb.name + "\n";
            }
        }
        str += "\n";
    }

    if (markerset_descs.size() || rigidbody_descs.size() || skeleton_descs.size()) {
        str += "Description: \n";
        for (auto& desc : markerset_descs) { str += "MarkerSet: " + desc.name + "\n"; }
        for (auto& desc : rigidbody_descs) { str += "RigidBody: " + desc.name + "\n"; }
        for (auto& desc : skeleton_descs) { str += "Skeleton: " + desc.name + "\n"; }
    }

    ofDrawBitmapStringHighlight(str, 10, 20, ofColor(40), ofColor(255));

    ofPopStyle();
}

void ofxNokov::debugDraw()
{
    debugDrawMarkers();
    debugDrawInformation();
}
