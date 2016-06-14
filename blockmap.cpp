#include "gridblock.h"
#include "blockmap.h"

namespace Urho3D {

    template <> unsigned MakeHash(const IntVector2& value)
    {
        return LucKey::IntVector2ToHash(value);
    }
}


void BlockMap::RegisterObject(Context *context){ context->RegisterFactory<BlockMap>(); }

BlockMap::BlockMap(Context* context) : LogicComponent(context),
    map_{}
{
}
void BlockMap::OnNodeSet(Node *node)
{ (void)node;

    for (int y{0}; y < MAP_HEIGHT; ++y){
        Sheet sheet{};
        for (int x{0}; x < MAP_WIDTH; ++x)
            for (int z{0}; z < MAP_DEPTH; ++z){
                Node* gridNode_{ node_->CreateChild("GridBlock") };
                GridBlock* gridBlock{ gridNode_->CreateComponent<GridBlock>() };
                gridBlock->Init(IntVector3(x, y, z));

                IntVector2 sheetCoords(x, z);
                sheet[sheetCoords] = gridBlock;
            }
    }
}

Block* BlockMap::GetBlock(IntVector3 coords)
{
    Block* block{ nullptr };
    Sheet sheet{};

    if (map_.TryGetValue(coords.y_, sheet)){
        GridBlock* gridBlock{ nullptr };
        if (sheet.TryGetValue(IntVector2(coords.x_, coords.z_), gridBlock))
            block = gridBlock->block_;
    }

    return block;
}

void BlockMap::SetBlock(IntVector3 coords, Block* block)
{
    Sheet sheet{};
    if (map_.TryGetValue(coords.y_, sheet)){
        GridBlock* gridBlock{ nullptr };
        if (sheet.TryGetValue(IntVector2(coords.x_, coords.z_), gridBlock))
            gridBlock->block_ = block;
    }
}


