# ofxNokov

openFrameworks addon for [Nokov (XING)](http://www.nokov.com/) motion capture system.

This addon wraps the Nokov XING SDK and provides an API compatible with [ofxNatNet](https://github.com/satoruhiga/ofxNatNet), making it easy to migrate projects from OptiTrack NatNet to Nokov.

## Supported Platforms

| Platform | Architecture | Library |
|---|---|---|
| Linux (PC) | x86_64 | `libnokov_sdk.so` |
| Linux (RPi 32bit) | armv6l (Linaro) | `libnokov_sdk.so` |
| Linux (RPi 64bit) | aarch64 | `libnokov_sdk.so` |
| macOS | x86_64 / arm64 | `libnokov_sdk.dylib` |
| Windows | x86_64 | `nokov_sdk.dll` |

## Dependencies

- openFrameworks (0.11.x+)
- ofxOsc (bundled with oF)

For the example app (nokov2OSC):
- ofxPoco
- ofxWatcher

## Installation

1. Clone or copy this repository into your openFrameworks `addons/` directory:
   ```
   cd /path/to/openFrameworks/addons/
   git clone https://github.com/icq4ever/ofxNokov.git
   ```

2. Add `ofxNokov` to your project's `addons.make` file.

## Quick Start

```cpp
#include "ofxNokov.h"

ofxNokov nokov;

void ofApp::setup() {
    nokov.setup("192.168.1.100");  // Nokov server IP address
    nokov.setScale(100);
}

void ofApp::update() {
    nokov.update();

    for (int i = 0; i < nokov.getNumRigidBody(); i++) {
        const ofxNokov::RigidBody& rb = nokov.getRigidBodyAt(i);
        if (rb.isActive()) {
            ofLog() << rb.name << " pos: " << rb.getMatrix().getTranslation();
        }
    }
}

void ofApp::draw() {
    nokov.debugDraw();
}
```

## API Reference

### Setup & Connection

| Method | Description |
|---|---|
| `setup(string serverAddress)` | Connect to Nokov server |
| `update()` | Fetch latest frame data (call every frame) |
| `dispose()` | Disconnect and clean up |
| `isConnected()` | Check connection status |
| `sendRequestDescription()` | Request rigid body / skeleton descriptions |
| `needRequestDescription()` | Check if descriptions need refreshing |

### Rigid Bodies

| Method | Description |
|---|---|
| `getNumRigidBody()` | Number of rigid bodies in current frame |
| `getRigidBodyAt(int index)` | Get rigid body by index |
| `hasRigidBody(int id)` | Check if rigid body with ID exists |
| `getRigidBody(int id, RigidBody& rb)` | Get rigid body by ID |
| `hasRigidBodyByName(string name)` | Check if rigid body with name exists |
| `getRigidBodyByName(string name, RigidBody& rb)` | Get rigid body by name |

### RigidBody Properties

| Property / Method | Type | Description |
|---|---|---|
| `id` | `int` | Unique rigid body ID |
| `name` | `string` | Rigid body name (from Nokov software) |
| `matrix` | `ofMatrix4x4` | 4x4 transformation matrix |
| `markers` | `vector<ofVec3f>` | Associated marker positions |
| `mean_marker_error` | `float` | Tracking error |
| `isActive()` | `bool` | Whether currently tracked |
| `getMatrix()` | `const ofMatrix4x4&` | Get transformation matrix |

### Skeletons

| Method | Description |
|---|---|
| `getNumSkeleton()` | Number of skeletons |
| `getSkeletonAt(int index)` | Get skeleton by index |
| `hasSkeleton(int id)` | Check if skeleton with ID exists |
| `getSkeleton(int id, Skeleton& s)` | Get skeleton by ID |

### Markers

| Method | Description |
|---|---|
| `getNumMarker()` | Number of markers (labeled + unlabeled) |
| `getMarker(size_t index)` | Get marker position |

### Transform

| Method | Description |
|---|---|
| `setScale(float v)` | Set scale factor for all positions |
| `getScale()` | Get current scale |
| `setTransform(const ofMatrix4x4& m)` | Set custom transform matrix |
| `getTransform()` | Get current transform matrix |

### Debug Drawing

| Method | Description |
|---|---|
| `debugDraw()` | Draw markers + info overlay |
| `debugDrawMarkers()` | Draw markers and rigid bodies in 3D |
| `debugDrawInformation()` | Draw text info overlay |

## Example: nokov2OSC

The included `example-nokov2OSC` app receives rigid body data from Nokov and sends it via OSC. This is a direct equivalent of [natnet2OSC](https://github.com/icq4ever/natnet2OSC).

### OSC Message Format

All messages follow the pattern: `/rigidbody/{name}/{method}`

```
/rigidbody/myBody/location2d    float(x) float(y)
/rigidbody/myBody/audioLocation2d float(x) float(y)
/rigidbody/myBody/location3d    float(x) float(y) float(z)
/rigidbody/myBody/height        float(z)
/rigidbody/myBody/orientation   float(w) float(x) float(y) float(z)
/rigidbody/myBody/direction     float(degrees)
/rigidbody/myBody/eye2d         float(x) float(y)
/rigidbody/myBody/eye3d         float(x) float(y) float(z)
/rigidbody/myBody/pitch         float(degrees)
/rigidbody/myBody/eulerAngles   float(roll) float(pitch) float(yaw)
/rigidbody/myBody/matrix        float(m0) ... float(m15)
```

Default OSC destination: `127.0.0.1:11111`

### Controls

- **Enter** : Reconnect to Nokov server
- **F** : Toggle fullscreen

## ofxNatNet Migration Guide

Migrating from `ofxNatNet` to `ofxNokov` requires minimal changes:

```cpp
// Before (ofxNatNet)
ofxNatNet natnet;
natnet.setup("127.0.0.1", "192.168.1.100");
natnet.setScale(100);
natnet.setDuplicatedPointRemovalDistance(20);
natnet.update();
const ofxNatNet::RigidBody& rb = natnet.getRigidBodyAt(i);

// After (ofxNokov)
ofxNokov nokov;
nokov.setup("192.168.1.100");  // server address only
nokov.setScale(100);
// no duplicated point removal needed (SDK handles it)
nokov.update();
const ofxNokov::RigidBody& rb = nokov.getRigidBodyAt(i);
```

The `RigidBody` class interface is identical: `id`, `name`, `matrix`, `markers`, `isActive()`, `getMatrix()`.

## SDK Version

Based on Nokov XING SDK v4.1.0.5634

## License

This addon is provided under the same license as openFrameworks.
The Nokov SDK libraries are proprietary and subject to Nokov's licensing terms.
