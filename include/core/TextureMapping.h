#ifndef TEXTURE_MAPPING_H
#define TEXTURE_MAPPING_H

#include <vector>
#include <string>
#include <unordered_map>
#include <fstream>
#include <sstream>
#include <iostream>

// Texture Mapping Configuration Reader
// Reads texture ID to filename mapping from texture_mapping.txt

namespace TextureMapping {
    
    struct TextureInfo {
        int id;
        std::string filename;  // .h file name (e.g., "brick.h")
        std::string name;      // Display name (e.g., "Brick")
        
        TextureInfo() : id(0), filename(""), name("") {}
        TextureInfo(int i, const std::string& f, const std::string& n) 
            : id(i), filename(f), name(n) {}
    };
    
    class TextureConfig {
    private:
        std::unordered_map<int, TextureInfo> mapping;
        bool loaded;
        
    public:
        TextureConfig() : loaded(false) {}
        
        // Load texture mapping from configuration file
        bool loadFromFile(const std::string& configPath = "include/core/texture_mapping.txt") {
            std::ifstream file(configPath);
            if (!file.is_open()) {
                std::cerr << "Error: Cannot open texture mapping file: " << configPath << std::endl;
                return false;
            }
            
            mapping.clear();
            std::string line;
            int lineNum = 0;
            
            while (std::getline(file, line)) {
                lineNum++;
                
                line.erase(0, line.find_first_not_of(" \t\r\n"));
                line.erase(line.find_last_not_of(" \t\r\n") + 1);
                
                // Skip empty lines and comments
                if (line.empty() || line[0] == '#') {
                    continue;
                }
                
                // Parse line: ID:filename:name
                std::istringstream iss(line);
                std::string idStr, filename, name;
                
                if (std::getline(iss, idStr, ':') && 
                    std::getline(iss, filename, ':') && 
                    std::getline(iss, name)) {
                    
                    try {
                        int id = std::stoi(idStr);
                        
                        // Handle "null" filename for empty space
                        if (filename == "null") {
                            filename = "";
                        }
                        
                        mapping[id] = TextureInfo(id, filename, name);
                        
                    } catch (const std::exception& e) {
                        std::cerr << "Warning: Invalid texture ID on line " << lineNum 
                                  << ": " << idStr << std::endl;
                    }
                } else {
                    std::cerr << "Warning: Invalid format on line " << lineNum 
                              << ": " << line << std::endl;
                }
            }
            
            file.close();
            loaded = true;
            
            std::cout << "✓ Loaded " << mapping.size() << " texture mappings from " 
                      << configPath << std::endl;
            
            return true;
        }
        
        const TextureInfo& getTextureInfo(int id) const {
            static TextureInfo empty;
            auto it = mapping.find(id);
            if (it != mapping.end()) {
                return it->second;
            }
            return empty;
        }
        
        std::string getFilename(int id) const {
            const TextureInfo& info = getTextureInfo(id);
            return info.filename;
        }
        
        std::string getName(int id) const {
            const TextureInfo& info = getTextureInfo(id);
            return info.name.empty() ? "Unknown" : info.name;
        }
        
        // Check if texture ID exists
        bool hasTexture(int id) const {
            return mapping.find(id) != mapping.end();
        }
        
        // Get all texture IDs
        std::vector<int> getAllTextureIDs() const {
            std::vector<int> ids;
            for (const auto& pair : mapping) {
                ids.push_back(pair.first);
            }
            return ids;
        }
        
        // Check if configuration is loaded
        bool isLoaded() const {
            return loaded;
        }
    };
    
} // namespace TextureMapping

#endif // TEXTURE_MAPPING_H
