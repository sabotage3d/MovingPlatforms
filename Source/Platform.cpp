#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Core/Context.h>
#include <iostream>


#define PI 3.14159264f
inline float Sinerp(float min, float max, float weight)
{
    return min + (max - min) * std::sin(weight * PI * 0.5f);
}

#include "Platform.h"

#include <Urho3D/DebugNew.h>

Platform::Platform(Context* context) :
LogicComponent(context)
{
    // Only the scene update event is needed: unsubscribe from the rest for optimization
    SetUpdateEventMask(USE_UPDATE);
}

void Platform::Start()
{
  SubscribeToEvent(node_, E_NODECOLLISION, HANDLER(Platform, HandleNodeCollision));
    
    direction_ = node_->GetPosition();
}

void Platform::HandleNodeCollision(StringHash eventType, VariantMap& eventData)
{
    using namespace NodeCollision;
    
    RigidBody* otherBody = (RigidBody*)eventData[P_OTHERBODY].GetPtr();
    
    
    Node* otherNode = (Node*)eventData[P_OTHERNODE].GetPtr();
}


void Platform::SetId(int id)
{
    id_ = id;
}

void Platform::Update(float timeStep)
{
    Time* time= GetSubsystem<Time>();
 
    
    elapsedTime_ += timeStep/5.0;
    
    elapsedTime_ += timeStep/(float)id_;
    
    float cycle = 0;
    
    if(id_%2)
    {
        cycle = (std::sin(elapsedTime_ )/100.0f)*10.0f;
    }
    else
    {
        cycle = (std::cos(elapsedTime_ )/100.0f)*7.0f;
    }
    
    direction_ += Vector3(cycle,0.0,0.0);



    node_->SetPosition(direction_);
}