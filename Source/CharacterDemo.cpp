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

#include <Urho3D/Core/CoreEvents.h>
#include <Urho3D/Core/ProcessUtils.h>
#include <Urho3D/Engine/Engine.h>
#include <Urho3D/Graphics/AnimatedModel.h>
#include <Urho3D/Graphics/AnimationController.h>
#include <Urho3D/Graphics/Camera.h>
#include <Urho3D/Graphics/Light.h>
#include <Urho3D/Graphics/Material.h>
#include <Urho3D/Graphics/Octree.h>
#include <Urho3D/Graphics/Renderer.h>
#include <Urho3D/Graphics/Zone.h>
#include <Urho3D/Input/Controls.h>
#include <Urho3D/Input/Input.h>
#include <Urho3D/IO/FileSystem.h>
#include <Urho3D/Physics/CollisionShape.h>
#include <Urho3D/Physics/PhysicsWorld.h>
#include <Urho3D/Physics/RigidBody.h>
#include <Urho3D/Resource/ResourceCache.h>
#include <Urho3D/Scene/Scene.h>
#include <Urho3D/UI/Font.h>
#include <Urho3D/UI/Text.h>
#include <Urho3D/UI/UI.h>

#include "Character.h"
#include "CharacterDemo.h"

#include <Urho3D/DebugNew.h>
#include <Urho3D/Graphics/DebugRenderer.h>

#include "Platform.h"

DEFINE_APPLICATION_MAIN(CharacterDemo)

CharacterDemo::CharacterDemo(Context* context) :
    Sample(context)
{
    // Register factory and attributes for the Character component so it can be created via CreateComponent, and loaded / saved
    Character::RegisterObject(context);
    context->RegisterFactory<Platform>();
}

CharacterDemo::~CharacterDemo()
{
}

void CharacterDemo::Start()
{
    // Execute base class startup
    Sample::Start();

    // Create static scene content
    CreateScene();
    // Create the controllable character
    CreateCharacter();

    // Subscribe to necessary events
    SubscribeToEvents();
}

void CharacterDemo::CreateScene()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();

    scene_ = new Scene(context_);

    // Create scene subsystem components
    scene_->CreateComponent<Octree>();
    scene_->CreateComponent<PhysicsWorld>();
    scene_->CreateComponent<DebugRenderer>();

    // Create camera and define viewport. We will be doing load / save, so it's convenient to create the camera outside the scene,
    // so that it won't be destroyed and recreated, and we don't have to redefine the viewport on load
    cameraNode_ = new Node(context_);
    Camera* camera = cameraNode_->CreateComponent<Camera>();
    camera->SetFarClip(300.0f);
    GetSubsystem<Renderer>()->SetViewport(0, new Viewport(context_, scene_, camera));

    // Create static scene content. First create a zone for ambient lighting and fog control
    Node* zoneNode = scene_->CreateChild("Zone");
    Zone* zone = zoneNode->CreateComponent<Zone>();
    zone->SetAmbientColor(Color(0.15f, 0.15f, 0.15f));
    zone->SetFogColor(Color(0.5f, 0.5f, 0.7f));
    zone->SetFogStart(100.0f);
    zone->SetFogEnd(300.0f);
    zone->SetBoundingBox(BoundingBox(-1000.0f, 1000.0f));

    // Create a directional light with cascaded shadow mapping
    Node* lightNode = scene_->CreateChild("DirectionalLight");
    lightNode->SetDirection(Vector3(0.3f, -0.5f, 0.425f));
    Light* light = lightNode->CreateComponent<Light>();
    light->SetLightType(LIGHT_DIRECTIONAL);
    light->SetCastShadows(true);
    light->SetShadowBias(BiasParameters(0.00025f, 0.5f));
    light->SetShadowCascade(CascadeParameters(10.0f, 50.0f, 200.0f, 0.0f, 0.8f));
    light->SetSpecularIntensity(0.5f);

    // Create the floor object
    Node* floorNode = scene_->CreateChild("Floor");
    floorNode->SetPosition(Vector3(0.0f, -0.5f, 0.0f));
    floorNode->SetScale(Vector3(200.0f, 1.0f, 200.0f));
    StaticModel* object = floorNode->CreateComponent<StaticModel>();
    object->SetModel(cache->GetResource<Model>("Models/Box.mdl"));
    object->SetMaterial(cache->GetResource<Material>("Materials/Stone.xml"));

    RigidBody* body = floorNode->CreateComponent<RigidBody>();
    // Use collision layer bit 2 to mark world scenery. This is what we will raycast against to prevent camera from going
    // inside geometry
    body->SetCollisionLayer(2);
    CollisionShape* shape = floorNode->CreateComponent<CollisionShape>();
    shape->SetBox(Vector3::ONE);

    // Create Platforms of varying sizes
    const unsigned NUM_PLATFORMS = 60;
    for (unsigned i = 0; i < NUM_PLATFORMS; ++i)
    {
        Node* objectNode = scene_->CreateChild("Platform");
        objectNode->SetPosition(Vector3(Random(-10.0f,10.0f), 0.0f, i*4.0f));
        //objectNode->SetRotation(Quaternion(0.0f, Random(360.0f), 0.0f));
        //objectNode->SetScale(2.0f + Random(5.0f));
        objectNode->SetScale(Vector3(40,1,3));
        StaticModel* object = objectNode->CreateComponent<StaticModel>();
        object->SetModel(cache->GetResource<Model>("Models/box.mdl"));
        object->SetMaterial(cache->GetResource<Material>("Materials/Jack.xml"));
        object->SetCastShadows(true);

        RigidBody* body = objectNode->CreateComponent<RigidBody>();
        body->SetCollisionLayer(2);
        CollisionShape* shape = objectNode->CreateComponent<CollisionShape>();
        shape->SetBox(Vector3::ONE);
        
        body->SetFriction(1.0f);
        
        objectNode->CreateComponent<Platform>();
        
        objectNode->GetComponent<Platform>()->SetId(i);
        
        if(i%2)
        {
            body->SetKinematic(true);
        }
        
    }


}

void CharacterDemo::CreateCharacter()
{
    ResourceCache* cache = GetSubsystem<ResourceCache>();

    Node* objectNode = scene_->CreateChild("Jack");
    objectNode->SetPosition(Vector3(0.0f, 2.0f, 0.0f));
    objectNode->SetScale(Vector3(1.0,2.0,1.0));

    // Create the rendering component + animation controller
    StaticModel* object = objectNode->CreateComponent<StaticModel>();
    object->SetModel(cache->GetResource<Model>("Models/box.mdl"));
    //object->SetModel(cache->GetResource<Model>("Models/Jack.mdl"));
    object->SetMaterial(cache->GetResource<Material>("Materials/Jack.xml"));
    object->SetCastShadows(true);
    //objectNode->CreateComponent<AnimationController>();

    // Set the head bone for manual control
    //object->GetSkeleton().GetBone("Bip01_Head")->animated_ = false;

    // Create rigidbody, and set non-zero mass so that the body becomes dynamic
    RigidBody* body = objectNode->CreateComponent<RigidBody>();
    body->SetCollisionLayer(1);
    body->SetMass(1.0f);
    body->SetFriction(1.0f);

    // Set zero angular factor so that physics doesn't turn the character on its own.
    // Instead we will control the character yaw manually
    body->SetAngularFactor(Vector3::ZERO);

    // Set the rigidbody to signal collision also when in rest, so that we get ground collisions properly
    body->SetCollisionEventMode(COLLISION_ALWAYS);

    // Set a capsule shape for collision
    CollisionShape* shape = objectNode->CreateComponent<CollisionShape>();
    shape->SetBox(object->GetBoundingBox().Size());
    //shape->SetCapsule(0.7f, 1.8f, Vector3(0.0f, 0.9f, 0.0f));

    // Create the character logic component, which takes care of steering the rigidbody
    // Remember it so that we can set the controls. Use a WeakPtr because the scene hierarchy already owns it
    // and keeps it alive as long as it's not removed from the hierarchy
    character_ = objectNode->CreateComponent<Character>();
}



void CharacterDemo::SubscribeToEvents()
{
    
    SubscribeToEvent(E_POSTRENDERUPDATE, HANDLER(CharacterDemo, HandlePostRenderUpdate));
    // Subscribe to Update event for setting the character controls before physics simulation
    SubscribeToEvent(E_UPDATE, HANDLER(CharacterDemo, HandleUpdate));

    // Subscribe to PostUpdate event for updating the camera position after physics simulation
    SubscribeToEvent(E_POSTUPDATE, HANDLER(CharacterDemo, HandlePostUpdate));
    


    // Unsubscribe the SceneUpdate event from base class as the camera node is being controlled in HandlePostUpdate() in this sample
    UnsubscribeFromEvent(E_SCENEUPDATE);
}

void CharacterDemo::HandleUpdate(StringHash eventType, VariantMap& eventData)
{
    using namespace Update;

    Input* input = GetSubsystem<Input>();

    if (character_)
    {
        // Clear previous controls
        character_->controls_.Set(CTRL_FORWARD | CTRL_BACK | CTRL_LEFT | CTRL_RIGHT | CTRL_JUMP, false);

    

        // Update controls using keys
        UI* ui = GetSubsystem<UI>();
        if (!ui->GetFocusElement())
        {
           
            character_->controls_.Set(CTRL_FORWARD, input->GetKeyDown('W'));
            character_->controls_.Set(CTRL_BACK, input->GetKeyDown('S'));
            character_->controls_.Set(CTRL_LEFT, input->GetKeyDown('A'));
            character_->controls_.Set(CTRL_RIGHT, input->GetKeyDown('D'));
        
            character_->controls_.Set(CTRL_JUMP, input->GetKeyDown(KEY_SPACE));

            // Add character yaw & pitch from the mouse motion or touch input
           
            character_->controls_.yaw_ += (float)input->GetMouseMoveX() * YAW_SENSITIVITY;
            character_->controls_.pitch_ += (float)input->GetMouseMoveY() * YAW_SENSITIVITY;
 
            // Limit pitch
            character_->controls_.pitch_ = Clamp(character_->controls_.pitch_, -80.0f, 80.0f);
         
        }

        // Set rotation already here so that it's updated every rendering frame instead of every physics frame
        character_->GetNode()->SetRotation(Quaternion(character_->controls_.yaw_, Vector3::UP));
    }
}

void CharacterDemo::HandlePostUpdate(StringHash eventType, VariantMap& eventData)
{
    if (!character_)
        return;

    Node* characterNode = character_->GetNode();

    // Get camera lookat dir from character yaw + pitch
    Quaternion rot = characterNode->GetRotation();
    Quaternion dir = rot * Quaternion(character_->controls_.pitch_, Vector3::RIGHT);


    // Third person camera: position behind the character
    Vector3 aimPoint = characterNode->GetPosition() + rot * Vector3(0.0f, 1.7f, 0.0f);

    // Collide camera ray with static physics objects (layer bitmask 2) to ensure we see the character properly
    Vector3 rayDir = dir * Vector3::BACK;

    cameraNode_->SetPosition(aimPoint + rayDir * 10);
    cameraNode_->SetRotation(dir);

}

void CharacterDemo::HandlePostRenderUpdate(StringHash eventType, VariantMap& eventData)
{

    scene_->GetComponent<PhysicsWorld>()->DrawDebugGeometry(true);
    
    
}
