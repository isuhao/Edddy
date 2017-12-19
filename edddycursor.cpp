/* Edddy
// Copyright (C) 2016 LucKey Productions (luckeyproductions.nl)
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
// Commercial licenses are available through frode@lindeijer.nl
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "edddycam.h"
#include "resourcemaster.h"
#include "inputmaster.h"
#include "effectmaster.h"
#include "castmaster.h"
#include "editmaster.h"
#include "blockmap.h"

#include "edddycursor.h"

void EdddyCursor::RegisterObject(Context *context)
{
    context->RegisterFactory<EdddyCursor>();
}

EdddyCursor::EdddyCursor(Context* context) : LogicComponent(context),
    coords_{},
    targetRotation_{},
    axisLock_{},
    previousAxisLock_{}
{
    axisLock_.flip();
    axisLock_[1] = false;

    previousAxisLock_ = axisLock_;
    previousAxisLock_.flip();
}

void EdddyCursor::OnNodeSet(Node *node)
{ if (!node) return;

    blockNode_ = node_->CreateChild("PREVIEW");

    RigidBody* rigidBody_{ node_->CreateComponent<RigidBody>() };
    rigidBody_->SetKinematic(true);

    for (int c{0}; c < 3; ++c){

        CollisionShape* hitPlane{ node_->CreateComponent<CollisionShape>() };
        hitPlane->SetBox(Vector3(c == 0 ? 0.0f : 1000.0f,
                                 c == 1 ? 0.0f : 1000.0f,
                                 c == 2 ? 0.0f : 1000.0f));
    }

    boxNode_ = node_->CreateChild("BOX");
    boxModel_ = boxNode_->CreateComponent<StaticModel>();
    boxModel_->SetModel(GetSubsystem<ResourceMaster>()->GetModel("Cursor"));
    boxModel_->SetMaterial(MAT_GLOWWIRE);

    for (bool wire : {true, false}){
        StaticModel* blockModel{ blockNode_->CreateComponent<StaticModel>() };

        if (wire) {
            blockModels_.first_ = blockModel;
        } else {
            blockModels_.second_ = blockModel;
        }
    }

    SubscribeToEvent(E_CURRENTBLOCKCHANGE, URHO3D_HANDLER(EdddyCursor, UpdateModel));
    SubscribeToEvent(E_CURRENTMAPCHANGE, URHO3D_HANDLER(EdddyCursor, HandleMapChange));
}
void EdddyCursor::DelayedStart()
{
//    MoveTo(GetSubsystem<EditMaster>()->GetCurrentBlockMap()->GetCenter());
//    SendEvent(E_CURSORSTEP);
}
void EdddyCursor::HandleMapChange(StringHash eventType, VariantMap& eventData)
{ (void)eventType;

    BlockMap* blockMap{ static_cast<BlockMap*>(eventData[CurrentMapChange::P_MAP].GetPtr()) };

    if (blockMap) {

        UpdateSizeAndOffset();
        MoveTo(blockMap->GetCenter());
        node_->SetEnabled(true);

    } else {

        node_->SetEnabled(false);
    }
}
void EdddyCursor::UpdateModel(StringHash eventType, VariantMap& eventData)
{ (void)eventType;

    Block* currentBlock{ static_cast<Block*>(eventData[CurrentBlockChange::P_BLOCK].GetPtr()) };

    if (currentBlock) {

        Model* model{ currentBlock->GetModel() };
        if (model) {

            blockModels_.first_->SetModel(model);
            blockModels_.first_->SetMaterial(MAT_GLOWWIRE);
            blockModels_.second_->SetModel(model);
            blockModels_.second_->SetMaterial(MAT_TRANSPARENTGLOW);
            if (boxNode_->IsEnabled())
                boxNode_->SetEnabled(false);
        }

    } else {

        blockModels_.first_->SetModel(nullptr);
        blockModels_.second_->SetModel(nullptr);

        if (!boxNode_->IsEnabled())
            boxNode_->SetEnabled(true);
    }
}

void EdddyCursor::Hide()
{
    hidden_ = true;
    boxNode_->SetEnabled(false);
    blockNode_->SetEnabled(false);

    SendEvent(E_CURSORSTEP);
}
void EdddyCursor::Show()
{
    hidden_ = false;
    blockNode_->SetEnabled(true);
    if (blockModels_.first_->GetModel()) {
        boxNode_->SetEnabled(false);
    } else {
        boxNode_->SetEnabled(true);
    }
}
void EdddyCursor::ToggleVisibility()
{
    hidden_ ? Show()
            : Hide();
}

void EdddyCursor::UpdateSizeAndOffset()
{
    BlockMap* currentMap{ GetSubsystem<EditMaster>()->GetCurrentBlockMap() };
    Vector3 blockSize{ currentMap ? currentMap->GetBlockSize() : Vector3::ONE };

    boxNode_->SetScale(Vector3(blockSize.x_, blockSize.y_, blockSize.z_));
//    blockNode_->SetPosition(Vector3::DOWN * blockSize.y_ * 0.5f);
}

void EdddyCursor::SetAxisLock(std::bitset<3> lock)
{
    if (lock.all() || lock.none())
        return;

    if (lock != axisLock_){
        previousAxisLock_ = axisLock_;
        axisLock_ = lock;
        SendEvent(E_CURSORSTEP);
    }
}

void EdddyCursor::Step(IntVector3 step)
{
    BlockMap* currentMap{ GetSubsystem<EditMaster>()->GetCurrentBlockMap() };
    IntVector3 mapSize{ currentMap ? currentMap->GetMapSize() : IntVector3::ONE };

    if (axisLock_.count() == 2){
        switch (axisLock_.to_ulong()) {
        case 5: step = IntVector3(step.x_, step.z_, step.y_);
            break;
        case 3: step = IntVector3(step.x_, step.y_, step.z_);
            break;
        case 6: step = IntVector3(step.z_, step.y_, step.x_);
            break;
        }
    } else {
        switch (axisLock_.to_ulong()) {
        case 1: step = IntVector3(step.x_ + step.y_ + step.z_, 0, 0);
            break;
        case 2: step = IntVector3(0, step.x_ + step.y_ + step.z_, 0);
            break;
        case 4: step = IntVector3(0, 0, step.x_ + step.y_ + step.z_);
            break;
        }
    }


    IntVector3 resultingCoords{ coords_ + step };
    if (resultingCoords.x_ < 0
     || resultingCoords.y_ < 0
     || resultingCoords.z_ < 0
     || resultingCoords.x_ >= mapSize.x_
     || resultingCoords.y_ >= mapSize.y_
     || resultingCoords.z_ >= mapSize.z_)
    {
        return;
    }

    SetCoords(resultingCoords);
}

bool EdddyCursor::CoordsOnMap(IntVector3 coords)
{
    BlockMap* currentMap{ GetSubsystem<EditMaster>()->GetCurrentBlockMap() };
    IntVector3 mapSize{ currentMap ? currentMap->GetMapSize() : IntVector3::ONE };

    if ( (coords.x_ < 0 || coords.x_ >= mapSize.x_)
      || (coords.y_ < 0 || coords.y_ >= mapSize.y_)
      || (coords.z_ < 0 || coords.z_ >= mapSize.z_))
    {
        return false;
    }
    else
    {
        return true;
    }

}

void EdddyCursor::SetCoords(IntVector3 coords)
{
    BlockMap* currentMap{ GetSubsystem<EditMaster>()->GetCurrentBlockMap() };
    Vector3 blockSize{ currentMap ? GetSubsystem<EditMaster>()->GetCurrentBlockMap()->GetBlockSize() : Vector3::ONE };

    if (!CoordsOnMap(coords)) {

        if (!hidden_) {
            Hide();
        }
        return;

    } else if (coords == coords_ && !hidden_) {

        return;
    }

    if (hidden_)
        Show();

    coords_ = coords;

    GetSubsystem<EffectMaster>()->TranslateTo(node_, Vector3{
                                coords.x_ * blockSize.x_,
                                coords.y_ * blockSize.y_,
                                coords.z_ * blockSize.z_}, 0.13f);
    SendEvent(E_CURSORSTEP);
}

void EdddyCursor::MoveTo(Vector3 position)
{
    BlockMap* currentMap{ GetSubsystem<EditMaster>()->GetCurrentBlockMap() };
    Vector3 blockSize{ currentMap ? currentMap->GetBlockSize() : Vector3::ONE };

    //if (grid)
    IntVector3 coords{ static_cast<int>(round(position.x_ / blockSize.x_)),
                       static_cast<int>(round(position.y_ / blockSize.y_)),
                       static_cast<int>(round(position.z_ / blockSize.z_)) };

    SetCoords(IntVector3(
            axisLock_[0] ? coords.x_ : coords_.x_,
            axisLock_[1] ? coords.y_ : coords_.y_,
            axisLock_[2] ? coords.z_ : coords_.z_));
}

void EdddyCursor::Rotate(bool clockWise)
{
    Vector3 axis{ Vector3::UP };
    targetRotation_ = Quaternion(clockWise ? 90.0f :  -90.0f, axis) * targetRotation_;
    GetSubsystem<EffectMaster>()->RotateTo(node_, targetRotation_, 0.1f);
}
void EdddyCursor::SetRotation(Quaternion rot)
{
//    node_->RemoveAttributeAnimation("Position");
    node_->SetRotation(rot);
    targetRotation_ = rot;
}

void EdddyCursor::HandleMouseMove()
{
    Ray mouseRay{ GetSubsystem<InputMaster>()->GetMouseRay() };

    PODVector<PhysicsRaycastResult> hitResults{};

    if (GetSubsystem<CastMaster>()->PhysicsRayCast(hitResults, mouseRay, M_LARGE_VALUE, M_MAX_UNSIGNED)){

        float closest{ M_INFINITY };
        PhysicsRaycastResult closestResult{};

        for (PhysicsRaycastResult result: hitResults)
            if (((((axisLock_.count() == 2) != (axisLock_[1] != axisLock_[2])) && Abs(result.normal_.x_) > 0.5f)
              || (((axisLock_.count() == 2) != (axisLock_[0] != axisLock_[2])) && Abs(result.normal_.y_) > 0.5f)
              || (((axisLock_.count() == 2) != (axisLock_[0] != axisLock_[1])) && Abs(result.normal_.z_) > 0.5f))
              && result.distance_ < closest)
            {
                closest = result.distance_;
                closestResult = result;
            }
        if (closestResult.body_)
            MoveTo(closestResult.position_);
    }
}
