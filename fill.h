/* Edddy
// Copyright (C) 2017 LucKey Productions (luckeyproductions.nl)
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
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

#ifndef FILL_H
#define FILL_H

#include <Urho3D/Urho3D.h>
#include "tool.h"

class Block;

class Fill : public Tool
{
    URHO3D_OBJECT(Fill, Tool);
public:
    Fill(Context* context);
    void Apply(bool shiftDown, bool ctrlDown, bool altDown) override;
private:
    void Flood(IntVector3 coords, HashSet<IntVector3>& area, bool obeyLock);

    Block* targetBlock_;
    Block* replacementBlock_;
};

#endif // FILL_H
