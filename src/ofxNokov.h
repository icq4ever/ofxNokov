#pragma once

#include "ofMain.h"
#include "NokovSDKWrapper.h"

#include <mutex>
#include <map>
#include <vector>
#include <atomic>

class ofxNokov
{
public:
    typedef ofVec3f Marker;

    class RigidBody
    {
        friend class ofxNokov;

    public:
        int id;
        ofMatrix4x4 matrix;
        vector<Marker> markers;
        float mean_marker_error;
        string name;

        inline bool isActive() const { return _active; }
        const ofMatrix4x4& getMatrix() const { return matrix; }

    private:
        bool _active;
        ofVec3f raw_position;
    };

    class Skeleton
    {
    public:
        int id;
        vector<RigidBody> joints;
    };

    class RigidBodyDescription
    {
    public:
        string name;
        int id;
        int parent_id;
        ofVec3f offset;
    };

    class SkeletonDescription
    {
    public:
        string name;
        int id;
        vector<RigidBodyDescription> joints;
    };

    class MarkerSetDescription
    {
    public:
        string name;
        vector<string> marker_names;
    };

    ofxNokov();
    ~ofxNokov();

    void setup(string serverAddress);
    void update();
    void dispose();

    bool isConnected() const;
    int getFrameNumber() const { return frame_number; }
    float getLatency() const { return latency; }

    void setScale(float v);
    ofVec3f getScale() const;

    void setTransform(const ofMatrix4x4& m);
    const ofMatrix4x4& getTransform() const;

    inline const size_t getNumMarker() const { return markers.size(); }
    inline const Marker& getMarker(size_t index) const { return markers[index]; }

    inline const size_t getNumRigidBody() const { return rigidbodies_arr.size(); }
    inline const RigidBody& getRigidBodyAt(int index) const
    {
        return rigidbodies_arr[index];
    }

    inline const bool hasRigidBody(int id) const
    {
        return rigidbodies.find(id) != rigidbodies.end();
    }

    inline const bool getRigidBody(int id, RigidBody& RB) const
    {
        if (!hasRigidBody(id)) return false;
        RB = rigidbodies.at(id);
        return true;
    }

    inline const bool hasRigidBodyByName(string name) const
    {
        return name_to_stream_id.find(name) != name_to_stream_id.end()
            && rigidbodies.find(name_to_stream_id.at(name)) != rigidbodies.end();
    }

    inline const bool getRigidBodyByName(string name, RigidBody& RB) const
    {
        if (!hasRigidBodyByName(name)) return false;
        RB = rigidbodies.at(name_to_stream_id.at(name));
        return true;
    }

    inline const size_t getNumSkeleton() const { return skeletons_arr.size(); }
    inline const Skeleton& getSkeletonAt(int index) const
    {
        return skeletons_arr[index];
    }

    inline const bool hasSkeleton(int id) const
    {
        return skeletons.find(id) != skeletons.end();
    }

    inline const bool getSkeleton(int id, Skeleton& S) const
    {
        if (!hasSkeleton(id)) return false;
        S = skeletons.at(id);
        return true;
    }

    void sendRequestDescription();
    bool needRequestDescription() const;

    inline const vector<MarkerSetDescription> getMarkerSetDescriptions() const { return markerset_descs; }
    inline const vector<RigidBodyDescription> getRigidBodyDescriptions() const { return rigidbody_descs; }
    inline const vector<SkeletonDescription> getSkeletonDescriptions() const { return skeleton_descs; }

    void debugDraw();
    void debugDrawInformation();
    void debugDrawMarkers();

private:
    ofxNokov(const ofxNokov&);
    ofxNokov& operator=(const ofxNokov&);

    // Nokov SDK client
    NokovSDKClientBase* client;

    // transform
    ofMatrix4x4 transform;
    float scaleFactor;

    // connection state
    std::atomic<bool> connected;
    std::atomic<bool> need_to_update_descs;

    // frame data (main thread copy)
    int frame_number;
    float latency;

    vector<Marker> markers;
    map<int, RigidBody> rigidbodies;
    vector<RigidBody> rigidbodies_arr;
    map<int, Skeleton> skeletons;
    vector<Skeleton> skeletons_arr;

    vector<RigidBodyDescription> rigidbody_descs;
    vector<SkeletonDescription> skeleton_descs;
    vector<MarkerSetDescription> markerset_descs;

    map<string, int> name_to_stream_id;
    map<int, string> stream_id_to_name;

    // callback thread data (written from SDK callback)
    std::mutex dataMutex;

    int _frame_number;
    float _latency;

    vector<Marker> _markers;
    map<int, RigidBody> _rigidbodies;
    vector<RigidBody> _rigidbodies_arr;
    map<int, Skeleton> _skeletons;
    vector<Skeleton> _skeletons_arr;

    vector<RigidBodyDescription> _rigidbody_descs;
    vector<SkeletonDescription> _skeleton_descs;
    vector<MarkerSetDescription> _markerset_descs;

    map<string, int> _name_to_stream_id;

    float _last_packet_time;

    // static callbacks for Nokov SDK
    static void XINGYING_CALLCONV dataCallback(sFrameOfMocapData* pFrameOfData, void* pUserData);
    static void XINGYING_CALLCONV messageCallback(int msgType, char* msg);

    void processFrame(sFrameOfMocapData* pFrameOfData);
    void processDescriptions(sDataDescriptions* pDataDescriptions);
};
