#include <cstdint>
#include <optional>

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;

    bool isComplete() {
        return graphicsFamily.has_value();
    }
};

// Find the queue families supported by a given physical device
QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
