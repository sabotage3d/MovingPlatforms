//
// Copyright (c) 2008-2015 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

#include <Urho3D/Core/Context.h>
#include <Urho3D/Graphics/AnimationController.h>
#include <Urho3D/IO/MemoryBuffer.h>
#include <Urho3D/Physics/PhysicsEvents.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/Scene/SceneEvents.h>


#include <Urho3D/Resource/ResourceCache.h>


#include "Character.h"
#include "Platform.h"

#include <iostream>

Character::Character(Context* context) :
    LogicComponent(context),
    onGround_(false),
    okToJump_(true),
    inAirTimer_(0.0f),
    onPlatform_(false),
    transform_(Vector3(0,0,0)),
    contactTransform_(Vector3(0,0,0)),
    platformTransform_(Vector3(0,0,0)),
    currentTransform_(Vector3(0,0,0)),
    switchTransform_(true)
{
    // Only the physics update event is needed: unsubscribe from the rest for optimization
    SetUpdateEventMask(USE_FIXEDUPDATE);
}

void Character::RegisterObject(Context* context)
{
    context->RegisterFactory<Character>();

}

void Character::Start()
{
    // Component has been inserted into its scene node. Subscribe to events now
    SubscribeToEvent(GetNode(), E_NODECOLLISION, HANDLER(Character, HandleNodeCollision));
    SubscribeToEvent(GetNode(), E_NODECOLLISIONSTART, HANDLER(Character, HandleNodeCollisionStart));
    SubscribeToEvent(GetNode(), E_NODECOLLISIONEND, HANDLER(Character, HandleNodeCollisionEnd));
    
    CreateSphere(Urho3D::Vector3(0,0,0));
}

void Character::FixedUpdate(float timeStep)
{
    /// \todo Could cache the components for faster access instead of finding them each frame
    RigidBody* body = GetComponent<RigidBody>();

    // Update the in air timer. Reset if grounded
    if (!onGround_)
        inAirTimer_ += timeStep;
    else
        inAirTimer_ = 0.0f;
    // When character has been in air less than 1/10 second, it's still interpreted as being on ground
    bool softGrounded = inAirTimer_ < INAIR_THRESHOLD_TIME;

    // Update movement & animation
    const Quaternion& rot = node_->GetRotation();
    Vector3 moveDir = Vector3::ZERO;
    const Vector3& velocity = body->GetLinearVelocity();
    // Velocity on the XZ plane
    Vector3 planeVelocity(velocity.x_, 0.0f, velocity.z_);

    if (controls_.IsDown(CTRL_FORWARD))
        moveDir += Vector3::FORWARD;
    if (controls_.IsDown(CTRL_BACK))
        moveDir += Vector3::BACK;
    if (controls_.IsDown(CTRL_LEFT))
        moveDir += Vector3::LEFT;
    if (controls_.IsDown(CTRL_RIGHT))
        moveDir += Vector3::RIGHT;

    // Normalize move vector so that diagonal strafing is not faster
    if (moveDir.LengthSquared() > 0.0f)
        moveDir.Normalize();

    // If in air, allow control, but slower than when on ground
    body->ApplyImpulse(rot * moveDir * (softGrounded ? MOVE_FORCE : INAIR_MOVE_FORCE));

    if (softGrounded)
    {
        // When on ground, apply a braking force to limit maximum ground velocity
        Vector3 brakeForce = -planeVelocity * BRAKE_FORCE;
        body->ApplyImpulse(brakeForce);

        // Jump. Must release jump control inbetween jumps
        if (controls_.IsDown(CTRL_JUMP))
        {
            if (okToJump_)
            {
                body->ApplyImpulse(Vector3::UP * JUMP_FORCE);
                okToJump_ = false;
            }
        }
        else
            okToJump_ = true;
    }

    
    if(onPlatform_)
    {
        if(switchTransform_)
        {
            Vector3 diff = currentTransform_ - platformTransform_  ;
            Vector3 combine =  diff + otherBody_->GetWorldPosition();
            
            node_->GetComponent<RigidBody>()->SetFriction(0);
            node_->GetComponent<RigidBody>()->SetRestitution(0);
            node_->SetWorldPosition(combine);
            
            testSphere_->SetWorldPosition(combine);
        }
        else
        {
            node_->SetParent(otherBody_);
        }
        
    }

    
    // Reset grounded flag for next frame
    onGround_ = false;
    
    //onPlatform_ = false;
    
}

void Character::HandleNodeCollision(StringHash eventType, VariantMap& eventData)
{
    using namespace NodeCollision;
    
    Node* otherNode = (Node*)eventData[P_OTHERNODE].GetPtr();
    
    MemoryBuffer contacts(eventData[P_CONTACTS].GetBuffer());
    
    while (!contacts.IsEof())
    {
        Vector3 contactPosition = contacts.ReadVector3();
        Vector3 contactNormal = contacts.ReadVector3();
        /*float contactDistance = */contacts.ReadFloat();
        /*float contactImpulse = */contacts.ReadFloat();
        
        // If contact is below node center and mostly vertical, assume it's a ground contact
        if (contactPosition.y_ < (node_->GetPosition().y_ + 1.0f))
        {
            float level = Abs(contactNormal.y_);
            if (level > 0.75)
                onGround_ = true;
        }
        
        
    }
}

void Character::HandleNodeCollisionStart(StringHash eventType, VariantMap& eventData)
{
    // Check collision contacts and see if character is standing on ground (look for a contact that has near vertical normal)
    using namespace NodeCollision;
    
    Node* otherNode = (Node*)eventData[P_OTHERNODE].GetPtr();
    
    Platform *platform = otherNode->GetComponent<Platform>();
    
    
    
    MemoryBuffer contacts(eventData[P_CONTACTS].GetBuffer());
    
    
    while (!contacts.IsEof())
    {
        Vector3 contactPosition = contacts.ReadVector3();
        Vector3 contactNormal = contacts.ReadVector3();
        /*float contactDistance = */contacts.ReadFloat();
        /*float contactImpulse = */contacts.ReadFloat();
        
        
        if (platform)
        {
            if (!onPlatform_)
            {
                if(contactNormal.DotProduct(Vector3(0,1.0,0))==1.0)
                {
                    
                    otherBody_ = otherNode;
                    contactTransform_ = contactPosition;
                    platformTransform_ = otherNode->GetWorldPosition();
                    currentTransform_ = node_->GetWorldPosition();
                    
                    
                    onPlatform_ = true;
                }
                
            }
            
        }
    }
    
}

void Character::HandleNodeCollisionEnd(StringHash eventType, VariantMap& eventData)
{
    onPlatform_ = false;
}

void Character::CreateSphere(Urho3D::Vector3 position)
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();
    
    testSphere_ = GetScene()->CreateChild("SmallBox2");
    
    
    testSphere_->SetScale(0.25f);
    
    StaticModel* sphereObject = testSphere_->CreateComponent<StaticModel>();
    
    sphereObject->SetModel(cache->GetResource<Model>("Models/Sphere.mdl"));
    
    sphereObject->SetMaterial(cache->GetResource<Material>("Materials/Editor/RedUnlit.xml"));
    
    sphereObject->SetCastShadows(false);

}
