#include "OSCSending.h"
#include "ofxWatcher.h"
#include "ofJson.h"

void OSCSending::setDestinationSettingsFile(std::filesystem::path &filepath)
{
	ofxWatchPath(filepath, ofLoadJson, [&](const ofJson &json) {
		std::vector<std::pair<const std::string, unsigned short>> destinations;
		for(int i = 0; i < json.size(); ++i) {
			auto dst = json[i];
			destinations.push_back(std::make_pair(dst[0].get<std::string>(), dst[1].get<int>()));
		}
		setupDestinations(destinations);
	});
}

void OSCSending::setupDestinations(std::vector<std::pair<const std::string, unsigned short>> dst)
{
	senders.clear();
	senders.reserve(dst.size());
	for(auto &&d : dst) {
		auto sender = std::make_shared<ofxOscSender>();
		sender->setup(d.first, d.second);
		senders.push_back(sender);
	}
}

void OSCSending::send(int ID, const RigidBodyInfo &rb, bool as_bundle)
{
	auto makeOscAddress = [](std::vector<std::string> address, std::string method) {
		std::stringstream ss;
		for(auto &&a : address) {
			ss << "/" << a;
		}
		ss << "/" << method;
		return ss.str();
	};
	ofxOscBundle bundle;
	auto procMessage = [&](const ofxOscMessage &msg) {
		if(as_bundle) {
			bundle.addMessage(msg);
		}
		else {
			for(auto &&s : senders) {
				s->sendMessage(msg, false);
			}
		}
	};
	const ofMatrix4x4 &matrix = rb.getMatrix();
	
	ofVec3f location = matrix.getTranslation();
	ofQuaternion orientation = matrix.getRotate();

	ofMatrix4x4 matT = rb.getMatrix();
	matT.rotate(-90, 0, 0, 1);
	ofVec3f audioLocation = matT.getTranslation();

	std::vector<std::string> address;
	address.push_back("rigidbody");
	//address.push_back(ofToString(ID));
	address.push_back(rb.getName());
	//{
	//	std::string method = makeOscAddress(address, "name");
	//	std::string data{rb.getName()};
	//	ofxOscMessage msg;
	//	msg.setAddress(method);
	//	msg.addStringArg(data);
	//	procMessage(msg);
	//}
	{
		std::string method = makeOscAddress(address, "location2d");
		ofVec2f data{location.x, location.y};
		ofxOscMessage msg;
		msg.setAddress(method);
		msg.addFloatArg(data.x);
		msg.addFloatArg(data.y);
		procMessage(msg);
	}
	{
		std::string method = makeOscAddress(address, "audioLocation2d");
		ofVec2f data{ audioLocation.x, audioLocation.y };
		ofxOscMessage msg;
		msg.setAddress(method);
		msg.addFloatArg(data.x);
		msg.addFloatArg(data.y);
		procMessage(msg);
	}
	{
		std::string method = makeOscAddress(address, "location3d");
		ofVec3f data{location.x, location.y, location.z};
		ofxOscMessage msg;
		msg.setAddress(method);
		msg.addFloatArg(data.x);
		msg.addFloatArg(data.y);
		msg.addFloatArg(data.z);
		procMessage(msg);
	}
	{
		std::string method = makeOscAddress(address, "height");
		float data{location.z};
		ofxOscMessage msg;
		msg.setAddress(method);
		msg.addFloatArg(data);
		procMessage(msg);
	}
	{
		std::string method = makeOscAddress(address, "orientation");
		ofxOscMessage msg;
		msg.setAddress(method);
		msg.addFloatArg(orientation[0]);
		msg.addFloatArg(orientation[1]);
		msg.addFloatArg(orientation[2]);
		msg.addFloatArg(orientation[3]);
		procMessage(msg);
	}
	// may need some other conversion from quaternion to angles
	ofVec3f axis = ofVec3f(0,0,1)*orientation;
	ofVec2f xy{axis.x, axis.y};
	{
		std::string method = makeOscAddress(address, "direction");
		float data = ofRadToDeg(atan2(-xy.y, -xy.x)) + 180;
		ofxOscMessage msg;
		msg.setAddress(method);
		msg.addFloatArg(data);
		procMessage(msg);
	}
	{
		std::string method = makeOscAddress(address, "eye2d");
		ofVec2f data = xy.getNormalized();
		ofxOscMessage msg;
		msg.setAddress(method);
		msg.addFloatArg(data.x);
		msg.addFloatArg(data.y);
		procMessage(msg);
	}
	{
		std::string method = makeOscAddress(address, "eye3d");
		ofVec3f data = axis;
		ofxOscMessage msg;
		msg.setAddress(method);
		msg.addFloatArg(data.x);
		msg.addFloatArg(data.y);
		msg.addFloatArg(data.z);
		procMessage(msg);
	}
	{
		std::string method = makeOscAddress(address, "pitch");
		float data = ofRadToDeg(atan2(axis.z, xy.length()));
		ofxOscMessage msg;
		msg.setAddress(method);
		msg.addFloatArg(data);
		procMessage(msg);
	}
	{
		std::string method = makeOscAddress(address, "eulerAngles");
		ofVec3f euler = orientation.getEuler();
		ofVec3f data{euler.x, euler.y, euler.z};
		ofxOscMessage msg;
		msg.setAddress(method);
		msg.addFloatArg(data.x);
		msg.addFloatArg(data.y);
		msg.addFloatArg(data.z);
		procMessage(msg);
	}
	{
		std::string method = makeOscAddress(address, "matrix");
		auto data = matrix.getPtr();
		ofxOscMessage msg;
		msg.setAddress(method);
		for(int i = 0; i < 16; ++i) {
			msg.addFloatArg(data[i]);
		}
		procMessage(msg);
	}
	if(as_bundle) {
		for(auto &&s : senders) {
			s->sendBundle(bundle);
		}
	}
}

