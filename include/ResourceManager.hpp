#pragma once
#include <unordered_map>
#include <string>
#include <memory>
#include <stdexcept>
#include <format>
#include <functional>

template <typename T>
class ResourceManager {
public:
    void add(std::string name, std::shared_ptr<T> resource) {
        resources[name] = resource;
    }

    void perform_action_on_all(std::function<void(T&)> func) {
        for (auto kp : resources) {
            func(*kp.second);
        }
    }

    std::shared_ptr<T> get(std::string name) {
        if (resources.count(name) == 0) {
            throw std::runtime_error(std::format("Specified resource '{}' does not exist.", name));
        } else {
            return resources.at(name);
        }
    }
private:
    std::unordered_map<std::string, std::shared_ptr<T>> resources;
};