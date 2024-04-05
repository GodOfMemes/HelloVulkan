#pragma once

#include "Graphics/GraphicsDevice.hpp"

// TODO: Make Buffer, Texture, Mesh and Pipeline inherit from this, and move to a more descriptive folder
class ResourceBase
{
    friend class GraphicsDevice;
public:
    ResourceBase(GraphicsDevice* gd, bool resetWhenSwapUpdates = false)
        : gd(gd), resetWhenSwapUpdates(resetWhenSwapUpdates) {}
    virtual ~ResourceBase() = default;

    virtual void Create() {}
    virtual void Destroy() = 0;
protected:
    GraphicsDevice* gd;
private:
    bool resetWhenSwapUpdates;
};