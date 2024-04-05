#pragma once

#include "Minimal.hpp"

struct FrameCounter
{
public:
    FrameCounter(size_t graphLength = 100, float graphDelay = 0.1f)
        : _fps(0),
        _delta(0),
        _delayedDeltaMs(0), 
        _timer(0), 
        _graphLength(graphLength), 
        _graphDelay(graphDelay) 
    {
        _data.resize(graphLength,0);
    }

    [[nodiscard]] float GetDeltaSecond() const { return _delta; }
    [[nodiscard]] float GetDelayedDeltaMillisecond() const { return _delayedDeltaMs; }
    [[nodiscard]] float GetCurrentFPS() const { return _fps; }
    [[nodiscard]] const float* GetGraph() const { return _data.data(); }
    [[nodiscard]]  size_t GetGraphLength() const { return _graphLength; }

	void Update(float currentFrame)
    {
        _delta = currentFrame;

        _timer += _delta;
        if (_timer >= _graphDelay && !_data.empty())
        {
            _fps = 1.0f / _delta;
            _delayedDeltaMs = _delta * 1000.f;
            std::ranges::rotate(_data.begin(), _data.begin() + 1, _data.end());
            _data[_graphLength - 1] = _fps;
            _timer = 0.f;
        }
    }
private:
    float _fps, _delta, _delayedDeltaMs, _timer;
    std::vector<float> _data{};

    size_t _graphLength;
    float _graphDelay;
};