#ifndef SDL2_PROPERTIES_H
#define SDL2_PROPERTIES_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>

using SDL_PropertiesID = uint32_t;

class SDLPropertiesManager {
public:
    // Get the unique ID for a string key. Registers it if new.
    static SDL_PropertiesID GetPropertyID(const std::string& key) {
        static std::unordered_map<std::string, SDL_PropertiesID> keyToID;
        static SDL_PropertiesID nextID = 1;
        static std::mutex mutex;

        std::lock_guard<std::mutex> lock(mutex);
        auto it = keyToID.find(key);
        if (it != keyToID.end()) {
            return it->second;
        }
        SDL_PropertiesID id = nextID++;
        keyToID[key] = id;
        return id;
    }
};

// A simple properties container: maps SDL_PropertiesID to std::string values.
class SDLProperties {
    std::unordered_map<SDL_PropertiesID, std::string> properties;

public:
    void Set(SDL_PropertiesID id, const std::string& value) {
        properties[id] = value;
    }

    bool Get(SDL_PropertiesID id, std::string& outValue) const {
        auto it = properties.find(id);
        if (it != properties.end()) {
            outValue = it->second;
            return true;
        }
        return false;
    }

    void Remove(SDL_PropertiesID id) {
        properties.erase(id);
    }

    void Clear() {
        properties.clear();
    }
};

#endif // SDL2_PROPERTIES_H